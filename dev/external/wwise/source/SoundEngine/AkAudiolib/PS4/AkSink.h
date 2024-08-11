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
// AkSink.h
//
// Platform dependent part
//
//////////////////////////////////////////////////////////////////////
#pragma once

#include "AkParameters.h"
#include "AkCommon.h"
#include "PlatformAudiolibDefs.h" // FT
#include "AkLEngineStructs.h"
#include "AkFileParserBase.h"
#include "AkCaptureMgr.h"
#include <stdint.h>
#include <audioout.h>

namespace AK
{
	class IAkStdStream;
};

//----------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------

#define AK_NUM_VOICE_BUFFERS 		(2)
#define AKSINK_MAXCAPTURENAMELENGTH				(128)
#define	AK_PRIMARY_BUFFER_SAMPLE_TYPE			AkReal32 //AkReal32 //AkInt16
#define	AK_PRIMARY_BUFFER_BITS_PER_SAMPLE		(8 * sizeof(AK_PRIMARY_BUFFER_SAMPLE_TYPE))


//----------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------

class CAkSink
	: public CAkObject
{
public:
	static CAkSink * Create( AkOutputSettings & in_settings, AkSinkType in_eType, AkUInt32 in_uInstance );

	virtual AKRESULT Init(bool in_bIsOutputRecordable) = 0;
	virtual void Term() = 0;
	AKRESULT Play() {return AK_Success;}

	AKRESULT Start(){return AK_Success;}

	AkForceInline AkChannelMask GetSpeakerConfig() { return m_SpeakersConfig; }
	
	// Hack for PS4 channel configs.
	virtual bool Force71() = 0;

	AkPipelineBufferBase& NextWriteBuffer()
	{
		m_MasterOut.AttachInterleavedData( 
			GetRefillPosition(), 
			AK_NUM_VOICE_REFILL_FRAMES, 
			0, 
			m_MasterOut.GetChannelMask());
		return m_MasterOut;
	};
	virtual AkForceInline void* GetRefillPosition() = 0;

	virtual bool IsStarved() = 0;
	virtual void ResetStarved() = 0;
	virtual AKRESULT IsDataNeeded( AkUInt32 & out_uBuffersNeeded ) = 0;

	virtual AKRESULT PassData() = 0;
	virtual AKRESULT PassSilence() = 0;

	virtual AKRESULT FillOutputParam(SceAudioOutOutputParam& out_rParam){return AK_Fail;}

	void StartOutputCapture(const AkOSChar* in_CaptureFileName);
	void StopOutputCapture();
#ifndef AK_OPTIMIZED
	AkSinkStats m_stats;
#endif

protected:
	CAkSink(AkChannelMask uChannelMask);

#ifndef AK_OPTIMIZED
	void UpdateProfileData( AkReal32 * in_pfSamples, AkUInt32 in_uNumSamples );
	void UpdateProfileData( AkInt16 * in_psSamples, AkUInt32 in_uNumSamples );
	void UpdateProfileSilence( AkUInt32 in_uNumSamples );
#endif // AK_OPTIMIZED
	AkCaptureFile*			m_pCapture;

	AkChannelMask m_SpeakersConfig;				// speakers config
	AkUInt32 m_ulNumChannels;
	AkUInt32 m_ulRefillSamples;
	AkUInt32 m_unBufferSize;							// Size of one buffer
	AkPipelineBufferBase m_MasterOut;
};

//====================================================================================================
//====================================================================================================

class CAkSinkPS4
	: public CAkSink
{
public:

	CAkSinkPS4(AkChannelMask uChannelMask, AkUInt32 in_uType, AkUInt32 in_userId);

	virtual AKRESULT Init( bool in_bIsOutputRecordable );
	virtual void Term();

	// Hack for PS4 channel configs.
	virtual bool Force71() { return m_bForce71; }

	virtual bool IsStarved();
	virtual void ResetStarved();

	virtual AKRESULT IsDataNeeded( AkUInt32 & out_uBuffersNeeded );

	virtual AKRESULT PassData();
	virtual AKRESULT PassSilence();	

	virtual AKRESULT FillOutputParam(SceAudioOutOutputParam& out_rParam);

	// Helpers.
	virtual void* GetRefillPosition() { return m_ppvRingBuffer[m_uWriteBufferIndex]; }
	void * GetAndAdvanceReadPointer();
	void AdvanceWritePointer();
	
	CAkSinkPS4 *			pNextItem;	// List bare sibling.

protected:

	void _Term();	// Internal/Deferred termination.

	int						m_port;									// sceAudioOut main port
	AkUInt32				m_uType;								// Port type
	AkUInt32				m_uUserId;								// User Id.
    
	CAkLock					m_lockRingBuffer;						// Ring buffer lock
	void *					m_pvAudioBuffer;						// Ring buffer memory
	void *					m_ppvRingBuffer[2];						// Double buffer
	AkUInt8					m_uReadBufferIndex;						// Read index		
	AkUInt8					m_uWriteBufferIndex;					// Write index		
	bool					m_bEmpty;								// Double buffer empty
	bool					m_bStarved;								// Starvation flag.

	bool 					m_ManagedAudioOutInit;
	bool					m_bPendingPortClose;

	// Hack for PS4 channel configs.
	bool					m_bForce71;

	// Common audio output thread management.
	static AK_DECLARE_THREAD_ROUTINE(AudioOutThreadFunc);
	static AKRESULT RegisterSink( CAkSinkPS4 * in_pSink );
	static bool UnregisterSink( CAkSinkPS4 * in_pSink, bool & out_bIsLast );	// returns true if sink was found and unregistered, false otherwise.

	struct LibAudioPort
	{
		CAkSinkPS4* pSink;
		void* pRingBuffer;
		int port;
		bool bPendingClose;
	};

	typedef AkArray<LibAudioPort, LibAudioPort&, ArrayPoolLEngineDefault> LibAudioSinks;
	static LibAudioSinks	s_listSinks;
	static CAkLock			s_lockSinks;
	static AkThread			s_hAudioOutThread;						// Thread for all libAudio sinks.
	static AkUInt32			s_uSinkCount;
};


#ifndef AK_OPTIMIZED
class CAkSinkDummy : public CAkSink
{
public:

	CAkSinkDummy(AkChannelMask uChannelMask);

	virtual AKRESULT Init( bool in_bIsOutputRecordable );
	virtual void Term();

	// Hack for PS4 channel configs.
	virtual bool Force71() { return false; }

	virtual bool IsStarved() { return false; }
	virtual void ResetStarved() {}

	virtual AKRESULT IsDataNeeded( AkUInt32 & out_uBuffersNeeded );

	virtual AKRESULT PassData();
	virtual AKRESULT PassSilence();

	virtual AkForceInline void* GetRefillPosition(){ return m_pCaptureBuffer; }
private:
	AK_PRIMARY_BUFFER_SAMPLE_TYPE*	m_pCaptureBuffer;
};
#endif


extern CAkSink* g_pAkSink;
