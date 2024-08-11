//////////////////////////////////////////////////////////
// DexZip - a game friendly package format              //
// Created by Tomasz "Dex" Jonarski						//
// Public Domain										//
//////////////////////////////////////////////////////////

#include "dzipint.h"

static bool _dzip_initialized = false;

// 12 simultinously opened zip streams should be enough. Be warned though, increasing this number will also drastically increase memory consumption (384kB per each stream)
#define MAX_FOPEN_FILES		10

#if defined(_M_IX86)
#include <windows.h>
#elif defined(_M_PPC)
#include <xtl.h>
#endif
static CRITICAL_SECTION _dzip_critical_section;


class ScopeLock
{
public:
	ScopeLock()
	{
		EnterCriticalSection( &_dzip_critical_section );
	}
	~ScopeLock()
	{
		LeaveCriticalSection( &_dzip_critical_section );
	}

};


static struct dzip_file s_dzip_files[ MAX_FOPEN_FILES ];
static char s_dzip_files_used[ MAX_FOPEN_FILES ];

// DEBUG
static char s_dzip_files_names[ MAX_FOPEN_FILES ][ 260 ];

static void _InitializeDzip()
{
int i;

	InitializeCriticalSection( &_dzip_critical_section );

	_dzip_initialized = true;

	for( i = 0; i < MAX_FOPEN_FILES ; i++ )
	{
		s_dzip_files_used[ i ] = 0;
		s_dzip_files[ i ].cbuffer = (unsigned char*) dzip_allocate_memory( DZIP_BLOCK_SIZE*2 );
		s_dzip_files[ i ].ubuffer = (unsigned char*) dzip_allocate_memory( DZIP_BLOCK_SIZE );
	}

}

static struct dzip_file* _AllocateDzipFileStruct( const char* name )
{
int i;
	ScopeLock scope_lock;

	for( i = 0; i < MAX_FOPEN_FILES ; i++ )
	{
		if( s_dzip_files_used[ i ] == 0 )
		{
			s_dzip_files_used[ i ] = 1;
			s_dzip_files[ i ].zip = 0;
			s_dzip_files[ i ].usize = 0;
			s_dzip_files[ i ].uoffset = 0;
			s_dzip_files[ i ].numblocks = 0;
			s_dzip_files[ i ].fileSize = 0;
			s_dzip_files[ i ].fileOffset = 0;
			s_dzip_files[ i ].blocks = 0;
			s_dzip_files[ i ].pos = 0;
			s_dzip_files[ i ].next = 0;
			s_dzip_files[ i ].buffer = 0;
			s_dzip_files[ i ].isToPrebuffer = false;
			// DEBUG
			strcpy( &s_dzip_files_names[ i ][ 0 ], name );
			return &s_dzip_files[ i ];
		}
	}

	dzip_file* dzfile = (dzip_file*)dzip_allocate_memory( sizeof(dzip_file) );
	memset( dzfile, 0, sizeof(dzip_file) );
	dzfile->cbuffer = (unsigned char*) dzip_allocate_memory( DZIP_BLOCK_SIZE*2 );
	dzfile->ubuffer = (unsigned char*) dzip_allocate_memory( DZIP_BLOCK_SIZE );
	return dzfile;
	//while(true){}
	//exit(1);
}

static void _FreeDzipFileStruct( struct dzip_file* dzfile )
{
int i = 0;
	ScopeLock scope_lock;

	for( i = 0; i < MAX_FOPEN_FILES ; i++ )
	{
		if( dzfile == &s_dzip_files[ i ] )
		{
			s_dzip_files_used[ i ] = 0;
			memset( &s_dzip_files_names[ i ][ 0 ], 0, 260 );
			return;
		}
	}

	dzip_free_memory( dzfile->cbuffer );
	dzip_free_memory( dzfile->ubuffer );
	dzip_free_memory( dzfile );
	//while(true){}
	//exit(1);
}

DZIP_EXTERN struct dzip_file* dzip_fopen( struct dzip* zip, int index )
{
	if( _dzip_initialized == false )
		_InitializeDzip();

	struct dzip_file* file;
	int numBlocks;

	/* check index */
	if ( index < 0 || index >= zip->nentry )
	{
		return 0;
	}

	/* invalid file */
	if ( !zip->entry[index].filename )
	{
		return 0;
	}

	file = _AllocateDzipFileStruct( zip->entry[index].filename );


	/* load block count */
	dfile_seek( zip->zp, zip->entry[index].offset );
	numBlocks = (int)( ( zip->entry[index].usize + ( DZIP_BLOCK_SIZE - 1 ) ) / DZIP_BLOCK_SIZE );

	/* load blocks */
	file->blocks = ( unsigned* ) dzip_allocate_memory( ( numBlocks+1 ) * sizeof(unsigned) );
	dfile_read( zip->zp, file->blocks, numBlocks * sizeof(unsigned) );

	for( int i = 0; i < numBlocks; i++ )
		SwapBytes( file->blocks[ i ] );

	/* last block marks end of compressed file data */
	file->blocks[ numBlocks ] = (long)zip->entry[index].csize;

	/* copy other file info */
	file->fileOffset = zip->entry[index].offset;
	file->fileSize = (long)zip->entry[index].usize;	
	file->numblocks = numBlocks;
	file->zip = zip;

	/* reset */
	file->uoffset = 0;
	file->usize = 0;
	file->pos = 0;
	

	if( dzip_logg )
	{
		static long long previousBlockOffset = -1;

		char* fileName = 0;
		for( int i = 0; i != file->zip->nentry; ++i )
		{
			if( file->zip->entry[ i ].offset == file->fileOffset )
			{
				fileName = file->zip->entry[ i ].filename;
				break;
			}
		}

		static int buffersMaxHeap = 0;
		char temp[1024];
		sprintf( temp, "++++++ offDif:%10d dzipsize:%10d offset:%10d size:%10d memHeap:%10d memMaxHeap:%10d (((%s))) ", file->fileOffset - previousBlockOffset, dfile_size( file->zip->zp ), file->fileOffset, file->blocks[ file->numblocks ], buffersCurrentHeap, buffersMaxHeap, fileName );

		previousBlockOffset = file->fileOffset + file->blocks[ file->numblocks ];
		buffersMaxHeap = buffersCurrentHeap;
		dzip_logg( temp );
	}

	/* done */
	return file;
}

DZIP_EXTERN void dzip_fprebuffer( struct dzip_file* file )
{
	long blockSize;

	if( file->buffer ) return;

	blockSize = file->blocks[ file->numblocks ];

	file->buffer = dzip_allocate_memory( blockSize );

	if( file->buffer )
	{
		buffersCurrentHeap += blockSize;
		file->isToPrebuffer = true;
		return;
	}

	if( !dzip_logg ) return;

	char temp[1024];
	sprintf( temp, "++++++ LACK OF MEMORY FOR PREBUFFERING DZIP RESOURCE requestedSize:%i ++++++", blockSize );
	dzip_logg( temp );
}

DZIP_EXTERN void dzip_fclose( struct dzip_file* file )
{
	if ( file )
	{
		if( file->buffer )
		{
			buffersCurrentHeap -= file->blocks[ file->numblocks ];

			dzip_free_memory( file->buffer );
			file->buffer = 0;
		}

		/* free buffers */
		dzip_free_memory( file->blocks );
		file->blocks = NULL;


		_FreeDzipFileStruct( file );
	}
}

DZIP_EXTERN long dzip_ftell( struct dzip_file* file )
{
	return file->pos;
}

DZIP_EXTERN long dzip_fsize( struct dzip_file* file )
{
	return file->fileSize;
}

DZIP_EXTERN void dzip_fseek( struct dzip_file* file, long offset, int mode )
{
	/* change file position */
	if ( mode == SEEK_SET )
	{
		file->pos = offset;
	}
	else if ( mode == SEEK_CUR )
	{
		file->pos += offset;
	}
	else if ( mode == SEEK_END )
	{
		file->pos = file->fileSize - offset;
	}
}

DZIP_EXTERN size_t dzip_fread( void* buf, size_t size, struct dzip_file* file )
{
	int blockIndex;
	long uend, readCount=0, maxRead, uleft;

	/* calculate end of block */
	uend = file->uoffset + file->usize;

	/* fastest case: the read is inside current block */
	if ( ( file->pos >= file->uoffset ) && ( ( file->pos + size ) < (size_t)uend ) )
	{
		/* copy data */
		memcpy( buf, file->ubuffer + ( file->pos - file->uoffset ), size );
		file->pos += long( size );

		/* done */
		return size;
	}

	/* find block index within the range */
	blockIndex = file->pos / DZIP_BLOCK_SIZE;
	if ( blockIndex < 0 || blockIndex >= file->numblocks )
	{
		/* eof */
		return 0;
	}

	/* decompress initial block if reading pos is outside current block */
	if ( (file->pos < file->uoffset) || (file->pos >= (file->uoffset + file->usize)) )
	{
		/* decompress block */
		if ( 0 == dzip_uncompressblock( file, blockIndex ) )
		{
			/* error decompressing block */
			return 0;
		}
	}

	/* read data */
	readCount = 0;
	while ( size > 0 )
	{
		/* how much can we read from current block ? */
		uleft = file->usize - ( file->pos - file->uoffset );

		/* calculate how much to read */
		maxRead = long( size );
		if ( maxRead > uleft )
		{
			maxRead = uleft;
		}

		/* copy data */
		memcpy( buf, file->ubuffer + ( file->pos - file->uoffset ), maxRead );

		/* advance */
		buf = (char*)buf + maxRead;
		file->pos += maxRead;
		size -= maxRead;
		readCount += maxRead;

		/* decompress next block */
		if ( size > 0 )
		{
			/* go to next block */
			blockIndex = blockIndex + 1;

			/* decompress it */
			if ( !dzip_uncompressblock( file, blockIndex ) )
			{
				break;
			}
		}
	}
	
	/* return amount of data read */
	return readCount;
}

char GDebugString[ 1024 ] = {0};

FILE* dzip_tmpfile()
{
	 CHAR* fileName = _tempnam( NULL, "dzip" );  
	 if ( fileName == NULL )
	 {
		 return NULL;
	 }

	 strcpy_s( GDebugString, 1024, fileName );

	 FILE* file = fopen( fileName, "wbTD+" );
	 free( fileName );
	 return file;
}

DZIP_EXTERN const char *dzip_get_debug_string()
{
	return GDebugString;
}
