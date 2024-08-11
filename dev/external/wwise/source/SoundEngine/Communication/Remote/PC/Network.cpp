/***********************************************************************
  The content of this file includes source code for the sound engine
  portion of the AUDIOKINETIC Wwise Technology and constitutes "Level
  Two Source Code" as defined in the Source Code Addendum attached
  with this file.  Any use of the Level Two Source Code shall be
  subject to the terms and conditions outlined in the Source Code
  Addendum and the End User License Agreement for Wwise(R).

  Version: v2013.2.9  Build: 4872
  Copyright (c) 2006-2014 Audiokinetic Inc.
 ***********************************************************************/


/*	----------------------------------------------------------------------------------------
	PC (Windows) implementation.
	Location: \Wwise\Communication\Remote\PC
	Header location: \Wwise\Communication\Remote\CodeBase
	-----------------------------------------------------------------------------------------
*/


#include "stdafx.h"

#include "Network.h"

namespace Network
{
	AKRESULT Init( AkMemPoolId in_pool, bool in_bInitSystemLib )
	{
		if (in_bInitSystemLib)
		{
			WSAData wsaData = { 0 };
			return ::WSAStartup( MAKEWORD( 2, 2 ), &wsaData ) == 0 ? AK_Success : AK_Fail;
		}

		return AK_PartialSuccess;
	}

	void Term(bool in_bTermSystemLib)
	{
		if (in_bTermSystemLib)
			::WSACleanup();
	}

	AkInt32 GetLastError()
	{
		return ::WSAGetLastError();
	}

	void GetMachineName( char* out_pMachineName, AkInt32* io_pStringSize )
	{
#ifndef AK_USE_METRO_API
		::GetComputerNameA( out_pMachineName, (DWORD*)io_pStringSize );
#else
		static char* pszUnnamed = { "Unnamed" };

		strncpy( out_pMachineName, pszUnnamed, *io_pStringSize );
		*io_pStringSize = (AkInt32) strlen( pszUnnamed );
#endif
	}

	bool SameEndianAsNetwork()
	{
		return ::htons( 12345 ) == 12345;
	}
}