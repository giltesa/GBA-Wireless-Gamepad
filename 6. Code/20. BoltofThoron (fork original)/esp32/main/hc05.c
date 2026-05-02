//
// Created by Shyri
// Simulates a subset of HC-05 Bluetooth module commands and operations
//
#include "hc05.h"

#include "btstack.h"
#include "btstack_event.h"
#include "btstack_stdin.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "btstack_run_loop_freertos.h"
#include "driver/gpio.h"

#define DEVICE_NAME "GBA Bluetooth"

#define COM_MODE_REQUEST "$$$"
#define COM_MODE_RESPONSE "CMD"
#define AUTOCONNECT_REQUEST "CFR"
#define CONNECTED_RESULT "%CONNECT"


static bool inCommandMode = false;

typedef enum DESCRIPTOR_BUTTONS {
    DPAD_N = 0x00,
    DPAD_NE = 0x01,
    DPAD_E = 0x02,
    DPAD_SE = 0x03,
    DPAD_S = 0x04,
    DPAD_SW = 0x05,
    DPAD_W = 0x06,
    DPAD_NW = 0x07,
    DPAD_RELEASED = 0x08,

    HID_KEY_A = 0x10,
    HID_KEY_B = 0x20,

    HID_KEY_R = 0x40,
    HID_KEY_L = 0x80,

    HID_KEY_START = 0x01,
    HID_KEY_SELECT = 0x02

} DESCRIPTOR_BUTTONS_BITS;

typedef enum HC05_BUTTONS {
    BUTTON_A = (1 << 0),
    BUTTON_B = (1 << 1),
    BUTTON_L = (1 << 4),
    BUTTON_R = (1 << 5),
    BUTTON_SELECT = (1 << 4),
    BUTTON_START = (1 << 5),
    PAD_DOWN = 0x7F,
    PAD_UP = 0x80,
    PAD_RIGHT = 0x7F,
    PAD_LEFT = 0x80
} HC05_BUTTONS_BITS;

const uint8_t hid_descriptor_gba[] = {
	// Gamepad 1 with DPAD and 8 buttons (2 unused) - 32 bits total
	0x05, 0x01,		// USAGE_PAGE (Generic Desktop)
	0x09, 0x05,		// USAGE (Gamepad)
	0xA1, 0x01,		// COLLECTION (Application)
		//Padding - 16 bits
		0x95, 0x02,		//     REPORT_COUNT = 3
        0x75, 0x08,		//     REPORT_SIZE = 8
        0x81, 0x03,		//     INPUT = Cnst,Var,Abs
		0xA1, 0x00,		//	 COLLECTION (Physical)
			// Buttons - 8 bits (Buttons X, Y are unused)
			0x05, 0x09,			//   USAGE_PAGE (Button)
			0x19, 0x01,			//   USAGE_MINIMUM (Button 1)
			0x29, 0x08,			//   USAGE_MAXIMUM (Button 8)
			0x15, 0x00,			//   LOGICAL_MINIMUM (0)
			0x25, 0x01,			//   LOGICAL_MAXIMUM (1)
			0x95, 0x08,			//   REPORT_COUNT (8)
			0x75, 0x01,			//   REPORT_SIZE (1)
			0x81, 0x02,			//   INPUT (Data,Var,Abs)
			// DPAD - 8 bits
			0x05, 0x01,			//	 USAGE_PAGE (Generic Desktop)
			0x09, 0x39,			//	 USAGE (Hat switch)
			0x15, 0x00,			//	 LOGICAL_MINIMUM (0)
			0x25, 0x07,			//	 LOGICAL_MAXIMUM (7)
			0x35, 0x00,			//	 PHYSICAL_MINIMUM (0)
			0x46, 0x3B, 0x01,	//	 PHYSICAL_MAXIMUM (315)
			0x65, 0x14,			//	 UNIT (Degrees)
			0x95, 0x01,			//	 REPORT_COUNT (1)
			0x75, 0x08,			//	 REPORT_SIZE (8)
			0x81, 0x42,			//	 INPUT (Data,Var,Abs,Null State)
		0xC0, 		// END_COLLECTION (Physical)		
	0xC0 		// END_COLLECTION (Application)
};


//								Input HID	Res.  Btn1	Btn2
static uint8_t send_report[] = {0xA1, 0x11, 0x00, 0x00, 0x08};

static uint8_t hid_service_buffer[400];
static uint8_t device_id_sdp_service_buffer[400];
static const char hid_device_name[] = DEVICE_NAME;
static uint16_t hid_cid = 0;

static uint8_t but1_send = 0;
static uint8_t but2_send = DPAD_RELEASED;

static bool connected = false;

static const TickType_t xDelay = 1000 / portTICK_PERIOD_MS;

static btstack_packet_callback_registration_t hci_event_callback_registration;

void interpretMessage(int length, char *message);

void initBluetooth();

void initStatusIndicator();

void updateStatusIndicator(uint32_t level);

void updateButtons(const char *message);

static void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t packet_size);

void connectLastDevice();

esp_err_t save_addr(bd_addr_t *addr);

esp_err_t read_addr(bd_addr_t *addr);


static void read_uart() {
    while (1) {
        checkHC05();
        vTaskDelay(2);
    }
}

static void update_led() {
	while (1) {
		if (connected && gpio_get_level(GPIO_NUM_2) == 0) {
			updateStatusIndicator(1);
		} else if (!connected) {
			updateStatusIndicator(1);
			vTaskDelay(xDelay);
			updateStatusIndicator(0);
			vTaskDelay(xDelay);
		}
		vTaskDelay(xDelay);
	}
}

void initHC05() {
	initStatusIndicator();
    initBluetooth();
    initUART();
    xTaskCreate(read_uart, "read_uart", 2048, NULL, 1, NULL);
	xTaskCreate(update_led, "update_led", 1024, NULL, 0, NULL);
}

void initBluetooth() {
    hci_event_callback_registration.callback = &packet_handler;
    hci_add_event_handler(&hci_event_callback_registration);
    hci_register_sco_packet_handler(&packet_handler);
    gap_discoverable_control(1);
    gap_set_class_of_device(0x2508);
    gap_set_local_name(DEVICE_NAME);

    l2cap_init();
    sdp_init();
    memset(hid_service_buffer, 0, sizeof(hid_service_buffer));
    hid_create_sdp_record(hid_service_buffer, 0x10001, 0x2508, 33, 0, 0, 0, hid_descriptor_gba,
                          sizeof(hid_descriptor_gba), hid_device_name);
    sdp_register_service(hid_service_buffer);
    device_id_create_sdp_record(device_id_sdp_service_buffer, 0x10003, DEVICE_ID_VENDOR_ID_SOURCE_BLUETOOTH,
                                BLUETOOTH_COMPANY_ID_BLUEKITCHEN_GMBH, 1, 1);
    sdp_register_service(device_id_sdp_service_buffer);
    hid_device_init(1, sizeof(hid_descriptor_gba), hid_descriptor_gba);
    hid_device_register_packet_handler(&packet_handler);

    hci_power_control(HCI_POWER_ON);
}

void initStatusIndicator() {
	gpio_config_t io_conf = {};
	
	io_conf.pin_bit_mask = (1ULL << GPIO_NUM_2);
	io_conf.mode = GPIO_MODE_OUTPUT;
	io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
	io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
	io_conf.intr_type = GPIO_INTR_DISABLE;
	
	gpio_config(&io_conf);
}

void checkHC05() {
    char message[50];
    int length = uartRead(message);

    if (length > 0) {
        interpretMessage(length, message);
    }
}

void interpretMessage(int length, char *message) {
    if (inCommandMode) {
        if (strncmp((const char *) message, AUTOCONNECT_REQUEST, length - 1) == 0) { // length-1 cause ending 0x0D
            inCommandMode = false;
            connectLastDevice();
        }
    } else {
        if (strncmp((const char *) message, COM_MODE_REQUEST, length) == 0) {
            inCommandMode = true;
            uartWrite(COM_MODE_RESPONSE);
        } else if (connected) {
            // Check headers
            if (message[0] == 0xFD && message[1] == 0x06) {
                // Get HC-05 buttons
                updateButtons(message);
            } else if (message[0] == 0x00 && message[1] == 0x0D) {
                // Disconnection requested
                hid_device_disconnect(hid_cid);
            }
        }
    }
}

void bin(unsigned n) { 
    unsigned i; 
    for (i = 1 << 7; i > 0; i = i / 2) 
        (n & i)? printf("1"): printf("0"); 
} 

void updateButtons(const char *message) {
    uint8_t xAxis = message[2];
    uint8_t yAxis = message[3];
    uint8_t buttons1 = message[6];
    uint8_t buttons2 = message[7];

    but1_send = 0x00;
    but2_send = 0x00;

    if (yAxis == PAD_DOWN) {
        if (xAxis == PAD_LEFT) {
            but2_send += DPAD_SW;
        } else if (xAxis == PAD_RIGHT) {
            but2_send += DPAD_SE;
        } else {
            but2_send += DPAD_S;
        }
    } else if (yAxis == PAD_UP) {
        if (xAxis == PAD_LEFT) {
            but2_send += DPAD_NW;
        } else if (xAxis == PAD_RIGHT) {
            but2_send += DPAD_NE;
        } else {
            but2_send += DPAD_N;
        }
    } else if (xAxis == PAD_LEFT) {
            but2_send += DPAD_W;
    } else if (xAxis == PAD_RIGHT) {
            but2_send += DPAD_E;
    } else {
        but2_send += DPAD_RELEASED;
    }

    if (buttons1 & BUTTON_A) {
        but1_send += HID_KEY_A;
    }

    if (buttons1 & BUTTON_B) {
        but1_send += HID_KEY_B;
    }

    if (buttons1 & BUTTON_L) {
        but1_send += HID_KEY_L;
    }

    if (buttons1 & BUTTON_R) {
        but1_send += HID_KEY_R;
    }

    if (buttons2 & BUTTON_START) {
		but1_send += HID_KEY_START;
    }

    if (buttons2 & BUTTON_SELECT) {
		but1_send += HID_KEY_SELECT;
    }
    

    printf("but1: ");
    bin((uint8_t) but1_send);
    printf("\nbut2: ");
    bin((uint8_t) but2_send);
    printf("\n");
}

void updateStatusIndicator(uint32_t level) {
	// set status of GPIO2 to reflect connection status on LED
	gpio_set_level(GPIO_NUM_2, level);
	
	// use this instead for debugging
	//if (gpio_set_level(GPIO_NUM_2, level) != ESP_OK)
	//{
		// because this only influences the LED indicator, we will only print warning message
		//printf("Failed to update GPIO2 on ESP32\n");	
	//}
}


static void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t packet_size) {
    UNUSED(channel);
    UNUSED(packet_size);

    bd_addr_t event_addr;


    switch (packet_type) {
        case HCI_EVENT_PACKET:
            switch (packet[0]) {
                case HCI_EVENT_HID_META:
                    switch (hci_event_hid_meta_get_subevent_code(packet)) {
                        case HID_SUBEVENT_CONNECTION_OPENED:
                            if (hid_subevent_connection_opened_get_status(packet)) return;
                            hid_cid = hid_subevent_connection_opened_get_hid_cid(packet);
                            hid_device_request_can_send_now_event(hid_cid); //start loop
                            log_info("HID Connected");

                            hid_subevent_connection_opened_get_bd_addr(packet, event_addr);
                            char *address = bd_addr_to_str(event_addr);
                            printf("Saving address: %s\n", address);
							
							save_addr(event_addr);

                            connected = true;

                            uartWrite(CONNECTED_RESULT);

                            break;
                        case HID_SUBEVENT_CONNECTION_CLOSED:
                            log_info("HID Disconnected");
                            hid_cid = 0;
                            break;
                        case HID_SUBEVENT_CAN_SEND_NOW:
								send_report[3] = but1_send;
								send_report[4] = but2_send;
								hid_device_send_interrupt_message(hid_cid, &send_report[0], sizeof(send_report));
								hid_device_request_can_send_now_event(hid_cid);							
                            break;
                        default:
                            break;
                    }
                    break;

                case HCI_EVENT_DISCONNECTION_COMPLETE:
                    printf("Disconnected\n");
                    connected = false;
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
}

void connectLastDevice() {
    bd_addr_t mac_addr;
    if (read_addr(&mac_addr) == ESP_OK) {
        char *address = bd_addr_to_str(mac_addr);
        printf("Saved Address: %s \n", address);

        hid_device_connect(mac_addr, &hid_cid);
    } else {
        uartWrite("ERR");
    }
}

esp_err_t save_addr(bd_addr_t *addr) {
    nvs_handle my_handle;
    esp_err_t err;

    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK) return err;

    size_t size = sizeof(bd_addr_t);
    err = nvs_set_blob(my_handle, "mac_addr", addr, size);
    if (err != ESP_OK) return err;

    err = nvs_commit(my_handle);
    if (err != ESP_OK) return err;

    nvs_close(my_handle);

    return ESP_OK;
}


esp_err_t read_addr(bd_addr_t *addr) {
    nvs_handle my_handle;
    esp_err_t err;

    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK) return err;

    size_t size = sizeof(bd_addr_t);
    nvs_get_blob(my_handle, "mac_addr", addr, &size);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) return err;

    nvs_close(my_handle);
    return err;
}