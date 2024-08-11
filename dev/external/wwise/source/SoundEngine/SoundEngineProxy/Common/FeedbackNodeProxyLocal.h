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
#ifndef PROXYCENTRAL_CONNECTED

#include "ParameterNodeProxyLocal.h"
#include "IFeedbackNodeProxy.h"

class FeedbackNodeProxyLocal
	: public ParameterNodeProxyLocal
	, virtual public IFeedbackNodeProxy
{
public:
	FeedbackNodeProxyLocal( AkUniqueID in_id );
	virtual ~FeedbackNodeProxyLocal();	

	virtual void AddPluginSource( 
		AkUniqueID	in_srcID,
		AkUInt16 in_idDeviceCompany, AkUInt16 in_idDevicePlugin
		);

	virtual void SetSourceVolumeOffset(AkUniqueID in_srcID, AkReal32 in_fOffset);

	virtual void RemoveAllSources();
};

#endif
#endif // #ifndef AK_OPTIMIZED
