/*
 * Copyright (C) 2018 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*! \file
* \brief Device driver for monitoring ambient light intensity in (lux)
* proximity detection (prox), and Beam functionality within the
* AMS TMX49xx family of devices.
*/

#ifndef __STALS_VD6281_H
#define __STALS_VD6281_H

#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/input.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/interrupt.h>
#include <linux/leds.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/workqueue.h>
#include <linux/spinlock.h>
#include <linux/gpio.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/time.h>
#include <linux/timer.h>
#include <linux/file.h>
#include <linux/syscalls.h>
#include <linux/pinctrl/consumer.h>
#include <linux/pm_qos.h>
#include <linux/ktime.h>
#include <linux/hrtimer.h>
#include <linux/iio/consumer.h>
#include <linux/iio/iio.h>
#ifdef CONFIG_OF
#include <linux/of_gpio.h>
#include <linux/of_device.h>
#endif

#define HEADER_VERSION		"1"

/*! Constants  */
//!\{
#define STALS_ALS_MAX_CHANNELS 6              //!< Number of channels of the STALS
//!\}

/** 
 * @enum STALS_Channel_Id_t
 * 
 * Constants listing the channels of the device.
 */
enum STALS_Channel_Id_t {
    STALS_CHANNEL_1             = 0x01,        //!< channel 1
    STALS_CHANNEL_2             = 0x02,        //!< channel 2
    STALS_CHANNEL_3             = 0x04,        //!< channel 3
    STALS_CHANNEL_4             = 0x08,        //!< channel 4
    STALS_CHANNEL_5             = 0x10,        //!< channel 5
    STALS_CHANNEL_6             = 0x20,        //!< channel 6
};


/** 
 * @enum STALS_Color_Id_t
 * 
 * Constants listing the color light. this is used by the \ref STALS_GetChannelColor function to get what are color filers set on the channels
 */
enum STALS_Color_Id_t {
    STALS_COLOR_IR              = 0x01,        //!< Color IR
    STALS_COLOR_RED             = 0x02,        //!< Color RED
    STALS_COLOR_GREEN           = 0x03,        //!< Color GREEN
    STALS_COLOR_BLUE            = 0x04,        //!< Color BLUE
    STALS_COLOR_UV              = 0x05,        //!< Color UV
    STALS_COLOR_CLEAR           = 0x06,        //!< white light
};

/** 
 * @enum STALS_ErrCode_t
 * 
 * This enumeration is aimed at defining the different errors that can be returned by the STALS driver
 */
typedef enum  {
    STALS_NO_ERROR = 0,                        //!< No error 
    STALS_ERROR_INVALID_PARAMS,                //!< Provided parameters are invalid
    STALS_ERROR_INIT,                          //!< Error in the initialization of the VD621 device
    STALS_ERROR_TIME_OUT,                      //!< A time out has expired before an operation was completed 
    STALS_ERROR_INVALID_DEVICE_ID,             //!< The Provided device identifier is invalid
    STALS_ERROR_WRITE,                         //!< The trial to write on the I2C bus has failed
    STALS_ERROR_READ,                          //!< The trial to read from the I2C bus has failed
    STALS_ERROR_ALREADY_STARTED,               //!< The device is already started 
    STALS_ERROR_NOT_STARTED,                   //!< The device is not started 
    STALS_ERROR_NOT_SUPPORTED,                 //!< The called function is not supported, likely because not yet implemented
    STALS_ERROR_FNCT_DEPRECATED,               //!< The called function is deprecated
    STALS_ERROR_LAST_ERROR_CODE,
} STALS_ErrCode_t;



/** 
 * @enum STALS_Control_t 
 * 
 * This enumeration is aimed at defining the Enable and disable flags
 */
enum STALS_Control_t
{                                      
    STALS_CONTROL_DISABLE = 0,                  //!< Disable the feature
    STALS_CONTROL_ENABLE = 1,                   //!< Enable the feature
};

/** 
 * @enum STALS_Control_Id_t
 * 
 * This enumeration is aimed at defining the different parameters identifiers 
 */
enum STALS_Control_Id_t {
    /*!
     * Control to be used to enable or disable the pedestal\n
     * To enable the pedestal : \ref STALS_SetControl(pHandle, STALS_PEDESTAL_ENABLE, STALS_CONTROL_ENABLE);\n
     * To disable the pedestal : \ref STALS_SetControl(pHandle, STALS_PEDESTAL_ENABLE, STALS_CONTROL_DISABLE);\n
     * To know is the pedestal is enabled : \n
     * enum  STALS_Control_t Flag; \ref STALS_GetControl(pHandle, STALS_PEDESTAL_ENABLE, &Flag);
     */
    STALS_PEDESTAL_ENABLE       = 0,        

    /*!
     * Control to set the value of the pedestal\n
     * For example : \ref STALS_SetControl(pHandle, STALS_PEDESTAL_VALUE, 6);
     */
    STALS_PEDESTAL_VALUE        = 1,           

    /*!
     * Control to set if dark is output. For VD6281 dark count will be output on channel 2.
     */
    STALS_OUTPUT_DARK_ENABLE    = 3,

    /*!
     * Control to set drive current in sda pad in mA when device is driving sda line.
     * Possible values for vd6281 are 4, 8, 12, 16, 20 mA. 
    */
    STALS_SDA_DRIVE_VALUE_MA    = 4,

    /*!
     * Control to set if OTP information is use by driver. It is enable by default. Unless you know what you are doing don't change this value\n
     * For example : \ref STALS_SetControl(pHandle, STALS_OTP_USAGE_ENABLE, STALS_CONTROL_DISABLE).
     */
    STALS_OTP_USAGE_ENABLE      = 128,

    /*!
     * Control to get/set workaround state:
     * - use STALS_SetControl() to enable/disable given workaround. Msb bit of
     *   ControlValue control if wa is enable or disable. Others bits are the wa
     *   number.
     * - use STALS_GetControl() to get wa status. pControlValue is then an inout
     *   parameter. you call it with the wa for which you want to know state. on
     *   exit you read msb bit to know wa status.
     */
    STALS_WA_STATE              = 129
};


/** 
 * @struct STALS_FlickerInfo_t
 * 
 * This structure contains the fields filled by the driver and that contains the discovered information related to the flicker 
 */
struct STALS_FlickerInfo_t {
    uint32_t Frequency;                          //!< Value of the frequency
    uint8_t  ConfidenceLevel;                    //!< Confidence on the relevance of the measured value.
    uint8_t  IsMeasureFinish;                    //!< Value is 1 if measure is finish
};

/** 
 * @enum STALS_Mode_t 
 * 
 * This enumeration is aimed at defining the different behavior modes of the STALS device
 */
enum STALS_Mode_t {
    /*!
     * Single shot mode. In this mode, the STALS records and accumulates the light for 1 single period. \n
     * The STALS driver client is responsible for starting the next integration period, by calling the STALS_Start function again 
     */    
    STALS_MODE_ALS_SINGLE_SHOT  = 0,

    /*!
     * Synchronous mode. This mode, is a continuous measurement mode meaning that this needs to be stopped by calling \ref STALS_Stop.
     * But an handshake is necessary to have the registers updated with the values of the on going measurement. This handshake is performed by 
     * the \ref STALS_GetAlsValues function\n
     * Two options are available for the handshake \n
     * 1. by I2C reading. in this case the AC data is available on the GPIO pin
     * 2. by interrupt acknowledgment. in this case the AC data is NOT available 
     */    
    STALS_MODE_ALS_SYNCHRONOUS  = 1,

    /*!
     * Flicker mode. This mode outputs PDM on the GPIO or CLKIN pin.
     */    
    STALS_MODE_FLICKER          = 2
};


/** 
 * @enum STALS_Mode_t 
 * 
 * This enumeration is aimed at defining the different behavior modes of the STALS device
 */
enum STALS_FlickerOutputType_t
{
    STALS_FLICKER_OUTPUT_ANALOG = 0,            //!< Analog. DEPRECATED, use STALS_FLICKER_OUTPUT_ANALOG_CFG_1 instead.
    STALS_FLICKER_OUTPUT_ANALOG_CFG_1 = 0,      //!< Analog. PDM is output to GPIO through pad internal resistance.
    STALS_FLICKER_OUTPUT_DIGITAL_PDM = 1,       //!< PDM Digital. This modes needs an external clock to feed the device through the corresponding pin.
    STALS_FLICKER_OUTPUT_ANALOG_CFG_2 = 2,      //!< Analog. PDM is output to CLKIN. Not supported by vd6281 cut2.0.
};


/** 
 * @struct STALS_Als_t 
 * 
 * This structure is aimed at defining the parameters providing the event counts values of the last light integration, for the selected channels
 */
struct STALS_Als_t {
    uint8_t Channels;                                //!< Flag to be ORED by the driver client in order to understand what are the channels that provide a valid value. 0x3F means that all the channels are valid
    uint32_t CountValue[STALS_ALS_MAX_CHANNELS];     //!< Array providing the event counts value for each of the selected channels. This is value after per device calibration.
    uint32_t CountValueRaw[STALS_ALS_MAX_CHANNELS];  //!< Array providing the event counts value for each of the selected channels. This is value before per device calibration.
};


/**
 * This function Initializes the STALS driver 
 *
 * @param pDeviceName                      Name of the device. Shall be VD6281
 * @param pClient                          Pointer on an client specific plaform specific structure, provided up to the underlying plaform adaptation layers
 * @param pHandle                          Pointer on an opaque pointer to be used as the id of the instance of the driver
 *
 * @note SlaveAddress. If set to the default address of the device, then the irq pin does not need to be set to low.\n
 * WARNING : to set a new I2C slave address, the GPIO pin MUST be set to low and the Init function will then perform an I2C transaction with this I2C slave address. This transaction, being the first one after the power up of the device,
 * will set this new slave address and all further I2C address shall be performed with this I2C slave address.
 * 
 * @note A call to the \ref STALS_Init function shall be done to set the VD6281 device in IDLE mode.
 * @note The VD6281 device needs a delay between powering it and calling \ref STALS_Init. Please refer to the device user manual for more details.
 * 
 * \retval  STALS_NO_ERROR                Success
 * \retval  STALS_ERROR_INIT              Could not initialize the driver
 * \retval  STALS_ERROR_INVALID_PARAMS    At least one of the provided parameters to the function is invalid
 * \retval  STALS_ERROR_WRITE             Could not write any data into the device through I2C
 */
STALS_ErrCode_t STALS_Init(char * pDeviceName, void * pClient, void ** pHandle);

/**
 * This function terminates the provided STALS driver instance
 *
 * @param pHandle                          Opaque pointer used as the id of the instance of the driver
 *
 * \retval  STALS_NO_ERROR                 Success
 * \retval  STALS_ERROR_INVALID_PARAMS     At least one of the provided parameters to the function is invalid
 */
STALS_ErrCode_t STALS_Term(void * pHandle);

/**
 * This function .......
 *
 * @param pVersion                         Pointer on a value that contains the version of the driver once this function is called
 * @param pRevision                        Pointer on a value that contains the revision of the driver once this function is called
 *
 * @note The MAJOR number of version sits in the upper 16 bits of *pVersion, the MINOR number of version sits in the lower 16 bits of *pVersion
 * The *pRevision value contains the revision of the driver
 * 
 * \retval  STALS_NO_ERROR                 Success
 * \retval  STALS_ERROR_INVALID_PARAMS     At least one of the provided parameters to the function is invalid
 */
STALS_ErrCode_t STALS_GetVersion(uint32_t *pVersion, uint32_t *pRevision);


/**
 * This function returns the color filter of the provided channel identifier
 *
 * @param pHandle                          Opaque pointer used as the id of the instance of the driver
 * @param ChannelId                        Channel Id. Permitted values are 0 up to \ref STALS_ALS_MAX_CHANNELS - 1
 * @param pColor                           Pointer on a value in which the color of the channel is returned
 *
 * \retval  STALS_NO_ERROR                 Success
 * \retval  STALS_ERROR_INVALID_PARAMS     At least one of the provided parameters to the function is invalid
 */
STALS_ErrCode_t STALS_GetChannelColor(void * pHandle, enum STALS_Channel_Id_t ChannelId, enum STALS_Color_Id_t * pColor);

/**
 * This function sets the exposure time into the device, after having tuned to the closest value that the device can support.
 * Note that a fixed readout period of ~6 ms takes place just after exposure time, needed by the device to set the event count values in the registers
 * It also returns the actual applied value in the device
 *
 * For VD6281 cut 2.0 possible values are multiples of 1.6 ms with a range of 1.6 ms to 1.6 s.
 *
 * @note Note Exposure time is irrelevant for flicker detection.
 * 
 * @param pHandle                          Opaque pointer used as the id of the instance of the driver
 * @param ExpoTimeInUs                     Exposure time in microseconds
 * @param pAppliedExpoTimeUs               Pointer on in which the value of the actual exposure time is returned
 * 
 * \retval  STALS_NO_ERROR                 Success
 * \retval  STALS_ERROR_INVALID_PARAMS     At least one of the provided parameters to the function is invalid
 * \retval  STALS_ERROR_ALREADY_STARTED    Exposure can not be set when the device is running
 * \retval  STALS_ERROR_WRITE              Could not write any data into the device through I2C
 */
STALS_ErrCode_t STALS_SetExposureTime(void * pHandle, uint32_t ExpoTimeInUs, uint32_t *pAppliedExpoTimeUs);

/**
 * This function returns the actual exposure time
 *
 * @param pHandle                          Opaque pointer used as the id of the instance of the driver
 * @param pAppliedExpoTimeUs               Pointer in which the value of the actual exposure time is returned
 *
 * \retval  STALS_NO_ERROR                 Success
 * \retval  STALS_ERROR_INVALID_PARAMS     At least one of the provided parameters to the function is invalid
 */
STALS_ErrCode_t STALS_GetExposureTime(void * pHandle, uint32_t *pAppliedExpoTimeUs);

/**
 * This function sets an inter measurement time into the device, after having tuned to the closest value that the device can support.
 * It also returns the actual applied value in the device
 *
 * @param pHandle                          Opaque pointer used as the id of the instance of the driver
 * @param InterMeasurmentInUs              Inter measurement
 * @param pAppliedInterMeasurmentInUs      Pointer on in which the value of the actual inter measurement time is returned
 *
 * \retval  STALS_NO_ERROR                 Success
 * \retval  STALS_ERROR_INVALID_PARAMS     At least one of the provided parameters to the function is invalid
 * \retval  STALS_ERROR_ALREADY_STARTED    Inter measurement can not be set when the device is running
 * \retval  STALS_ERROR_WRITE              Could not write any data into the device through I2C
 */
STALS_ErrCode_t STALS_SetInterMeasurementTime(void * pHandle, uint32_t InterMeasurmentInUs, uint32_t *pAppliedInterMeasurmentInUs);

/**
 * This function returns the actual inter measurement
 *
 * @param pHandle                          Opaque pointer used as the id of the instance of the driver
 * @param pAppliedInterMeasurmentInUs      Pointer on in which the value of the actual inter measurement time is returned
 *
 * \retval  STALS_NO_ERROR                 Success
 * \retval  STALS_ERROR_INVALID_PARAMS     At least one of the provided parameters to the function is invalid
 */
STALS_ErrCode_t STALS_GetInterMeasurementTime(void * pHandle, uint32_t *pAppliedInterMeasurmentInUs);


/**
 * This function returns the version of the device
 *
 * @param Handle                           Handle on the driver instance
 * @param pDeviceID                        Pointer in which the ID of the device is returned 
 * @param pRevisionID                      Pointer in which the revision of the device is returned 
 *
 * \retval  STALS_NO_ERROR                 Success
 * \retval  STALS_ERROR_INVALID_PARAMS     At least one of the provided parameters to the function is invalid
 * \retval  STALS_ERROR_WRITE              Could not write any data into the device through I2C
 */
STALS_ErrCode_t STALS_GetProductVersion(void * pHandle, uint8_t *pDeviceID, uint8_t *pRevisionID );

/**
 * This function sets an analog gain on the provided channel id.
 *
 * @note The gain impacts the signal output amplitude, but not the values returned by STALS_GetFlicker, 
 * unless the gain is exceeds a maximum value that will flatten the signal output because of its impact on saturation
 *
 * @param pHandle                          Opaque pointer used as the id of the instance of the driver
 * @param ChannelId                        this id idenfiies the channel number. See \ref STALS_Channel_Id_t 
 * @param Gain                             Gain in 8.8 fixed point unit
 * @param pAppliedGain                     Pointer in which the value of the actual gain applied in the device is returned. Value in 8.8 fixed point unit
 *
 * \retval  STALS_NO_ERROR                 Success
 * \retval  STALS_ERROR_INVALID_PARAMS     At least one of the provided parameters to the function is invalid
 * \retval  STALS_ERROR_ALREADY_STARTED    Gain can not be set when the device is running
 * \retval  STALS_ERROR_WRITE              Could not write any data into the device through I2C
 * \retval  STALS_ERROR_READ               Could not read any data from the device through I2C
 */
STALS_ErrCode_t STALS_SetGain(void * pHandle, enum STALS_Channel_Id_t ChannelId, uint16_t Gain, uint16_t *pAppliedGain);

/**
 * This function gets the actual gain applied in the device
 *
 * @param pHandle                          Opaque pointer used as the id of the instance of the driver
 * @param ChannelId                        this Id idenfiies the channel number. see \ref STALS_Channel_Id_t 
 * @param pAppliedGain                     Pointer in which the value of the actual gain applied in the device is returned. Value in 8.8 fixed point unit
 *
 * \retval  STALS_NO_ERROR                 Success
 * \retval  STALS_ERROR_INVALID_PARAMS     At least one of the provided parameters to the function is invalid
 */
STALS_ErrCode_t STALS_GetGain(void * pHandle, enum STALS_Channel_Id_t ChannelId, uint16_t *pAppliedGain);

/**
 * This function set CLKIN and GPIO gpio's settings according to FlickerOutputType. Note that those parameters are
 * effectively set when flicker is started.
 *
 * @param pHandle                          Opaque pointer used as the id of the instance of the driver
 * @param FlickerOutputType                Data ouptut type for the flicker mode. see \ref STALS_FlickerOutputType_t
 * 
 * \retval  STALS_NO_ERROR                 Success
 * \retval  STALS_ERROR_INVALID_PARAMS     At least one of the provided parameters to the function is invalid
 * \retval  STALS_ERROR_WRITE              Could not write any data into the device through I2C
 * \retval  STALS_ERROR_READ               Could not read any data from the device through I2C
 */
STALS_ErrCode_t STALS_SetFlickerOutputType(void * pHandle, enum STALS_FlickerOutputType_t FlickerOutputType);
 
/**
 * This function starts the device
 *
 * @param pHandle                          Opaque pointer used as the id of the instance of the driver
 * @param Mode                             Mode. shall be \ref STALS_MODE_ALS_SINGLE_SHOT, \ref STALS_MODE_ALS_SYNCHRONOUS or \ref STALS_MODE_FLICKER\n
 * @param Channels.                        For the ALS modes, this is an ORED value of the \ref STALS_Channel_Id_t channels.
 *                                         For the flicker mode, this is one of the \ref STALS_Channel_Id_t channels.\n 
 *
 * @note As the ALS and FLICKER modes can run independently, Two consecutive calls to this START function with \ref STALS_MODE_ALS_SYNCHRONOUS and \ref STALS_MODE_FLICKER modes are permitted.
 * 
 * \retval  STALS_NO_ERROR                 Success
 * \retval  STALS_ERROR_INVALID_PARAMS     At least one of the provided parameters to the function is invalid
 * \retval  STALS_ALREADY_STARTED          The device is already running in the provided mode or in an incompatible mode
 * \retval  STALS_ERROR_WRITE              Could not write any data into the device through I2C
 */
STALS_ErrCode_t STALS_Start(void * pHandle, enum STALS_Mode_t Mode, uint8_t Channels);

/**
 * This function stops the device
 *
 * @param pHandle                          Opaque pointer used as the id of the instance of the driver
 * @param Mode                             Mode. shall be \ref STALS_MODE_ALS_SINGLE_SHOT, \ref STALS_MODE_ALS_SYNCHRONOUS or \ref STALS_MODE_FLICKER.
 *
 * @note note
 * 
 * \retval  STALS_NO_ERROR                 Success
 * \retval  STALS_ERROR_INVALID_PARAMS     At least one of the provided parameters to the function is invalid
 * \retval  STALS_ALREADY_NOT_STARTED      The device is not running the provided mode
 * \retval  STALS_ERROR_WRITE              Could not write any data into the device through I2C
 */
STALS_ErrCode_t STALS_Stop(void * pHandle, enum STALS_Mode_t Mode);

/**
 * This function that provides the event counts values for the selected channels
 *
 * @param pHandle                          Opaque pointer used as the id of the instance of the driver
 * @param Channels                         an ORED value of the \ref STALS_Channel_Id_t that permits to select the channels from which the event counts values are to be retrieved.
 * @param pAlsValue                        Pointer on a structure storing the counted events values
 * @param pMeasureValid                    Pointer on a flag telling if the measurement is valid. 
 *
 * \retval  STALS_NO_ERROR                 Success
 * \retval  STALS_ERROR_INVALID_PARAMS     At least one of the provided parameters to the function is invalid
 * \retval  STALS_ERROR_WRITE              Could not write any data into the device through I2C
 * \retval  STALS_ERROR_READ               Could not read any data from the device through I2C
 */
STALS_ErrCode_t STALS_GetAlsValues(void * pHandle, uint8_t Channels, struct STALS_Als_t * pAlsValue, uint8_t * pMeasureValid);


/**
 * This function Gets the flicker main harmonic frequency
 *
 * @param pHandle                          Opaque pointer used as the id of the instance of the driver
 * @param pFlickerInfo                     A pointer on an \ref STALS_FlickerInfo_t structure 
 *
 * \retval  STALS_NO_ERROR                 Success
 * \retval  STALS_ERROR_INVALID_PARAMS     At least one of the provided parameters to the function is invalid
 * \retval  STALS_ERROR_READ               Could not read any data from the device through I2C
 */
STALS_ErrCode_t STALS_GetFlickerFrequency(void * pHandle, struct STALS_FlickerInfo_t * pFlickerInfo);


/**
 * This function sets a control to the STALS driver
 *
 * @param pHandle                          Opaque pointer used as the id of the instance of the driver
 * @param ParamId                          Identifier of the param provided
 * @param ControlValue                     The value of the control 
 *
 * \retval  STALS_NO_ERROR                 Success
 * \retval  STALS_ERROR_INVALID_PARAMS     At least one of the provided parameters to the function is invalid
 */
STALS_ErrCode_t STALS_SetControl(void * pHandle, enum STALS_Control_Id_t ControlId, uint32_t ControlValue);

/**
 * This function gets a control from the STALS driver
 *
 * @param pHandle                          Opaque pointer used as the id of the instance of the driver
 * @param ParamId                          Identifier of the param provided
 * @param pControlValue                    Pointer on a parameter in which the parameter value is set
 *
 * \retval  STALS_NO_ERROR                 Success
 * \retval  STALS_ERROR_INVALID_PARAMS     At least one of the provided parameters to the function is invalid
*/
STALS_ErrCode_t STALS_GetControl(void * pHandle, enum STALS_Control_Id_t ControlId, uint32_t * pControlValue);

/* following functions must be implemented by platform integrator */
STALS_ErrCode_t STALS_WrByte(void *pClient, uint8_t index, uint8_t data);
STALS_ErrCode_t STALS_RdByte(void *pClient, uint8_t index, uint8_t *data);
/* following function is optional. There is a default weak implementation */
STALS_ErrCode_t STALS_RdMultipleBytes(void *pClient, uint8_t index, uint8_t *data, int nb);

#ifndef BIT
#define BIT(b)							(1UL << (b))
#endif

#define VD6281_DEVICE_ID					0x00
#define VD6281_REVISION_ID					0x01
#define VD6281_IRQ_CTRL_ST					0x02
#define VD6281_ALS_CTRL						0x03
#define VD6281_ALS_START					BIT(0)
#define VD6281_ALS_CONTINUOUS					BIT(1)
#define VD6281_ALS_CONTINUOUS_SLAVED				BIT(2)
#define VD6281_CONTINUOUS_PERIOD				0x04

#define VD6281_CHANNELx_MM(c)					(0x06 + 4 * (c))
#define VD6281_CHANNELx_LM(c)					(0x07 + 4 * (c))
#define VD6281_CHANNELx_LL(c)					(0x08 + 4 * (c))

#define VD6281_EXPOSURE_M					0x1d
#define VD6281_EXPOSURE_L					0x1e
#define VD6281_CHANNEL_VREF(chan)				(0x25 + (chan))

#define VD6281_AC_EN						0x2D
#define VD6281_DC_EN						0x2E
#define VD6281_AC_CLAMP_EN					0x2F
#define VD6281_DC_CLAMP_EN					0x30
#define VD6281_AC_MODE						0x31
#define AC_EXTRACTOR						BIT(0)
#define AC_EXTRACTOR_ENABLE					BIT(0)
#define AC_EXTRACTOR_DISABLE					0
#define AC_CHANNEL_SELECT					0x0e
#define PDM_SELECT_OUTPUT					BIT(4)
#define PDM_SELECT_GPIO						0
#define PDM_SELECT_CLKIN					BIT(4)
#define PDM_SELECT_CLK						BIT(5)
#define PDM_SELECT_INTERNAL_CLK					0
#define PDM_SELECT_EXTERNAL_CLK					BIT(5)
#define AC_PEDESTAL						BIT(6)
#define AC_PEDESTAL_ENABLE					0
#define AC_PEDESTAL_DISABLE					BIT(6)
#define VD6281_AC_PEDESTAL					0x32
#define VD6281_PEDESTAL_VALUE_MASK				0x07
#define VD6281_AC_SAT_METRIC_M					0x33
#define VD6281_AC_SAT_METRIC_L					0x34
#define VD6281_AC_ACC_PERIODS_M					0x35
#define VD6281_AC_ACC_PERIODS_L					0x36
#define VD6281_AC_NB_PERIODS					0x37
#define VD6281_AC_AMPLITUDE_M					0x38
#define VD6281_AC_AMPLITUDE_L					0x39

#define VD6281_SDA_DRIVE					0x3c
#define VD6281_SDA_DRIVE_MASK					0x07
#define VD6281_OSC10M						0x3d
#define VD6281_OSC10M_TRIM_M					0x3e
#define VD6281_OSC10M_TRIM_L					0x3f
#define VD6281_OSC50K_TRIM					0x40

#define VD6281_INTR_CFG						0x41
#define VD6281_DTEST_SELECTION					0x47

#define VD6281_SEL_PD_x(c)					(0x6B + (c))

#define VD6281_OTP_CTRL1					0x58

#define VD6281_OTP_STATUS					0x5a
#define VD6281_OTP1_DATA_READY					BIT(1)
#define VD6281_OTP2_DATA_READY					BIT(3)
#define VD6281_OTP_DATA_READY					(VD6281_OTP1_DATA_READY | VD6281_OTP2_DATA_READY)

#define VD6281_OTP_BANK_0					0x5b
#define VD6281_OTP_BANK_1					0x63

#define VD6281_SPARE_1						0x72

#define VD6281_GLOBAL_RESET					0xFE

#define VD6281_CHANNEL_NB			STALS_ALS_MAX_CHANNELS
#define PWR_ON		1
#define PWR_OFF		0
#define PM_RESUME	1
#define PM_SUSPEND	0
#define NAME_LEN		32

#ifndef VD6281_CONFIG_DEVICES_MAX
#define VD6281_CONFIG_DEVICES_MAX		1
#endif

#define FLICKER_DATA_CNT	200
#define VD6281_IOCTL_MAGIC		0xFD
#define VD6281_IOCTL_READ_FLICKER	_IOR(VD6281_IOCTL_MAGIC, 0x01, int *)

#define VD6281_ADC_TIMEOUT	(msecs_to_jiffies(100))


/* #define ALS_DBG */
/* #define ALS_INFO */

#ifndef ALS_dbg
#ifdef ALS_DBG
#define ALS_dbg(format, arg...)		\
				printk(KERN_DEBUG "ALS_dbg : "format, ##arg)
#define ALS_err(format, arg...)		\
				printk(KERN_DEBUG "ALS_err : "format, ##arg)
#else
#define ALS_dbg(format, arg...)		{if (als_debug)\
				printk(KERN_DEBUG "ALS_dbg : "format, ##arg);\
					}
#define ALS_err(format, arg...)		{if (als_debug)\
				printk(KERN_DEBUG "ALS_err : "format, ##arg);\
					}
#endif
#endif

#ifndef ALS_info
#ifdef ALS_INFO
#define ALS_info(format, arg...)	\
				printk(KERN_INFO "ALS_info : "format, ##arg)
#else
#define ALS_info(format, arg...)	{if (als_info)\
				printk(KERN_INFO "ALS_info : "format, ##arg);\
					}
#endif
#endif

enum {
	DEBUG_REG_STATUS = 1,
	DEBUG_VAR,
};

struct mode_count {
	s32 hrm_cnt;
	s32 amb_cnt;
	s32 prox_cnt;
	s32 sdk_cnt;
	s32 cgm_cnt;
	s32 unkn_cnt;
};

enum platform_pwr_state {
	POWER_ON,
	POWER_OFF,
	POWER_STANDBY,
};

enum dev_state {
	DEV_FREE = 0,
	DEV_INIT,
	DEV_ALS_RUN,
	DEV_FLICKER_RUN,
	DEV_BOTH_RUN
};

enum  {
	EOL_STATE_INIT = -1,
	EOL_STATE_100,
	EOL_STATE_120,
	EOL_STATE_DONE
};

struct VD6281_device {
	enum dev_state st;
	enum STALS_Mode_t als_started_mode;
	void *client;
	void *hdl;
	uint8_t device_id;
	uint8_t revision_id;
	uint8_t dc_chan_en;
	uint8_t ac_chan_en;
	struct {
		uint8_t chan;
	} als;
	struct {
		uint8_t chan;
	} flk;
	uint16_t gains[VD6281_CHANNEL_NB];
	uint32_t exposure;
	enum STALS_FlickerOutputType_t flicker_output_type;
	enum STALS_Control_t is_otp_usage_enable;
	enum STALS_Control_t is_output_dark_enable;
	uint64_t otp_bits[2];
	struct {
		uint16_t hf_trim;
		uint8_t lf_trim;
		uint8_t filter_config;
		uint8_t gains[STALS_ALS_MAX_CHANNELS];
	} otp;
	uint8_t wa_codex_460857;
	uint8_t wa_codex_462997;
	uint8_t wa_codex_461428;
	uint8_t wa_codex_464174;
	uint8_t wa_codex_478654;
};

struct vd6281_device_data {
	struct i2c_client *client;
	struct device *dev;
	struct input_dev *als_input_dev;
	struct mutex i2clock;
	struct mutex activelock;
	struct mutex suspendlock;
	struct mutex flickerdatalock;
	struct miscdevice miscdev;
	struct pinctrl *als_pinctrl;
	struct pinctrl_state *pins_sleep;
	struct pinctrl_state *pins_idle;
	struct delayed_work rear_als_work;
	struct workqueue_struct *flicker_work;
	struct iio_channel *chan;
	struct tasklet_hrtimer timer;
	struct VD6281_device devices;
	struct completion	completion;
	char *vdd_1p8;
	char *i2c_1p8;
	u8 enabled;
	u32 sampling_period_ns;
	u8 regulator_state;
	s32 pin_als_int;
	s32 pin_als_en;
	s32 dev_irq;
	u8 irq_state;
	u32 reg_read_buf;
	u8 debug_mode;
	struct mode_count mode_cnt;
#ifdef CONFIG_ARCH_QCOM
	struct pm_qos_request pm_qos_req_fpm;
#endif
	bool pm_state;
	int isTrimmed;

	u8 part_type;
	u8 part_revision;
	u32 i2c_err_cnt;
	u32 user_ir_data;
	u32 user_flicker_data;
	u8 is_als_start;
	u8 is_flk_start;
	char *eol_result;
	u8 eol_enable;
	u8 eol_result_status;
	s16 eol_state;
	u32 eol_count;
	u32 eol_awb;
	u32 eol_clear;
	u32 eol_flicker;
	u8 eol_flicker_count;
	u32 eol_flicker_awb[6][3];
	u32 eol_pulse_duty[2];
	u32 eol_pulse_count;
	u32 eol_ir_spec[4];
	u32 eol_clear_spec[4];
	s32 pin_led_en;
	int flicker_data[300];
	int flicker_cnt;

#ifdef CONFIG_STM_ALS_EOL_MODE
	u16 awb_sample_cnt;
//	int *flicker_data;
	int flicker_data_cnt;
	u8 fifodata[256];
#endif
};



STALS_ErrCode_t set_channel_gain(struct VD6281_device *dev, int c, uint16_t Gain);
STALS_ErrCode_t set_pedestal_value(struct VD6281_device *dev, uint32_t value);
void vd6281_put_device(struct VD6281_device *dev);
struct VD6281_device *vd6281_get_device(void **pHandle);
void vd6281_flicker_work_cb(struct work_struct *w);

#ifdef CONFIG_ARCH_QCOM
extern int sensors_create_symlink(struct kobject *target, const char *name);
extern void sensors_remove_symlink(struct kobject *target, const char *name);
extern int sensors_register(struct device **dev, void *drvdata,
	struct device_attribute *attributes[], char *name);
#else
extern int sensors_create_symlink(struct input_dev *inputdev);
extern void sensors_remove_symlink(struct input_dev *inputdev);
extern int sensors_register(struct device *dev, void *drvdata,
	struct device_attribute *attributes[], char *name);
#endif
extern void sensors_unregister(struct device *dev,
	struct device_attribute *attributes[]);

extern unsigned int lpcharge;

#endif /* __STALS_VD6281_H */
