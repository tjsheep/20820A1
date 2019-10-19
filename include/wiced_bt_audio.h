/*
 * Copyright 2019, Cypress Semiconductor Corporation or a subsidiary of
 * Cypress Semiconductor Corporation. All Rights Reserved.
 *
 * This software, including source code, documentation and related
 * materials ("Software"), is owned by Cypress Semiconductor Corporation
 * or one of its subsidiaries ("Cypress") and is protected by and subject to
 * worldwide patent protection (United States and foreign),
 * United States copyright laws and international treaty provisions.
 * Therefore, you may use this Software only as provided in the license
 * agreement accompanying the software package from which you
 * obtained this Software ("EULA").
 * If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
 * non-transferable license to copy, modify, and compile the Software
 * source code solely for use in connection with Cypress's
 * integrated circuit products. Any reproduction, modification, translation,
 * compilation, or representation of this Software except as specified
 * above is prohibited without the express written permission of Cypress.
 *
 * Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
 * reserves the right to make changes to the Software without notice. Cypress
 * does not assume any liability arising out of the application or use of the
 * Software or any product or circuit described in the Software. Cypress does
 * not authorize its products for use in any products where a malfunction or
 * failure of the Cypress product may reasonably be expected to result in
 * significant property damage, injury or death ("High Risk Product"). By
 * including Cypress's product in a High Risk Product, the manufacturer
 * of such system or application assumes all risk of such use and in doing
 * so agrees to indemnify Cypress against all liability.
 */

/** @file
 *
 * @addtogroup  wicedbt_audio_utils     WICED Audio Utilities
 * @ingroup     wicedbt_av
 *
 * @{
 */
#ifndef _WICED_BT_AUDIO_H_
#define _WICED_BT_AUDIO_H_

#include "wiced_bt_a2d.h"
#include "wiced_bt_a2d_sbc.h"

/*****************************************************************************
**           Constants
*****************************************************************************/

/** wiced audio routes */
typedef enum
{
    AUDIO_ROUTE_I2S                  = 0x00,  /**< Route the PCM Samples over I2S. Read from I2S in case of audio source,write to I2S in case of audio sink */
    AUDIO_ROUTE_UART                 = 0x01,  /**< Route the PCM samples over transport. Receive the audio data to be sent OTA from transport in case of audio source.Receive the audio data OTA, decode and send to transport in case of audio sink */
    AUDIO_ROUTE_SINE                 = 0x02,  /**< Route the stored sine samples over the air. Applicable in case of audio source */
    AUDIO_ROUTE_APP                  = 0x03,  /**< Route the PCM samples to app.  Receive the audio data OTA, decode and send to app. Applicable in case of audio sink */
    AUDIO_ROUTE_COMPRESSED_TRANSPORT = 0x04,  /**< Route the compressed audio data(AVDTP media packet, including the header) over transport. Receive the audio data OTA and send to transport. Applicable in case of audio sink */
    AUDIO_ROUTE_COMPRESSED_APP       = 0x05,  /**< Route the compressed audio data(AVDTP media packet, including the header) to app. Receive the audio data OTA and send to transport. Applicable in case of audio sink */
}wiced_audio_route_t;

/** wiced device roles */
typedef enum
{
    WICED_AUDIO_SOURCE_ROLE = 1 << 1,   /**< Audio Source */
    WICED_AUDIO_SINK_ROLE   = 1 << 2,   /**< Audio Sink */
    WICED_HF_ROLE           = 1 << 3,   /**< Handsfree */
}wiced_device_role_t;

/*****************************************************************************
 *          Type Definitions
 *****************************************************************************/
/** Audio buffer configuration structure
 *  Please refer wiced_audio_buffer_initialize API documentation for recommended size
 */
typedef struct
{
    wiced_device_role_t     role;                            /**< Role if audio source, sink or hf device */
    // Size of memory needed by the audio source device to store the Audio data received from a host MCU over UART/SPI transport before encoding and transmitting over BT radio.
    //Or Size of the memory needed by the audio sink device to send the received audio data to the host MCU over transport
    uint32_t                audio_tx_buffer_size;           /**< Audio Tx buffer size */
    // Size of memory to allocate for use by codec and audio jitter buffer. Applicattion specifies the size based on the codecs supportted for various use cases. See wiced_audio_buffer_initialize()
    uint32_t                audio_codec_buffer_size;        /**< Codec buffer size */
    uint8_t                 audio_tx_buffer_watermark_level;/**< Watermark level in % in terms of % of buffers remaining. Default is 50 for 50%. E.g. if set to 70, fw will request additional data when buffer reaches 70% of the full buffer size (i.e., audio_tx_buffer_size). Only applicable for A2DP source applications */
} wiced_bt_audio_config_buffer_t;

/** A2DP statistics structure
 *
 * Note :
 *      packet_ack_stats_table [0] will give No of packets having delay between 0 - 9 msec
 *      packet_ack_stats_table [1] will give No of packets having delay between 10 -19 msec
 *      ...
 *      packet_ack_stats_table [9] will give No of packets having delay between 90 -99 msec
 *      packet_ack_stats_table [10] will give No of packets having delay > 100 msec
 */
typedef struct {
    uint32_t duration;                       /**< Duration of the a2dp streaming in which stats were captured */
    uint32_t packet_ack_stats_table[11];     /**< Delay between packet sent OTA and ack received */
} wiced_bt_a2dp_statistics_t;

/** Audio suspend complete callback */
typedef void ( *wiced_audio_suspend_complete_cback_t )( void );

/**
 * Audio buffer empty callback
 *
 * Callback invoked on  audio buffer empty event.
 * Registered using wiced_audio_register_buffer_empty_cback()
 *
 * @param total_intr_count      : Total number of i2s interrupts
 * @param total_empty_count      : Total number of times the audio buffer is empty
  * @param consecutive_empty_count             : Number of consecutive times the audio buffer is empty, resets on receiving audio data
 *
 */
typedef void ( *wiced_audio_buffer_empty_cback_t )( uint32_t total_intr_count,
                            uint32_t total_empty_count, uint32_t consecutive_empty_count );

/*****************************************************************************
 *          Function Prototypes
 *****************************************************************************/

/**
 * Function         wiced_audio_buffer_initialize
 *
 * Audio buffer initialization. Allocates the audio buffer and returns pointer to the audio buffer. This buffer
 * is used by the a2dp source to hold the audio data received over the transport from the host
 *
 * @param[in]      wiced_audio_cfg_pool        : Size of the audio tx buffer and codec buffer to be allocated
 *
 * @return         WICED_SUCCESS if Success, otherwise WICED_NO_MEMORY.
 *
 * NOTE :
 *      Recommended audio_tx_buffer_size : If the application includes A2DP source profile, the transport buffer size shall be set to at least 8KB.
 *                                         This buffer is not used for any other profiles and shall be set to 0 when A2DP source profile is not used.
 *
 *      Recommended audio_codec_buffer_size:
 *
 *  In case of A2DP sink profile, set the transport buffer size if required to send the audio data over the transport
 *       ----------------------------------------------------------------------------------
 *      |           USE CASE            |                Recommended  size                 |
 *      |----------------------------------------------------------------------------------|
 *      |   A2DP Source (SBC Encode)    |                       8KB                        |
 *      |   A2DP Sink(SBC Decode)*      |      11KB (300ms jitter buffer) + 8KB for codec  |
 *      |                               |                    Total 19KB                    |
 *      |   MSBC (WBS CODEC)*           |   13KB = 5KB(for Encoding)+ 8KB(for Decoding)    |
 *       ----------------------------------------------------------------------------------
 *       In case of multiple profiles used by application simultaneously, example A2DP sink and HFP device profiles,
 *            size specified should be the maximum of the recommended value for each profile supported.
 */
wiced_result_t wiced_audio_buffer_initialize( wiced_bt_audio_config_buffer_t wiced_audio_cfg_pool );

/**
 * Function         wiced_audio_start
 *
 * Used by the A2DP source to start the audio streaming. I2S interrupts are used for timing
 * the audio data by default. Software timer can also be used for timing the audio data which
 * can be enabled by using the API wiced_audio_use_sw_timing. Enable the software timer before
 * invoking wiced_audio_start
 *
 * @param[in]      is_master        : If the I2S has to configured as master when using I2S interrupts
 *                                             for timing the audio data. Set to 1 to configure I2S in the master
 *                                             mode. Set to 0 to configure the I2S in the slave mode.
 *
 * @param[in]      audio_route        : Audio route to be configured.
 *              Configure as AUDIO_ROUTE_I2S if the audio source is I2S
 *              Configure as AUDIO_ROUTE_UART if the audio source is FILE
 *              Configure as AUDIO_ROUTE_SINE if the audio source is the stored sine wave
 *
 * @param[in]      lcid        : lcid
 *
 * @param[in]      pSbc        : SBC codec info
 *
 * @return          void
 *
 */
void wiced_audio_start( int is_master, int audio_route, uint16_t lcid, wiced_bt_a2d_sbc_cie_t * pSbc );

/**
 * Function         wiced_audio_stop
 *
 * Used by the A2DP source to stop the audio streaming
 *
 * @param[in]      lcid        : lcid
 *
 * @return          void
 *
 */
void wiced_audio_stop( uint16_t lcid );

/**
 * Function         wiced_audio_suspend
 *
 * Used by the A2DP source to suspend the audio streaming
 *
 * @param[in]      lcid        : lcid
 *
 * @return          void
 *
 */
void wiced_audio_suspend ( uint16_t lcid, wiced_audio_suspend_complete_cback_t p_cback );

/**
 * Function         wiced_audio_register_buffer_empty_cback
 *
 * Register the callback function to be invoked on audio buffer empty event
 *
 * @param[in]      p_cback        : callback function to be invoked on audio buffer empty event
 *
 * @return          void
 *
 */
void wiced_audio_register_buffer_empty_cback( wiced_audio_buffer_empty_cback_t p_cback );

/**
 * Function         wiced_audio_use_sw_timing
 *
 * Used by the A2DP source to time the audio data using software timer. Sofware timer internally uses
 * slot timer to time the audio data. Period of the software timer is determined based on the chosen
 * audio codec sampling frequency and the number of samples.
 * Note:
 * I2S interrupts are used for timing the audio data by default. Software timer can also be
 * used for timing the audio data which can be enabled by using the API wiced_audio_use_sw_timing.
 * Enable the software timer before invoking wiced_audio_start.
 *
 * @param[in]      enable        : enables the sw timer
 *
 * @return          void
 *
 */
void wiced_audio_use_sw_timing( int enable );

/**
 * Function         wiced_audio_set_sinwave
 *
 * Used by the A2DP source to set the sine audio samples to be used in audio streaming when the
 * audio route is set to sine.
 *
 * @param[in]      pOut        : sine audio samples
 *
 * @return          void
 *
 */
void wiced_audio_set_sinwave( int16_t* pIn );

/**
 * Function         wiced_audio_get_statistics
 *
 * Used by the A2DP source to read A2DP streaming statistics
 *
 * @param[out]      buffer_stat        : pointer to fill statistics.
 *
 * @return          void
 *
 */
void wiced_audio_get_statistics (wiced_bt_a2dp_statistics_t *buffer_stat);

/**
* Function Name     wiced_audio_set_packet_types
*
* Sets the packet types used for a specific ACL connection and also sets the
* max_audio_mtu that the audio processing unit uses, based on the provided
* packet types, to format the SBC frames. By setting the max_audio_mtu according
* to the packet type, it helps to ensure no fragmentation across SBC frames
* occur.
*
* @param[in] peer_bda        Peer address of the desired ACL connection
* @param[in] packet_types    Packet type bitmask. See hcidef.h for definitions
*                            for packet type masks (BT1.2 and BT2.0 definitions).
*
* @return result             WICED_BT_PENDING is command has started successfully
*                            else wiced result error code.
*
*/
wiced_result_t wiced_audio_set_packet_types( BD_ADDR peer_bda, UINT16 packet_types );

#endif // _WICED_BT_AUDIO_H_
/** @} wicedbt_audio_utils */
