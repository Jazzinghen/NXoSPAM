#include "base/drivers/motors.h"
#include "base/drivers/radar.h"
#include "base/types.h"
#include "base/core.h"

#include "commands.h"
#include "base/util.h"

typedef enum {
  NOBRAKE             = (0<<4),       /* Brake disabled */
  BRAKE               = (1<<4),       /* Brake enabled */

  WHR_TIME            = (0<<3),       /* Rotation for a given time */
  WHR_ANGLE           = (1<<3),       /* Rotation of a given angle */

  MOV_RUN             = (0<<3),       /* Run forward until new command */
  MOV_TURN            = (1<<3),       /* Turn */

  MOTOR_0             = 1,            /* Motor 0 selected */
  MOTOR_1             = 2,            /* Motor 1 selected */
  MOTOR_2             = 4             /* Motor 2 selected */
} variant_t;


/* HOW DOES IT WORK?
 *
 * There's a function for command interpretation and several functions for
 * building new commands. Each command is represented by a byte, and may
 * require one or more parameters.
 *
 * The command class is encoded into the three most significative bits of
 * the first byte of the buffer (little endian). The remaining bits are used
 * for command variants.
 *
 * A special class of commands, called "get" is used for data retriving.
 * This kind of command will store the retrieved data into the buffer in a
 * conventional order (depending on the request).
 *
 */

/* Generic mask for actions */
#define MSK_ACTION(cmd)         ((cmd) & 0xe0)

/* Generic brake mask for all movement */
#define MSK_BRAKE(cmd)          (((cmd) & 0x10) > 0)

/* Masks for wheel_rotate */
#define MSK_WHR_ANGLE(cmd)      ((cmd) & 0x08)

/* Masks for motor selection, used for all calls related to motors */
#define MSK_SELECT_MOTOR_0(cmd) ((cmd) & 0x01)
#define MSK_SELECT_MOTOR_1(cmd) ((cmd) & 0x02)
#define MSK_SELECT_MOTOR_2(cmd) ((cmd) & 0x04)

/* Masks for movement */
#define MSK_MOV_TURN(cmd)       ((cmd) & 0x08)

/* Masks for getters */
#define MSK_GET_INFO(cmd)       ((cmd) & 0x1f)

typedef enum {
  HALT                = (1<<5),       /* NXT Shutdown */
  ROTATE              = (2<<5),       /* Generic wheel rotation */
  STOP                = (3<<5),       /* Generic wheel stop */
  MOVE                = (4<<5),       /* Robot movement */
  GET                 = (5<<5),       /* Data retriving */
} action_t;

static bool wheel_rotate(U8 cmd, U8 *buffer) {
  void (*call) (U8, S8, U32, bool);
  S8 speed;
  U32 angle;

  speed = nx_extract_type(buffer, S8);
  if (speed > 100 || speed < -100)
    return FALSE;
  angle = nx_extract_type(buffer, U32);

  if (MSK_WHR_ANGLE(cmd))
    call = nx_motors_rotate_angle;
  else
    call = nx_motors_rotate_time;

  if (MSK_SELECT_MOTOR_0(cmd))
    call(0, speed, angle, MSK_BRAKE(cmd));
  if (MSK_SELECT_MOTOR_1(cmd))
    call(1, speed, angle, MSK_BRAKE(cmd));
  if (MSK_SELECT_MOTOR_2(cmd))
    call(2, speed, angle, MSK_BRAKE(cmd));

  return TRUE;
}

static bool wheel_stop(U8 cmd, U8 *buffer) {
  buffer = buffer;    /* Shutting down compiler :) */
  if (MSK_SELECT_MOTOR_0(cmd))
    nx_motors_stop(0, MSK_BRAKE(cmd));
  if (MSK_SELECT_MOTOR_1(cmd))
    nx_motors_stop(1, MSK_BRAKE(cmd));
  if (MSK_SELECT_MOTOR_2(cmd))
    nx_motors_stop(2, MSK_BRAKE(cmd));

  return TRUE;
}

static bool robot_move(U8 cmd, U8 *buffer) {
  S8 data_8;
  U32 data_32;

  data_8 = nx_extract_type(buffer, S8);
  if (data_8 > 100 || data_8 < -100)
    return FALSE;
  if (MSK_MOV_TURN(cmd)) {
    /* Turn the robot */
    data_32 = nx_extract_type(buffer, U32);
    nx_motors_rotate_angle(0, data_8, data_32, MSK_BRAKE(cmd));
    nx_motors_rotate_angle(2, -data_8, data_32, MSK_BRAKE(cmd));
  } else {
    /* Forward until new command */
    nx_motors_rotate(0, data_8);
    nx_motors_rotate(2, data_8);
  }

  return TRUE;
}

/* General conventions for getters:
 *
 * 1. The first byte contains an error code:
 *      == 0 -> No error
 *      != 0 -> Different error, depending on the request
 *
 * 2. The following bytes contains the data, in a specific order depending
 *    on the request
 *
 */

/* Radar getter command */
#define GETCMD_RADAR 1

/* Radar detection error */
#define GETCMD_RADAR_EDETECT 1

/* Radar access definition, used for the radar getter. The radar must be
 * positioned on the physical port 1 */
#define RADAR 0

static bool get_info(U8 cmd, U8 *buffer) {
  cmd = MSK_GET_INFO(cmd);
  buffer=buffer;
  switch (cmd) {
  case GETCMD_RADAR:
    if (nx_radar_detect(RADAR)) {
      buffer[0] = 0;
      buffer[1] = nx_radar_read_distance(RADAR, 0);
    } else
      buffer[0] = GETCMD_RADAR_EDETECT;
    break;
  default:
    return FALSE;
  }
  return TRUE;
}

bool nx_cmd_interpret(U8 *buffer, size_t len, bool *req_ack) {
  U8 cmd;
  bool (*call) (U8, U8 *) = NULL;

  if (len < 6)
    return FALSE;

  cmd = buffer[0];
  *req_ack = FALSE;
  buffer++;
  switch (MSK_ACTION(cmd)) {
  case HALT:
    nx_core_halt();
    break;
  case ROTATE:
    call = wheel_rotate;
    break;
  case STOP:
    call = wheel_stop;
    break;
  case MOVE:
    call = robot_move;
    break;
  case GET:
    *req_ack = TRUE;
    call = get_info;
    break;
  default:
    return FALSE;
  }
  return call(cmd, buffer);
}
