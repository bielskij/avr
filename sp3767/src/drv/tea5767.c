/**
 * @file:   drv/tea5767.h
 * @date:   2014-02-19
 * @Author: Jaroslaw Bielski (bielski.j@gmail.com)
 *
 **********************************************
 * Copyright (c) 2013 Jaroslaw Bielski.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the GNU Public License v3.0
 * which accompanies this distribution, and is available at
 * http://www.gnu.org/licenses/gpl.html
 *
 * Contributors:
 *     Jaroslaw Bielski (bielski.j@gmail.com)
 *********************************************
 *
 *
 * @brief
 */

#include <util/delay.h>

#include "drv/i2c.h"
#include "drv/tea5767.h"


#define DRV_TEA5767_I2C_ADDRESS 0x60

#define BYTE0_WRITE_FLAG_MUTE        0x80
#define BYTE0_WRITE_FLAG_SEARCH_MODE 0x40

#define BYTE2_WRITE_FLAG_SEARCH_INCREASED       0x80 // Increased frequency search
#define BYTE2_WRITE_FLAG_SEARCH_STOP_HIGH       0x60 // High: ADC output size is 10
#define BYTE2_WRITE_FLAG_SEARCH_STOP_IN         0x40 // In: ADC output size 7
#define BYTE2_WRITE_FLAG_SEARCH_STOP_LOW        0x20 // Low: ADC output size of 5
#define BYTE2_WRITE_FLAG_SIDE_INJECTION_HIGH    0x10
#define BYTE2_WRITE_FLAG_MONO                   0x08
#define BYTE2_WRITE_FLAG_MUTE_LEFT              0x04
#define BYTE2_WRITE_FLAG_MUTE_RIGHT             0x02
#define BYTE2_WRITE_FLAG_SWP1_HIGH              0x01

#define BYTE3_WRITE_FLAG_SWP2_HIGH                0x80
#define BYTE3_WRITE_FLAG_STANDBY                  0x40
#define BYTE3_WRITE_FLAG_BAND_TYPE_JAPAN          0x20
#define BYTE3_WRITE_FLAG_FXTAL_32768KHZ           0x10
#define BYTE3_WRITE_FLAG_SOFTWARE_MUTE_OPEN       0x08
#define BYTE3_WRITE_FLAG_WHITE_HIGH_CUT_OPEN      0x04
#define BYTE3_WRITE_FLAG_WHITE_HIGH_CUT_OPEN      0x04
#define BYTE3_WRITE_FLAG_STEREO_NOISE_REMOVAL_ON  0x02
#define BYTE3_WRITE_FLAG_SWP1_MODE_READY          0x01
#define BYTE3_WRITE_FLAG_SWP1_MODE_USER_PIO       0x01

#define BYTE4_WRITE_FLAG_ENABLE_PLL_REF           0x80 // 6.5 MHz PLL reference frequency is enabled;
#define BYTE4_WRITE_FLAG_DEEMPHASIS_75US          0x40 // de-emphasis time constant is 75us. If 0, de-emphasis time constant is 50us.

#define BYTE0_READ_FLAG_READY      0x80
#define BYTE0_READ_FLAG_BAND_LIMIT 0x40
#define BYTE0_READ_MASK_PLL        0x3f

#define BYTE1_READ_MASK_PLL        0xff

#define BYTE2_READ_FLAG_STEREO     0x80
#define BYTE2_READ_MASK_IF_COUNTER 0x7f

#define BYTE3_READ_MASK_LEVEL      0xf0
#define BYTE3_READ_MASK_CHIP_ID    0x0e

#define ADC_LEVEL_LOW    0x50
#define ADC_LEVEL_MIDDLE 0x70
#define ADC_LEVEL_HIGH   0xa0


#define PLL_LOW_FREQUENCY  0x299D // 87.5MHz
#define PLL_HIGH_FREQUENCY 0x3364 // 108MHz


typedef struct _DrvTea5767Context {
	DrvTea5767SignalThreshold signalThreshold;
	DrvTea5767ChannelsMode    channelsMode;
	_BOOL                     muted;
	_U16                      pll;
	_BOOL                     standby;
	_BOOL                     highSide;
} DrvTea5767Context;


static DrvTea5767Context context = { 0 };


static _U16 _convertFrequencyToPll(_U32 frequency, _BOOL high) {
	if (high) {
		frequency += 225;

	} else {
		frequency -= 225;
	}

	return (125 * frequency) >> 10;
}


static _U32 _convertPllToFrequency(_U16 pll, _BOOL high) {
	_U32 freq = (((_U32) pll << 3) + ((_U32)pll * 24) / 125);

	if (high) {
		freq -= 225;

	} else {
		freq += 225;
	}

	_U16 rest = freq % 100;

	// Round to 100kHz
	freq -= rest;

	if (rest > 50) {
		freq += 100;
	}

	return freq;
}


static _S8 _i2cTransfer(_U8 buffer[5], _BOOL write) {
	_S8 ret = 0;

	{
		DrvI2cMessage msg;

		msg.slaveAddress = DRV_TEA5767_I2C_ADDRESS;

		if (write) {
			msg.flags    = DRV_I2C_MESSAGE_FLAG_WRITE;

		} else {
			msg.flags    = DRV_I2C_MESSAGE_FLAG_READ;
		}

		msg.data         = buffer;
		msg.dataSize     = 5;

		ret = drv_i2c_transfer(&msg, 1);
	}

	return ret;
}


static _S8 _tunerSet(DrvTea5767ChannelsMode channelsMode, _BOOL muted, _U16 pll, _BOOL highSide, _BOOL standby, _BOOL scanMode, _BOOL backward) {
	_S8 ret = 0;

	{
		_U8 buffer[5] = { 0 };

		if (muted) {
			buffer[0] |= BYTE0_WRITE_FLAG_MUTE;
		}

		buffer[0] |= ((pll & 0x3f00) >> 8);
		buffer[1] |= ((pll & 0x00ff));

		switch (channelsMode) {
			case DRV_TEA5767_CHANNELS_MODE_MONO:
				buffer[2] |= BYTE2_WRITE_FLAG_MONO;
				break;

			case DRV_TEA5767_CHANNELS_MODE_MONO_LEFT:
				buffer[2] |= BYTE2_WRITE_FLAG_MUTE_RIGHT;
				break;

			case DRV_TEA5767_CHANNELS_MODE_MONO_RIGHT:
				buffer[2] |= BYTE2_WRITE_FLAG_MUTE_LEFT;
				break;

			case DRV_TEA5767_CHANNELS_MODE_STEREO:
				buffer[3] |= BYTE3_WRITE_FLAG_STEREO_NOISE_REMOVAL_ON;
				break;
		}

		if (scanMode) {
			buffer[0] |= BYTE0_WRITE_FLAG_SEARCH_MODE;

			if (! backward) {
				buffer[2] |= BYTE2_WRITE_FLAG_SEARCH_INCREASED;
			}

			switch (context.signalThreshold) {
				case DRV_TEA5767_SIGNAL_THRESHOLD_LOW:
					buffer[2] |= BYTE2_WRITE_FLAG_SEARCH_STOP_LOW;
					break;

				case DRV_TEA5767_SIGNAL_THRESHOLD_MEDIUM:
					buffer[2] |= BYTE2_WRITE_FLAG_SEARCH_STOP_IN;
					break;

				case DRV_TEA5767_SIGNAL_THRESHOLD_HIGH:
					buffer[2] |= BYTE2_WRITE_FLAG_SEARCH_STOP_HIGH;
					break;
			}
		}

		buffer[3] |= BYTE3_WRITE_FLAG_WHITE_HIGH_CUT_OPEN;

		if (standby) {
			buffer[3] |= BYTE3_WRITE_FLAG_STANDBY;
		}

		if (highSide) {
			buffer[2] |= BYTE2_WRITE_FLAG_SIDE_INJECTION_HIGH;
		}

		buffer[3] |= BYTE3_WRITE_FLAG_FXTAL_32768KHZ;

		ret = _i2cTransfer(buffer, TRUE);
	}

	return ret;
}


static _S8 _applyChanges(DrvTea5767ChannelsMode channelsMode, _BOOL muted, _U16 pll, _BOOL highSide, _BOOL standby, _BOOL force) {
	_S8 ret = 0;

	{
		_BOOL changed = FALSE;

		if (
			(context.channelsMode != channelsMode) ||
			(context.muted        != muted)        ||
			(context.pll          != pll)          ||
			(context.standby      != standby)      ||
			(context.highSide     != highSide)     || force
		) {
			changed = TRUE;
		}

		if (changed) {
			ret = _tunerSet(channelsMode, muted, pll, highSide, standby, FALSE, FALSE);
			if (ret == 0) {
				context.channelsMode = channelsMode;
				context.muted        = muted;
				context.pll          = pll;
				context.standby      = standby;
				context.highSide     = highSide;
			}
		}
	}

	return ret;
}


static _S8 _restoreState(void) {
	return _applyChanges(context.channelsMode, context.muted, context.pll, context.highSide, context.standby, TRUE);
}


static void _resetContext(void) {
	context.signalThreshold = DRV_TEA5767_SIGNAL_THRESHOLD_HIGH;
	context.channelsMode    = DRV_TEA5767_CHANNELS_MODE_STEREO;
	context.muted           = TRUE;
	context.pll             = PLL_LOW_FREQUENCY; // 87.5 MHz
	context.standby         = FALSE;
	context.highSide        = FALSE;
}


static _U8 _getSignalLevelFromBuffer(_U8 buffer[5]) {
	return ((buffer[3] & BYTE3_READ_MASK_LEVEL) >> 4);
}


static _U16 _getPllFromBuffer(_U8 buffer[5]) {
	return ((_U16)(buffer[0] & BYTE0_READ_MASK_PLL) << 8) | (buffer[1] & BYTE1_READ_MASK_PLL);
}


void drv_tea5767_initialize(void) {
	_resetContext();
}


_S8 drv_tea5767_tune(_U32 frequency, _BOOL force) {
	_S8 ret = 0;

	do {
		_BOOL highSide = FALSE;
		_U16 pll;

		if (frequency < DRV_TEA5767_FREQUENCY_MIN && frequency > DRV_TEA5767_FREQUENCY_MAX) {
			ret = -1;
			break;
		}

		// Check which injection side should be set
		{
			_U8 data[5];
			_U8 signalHigh;
			_U8 signalLow;

			pll = _convertFrequencyToPll(frequency + 450, TRUE);

			ret  = _tunerSet(DRV_TEA5767_CHANNELS_MODE_MONO, TRUE, pll, TRUE, FALSE, FALSE, FALSE);
			ret |= _i2cTransfer(data, FALSE);
			if (ret != 0) {
				break;
			}

			signalHigh = _getSignalLevelFromBuffer(data);

			pll = _convertFrequencyToPll(frequency - 450, TRUE);

			ret  = _tunerSet(DRV_TEA5767_CHANNELS_MODE_MONO, TRUE, pll, TRUE, FALSE, FALSE, FALSE);
			ret |= _i2cTransfer(data, FALSE);
			if (ret != 0) {
				break;
			}

			signalLow = _getSignalLevelFromBuffer(data);

			if (signalHigh < signalLow) {
				highSide = TRUE;
			}
		}

		pll = _convertFrequencyToPll(frequency, highSide);

		ret = _applyChanges(context.channelsMode, context.muted, pll, highSide, context.standby, force);
	} while (0);

	return ret;
}


_S8 drv_tea5767_getStatus(DrvTea5767Status *status) {
	_S8 ret = 0;

	{
		_U8 responseBuffer[5] = { 0 };

		ret = _i2cTransfer(responseBuffer, FALSE);
		if (ret == 0) {
			status->muted = context.muted;

			{
				status->state = 0;

				if (! (responseBuffer[0] & BYTE0_READ_FLAG_READY)) {
					status->state |= DRV_TEA5767_STATE_BUSY;
				}

				if (responseBuffer[2] & BYTE2_READ_FLAG_STEREO) {
					status->state |= DRV_TEA5767_STATE_STEREO;
				}

				if ((responseBuffer[3] & BYTE3_READ_MASK_LEVEL) >= ADC_LEVEL_LOW) {
					status->state |= DRV_TEA5767_STATE_LOCKED;
				}

				status->signalRate = _getSignalLevelFromBuffer(responseBuffer);
			}

			{
				_U16 pll = _getPllFromBuffer(responseBuffer);

				status->frequency = _convertPllToFrequency(pll, context.highSide);
			}

			status->standby = context.standby;

		} else {
			status->frequency  = 0;
			status->muted      = FALSE;
			status->signalRate = 0;
			status->standby    = FALSE;
			status->state      = 0;
		}
	} while (0);

	return ret;
}


static _U16 _getNextStation(_U16 startPll, _BOOL backward) {
	_U16 ret = 0xffff;

	_U16 pll = startPll;

	do {
		_S8   i2cRet;
		_U8   data[5];
		_BOOL finished = FALSE;

		i2cRet = _tunerSet(DRV_TEA5767_CHANNELS_MODE_STEREO, TRUE, pll, FALSE, FALSE, TRUE, backward);
		if (i2cRet != 0) {
			break;
		}

		do {
			i2cRet = _i2cTransfer(data, FALSE);
			if (i2cRet != 0) {
				break;
			}

			if (data[0] & BYTE0_READ_FLAG_BAND_LIMIT) {
				finished = TRUE;
				break;
			}

			if (data[0] & BYTE0_READ_FLAG_READY) {
				pll = _getPllFromBuffer(data);
				break;
			}
		} while (1);

		if (i2cRet != 0) {
			break;
		}

		if (finished) {
			break;
		}

		{
			_S8 signalLow = _getSignalLevelFromBuffer(data);
			_S8 signalHigh;
			_U8 ifc;

			i2cRet |= _tunerSet(DRV_TEA5767_CHANNELS_MODE_STEREO, TRUE, pll, TRUE, FALSE, FALSE, FALSE);
			i2cRet |= _i2cTransfer(data, FALSE);

			if (i2cRet != 0) {
				break;
			}

			signalHigh = _getSignalLevelFromBuffer(data);
			ifc        = data[2] & BYTE2_READ_MASK_IF_COUNTER;

			if (abs(signalLow - signalHigh) < 2 && (ifc > 0x31) && (ifc < 0x3e)) {
				ret = pll;
				break;
			}

			pll = _convertFrequencyToPll(_convertPllToFrequency(pll, FALSE) + 100, FALSE);
		}
	} while (1);

	return ret;
}


_S8 drv_tea5767_scan(DrvTea5767TunedCallback callback) {
	_S8 ret = 0;

	{
		_U16 pll = PLL_LOW_FREQUENCY;

		do {
			pll = _getNextStation(pll, FALSE);
			if (pll != 0xffff) {
				callback(_convertPllToFrequency(pll, FALSE));

				// Next 100kHz
				pll += 13;
			}
		} while (pll != 0xffff);

		// Revert changes
		ret |= _restoreState();
	}

	return ret;
}


_S8 drv_tea5767_channelUp(DrvTea5767TunedCallback callback) {
	_S8 ret = 0;

	do {
		_U16 pll;

		{
			DrvTea5767Status status;

			ret = drv_tea5767_getStatus(&status);
			if (ret != 0) {
				break;
			}

			if (status.frequency + 100 > DRV_TEA5767_FREQUENCY_MAX) {
				ret = -1;
				break;
			}

			pll = _convertFrequencyToPll(status.frequency + 100, FALSE);
		}

		pll = _getNextStation(pll, FALSE);
		if (pll != 0xffff) {
			_U32 frequency = _convertPllToFrequency(pll, FALSE);

			ret = drv_tea5767_tune(frequency, TRUE);
			if (ret == 0) {
				callback(frequency);
			}
		}
	} while (0);

	return ret;
}


_S8 drv_tea5767_channelDown(DrvTea5767TunedCallback callback) {
	_S8 ret = 0;

	do {
		_U16 pll;

		{
			DrvTea5767Status status;

			ret = drv_tea5767_getStatus(&status);
			if (ret != 0) {
				break;
			}

			if (status.frequency - 100 < DRV_TEA5767_FREQUENCY_MIN) {
				ret = -1;
				break;
			}

			pll = _convertFrequencyToPll(status.frequency - 100, FALSE);
		}

		pll = _getNextStation(pll, TRUE);
		if (pll != 0xffff) {
			_U32 frequency = _convertPllToFrequency(pll, FALSE);

			ret = drv_tea5767_tune(frequency, TRUE);
			if (ret == 0) {
				callback(frequency);
			}
		}
	} while (0);

	return ret;
}


_S8 drv_tea5767_mute(_BOOL mute) {
	_S8 ret = 0;

	ret = _applyChanges(context.channelsMode, mute, context.pll, context.highSide, context.standby, FALSE);

	return ret;
}


_S8 drv_tea5767_getConfig(DrvTea5767Config *config) {
	_S8 ret = 0;

	config->channelsMode    = context.channelsMode;
	config->signalThreshold = context.signalThreshold;

	return ret;
}


_S8 drv_tea5767_setConfig(DrvTea5767Config *config) {
	_S8 ret = 0;

	context.signalThreshold = config->signalThreshold;

	ret = _applyChanges(config->channelsMode, context.muted, context.pll, context.highSide, context.standby, FALSE);

	return ret;
}


_S8 drv_tea5767_standby(_BOOL standby) {
	_S8 ret = 0;

	ret = _applyChanges(context.channelsMode, context.muted, context.pll, context.highSide, standby, FALSE);

	return ret;
}

_S8 drv_tea5767_reset(void) {
	_S8 ret;

	_resetContext();

	ret = _restoreState();

	return ret;
}
