/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once
#ifndef _ORBIS_API_CALLS_H_
#define _ORBIS_API_CALLS_H_

#ifndef RED_FINAL_BUILD
#	define DEBUG_ORBIS_API_CALLS
#endif

/*
 * If you need to be able to gracefully deal with orbis API calls failing, call the function directly and do what you need to do with the return value
 *
 * If however, the return value is only useful when debugging, or there's not much you can do if the function fails, use these macros.
 * 
 * Don't ever call an orbis API function directly without doing something with the return value
 *
 */

#ifdef DEBUG_ORBIS_API_CALLS
#	define ORBIS_SYS_CALL( func ) OrbisCheckSuccess( func, #func )
#	define ORBIS_SYS_CALL_RET( func ) if( !OrbisCheckSuccess( func, #func ) ) return false

RED_INLINE Bool OrbisCheckSuccess( Int32 sceErr, const char* funcName )
{
	if ( sceErr < SCE_OK )
	{
		RED_LOG_ERROR( OrbisAPI, TXT( "%hs failed with error code 0x%08X" ), funcName, sceErr );
		return false;
	}

	return true;
}

#else
#	define ORBIS_SYS_CALL( func ) func
#	define ORBIS_SYS_CALL_RET( func ) if( ( func ) < SCE_OK ) return false
#endif

#include <user_service/user_service_defs.h>

// Common type of callback
typedef std::function< void( SceUserServiceUserId ) > TOrbisUserEvent;

#endif // _ORBIS_API_CALLS_H_
