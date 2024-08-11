//////////////////////////////////////////////////////////
// DexZip - a game friendly package format              //
// Created by Tomasz "Dex" Jonarski						//
// Public Domain										//
//////////////////////////////////////////////////////////

#include "dzipint.h"

#include <io.h>
#include <fcntl.h>
#include <sys/stat.h> 
#if defined(_XBOX)
#include <Xtl.h>
#else
#include <Windows.h>
#endif

int dfile_open( char* fileName, int flags )
{
	if ( flags == DZIP_CREATE )
	{
		return _open( fileName, _O_BINARY | _O_RDWR | _O_CREAT, _S_IREAD | _S_IWRITE );
	}
	else if ( flags == DZIP_READ )
	{
		return _open( fileName, _O_BINARY | _O_RDONLY, _S_IREAD ); 
	}
	else if ( flags == DZIP_READWRITE )
	{
		return _open( fileName, _O_BINARY | _O_RDWR, _S_IREAD | _S_IWRITE );
	}

	return -1; 
}

int dfile_open_w( wchar_t* fileName, int flags )
{
	if ( flags == DZIP_CREATE )
	{
		return _wopen( fileName, _O_BINARY | _O_RDWR | _O_CREAT, _S_IREAD | _S_IWRITE );
	}
	else if ( flags == DZIP_READ )
	{
		return _wopen( fileName, _O_BINARY | _O_RDONLY, _S_IREAD ); 
	}
	else if ( flags == DZIP_READWRITE )
	{
		return _wopen( fileName, _O_BINARY | _O_RDWR, _S_IREAD | _S_IWRITE );
	}

	return -1; 
}

void dfile_close( int file )
{
	_close( file );
}

unsigned long long dfile_size( int file )
{
	long long offset, size;
	offset = _telli64( file );
	_lseeki64( file, 0, SEEK_END );
	size = _telli64( file );
	_lseeki64( file, offset, SEEK_SET );
	return size;
}

unsigned long long dfile_tell( int file )
{
	return _telli64( file );
}

void dfile_seek( int file, unsigned long long pos )
{
	_lseeki64( file, pos, SEEK_SET );
}

void dfile_write( int file, void* buf, unsigned long long size )
{
	unsigned long long maxSzie = ( unsigned int ) -1;
	while ( size > maxSzie )
	{
		_write( file, buf, ( unsigned int ) maxSzie );
		size -= maxSzie;
		buf = ( ( char* ) buf ) + ( ( ptrdiff_t ) maxSzie );
	}

	_write( file, buf, ( unsigned int ) size );
}

void dfile_read( int file, void* buf, unsigned long long size )
{
/*
	if( !dzip_acc )
	{
		_read( file, buf, (size_t)size );
		return;
	}

	LARGE_INTEGER start, stop;
	QueryPerformanceCounter( &start );
	_read( file, buf, (size_t)size );
	QueryPerformanceCounter( &stop );
	if( !dzip_acc )
	{
		return;
	}
	dzip_acc( start.QuadPart, stop.QuadPart, dfile_tell( file ), size, ::GetCurrentThreadId() );*/

	unsigned long long maxSzie = ( unsigned int ) -1;
	while ( size > maxSzie )
	{
		_read( file, buf, ( unsigned int ) maxSzie );
		size -= maxSzie;
		buf = ( ( char* ) buf ) + ( ( ptrdiff_t ) maxSzie );
	}

	_read( file, buf, ( unsigned int ) size );
}

void dfile_trunc( int file, unsigned long long size )
{
	_chsize_s( file, size );
}
