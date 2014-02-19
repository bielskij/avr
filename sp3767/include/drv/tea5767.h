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

#ifndef DRV_TEA5767_H_
#define DRV_TEA5767_H_

#include "common/types.h"

/*!
 * Minimal signal rate.
 */
#define DRV_TEA5767_SIGNAL_MIN 0

/*!
 * Maximal signal rate.
 */
#define DRV_TEA5767_SIGNAL_MAX 15

/*!
 * Minimal supported frequency (in kHz)
 */
#define DRV_TEA5767_FREQUENCY_MIN  87500
/*!
 * MAximal supported frequency (in kHz)
 */
#define DRV_TEA5767_FREQUENCY_MAX 108000

/*!
 * Tuner state
 */
typedef enum _DrvTea5767State {
	/// Tuner is busy (ex during scanning or switching to other channel)
	DRV_TEA5767_STATE_BUSY   = 0x01,

	/// Signal locked
	DRV_TEA5767_STATE_LOCKED = 0x02,

	/// Stereo audio
	DRV_TEA5767_STATE_STEREO = 0x04
} DrvTea5767State;

/*!
 * Analog audio channels output modes.
 */
typedef enum _DrvTea5767ChannelsMode {
	/// Both channels are available in stereo mode
	DRV_TEA5767_CHANNELS_MODE_STEREO,

	/// Both channels are available in mono mode
	DRV_TEA5767_CHANNELS_MODE_MONO,

	/// Only right channel is available in mono mode (left channel is muted)
	DRV_TEA5767_CHANNELS_MODE_MONO_RIGHT,

	/// Only left channel is available in mono mode (right channel is muted)
	DRV_TEA5767_CHANNELS_MODE_MONO_LEFT
} DrvTea5767ChannelsMode;

/*!
 * Input signal threshold. Value is used during scanning all frequencies and switching between different channels.
 */
typedef enum _DrvTea5767SignalThreshold {
	/// Low signal - signal ratio 5
	DRV_TEA5767_SIGNAL_THRESHOLD_LOW,

	/// Medium signal - signal ratio 7
	DRV_TEA5767_SIGNAL_THRESHOLD_MEDIUM,

	/// High signal - signal ratio 10
	DRV_TEA5767_SIGNAL_THRESHOLD_HIGH
} DrvTea5767SignalThreshold;

/*!
 * Tuner status
 */
typedef struct _DrvTea5767Status {
	/// Current state
	DrvTea5767State state;

	/// Signal ratio
	_U8             signalRate;

	/// Analog outputs muted
	_BOOL           muted;

	/// Current frequency (in kHz)
	_U32            frequency;

	/// Tuner in standby mode
	_BOOL           standby;
} DrvTea5767Status;

/*!
 * Tuner config parameters
 */
typedef struct _DrvTea5767Config {
	/// Channels mode, see @DrvTea5767ChannelsMode
	DrvTea5767ChannelsMode    channelsMode;

	/// Signal ratio threshold
	DrvTea5767SignalThreshold signalThreshold;
} DrvTea5767Config;

/*!
 * Callback called when tuner finds any radio channel.
 *
 * @param[out] frequency frequency of found channel.
 */
typedef void (*DrvTea5767TunedCallback)(_U32 frequency);


/*!
 * Initializes internal module
 */
void drv_tea5767_initialize(void);

/*!
 * Terminates internal module
 */
void drv_tea5767_terminate(void);

/*!
 * Performs tune.
 *
 * @param[in] frequency frequency to tune (in kHz)
 * @param[in] force     if tuner is already tuned to the same frequency this flag
 *                      performs force tune operation to the same frequency
 *
 * @return 0 if no error occurred. In other situations function shall return -1.
 */
_S8 drv_tea5767_tune(_U32 frequency, _BOOL force);

/*!
 * Gives tuner status.
 *
 * @param[in] status pointer to status structure. It has to be different from null!
 *
 * @return 0 if no error occurred. In other situations function shall return -1.
 */
_S8 drv_tea5767_getStatus(DrvTea5767Status *status);

/*!
 * Performs scanning. On each found station callback should be called with its frequency.
 *
 * @param[in] callback
 *
 * @return 0 if no error occurred. In other situations function shall return -1.
 */
_S8 drv_tea5767_scan(DrvTea5767TunedCallback callback);

/*!
 * Switch channel to the next one.
 *
 * @param[in] callback
 *
 * @return 0 if no error occurred. In other situations function shall return -1.
 */
_S8 drv_tea5767_channelUp(DrvTea5767TunedCallback callback);

/*!
 * Switch channel to the previous one.
 *
 * @param[in] callback
 *
 * @return 0 if no error occurred. In other situations function shall return -1.
 */
_S8 drv_tea5767_channelDown(DrvTea5767TunedCallback callback);

/*!
 * Mutes analog audio outputs.
 *
 * @param[in] mute
 *
 * @return 0 if no error occurred. In other situations function shall return -1.
 */
_S8 drv_tea5767_mute(_BOOL mute);

/*!
 * Gives tuner config.
 *
 * @param[in] config
 *
 * @return 0 if no error occurred. In other situations function shall return -1.
 */
_S8 drv_tea5767_getConfig(DrvTea5767Config *config);

/*!
 * Propagates tuner config.
 *
 * @param[in] config
 *
 * @return 0 if no error occurred. In other situations function shall return -1.
 */
_S8 drv_tea5767_setConfig(DrvTea5767Config *config);

/*!
 * Switches tuner to standby mode.
 *
 * @param[in] standby
 *
 * @return 0 if no error occurred. In other situations function shall return -1.
 */
_S8 drv_tea5767_standby(_BOOL standby);

/*!
 * Resets tuner device and set default state.
 *
 * @param[in] standby
 *
 */
_S8 drv_tea5767_reset(void);

#endif /* DRV_TEA5767_H_ */
