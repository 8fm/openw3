//////////////////////////////////////////////////////////
// DexZip - a game friendly package format              //
// Created by Tomasz "Dex" Jonarski						//
// Public Domain										//
//////////////////////////////////////////////////////////

#include "dzipint.h"

int GErrorCode;

DZIP_EXTERN int dzip_get_last_error()
{
	return GErrorCode;
}

void set_error_code( int code )
{
	GErrorCode = code;
}	

DZIP_EXTERN int dzip_write(struct dzip * zip, char* fileName, long long fileTime, void* data, size_t usize )
{
	char buffer[ DZIP_BLOCK_SIZE ];
	FILE* tm;
	long long csize, offset, block;
	int index;

	/* pack data to temporary file */
	tm = dzip_pack( data, usize, 6 );
	if ( !tm )
	{
		// do not set error code here, set inside dzip_pack instead!
		return -1;
	}

	/* calculate file size */
	fseek( tm, 0, SEEK_END );
	csize = ftell( tm );
	if ( csize == 0 )
	{	
		set_error_code( DZIP_ERROR_ZERO_SIZE );
		fclose(tm);
		return -1;
	}

	/* try to reuse existing place */
	index = dzip_name_locate( zip, fileName );
	if ( index != -1 && zip->entry[index].csize >= (unsigned long long)csize )
	{
		/* save in the existing entry */
		offset = zip->entry[index].offset;
	}
	else
	{
		/* allocate new space in the archive */
		offset = dzip_allocspace( zip, csize );

		/* allocate new entry */
		if ( index == -1 )		
		{
			index = dzip_allocentry( zip );
			zip->entry[index].filename = my_strdup( fileName );
		}

		/* update offset */
		zip->entry[index].offset = offset;
	}

	/* update entry */
	zip->entry[index].csize = csize;
	zip->entry[index].usize = usize;
#ifndef W2_PLATFORM_XBOX360
	zip->entry[index].time = fileTime;
#endif

	/* copy file content */
	{
		/* rewind */
		fseek( tm, 0, SEEK_SET );
		dfile_seek( zip->zp, offset );

		/* write in blocks */
		while ( csize > 0 )
		{
			/* calculate block size */
			block = csize;
			if ( block > sizeof(buffer) )
			{
				block = sizeof(buffer);			
			}
			
			/* read source data */
			fread( buffer, 1, (size_t)block, tm );

			/* write to target */
			dfile_write( zip->zp, buffer, block );

			/* advance */
			csize -= block;
		}

		/* close source file */
		fclose( tm );
	}

	/* flush header and file list */
	if ( zip->autoheader )
	{
		dzip_writetables( zip );
	}

	/* return file index */
	return index;

}

int dzip_writeFile(struct dzip * zip, char* fileName, long long fileTime, char* absoluteFilePath )
{
#if defined(W2_PLATFORM_XBOX360)

	// what in heavens name you're trying to write ?!
	return -1;

#else

	char buffer[ DZIP_BLOCK_SIZE ];
	FILE* tm;
	long long csize, offset, block;
	size_t usize;
	int index;

	/* pack data to temporary file */
	tm = dzip_packFile( absoluteFilePath, 6, &usize );
	if ( !tm )
	{
		return -1;
	}

	/* calculate file size */
	fseek( tm, 0, SEEK_END );
	csize = ftell( tm );
	if ( csize == 0 )
	{		
		fclose(tm);
		return -1;
	}

	/* try to reuse existing place */
	index = dzip_name_locate( zip, fileName );
	if ( index != -1 && zip->entry[index].csize >= (unsigned long long)csize )
	{
		/* save in the existing entry */
		offset = zip->entry[index].offset;
	}
	else
	{
		/* allocate new space in the archive */
		offset = dzip_allocspace( zip, csize );

		/* allocate new entry */
		if ( index == -1 )		
		{
			index = dzip_allocentry( zip );
			zip->entry[index].filename = my_strdup( fileName );
		}

		/* update offset */
		zip->entry[index].offset = offset;
	}

	/* update entry */
	zip->entry[index].csize = csize;
	zip->entry[index].usize = usize;
	zip->entry[index].time = fileTime;

	/* copy file content */
	{
		/* rewind */
		fseek( tm, 0, SEEK_SET );
		dfile_seek( zip->zp, offset );

		/* write in blocks */
		while ( csize > 0 )
		{
			/* calculate block size */
			block = csize;
			if ( block > sizeof(buffer) )
			{
				block = sizeof(buffer);			
			}

			/* read source data */
			fread( buffer, 1, (size_t)block, tm );

			/* write to target */
			dfile_write( zip->zp, buffer, block );

			/* advance */
			csize -= block;
		}

		/* close source file */
		fclose( tm );
	}

	/* flush header and file list */
	if ( zip->autoheader )
	{
		dzip_writetables( zip );
	}

	/* return file index */
	return index;

#endif
}
