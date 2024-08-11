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

#include "stdafx.h"
#include "AkACPManager.h"
#include "AkSrcBankAt9.h"
#include "AkSrcFileAt9.h"
#include "ajm/at9_decoder.h"
#include "AkAudioLibTimer.h"
#include "AkAudioMgr.h"


CAkACPManager::CAkACPManager():
	m_AjmContextId(NULL)
{	
}

CAkACPManager::~CAkACPManager()
{	
}

AKRESULT CAkACPManager::Init()
{
	// Intialise the audio job manager
	AKVERIFY( sceAjmInitialize(0, &m_AjmContextId) == SCE_OK );

	// Load the atrac9 decoder
	AKVERIFY( sceAjmModuleRegister(m_AjmContextId, SCE_AJM_CODEC_AT9_DEC, 0) == SCE_OK );

	if (g_PDSettings.uLEngineAcpBatchBufferSize > 0)
	{
		m_pAcpBatchBuffer = (uint8_t*)AkAlloc( g_LEngineDefaultPoolId, g_PDSettings.uLEngineAcpBatchBufferSize ); 
		if( m_pAcpBatchBuffer == NULL )
		{	
			AKASSERT(!"Failed allocating m_pAcpBatchBuffer in CAkACPManager");
			return AK_Fail;
		}
	}

	return AK_Success;
}

void CAkACPManager::Term()
{
	if (m_pAcpBatchBuffer != NULL)
	{
		AkFree( g_LEngineDefaultPoolId, m_pAcpBatchBuffer );
	}

	AKVERIFY( sceAjmModuleUnregister(m_AjmContextId, SCE_AJM_CODEC_AT9_DEC) == SCE_OK );
	AKVERIFY( sceAjmFinalize(m_AjmContextId) == SCE_OK );
}

AKRESULT CAkACPManager::Register(CAkSrcBankAt9* pACPSrc)
{
	if ( pACPSrc->CreateHardwareInstance(m_AjmContextId) == AK_Success)
	{
		m_ACPBankSrc.AddFirst( pACPSrc );
		m_SoundsToInitialize++;
		return AK_Success;
	}
	
	return AK_Fail;
}

AKRESULT CAkACPManager::Register(CAkSrcFileAt9* pACPSrc)
{
	if ( pACPSrc->CreateHardwareInstance(m_AjmContextId) == AK_Success)
	{
		m_ACPFileSrc.AddFirst( pACPSrc );
		m_SoundsToInitialize++;
		return AK_Success;
	}
		
	return AK_Fail;
}

AKRESULT CAkACPManager::Unregister(CAkSrcBankAt9* pACPSrc)
{
	m_ACPBankSrc.Remove( pACPSrc );
	return AK_Success;
}

AKRESULT CAkACPManager::Unregister(CAkSrcFileAt9* pACPSrc)
{
	m_ACPFileSrc.Remove( pACPSrc );
	return AK_Success;
}

#ifdef AK_PS4_HW_DECODER_METRICS
static AkInt64 gDecodingFrame = 0;
#endif

AKRESULT CAkACPManager::Update()
{
#ifndef AK_OPTIMIZED
	AK_START_TIMER_DSP();
#endif
	SceAjmBatchId batchId;
	SceAjmBatchError ajmBatchError;
	SceAjmBatchError ajmBatchError2;
	SceAjmBatchInfo batchInfo;

	bool decodingJobFound = !m_ACPBankSrc.IsEmpty() || !m_ACPFileSrc.IsEmpty();

#ifdef AT9_STREAM_DEBUG_OUTPUT
	char msg[256];
	sprintf( msg, "-- -- -- -- -- -- -- -- FRAME UPDATE \n" );
	AKPLATFORM::OutputDebugMsg( msg );
#endif

	while (decodingJobFound)
	{
#ifdef AT9_STREAM_DEBUG_OUTPUT
		sprintf( msg, "-- -- -- -- -- -- -- -- FRAME UPDATE -- -- -- -- LOOP START \n" );
		AKPLATFORM::OutputDebugMsg( msg );
#endif
		decodingJobFound = false;
		sceAjmBatchInitialize(m_pAcpBatchBuffer, g_PDSettings.uLEngineAcpBatchBufferSize, &batchInfo);

		for ( auto it = m_ACPBankSrc.Begin(); it != m_ACPBankSrc.End(); ++it )
		{
			CAkSrcBankAt9* pACPSrc = (*it);

			if (pACPSrc->DataNeeded())
			{
				if ( pACPSrc->InitIfNeeded(&batchInfo) )
				{
					m_SoundsToInitialize--;
				}

				if ( pACPSrc->CreateDecodingJob(&batchInfo) == AK_Success )
				{
					decodingJobFound = true;
				}
			}
		}

		for ( auto it = m_ACPFileSrc.Begin(); it != m_ACPFileSrc.End(); ++it )
		{
			CAkSrcFileAt9* pACPSrc = (*it);

			if (pACPSrc->DataNeeded())
			{
				pACPSrc->UpdateStreamBuffer(); // fill stream buffer if needed
				if (pACPSrc->IsDataReady())
				{
					if ( pACPSrc->InitIfNeeded(&batchInfo) )
					{
						m_SoundsToInitialize--;
					}

					if ( pACPSrc->CreateDecodingJob(&batchInfo) == AK_Success )
					{
						decodingJobFound = true;
					}
				}
			}
		}

		if (decodingJobFound)
		{
			// Run all jobs
			SceAjmBatchError ajmBatchError;
			int result = sceAjmBatchStart(m_AjmContextId, &batchInfo, SCE_AJM_PRIORITY_GAME_DEFAULT, &ajmBatchError, &batchId);
			if (result != SCE_OK) //todo
			{
				char msg[200]; 
				sprintf( msg, "DECODING ERROR!!\n" );
				AKPLATFORM::OutputDebugMsg( msg );

				sceAjmBatchErrorDump(&batchInfo, &ajmBatchError);
			}

			result = sceAjmBatchWait(m_AjmContextId, batchId, SCE_AJM_WAIT_INFINITE, &ajmBatchError);
			if (result != SCE_OK)
			{
				sceAjmBatchErrorDump(&batchInfo, &ajmBatchError);
			}

			// Update 
			for ( auto it = m_ACPBankSrc.Begin(); it != m_ACPBankSrc.End(); ++it )
			{
				CAkSrcBankAt9* pACPSrc = (*it);
				if (pACPSrc->DecodingStarted())
				{
					pACPSrc->DecodingEnded();
				}
			}

			for ( auto it = m_ACPFileSrc.Begin(); it != m_ACPFileSrc.End(); ++it )
			{
				CAkSrcFileAt9* pACPSrc = (*it);
				if (pACPSrc->DecodingStarted())
				{
					if (!pACPSrc->DecodingEnded())
					{
						sceAjmBatchErrorDump(&batchInfo, &ajmBatchError);
					}
				}
			}
		}
	}

#ifndef AK_OPTIMIZED
	AK_STOP_TIMER_DSP();
#endif
	
	return AK_Success;
}