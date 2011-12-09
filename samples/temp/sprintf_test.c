#include <stdio.h>

#define PUT_2_LOG(log, ...) sprinf(log += strlen(log), __VA_ARGS__)

int main()
{
 char *p = malloc(100);
 
 sprintf(p, "%x\n", 3735943697);
 printf("%s", p);
}
