/** @file systick.h
 *  @brief System timer interface.
 */

/* Copyright (C) 2007 the NxOS developers
 *
 * See AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */

#ifndef __NXOS_BASE_DRIVERS_SYSTICK_H__
#define __NXOS_BASE_DRIVERS_SYSTICK_H__

#include "base/types.h"

/** @addtogroup driver */
/*@{*/

/** @defgroup systick System timer
 *
 * The system timer is in charge of (surprise!) keeping system time. It
 * counts the number of milliseconds elapsed since bootup, provides a
 * few busy waiting functions, and allows application kernels to install
 * a scheduling callback that will run periodically.
 */
/*@{*/

/** Return the number of milliseconds elapsed since bootup. */
U32 nx_systick_get_ms(void);

/** Sleep for @a ms milliseconds.
 *
 * @param ms The number of milliseconds to sleep.
 *
 * @note As the Baseplate provides no scheduler, this sleeping is a busy
 * wait loop.
 */
void nx_systick_wait_ms(U32 ms);

/** Sleep for approximately @a ns nanoseconds.
 *
 * @param ns The number of nanoseconds to sleep.
 *
 * @note This sleep routine is a busy loop whose accuracy is based
 * entirely on the instruction timings and pipeline delays in the ARM7
 * cpu. It may not be exact.
 */
void nx_systick_wait_ns(U32 ns);

/** Install @a scheduler_cb as the scheduler callback.
 *
 * The scheduler callback will be invoked every millisecond once it is
 * installed. The scheduler callback runs in a medium priority interrupt
 * handler (higher than all the device drivers expect for the AVR link
 * and system timer).
 *
 * @param scheduler_cb The scheduler callback to install.
 *
 * @note The callback does not have to implement a scheduler. It's just
 * that implementing a scheduler is the most common reason to want such
 * a periodic callback.
 */
void nx_systick_install_scheduler(nx_closure_t scheduler_cb);

/** Trigger a call to the installed scheduler callback.
 *
 * The callback will be run in a low priority interrupt handler, as if
 * it had been called by the system timer.
 *
 * @note If no scheduler callback has been installed (by
 * ns_systick_install_scheduler()), the call has no effect.
 */
void nx_systick_call_scheduler(void);

/** Inhibit the scheduler callback temporarily.
 *
 * This will simply prevent the systick driver from calling the
 * scheduler. It may still be invoked manually with
 * nx_systick_call_scheduler().
 */
void nx_systick_mask_scheduler(void);

/** Uninhibit the scheduler callback.
 *
 * Use this to reenable calling the scheduler interrupt after
 * deactivating it with nx_systick_mask_scheduler().
 */
void nx_systick_unmask_scheduler(void);

/*@}*/
/*@}*/

#endif /* __NXOS_BASE_DRIVERS_SYSTICK_H__ */
