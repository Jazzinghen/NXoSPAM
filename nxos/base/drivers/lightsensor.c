/* Functions set for NXT Lightsensors
 * 
 * This set includes function for:
 *
 * -SPAMlight control
 * -Raw and  Percentile Readings
 */

#include "base/drivers/sensors.h"


/** Initializes the Lightsensor in analog mode. */

void nx_lightsensor_init (U32 port) {
  nx_sensors_analog_enable(port);
}

/** Lights up the SPAMlight! */

void nx_lightsensor_fire_spamlight (U32 port) {
  nx_sensors_analog_digi_set(port, 0);
}

/** Shuts down the SPAMlight. */

void nx_lightsensor_kill_spamlight (U32 port) {
  nx_sensors_analog_digi_clear(port, 0);
}

/** Returns the RAW value of light sensed by the Lightsensor */

S32 nx_lightsensor_get_raw (U32 port) {
  return 1023 - nx_sensors_analog_get(port);
}

/** Returns the Percentile value of light */

S32 nx_lightsensor_get_light (U32 port) {
  return ((1023 - nx_sensors_analog_get(port))*100) >> 10;
}

/** Disables the Lightsensor */

void nx_lightsensor_disable (U32 port) {
  nx_sensors_analog_disable(port);
}
