/*
 * base class for premap, preload and prefetch. contain utility functions
 * and main routine to mount/unmount a channel
 *
 *  Created on: Dec 5, 2011
 *      Author: d'b
 */
#include <stdio.h>
#include <fcntl.h>

#include <src/manifest/manifest_parser.h>
#include <src/manifest/manifest_setup.h>
#include "src/manifest/mount_channel.h"

/*
 * mount given channel (must be constructed) with a given mode/attributes
 * return 0 - when everything is ok, otherwise - negative error
 */
int MountChannel(struct NaClApp *nap, enum ChannelType ch)
{
  struct PreOpenedFileDesc *channel = &nap->manifest->user_setup->channels[ch];
  if(channel)
  {
    switch(channel->mounted)
    {
      int code;
      case MAPPED:
        code = PremapChannel(nap, channel);
        COND_ABORT(code, "cannot premap channel\n");
        break;
      case LOADED:
        code = PreloadChannel(nap, channel);
        COND_ABORT(code, "cannot preload channel\n");
        break;
      case NETWORK:
        code = PrefetchChannel(nap, channel);
        COND_ABORT(code, "cannot allocate network channel\n");
        break;
      default:
        COND_ABORT(1, "mounting method not supported\n");
        break;
    }
  }
  return 0;
}

/*
 * return size of given file or -1 (max_size) if fail
 */
uint64_t GetFileSize(const char *name)
{
  struct stat fs;
  int handle;
  int i;

  handle = open(name, O_RDONLY);
  if(handle < 0) return -1;

  i = fstat(handle, &fs);
  close(handle);
  return i < 0 ? -1 : fs.st_size;
}
