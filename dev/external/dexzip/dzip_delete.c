//////////////////////////////////////////////////////////
// DexZip - a game friendly package format              //
// Created by Tomasz "Dex" Jonarski						//
// Public Domain										//
//////////////////////////////////////////////////////////

#include "dzipint.h"

DZIP_EXTERN int dzip_delete( struct dzip* zip, int index )
{
	if ( zip )
	{
		if ( index >= 0 && index < zip->nentry )
		{
			dzip_freeentry( zip, index );
			return 1;
		}
	}

	return 0;
}
