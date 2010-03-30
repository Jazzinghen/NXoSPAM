/* Copyright (C) 2007 the NxOS developers
 *
 * See AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */

#include "base/nxt.h"
#include "base/types.h"
#include "base/util.h"
#include "base/assert.h"
#include "base/drivers/systick.h"
#include "base/drivers/_twi.h"

#include "base/drivers/_avr.h"

#define AVR_ADDRESS 1
#define AVR_MAX_FAILED_CHECKSUMS 3

const char avr_init_handshake[] =
  "\xCC" "Let's samba nxt arm in arm, (c)LEGO System A/S";

static volatile struct {
  /* The current mode of the AVR state machine. */
  enum {
    AVR_UNINITIALIZED = 0, /* Initialization not completed. */
    AVR_LINK_DOWN,         /* No handshake has been sent. */
    AVR_INIT,              /* Handshake send in progress. */
    AVR_WAIT_2MS,          /* Timed wait after the handshake. */
    AVR_WAIT_1MS,          /* More timed wait. */
    AVR_SEND,              /* Sending of to_avr in progress. */
    AVR_RECV,              /* Reception of from_avr in progress. */
  } mode;

  /* Used to detect link failures and restart the AVR link. */
  U8 failed_consecutive_checksums;
} avr_state = {
  AVR_UNINITIALIZED, /* We start uninitialized. */
  0,                 /* No failed checksums yet. */
};


/* Contains all the commands that are periodically sent to the AVR. */
static volatile struct {
  /* Tells the AVR to perform power management: */
  enum {
    AVR_RUN = 0,    /* No power management (normal runtime mode). */
    AVR_POWER_OFF,  /* Power down the brick. */
    AVR_RESET_MODE, /* Go into SAM-BA reset mode. */
  } power_mode;

  /* The speed and braking configuration of the motor ports. */
  S8 motor_speed[NXT_N_MOTORS];
  U8 motor_brake;

  /* TODO: enable controlling of input power. Currently the input
   * stuff is ignored.
   */
} to_avr = {
  AVR_RUN,     /* Start in normal power mode. */
  { 0, 0, 0 }, /* All motors are off... */
  0            /* And set to coast. */
};


/* Contains all the status data periodically received from the AVR. */
static volatile struct {
  /* The analog reading of the analog pin on all active sensors. */
  U16 adc_value[NXT_N_SENSORS];

  /* The state of the NXT's buttons. Given the way that the buttons
   * are handled in hardware, only one button is reported pressed at a
   * time. See the nx_avr_button_t enumeration for values to test for.
   */
  U8 buttons;

  /* Battery information. */
  struct {
    bool is_aa; /* True if the power supply is AA batteries (as
                 * opposed to a battery pack).
                 */
    U16 charge; /* The remaining battery charge in mV. */
  } battery;

  /* The version of the AVR firmware. The currently supported version
   * is 1.1.
   */
  struct {
    U8 major;
    U8 minor;
  } version;
} from_avr;


/* The following two arrays hold the data structures above, converted
 * into the raw ARM-AVR communication format. Data to send is
 * serialized into this buffer prior to sending, and received data is
 * received into here before being deserialized into the status
 * struct.
 */
static U8 raw_from_avr[(2 * NXT_N_SENSORS) + /* Sensor A/D value. */
                       2 + /* Buttons reading.  */
                       2 + /* Battery type, charge and AVR firmware
                            * version. */
                       1]; /* Checksum. */
static U8 raw_to_avr[1 + /* Power mode    */
                     1 + /* PWM frequency */
                     4 + /* output % for the 4 (?!)  motors */
                     1 + /* Output modes (brakes) */
                     1 + /* Input modes (sensor power) */
                     1]; /* Checksum */

/* Serialize the to_avr data structure into raw_to_avr, ready for
 * sending to the AVR.
 */
static void avr_pack_to_avr(void) {
  U32 i;
  U8 checksum = 0;

  memset(raw_to_avr, 0, sizeof(raw_to_avr));

  /* Marshal the power mode configuration. */
  switch (to_avr.power_mode) {
  case AVR_RUN:
    /* Normal operating mode. First byte is 0, and the second (PWM
     * frequency is set to 8.
     */
    raw_to_avr[1] = 8;
    break;
  case AVR_POWER_OFF:
    /* Tell the AVR to shut us down. */
    raw_to_avr[0] = 0x5A;
    raw_to_avr[1] = 0;
    break;
  case AVR_RESET_MODE:
    /* Tell the AVR to boot SAM-BA. */
    raw_to_avr[0] = 0x5A;
    raw_to_avr[1] = 0xA5;
  }

  /* Marshal the motor speed settings. */
  raw_to_avr[2] = to_avr.motor_speed[0];
  raw_to_avr[3] = to_avr.motor_speed[1];
  raw_to_avr[4] = to_avr.motor_speed[2];

  /* raw_to_avr[5] is the value for the 4th motor, which doesn't
   * exist. This is probably a bug in the AVR firmware, but it is
   * required. So we just latch the value to zero.
   */

  /* Marshal the motor brake settings. */
  raw_to_avr[6] = to_avr.motor_brake;

  /* Calculate the checksum. */
  for (i=0; i<(sizeof(raw_to_avr)-1); i++)
    checksum += raw_to_avr[i];
  raw_to_avr[sizeof(raw_to_avr)-1] = ~checksum;
}

/* Small helper to convert two bytes into an U16. */
static inline U16 unpack_word(U8 *word) {
  return *((U16*)word);
}

/* Deserialize the AVR data structure in raw_from_avr into the
 * from_avr status structure.
 */
static void avr_unpack_from_avr(void) {
  U8 checksum = 0;
  U16 word;
  U32 voltage;
  U32 i;
  U8 *p = raw_from_avr;

  /* Compute the checksum of the received data. This is done by doing
   * the unsigned sum of all the bytes in the received buffer. They
   * should add up to 0xFF.
   */
  for (i=0; i<sizeof(raw_from_avr); i++)
    checksum += raw_from_avr[i];

  if (checksum != 0xff) {
    avr_state.failed_consecutive_checksums++;
    return;
  } else {
    avr_state.failed_consecutive_checksums = 0;
  }

  /* Unpack and store the 4 sensor analog readings. */
  for (i = 0; i < NXT_N_SENSORS; i++) {
    from_avr.adc_value[i] = unpack_word(p);
    p += 2;
  }

  /* Grab the buttons word (an analog reading), and compute the state
   * of buttons from that.
   */
  word = unpack_word(p);
  p += 2;

  if (word > 1023)
    from_avr.buttons = BUTTON_OK;
  else if (word > 720)
    from_avr.buttons = BUTTON_CANCEL;
  else if (word > 270)
    from_avr.buttons = BUTTON_RIGHT;
  else if (word > 60)
    from_avr.buttons = BUTTON_LEFT;
  else
    from_avr.buttons = BUTTON_NONE;

  /* Process the last word, which is a mix and match of many
   * values.
   */
  word = unpack_word(p);

  /* Extract the AVR firmware version, as well as the type of power
   * supply connected.
   */
  from_avr.version.major = (word >> 13) & 0x3;
  from_avr.version.minor = (word >> 10) & 0x7;
  from_avr.battery.is_aa = (word & 0x8000) ? TRUE : FALSE;

  /* The rest of the word is the voltage value, in units of
   * 13.848mV. As the NXT does not have a floating point unit, the
   * multiplication by 13.848 is approximated by a multiplication by
   * 3545 followed by a division by 256.
   */
  voltage = word & 0x3ff;
  voltage = (voltage * 3545) >> 9;
  from_avr.battery.charge = voltage;
}

/* Initialize the NXT-AVR communication. */
void nx__avr_init(void) {
  /* Set up the TWI driver to turn on the i2c bus, and kickstart the
   * state machine to start transmitting.
   */
  nx__twi_init();
  avr_state.mode = AVR_LINK_DOWN;
}

/* The main AVR driver state machine. This routine gets called
 * periodically every millisecond by the system timer code.
 *
 * It is called directly in the main system timer interrupt, and so
 * must return as fast as possible.
 */
void nx__avr_fast_update(void) {
  /* The action taken depends on the state of the AVR
   * communication.
   */
  switch (avr_state.mode) {
  case AVR_UNINITIALIZED:
    /* Because the system timer can call this update routine before
     * the driver is initialized, we have this safe state. It does
     * nothing and immediately returns.
     *
     * When the AVR driver initialization code runs, it will set the
     * state to AVR_LINK_DOWN, which will kickstart the state machine.
     */
    return;

  case AVR_LINK_DOWN:
    /* ARM-AVR link is not initialized. We need to send the hello
     * string to tell the AVR that we are alive. This will (among
     * other things) stop the "clicking brick" sound, and avoid having
     * the brick powered down after a few minutes by an AVR that
     * doesn't see us coming up.
     */
    nx__twi_write_async(AVR_ADDRESS, (U8*)avr_init_handshake,
                    sizeof(avr_init_handshake)-1);
    avr_state.failed_consecutive_checksums = 0;
    avr_state.mode = AVR_INIT;
    break;

  case AVR_INIT:
    /* Once the transmission of the handshake is complete, go into a 2
     * millisecond wait, which is accomplished by the use of two
     * intermediate state machine states.
     */
    if (nx__twi_ready())
      avr_state.mode = AVR_WAIT_2MS;
    break;

  case AVR_WAIT_2MS:
    /* Wait another millisecond... */
    avr_state.mode = AVR_WAIT_1MS;
    break;

  case AVR_WAIT_1MS:
    /* Now switch the state to send mode, but also set the receive
     * done flag. On the next refresh cycle, the communication will be
     * in "production" mode, and will start by reading data back from
     * the AVR.
     */
    avr_state.mode = AVR_SEND;
    break;

  case AVR_SEND:
    /* If the transmission is complete, switch to receive mode and
     * read the status structure from the AVR.
     */
    if (nx__twi_ready()) {
      avr_state.mode = AVR_RECV;
      memset(raw_from_avr, 0, sizeof(raw_from_avr));
      nx__twi_read_async(AVR_ADDRESS, raw_from_avr,
                     sizeof(raw_from_avr));
    }

  case AVR_RECV:
    /* If the transmission is complete, unpack the read data into the
     * from_avr struct, pack the data in the to_avr struct into a raw
     * buffer, and shovel that over the i2c bus to the AVR.
     */
    if (nx__twi_ready()) {
      avr_unpack_from_avr();
      /* If the number of failed consecutive checksums is over the
       * restart threshold, consider the link down and reboot the
       * link. */
      if (avr_state.failed_consecutive_checksums >= AVR_MAX_FAILED_CHECKSUMS) {
        avr_state.mode = AVR_LINK_DOWN;
      } else {
        avr_state.mode = AVR_SEND;
        avr_pack_to_avr();
        nx__twi_write_async(AVR_ADDRESS, raw_to_avr, sizeof(raw_to_avr));
      }
    }
    break;
  }
}

U32 nx__avr_get_sensor_value(U32 n) {
  NX_ASSERT(n < NXT_N_SENSORS);

  return from_avr.adc_value[n];
}

void nx__avr_set_motor(U32 motor, int power_percent, bool brake) {
  NX_ASSERT(motor < NXT_N_MOTORS);

  to_avr.motor_speed[motor] = power_percent;
  if (brake)
    to_avr.motor_brake |= (1 << motor);
  else
    to_avr.motor_brake &= ~(1 << motor);
}

void nx__avr_power_down(void) {
  while (1)
    to_avr.power_mode = AVR_POWER_OFF;
}

void nx__avr_firmware_update_mode(void) {
  while (1)
    to_avr.power_mode = AVR_RESET_MODE;
}

nx_avr_button_t nx_avr_get_button(void) {
  return from_avr.buttons;
}

U32 nx_avr_get_battery_voltage(void) {
  return from_avr.battery.charge;
}

bool nx_avr_battery_is_aa(void) {
  return from_avr.battery.is_aa;
}

void nx_avr_get_version(U8 *major, U8 *minor) {
  if (major)
    *major = from_avr.version.major;
  if (minor)
    *minor = from_avr.version.minor;
}
