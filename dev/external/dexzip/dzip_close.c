//////////////////////////////////////////////////////////
// DexZip - a game friendly package format              //
// Created by Tomasz "Dex" Jonarski						//
// Public Domain										//
//////////////////////////////////////////////////////////

#include "dzipint.h"

DZIP_EXTERN void dzip_close( struct dzip* zip )
{
	int i;

	if ( zip )
	{
		/* free file names */
		for ( i=0; i<zip->nentry; i++ )
		{
			dzip_free_memory( zip->entry[i].filename );
		}

		/* free entry table */
		dzip_free_memory( zip->entry );

		/* close file */
		dfile_close( zip->zp );

		/* free memory */
		dzip_free_memory( zip );
	}
}
