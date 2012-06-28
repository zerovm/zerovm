/*
 * test for user manifest accessors provided by zrt
 */
#include <stdio.h>
#include "api/zvm.h"

/* to show enum values as strings */
static char *enum_strings[] = {
    "InputChannel",
    "OutputChannel",
    "LogChannel",
    "NetworkInputChannel",
    "NetworkOutputChannel"
};

int main(int argc, char **argv)
{
  enum ChannelType ch;

  /* other user manifest fields */
#define SHOW_S_FIELD(func) printf(#func" = %s\n", func())
#define SHOW_I_FIELD(func) printf(#func" = 0x%X\n", func())
  SHOW_S_FIELD(zvm_content_type);
  SHOW_S_FIELD(zvm_timestamp);
  SHOW_S_FIELD(zvm_x_object_meta_tag);
  SHOW_S_FIELD(zvm_user_etag);
  SHOW_I_FIELD(zvm_syscallback);
  printf("\n");

  /* show channels */
#define SHOW_CH_I_FIELD(func, ch) printf(#func"(%s) = 0x%X\n", enum_strings[ch], func(ch))
#define SHOW_CH_II_FIELD(func, ch) printf(#func"(%s) = 0x%llX\n", enum_strings[ch], func(ch))
#define SHOW_CH_S_FIELD(func, ch) printf(#func"(%s) = %s\n", enum_strings[ch], func(ch))
  for(ch = InputChannel; ch < ChannelTypesCount; ++ch)
  {
    /* main fields */
    SHOW_CH_I_FIELD(zvm_channel_addr, (int32_t)ch);
    SHOW_CH_I_FIELD(zvm_channel_size, ch);
    SHOW_CH_S_FIELD(zvm_channel_name, ch);
    SHOW_CH_I_FIELD(zvm_channel_handle, ch);
    SHOW_CH_I_FIELD(zvm_channel_mode, ch);
    SHOW_CH_I_FIELD(zvm_channel_type, ch);
    SHOW_CH_II_FIELD(zvm_channel_fsize, ch);

    /* limits */
    SHOW_CH_II_FIELD(zvm_channel_get_size_limit, ch);
    SHOW_CH_II_FIELD(zvm_channel_get_count_limit, ch);
    SHOW_CH_II_FIELD(zvm_channel_put_size_limit, ch);
    SHOW_CH_II_FIELD(zvm_channel_put_count_limit, ch);

    /* counters */
    SHOW_CH_II_FIELD(zvm_channel_get_size_count, ch);
    SHOW_CH_II_FIELD(zvm_channel_get_count_count, ch);
    SHOW_CH_II_FIELD(zvm_channel_put_size_count, ch);
    SHOW_CH_II_FIELD(zvm_channel_put_count_count, ch);
    printf("\n");
  }

  /* try to set syscallback to invalid address */
    
  return 0;
}
