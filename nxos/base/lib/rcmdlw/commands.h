#ifndef __COMMANDS_H__
#define __COMMANDS_H__

#include "base/types.h"

/** @addtogroup lib
 *@{*/

/** Interpreter for command
 *
 * The remote command can be parsed by using this procedure. The buffer will
 * be refilled with the acknowledgment for the required command and, if
 * required, with the data to send back.
 *
 * @param buffer The command buffer;
 * @param len The command buffer's length;
 * @param req_ack Will be setted to TRUE if the command requires
 *                acknowledgment.
 * @return TRUE if the interpretation succeded, FALSE otherwise.
 */
bool nx_cmd_interpret(U8 *buffer, size_t len, bool *req_ack);

/*@}*/

#endif /* __COMMANDS_H__ */

