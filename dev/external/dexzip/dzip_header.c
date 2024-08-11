//////////////////////////////////////////////////////////
// DexZip - a game friendly package format              //
// Created by Tomasz "Dex" Jonarski						//
// Public Domain										//
//////////////////////////////////////////////////////////

#include "dzipint.h"

int dzip_allocentry( struct dzip* zip )
{
	#if defined(W2_PLATFORM_XBOX360)
		// no, no, no, no....
		set_error_code( DZIP_ERROR_WRONG_PLATFORM );
		return -1;
	#else
	/* out of place of entries, grow */
	if ( zip->nentry_alloc == zip->nentry )
	{
		/* grow by 2 */
		zip->nentry_alloc = zip->nentry_alloc * 2;
		if ( zip->nentry_alloc < 16 )
		{
			/* fallback for empty table */
			zip->nentry_alloc = 16;
		}

		/* grow table */
		zip->entry = (struct dzip_entry*) dzip_realloc_memory( zip->entry, sizeof( struct dzip_entry ) * zip->nentry_alloc );
		memset( zip->entry + zip->nentry, 0, sizeof( struct dzip_entry ) * ( zip->nentry_alloc - zip->nentry ) );
	}

	/* allocate element */
	zip->nentry = zip->nentry + 1;
	return zip->nentry-1;
	#endif
}

void dzip_freeentry( struct dzip* zip, int index )
{
	#if defined(W2_PLATFORM_XBOX360)
	// you like delete things, don't you ?
	return;
	#else
	struct dzip_entry* entry = &zip->entry[ index ];
	int i;

	/* clear file name to mark that entry is free */
	dzip_free_memory( entry->filename );
	entry->filename = NULL;

	/* clear unneeded stats */
	entry->time = 0;
	entry->usize = 0;
	entry->csize = 0;
	entry->offset = 0;

	/* fallback to header size */
	zip->eoffset = sizeof( struct dzip_header );

	/* calculate new position at the end of file */
	for ( i=zip->nentry-1; i>=0; --i )
	{
		if ( zip->entry[i].filename )
		{
			/* clamp file size */
			zip->eoffset = zip->entry[i].offset + zip->entry[i].csize;

			/* done */
			break;
		}
	}
	#endif
}

long long dzip_allocspace( struct dzip* zip, long long neededSize )
{
	#if defined(W2_PLATFORM_XBOX360)
	// there's no space... live with it...
	return -1;
	#else
	long long offset;

	/* allocate from file end */
	offset = zip->eoffset;
	zip->eoffset += neededSize;

	/* return file offset */
	return offset;
	#endif
}

void dzip_writetables( struct dzip* zip )
{
	#if defined(W2_PLATFORM_XBOX360)
	// you filthy little bastard...
	return;
	#else
	struct dzip_header header;
	struct dzip_entry* entry;
	unsigned short len;
	int i;
	unsigned long long tablesEnd;

	// calculate the file table offset 
	header.eoffset = zip->eoffset;
	header.magic = 'PIZD'; // LOL
	header.version = 2;    // version 2 with CRC
	header.nentry = 0;

	// count number of files
	for ( i=0; i<zip->nentry; i++ )
	{
		entry = &zip->entry[i];
		if ( entry->filename )
		{
			// count files
			header.nentry++;
		}
	}

	// go to the end of the data
	dfile_seek( zip->zp, header.eoffset );

	// save each file entry
	for ( i=0; i<zip->nentry; i++ )
	{
		entry = &zip->entry[i];
		if ( entry->filename )
		{
			/* file data */
	 		len = ( unsigned short ) strlen(entry->filename) + 1;
			dfile_write( zip->zp, &len, sizeof(len)  );
			dfile_write( zip->zp, entry->filename, len );
			dfile_write( zip->zp, &entry->time, sizeof(entry->time) );
			dfile_write( zip->zp, &entry->usize, sizeof(entry->usize) );

			/* common data */
			dfile_write( zip->zp, &entry->offset, sizeof(entry->offset) );
			dfile_write( zip->zp, &entry->csize, sizeof(entry->csize) );
		}
	}

	// get tables size
	tablesEnd = dfile_tell( zip->zp );

	// calculate crc
	header.crc = dzip_calculate_header_crc( zip );

	// refresh header
	dfile_seek( zip->zp, 0 );
	dfile_write( zip->zp, &header, sizeof(header) );

	// go to the end of file (it is expected from this function...)
	dfile_seek( zip->zp, tablesEnd );

	#endif
}

DZIP_EXTERN void dzip_auto_header( struct dzip* zip, int flag )
{
	zip->autoheader = flag;
}

DZIP_EXTERN void dzip_write_header( struct dzip* zip )
{
	/* flush header and file list */
	dzip_writetables( zip );
}

static void hash_value( unsigned long long l, unsigned long long* hash )
{
	*hash ^= l;
	*hash *= 0x100000001B3;
}

unsigned long long dzip_calculate_header_crc( struct dzip* zip )
{
	int i, j, len;
	struct dzip_entry* entry;
	unsigned long long hash = 0xFFFFFFFF;

	/* process each file entry */
	for ( i=0; i<zip->nentry; i++ )
	{
		entry = reinterpret_cast<dzip_entry*>( &zip->entry[i] );
		if ( entry->filename )
		{
			/* calculate hash from filename */
			len = (int)strlen(entry->filename);
			for( j = 0; j < len; ++j )
			{
				hash_value( entry->filename[ j ], &hash );
			}

			/* add to hash other entry data */
			hash_value( len, &hash );
			hash_value( entry->time, &hash );
			hash_value( entry->usize, &hash );
			hash_value( entry->offset, &hash );
			hash_value( entry->csize, &hash );
		}
	}

	return hash;
}
