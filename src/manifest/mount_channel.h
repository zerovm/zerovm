/*
 * mount_channel.h
 *
 *  Created on: Dec 6, 2011
 *      Author: dazo
 */

#ifndef MOUNT_CHANNEL_H_
#define MOUNT_CHANNEL_H_

#include "api/zvm_manifest.h"
#include "src/service_runtime/sel_ldr.h"

EXTERN_C_BEGIN
/*
 * return zvm preopened file descriptor by channel number
 */
struct PreOpenedFileDesc *GetChannelById(
    struct SetupList *setup, enum ChannelType desc);

/*
 * mount given channel (must be constructed) with a given mode/attributes
 * return 0 - when everything is ok, otherwise - negative error
 */
int MountChannel(struct NaClApp *nap, enum ChannelType ch);

/*
 * return size of given file or -1 (max_size) if fail
 */
uint64_t GetFileSize(const char *name);

/*
 * preallocate channel. if size of the file less then allowed
 * extend it to allowed size. for output files only.
 * note: must be called from Pre*Channel() after file opened and measured
 */
void PreallocateChannel(struct PreOpenedFileDesc* channel);

/*
 * preload given file (channel). return 0 if success, otherwise negative errcode
 */
int PreloadChannel(struct NaClApp *nap, struct PreOpenedFileDesc* channel);

/*
 * premap given file (channel). return 0 if success, otherwise negative errcode
 */
int PremapChannel(struct NaClApp *nap, struct PreOpenedFileDesc* channel);

/*
 * preallocate given network channel. return 0 if success, otherwise negative errcode
 */
int PrefetchChannel(struct NaClApp *nap, struct PreOpenedFileDesc* channel);

EXTERN_C_END
#endif /* MOUNT_CHANNEL_H_ */
