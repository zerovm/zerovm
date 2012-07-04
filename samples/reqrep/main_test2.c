/*
 * main_test2.c
 *
 *  Created on: 29.04.2012
 *      Author: YaroslavLitvinov
 *  REQREP example is simulates two network nodes transmitting data from each to other.
 *  Data packet size=1000000bytes, and it's sending 100times in loop from one node to another and vice versa.
 */

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "api/zrt.h"

#define WRITE_FMT_LOG(fmt, args...) fprintf(stderr, fmt, args);

#define WRITE_LOG(str) fprintf(stderr, "%s\n", str);

#define FDR 3
#define FDW 4

int main(int argc, char **argv){
	WRITE_LOG("test2 started\n");
	int testlen = 1000000;
	char *buf = malloc(testlen+1);
	buf[testlen] = '\0';
	for (int i=0; i < 100; i++){
		ssize_t bwrote = write(FDW, buf, testlen);
		WRITE_FMT_LOG("#%d case1: write passed=%d, wrote=%d\n", i, testlen, (int)bwrote );
		ssize_t bread = read(FDR, buf, testlen);
		WRITE_FMT_LOG("#%d case2: read requested=%d, read=%d\n", i, testlen, (int)bread );
	}
	free(buf);
	return 0;
}
