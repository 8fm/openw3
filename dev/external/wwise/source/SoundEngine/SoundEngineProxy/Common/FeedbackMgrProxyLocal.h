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
#ifndef AK_OPTIMIZED

#pragma once

#include "ObjectProxyLocal.h"
#include "IFeedbackMgrProxy.h"

class FeedbackMgrProxyLocal : public IFeedbackMgrProxy
{
public:
	FeedbackMgrProxyLocal();
	~FeedbackMgrProxyLocal();

	virtual void Enable( AkInt16 in_usCompany, AkInt16 in_usPlugin, AkCreatePluginCallback in_fnInit, bool in_bEnable );
};
#endif // #ifndef AK_OPTIMIZED
