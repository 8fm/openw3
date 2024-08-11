//////////////////////////////////////////////////////////
// DexZip - a game friendly package format              //
// Created by Tomasz "Dex" Jonarski						//
// Public Domain										//
//////////////////////////////////////////////////////////

#include "dzipint.h"

DZIP_EXTERN int dzip_get_num_files(struct dzip * zip)
{
	if ( zip )
	{
		return zip->nentry;
	}

	return 0;
}

DZIP_EXTERN const char *dzip_get_name(struct dzip * zip, int index )
{
	if ( zip )
	{
		if ( index >= 0 && index < zip->nentry )
		{
			/* return entry file name, can by NULL if entry was deleted */
			return zip->entry[index].filename;
		}
	}

	return 0;
}

DZIP_EXTERN long long dzip_get_time(struct dzip * zip, int index)
{
	#if !defined(W2_PLATFORM_XBOX360)
	if ( zip )
	{
		if ( index >= 0 && index < zip->nentry )
		{
			/* return entry access time */
			return zip->entry[index].time;
		}
	}
	#endif
	return 0;
}

DZIP_EXTERN long long dzip_get_size(struct dzip * zip, int index)
{
	if ( zip )
	{
		if ( index >= 0 && index < zip->nentry )
		{
			/* return entry access time */
			return zip->entry[index].usize;
		}
	}

	return 0;
}

DZIP_EXTERN long long dzip_get_csize(struct dzip * zip, int index)
{
	if ( zip )
	{
		if ( index >= 0 && index < zip->nentry )
		{
			/* return entry access time */
			return zip->entry[index].csize;
		}
	}

	return 0;
}

DZIP_EXTERN long long dzip_get_wastedspace(struct dzip * zip)
{
	long long allFiles, totalSize;
	int i;

	/* count file size */
	allFiles = sizeof( struct dzip_header );
	for ( i=0; i<zip->nentry; i++ )
	{
		if ( zip->entry[i].filename )
		{
			/* file size */
			allFiles += zip->entry[i].csize;

			/* count entry data */
			allFiles += sizeof( unsigned short ); // strlen
			allFiles += strlen( zip->entry[i].filename ) + 1; // filename
			allFiles += sizeof( long long ) * 4; // entry info
		}
	}

	/* count wasted space */
	totalSize = dfile_size( zip->zp );
	return totalSize - allFiles;
}


