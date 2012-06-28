/*
 * mount_channel.h
 *
 *  Created on: Dec 6, 2011
 *      Author: dazo
 */

#ifndef MOUNT_CHANNEL_H_
#define MOUNT_CHANNEL_H_

#include "api/zvm.h"
#include "src/service_runtime/sel_ldr.h"

EXTERN_C_BEGIN

/*
 * construct channel, mount it and update system_manifest.
 * note: space for channels must be already allocated.
 * todo(NETWORKING): put network channel initialization here
 * if successful return 0
 */
void ChannelCtor(struct NaClApp *nap, enum ChannelType ch);

/* close / deallocate resources used by channel */
void ChannelDtor(struct NaClApp *nap, enum ChannelType ch);

EXTERN_C_END
#endif /* MOUNT_CHANNEL_H_ */
