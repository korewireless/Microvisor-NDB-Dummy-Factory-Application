/**
 *
 * Microvisor Network Helper
 * Version 1.0.0
 * Copyright © 2022, Twilio
 * Licence: Apache 2.0
 *
 */


#include "network.h"
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include "stm32u5xx_hal.h"
#include "messages.h"

#define NETWORK_NOTIFICATION_BUFFER_COUNT 8
// If you change the IRQ here, you must also rename the Handler function at the bottom of the page
// which the HAL calls.
#define NETWORK_NOTIFICATION_IRQ TIM1_BRK_IRQn

#define USER_TAG_REQUEST_NETWORK 1
#define TAG_CHANNEL_HTTP 2

static volatile struct MvNotification notification_buffer[NETWORK_NOTIFICATION_BUFFER_COUNT] __attribute__((aligned(8)));
static volatile uint32_t              current_notification_index = 0;
static MvNotificationHandle           notification_center_handle = 0;

static MvNetworkHandle                network_handle = 0;
static volatile bool                  network_status_changed = false;
static volatile bool                  solicited_change = false;

static volatile bool                   want_network = false;
static volatile bool                  _have_network = false;

static uint8_t http_receive_buffer[4096] __attribute__ ((aligned(512)));
static uint8_t http_send_buffer[512] __attribute__ ((aligned(512)));

static void configure_network();
static void configure_notification_center();
static void spin_network();
static void release_network();
static bool have_network();

static QueueHandle_t in_queue;
static QueueHandle_t out_queue;

static void handle_messages();
static void spin_network();

/**
 * @brief Function implementing the Network task thread.
 *
 * @param  argument: instance of struct NetworkTaskArgument
 */
void start_network_task(void *argument) {
    struct NetworkTaskArgument *typed_argument = (struct NetworkTaskArgument*) argument;
    uint32_t last_tick = 0;

    out_queue = typed_argument->out_queue;
    in_queue = typed_argument->in_queue;

    // The task's main loop
    while (1) {
        handle_messages();

        // Get the ms timer value
        uint32_t tick = HAL_GetTick();
        if (tick - last_tick > 500) {
            last_tick = tick;

            spin_network();
        }

        // End of cycle delay
        vTaskDelay(have_network() ? 1000 : 100);
    }
}


static void handle_messages() {
    enum Message message = NoneMessage;

    while (xQueueReceive(in_queue, &message, 0)) {
        switch (message) {
            case JoinNetworkMessage:
                want_network = true;
                break;
            case DropNetworkMessage:
                want_network = false;
                break;
            default:
                break;
        }
    }
}

/**
 * @brief Call regularly from host application to allow for handling of network status.
 */
static void spin_network() {
    enum MvNetworkStatus network_status;

    if (mvGetNetworkStatus(network_handle, &network_status) == MV_STATUS_OKAY) {
        bool new_have_network = (network_status == MV_NETWORKSTATUS_CONNECTED);
        if (new_have_network != _have_network) {
            enum Message message = new_have_network ? NetworkConnectedMessage : NetworkDisconnectedMessage;

            xQueueSend(out_queue, &message, 0);
        }
        _have_network = new_have_network;
    }

    // The interrupt is only triggered when status changes after boot up, since network is likely
    // connected by Microvisor by the time the application starts, this won't get fired until we
    // lose connection in most cases.
    if (network_status_changed) {
        network_status_changed = false;
        solicited_change = false;
    }

    if (want_network && !have_network()) {
        if (network_handle == 0) {
            configure_network();
        }
    } else if (!want_network && network_handle != 0) {
        release_network();
    }
}


/*
 * @brief Indicates if we have a current network connection or not.
 */
static inline bool have_network() {
    return _have_network;
}


/*
 * @brief Returns the current network handle if one is available.
 */
MvNetworkHandle get_network_handle() {
    return network_handle;
}


/**
 * @brief Release existing network connection if we own it, otherwise do nothing.
 */
void release_network() {
    want_network = false;

    if (network_handle == 0) {
        return;
    }

    enum MvStatus status = mvReleaseNetwork(&network_handle);
    assert(status == MV_STATUS_OKAY);
}


/**
 * @brief Obtain network handle and configure a notification center for networking events.
 */
static void configure_network() {
    want_network = true;

    if (network_handle != 0) { // we already have a conection handle, drop out
        return;
    }

    configure_notification_center();

    struct MvRequestNetworkParams network_config = {
        .version = 1,
        .v1 = {
            .notification_handle = notification_center_handle,
            .notification_tag = USER_TAG_REQUEST_NETWORK
        }
    };

    // Ask Microvisor for an active network connection.  If a connection exists no
    // change will happen.  If a connection does not exist, Microvisor will attempt
    // to connect.
    enum MvStatus status = mvRequestNetwork(&network_config, &network_handle);
    assert(status == MV_STATUS_OKAY);
}


/**
 * @brief Configure a notification center to use for network events.
 */
static void configure_notification_center() {
    if (notification_center_handle != 0) {
        return;
    }

    memset((void *)notification_buffer, 0xff, sizeof(notification_buffer));

    static struct MvNotificationSetup notification_config = {
        .irq = NETWORK_NOTIFICATION_IRQ,
        .buffer = (struct MvNotification *)notification_buffer,
        .buffer_size = sizeof(notification_buffer)
    };

    // Configure notifications with Microvisor
    enum MvStatus status = mvSetupNotifications(&notification_config, &notification_center_handle);
    assert(status == MV_STATUS_OKAY);

    // Enable the notification IRQ via the HAL
    NVIC_ClearPendingIRQ(NETWORK_NOTIFICATION_IRQ);
    NVIC_EnableIRQ(NETWORK_NOTIFICATION_IRQ);
}


/**
 * @brief Handle network notification events
 */
/**
 *  @brief Network notification ISR.
 */
void TIM1_BRK_IRQHandler(void) {
    volatile struct MvNotification *notification = &notification_buffer[current_notification_index];
    if (notification->event_type == MV_EVENTTYPE_NETWORKSTATUSCHANGED) {
        network_status_changed = true;
        solicited_change = (notification->tag == USER_TAG_REQUEST_NETWORK);
    }

    // Point to the next record to be written
    current_notification_index = (current_notification_index + 1) % NETWORK_NOTIFICATION_BUFFER_COUNT;

    // Clear the current notifications event
    // See https://www.twilio.com/docs/iot/microvisor/microvisor-notifications#buffer-overruns
    notification->event_type = 0;
}

enum MvStatus open_http_channel(MvChannelHandle *handle_out) {
    struct MvOpenChannelParams ch_params = {
        .version = 1,
        .v1 = {
            .notification_handle = notification_center_handle,
            .notification_tag = TAG_CHANNEL_HTTP,
            .network_handle = network_handle,
            .receive_buffer = (uint8_t*)http_receive_buffer,
            .receive_buffer_len = sizeof(http_receive_buffer),
            .send_buffer = (uint8_t*)http_send_buffer,
            .send_buffer_len = sizeof(http_send_buffer),
            .channel_type = MV_CHANNELTYPE_HTTP,
	    .endpoint = {
                .data = (uint8_t*) "dummy",
		.length = 5
	    }
        }
    };

    return mvOpenChannel(&ch_params, handle_out);
}
