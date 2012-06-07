/*
 * errcodes.h
 *
 *  Created on: 25.05.2012
 *      Author: YaroslavLitvinov
 */

#ifndef ERRCODES_H_
#define ERRCODES_H_

enum zmq_errcode{ERR_OK=0, ERR_ERROR=-1, ERR_ALREADY_EXIST=-2, ERR_NOT_FOUND=-3, ERR_NO_MEMORY=-4, ERR_BAD_ARG=-5};

enum zvm_errcode{ EZVMUNKNOWN=-1, EZVM_OK, EZVM_NOT_INITED, EZVMBAD_ARG, EZVMDBNOTEXIST_OREMPTY=1, EZVM_DBREADERR=2, EZVM_SOCK_ERROR=3};

enum zmq_sock_capab{ ENOTALLOWED=0, EREAD=1, EWRITE=2, EREADWRITE=3};

#endif /* ERRCODES_H_ */
