//////////////////////////////////////////////////////////
// DexZip - a game friendly package format              //
// Created by Tomasz "Dex" Jonarski						//
// Public Domain										//
//////////////////////////////////////////////////////////

#include "dzipint.h"

static void dfile_moveData( int file, long long srcOffset, long long destOffset, long long size )
{
	long long blockSize;
	void* buf;

	/* create buffer */
	buf = dzip_allocate_memory( DZIP_BLOCK_SIZE );

	/* move data */
	while ( size )
	{
		/* calculate size of the block to move */
		blockSize = size;
		if ( blockSize > DZIP_BLOCK_SIZE )
		{
			blockSize = DZIP_BLOCK_SIZE;
		}

		/* read source data */
		dfile_seek( file, srcOffset );
		dfile_read( file, buf, blockSize );

		/* write target data */
		dfile_seek( file, destOffset );
		dfile_write( file, buf, blockSize );

		/* advance */
		srcOffset += blockSize;
		destOffset += blockSize;
		size -= blockSize;
	}

	/* free buffer */
	dzip_free_memory( buf );
}

#if !defined(W2_PLATFORM_XBOX360)
static int dzip_compareEntriesOffset( const void *a, const void *b ) 
{
	struct dzip_entry* entryA, *entryB;
	entryA = *(struct dzip_entry**)a;
	entryB = *(struct dzip_entry**)b;
	if ( entryA->offset < entryB->offset ) return -1;
	if ( entryA->offset > entryB->offset ) return 1;
	return 0;
}
#endif

DZIP_EXTERN int dzip_compact( struct dzip* zip )
{
	#if defined(W2_PLATFORM_XBOX360)
	// why don't you try compacting your brain, before trying to compact something on the xbox360
	return -1;
	#else
	unsigned long long fileOffset = 0;
	struct dzip_entry** entriesOrder;
	long long realSize;
	int i, j, numValid;

	/* count valid entries */
	numValid = 0;
	for ( i=0; i<zip->nentry; i++ )
	{
		if ( zip->entry[i].filename )
		{
			numValid++;
		}
	}

	/* compact space by moving valid entries to the beginning of the file */
	fileOffset = sizeof( struct dzip_header );
	if ( numValid )
	{
		/* sort entries by position in the file */
		entriesOrder = ( struct dzip_entry** ) dzip_allocate_memory( sizeof( struct dzip_entry* ) * numValid );
		for ( i=0, j=0; i<zip->nentry; i++ )
		{
			if ( zip->entry[i].filename )
			{
				entriesOrder[ j ] = &zip->entry[i];
				j++;
			}
		}

		/* sort entries */
		qsort( entriesOrder, numValid, sizeof( struct dzip_entry* ), dzip_compareEntriesOffset );

		/* move files */
		for ( i=0; i<numValid; i++ )
		{
			/* move valid files */
			if ( entriesOrder[i]->filename )
			{
				/* update progress */
				dzip_progress( entriesOrder[i]->filename, i, numValid );

				/* move */
				if ( entriesOrder[i]->offset != fileOffset )
				{
					/* move data */
					dfile_moveData( zip->zp, entriesOrder[i]->offset, fileOffset, entriesOrder[i]->csize );

					/* set new offset */
					entriesOrder[i]->offset = fileOffset;
				}

				/* advance */
				fileOffset += entriesOrder[i]->csize;
			}
		}
	}

	/* set the size */
	zip->eoffset = fileOffset;

	/* write new header */
	dzip_writetables( zip );
	
	/* get real size of file after tables were written */
	realSize = dfile_tell( zip->zp );
 
	/* clamp file */
	dfile_trunc( zip->zp, realSize );	

	/* done */
	return 0;
	#endif	
}
