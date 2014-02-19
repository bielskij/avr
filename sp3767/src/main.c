#include <stdio.h>

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "common/utils.h"

#include "drv/i2c.h"
#include "drv/tea5767.h"

#include "common/debug.h"


static void _twiDetectCallback(_U8 address, _BOOL read, _BOOL write) {
	debug_print("New device! at: ");
	debug_dumpHex(address, 1);
	debug_print(", read: ");
	debug_dumpHex(read, 1);
	debug_print(", write: ");
	debug_dumpHex(write, 1);
	debug_print("\r\n");
}


static void _tunedCallback(_U32 frequency) {
	debug_print("Tuned! frequency: ");
	debug_dumpDec(frequency);
	debug_print("\r\n");
}


void main(void) {
	debug_initialize();

	{
		DBG(("START"));

		// Initialize drivers
		drv_i2c_initialize();
		drv_tea5767_initialize();

		// Reset tuner to default state.
		drv_tea5767_reset();

		while (1) {
			char c = debug_getc();

			switch (c) {

				case 'd':
					{
						DBG(("Searching I2C devices..."));
						drv_i2c_detect(_twiDetectCallback);
						DBG(("Searching finished!"));
					}
					break;

				case '1':
					drv_tea5767_mute(TRUE);
					break;

				case '2':
					drv_tea5767_mute(FALSE);
					break;

				case '3':
				case '4':
				case '5':
				case '6':
					{
						DrvTea5767Config config;

						drv_tea5767_getConfig(&config);

						if (c == '3') {
							config.channelsMode = DRV_TEA5767_CHANNELS_MODE_STEREO;

						} else if (c == '4') {
							config.channelsMode = DRV_TEA5767_CHANNELS_MODE_MONO;

						} else if (c == '5') {
							config.channelsMode = DRV_TEA5767_CHANNELS_MODE_MONO_RIGHT;

						} else {
							config.channelsMode = DRV_TEA5767_CHANNELS_MODE_MONO_LEFT;
						}

						drv_tea5767_setConfig(&config);
					}
					break;

				case '7':
					drv_tea5767_tune(93300, TRUE); // RMF FM
					break;

				case '8':
					drv_tea5767_tune(95600, TRUE); // ZET
					break;

				case '9':
					drv_tea5767_standby(TRUE);
					break;

				case '0':
					drv_tea5767_standby(FALSE);
					break;

				case 's':
					{
						DrvTea5767Status status;

						drv_tea5767_getStatus(&status);

						debug_print("-- Status --\r\n");

						debug_print(" State: ");
						{
							if (status.state & DRV_TEA5767_STATE_BUSY) {
								debug_print("BUSY");
							}

							if (status.state & DRV_TEA5767_STATE_LOCKED) {
								debug_print(" | LOCKED");
							}

							if (status.state & DRV_TEA5767_STATE_STEREO) {
								debug_print(" | STEREO");
							}
						}
						debug_print("\r\n");

						debug_print(" Signal: ");
						{
							debug_print(" min: ");
							debug_dumpDec(DRV_TEA5767_SIGNAL_MIN);
							debug_print(" cur: ");
							debug_dumpDec(status.signalRate);
							debug_print(" max: ");
							debug_dumpDec(DRV_TEA5767_SIGNAL_MAX);
						}
						debug_print("\r\n");

						debug_print(" Muted: ");
						{
							if (status.muted) {
								debug_print("T");

							} else {
								debug_print("F");
							}
						}
						debug_print("\r\n");

						debug_print(" Standby: ");
						{
							if (status.standby) {
								debug_print("T");

							} else {
								debug_print("F");
							}
						}
						debug_print("\r\n");

						debug_print(" Frequency: ");
						{
							debug_dumpDec(status.frequency);
						}
						debug_print("\r\n");
					}
					break;

				case 'a':
					drv_tea5767_scan(_tunedCallback);
					break;

				case 'z':
				case 'x':
				case 'c':
					{
						DrvTea5767Config config;

						drv_tea5767_getConfig(&config);

						if (c == 'z') {
							config.signalThreshold = DRV_TEA5767_SIGNAL_THRESHOLD_LOW;

						} else if (c == 'x') {
							config.signalThreshold = DRV_TEA5767_SIGNAL_THRESHOLD_MEDIUM;

						} else {
							config.signalThreshold = DRV_TEA5767_SIGNAL_THRESHOLD_HIGH;
						}

						drv_tea5767_setConfig(&config);
					}
					break;

				case '[':
					drv_tea5767_channelDown(_tunedCallback);
					break;

				case ']':
					drv_tea5767_channelUp(_tunedCallback);
					break;
			}

			debug_print("-- MENU --\r\n");
			debug_print("s. Status\r\n");
			debug_print("1. Mute\r\n");
			debug_print("2. Unmute\r\n");
			debug_print("3. STEREO\r\n");
			debug_print("4. MONO\r\n");
			debug_print("5. MONO-R\r\n");
			debug_print("6. MONO-L\r\n");
			debug_print("7. Tune RMF-FM\r\n");
			debug_print("8. Tune ZET\r\n");
			debug_print("9. Standby in\r\n");
			debug_print("0. Standby out\r\n");
			debug_print("a. Scan\r\n");
			debug_print("z. Signal threshold lo\r\n");
			debug_print("x. Signal threshold mid\r\n");
			debug_print("c. Signal threshold hi\r\n");
			debug_print("[. Previous station.\r\n");
			debug_print("]. Next station.\r\n");
			debug_print("----------\r\n");
		}

		drv_tea5767_terminate();
		drv_i2c_terminate();
	}
}
