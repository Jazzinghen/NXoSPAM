/* Copyright (c) 2007,2008 the NxOS developers
 *
 * See AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */

#include "base/core.h"
#include "base/drivers/systick.h"
#include "base/drivers/avr.h"
#include "base/drivers/radar.h"

#include "base/drivers/motors.h"
#include "base/display.h"
#include "base/drivers/lightsensor.h"

#include "tests/tests.h"

/* Various definitions */

#define LIGHT_SENSOR      0
#define RADAR_SENSOR      1
#define RADAR_MOTOR       0
#define RIGHT_MOTOR       1
#define LEFT_MOTOR        2

/* The security hook lets the tester shut down the brick despite the
 * main thread of execution being locked up due to a bug. As long as the
 * system timer and AVR link are working, pressing the Cancel button
 * should power off the brick.
 */
static void security_hook(void) {
  if (nx_avr_get_button() == BUTTON_CANCEL)
    nx_core_halt();
}

static void display_rotation_data (S8 vel, S32 tot, S8 sangle) {
  nx_display_clear();
  /* Display Light Intensity as read by the Light Sensor */
  nx_display_string("Luminosità: ");
  nx_display_uint(nx_lightsensor_get_raw(LIGHT_SENSOR));
  nx_display_end_line();
  /* Display Radar Motor's Speed */
  nx_display_string("Velocità: ");
  if (vel >= 0) {
    nx_display_uint(vel);
  } else {
    nx_display_string("-");
    nx_display_uint(vel);
  };
  nx_display_end_line();
  /* Display actual angle of tha Radar's Motor */
  nx_display_string("Start Angle: ");
  nx_display_int(sangle);
  nx_display_end_line();
  nx_display_string("Angolo: ");
  if (tot >= 0) {
    nx_display_uint(tot % 360);
  } else {
    nx_display_string("-");
    nx_display_uint((tot*-1) % 360);
  }
  nx_display_end_line();
}

void main(void) {
  nx_systick_install_scheduler(security_hook);

  bool moving = FALSE;
  S32 total_rotation = 0;
  S8 speed = 60;
  S8 start_angle = 0;

  //tests_all();
  //tests_usb();
  //tests_bt();
  //tests_usb_hardcore();
  //tests_radar();
  //tests_util();
  //tests_defrag();
  
  nx_radar_init(RADAR_SENSOR);
  nx_lightsensor_init(LIGHT_SENSOR);
  //nx_lightsensor_fire_spamlight(LIGHT_SENSOR);
  
 /* for (fuffa_rot = 0; (nx_motors_get_tach_count(0) % 360) < 90; fuffa_rot++) {
      nx_systick_wait_ms(1000);
      nx_motors_rotate_angle(0, 100, 1, TRUE);
  } */
    
  for(;;) {
    nx_systick_wait_ms(500);
    //nx_display_cursor_set_pos(9, 3);
    nx_display_clear();
    total_rotation = nx_motors_get_tach_count(RADAR_MOTOR); 
    display_rotation_data(speed, total_rotation, start_angle);
    
    
    
    switch (nx_avr_get_button()) {
      case BUTTON_LEFT:
       /* if (speed >= -95) { 
          speed -= 5;
        } else {
          speed = -100;
        }*/
        nx_motors_rotate (0, -60);
        nx_systick_wait_ms(100);
        start_angle = nx_motors_get_tach_count(0);
        nx_motors_rotate_angle(RADAR_MOTOR, -35, 45, FALSE);
        /*nx_systick_wait_ms(1000);
         *nx_motors_rotate_angle(0, 100, 45, TRUE);
         */
        break;
      case BUTTON_RIGHT:
        /*if (speed <= 95) { 
          speed += 5;
        } else {
          speed = 100;
        }
        nx_motors_rotate (0, speed);*/
        nx_motors_rotate (0, 60);
        nx_systick_wait_ms(100);
        start_angle = nx_motors_get_tach_count(0);
        nx_motors_rotate_angle(RADAR_MOTOR, 35, 45, FALSE);
        break;
      case BUTTON_OK:
        if (moving) {
          nx_motors_stop(RADAR_MOTOR, TRUE);
          //nx_motors_stop(1, TRUE);
          //nx_motors_stop(2, TRUE);
          moving = !moving;
        } else {
          nx_motors_rotate(RADAR_MOTOR, speed);
          //nx_motors_rotate(1, 20);
          //nx_motors_rotate(2, 20);
          moving = !moving;
        }
        break;
      default:
        break;
    } 
  };
}
