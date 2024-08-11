//////////////////////////////////////////////////////////
// DexZip - a game friendly package format              //
// Created by Tomasz "Dex" Jonarski						//
// Public Domain										//
//////////////////////////////////////////////////////////

#ifndef _D_ZIP_H_
#define _D_ZIP_H_

#include <sys/types.h>
#include <stdio.h>
#include <time.h>
#include "fastlz.h"

#define DZIP_EXTERN

#define DZIP_CREATE				1
#define DZIP_READ				2
#define DZIP_READWRITE			3

struct dzip;
struct dzip_file;

typedef int (__stdcall *dzip_callback)( char* file, long long cur, long long total);

typedef void* (__stdcall *dzip_malloc)( size_t size );
typedef void (__stdcall *dzip_free)( void* ptr );
typedef void* (__stdcall *dzip_realloc)( void* ptr, size_t size );
DZIP_EXTERN void dzip_setallocator( dzip_malloc _allocate, dzip_free _free, dzip_realloc _realloc );

DZIP_EXTERN void dzip_setcallback( dzip_callback );

typedef void (__stdcall *dzip_logger)( const char* log );
DZIP_EXTERN void dzip_loggercallback( dzip_logger _logger);

DZIP_EXTERN struct dzip* dzip_open( char*, int );
DZIP_EXTERN struct dzip* dzip_open_w( wchar_t*, int );
DZIP_EXTERN struct dzip* dzip_open_with_encryption( char* fileName, int flags, const char* key, unsigned int keyLength );
DZIP_EXTERN void dzip_close( struct dzip* );
DZIP_EXTERN long long dzip_get_wastedspace(struct dzip *);
DZIP_EXTERN const char *dzip_get_name(struct dzip *, int);
DZIP_EXTERN long long dzip_get_time(struct dzip *, int);
DZIP_EXTERN long long dzip_get_size(struct dzip *, int);
DZIP_EXTERN long long dzip_get_csize(struct dzip *, int);
DZIP_EXTERN int dzip_get_num_files(struct dzip *);
DZIP_EXTERN int dzip_write(struct dzip *, char*, long long, void*, size_t );
DZIP_EXTERN int dzip_writeFile(struct dzip *, char*, long long, char* );
DZIP_EXTERN int dzip_name_locate(struct dzip *, const char *);
DZIP_EXTERN void dzip_auto_header( struct dzip*, int );
DZIP_EXTERN void dzip_write_header( struct dzip* );
DZIP_EXTERN struct dzip_file* dzip_fopen( struct dzip*, int );
DZIP_EXTERN void dzip_fprebuffer( struct dzip_file* );
DZIP_EXTERN void dzip_fclose( struct dzip_file* );
DZIP_EXTERN long dzip_ftell( struct dzip_file* );
DZIP_EXTERN long dzip_fsize( struct dzip_file* );
DZIP_EXTERN void dzip_fseek( struct dzip_file*, long , int );
DZIP_EXTERN size_t dzip_fread( void*, size_t, struct dzip_file* );
DZIP_EXTERN int dzip_delete( struct dzip*, int );
DZIP_EXTERN int dzip_compact( struct dzip* );
DZIP_EXTERN int dzip_get_last_error();   
DZIP_EXTERN const char *dzip_get_debug_string();   
DZIP_EXTERN int dzip_get_last_error();   
DZIP_EXTERN const char *dzip_get_debug_string();   

#endif //  _D_ZIP_H_
