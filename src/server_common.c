/**
 * Copyright (c) 2023 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "btstack.h"
#include "hardware/adc.h"
#include "mpu6050_i2c.h"

#include "temp_sensor.h"
#include "server_common.h"

#define APP_AD_FLAGS 0x06 // General discoverable, BR/EDR not supported
static uint8_t adv_data[] = {
    // Flags general discoverable
    0x02, BLUETOOTH_DATA_TYPE_FLAGS, APP_AD_FLAGS,
    // Name
    0x0b, BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME, 'i', 'n', 'e', 'r', 't', 'i', 'a', 'l', ' ', '1',
    0x03, BLUETOOTH_DATA_TYPE_COMPLETE_LIST_OF_16_BIT_SERVICE_CLASS_UUIDS, 
    0x1a, 0x18, // Ox181A = environmental sensing service
    //0x10, 0xff // 0xFF10 = custom service
};
static const uint8_t adv_data_len = sizeof(adv_data);

int le_notification_enabled;
hci_con_handle_t con_handle;
uint16_t current_temp;
float accel[3], gyro[3], temp;
int counter = 0;

void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    UNUSED(size);
    UNUSED(channel);
    bd_addr_t local_addr;
    if (packet_type != HCI_EVENT_PACKET) return;

    uint8_t event_type = hci_event_packet_get_type(packet);
    switch(event_type){
        case BTSTACK_EVENT_STATE:
            if (btstack_event_state_get_state(packet) != HCI_STATE_WORKING) return;
            bd_addr_t local_addr_ = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x0c };
            gap_random_address_set(local_addr_);
            //gap_local_bd_addr(local_addr);
            printf("BTstack up and running on %s.\n", bd_addr_to_str(local_addr_));

            // setup advertisements
            uint16_t adv_int_min = 800;
            uint16_t adv_int_max = 800;
            uint8_t adv_type = 0;
            bd_addr_t null_addr;
            memset(null_addr, 0, 6);
            gap_advertisements_set_params(adv_int_min, adv_int_max, adv_type, 0, null_addr, 0x07, 0x00);
            assert(adv_data_len <= 31); // ble limitation
            gap_advertisements_set_data(adv_data_len, (uint8_t*) adv_data);
            gap_advertisements_enable(1);

            poll_temp();

            break;
        case HCI_EVENT_DISCONNECTION_COMPLETE:
            printf("HCI_EVENT_DISCONNECTION_COMPLETE\n");
            le_notification_enabled = 0;
            break;
        case ATT_EVENT_CAN_SEND_NOW:
            printf("ATT_EVENT_CAN_SEND_NOW\n");

            if (counter == 2) {
                counter = 0;
            }
            else {
                counter++;
            }

            if (counter == 0) {
                printf("notify temp %d\n", current_temp);
                att_server_notify(con_handle, ATT_CHARACTERISTIC_ORG_BLUETOOTH_CHARACTERISTIC_TEMPERATURE_01_VALUE_HANDLE, (uint8_t*)&current_temp, sizeof(current_temp));    
            }
            else if (counter == 1) {
                printf("notify accel %f %f %f\n", accel[0], accel[1], accel[2]);
                att_server_notify(con_handle, ATT_CHARACTERISTIC_00000001_0001_11EE_B962_0242AC120002_01_VALUE_HANDLE, (uint8_t*)&accel, sizeof(accel));
            }
            else if (counter == 2) {
                printf("notify gyro %f %f %f\n", gyro[0], gyro[1], gyro[2]);
                att_server_notify(con_handle, ATT_CHARACTERISTIC_00000002_0001_11EE_B962_0242AC120002_01_VALUE_HANDLE, (uint8_t*)&gyro, sizeof(gyro));
            }

            break;
        default:
            break;
    }
}

uint16_t att_read_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t offset, uint8_t * buffer, uint16_t buffer_size) {
    UNUSED(connection_handle);

    printf("Read callback %d\n", att_handle);


    switch (att_handle)
    {
    case ATT_CHARACTERISTIC_ORG_BLUETOOTH_CHARACTERISTIC_TEMPERATURE_01_VALUE_HANDLE:
        printf("Read temp %d\n", current_temp);
        return att_read_callback_handle_blob((const uint8_t *)&current_temp, sizeof(current_temp), offset, buffer, buffer_size);
    case ATT_CHARACTERISTIC_00000001_0001_11EE_B962_0242AC120002_01_VALUE_HANDLE:
        printf("Read accel");
        return att_read_callback_handle_blob((const uint8_t *)&accel, sizeof(accel), offset, buffer, buffer_size);
    case ATT_CHARACTERISTIC_00000002_0001_11EE_B962_0242AC120002_01_VALUE_HANDLE:
        printf("Read gyro");
        return att_read_callback_handle_blob((const uint8_t *)&gyro, sizeof(gyro), offset, buffer, buffer_size);
    default:
        printf("Read unknown\n");
        break;
    }

    return 0;
}

int att_write_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size) {
    UNUSED(transaction_mode);
    UNUSED(offset);
    UNUSED(buffer_size);
    
    printf("Write callback %d\n", att_handle);

    le_notification_enabled = little_endian_read_16(buffer, 0) == GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_NOTIFICATION;
    con_handle = connection_handle;
    if (le_notification_enabled && att_handle == ATT_CHARACTERISTIC_00000002_0001_11EE_B962_0242AC120002_01_VALUE_HANDLE) {
        att_server_request_can_send_now_event(con_handle);
    }

    return 0;
}

void poll_temp(void) {
    adc_select_input(ADC_CHANNEL_TEMPSENSOR);
    uint32_t raw32 = adc_read();
    const uint32_t bits = 12;

    // Scale raw reading to 16 bit value using a Taylor expansion (for 8 <= bits <= 16)
    uint16_t raw16 = raw32 << (16 - bits) | raw32 >> (2 * bits - 16);

    // ref https://github.com/raspberrypi/pico-micropython-examples/blob/master/adc/temperature.py
    const float conversion_factor = 3.3 / (65535);
    float reading = raw16 * conversion_factor;
    
    // The temperature sensor measures the Vbe voltage of a biased bipolar diode, connected to the fifth ADC channel
    // Typically, Vbe = 0.706V at 27 degrees C, with a slope of -1.721mV (0.001721) per degree. 
    float deg_c = 27 - (reading - 0.706) / 0.001721;
    current_temp = deg_c * 100;
    //printf("Write temp %.2f degc\n", deg_c);
 }

 void poll_mpu6050(void) {
    mpu6050_read(accel, gyro, &temp);
 }