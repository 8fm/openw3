//////////////////////////////////////////////////////////
// DexZip - a game friendly package format              //
// Created by Tomasz "Dex" Jonarski						//
// Public Domain										//
//////////////////////////////////////////////////////////

#include "dzipint.h"

char* my_strdup( const char* string )
{
	int length = (int)strlen( string ) + 1;
	char* result = (char*) dzip_allocate_memory( length );
	strcpy( result, string );
	return result;
}

struct dzip* open_zip( int file, int created )
{
	struct dzip* zip;
	struct dzip_header header;
	struct dzip_entry* entry;
#if defined(W2_PLATFORM_XBOX360)
	struct dzip_entry entryRead;
#endif
	unsigned short len;
	int i;
	unsigned long long crc;
	(crc);

	/* create zip file */
	zip = ( struct dzip* ) dzip_allocate_memory( sizeof( struct dzip ) );
	memset( zip, 0, sizeof( struct dzip ) );

	/* initial settings */
	zip->zp = file;
	zip->autoheader = 1;

	/* load header and file table */
	if ( !created )
	{
		/* check magic */
		dfile_read( zip->zp, &header, sizeof(header) );
		SwapBytes( header.eoffset );
		SwapBytes( header.magic );
		SwapBytes( header.nentry );
		SwapBytes( header.version );
		SwapBytes( header.crc );
		if ( header.magic != 'PIZD' )
		{
			dzip_close( zip );
			return 0;
		}

		/* old version is not supported */
		if( header.version < 2 )
		{
			dzip_close( zip );
			return 0;
		}

		/* get position after last file in archive */
		zip->eoffset = header.eoffset;

		/* set file table size */
		zip->nentry = zip->nentry_alloc = header.nentry;
		if ( zip->nentry )
		{
			/* allocate file table */
#if defined(W2_PLATFORM_XBOX360)
			zip->entry = ( struct dzip_entry_memory * ) dzip_allocate_memory( sizeof( struct dzip_entry_memory ) * zip->nentry );
#else
			zip->entry = ( struct dzip_entry * ) dzip_allocate_memory( sizeof( struct dzip_entry ) * zip->nentry );
#endif

			/* go to file table */
			dfile_seek( zip->zp, header.eoffset );

			/* load entries */
			for ( i=0; i<zip->nentry; i++ )
			{
#if defined(W2_PLATFORM_XBOX360)
				// on Xbox360 read to temporary structure
				entry = &entryRead;
#else
				// on PC read directly into structures
				entry = &zip->entry[i];
#endif

				/* load file name */
				dfile_read( zip->zp, &len, sizeof(len) );
				SwapBytes( len );
				entry->filename = (char*)dzip_allocate_memory( len+1 );
				dfile_read( zip->zp, entry->filename, len );

				/* load entry data */
				dfile_read( zip->zp, &entry->time, sizeof(entry->time) );
				dfile_read( zip->zp, &entry->usize, sizeof(entry->usize) );
				dfile_read( zip->zp, &entry->offset, sizeof(entry->offset) );
				dfile_read( zip->zp, &entry->csize, sizeof(entry->csize) );
				SwapBytes( entry->csize );
				SwapBytes( entry->offset );
				SwapBytes( entry->time );
				SwapBytes( entry->usize );

#if defined(W2_PLATFORM_XBOX360)

				zip->entry[i].filename = entryRead.filename;
				zip->entry[i].offset = (unsigned int)entryRead.offset;
				zip->entry[i].csize = (unsigned int)entryRead.csize;
				zip->entry[i].usize = (unsigned int)entryRead.usize;

#endif
			}
		}
	}
	else
	{
		/* Save header in newly created file */
		zip->eoffset = sizeof( struct dzip_header );
		dzip_writetables( zip );
	}

	/* Archive opened */
	return zip;
}

DZIP_EXTERN struct dzip* dzip_open_w( wchar_t* fileName, int flags )
{
	int created=0;
	int file;

	/* open file */
	file = dfile_open_w( fileName, flags == DZIP_READ ? flags : 0 );
	if ( file == -1 )
	{
		if ( flags & DZIP_CREATE )
		{
			/* open with create flag */
			file = dfile_open_w( fileName, flags );
			if ( file == -1 )
			{
				return 0;
			}

			/* file was created */
			created = 1;		
		}
		else
		{
			return 0;
		}
	}

	return open_zip( file, created );
}

DZIP_EXTERN struct dzip* dzip_open( char* fileName, int flags )
{
	int created=0;
	int file;

	/* open file */
	file = dfile_open( fileName, flags == DZIP_READ ? flags : 0 );
	if ( file == -1 )
	{
		if ( flags & DZIP_CREATE )
		{
			/* open with create flag */
			file = dfile_open( fileName, flags );
			if ( file == -1 )
			{
				return 0;
			}

			/* file was created */
			created = 1;		
		}
		else
		{
			return 0;
		}
	}

	return open_zip( file, created );
}

static unsigned long long create_hash_from_str( const char* key, unsigned int keyLength )
{
	unsigned long long hash = 0xFFFFFFFF;
	unsigned int byte = 0, i;

	for( i=0; i<keyLength; i++)
	{
		hash ^= key[ i ] << ( byte * 8 );
		byte = ( byte + 1 ) % 8;
	}

	return hash;
}

static void mangle_str( char* str, unsigned int len, unsigned long long* key )
{
	unsigned int j, byte = 0;

	for( j = 0; j < len; ++j )
	{
		str[ j ] = str[ j ] ^ ( char )( *key >> byte );
		byte = ( byte + 1 ) % 8;

		// Modify key
		*key *= 0x100000001B3;
	}
}

DZIP_EXTERN struct dzip* dzip_open_with_encryption( char* fileName, int flags, const char* key, unsigned int keyLength )
{
	struct dzip* zip;
	struct dzip_header header;
	struct dzip_entry* entry;
#if defined(W2_PLATFORM_XBOX360)
	struct dzip_entry entryRead;
#endif
	unsigned short len;
	int i, created=0;
	int file;
	unsigned long long crc, hashedkey;
	(crc);
	/* convert key */
	hashedkey = create_hash_from_str( key, keyLength );

	/* open file */
	file = dfile_open( fileName, flags == DZIP_READ ? flags : 0 );
	if ( file == -1 )
	{
		if ( flags & DZIP_CREATE )
		{
			/* open with create flag */
			file = dfile_open( fileName, flags );
			if ( file == -1 )
			{
				return 0;
			}

			/* file was created */
			created = 1;		
		}
		else
		{
			return 0;
		}
	}

	/* create zip file */
	zip = ( struct dzip* ) dzip_allocate_memory( sizeof( struct dzip ) );
	memset( zip, 0, sizeof( struct dzip ) );

	/* initial settings */
	zip->zp = file;
	zip->autoheader = 1;

	/* load header and file table */
	if ( !created )
	{
		/* check magic */
		dfile_read( zip->zp, &header, sizeof(header) );
		SwapBytes( header.eoffset );
		SwapBytes( header.magic );
		SwapBytes( header.nentry );
		SwapBytes( header.version );
		SwapBytes( header.crc );
		if ( header.magic != 'PIZD' )
		{
			dzip_close( zip );
			return 0;
		}

		/* old version is not supported */
		if( header.version < 2 )
		{
			dzip_close( zip );
			return 0;
		}

		/* get position after last file in archive */
		zip->eoffset = header.eoffset;

		/* set file table size */
		zip->nentry = zip->nentry_alloc = header.nentry;
		if ( zip->nentry )
		{
			/* allocate file table */
#if defined(W2_PLATFORM_XBOX360)
			zip->entry = ( struct dzip_entry_memory * ) dzip_allocate_memory( sizeof( struct dzip_entry_memory ) * zip->nentry );
#else
			zip->entry = ( struct dzip_entry * ) dzip_allocate_memory( sizeof( struct dzip_entry ) * zip->nentry );
#endif

			/* go to file table */
			dfile_seek( zip->zp, header.eoffset );

			/* load entries */
			for ( i=0; i<zip->nentry; i++ )
			{
#if defined(W2_PLATFORM_XBOX360)
				// on Xbox360 read to temporary structure
				entry = &entryRead;
#else
				// on PC read directly into structures
				entry = &zip->entry[i];
#endif

				/* load file name */
				dfile_read( zip->zp, &len, sizeof(len) );
				SwapBytes( len );
				entry->filename = (char*)dzip_allocate_memory( len+1 );
				dfile_read( zip->zp, entry->filename, len );

				/* decrypt filename */
				mangle_str( entry->filename, len, &hashedkey );

				/* load entry data */
				dfile_read( zip->zp, &entry->time, sizeof(entry->time) );
				dfile_read( zip->zp, &entry->usize, sizeof(entry->usize) );
				dfile_read( zip->zp, &entry->offset, sizeof(entry->offset) );
				dfile_read( zip->zp, &entry->csize, sizeof(entry->csize) );
				SwapBytes( entry->csize );
				SwapBytes( entry->offset );
				SwapBytes( entry->time );
				SwapBytes( entry->usize );

#if defined(W2_PLATFORM_XBOX360)

				zip->entry[i].filename = entryRead.filename;
				zip->entry[i].offset = (unsigned int)entryRead.offset;
				zip->entry[i].csize = (unsigned int)entryRead.csize;
				zip->entry[i].usize = (unsigned int)entryRead.usize;

#endif
			}
		}
	}
	else
	{
		/* Save header in newly created file */
		zip->eoffset = sizeof( struct dzip_header );
		dzip_writetables( zip );
	}

	/* Archive opened */
	return zip;
}
