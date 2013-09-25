#include <usb.h>
#include <string.h>
#include <unistd.h>

#include "bootloader/common/protocol.h"
#include "burner/bootloader.h"

#define DEBUG_LEVEL 4
#include "burner/common/debug.h"


#define BOOTLOADER_CHECK_DEVICES_INTERVAL 200


typedef struct _McuParameters {
	struct {
		_U8 byte1;
		_U8 byte2;
		_U8 byte3;
	} id;

	char *name;

	struct {
		_U32 size;
		_U32 pageSize;
	} flash;

	struct {
		_U32 size;
	} e2prom;
} McuParameters;


typedef struct _McuInformation {
	struct {
		_U8 major;
		_U8 minor;
	} bootloaderVersion;

	struct {
		_U8 byte1;
		_U8 byte2;
		_U8 byte3;
	} signature;

	_U16 bootloaderSizeInPages;
} McuInformation;


typedef struct _BootloaderPrivateData {
	usb_dev_handle *deviceHandle;
	McuParameters  *mcuParameters;
	_U32            bootloaderSectionSize;
} BootloaderPrivateData;


static _BOOL initialized = FALSE;

BootloaderPrivateData privateData;

static const _U16 idVendor  = 0x16c0;
static const _U16 idProduct = 0x05dc;

static const char *deviceName = "USB jboot";
static const char *vendorName = "obdev.at";

static McuParameters mcu[] = {
	{
		.id    = { 0x1e, 0x95, 0x0f },
		.name  = "ATmega328P",
		.flash = {
			.size     = 32 * 1024,
			.pageSize = 128,
		},
		.e2prom = {
			.size = 2 * 1024,
		}
	}
};


static CommonError _mcuCommandConnect(_U32 timeout) {
	CommonError ret = COMMON_NO_ERROR;

	do {
		_S32 usbRet;
		_U8  response;

		usbRet = usb_control_msg(
			privateData.deviceHandle,
			USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN,
			BOOTLOADER_COMMON_COMMAND_CONNECT,
			0,
			0,
			&response,
			1,
			timeout
		);
		if (usbRet < 0) {
			ERR(("_mcuCommandConnect(): USB error '%s'!", usb_strerror()));

			ret = COMMON_ERROR;
			break;
		}

		if (usbRet != 1) {
			ERR(("_mcuCommandConnect(): Bad response length! (%d)", usbRet));

			ret = COMMON_ERROR_BAD_PARAMETER;
			break;
		}

		if (response != BOOTLOADER_COMMON_COMMAND_STATUS_OK) {
			ERR(("_mcuCommandConnect(): Bad response! %#x", response));

			ret = COMMON_ERROR_BAD_PARAMETER;
			break;
		}
	} while (0);

	return ret;
}


static CommonError _mcuCommandGetInfo(McuInformation *info, _U32 timeout) {
	CommonError ret = COMMON_NO_ERROR;

	do {
		_S32 usbRet;
		_U8  response[8] = { 0 };

		usbRet = usb_control_msg(
			privateData.deviceHandle,
			USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN,
			BOOTLOADER_COMMON_COMMAND_GET_INFO,
			0,
			0,
			response,
			sizeof(response),
			timeout
		);
		if (usbRet < 0) {
			ERR(("_mcuCommandGetInfo(): USB error '%s'!", usb_strerror()));

			ret = COMMON_ERROR;
			break;
		}

		if (usbRet != 7) {
			ERR(("_mcuCommandGetInfo(): Bad response!"));

			ret = COMMON_ERROR_BAD_PARAMETER;
			break;
		}

		if (response[0] != BOOTLOADER_COMMON_COMMAND_STATUS_OK) {
			ERR(("_mcuCommandGetInfo(): Bad response! %#x", response[0]));

			ret = COMMON_ERROR_BAD_PARAMETER;
			break;
		}

		info->bootloaderVersion.major = response[1];
		info->bootloaderVersion.minor = response[2];

		info->bootloaderSizeInPages   = response[3];

		info->signature.byte1         = response[4];
		info->signature.byte2         = response[5];
		info->signature.byte3         = response[6];

		DBG(("_mcuCommandGetInfo(): Bootloader version: %d.%d, sizeInPages: %d, signature: %02x %02x %02x",
			info->bootloaderVersion.major, info->bootloaderVersion.minor, info->bootloaderSizeInPages,
			info->signature.byte1, info->signature.byte2, info->signature.byte3
		));
	} while (0);

	return ret;
}


static CommonError _mcuCommandReboot(_U32 timeout) {
	CommonError ret = COMMON_NO_ERROR;

	do {
		_S32 usbRet;
		_U8 response;

		usbRet = usb_control_msg(
			privateData.deviceHandle,
			USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN,
			BOOTLOADER_COMMON_COMMAND_REBOOT,
			0,
			0,
			&response,
			1,
			timeout
		);
		if (usbRet < 0) {
			ERR(("_mcuCommandReboot(): USB error '%s'!", usb_strerror()));

			ret = COMMON_ERROR;
			break;
		}

		if (usbRet != 1) {
			ERR(("_mcuCommandReboot(): Bad response!"));

			ret = COMMON_ERROR_BAD_PARAMETER;
			break;
		}

		if (response != BOOTLOADER_COMMON_COMMAND_STATUS_OK) {
			ERR(("_mcuCommandReboot(): Bad response! %#x", response));

			ret = COMMON_ERROR_BAD_PARAMETER;
			break;
		}
	} while (0);

	return ret;
}


static CommonError _mcuCommandFlashPageErase(_U32 pageNumber, _U32 timeout) {
	CommonError ret = COMMON_NO_ERROR;

	do {
		_S32 usbRet;
		_U8  response;

		usbRet = usb_control_msg(
			privateData.deviceHandle,
			USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN,
			BOOTLOADER_COMMON_COMMAND_FLASH_ERASE_PAGE,
			0,
			pageNumber,
			&response,
			1,
			timeout
		);
		if (usbRet < 0) {
			ERR(("_mcuCommandFlashPageErase(): USB error '%s'!", usb_strerror()));

			ret = COMMON_ERROR;
			break;
		}

		if (usbRet != 1) {
			ERR(("_mcuCommandFlashPageErase(): Bad response!"));

			ret = COMMON_ERROR_BAD_PARAMETER;
			break;
		}

		if (response != BOOTLOADER_COMMON_COMMAND_STATUS_OK) {
			ERR(("_mcuCommandFlashPageErase(): Bad response! %#x", response));

			ret = COMMON_ERROR_BAD_PARAMETER;
			break;
		}
	} while (0);

	return ret;
}


static CommonError _mcuCommandFlashPageRead(_U32 pageNumber, _U8 *buffer, _U32 bufferSize, _U32 timeout) {
	CommonError ret = COMMON_NO_ERROR;

	do {
		_S32 usbRet;

		usbRet = usb_control_msg(
			privateData.deviceHandle,
			USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN,
			BOOTLOADER_COMMON_COMMAND_FLASH_READ_PAGE,
			0,
			pageNumber,
			buffer,
			bufferSize,
			timeout
		);
		if (usbRet < 0) {
			ERR(("_mcuCommandFlashPageRead(): USB error '%s'!", usb_strerror()));

			ret = COMMON_ERROR;
			break;
		}

		if (usbRet != bufferSize) {
			ERR(("_mcuCommandFlashPageRead(): Bad response!, %d", usbRet));

			ret = COMMON_ERROR_BAD_PARAMETER;
			break;
		}
	} while (0);

	return ret;
}


static CommonError _mcuCommandFlashPageWrite(_U32 pageNumber, _U8 *buffer, _U32 bufferSize, _U32 timeout) {
	CommonError ret = COMMON_NO_ERROR;

	do {
		_S32 usbRet;
		_U8  response;

		usbRet = usb_control_msg(
			privateData.deviceHandle,
			USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_OUT,
			BOOTLOADER_COMMON_COMMAND_FLASH_WRITE_PAGE,
			0,
			pageNumber,
			buffer,
			bufferSize,
			timeout
		);
		if (usbRet < 0) {
			ERR(("_mcuCommandFlashPageWrite(): USB error '%s'!", usb_strerror()));

			ret = COMMON_ERROR;
			break;
		}

		if (usbRet != bufferSize) {
			ERR(("_mcuCommandFlashPageWrite(): Bad response! %d", usbRet));

			ret = COMMON_ERROR_BAD_PARAMETER;
			break;
		}
	} while (0);

	return ret;
}


static CommonError _mcuCommandE2PromRead(_U32 offset, _U8 *buffer, _U32 bufferSize, _U32 timeout) {
	CommonError ret = COMMON_NO_ERROR;

	do {
		_U32 i;

		for (i = 0; i < bufferSize; i++) {
			_S32 usbRet;
			_U8  response[2];

			usbRet = usb_control_msg(
				privateData.deviceHandle,
				USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN,
				BOOTLOADER_COMMON_COMMAND_E2PROM_READ,
				0,
				offset + i,
				response,
				2,
				timeout
			);
			if (usbRet < 0) {
				ERR(("_mcuCommandE2Prom(): USB error '%s'!", usb_strerror()));

				ret = COMMON_ERROR;
				break;
			}

			if ((usbRet != 2) || (response[0] != BOOTLOADER_COMMON_COMMAND_STATUS_OK)) {
				ERR(("_mcuCommandE2Prom(): Bad response!, %d", usbRet));

				ret = COMMON_ERROR_BAD_PARAMETER;
				break;
			}

			buffer[i] = response[1];
		}
	} while (0);

	return ret;
}


static CommonError _mcuCommandE2PromWrite(_U32 offset, _U8 *buffer, _U32 bufferSize, _U32 timeout) {
	CommonError ret = COMMON_NO_ERROR;

	do {
		_U32 i;

		DBG(("_mcuCommandE2PromWrite(): Call for buffer: %p, bufferSize: %d", buffer, bufferSize));

		for (i = 0; i < bufferSize; i++) {
			_S32 usbRet;
			_U8  response;

			usbRet = usb_control_msg(
				privateData.deviceHandle,
				USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN,
				BOOTLOADER_COMMON_COMMAND_E2PROM_WRITE,
				buffer[i],
				offset + i,
				&response,
				2,
				timeout
			);
			if (usbRet < 0) {
				ERR(("_mcuCommandE2PromWrite(): USB error '%s'!", usb_strerror()));

				ret = COMMON_ERROR;
				break;
			}

			if ((usbRet != 1) || (response != BOOTLOADER_COMMON_COMMAND_STATUS_OK)) {
				ERR(("_mcuCommandE2PromWrite(): Bad response!, %d", usbRet));

				ret = COMMON_ERROR_BAD_PARAMETER;
				break;
			}
		}
	} while (0);

	return ret;
}


_S32 usbGetStringAscii(usb_dev_handle *dev, int index, char *buf, int buflen) {
	_S32 ret = 0;

	do {
		char buffer[256] = { 0 };
		_S32 rval;
		_S32 i;

		ret = usb_get_string_simple(dev, index, buf, buflen);
		if (ret >= 0) {
			DBG(("usbGetStringAscii(): Read by usb_get_string_simple"));

			break;
		}

		ret = usb_control_msg(dev, USB_ENDPOINT_IN, USB_REQ_GET_DESCRIPTOR, (USB_DT_STRING << 8) + index, 0x0409, buffer, sizeof(buffer), 5000);
		if (ret < 0) {
			DBG(("usbGetStringAscii(): usb_control_msg() returned error! %d", ret));

			break;
		}

		if (buffer[1] != USB_DT_STRING){
			DBG(("usbGetStringAscii(): no string data! %02x", buffer[1]));
			*buf = 0;

			ret = 0;
			break;
		}

		if ((_U8) buffer[0] < ret) {
			ret = (_U8) buffer[0];
		}

		ret /= 2;

		for (i = 1; i < rval; i++){
			if (i > buflen) {
				break;
			}

			buf[i - 1] = buffer[2 * i];

			if (buffer[2 * i + 1] != 0) {
				buf[i - 1] = '?';
			}
		}

		buf[i - 1] = 0;
	} while (0);

	return ret;
}


static _U8 _crc8_getForByte(_U8 byte, _U8 polynomial, _U8 start) {
	_U8 remainder = start;

	remainder ^= byte;

	{
		_U8 bit;

		for (bit = 0; bit < 8; bit++) {
			if (remainder & 0x01) {
				remainder = (remainder >> 1) ^ polynomial;

			} else {
				remainder = (remainder >> 1);
			}

		}
	}

    return remainder;
}


static _U32 _getTime(void) {
	_U32 ret = 0;

	{
		struct timeval tv;

		gettimeofday(&tv, NULL);

		ret = tv.tv_sec * 1000 + tv.tv_usec / 1000;
	}
}


CommonError bootloader_initialize(void) {
	CommonError ret = COMMON_NO_ERROR;

	ASSERT(! initialized);

	{
		usb_init();

		privateData.deviceHandle  = NULL;
		privateData.mcuParameters = NULL;

		initialized = TRUE;
	}

	return ret;
}


CommonError bootloader_terminate(void) {
	CommonError ret = COMMON_NO_ERROR;

	ASSERT(initialized);

	{
		initialized = FALSE;
	}

	return ret;
}


CommonError bootloader_connect(BootloaderTargetInformation *targetInformation, _U32 timeout) {
	CommonError ret = COMMON_NO_ERROR;

	ASSERT(initialized);
	ASSERT(targetInformation != NULL);

	{
		usb_dev_handle *deviceHandle = NULL;

		do {
			_U32  startTime = _getTime();
			_BOOL firstLoop = FALSE;

			do {
				struct usb_device *dev;

				_S32 libUsbRet;

				if (timeout != BOOTLOADER_TIMEOUT_INFINITY) {
					if (_getTime() - startTime >= timeout) {
						ret = COMMON_ERROR_TIMEOUT;

						break;
					}
				}

				{
					_S32 findRet = 0;

					findRet |= usb_find_busses();
					findRet |= usb_find_devices();
					if (! firstLoop && (findRet == 0)) {
						usleep(BOOTLOADER_CHECK_DEVICES_INTERVAL * 1000);

						continue;
					}
				}

				firstLoop = FALSE;

				// iterate all buses to find bootloader
				{
					struct usb_bus *bus = usb_get_busses();

					DBG(("bootloader_connect(): bus: %p", bus));

					while (bus != NULL) {
						usb_dev_handle *tmpHandle = NULL;

						dev = bus->devices;

						while (dev != NULL) {
							DBG(("bootloader_connect(): Found device with PID: %04x VID: %04x", dev->descriptor.idProduct, dev->descriptor.idVendor));

							if (
								(dev->descriptor.idVendor  == idVendor) &&
								(dev->descriptor.idProduct == idProduct)
							) {
								DBG(("bootloader_connect(): Got device with proper PID: %04x, VID: %04x!", dev->descriptor.idProduct, dev->descriptor.idVendor));

								do {
									tmpHandle = usb_open(dev);
									if (tmpHandle == NULL) {
										REPORT(("Unable to open device with PID: %04x VID: %04x", idProduct, idVendor));

										break;
									}

									if (dev->descriptor.iManufacturer > 0) {
										char vendor[256] = { 0 };

										libUsbRet = usbGetStringAscii(tmpHandle, dev->descriptor.iManufacturer, vendor, sizeof(vendor));
										if (libUsbRet >= 0) {
											DBG(("bootloader_connect(): vendor: '%s'", vendor));

											if (strcmp(vendor, vendorName) == 0) {
												DBG(("bootloader_connect(): Found device with proper vendor!"));

											} else {
												break;
											}

										} else {
											DBG(("bootloader_connect(): Error reading vendor!"));
										}
									}

									if (dev->descriptor.iProduct > 0) {
										char product[256] = { 0 };

										libUsbRet = usbGetStringAscii(tmpHandle, dev->descriptor.iProduct, product, sizeof(product));
										if (libUsbRet >= 0) {
											DBG(("bootloader_connect(): product: '%s'", product));

											if (strcmp(product, deviceName) == 0) {
												DBG(("bootloader_connect(): Found device with proper product name!"));

												deviceHandle = tmpHandle;
												break;

											} else {
												break;
											}

										} else {
											DBG(("bootloader_connect(): Error reading product!"));
										}
									}
								} while (0);

								if (deviceHandle == NULL) {
									if (tmpHandle != NULL) {
										DBG(("bootloader_connect(): Closing device!"));

										usb_close(tmpHandle);

										tmpHandle = NULL;
									}

								} else {
									break;
								}
							}

							dev = dev->next;
						}

						if (deviceHandle != NULL) {
							break;
						}

						bus = bus->next;
					}
				}

				if (deviceHandle != NULL) {
					break;
				}
			} while (1);

			if (ret != COMMON_NO_ERROR) {
				break;
			}

			if (deviceHandle == NULL) {
				ret = COMMON_ERROR_NO_DEVICE;
				break;
			}

			privateData.deviceHandle = deviceHandle;

			{
				McuInformation info = { 0 };

				ret = _mcuCommandConnect(timeout - (_getTime() - startTime));
				if (ret != COMMON_NO_ERROR) {
					ERR(("bootloader_connect(): Unable to establish connection with bootloader!"));

					break;
				}

				ret = _mcuCommandGetInfo(&info, timeout - (_getTime() - startTime));
				if (ret != COMMON_NO_ERROR) {
					ERR(("bootloader_connect(): Unable to retrieve bootloader and MCU parameters!"));

					break;
				}

				{
					_U32 i;

					for (i = 0; i < sizeof(mcu) / sizeof(*mcu); i++) {
						if (
							(mcu[i].id.byte1 == info.signature.byte1) &&
							(mcu[i].id.byte2 == info.signature.byte2) &&
							(mcu[i].id.byte3 == info.signature.byte3)
						) {
							DBG(("bootloader_connect(): Found mcu parameters !"));

							privateData.mcuParameters = &mcu[i];
							break;
						}
					}

					if (privateData.mcuParameters == NULL) {
						ERR(("bootloader_connect(): Not supported MCU! (%02x%02x%02x)", info.signature.byte1, info.signature.byte2, info.signature.byte3));

						ret = COMMON_ERROR_NO_DEVICE;
						break;
					}
				}

				privateData.bootloaderSectionSize = info.bootloaderSizeInPages * privateData.mcuParameters->flash.pageSize;

//				DBG(("Found bootloader in revision: %d.%d on MCU: '%s'", info.bootloaderVersion.major, info.bootloaderVersion.minor, privateData.mcuParameters->name));

				targetInformation->bootloader.versionMajor = info.bootloaderVersion.major;
				targetInformation->bootloader.versionMinor = info.bootloaderVersion.minor;

				targetInformation->flash.pageSize   = privateData.mcuParameters->flash.pageSize;
				targetInformation->flash.pagesCount = (privateData.mcuParameters->flash.size / privateData.mcuParameters->flash.pageSize) - info.bootloaderSizeInPages;

				targetInformation->e2prom.size = privateData.mcuParameters->e2prom.size;

				targetInformation->mcu.name = privateData.mcuParameters->name;
			}
		} while (0);

		if (ret != COMMON_NO_ERROR) {
			if (deviceHandle != NULL) {
				usb_close(deviceHandle);
			}

			privateData.deviceHandle = NULL;
		}
	}

	return ret;
}


CommonError bootloader_disconnect(void) {
	CommonError ret = COMMON_NO_ERROR;

	ASSERT(initialized);

	{
		if (privateData.deviceHandle) {
			usb_close(privateData.deviceHandle);

			privateData.deviceHandle = NULL;
		}
	}

	return ret;
}


CommonError bootloader_reset(_U32 timeout) {
	CommonError ret = COMMON_NO_ERROR;

	ASSERT(initialized);

	{
		DBG(("bootloader_reset(): Reset"));

		ret = _mcuCommandReboot(timeout);
	}

	return ret;
}


CommonError bootloader_flashPageErase(_U32 pageAddress, _U32 timeout) {
	CommonError ret = COMMON_NO_ERROR;

	ASSERT(initialized);

	{
		DBG(("bootloader_flashPageErase(): Erasing page number: %d", pageAddress));

		ret = _mcuCommandFlashPageErase(pageAddress, timeout);
	}

	return ret;
}


CommonError bootloader_flashPageRead(_U32 pageAddress, _U8 *pageBuffer, _U32 pageBufferSize, _U32 timeout, _U32 *pageBufferReadSize) {
	CommonError ret = COMMON_NO_ERROR;

	ASSERT(initialized);

	{
		DBG(("bootloader_flashPageRead(): Reading from page %d", pageAddress));

		ret = _mcuCommandFlashPageRead(pageAddress, pageBuffer, pageBufferSize, timeout);
	}

	return ret;
}


CommonError bootloader_flashPageWrite(_U32 pageAddress, _U8 *pageBuffer, _U32 pageBufferSize, _U32 timeout, _U32 *pageBufferWritten) {
	CommonError ret = COMMON_NO_ERROR;

	ASSERT(initialized);

	{
		DBG(("bootloader_flashPageWrite(): Writing page number: %d", pageAddress));

		ret = _mcuCommandFlashPageWrite(pageAddress, pageBuffer, pageBufferSize, timeout);
	}

	return ret;
}


CommonError bootloader_e2promRead(_U32 address, _U8 *e2promBuffer, _U32 e2promBufferSize, _U32 timeout, _U32 *e2promBufferReadSize) {
	CommonError ret = COMMON_NO_ERROR;

	ASSERT(initialized);

	{
		DBG(("bootloader_e2promRead(): Reading e2prom from: %d, size: %d", address, e2promBufferSize));

		ret = _mcuCommandE2PromRead(address, e2promBuffer, e2promBufferSize, timeout);
	}

	return ret;
}


CommonError bootloader_e2promWrite(_U32 address, _U8 *e2promBuffer, _U32 e2promBufferSize, _U32 timeout, _U32 *e2promBufferWritten) {
	CommonError ret = COMMON_NO_ERROR;

	ASSERT(initialized);

	{
		DBG(("bootloader_e2promWrite(): Writing e2prom at: %d, size: %d", address, e2promBufferSize));

		ret = _mcuCommandE2PromWrite(address, e2promBuffer, e2promBufferSize, timeout);
	}

	return ret;
}
