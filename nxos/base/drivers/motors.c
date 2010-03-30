/* Copyright (C) 2007 the NxOS developers
 *
 * See AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */

#include "base/at91sam7s256.h"

#include "base/types.h"
#include "base/nxt.h"
#include "base/interrupts.h"
#include "base/assert.h"
#include "base/drivers/systick.h"
#include "base/drivers/aic.h"
#include "base/drivers/_avr.h"

#include "base/drivers/_motors.h"

/* The following are easier mnemonics for the pins used by the
 * tachymeter. Each motor has a tach-pulse pin whose value flips at
 * regular rotation intervals, and a direction pin whose value depends
 * on the direction of rotation.
 */
#define MOTOR_A_TACH AT91C_PIO_PA15
#define MOTOR_A_DIR AT91C_PIO_PA1
#define MOTOR_B_TACH AT91C_PIO_PA26
#define MOTOR_B_DIR AT91C_PIO_PA9
#define MOTOR_C_TACH AT91C_PIO_PA0
#define MOTOR_C_DIR AT91C_PIO_PA8

/* We often need to manipulate all tach pins and/or all dir pins
 * together. Lets's simplify doing that.
 */
#define MOTORS_TACH (MOTOR_A_TACH | MOTOR_B_TACH | MOTOR_C_TACH)
#define MOTORS_DIR (MOTOR_A_DIR | MOTOR_B_DIR | MOTOR_C_DIR)
#define MOTORS_ALL (MOTORS_TACH | MOTORS_DIR)

/* The pin mapping for the motor tacho inputs. */
static const struct {
  U32 tach;
  U32 dir;
} motors_pinmap[NXT_N_MOTORS] = {
  { MOTOR_A_TACH, MOTOR_A_DIR },
  { MOTOR_B_TACH, MOTOR_B_DIR },
  { MOTOR_C_TACH, MOTOR_C_DIR },
};

/* Definitions of the state of each motor. */
static volatile struct {
  /* The mode this motor is currently in. */
  enum {
    MOTOR_STOP = 0,      /* No rotation. */
    MOTOR_CONFIGURING,   /* Reconfiguration in progress. */
    MOTOR_ON_CONTINUOUS, /* Rotate until stopped. */
    MOTOR_ON_ANGLE,      /* Rotate a given angle. */
    MOTOR_ON_TIME,       /* Rotate a given time. */
  } mode;

  /* If in one of the automatic modes (angle/time rotation), defines
   * whether the motor will brake (TRUE) or coast (FALSE) when
   * stopped.
   */
  bool brake;

  /* The current tachymeter count. Was declared as Unsigned 32, but sign is
   * needed because rotation can be counter-clockwise from the beginning.
   */
  S32 current_count;

  /* The target is a mode-dependent value.
   *  - In stop and continuous mode, it is not used.
   *  - In angle mode, holds the target tachymeter count. (Needs sign)
   *  - In time mode, holds the remaining number of milliseconds to
   *    run.
   */
  S32 target;
} motors_state[NXT_N_MOTORS] = {
  { MOTOR_STOP, TRUE, 0, 0 },
  { MOTOR_STOP, TRUE, 0, 0 },
  { MOTOR_STOP, TRUE, 0, 0 },
};

/* Tachymeter interrupt handler, triggered by a change of value of a
 * tachymeter pin.
 */
static void motors_isr(void) {
  int i;
  U32 changes;
  U32 pins;
  S32 time; // Fixed time variable

  /* Acknowledge the interrupt and grab the state of the pins. */
  changes = *AT91C_PIOA_ISR;
  pins = *AT91C_PIOA_PDSR;

  /* Grab the time, as we're going to use it to check for timed
   * rotation end.
   */
  time = nx_systick_get_ms();

  /* Check each motor's tachymeter. */
  for (i=0; i<NXT_N_MOTORS; i++) {
    if (changes & motors_pinmap[i].tach) {
      U32 tach = pins & motors_pinmap[i].tach;
      U32 dir = pins & motors_pinmap[i].dir;

      /* If the tachymeter pin value is the opposite the direction pin
       * value, then the motor is rotating 'forwards' (positive speed
       * value given to start it). Otherwise, it's rotating
       * 'backwards', and we should decrement the tachymeter count
       * instead of incrementing it.
       */
      if ((tach && !dir) || (!tach && dir))
        motors_state[i].current_count++;
      else
        motors_state[i].current_count--;

      /* If we are in angle rotation mode, check to see if we've
       * reached the target tachymeter value. If so, shut down the
       * motor.
       */
      if ((motors_state[i].mode == MOTOR_ON_ANGLE &&
           motors_state[i].current_count == motors_state[i].target) ||
          (motors_state[i].mode == MOTOR_ON_TIME &&
           time >= motors_state[i].target))
        nx_motors_stop(i, motors_state[i].brake);
    }
  }
}

void nx__motors_init(void)
{
  nx_interrupts_disable();

  /* Enable the PIO controller. */
  *AT91C_PMC_PCER = (1 << AT91C_ID_PIOA);

  /* Disable all PIO interrupts until we are ready to handle them. */
  *AT91C_PIOA_IDR = ~0;

  /* Configure all tachymeter pins:
   *  - Enable input glitch filtering
   *  - Disable pull-up (externally driven pins)
   *  - Assign pins to the PIO controller
   *  - Set pins to be inputs
   */
  *AT91C_PIOA_IFER = MOTORS_ALL;
  *AT91C_PIOA_PPUDR = MOTORS_ALL;
  *AT91C_PIOA_PER = MOTORS_ALL;
  *AT91C_PIOA_ODR = MOTORS_ALL;

  /* Register the tachymeter interrupt handler. */
  nx_aic_install_isr(AT91C_ID_PIOA, AIC_PRIO_SOFTMAC,
                  AIC_TRIG_LEVEL, motors_isr);

  /* Trigger interrupts on changes to the state of the tachy pins. */
  *AT91C_PIOA_IER = MOTORS_TACH;

  nx_interrupts_enable();
}

void nx_motors_stop(U8 motor, bool brake) {
  NX_ASSERT(motor < NXT_N_MOTORS);

  motors_state[motor].mode = MOTOR_STOP;
  nx__avr_set_motor(motor, 0, brake);
}

void nx_motors_rotate(U8 motor, S8 speed) {
  NX_ASSERT(motor < NXT_N_MOTORS);

  /* Cap the given motor speed. */
  if (speed > 0 && speed > 100)
    speed = 100;
  else if (speed < 0 && speed < -100)
    speed = -100;

  /* Continuous mode has no target or brake parameter, just set the
   * mode and fire up the motor.
   */
  motors_state[motor].mode = MOTOR_ON_CONTINUOUS;
  nx__avr_set_motor(motor, speed, FALSE);
}

void nx_motors_rotate_angle(U8 motor, S8 speed, U32 angle, bool brake) {
  NX_ASSERT(motor < NXT_N_MOTORS);
  NX_ASSERT(speed <= 100);
  NX_ASSERT(speed >= -100);

  /* If we're not moving, we can never reach the target. Take a
   * shortcut.
   */
  if (speed == 0) {
    nx_motors_stop(motor, brake);
    return;
  }

  /* Set the motor to configuration mode. This way, if we are
   * overriding another intelligent mode, the tachymeter interrupt
   * handler will ignore the motor while we tweak its settings.
   */
  motors_state[motor].mode = MOTOR_CONFIGURING;

  /* Set the target tachymeter value based on the rotation
   * direction */
  if (speed < 0)
    motors_state[motor].target =
      motors_state[motor].current_count - angle;
  else
    motors_state[motor].target =
      motors_state[motor].current_count + angle;

  /* Remember the brake setting, change to angle target mode and fire
   * up the motor.
   */
  motors_state[motor].brake = brake;
  motors_state[motor].mode = MOTOR_ON_ANGLE;
  nx__avr_set_motor(motor, speed, FALSE);
}

void nx_motors_rotate_time(U8 motor, S8 speed, U32 ms, bool brake) {
  NX_ASSERT(motor < NXT_N_MOTORS);
  NX_ASSERT(speed <= 100);
  NX_ASSERT(speed >= -100);

  /* If we're not moving, we can never reach the target. Take a
   * shortcut.
   */
  if (speed == 0) {
    nx_motors_stop(motor, brake);
    return;
  }

  /* Set the motor to configuration mode. This way, if we are
   * overriding another intelligent mode, the tachymeter interrupt
   * handler will ignore the motor while we tweak its settings.
   */
  motors_state[motor].mode = MOTOR_CONFIGURING;

  /* Set the target system time. */
  motors_state[motor].target = nx_systick_get_ms() + ms;

  /* Remember the brake setting, change to angle target mode and fire
   * up the motor.
   */
  motors_state[motor].brake = brake;
  motors_state[motor].mode = MOTOR_ON_TIME;
  nx__avr_set_motor(motor, speed, FALSE);
}

S32 nx_motors_get_tach_count(U8 motor) {
  NX_ASSERT(motor < NXT_N_MOTORS);

  return motors_state[motor].current_count;
}
