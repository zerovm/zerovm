/*
 * manifest_parse.h
 *
 *  Created on: Nov 1, 2011
 *      Author: d'b
 */
#ifndef SERVICE_RUNTIME_ZMQ_SYSCALLS_H__
#define SERVICE_RUNTIME_ZMQ_SYSCALLS_H__ 1

#define SPACES " \t"
#define EOL "\r\n"
#define MAX_STR_LEN 1024

/* show error, deallocate resources and return error code */
#define ERR(s)\
	do {\
		printf(s);\
		if(result != NULL) free(result);\
		if(str != NULL) free(str);\
		if(f != NULL) fclose(f);\
		return 0;\
	} while (0)

/*
 * open manifest file, parse it and initialize in the given
 * NaClApp structure two fields: "manifest" and "manifest_size"
 */
int ParseManifest(char *name, struct NaClApp *nap);

/*
 * get value by key from the manifest. if not found - NULL
 */
char * GetValueByKey(struct NaClApp *nap, char *key);

#endif
