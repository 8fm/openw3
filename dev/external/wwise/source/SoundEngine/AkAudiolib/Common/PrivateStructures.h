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
// PrivateStructures.h
//
// AkAudioLib Internal structures definitions
//
//////////////////////////////////////////////////////////////////////
#ifndef _PRIVATE_STRUCTURES_H_
#define _PRIVATE_STRUCTURES_H_

#include "AkContinuationList.h"
#include "AkSettings.h"
#include "AkAudioLib.h"
#include "AkMath.h"

class AkExternalSourceArray
{
public:

	static AkExternalSourceArray* Create(AkUInt32 in_nCount, AkExternalSourceInfo* in_pSrcs);

	void AddRef(){m_cRefCount++;}
	void Release();
	AkUInt32 Count(){return m_nCount;}
	const AkExternalSourceInfo* Sources(){return m_pSrcs;}

private:
	AkUInt32 m_cRefCount;
	AkUInt32 m_nCount;
	AkExternalSourceInfo m_pSrcs[1];	//Variable size array.
};

class UserParams
{
public:
	UserParams()
	{
		m_PlayingID = 0;
		m_CustomParam.customParam = 0;
		m_CustomParam.ui32Reserved = 0;
		m_CustomParam.pExternalSrcs = NULL;
	}

	UserParams(const UserParams& in_rCopy)
	{
		m_CustomParam.pExternalSrcs = NULL;
		*this = in_rCopy;
	}

	~UserParams()
	{
		if (m_CustomParam.pExternalSrcs != NULL)
			m_CustomParam.pExternalSrcs->Release();
	}

	UserParams& operator=(const UserParams& in_rCopy)
	{
		Init(in_rCopy.m_PlayingID, in_rCopy.m_CustomParam);
		return *this;
	}

	void Init(AkPlayingID in_playingID, const AkCustomParamType& in_params)
	{
		m_PlayingID = in_playingID;
		m_CustomParam.customParam = in_params.customParam;
		m_CustomParam.ui32Reserved = in_params.ui32Reserved;
		
		SetExternalSources(in_params.pExternalSrcs);
	}

	void SetExternalSources(AkExternalSourceArray* in_pSources)
	{
		if (m_CustomParam.pExternalSrcs)
			m_CustomParam.pExternalSrcs->Release();
		
		if (in_pSources)
			in_pSources->AddRef();

		m_CustomParam.pExternalSrcs = in_pSources;
	}

	void SetPlayingID(AkPlayingID in_playingID){ m_PlayingID = in_playingID; }

	const AkCustomParamType& CustomParam() const {return m_CustomParam;}
	AkPlayingID				PlayingID() const {return m_PlayingID;}

private:
	AkCustomParamType m_CustomParam;
	AkPlayingID	m_PlayingID;
};

//----------------------------------------------------------------------------------------------------
// parameters needed for play and play&continue actions
//----------------------------------------------------------------------------------------------------
class CAkTransition;
class CAkTransition;
class CAkPath;

// Synchronisation type. Applies to Source transitions as well as to State changes.
enum AkSyncType
{
    SyncTypeImmediate		= 0,
    SyncTypeNextGrid		= 1,
    SyncTypeNextBar			= 2,
    SyncTypeNextBeat		= 3,
    SyncTypeNextMarker		= 4,
    SyncTypeNextUserMarker	= 5,
    SyncTypeEntryMarker		= 6,    // not used with SrcTransRules.
    SyncTypeExitMarker		= 7

#define NUM_BITS_SYNC_TYPE  (5)
};

enum AkPlaybackState
{
	PB_Playing,
		PB_Paused
};

enum ActionParamType
{
	ActionParamType_Stop	= 0,
	ActionParamType_Pause	= 1,
	ActionParamType_Resume	= 2,
	ActionParamType_Break	= 3, // == PlayToEnd
	ActionParamType_Seek	= 4
};

struct AkPathInfo
{
	CAkPath*	pPBPath;
	AkUniqueID	PathOwnerID;
};

struct ContParams
{
	// Default constructor
	ContParams()
	{
		// doesn't init a thing by default
	}

	// Copy constructor, copy everything BUT THE spContList
	ContParams( ContParams* in_pFrom )
		: pPlayStopTransition( in_pFrom->pPlayStopTransition ) 
		, pPauseResumeTransition( in_pFrom->pPauseResumeTransition )
		, pPathInfo( in_pFrom->pPathInfo )
		, bIsPlayStopTransitionFading( in_pFrom->bIsPlayStopTransitionFading )
		, bIsPauseResumeTransitionFading( in_pFrom->bIsPauseResumeTransitionFading )
		, ulPauseCount( in_pFrom->ulPauseCount )
	{
		// DO NOT COPY spContList object.
	}
	
	ContParams( AkPathInfo* in_pPathInfo )
		: pPlayStopTransition( NULL ) 
		, pPauseResumeTransition( NULL )
		, pPathInfo( in_pPathInfo )
		, bIsPlayStopTransitionFading( false )
		, bIsPauseResumeTransitionFading( false )
		, ulPauseCount( 0 )
	{
	}

	CAkTransition*						pPlayStopTransition;		// the running play / stop transition
	CAkTransition*						pPauseResumeTransition;		// the running pause / resume transition
	AkPathInfo*							pPathInfo;					// the current path if any
	bool								bIsPlayStopTransitionFading;
	bool								bIsPauseResumeTransitionFading;
	CAkSmartPtr<CAkContinuationList>	spContList;
	AkUInt32							ulPauseCount;
};

struct TransParams
{
	AkTimeMs				TransitionTime;				// how long this should take
	AkCurveInterpolation	eFadeCurve;					// what shape it should have
};

struct PlaybackTransition
{
	CAkTransition*		pvPSTrans;	// Play Stop transition reference.
	CAkTransition*		pvPRTrans;	// Pause Resume transition reference.
	AkUInt8				bIsPSTransFading : 1;
	AkUInt8				bIsPRTransFading : 1;
	PlaybackTransition()
		:pvPSTrans( NULL )
		,pvPRTrans( NULL )
		,bIsPSTransFading( false )
		,bIsPRTransFading( false )
	{}
};

struct AkMusicGrid
{
	AkMusicGrid() 
		: uBeatDuration( 0 )
		, uBarDuration( 0 )
		, uGridDuration( 0 )
		, uGridOffset( 0 ) {}
    AkUInt32    uBeatDuration;      // Beat duration in samples. NOTE (WG-4233) Complex time signatures might define non-constant beat durations.
    AkUInt32    uBarDuration;       // Bar duration in samples.
    AkUInt32    uGridDuration;      // Grid duration in samples.
    AkUInt32    uGridOffset;        // Grid offset in samples.
};

// Time conversion helpers.
// ---------------------------------------------------------
namespace AkTimeConv
{
	static inline AkInt32 MillisecondsToSamples( 
        AkReal64 in_milliseconds 
        )
    {
		return AkMath::Round64( in_milliseconds * (AkReal64)AK_CORE_SAMPLERATE / 1000.0 );
    }
	static inline AkInt32 MillisecondsToSamples( 
        AkTimeMs in_milliseconds 
        )
    {
		// Platforms with core sample rates that are a multiple of 1000 may use integer calculation
		// Platforms that are with variable rates should not use integer calculations.
#if defined(AK_APPLE) || defined(AK_3DS) || defined AK_ANDROID
		return (AkInt32)( (AkInt64)in_milliseconds * AK_CORE_SAMPLERATE / 1000 );
#else
		AKASSERT( (AK_CORE_SAMPLERATE / 1000 * 1000) == AK_CORE_SAMPLERATE 
				|| !"Cannot use this path with this platform/sample rate" );
		return in_milliseconds * (AK_CORE_SAMPLERATE / 1000);
#endif
    }
    static inline AkInt32 SecondsToSamples( 
        AkReal64 in_seconds 
        )
    {
        return AkMath::Round64( in_seconds * (AkReal64)AK_CORE_SAMPLERATE );
    }
    static inline AkTimeMs SamplesToMilliseconds( 
        AkInt32 in_samples
        )
    {
		return (AkTimeMs)AkMath::Round64( 1000 * (AkReal64)in_samples / (AkReal64)AK_CORE_SAMPLERATE );
    }
	static inline AkReal64 SamplesToSeconds( 
        AkInt32 in_samples
        )
    {
        return (AkReal64)in_samples / (AkReal64)AK_CORE_SAMPLERATE;
    }

	static inline AkInt32 ToShortRange( 
		AkInt64 in_uNumSamples 
		)
	{
		AKASSERT( in_uNumSamples >= AK_INT_MIN && in_uNumSamples < AK_INT_MAX );
		return ((AkInt32)in_uNumSamples);
	}
}

#endif
