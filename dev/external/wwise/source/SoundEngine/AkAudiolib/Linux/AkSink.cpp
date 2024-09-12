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
// AkSink.cpp
//
// Platform dependent part
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "AkSink.h"

#include "AkSinkALSA.h"
#include "AkSinkPulseAudio.h"

extern AkPlatformInitSettings g_PDSettings;

AkAudioAPI CAkSink::s_CurrentAudioAPI = AkAPI_Default;


AK::IAkPlugin* AkCreateDefaultSink(AK::IAkPluginMemAlloc * in_pAllocator)
{
	return AK_PLUGIN_NEW(in_pAllocator, CAkSink());
}

CAkSink::CAkSink()
	: m_pRealSink(NULL)
{
}

CAkSink::~CAkSink()
{
}

AKRESULT CAkSink::Init(
	AK::IAkPluginMemAlloc* in_pAllocator,
	AK::IAkSinkPluginContext* in_pSinkPluginContext,
	AK::IAkPluginParam* in_pParams,
	AkAudioFormat& io_rFormat)
{
	AKRESULT eResult = AK_Success;

	AkAudioAPI eAPIType = (AkAudioAPI)(g_PDSettings.eAudioAPI & AkAPI_Default);
	if (eAPIType == 0 || eAPIType == AkAPI_Default)
	{
		eAPIType = s_CurrentAudioAPI; // Restore previously successfully used API, could be AkAPI_Default if still undetermined.
	}

	m_pRealSink = NULL;

	AK::IAkSinkPlugin* pLinuxSink = NULL;
	IDynamicAPI* pDynamicAPI = NULL;

	if ((eAPIType & AkAPI_PulseAudio) != 0)
	{
		CAkPulseAudioSink* sink = AK_PLUGIN_NEW(in_pAllocator, CAkPulseAudioSink());
		pLinuxSink = sink;
		pDynamicAPI = sink;
	}
	else if ((eAPIType & AkAPI_ALSA) != 0)
	{
		CAkAlsaSink* sink = AK_PLUGIN_NEW(in_pAllocator, CAkAlsaSink());
		pLinuxSink = sink;
		pDynamicAPI = sink;
	}

	if (pLinuxSink == NULL)
	{
		return AK_InsufficientMemory;
	}

	eResult = pDynamicAPI->DynamicLoadAPI();
	if (eResult == AK_Success)
	{
		eResult = pLinuxSink->Init(in_pAllocator, in_pSinkPluginContext, NULL, io_rFormat);
		if (eResult == AK_Success)
		{
			m_pRealSink = pLinuxSink;
			s_CurrentAudioAPI = eAPIType; // Use this directly for other devices / next time
		}
		else
		{
			pLinuxSink->Term(in_pAllocator); // Term() is only called for m_pRealSink
		}
	}

	return m_pRealSink != NULL ? AK_Success : eResult;
}

AKRESULT CAkSink::Term(AK::IAkPluginMemAlloc* in_pAllocator)
{
	if (m_pRealSink)
	{
		m_pRealSink->Term(in_pAllocator); // Plugin self deletes in Term()
		m_pRealSink = NULL;
	}
	AK_PLUGIN_DELETE(in_pAllocator, this);

	// Technically we could dlclose() here the lib opened in CAkLinuxSink::DynamicLoadAPI,
	// but considering that default sink might later be reinit,
	// there are only performance disadvantages in doing so
	return AK_Success;
}

AKRESULT CAkSink::Reset()
{
	return m_pRealSink->Reset();
}

AKRESULT CAkSink::GetPluginInfo(AkPluginInfo & out_rPluginInfo)
{
	return m_pRealSink->GetPluginInfo(out_rPluginInfo);
}

void CAkSink::Consume(AkAudioBuffer* in_pInputBuffer, AkRamp in_gain)
{
	return m_pRealSink->Consume(in_pInputBuffer, in_gain);
}

void CAkSink::OnFrameEnd()
{
	return m_pRealSink->OnFrameEnd();
}

bool CAkSink::IsStarved()
{
	return m_pRealSink->IsStarved();
}

void CAkSink::ResetStarved()
{
	return m_pRealSink->ResetStarved();
}

AKRESULT CAkSink::IsDataNeeded(AkUInt32& out_uBuffersNeeded)
{
	return m_pRealSink->IsDataNeeded(out_uBuffersNeeded);
}
