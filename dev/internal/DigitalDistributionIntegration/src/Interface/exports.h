/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once

class Interface;

extern "C"
{
	typedef void* (*ReallocatorFunc)( void* ptr, size_t size );
	typedef void (*FreeFunc)( const void* ptr );

	__declspec(dllexport) Interface* CreateInterface( ReallocatorFunc allocFunc, FreeFunc freeFunc );
	__declspec(dllexport) void DestroyInterface( Interface* ptr );
};
