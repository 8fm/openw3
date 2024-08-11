//////////////////////////////////////////////////////////
// DexZip - a game friendly package format              //
// Created by Tomasz "Dex" Jonarski						//
// Public Domain										//
//////////////////////////////////////////////////////////

#include "dzipint.h"

FILE* dzip_pack( void* data, size_t size, int level )
{
	int numBlocks, i, usize, csize;
	unsigned char *outBuf;
	unsigned* blockTable;
	unsigned char paddedData[16];
	FILE* tm;

	/* count number of blocks needed */
	numBlocks = ( ( ( int ) size ) + ( DZIP_BLOCK_SIZE - 1 ) ) / DZIP_BLOCK_SIZE;

	/* open temporary file */
	tm = dzip_tmpfile();
	if ( !tm )
	{
		/* unable to open temporary file */
		set_error_code( DZIP_ERROR_OPEN_TEMP_FAILED );
		return NULL;
	}

	/* seek to start */
	fseek( tm, 0, SEEK_SET );

	/* write block table */
	blockTable = ( unsigned * ) dzip_allocate_memory( numBlocks * sizeof(unsigned) );
	memset( blockTable, 0, numBlocks * sizeof(unsigned) );
	if ( fwrite( blockTable, sizeof(unsigned), numBlocks, tm ) != numBlocks )
	{
		set_error_code( DZIP_ERROR_FWRITE_FAILED );
		fclose( tm );
		return NULL;
	}

	/* allocate buffer */
	outBuf = ( unsigned char* ) dzip_allocate_memory( 2*DZIP_BLOCK_SIZE );

	/* compress file in blocks */
	for ( i=0; i<numBlocks; i++ )
	{
		/* save block offset */
		blockTable[i] = ftell( tm );
		SwapBytes( blockTable[i] );

		/* calculate size of data to compress */
		usize = (int)( (size > DZIP_BLOCK_SIZE) ? DZIP_BLOCK_SIZE : size );

		{
			/* fallback for to small data */
			if ( usize < 16 )
			{
				/* limit to 16 bytes to meet the fastlz reqs. */
				memcpy( paddedData, data, usize );
				data = paddedData;
				usize = 16;
			}

			/* compress data */
			csize = fastlz_compress( data, usize, outBuf );
			if ( csize == 0 )
			{
				/* error compressing */
				fclose( tm );
				dzip_free_memory( outBuf );
				set_error_code( DZIP_ERROR_COMPRESS_FAILED );
				return NULL;
			}
			
			/* write to output */
			if ( fwrite( outBuf, 1, csize, tm ) != csize || ferror(tm) )
			{
				/* error writing result */
				fclose( tm );
				dzip_free_memory( outBuf );
				set_error_code( DZIP_ERROR_FWRITE_FAILED );
				return NULL;
			}
		}

		/* advance */
		data = ( char* ) data + usize;
		size = size - usize;
	}

	/* free output buffer */
	dzip_free_memory( outBuf );

	/* write final block table */
	fseek( tm, 0, SEEK_SET );
	if ( fwrite( blockTable, sizeof(unsigned), numBlocks, tm ) != numBlocks )
	{
		set_error_code( DZIP_ERROR_FWRITE_FAILED );
		fclose( tm );
		return NULL;
	}

	/* done */
	return tm;
}

FILE* dzip_packFile( char* srcAbsoluteFilePath, int level, size_t* usizeOut )
{
	int numBlocks, i, usize, csize;
	unsigned char data[ DZIP_BLOCK_SIZE ];
	size_t size;
	unsigned char *outBuf;
	unsigned* blockTable;
	FILE* tm, *rf;

	/* open source file for reading */
	rf = fopen( srcAbsoluteFilePath, "rb" );
	if ( !rf )
	{
		/* unable to open source file */
		return NULL;
	}

	/* get file size */
	fseek( rf, 0, SEEK_END );
	size = ftell( rf );
	fseek( rf, 0, SEEK_SET );

	/* emit size */
	if ( usizeOut )
	{
		*usizeOut = size;
	}

	/* count number of blocks needed */
	numBlocks = (int)( ( size + ( DZIP_BLOCK_SIZE - 1 ) ) / DZIP_BLOCK_SIZE );

	/* open temporary file */
	tm = dzip_tmpfile();
	if ( !tm )
	{
		/* unable to open temporary file */
		fclose( rf );
		return NULL;
	}

	/* seek to start */
	fseek( tm, 0, SEEK_SET );

	/* write block table */
	blockTable = ( unsigned * ) dzip_allocate_memory( numBlocks * sizeof(unsigned) );
	memset( blockTable, 0, numBlocks * sizeof(unsigned) );
	fwrite( blockTable, sizeof(unsigned), numBlocks, tm );

	/* allocate buffer */
	outBuf = ( unsigned char* ) dzip_allocate_memory( 2*DZIP_BLOCK_SIZE );

	/* compress file in blocks */
	for ( i=0; i<numBlocks; i++ )
	{
		/* save block offset */
		blockTable[i] = ftell( tm );
		SwapBytes( blockTable[i] );

		/* calculate size of data to compress */
		usize = (int)( (size > DZIP_BLOCK_SIZE) ? DZIP_BLOCK_SIZE : size );

		/* read data from file */
		fread( data, 1, usize, rf );

		{
			/* fallback for to small data */
			if ( usize < 16 )
			{
				/* limit to 16 bytes to meet the fastlz reqs. */
				usize = 16;
			}

			/* compress data */
			csize = fastlz_compress( data, usize, outBuf );
			if ( csize == 0 )
			{
				/* error compressing */
				fclose( tm );
				fclose( rf );
				dzip_free_memory( outBuf );
				return NULL;
			}

			/* write to output */
			if ( fwrite( outBuf, 1, csize, tm ) != csize || ferror(tm) )
			{
				/* error writing result */
				fclose( tm );
				fclose( rf );
				dzip_free_memory( outBuf );
				return NULL;
			}
		}

		/* advance */
		size = size - usize;
	}

	/* close source file */
	fclose( rf );

	/* free output buffer */
	dzip_free_memory( outBuf );

	/* write final block table */
	fseek( tm, 0, SEEK_SET );
	fwrite( blockTable, sizeof(unsigned), numBlocks, tm );

	/* done */
	return tm;
}

int dzip_uncompressblock( struct dzip_file* file, int blockIndex )
{
	long long blockOffset;
	long usize, blockSize;
	
	/* invalid block index */
	if ( blockIndex < 0 || blockIndex >= file->numblocks )
	{
		return 0;
	}

	if( file->buffer )
	{
		if( file->isToPrebuffer )
		{
			file->isToPrebuffer = false;

			dfile_seek( file->zip->zp, file->fileOffset );
			dfile_read( file->zip->zp, file->buffer, file->blocks[ file->numblocks ] );
		}
		blockOffset = file->blocks[ blockIndex ];
		blockSize = file->blocks[ blockIndex+1 ] - file->blocks[ blockIndex ];

		unsigned char* buffer = (unsigned char*) file->buffer;

		memcpy( file->cbuffer, buffer + blockOffset, blockSize );
	}
	else
	{
		/* get the block offset */
		blockOffset = file->fileOffset + file->blocks[ blockIndex ];
		blockSize = file->blocks[ blockIndex+1 ] - file->blocks[ blockIndex ];

		/* seek to the block */
		dfile_seek( file->zip->zp, blockOffset );

		/* read block data to uncompresed buffer */
		dfile_read( file->zip->zp, file->cbuffer, blockSize );
	}

	/* decompress block */
	usize = fastlz_decompress( file->cbuffer, blockSize, file->ubuffer, DZIP_BLOCK_SIZE );
	if ( usize == 0 )
	{
		return 0;
	}

	/* setup info about decompresed block */
	file->uoffset = blockIndex * DZIP_BLOCK_SIZE;
	file->usize = usize;

	/* return block size */
	return usize;
}
