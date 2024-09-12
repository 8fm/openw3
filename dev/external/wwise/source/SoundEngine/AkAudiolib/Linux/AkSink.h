/***********************************************************************
  The content of this file includes source code for the sound engine
  portion of the AUDIOKINETIC Wwise Technology and constitutes "Level
  Two Source Code" as defined in the Source Code Addendum attached
  with this file.  Any use of the Level Two Source Code shall be
  subject to the terms and conditions outlined in the Source Code
  Addendum and the End User License Agreement for Wwise(R).

  Version:  Build: 
  Copyright (c) 2006-2020 Audiokinetic Inc.
 ***********************************************************************/

//////////////////////////////////////////////////////////////////////
//
// AkSink.h
//
// Platform dependent part
//
//////////////////////////////////////////////////////////////////////
#pragma once

#include <AK/SoundEngine/Common/AkSoundEngine.h>

///
/// The role of CAkSink is to initialize the Audio API
/// specified by AkAudioAPI in the AkPlatformInitSettings
/// and forward requests to it.
///
class CAkSink : public AK::IAkSinkPlugin	
{
public:
	CAkSink();
	~CAkSink();

	//
	// AK::IAkSinkPlugin implementation.
	//
	virtual AKRESULT Init(
		AK::IAkPluginMemAlloc* in_pAllocator,
		AK::IAkSinkPluginContext* in_pSinkPluginContext,
		AK::IAkPluginParam* in_pParams,
		AkAudioFormat& io_rFormat
	);

	virtual AKRESULT Term(
		AK::IAkPluginMemAlloc* in_pAllocator
	);
	
	virtual AKRESULT Reset();

	virtual AKRESULT GetPluginInfo(AkPluginInfo& out_rPluginInfo);
	
	virtual void Consume(
		AkAudioBuffer* in_pInputBuffer,
		AkRamp in_gain
	);
	
	virtual void OnFrameEnd();

	virtual bool IsStarved();
	
	virtual void ResetStarved();
	
	virtual AKRESULT IsDataNeeded(AkUInt32 & out_uBuffersNeeded);
	
	// Keep the currently used API so all sinks are of the same type.
	static AkAudioAPI s_CurrentAudioAPI;

private:

	AK::IAkSinkPlugin* m_pRealSink;	
};
