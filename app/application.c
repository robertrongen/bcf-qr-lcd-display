#include <application.h>
#include "qrcodegen.h"

#define BATTERY_UPDATE_INTERVAL (60 * 60 * 1000)

// LED instance
bc_led_t led;

// GFX instance
bc_gfx_t *gfx;

// LCD buttons instance
bc_button_t button_left;
bc_button_t button_right;


// QR code variables
char qr_code[250];
int orderId[2];
// char display_url[80] = 'http://blokko.blockchainadvies.nu/receive-order.html?orderid=';
char display_code[5]= "Hallo";
// char display_code[80]= "http://bit.ly/2ZwGiQR";
char display_url[80]= "http://blokko.blockchainadvies.nu/receive-order.html?orderid=1";
// char display_code[80]= "http://blokko.blockchainadvies.nu/receive-order.html?orderid=1";
char instruction[16] = "Accept+pay order";

/* 
 * QR Code generator demo (C)
 * Copyright (c) Project Nayuki. (MIT License)
 * https://www.nayuki.io/page/qr-code-generator-library
*/

/*---- BigClown Module ----*/

static const bc_radio_sub_t subs[] = {
    {"qr/-/chng/code", BC_RADIO_SUB_PT_STRING, bc_change_qr_value, (void *) PASSWD}
};

uint32_t display_page_index = 0;
bc_tmp112_t temp;

void battery_event_handler(bc_module_battery_event_t event, void *event_param)
{
    (void) event;
    (void) event_param;

    float voltage;
    int percentage;

    if (bc_module_battery_get_voltage(&voltage))
    {
        bc_radio_pub_battery(&voltage);
    }


    if (bc_module_battery_get_charge_level(&percentage))
    {
        bc_radio_pub_string("%d", percentage);
    }
}

void tmp112_event_handler(bc_tmp112_t *self, bc_tmp112_event_t event, void *event_param)
{
    (void) self;
    (void) event_param;

    if (event == BC_TMP112_EVENT_UPDATE)
    {
        float temperature = 0.0;
        bc_tmp112_get_temperature_celsius(&temp, &temperature);
        bc_radio_pub_temperature(BC_RADIO_PUB_CHANNEL_R1_I2C0_ADDRESS_ALTERNATE, &temperature);
    }
}

void bc_change_qr_value(uint64_t *id, const char *topic, void *value, void *param)
{
    int command = (int *) param;   //  warning: initialization makes integer from pointer without a cast [-Wint-conversion]
                                   //  warning: unused variable 'command' [-Wunused-variable]

    strncpy(qr_code, value, sizeof(qr_code));

    bc_eeprom_write(0, qr_code, sizeof(qr_code));
    get_qr_data();

    qrcode_project(qr_code);
    
    bc_scheduler_plan_now(500);
}

// ** Print QR code and messages **
static void printQr(const uint8_t qrcode[]) 
{
    get_qr_data();
    bc_gfx_clear(gfx);

    bc_gfx_set_font(gfx, &bc_font_ubuntu_13);
    // bc_gfx_draw_string(gfx, 2, 0, instruction, true);
    // bc_gfx_draw_string(gfx, 2, 10, display_code, true);

    bc_gfx_draw_string(gfx, 0, 4, display_code, true);

    uint32_t offset_x = 0;  // 8 -> med: 6, high: 0
    uint32_t offset_y = 0; // 32 -> med: 6, high: 0
    uint32_t box_size = 4; // 3 ->  med: 4, high: 4
	int size = qrcodegen_getSize(qrcode);
	int border = 0; // 2 -> 2
	for (int y = -border; y < size + border; y++) {
		for (int x = -border; x < size + border; x++) {
            uint32_t x1 = offset_x + x * box_size;
            uint32_t y1 = offset_y + y * box_size;
            uint32_t x2 = x1 + box_size;
            uint32_t y2 = y1 + box_size;

            bc_gfx_draw_fill_rectangle(gfx, x1, y1, x2, y2, qrcodegen_getModule(qrcode, x, y));
		}
	}
    bc_gfx_update(gfx);
}

void get_qr_data()
{
    get_orderId();
}

char get_orderId()
{
    int i = 0;
    int semicolons = 0;
    bool orderId_found = false;
    do
    {
        i++;
        if(qr_code[i] == ';')
        {
            semicolons++;
            if(semicolons == 2)
            {
                orderId_found = true;
            }
        }
    }
    while(!orderId_found);
    
    i += 3;

    int start_i = i;

    for(; qr_code[i] != ';'; i++)
    {
        orderId[i - start_i] = qr_code[i];
    }

    return orderId;   // warning: return makes integer from pointer without a cast [-Wint-conversion]
}

void lcd_event_handler(bc_module_lcd_event_t event, void *event_param)
{ 
    if(event == BC_MODULE_LCD_EVENT_LEFT_CLICK) 
    {
        bc_radio_pub_push_button(0);
        display_page_index = 0;

    }
    else if(event == BC_MODULE_LCD_EVENT_RIGHT_CLICK) 
    {
        if(display_page_index < 3)
        {
            display_page_index++;
        }
        else if(display_page_index == 3)
        {
            display_page_index = 0;
        }
        bc_scheduler_plan_now(0);
    }

}

enum qrcodegen_Mask maskPattern = qrcodegen_Mask_AUTO;  // Mask pattern
enum qrcodegen_Ecc errCorLvl = qrcodegen_Ecc_MEDIUM;  // Error correction level

void qrcode_project(char *text)
{
    bc_system_pll_enable();

	// Make and print the QR Code symbol
	uint8_t qrcode[qrcodegen_BUFFER_LEN_MAX];
	uint8_t tempBuffer[qrcodegen_BUFFER_LEN_MAX];
	bool ok = qrcodegen_encodeText(text, tempBuffer, qrcode, errCorLvl, qrcodegen_VERSION_MIN, qrcodegen_VERSION_MAX, maskPattern, true);

	if (ok)
    {
		printQr(qrcode);
    }

    bc_system_pll_disable();
}

void lcd_page_with_data()
{
    bc_gfx_set_font(gfx, &bc_font_ubuntu_15);
    bc_gfx_clear(gfx);
    bc_gfx_printf(gfx, 0, 0, true, "Scan to confirm order:");
    bc_gfx_printf(gfx, 0, 15, true, "Container #:");
    bc_gfx_printf(gfx, 0, 25, true, display_code);
    /*
    char _box_size[16];
    sprintf(_box_size, "%lu", box_size);
    bc_gfx_printf(gfx, 20, 55, true, _box_size);
    */
    bc_gfx_update(gfx);
}

void application_init(void)
{
    bc_radio_init(BC_RADIO_MODE_NODE_SLEEPING);
    bc_radio_set_subs((bc_radio_sub_t *) subs, sizeof(subs)/sizeof(bc_radio_sub_t));
    bc_radio_set_rx_timeout_for_sleeping_node(250);

    bc_log_init(BC_LOG_LEVEL_DUMP, BC_LOG_TIMESTAMP_ABS);

    // Initialize LED
    bc_led_init(&led, BC_GPIO_LED, false, false);

    bc_module_lcd_init();
    gfx = bc_module_lcd_get_gfx();
    bc_module_lcd_set_event_handler(lcd_event_handler, NULL);

/*
    // initialize TMP112 sensor
    bc_tmp112_init(&temp, BC_I2C_I2C0, BC_TAG_TEMPERATURE_I2C_ADDRESS_ALTERNATE);

    // set measurement handler (call "tmp112_event_handler()" after measurement)
    bc_tmp112_set_event_handler(&temp, tmp112_event_handler, NULL);

    // automatically measure the temperature every 15 minutes
    bc_tmp112_set_update_interval(&temp, 15 * 60 * 1000);
*/    

    // initialize LCD and load from eeprom
    bc_eeprom_read(0, qr_code, sizeof(qr_code));

    if(strstr(qr_code, "WIFI:S:") == NULL)
    {
        strncpy(qr_code, "WIFI:S:test;T:test;P:test;;", sizeof(qr_code));
    }

    // Initialze battery module
    bc_module_battery_init();
    bc_module_battery_set_event_handler(battery_event_handler, NULL);
    bc_module_battery_set_update_interval(BATTERY_UPDATE_INTERVAL);
    bc_radio_pairing_request("qr-terminal", VERSION);

}

/*---- Begin Demo suite ----*/

// Creates a single QR Code, then prints it to the console.
static void doBasicDemo(void) {
	const char *text = "http://blokko.blockchainadvies.nu/receive-order.html";                   // User-supplied text
	
	// Make and print the QR Code symbol
	uint8_t qrcode[qrcodegen_BUFFER_LEN_MAX];
	uint8_t tempBuffer[qrcodegen_BUFFER_LEN_MAX];
	bool ok = qrcodegen_encodeText(text, tempBuffer, qrcode, qrcodegen_Ecc_MEDIUM,
		qrcodegen_VERSION_MIN, qrcodegen_VERSION_MAX, qrcodegen_Mask_AUTO, true);
	if (ok)
		printQr(qrcode);
}

/*---- End Demo suite ----*/

void application_task(void)
{
    if(display_page_index == 0)
    {
        bc_log_debug("calling page 1");
        doBasicDemo();
    }
    else if(display_page_index == 1)
    {
        bc_log_debug("calling page 2");
        qrcode_project(qr_code);
    }
    else if(display_page_index == 2)
    {
        bc_log_debug("calling page 3");
        lcd_page_with_data();
    }
}
