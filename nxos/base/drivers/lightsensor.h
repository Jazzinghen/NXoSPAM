/** @file lightsensor.h
 *  @brief Lightsensor functions.
 *
 * List of Function to work with NXT Lightsensor.
 */

/* Copyright (C) 2010 the NXoSPAM developers
 *
 * Written by Michele "Jazzinghen" Bianchi.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */

#ifndef __NXOSPAM_LIGHTSENSOR_H__
#define __NXOSPAM_LIGHTSENSOR_H__

#include "base/types.h"

/** Initialize light sensor on port @a.
 *
 * @param port The Lightsensor port.
 */
 
void nx_lightsensor_init (U32 port);


/** Set ON the SPAMlight of the Lightsensor on port @a port
 *
 *  @param port The Lightsensor port.
 */

void nx_lightsensor_fire_spamlight (U32 port);

/** Set OFF the SPAMlight of the Lightsensor on port @a port
 *
 *  @param port The Lightsensor port.
 */

void nx_lightsensor_kill_spamlight (U32 port);

/** Get raw light level for Lightsensor on port @a port
 *
 *  @param port The Lightsensor port
 *  @return An S32. The raw value of the light read by the Lightsensor
 */

S32 nx_lightsensor_get_raw (U32 port);

/** Get the light level for Lightsensor on port @a port
 *
 *  @param port The Lightsensor port
 *  @return An S32. The percentual value of the light read by the Lightsensor
 */

S32 nx_lightsensor_get_light (U32 port);

/** Disable the Lightsensor on port @a port.
 *
 * @param port The Lightsensor port.
 */
void nx_lightsensor_disable (U32 port);

#endif /* __NXOSPAM_LIGHTSENSOR_H__ */
