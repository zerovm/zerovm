/*
 * this sample demonstrate zerovm api
 */
#include <stdio.h>
#include <string.h>
#include "api/zvm.h"

#define BIG_ENOUGH 0x10000

/* put message to the channel */
int print(int ch, const char *msg)
{
  return zvm_pwrite(ch, msg, strlen(msg), 0);
}

int main(int argc, char **argv)
{
  int i;
  struct UserManifest *setup;
  struct ZVMChannel *channels;
  
  /* initiate api */
  setup = zvm_init();
  channels = setup->channels;

  /* say "hello" to all available writable channels */
  for(i = 0; i < setup->channels_count; ++i)
  {
    struct ZVMChannel *channel = &channels[i];
    char buf[BIG_ENOUGH];

    /* skip not writable channels */
    if(!(channel->limits[PutsLimit] && channel->limits[PutSizeLimit]))
      continue;

    sprintf(buf, "hello, channel %s!\n", channel->name);
    print(i, buf);
  }

  /* return 0 */
  zvm_exit(0);
  return 0; /* not reachable */
}
