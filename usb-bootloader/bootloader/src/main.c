#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/boot.h>

#include <avr/pgmspace.h>
#include "usbdrv.h"

#include "bootloader/common/types.h"
#include "bootloader/common/crc8.h"
#include "bootloader/common/utils.h"
#include "bootloader/common/protocol.h"
#include "bootloader/common/debug.h"

// Uncomment to have hardware debug
//#define DEBUG_LED

#define BOOTLOADER_VERSION_MAJOR 0x00
#define BOOTLOADER_VERSION_MINOR 0x03

#define BOOTLOADER_SIZE_IN_PAGES (((_U16) FLASHEND + 1 - BOOTLOADER_SECTION_START_ADDRESS) / SPM_PAGESIZE)

#define BOOTLOADER_APPLICATION_PAGES_COUNT ((((_U16) (FLASHEND + 1) / SPM_PAGESIZE) - BOOTLOADER_SIZE_IN_PAGES))

#define BOOTLOADER_BYTE_APP_CRC8     (FLASHEND - BOOTLOADER_SIZE_IN_PAGES * SPM_PAGESIZE)
#define BOOTLOADER_BYTE_APP_CRC8_INV (FLASHEND - BOOTLOADER_SIZE_IN_PAGES * SPM_PAGESIZE - 1)

#define BOOTLOADER_ACTIVATION_PIO_BANK B
#define BOOTLOADER_ACTIVATION_PIO_PIN  0


typedef enum _BootloaderState {
	BOOTLOADER_STATE_IDLE,
	BOOTLOADER_STATE_PAGE_READ,
	BOOTLOADER_STATE_PAGE_WRITE,
	BOOTLOADER_STATE_RESET,
} BootloaderState;


static _U8 responseBuffer[8] = { 0 };

static BootloaderState bootloaderState = BOOTLOADER_STATE_IDLE;
static _U16            currentAddress  = 0;
static _U16            dataSize        = 0;

void __reset(void) {
	__asm__ __volatile__ ("rjmp __init2 \n\t"::);
}


static void __init2(void) __attribute__ ((section( ".init2" ), naked, used));
static void __init2(void) {
	// Initialize stack, r0
	__asm__ __volatile__ (
		"out __SP_L__, %A0" "\n\t"
		"out __SP_H__, %B0" "\n\t"
		"clr __zero_reg__"  "\n\t"
		"out __SREG__, __zero_reg__" "\n\t"
        : : "r" ((_U16)(RAMEND))
	);
}


static void __init3(void) __attribute__ ((section( ".init3" ), naked, used));
static void __init3(void) {
	// Disable watchdog
	wdt_reset();

	// application software should always clear the Watchdog System Reset Flag
	// (WDRF) and the WDE control bit in the initialisation routine, even if the Watchdog is not in use
	{
		// WDE is overridden by WDRF in MCUSR. This means that WDE is always set when WDRF is
		// set. To clear WDE, WDRF must be cleared first. This feature ensures multiple resets during conditions
		// causing failure, and a safe start-up after the failure.
		CLEAR_BIT_AT(MCUSR, WDRF);

		wdt_disable();
	}
}


static void __init9(void) __attribute__ ((section( ".init9" ), naked, used));
static void __init9(void) {
	// Jump to main
	__asm__ __volatile__ (
		"rjmp main" "\n\t"
		::
	);
}


static _U8 _e2promRead(_U16 address) {
	// Wait for completion of previous write
	while (CHECK_BIT_AT(EECR, EEPE));

	// Set up address register
	EEAR = address;

	// Start E2PROM read
	SET_BIT_AT(EECR, EERE);

	return EEDR;
}


static void _e2promWrite(_U16 address, _U8 value) {
	while (CHECK_BIT_AT(EECR, EEPE));

	EEAR = address;
	EEDR = value;

	// Write logical one to EEMPE
	SET_BIT_AT(EECR, EEMPE);

	// Start eeprom write by setting EEPE
	SET_BIT_AT(EECR, EEPE);
}


usbMsgLen_t usbFunctionSetup(uchar data[8]) {
	usbMsgLen_t ret = 0;

	{
		usbRequest_t *request = (void *) data;
		_U16 wIndex = request->wIndex.word;

		DBG(("SETUP"));

		if ((request->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_VENDOR){
			DBG(("Vendor"));

			if ((request->bmRequestType & USBRQ_DIR_MASK) == USBRQ_DIR_HOST_TO_DEVICE) {
				DBG(("Write"));

				switch (request->bRequest) {
					case BOOTLOADER_COMMON_COMMAND_FLASH_WRITE_PAGE:
						{
							DBG(("WPAG"));

							if (wIndex >= BOOTLOADER_APPLICATION_PAGES_COUNT) {
								responseBuffer[ret++] = BOOTLOADER_COMMON_COMMAND_STATUS_ERROR;

								break;
							}

							bootloaderState = BOOTLOADER_STATE_PAGE_WRITE;

							currentAddress  = wIndex * SPM_PAGESIZE;
							dataSize        = SPM_PAGESIZE;

							// Multiple write
							ret = 0xff;
						}
						break;

					default:
						break;
				}

			} else {
				DBG(("Read"));

				switch (request->bRequest) {
					case BOOTLOADER_COMMON_COMMAND_CONNECT:
						{
							DBG(("CONN"));

							responseBuffer[ret++] = BOOTLOADER_COMMON_COMMAND_STATUS_OK;
						}
						break;

					case BOOTLOADER_COMMON_COMMAND_GET_INFO:
						{
							DBG(("GETI"));

							responseBuffer[ret + 0] = BOOTLOADER_COMMON_COMMAND_STATUS_OK;
							// Version
							responseBuffer[ret + 1] = BOOTLOADER_VERSION_MAJOR;
							responseBuffer[ret + 2] = BOOTLOADER_VERSION_MINOR;
							// Size in pages of boot area
							responseBuffer[ret + 3] = BOOTLOADER_SIZE_IN_PAGES;

							responseBuffer[ret + 4] = SIGNATURE_0;
							responseBuffer[ret + 5] = SIGNATURE_1;
							responseBuffer[ret + 6] = SIGNATURE_2;

							ret = 7;
						}
						break;

					case BOOTLOADER_COMMON_COMMAND_FLASH_READ_PAGE:
						{
							DBG(("RPAG"));

							if (wIndex >= BOOTLOADER_APPLICATION_PAGES_COUNT) {
								responseBuffer[ret++] = BOOTLOADER_COMMON_COMMAND_STATUS_ERROR;

								break;
							}

							bootloaderState = BOOTLOADER_STATE_PAGE_READ;

							currentAddress  = wIndex * SPM_PAGESIZE;
							dataSize        = SPM_PAGESIZE;

							// Multiple read
							ret = 0xff;
						}
						break;

					case BOOTLOADER_COMMON_COMMAND_FLASH_ERASE_PAGE:
						{
							DBG(("EPAG"));

							if (wIndex >= BOOTLOADER_APPLICATION_PAGES_COUNT) {
								responseBuffer[ret++] = BOOTLOADER_COMMON_COMMAND_STATUS_ERROR;

								break;
							}

							boot_spm_busy_wait();
							boot_page_erase(wIndex * SPM_PAGESIZE);

							responseBuffer[ret++] = BOOTLOADER_COMMON_COMMAND_STATUS_OK;
						}
						break;

					case BOOTLOADER_COMMON_COMMAND_E2PROM_READ:
						{
							DBG(("EREA"));

							responseBuffer[ret + 0] = BOOTLOADER_COMMON_COMMAND_STATUS_OK;
							responseBuffer[ret + 1] = _e2promRead(wIndex);

							ret = 2;
						}
						break;

					case BOOTLOADER_COMMON_COMMAND_E2PROM_WRITE:
						{
							DBG(("EWRA"));

							_e2promWrite(wIndex, request->wValue.word);

							responseBuffer[ret++] = BOOTLOADER_COMMON_COMMAND_STATUS_OK;
						}
						break;

					case BOOTLOADER_COMMON_COMMAND_REBOOT:
						{
							responseBuffer[ret++] = BOOTLOADER_COMMON_COMMAND_STATUS_OK;

							bootloaderState = BOOTLOADER_STATE_RESET;
						}
						break;

					default:
						break;
				}
			}

		}else{

		}
	}

	usbMsgPtr = (usbMsgPtr_t) responseBuffer;

    return ret;
}


uchar usbFunctionRead(uchar *data, uchar len) {
	_U8 ret = 0;

	DBG(("read"));

	if (bootloaderState == BOOTLOADER_STATE_PAGE_READ) {
		while (ret < len) {
			data[ret] = pgm_read_byte(currentAddress++);

			ret      += 1;
			dataSize -= 1;

			if (dataSize == 0) {
				bootloaderState = BOOTLOADER_STATE_IDLE;

				break;
			}
		}
	}

    return ret;
}


uchar usbFunctionWrite(uchar *data, uchar len) {
	_U8 ret = 0;

	DBG(("write"));

	if (bootloaderState == BOOTLOADER_STATE_PAGE_WRITE) {
		_U8 idx  = 0;
		_U8 addr = SPM_PAGESIZE - dataSize;

		while (idx < len) {
			// Address is incremented by 1 per one data word (2 bytes)
			DBG(("FP"));

			boot_spm_busy_wait();
			boot_page_fill(addr, (_U16) data[idx] | (_U16) (data[idx + 1] << 8));

			addr     += 2;
			dataSize -= 2;
			idx      += 2;

			if (dataSize == 0) {
				break;
			}
		}

		if (dataSize == 0) {
			DBG(("PC"));

			boot_spm_busy_wait();
			boot_page_write(currentAddress);

			boot_spm_busy_wait();
			boot_rww_enable();

			bootloaderState = BOOTLOADER_STATE_IDLE;

			ret = 1;
		}
	}

	return ret;
}

/* ------------------------------------------------------------------------- */

__attribute__((OS_main)) int main(void) {
#if defined(DEBUG)
	debug_initialize();
#endif

#if defined(DEBUG_LED)
	SET_PIO_AS_OUTPUT(DDRB, 1);
	SET_PIO_LOW(PORTB, 1);
	_delay_ms(500);
	SET_PIO_HIGH(PORTB, 1);
#endif

	DBG(("C"));

	_BOOL imageInFlashIsValid = FALSE;

	{
		_U16 addr         = 0x0000;
		_U8 imageChecksum = 0;

		while (addr < BOOTLOADER_BYTE_APP_CRC8_INV) {
			imageChecksum = crc8_getForByte(pgm_read_byte(addr), IMAGE_CHECKSUM_POLYNOMIAL, imageChecksum);

			addr += 1;
		}

		DBG(("D"));

		{
			_U8 flashChecksum    = pgm_read_byte(BOOTLOADER_BYTE_APP_CRC8);
			_U8 flashChecksumInv = pgm_read_byte(BOOTLOADER_BYTE_APP_CRC8_INV);

			DBG(("E"));

			if ((imageChecksum == flashChecksum) && (~imageChecksum == flashChecksumInv)) {
				DBG(("CK OK"));

				imageInFlashIsValid = TRUE;

			} else {
				DBG(("CK NOK"));
			}
		}
	}

	SET_PIO_AS_INPUT(DECLARE_DDR(BOOTLOADER_ACTIVATION_PIO_BANK), BOOTLOADER_ACTIVATION_PIO_PIN);

	if (
		! PIO_IS_LOW(DECLARE_PIN(BOOTLOADER_ACTIVATION_PIO_BANK), BOOTLOADER_ACTIVATION_PIO_PIN) &&
		imageInFlashIsValid
	) {
		void (*entryPoint)() = 0x0000;

		entryPoint();
	}

	// Enable watchdog with 1s timer
	wdt_enable(WDTO_1S);

	DBG(("START"));

	// Move vectors to bootloader
	{
		// Enable change of Interrupt Vectors
		MCUCR = _BV(IVCE);
		// Move interrupts to Boot Flash section
		MCUCR = _BV(IVSEL);
	}

	// Initialize V-USB stack
    usbInit();

    // enforce re-enumeration, do this while interrupts are disabled!
    usbDeviceDisconnect();
    {
    	_U8 i = 255;

    	// fake USB disconnect for > 250 ms
        while (i) {
            wdt_reset();
            _delay_ms(1);

            i -= 1;
        }
    }
    usbDeviceConnect();

    sei();

    while (bootloaderState != BOOTLOADER_STATE_RESET) {
        // main event loop
        wdt_reset();

        usbPoll();
    }

	DBG(("REBO"));

	// Perform MCU reset
	{
		// Disable interrupts
		cli();

		// Disconnect USB
		usbDeviceDisconnect();

		// Disable VUSB interrupt vector
		CLEAR_BIT_AT(USB_INTR_ENABLE, USB_INTR_ENABLE_BIT);

		// Move interrupt vectors to 0x0000
		{
			/// Enable change of Interrupt Vectors
			SET_BIT_AT(MCUCR, IVCE);

			// Move interrupts to Main section
			CLEAR_BIT_AT(MCUCR, IVSEL);
		}
#if defined(DEBUG)
		debug_terminate();
#endif

		wdt_enable(WDTO_15MS);

		while (1) {
			__asm__ __volatile ("nop");
		}
	}

    return 0;
}
