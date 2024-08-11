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

#include <AK/SoundEngine/Common/AkTypes.h>
#include <AK/SoundEngine/Common/IAkPlugin.h>
#include "AkConversionTable.h"
#include "AkIndexable.h"
#include "AkKeyArray.h"
#include "AkRTPC.h"

class CAkRegisteredObj;

typedef CAkKeyArray<AkUInt32/*index*/, AkUniqueID/*media id*/> AkMediaMap;

class CAkFxBase
	: public CAkIndexable
{
public:
	// CAkFxBase
	virtual bool IsShareSet() = 0;

	AKRESULT SetInitialValues( AkUInt8* in_pData, AkUInt32 in_uDataSize );

	void SetFX( 
		AkPluginID in_FXID,
		AK::IAkPluginParam * in_pParam
		);

	AKRESULT SetFX( 
		AkPluginID in_FXID,
		void * in_pParamBlock,
		AkUInt32 in_uParamBlockSize
		);

	AK::IAkPluginParam * GetFXParam() { return m_pParam; }
	void SetFXParam( 
		AkPluginParamID in_uParamID,	// ID of the param to modify, will be done by the plug-in itself
		void * in_pParamBlock,			// Pointer to the Param block
		AkUInt32 in_uParamBlockSize		// BLOB size
		);

	void SetRTPC( 
		AkRtpcID			in_RTPC_ID,
		AkRTPC_ParameterID	in_ParamID,
		AkUniqueID			in_RTPCCurveID,
		AkCurveScaling		in_eScaling,
		AkRTPCGraphPoint*	in_pArrayConversion = NULL,
		AkUInt32			in_ulConversionArraySize = 0,
		bool				in_bNotify = true
		);

	void UnsetRTPC( 
		AkRTPC_ParameterID in_ParamID,
		AkUniqueID in_RTPCCurveID,
		bool				in_bNotify = true
		);

	AkPluginID GetFXID() { return m_FXID; }

	AkUniqueID GetMediaID( AkUInt32 in_uIdx ) { AkUniqueID * pID = m_media.Exists( in_uIdx ); return pID ? *pID : AK_INVALID_UNIQUE_ID; }
	void SetMediaID( AkUInt32 in_uIdx, AkUniqueID in_mediaID );

	void SubscribeRTPC( AK::IAkRTPCSubscriber* in_pSubscriber, CAkRegisteredObj * in_pGameObj = NULL );
	void UnsubscribeRTPC( AK::IAkRTPCSubscriber* in_pSubscriber );

protected:
	
	struct RTPCSubs
	{
		AkRtpcID			RTPCID;
		AkUniqueID			RTPCCurveID;
		AkRTPC_ParameterID	ParamID;		//# of the param that must be notified on change
		CAkConversionTable<AkRTPCGraphPoint, AkReal32>	ConversionTable;
	};
	typedef AkArray<RTPCSubs, const RTPCSubs&, ArrayPoolDefault, 2> RTPCSubsArray;

	AkPluginID				m_FXID;
	AK::IAkPluginParam *	m_pParam;
	AkMediaMap				m_media;
	RTPCSubsArray			m_rtpcsubs;

protected:
	CAkFxBase( AkUniqueID in_ulID );
	virtual ~CAkFxBase();
};

class CAkFxShareSet
	: public CAkFxBase
{
public:
	static CAkFxShareSet* Create( AkUniqueID in_ulID = 0 );

	// CAkCommonBase
	virtual AkUInt32 AddRef();
	virtual AkUInt32 Release();

	// CAkFxBase
	virtual bool IsShareSet() { return true; }

protected:
	CAkFxShareSet( AkUniqueID in_ulID );
};

class CAkFxCustom
	: public CAkFxBase
{
public:
	static CAkFxCustom* Create( AkUniqueID in_ulID = 0 );

	// CAkCommonBase
	virtual AkUInt32 AddRef();
	virtual AkUInt32 Release();

	// CAkFxBase
	virtual bool IsShareSet() { return false; }

protected:
	CAkFxCustom( AkUniqueID in_ulID );
};
