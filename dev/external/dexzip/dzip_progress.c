//////////////////////////////////////////////////////////
// DexZip - a game friendly package format              //
// Created by Tomasz "Dex" Jonarski						//
// Public Domain										//
//////////////////////////////////////////////////////////

#include "dzipint.h"

dzip_callback dzip_callbackFunc = NULL;

void dzip_setcallback( dzip_callback func )
{
	dzip_callbackFunc = func;
}

void dzip_progress( char* file, long long cur, long long total )
{
	if ( dzip_callbackFunc )
	{
		(*dzip_callbackFunc)( file, cur, total );
	}
}

dzip_malloc dzip_allocate_memory;
dzip_free dzip_free_memory;
dzip_realloc dzip_realloc_memory;

void dzip_setallocator( dzip_malloc _allocate, dzip_free _free, dzip_realloc _realloc )
{
	dzip_allocate_memory = _allocate;
	dzip_free_memory = _free;
	dzip_realloc_memory = _realloc;
}

dzip_logger dzip_logg = 0;
void dzip_loggercallback( dzip_logger _logger)
{
	dzip_logg = _logger;
}

int buffersCurrentHeap = 0;