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

#include "ParameterNodeProxyConnected.h"

class LayerContainerProxyConnected
	: public ParameterNodeProxyConnected
{
public:

	LayerContainerProxyConnected( AkUniqueID in_id );

	virtual void HandleExecute( AkUInt16 in_uMethodID, CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer );

private:
	DECLARE_BASECLASS( ParameterNodeProxyConnected );
};

#endif // #ifndef AK_OPTIMIZED
