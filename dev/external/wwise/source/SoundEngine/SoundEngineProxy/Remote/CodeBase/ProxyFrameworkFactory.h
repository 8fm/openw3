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
#pragma once

#ifndef AK_OPTIMIZED

/// \file 
/// Proxy Framework Factory.
/// \sa
/// - \ref initialization_comm
/// - \ref framerendering_comm
/// - \ref termination_comm

#include <AK/SoundEngine/Common/AkTypes.h>

namespace AK
{
	namespace Comm
	{
		class IProxyFrameworkConnected;
	}
}
/// Create an instance of the proxy framework.
/// \return Pointer to proxy framework interface.
/// - \ref initialization_comm
/// - \ref framerendering_comm
/// - \ref termination_comm
/// - AK::Comm::DEFAULT_MEMORY_POOL_ATTRIBUTES
AK::Comm::IProxyFrameworkConnected* AkCreateProxyFramework( AkMemPoolId in_pool );

#endif // #ifndef AK_OPTIMIZED
