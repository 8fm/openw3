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

//////////////////////////////////////////////////////////////////////
//
// AkFeedbackNode.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _FEEDBACK_NODE_H_
#define _FEEDBACK_NODE_H_

#include "AkMonitorData.h"
#include "AkSoundBase.h"
#include "AkParameters.h"
#include "AkBankMgr.h"
#include "AkCommon.h"
#include "AkSource.h"

// class corresponding to a Sound
//
// Author:  mjean
class CAkFeedbackNode : public CAkSoundBase
{
public:
	//Thread safe version of the constructor
	static CAkFeedbackNode* Create(AkUniqueID in_ulID = 0);

	virtual AkNodeCategory NodeCategory();

	// Call a play on the definition directly
    //
    // Return - AKRESULT - Ak_Success if succeeded
	virtual AKRESULT PlayInternal( AkPBIParams& in_rPBIParams );

	virtual AKRESULT ExecuteAction( ActionParams& in_rAction );
	virtual AKRESULT ExecuteActionExcept( ActionParamsExcept& in_rAction );
	
	AKRESULT AddPluginSource( 
		AkUniqueID	in_srcID,
		AkUInt16 in_idDeviceCompany, AkUInt16 in_idDevicePlugin
		);

	void	 SetSourceVolumeOffset(AkUniqueID in_srcID, AkReal32 in_fOffset);
	AkReal32 GetSourceVolumeOffset(CAkSource *in_pSource);

	void RemoveSource( AkUniqueID in_srcID );

	inline bool HasBankSource() {return false;}

	void RemoveAllSources();

	virtual AkObjectCategory Category();

	virtual AKRESULT IncrementPlayCount( CounterParameters& io_params );

	//Bank stuff
	bool SourceLoaded(){ return !m_arSrcInfo.IsEmpty(); }
	virtual AKRESULT PrepareData();
	virtual void UnPrepareData();
	AKRESULT SetInitialValues( AkUInt8* pData, AkUInt32 ulDataSize, CAkUsageSlot* in_pUsageSlot, bool in_bIsPartialLoadOnly );

protected:
	// Constructors
    CAkFeedbackNode( AkUniqueID in_ulID );

	//Destructor
    virtual ~CAkFeedbackNode();

	AKRESULT Init(){ return CAkParameterNode::Init(); }

	AKRESULT GetFeedbackParameters( AkFeedbackParams &io_Params, CAkSource *in_pSource, CAkRegisteredObj * in_GameObjPtr, bool in_bDoBusCheck = true );
	virtual AKRESULT GetAudioParameters(AkSoundParamsEx &io_Parameters, AkUInt32 in_ulParamSelect, AkMutedMap& io_rMutedMap, CAkRegisteredObj * in_GameObjPtr, bool in_bIncludeRange, AkPBIModValues& io_Ranges, bool in_bDoBusCheck /*= true*/);

// members
private:
	//Additional data grafted to CAkSource
	class SrcInfo : public CAkSource
	{
	public:	
		AkUInt16	m_idDeviceCompany;
		AkUInt16	m_idDevicePlugin;
		AkReal32	m_fVolumeOffset;
	};
	typedef CAkKeyArray<AkUniqueID, SrcInfo*> SrcInfoArray;
	SrcInfoArray			m_arSrcInfo;
};
#endif

