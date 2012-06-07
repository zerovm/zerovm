/*
 * comm.c
 *
 *  Created on: 05.06.2012
 *      Author: yaroslav
 */


#include <unistd.h>
#include <stdio.h>
#include <assert.h>

#include "defines.h"

ssize_t write_channel(int fd, const char *buf, size_t len){
	WRITE_FMT_LOG(LOG_DEBUG, "%s fd=%d len=%d\n", __func__, fd, (int)len);
	ssize_t w = write(fd, buf, len);
	if (w != len ){
		WRITE_FMT_LOG(LOG_DEBUG, "%s fd=%d write=%d, wrote=%d ASSERT\n", __func__, fd, (int)len, (int)w);
		assert( w == len );
	}
	WRITE_FMT_LOG(LOG_DEBUG, "%s fd=%d, wrote=%d OK\n", __func__, fd, (int)len);
	return w;
}
ssize_t read_channel(int fd, char *buf, size_t len){
	WRITE_FMT_LOG(LOG_DEBUG, "%s fd=%d len=%d\n", __func__, fd, (int)len);
	ssize_t r = read(fd, buf, len);
	if ( r!=len ){
		WRITE_FMT_LOG(LOG_DEBUG, "%s fd=%d read=%d, readed=%d ASSERT\n", __func__, fd, (int)len, (int)r);
		assert( r == len );
	}
	WRITE_FMT_LOG(LOG_DEBUG, "%s fd=%d read=%d OK\n", __func__, fd, (int)len);
	return r;
}
