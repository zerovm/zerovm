/*
 * trap.h
 *
 *  Created on: Dec 2, 2011
 *      Author: d'b
 */

#ifndef TRAP_H_
#define TRAP_H_

#include "api/zvm.h"
#include "src/loader/sel_ldr.h"
#include "src/channels/mount_channel.h"

EXTERN_C_BEGIN

/*
 * jump/call alignement of untrusted code. will prevent jumping into
 * the middle of instruction and execution of uncontrolled code sequence
 */
#define OP_ALIGNEMENT 0x20

/*
 * our "One Ring" syscall main routine
 * call given syscall with the given parameteres supposed to be invoked from NaClSysNanosleep()
 *
 * 1st parameter is a pointer to the command (function, arg1, argv2,..)
 * 2nd parameter is a pointer to return value(s)
 * return 0 if successful, otherwise - 1 (nanosleep restriction)
 * note: in a future will be moved to "nacl_syscall_handler.c" to be only syscall
 */
int32_t TrapHandler(struct NaClApp *nap, uint32_t args);

/* helper. should be used with macros */
/* todo(d'b): yakk! */
int ChannelIOMask(struct ChannelDesc *channel);
/* macros uses channel type and limits */
#define CHANNEL_READABLE(channel) ((ChannelIOMask(channel) & 1) == 1)
#define CHANNEL_WRITEABLE(channel) ((ChannelIOMask(channel) & 2) == 2)
/* macros uses only channel type */
#define CHANNEL_SEQ_READABLE(channel) (channel->type == 0 || channel->type == 2)
#define CHANNEL_SEQ_WRITEABLE(channel) (channel->type == 0 || channel->type == 1)
#define CHANNEL_RND_READABLE(channel) (channel->type == 1 || channel->type == 3)
#define CHANNEL_RND_WRITEABLE(channel) (channel->type == 2 || channel->type == 3)

EXTERN_C_END

#endif /* TRAP_H_ */
