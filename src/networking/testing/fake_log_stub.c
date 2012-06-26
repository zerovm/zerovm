/*
 * fake_log.c
 *
 *  Created on: 20.06.2012
 *      Author: yaroslav
 */

#include "src/platform/nacl_log.h"

void NaClLog(int         detail_level,
             char const  *fmt,
             ...){
	(void)detail_level;
	(void)fmt;
}
