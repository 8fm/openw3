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

#include "ObjectProxyLocal.h"
#include "ILayerProxy.h"


class LayerProxyLocal
	: public ObjectProxyLocal
	, virtual public ILayerProxy
{
public:

	LayerProxyLocal( AkUniqueID in_id );

	//
	// RTPC
	//

	virtual void SetRTPC(
		AkRtpcID in_RTPC_ID,
		AkRTPC_ParameterID in_ParamID,
		AkUniqueID in_RTPCCurveID,
		AkCurveScaling in_eScaling,
		AkRTPCGraphPoint* in_pArrayConversion = NULL,
		AkUInt32 in_ulConversionArraySize = 0
	);

	virtual void UnsetRTPC(
		AkRTPC_ParameterID in_ParamID,
		AkUniqueID in_RTPCCurveID
	);

	//
	// Child Associations
	//

	virtual void SetChildAssoc(
		AkUniqueID in_ChildID,
		AkRTPCGraphPoint* in_pCrossfadingCurve,	// NULL if none
		AkUInt32 in_ulCrossfadingCurveSize		// 0 if none
	);

	virtual void UnsetChildAssoc(
		AkUniqueID in_ChildID 
	);

	virtual void SetCrossfadingRTPC(
		AkRtpcID in_rtpcID
	);
};
#endif // #ifndef AK_OPTIMIZED
