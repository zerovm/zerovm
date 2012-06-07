/*
 * comm.h
 *
 *  Created on: 05.06.2012
 *      Author: yaroslav
 */

#ifndef COMM_H_
#define COMM_H_

#include <sys/types.h>

ssize_t write_channel(int fd, const char *buf, size_t len);
ssize_t read_channel(int fd, char *buf, size_t len);

#endif /* COMM_H_ */
