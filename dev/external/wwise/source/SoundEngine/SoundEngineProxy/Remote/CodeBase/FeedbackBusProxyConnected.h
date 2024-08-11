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

#include "BaseBusConnected.h"

class FeedbackBusProxyConnected : public BaseBusConnected
{
public:
	FeedbackBusProxyConnected( AkUniqueID in_id );
	virtual ~FeedbackBusProxyConnected();

private:
	DECLARE_BASECLASS( BaseBusConnected );
};
#endif // #ifndef AK_OPTIMIZED
