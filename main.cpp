/* 
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "hardware/gpio.h"
#include "bsp/board.h"
#include "tusb.h"
#include "DmxOutput.h"

//--------------------------------------------------------------------+
// MACRO CONSTANT TYPEDEF PROTYPES
//--------------------------------------------------------------------+

/* Blink pattern
 * - 250 ms  : device not mounted
 * - 1000 ms : device mounted
 * - 2500 ms : device is suspended
 */
enum  {
  BLINK_NOT_MOUNTED = 250,
  BLINK_MOUNTED = 1000,
  BLINK_SUSPENDED = 2500,
};

#define DMX_CHANNELS 500
// XXX hardcoded bigger buffer for usb rcv
static uint8_t dmx_buffer[512 + 1];

static uint32_t blink_interval_ms = BLINK_NOT_MOUNTED;

void led_blinking_task(void);

void dmx_task(DmxOutput *out)
{
	static uint64_t last_write_ts = 0;

	if (!out->busy() && last_write_ts + 10000 < time_us_64())
	{
		last_write_ts = time_us_64();
		out->write(dmx_buffer, DMX_CHANNELS);
	}
}

/*------------- MAIN -------------*/
int main(void)
{
  DmxOutput dmxOutput;
  
  dmxOutput.begin(21);
  /*
  gpio_init(21);
  gpio_set_dir(21, GPIO_OUT);
  */
  board_init();

  tusb_init();

  while (1)
  {
    tud_task(); // tinyusb device task
    led_blinking_task();
	dmx_task(&dmxOutput);
	if (tud_vendor_available() == 512) {
		tud_vendor_read(dmx_buffer + 1, 512);
	}
  }

  return 0;
}

//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void)
{
  blink_interval_ms = BLINK_MOUNTED;
}

// Invoked when device is unmounted
void tud_umount_cb(void)
{
  blink_interval_ms = BLINK_NOT_MOUNTED;
}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en)
{
  (void) remote_wakeup_en;
  blink_interval_ms = BLINK_SUSPENDED;
}

// Invoked when usb bus is resumed
void tud_resume_cb(void)
{
  blink_interval_ms = BLINK_MOUNTED;
}

//--------------------------------------------------------------------+
// USB HID
//--------------------------------------------------------------------+

/*
static uint64_t memcpy_time_us = 0;
// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize)
{
  // This example doesn't use multiple report and report ID
  (void) itf;
  (void) report_id;
  (void) report_type;
  uint64_t start;
  dmx_buffer[0] = 0; // dmx start code
  if (buffer[0] < 16) {
	  memcpy(dmx_buffer + 1 + 32 * buffer[0], buffer + 1, 32);
  }
  tud_hid_report(0, buffer, 0);
}
*/


void tud_vendor_tx_cb(uint8_t itf, uint32_t sent_bytes) {
/*
//XXX hardcoded
	tud_vendor_read(dmx_buffer + 1, 512);
	tud_vendor_read_flush();
*/
}


/*
void tud_vendor_rx_cb(uint8_t itf) {
}
*/

//--------------------------------------------------------------------+
// BLINKING TASK
//--------------------------------------------------------------------+
void led_blinking_task(void)
{
  static uint32_t start_ms = 0;
  static bool led_state = false;

  // Blink every interval ms
  if ( board_millis() - start_ms < blink_interval_ms) return; // not enough time
  start_ms += blink_interval_ms;

  board_led_write(led_state);
  led_state = 1 - led_state; // toggle
}

///////////////////////////////////////////////////////////////////////////////

// SPDX-License-Identifier: CC0-1.0

/*
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "bsp/board.h"
#include "tusb.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "hardware/watchdog.h"
#include "pico/unique_id.h"


#include "common/tusb_common.h"
#include "device/usbd.h"

struct gud_display;
bool gud_driver_req_timeout(unsigned int timeout_secs);
void gud_driver_setup(const struct gud_display *disp, void *framebuffer, void *compress_buf);

#include "gud.h"
#include "mipi_dbi.h"
#include "cie1931.h"

static uint32_t gud_display_edid_get_serial_number(void)
{
    pico_unique_board_id_t id_out;

    pico_get_unique_board_id(&id_out);
    return *((uint64_t*)(id_out.id));
}

static const struct gud_display_edid edid = {
    .name = "pico display",
    .pnp = "PIM",
    .product_code = 0x01,
    .year = 2021,
    .width_mm = 27,
    .height_mm = 16,

    .get_serial_number = gud_display_edid_get_serial_number,
};

int main(void)
{
    board_init();

    if (USE_WATCHDOG && watchdog_caused_reboot()) {
        LOG("Rebooted by Watchdog!\n");
        panic_reboot_blink_time = 1;
    }

    if (LED_ACTION)
        board_led_write(true);

    init_display();

    gud_driver_setup(&display, framebuffer, compress_buf);

    tusb_init();

    LOG("\n\n%s: CFG_TUSB_DEBUG=%d\n", __func__, CFG_TUSB_DEBUG);

    turn_off_rgb_led();

    if (USE_WATCHDOG)
        watchdog_enable(5000, 0); // pause_on_debug=0 so it can reset panics.

    while (1)
    {
        tud_task(); // tinyusb device task

        if (USE_WATCHDOG) {
            watchdog_update();

            uint64_t now = time_us_64();
            if (PANIC_REBOOT_BLINK_LED_MS && panic_reboot_blink_time && panic_reboot_blink_time < now) {
                static bool led_state;
                board_led_write(led_state);
                led_state = !led_state;
                panic_reboot_blink_time = now + (PANIC_REBOOT_BLINK_LED_MS * 1000);
            }

            // Sometimes we stop receiving USB requests, but the host thinks everything is fine.
            // Reset if we haven't heard from tinyusb, the host sends connector status requests every 10 seconds
            // Let the watchdog do the reset
            if (gud_driver_req_timeout(15))
                panic("Request TIMEOUT");
        }
    }

    return 0;
}

*/
