/*
 * this sample demonstrate zerovm api
 */

#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "api/zvm.h"

#define BIG_ENOUGH 0x10000

/* put stack to the channel 'channel_handle' in readable format*/
void print_stack(int channel_handle, const int* ptr)
{
    struct UserManifest *setup;
    struct ZVMChannel *channels;

    /* initiate api */
    setup = zvm_init();
    channels = setup->channels;

    int log_handle=2;
    char buf[BIG_ENOUGH];

    int i=0;
    do{
        const char *c= (const char*)&ptr[i];
        memset(buf, '\0', BIG_ENOUGH);
        int size = sprintf(buf, "\n sizeof(NULL)=%d, sizeof(c)=%d, address=%p, value=%08X, %c%c%c%c",
                (int)sizeof(NULL), (int)sizeof(c), &ptr[i], ptr[i], c[0], c[1], c[2], c[3] );
        zvm_pwrite(log_handle, buf, size, 0);
        i++;
    }while( &ptr[i] >= (int*)0xFFFFFF );
}

/* We are substitute glibc POSIX function to our defined function.
 * */
int mkdir(const char* pathname, mode_t mode){
    int *stack = (void*)&stack;
    if ( pathname != NULL ){
        print_stack( 2, stack);
    }
    return 0;
}

int main(int argc, char **argv)
{
    char str[] = {"mkdir"};
    //print_stack( 2, str);
    char* s=NULL;
    mkdir(s, 0xEEEEEE);
    (void)str;

    /* return 0 */
    zvm_exit(0);
    return 0; /* not reachable */
}
