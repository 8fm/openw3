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

#include "stdafx.h"
#ifndef AK_OPTIMIZED



#include <AK/Tools/Common/AkObject.h>
#include "ALBytesMem.h"

#include "ProxyFrameworkFactory.h"

#include "ProxyFrameworkConnected.h"

AK::Comm::IProxyFrameworkConnected* AkCreateProxyFramework( AkMemPoolId in_pool )
{
	AK::ALWriteBytesMem::SetMemPool( in_pool );
	return AkNew( in_pool, ProxyFrameworkConnected( in_pool ) );
}
#endif // #ifndef AK_OPTIMIZED
