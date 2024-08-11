//////////////////////////////////////////////////////////
// DexZip - a game friendly package format              //
// Created by Tomasz "Dex" Jonarski						//
// Public Domain										//
//////////////////////////////////////////////////////////

#ifndef _D_ZIP_INT_H_
#define _D_ZIP_INT_H_

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>

#include "dzip.h"
#include "fastlz.h"
#include "fileint.h"

#define DZIP_ERROR_NO_ERROR			0x0000
#define DZIP_ERROR_WRONG_PLATFORM	0x1000
#define DZIP_ERROR_ZERO_SIZE		0x1001
#define DZIP_ERROR_OPEN_TEMP_FAILED	0x1002
#define DZIP_ERROR_COMPRESS_FAILED	0x1003
#define DZIP_ERROR_FWRITE_FAILED	0x1004

void set_error_code( int code );

FILE* dzip_tmpfile();

typedef void* (__stdcall *dzip_malloc)( size_t size );
typedef void (__stdcall *dzip_free)( void* ptr );

extern dzip_malloc dzip_allocate_memory;
extern dzip_free dzip_free_memory;
extern dzip_realloc dzip_realloc_memory;

extern dzip_logger dzip_logg;

extern int buffersCurrentHeap;

char* my_strdup( const char* string );

#ifdef _MSC_VER
	#pragma warning ( disable: 4996 )
#endif

#if defined(_XBOX)

#include <Xtl.h>
#include <ppcintrinsics.h>

inline void SwapBytes( unsigned int& value )
{
	value = __loadwordbytereverse( 0, &value );
}

inline void SwapBytes( int& value )
{
	value = (int)__loadwordbytereverse( 0, &value );
}

inline void SwapBytes( unsigned short& value )
{
	value = __loadshortbytereverse( 0, &value );
}

inline void SwapBytes( short& value )
{
	value = (short)__loadshortbytereverse( 0, &value );
}

inline void SwapBytes( unsigned long long& value )
{
	value = (unsigned long long)__loaddoublewordbytereverse( 0, &value );
}

#else

#include <windows.h>

inline void SwapBytes( unsigned int& value )
{
}

inline void SwapBytes( int& value )
{
}

inline void SwapBytes( unsigned short& value )
{
}

inline void SwapBytes( short& value )
{
}

inline void SwapBytes( unsigned long long& value )
{
}

#endif

#undef DZIP_EXTERN
#define DZIP_EXTERN

#define DZIP_BLOCK_SIZE		65536		// 64 KB by default
#define DZIP_PAD_SIZE		4096		// 4 KB padding of each file

// entry in the file table
struct dzip_entry
{
	char* filename;						/* filename	*/
	unsigned long long offset;			/* offset in package */
	unsigned long long csize;			/* compressed size */
	unsigned long long usize;			/* uncompressed size */
	unsigned long long time;			/* file access time */	
};

// entry in the file table (in memory)
struct dzip_entry_memory
{
	char* filename;						/* filename	*/
	unsigned int offset;				/* offset in package */
	unsigned int csize;					/* compressed size */
	unsigned int usize;					/* uncompressed size */
};

// dzip file
struct dzip
{
//	wchar_t *zn;						/* file name */
	int zp;								/* file */
	int nentry;							/* number of entries */
	int nentry_alloc;					/* number of entries allocated */
	unsigned long long eoffset;			/* positing after last file in archive */
	#if defined(W2_PLATFORM_XBOX360)
	struct dzip_entry_memory *entry;	/* entries */
	#else
	struct dzip_entry *entry;			/* entries */
	#endif
	int autoheader;						/* write header after each operation */
	struct dzip_file* files;			/* cached file handles */
};

// dzip header
struct dzip_header
{
	int magic;						/* magic number - 'DZIP' */
	int version;					/* internal version */
	int nentry;						/* number of valid entries in the archive */
	unsigned long long eoffset;		/* position of file list in the archive */
	unsigned long long crc;			/* crc of header for validation */
};

// dzip file
struct dzip_file
{
	struct dzip*		zip;			/* archive */
	unsigned char*		cbuffer;		/* compressed data buffer */
	unsigned char*		ubuffer;		/* uncompressed buffer */
	long				usize;			/* uncompressed data size */
	long				uoffset;		/* offset of uncompressed data */
	int					numblocks;		/* number of blocks */
	long				fileSize;		/* file size */
	long long			fileOffset;		/* base file offset */
	unsigned*			blocks;			/* offsets to compressed blocks */
	long				pos;			/* current read pos */
	struct dzip_file*	next;			/* next free package */
	void*				buffer;
	bool				isToPrebuffer;
};

/* entries */
int dzip_allocentry( struct dzip* zip );
void dzip_freeentry( struct dzip* zip, int index );
void dzip_writetables( struct dzip* zip );
long long dzip_allocspace( struct dzip* zip, long long neededSize );

/* write block compressed file to a temporary file */
FILE* dzip_pack( void* data, size_t size, int level );
FILE* dzip_packFile( char* srcAbsoluteFilePath, int level, size_t* usizeOut );

/* uncompress file block */
int dzip_uncompressblock( struct dzip_file* file, int blockIndex );

/* callback */
void dzip_progress( char* file, long long cur, long long total );

/* calculates CRC of a header */
unsigned long long dzip_calculate_header_crc( struct dzip* zip );

#endif
