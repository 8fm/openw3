//////////////////////////////////////////////////////////
// DexZip - a game friendly package format              //
// Created by Tomasz "Dex" Jonarski						//
// Public Domain										//
//////////////////////////////////////////////////////////

#ifndef _D_ZIP_FILE_INT_H_
#define _D_ZIP_FILE_INT_H_

extern int dfile_open( char*, int );
extern int dfile_open_w( wchar_t*, int );
extern void dfile_close( int );
extern unsigned long long dfile_size( int );
extern unsigned long long dfile_tell( int );
extern void dfile_seek( int, unsigned long long );
extern void dfile_write( int, void*, unsigned long long );
extern void dfile_read( int, void*, unsigned long long );
extern void dfile_trunc( int, unsigned long long );

#endif /* _D_ZIP_FILE_INT_H_ */