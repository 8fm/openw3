//////////////////////////////////////////////////////////
// DexZip - a game friendly package format              //
// Created by Tomasz "Dex" Jonarski						//
// Public Domain										//
//////////////////////////////////////////////////////////

#include "dzipint.h"

DZIP_EXTERN int dzip_name_locate(struct dzip * zip, const char * fileName )
{
	const char *fn;
	int i;

	/* empty file name given */
	if ( fileName == NULL )
	{
		return -1;
	}

	/* linear search */
	for ( i=0; i<zip->nentry; i++ )
	{
		fn = zip->entry[i].filename;
		if ( fn != 0 )
		{
			if ( 0 == stricmp( fileName, fn ) )
			{
				return i;
			}
		}
	}

	/* not found */
	return -1;
}
