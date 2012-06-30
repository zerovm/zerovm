
/*
 ** 2007 September 14
 **
 ** The author disclaims copyright to this source code.  In place of
 ** a legal notice, here is a blessing:
 **
 **    May you do good and not evil.
 **    May you find forgiveness for yourself and forgive others.
 **    May you share freely, never taking more than you give.
 **
 *************************************************************************
 **
 ** OVERVIEW:
 **
 **   This file contains some example code demonstrating how the SQLite
 **   vfs feature can be used to have SQLite operate directly on an
 **   embedded media, without using an intermediate file system.
 **
 **   Because this is only a demo designed to run on a workstation, the
 **   underlying media is simulated using a regular file-system file. The
 **   size of the file is fixed when it is first created (default size 10 MB).
 **   From SQLite's point of view, this space is used to store a single
 **   database file and the journal file.
 **
 **   Any statement journal created is stored in volatile memory obtained
 **   from sqlite3_malloc(). Any attempt to create a temporary database file
 **   will fail (SQLITE_IOERR). To prevent SQLite from attempting this,
 **   it should be configured to store all temporary database files in
 **   main memory (see pragma "temp_store" or the SQLITE_TEMP_STORE compile
 **   time option).
 **
 ** ASSUMPTIONS:
 **
 **   After it has been created, the blob file is accessed using the
 **   following three functions only:
 **
 **       mediaRead();            - Read a 512 byte block from the file.
 **       mediaWrite();           - Write a 512 byte block to the file.
 **       mediaSync();            - Tell the media hardware to sync.
 **
 **   It is assumed that these can be easily implemented by any "real"
 **   media vfs driver adapting this code.
 **
 ** FILE FORMAT:
 **
 **   The basic principle is that the "database file" is stored at the
 **   beginning of the 10 MB blob and grows in a forward direction. The
 **   "journal file" is stored at the end of the 10MB blob and grows
 **   in the reverse direction. If, during a transaction, insufficient
 **   space is available to expand either the journal or database file,
 **   an SQLITE_FULL error is returned. The database file is never allowed
 **   to consume more than 90% of the blob space. If SQLite tries to
 **   create a file larger than this, SQLITE_FULL is returned.
 **
 **   No allowance is made for "wear-leveling", as is required by.
 **   embedded devices in the absence of equivalent hardware features.
 **
 **   The first 512 block byte of the file is reserved for storing the
 **   size of the "database file". It is updated as part of the sync()
 **   operation. On startup, it can only be trusted if no journal file
 **   exists. If a journal-file does exist, then it stores the real size
 **   of the database region. The second and subsequent blocks store the
 **   actual database content.
 **
 **   The size of the "journal file" is not stored persistently in the
 **   file. When the system is running, the size of the journal file is
 **   stored in volatile memory. When recovering from a crash, this vfs
 **   reports a very large size for the journal file. The normal journal
 **   header and checksum mechanisms serve to prevent SQLite from
 **   processing any data that lies past the logical end of the journal.
 **
 **   When SQLite calls OsDelete() to delete the journal file, the final
 **   512 bytes of the blob (the area containing the first journal header)
 **   are zeroed.
 **
 ** LOCKING:
 **
 **   File locking is a no-op. Only one connection may be open at any one
 **   time using this demo vfs.
 */

#include "sqlite3.h"
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h> //rand
#include <assert.h>
#include "vfs_channel.h"


#define CHECK_FLAGS(totest, flags)((totest&flags) == flags ?1 :0)

/*
 ** Maximum pathname length supported by the fs backend.
 */
#define MAX_PATH_NAME 50
#define BLOCKSIZE 512
#define BLOBSIZE 10485760


typedef struct tmp_file tmp_file;
struct tmp_file {
	sqlite3_file base;
	const char *zName;
	int nSize;
	int nAlloc;
	char *zAlloc;
	int input_chann_fd;         /* >=0 fd, -1 not suported*/
	int nRef;                   /* Number of pointers to this structure */
	tmp_file *pNext;
	tmp_file **ppThis;
};

/*
 ** Method declarations for tmp_file.
 */
static int tmpClose(sqlite3_file*);
static int tmpRead(sqlite3_file*, void*, int iAmt, sqlite3_int64 iOfst);
static int tmpWrite(sqlite3_file*, const void*, int iAmt, sqlite3_int64 iOfst);
static int tmpTruncate(sqlite3_file*, sqlite3_int64 size);
static int tmpSync(sqlite3_file*, int flags);
static int tmpFileSize(sqlite3_file*, sqlite3_int64 *pSize);
static int tmpLock(sqlite3_file*, int);
static int tmpUnlock(sqlite3_file*, int);
static int tmpCheckReservedLock(sqlite3_file*, int *pResOut);
static int tmpFileControl(sqlite3_file*, int op, void *pArg);
static int tmpSectorSize(sqlite3_file*);
static int tmpDeviceCharacteristics(sqlite3_file*);

/*
 ** Method declarations for fs_vfs.
 */
static int fsOpen(sqlite3_vfs*, const char *, sqlite3_file*, int , int *);
static int fsDelete(sqlite3_vfs*, const char *zName, int syncDir);
static int fsAccess(sqlite3_vfs*, const char *zName, int flags, int *);
static int fsFullPathname(sqlite3_vfs*, const char *zName, int nOut,char *zOut);
static void *fsDlOpen(sqlite3_vfs*, const char *zFilename);
static void fsDlError(sqlite3_vfs*, int nByte, char *zErrMsg);
static void (*fsDlSym(sqlite3_vfs*,void*, const char *zSymbol))(void);
static void fsDlClose(sqlite3_vfs*, void*);
static int fsRandomness(sqlite3_vfs*, int nByte, char *zOut);
static int fsSleep(sqlite3_vfs*, int microseconds);
static int fsCurrentTime(sqlite3_vfs*, double*);


typedef struct fs_vfs_t fs_vfs_t;
struct fs_vfs_t {
	sqlite3_vfs base;
	struct tmp_file *pFileList;
#ifndef ORIGIN
	int input_channel_fd;
#endif
};

static fs_vfs_t fs_vfs = {
		{
				1,                                          /* iVersion */
				0,                                          /* szOsFile */
				0,                                          /* mxPathname */
				0,                                          /* pNext */
				FS_VFS_NAME,                                /* zName */
				0,                                          /* pAppData */
				fsOpen,                                     /* xOpen */
				fsDelete,                                   /* xDelete */
				fsAccess,                                   /* xAccess */
				fsFullPathname,                             /* xFullPathname */
				fsDlOpen,                                   /* xDlOpen */
				fsDlError,                                  /* xDlError */
				fsDlSym,                                    /* xDlSym */
				fsDlClose,                                  /* xDlClose */
				fsRandomness,                               /* xRandomness */
				fsSleep,                                    /* xSleep */
				fsCurrentTime,                              /* xCurrentTime */
				0                                           /* xCurrentTimeInt64 */
		}
};


static sqlite3_io_methods tmp_io_methods = {
		1,                            /* iVersion */
		tmpClose,                     /* xClose */
		tmpRead,                      /* xRead */
		tmpWrite,                     /* xWrite */
		tmpTruncate,                  /* xTruncate */
		tmpSync,                      /* xSync */
		tmpFileSize,                  /* xFileSize */
		tmpLock,                      /* xLock */
		tmpUnlock,                    /* xUnlock */
		tmpCheckReservedLock,         /* xCheckReservedLock */
		tmpFileControl,               /* xFileControl */
		tmpSectorSize,                /* xSectorSize */
		tmpDeviceCharacteristics,     /* xDeviceCharacteristics */
		0,                            /* xShmMap */
		0,                            /* xShmLock */
		0,                            /* xShmBarrier */
		0                             /* xShmUnmap */
};

/* Useful macros used in several places */
#define MIN(x,y) ((x)<(y)?(x):(y))
#define MAX(x,y) ((x)>(y)?(x):(y))


/*
 ** Close a tmp-file.
 */
static int tmpClose(sqlite3_file *pFile){
	tmp_file *pTmp = (tmp_file *)pFile;

	/* Decrement the real_file ref-count. */
	pTmp->nRef--;
	assert(pTmp->nRef>=0);

	sqlite3_free(pTmp->zAlloc);
	return SQLITE_OK;
}

/*
 ** Read data from a tmp-file.
 */
static int tmpRead(
		sqlite3_file *pFile,
		void *zBuf,
		int iAmt,
		sqlite_int64 iOfst
){
	tmp_file *pTmp = (tmp_file *)pFile;
	if( (iAmt+iOfst)>pTmp->nSize ){
		memset(zBuf+iOfst, 0, iAmt );
		return SQLITE_IOERR_SHORT_READ;
	}
	memcpy(zBuf, &pTmp->zAlloc[iOfst], iAmt);
	return SQLITE_OK;
}

/*
 ** Write data to a tmp-file.
 */
static int tmpWrite(
		sqlite3_file *pFile,
		const void *zBuf,
		int iAmt,
		sqlite_int64 iOfst
){
	tmp_file *pTmp = (tmp_file *)pFile;
	if( (iAmt+iOfst)>pTmp->nAlloc ){
		int nNew = (int)(2*(iAmt+iOfst+pTmp->nAlloc));
		char *zNew = sqlite3_realloc(pTmp->zAlloc, nNew);
		if( !zNew ){
			return SQLITE_NOMEM;
		}
		pTmp->zAlloc = zNew;
		pTmp->nAlloc = nNew;
	}
	memcpy(&pTmp->zAlloc[iOfst], zBuf, iAmt);
	pTmp->nSize = (int)MAX(pTmp->nSize, iOfst+iAmt);
	return SQLITE_OK;
}

/*
 ** Truncate a tmp-file.
 */
static int tmpTruncate(sqlite3_file *pFile, sqlite_int64 size){
	tmp_file *pTmp = (tmp_file *)pFile;
	pTmp->nSize = (int)MIN(pTmp->nSize, size);
	return SQLITE_OK;
}

/*
 ** Sync a tmp-file.
 */
static int tmpSync(sqlite3_file *pFile, int flags){
	return SQLITE_OK;
}

/*
 ** Return the current file-size of a tmp-file.
 */
static int tmpFileSize(sqlite3_file *pFile, sqlite_int64 *pSize){
	tmp_file *pTmp = (tmp_file *)pFile;
	*pSize = pTmp->nSize;
	return SQLITE_OK;
}

/*
 ** Lock a tmp-file.
 */
static int tmpLock(sqlite3_file *pFile, int eLock){
	return SQLITE_OK;
}

/*
 ** Unlock a tmp-file.
 */
static int tmpUnlock(sqlite3_file *pFile, int eLock){
	return SQLITE_OK;
}

/*
 ** Check if another file-handle holds a RESERVED lock on a tmp-file.
 */
static int tmpCheckReservedLock(sqlite3_file *pFile, int *pResOut){
	*pResOut = 0;
	return SQLITE_OK;
}

/*
 ** File control method. For custom operations on a tmp-file.
 */
static int tmpFileControl(sqlite3_file *pFile, int op, void *pArg){
	return SQLITE_OK;
}

/*
 ** Return the sector-size in bytes for a tmp-file.
 */
static int tmpSectorSize(sqlite3_file *pFile){
	return BLOCKSIZE;
}

/*
 ** Return the device characteristic flags supported by a tmp-file.
 */
static int tmpDeviceCharacteristics(sqlite3_file *pFile){
	return 0;
}


/*
 ** Open an fs file handle.
 */
static int fsOpen(
		sqlite3_vfs *pVfs,
		const char *zName,
		sqlite3_file *pFile,
		int flags,
		int *pOutFlags
){
	fs_vfs_t *pFsVfs = (fs_vfs_t *)pVfs;
	tmp_file *pTmp = (tmp_file *)pFile;
	tmp_file *pTmpInList = 0;
	int nName = strlen(zName);
	int rc = SQLITE_OK;

	/*search opening file in our file list of opened files*/
	pTmpInList = pFsVfs->pFileList;
	for(; pTmpInList && strncmp(pTmpInList->zName, zName, nName); pTmpInList=pTmpInList->pNext);

	if (!pTmpInList){
		int journal = CHECK_FLAGS(flags, SQLITE_OPEN_MAIN_JOURNAL);
		int db = CHECK_FLAGS(flags, SQLITE_OPEN_MAIN_DB);
		int read_only = CHECK_FLAGS(flags, SQLITE_OPEN_READONLY);

		if( journal ){
			memset(pTmp, 0, sizeof(*pTmp));
			pTmp->base.pMethods = &tmp_io_methods;
			pTmp->zName = zName;
			pTmp->zAlloc = sqlite3_malloc(BLOBSIZE);
			pTmp->nAlloc = BLOBSIZE;
			pTmp->nSize = 0;
			pTmp->input_chann_fd = -1;
			/*open empty journal*/
			rc = SQLITE_OK;
		}
		else if( db && read_only ){
			/*open & read main db from stdin {=input_channel_fd}*/
			memset(pTmp, 0, sizeof(*pTmp));
			pTmp->base.pMethods = &tmp_io_methods;
			pTmp->zName = zName;
			pTmp->zAlloc = sqlite3_malloc(BLOBSIZE);
			pTmp->nAlloc = BLOBSIZE;
			pTmp->nSize = 0;
			pTmp->input_chann_fd = fs_vfs.input_channel_fd;
			if( pTmp->input_chann_fd >= 0 ){
				/*read from fd*/
				size_t chunk_read = 1024;
				ssize_t readed = -1;
				do{
					readed = read( pTmp->input_chann_fd, pTmp->zAlloc+pTmp->nSize, chunk_read );
					if ( readed > 0 ){
						pTmp->nSize += readed;
					}
				}while( chunk_read == readed && readed > 0 );
				*pOutFlags = SQLITE_OPEN_READONLY;
				rc = SQLITE_OK;
			}
		}
		else
			rc = SQLITE_ERROR;

		if (rc == SQLITE_OK){
			/*Add file to filelist*/
			tmp_file* tmp = pFsVfs->pFileList;
			pFsVfs->pFileList = pTmp;
			pFsVfs->pFileList->pNext = tmp;
			pFsVfs->pFileList->ppThis = &pFsVfs->pFileList;
		}
	}

	if (rc == SQLITE_OK){
		pTmp->nRef++;
	}
	else{
		pTmp->base.pMethods->xClose(pFile);
	}


	return rc;
}


/*
 ** Delete the file located at zPath. If the dirSync argument is true,
 ** ensure the file-system modifications are synced to disk before
 ** returning.
 */
static int fsDelete(sqlite3_vfs *pVfs, const char *zPath, int dirSync){
	int rc = SQLITE_OK;
	return rc;
}

/*
 ** Test for access permissions. Return true if the requested permission
 ** is available, or false otherwise.
 */
static int fsAccess(
		sqlite3_vfs *pVfs,
		const char *zPath,
		int flags,
		int *pResOut
){
	int rc = SQLITE_OK;
	fs_vfs_t *pFsVfs = (fs_vfs_t *)pVfs;
	tmp_file *pTmpInList = 0;
	int nName = (int)strlen(zPath);
	int isJournal = 0;

	assert(strlen("-journal")==8);
	if( nName>8 && strcmp("-journal", &zPath[nName-8])==0 ){
		isJournal = 1;
	}

	/*search opening file in our file list of opened files*/
	pTmpInList = pFsVfs->pFileList;
	for(; pTmpInList && strncmp(pTmpInList->zName, zPath, nName); pTmpInList=pTmpInList->pNext);

	*pResOut = (pTmpInList && pTmpInList->nSize);
	return rc;
}

/*
 ** Populate buffer zOut with the full canonical pathname corresponding
 ** to the pathname in zPath. zOut is guaranteed to point to a buffer
 ** of at least (FS_MAX_PATHNAME+1) bytes.
 */
static int fsFullPathname(
		sqlite3_vfs *pVfs,            /* Pointer to vfs object */
		const char *zPath,            /* Possibly relative input path */
		int nOut,                     /* Size of output buffer in bytes */
		char *zOut                    /* Output buffer */
){
	memcpy(zOut, zPath, strlen(zPath));
	return SQLITE_OK;
//	sqlite3_vfs *pParent = ((fs_vfs_t *)pVfs)->pParent;
//	return pParent->xFullPathname(pParent, zPath, nOut, zOut);
}

/*
 ** Open the dynamic library located at zPath and return a handle.
 */
static void *fsDlOpen(sqlite3_vfs *pVfs, const char *zPath){
	return NULL;
}

/*
 ** Populate the buffer zErrMsg (size nByte bytes) with a human readable
 ** utf-8 string describing the most recent error encountered associated
 ** with dynamic libraries.
 */
static void fsDlError(sqlite3_vfs *pVfs, int nByte, char *zErrMsg){
	return;
}

/*
 ** Return a pointer to the symbol zSymbol in the dynamic library pHandle.
 */
static void (*fsDlSym(sqlite3_vfs *pVfs, void *pH, const char *zSym))(void){
	return 0;
}

/*
 ** Close the dynamic library handle pHandle.
 */
static void fsDlClose(sqlite3_vfs *pVfs, void *pHandle){
	return;
}

/*
 ** Populate the buffer pointed to by zBufOut with nByte bytes of
 ** random data.
 */
static int fsRandomness(sqlite3_vfs *pVfs, int nByte, char *zBufOut){
	for( int i=0; i < nByte; i++ )
		zBufOut[i] = rand();
	return SQLITE_OK;
}

/*
 ** Sleep for nMicro microseconds. Return the number of microseconds
 ** actually slept.
 */
static int fsSleep(sqlite3_vfs *pVfs, int nMicro){
	sqlite3_vfs* parent = sqlite3_vfs_find(0);
	if ( parent )
		return parent->xSleep(parent, nMicro);
	else
		return SQLITE_ERROR;
}

/*
 ** Return the current time as a Julian Day number in *pTimeOut.
 */
static int fsCurrentTime(sqlite3_vfs *pVfs, double *pTimeOut){
	sqlite3_vfs* parent = sqlite3_vfs_find(0);
	if ( parent )
		return parent->xCurrentTime(parent, pTimeOut);
	else
		return SQLITE_ERROR;
}

/*
 ** This procedure registers the fs vfs with SQLite. If the argument is
 ** true, the fs vfs becomes the new default vfs. It is the only publicly
 ** available function in this file.
 */
int register_fs_channel(int channel_id){
#ifndef ORIGIN
	fs_vfs.input_channel_fd = channel_id;
#endif
	fs_vfs.base.mxPathname = MAX_PATH_NAME;
	fs_vfs.base.szOsFile = sizeof(tmp_file);
	return sqlite3_vfs_register(&fs_vfs.base, 1);
}

