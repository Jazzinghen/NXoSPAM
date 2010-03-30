/* Copyright (c) 2007,2008 the NxOS developers
 *
 * See AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */

#include "at91sam7s256.h"

#include "base/types.h"
#include "base/interrupts.h"
#include "base/_display.h"
#include "base/assert.h"
#include "drivers/_aic.h"
#include "drivers/_systick.h"
#include "drivers/_sound.h"
#include "drivers/_avr.h"
#include "drivers/_motors.h"
#include "drivers/_lcd.h"
#include "drivers/_sensors.h"
#include "drivers/_usb.h"
#include "drivers/i2c.h"

#include "base/_core.h"

/* main() is the entry point into the custom payload, not included in
 * the NxOS core.
 */
extern void main(void);

/* Application kernels can register a shutdown handler, which we keep here. */
static nx_closure_t shutdown_handler = NULL;

static void core_init(void) {
  nx__aic_init();
  nx_interrupts_enable();
  nx__systick_init();
  nx__sound_init();
  nx__avr_init();
  nx__motors_init();
  nx__lcd_init();
  nx__display_init();
  nx__sensors_init();
  nx__usb_init();
  nx_i2c_init(); // TODO: should be nx__i2c_init().

  /* Delay a little post-init, to let all the drivers settle down. */
  nx_systick_wait_ms(100);
}

/* Checks whether the system rebooted due to some kind of external
 * failure, and report if so.
 *
 * Currently detects brownout and watchdog resets.
 */
static void check_boot_errors(void) {
  int reset_status = *AT91C_RSTC_RSR;
  reset_status &= AT91C_RSTC_RSTTYP;

  /* A watchdog reset should never happen, given that we disable it
   * immediately at bootup. This is just a bug guard.
   */
  if (reset_status == AT91C_RSTC_RSTTYP_WATCHDOG) {
    NX_FAIL("**************\n"
	    "Watchdog fault\n"
	    "**************\n");
  } else if (reset_status == AT91C_RSTC_RSTTYP_BROWNOUT) {
    NX_FAIL("**************\n"
	    "Brownout fault\n"
	    "**************\n");
  }
}

void nx_core_halt(void) {
  if (shutdown_handler)
    shutdown_handler();
  nx__lcd_shutdown();
  nx__usb_disable();
  nx__avr_power_down();
}

void nx_core_register_shutdown_handler(nx_closure_t handler) {
  shutdown_handler = handler;
}

void nx__kernel_main(void) {
  core_init();
  check_boot_errors();
  main();
  nx_core_halt();
}
