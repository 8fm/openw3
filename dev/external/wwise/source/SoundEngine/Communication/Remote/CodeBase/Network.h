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
	Common file used by the communication for general network related functionalities.
	Location: \Wwise\Communication\Remote\CodeBase
	Implementation location: In the platform folders: Wwise\Communication\Remote\{Platform}
	
	See more information in the implementation file.
	-----------------------------------------------------------------------------------------
*/

#pragma once

#include "AkPrivateTypes.h"

namespace Network
{
	AKRESULT Init( AkMemPoolId in_pool, bool in_bInitSystemLib );
	void Term(bool in_bTermSystemLib);
	AkInt32 GetLastError();

	void GetMachineName( char* out_pMachineName, AkInt32* io_pStringSize );

	bool SameEndianAsNetwork();
}
