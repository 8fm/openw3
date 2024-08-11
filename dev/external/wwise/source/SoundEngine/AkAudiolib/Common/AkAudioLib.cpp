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

// AkAudioLib.cpp : Defines the entry point for the DLL application.
//
#include "stdafx.h"

#include "AkMath.h"
#include "AkAudioLib.h"
#include "AkAudioMgr.h"
#include "AkEffectsMgr.h"
#include "AkLEngine.h"
#include "AkStateMgr.h"
#include "AkAudioLibIndex.h"
#include "AkRegistryMgr.h"
#include "AkRegisteredObj.h"
#include "AkEvent.h"
#include "AkDynamicSequence.h"
#include "AkDialogueEvent.h"
#include "AkParameterNode.h"
#include "AkTransitionManager.h"
#include "AkPlayingMgr.h"
#include "AkPathManager.h"
#include "AkBankMgr.h"
#include "AkMonitor.h"
#include "AkCritical.h"

#include "AkURenderer.h"
#include "AkRTPCMgr.h"
#include "AkAudioLibTimer.h"
#include <AK/Tools/Common/AkPlatformFuncs.h>
#include <AK/Tools/Common/AkMonitorError.h>
#include "AkActionStop.h"
#include "AkProfile.h"
#include "AkEnvironmentsMgr.h"
#include "AkAttenuationMgr.h"
#include "AkFxBase.h"
#include "AkFXMemAlloc.h"
#include "AkPositionRepository.h"
#include "AkLayer.h"
#include "AkSink.h"
#include "AkOutputMgr.h"
#include "AkSpeakerPan.h"

#ifdef AK_MOTION
	#include "AkFeedbackMgr.h"
#endif // AK_MOTION

#include <AK/SoundEngine/Common/AkDynamicDialogue.h>
#include <AK/MotionEngine/Common/AkMotionEngine.h>

#include "AkPoolSizes.h"
#include <AK/Tools/Common/AkFNVHash.h>

#include "AkRuntimeEnvironmentMgr.h"

#include "ICommunicationCentral.h"

#if defined AK_WII_FAMILY
#include "AkWiimoteMgr.h"
#endif

//-----------------------------------------------------------------------------
// Behavioral engine singletons.
//-----------------------------------------------------------------------------
CAkAudioLibIndex*		g_pIndex			 = NULL;
CAkAudioMgr*			g_pAudioMgr			 = NULL;
CAkStateMgr*			g_pStateMgr			 = NULL;
CAkRegistryMgr*			g_pRegistryMgr		 = NULL;
CAkBankMgr*				g_pBankManager		 = NULL;
CAkTransitionManager*	g_pTransitionManager = NULL;
CAkPathManager*			g_pPathManager		 = NULL;
AKSOUNDENGINE_API CAkRTPCMgr* g_pRTPCMgr	 = NULL;
CAkEnvironmentsMgr*		g_pEnvironmentMgr	 = NULL;
CAkPlayingMgr*			g_pPlayingMgr		 = NULL;
CAkPositionRepository*  g_pPositionRepository = NULL;

CAkBusCallbackMgr*		g_pBusCallbackMgr	 = NULL;

#ifndef AK_WIN
AkUInt32 AKRANDOM::g_uiRandom;
#endif

AKSOUNDENGINE_API AkMemPoolId g_DefaultPoolId = AK_INVALID_POOL_ID;

#ifndef AK_OPTIMIZED
AK::Comm::ICommunicationCentral * g_pCommCentral	= NULL;
#endif

// Behavioral settings.
AkInitSettings			g_settings     		= { 0, };
AKSOUNDENGINE_API AkAssertHook			g_pAssertHook		= NULL;

CAkLock g_csMain;

typedef AkArray<AkGlobalCallbackFunc, AkGlobalCallbackFunc, ArrayPoolDefault> BehavioralExtensionArray;
BehavioralExtensionArray g_aBehavioralExtensions;
AkExternalStateHandlerCallback g_pExternalStateHandler = NULL;
AkExternalBankHandlerCallback g_pExternalBankHandlerCallback = NULL;
AkExternalProfileHandlerCallback g_pExternalProfileHandlerCallback = NULL;

extern AkReal32 g_fVolumeThreshold;
extern AkReal32 g_fVolumeThresholdDB;
AK::SoundEngine::AkCommandPriority g_eVolumeThresholdPriority = AK::SoundEngine::AkCommandPriority_None;
AK::SoundEngine::AkCommandPriority g_eNumVoicesPriority = AK::SoundEngine::AkCommandPriority_None;

namespace AK
{

namespace SoundEngine
{

// Privates

static bool s_bInitialized = false;
static AkPlayingID g_PlayingID = 0;

#ifdef _DEBUG
#define CHECK_SOUND_ENGINE_INPUT_VALID
#endif

#ifdef CHECK_SOUND_ENGINE_INPUT_VALID

static bool IsValidFloatVector( const AkVector & in_vec )
{
	return AkMath::IsValidFloatInput( in_vec.X ) &&
		AkMath::IsValidFloatInput( in_vec.Y ) &&
		AkMath::IsValidFloatInput( in_vec.Z );
}

static bool IsValidFloatVolumes( const AkSpeakerVolumes & in_volumes )
{
	return AkMath::IsValidFloatInput( in_volumes.fFrontLeft ) &&
		AkMath::IsValidFloatInput( in_volumes.fFrontRight )
#ifdef AK_REARCHANNELS
		&& AkMath::IsValidFloatInput( in_volumes.fRearLeft )
		&& AkMath::IsValidFloatInput( in_volumes.fRearRight )
#endif // AK_REARCHANNELS
#ifdef AK_71AUDIO
		&& AkMath::IsValidFloatInput( in_volumes.fSideLeft )
		&& AkMath::IsValidFloatInput( in_volumes.fSideRight )
#endif // AK_71AUDIO
#ifdef AK_LFECENTER
		&& AkMath::IsValidFloatInput( in_volumes.fCenter )
		&& AkMath::IsValidFloatInput( in_volumes.fLfe )
#endif // AK_LFECENTER
	;
}

#endif // CHECK_SOUND_ENGINE_INPUT_VALID

// Forward declarations

static AKRESULT CreateDefaultMemPools();
static void DestroyDefaultMemPools();
static AKRESULT PreInit( AkInitSettings * io_pSettings );
static AKRESULT InitRenderer();

//////////////////////////////////////////////////////////////////////////////////
// MAIN PUBLIC INTERFACE IMPLEMENTATION
//////////////////////////////////////////////////////////////////////////////////

static void _MakeLower( char* in_pString, size_t in_strlen )
{
	for( size_t i = 0; i < in_strlen; ++i )
	{
		if( in_pString[i] >= 'A' && in_pString[i] <= 'Z' )
		{
			in_pString[i] += 0x20;  
		}
	}
}

static inline AkUInt32 HashName( const char * in_pString, size_t in_strlen )
{
	AK::FNVHash32 MainHash;
	return MainHash.Compute( (const unsigned char *) in_pString, (unsigned int)in_strlen );
}

#ifdef AK_SUPPORT_WCHAR
AkUInt32 GetIDFromString( const wchar_t* in_pszString )
{
	if( !in_pszString )
		return AK_INVALID_UNIQUE_ID;

	// 1- Make char string.

	char szString[ AK_MAX_STRING_SIZE ]; 
	
	AkWideCharToChar( in_pszString, AK_MAX_STRING_SIZE-1, szString );
	szString[ AK_MAX_STRING_SIZE-1 ] = 0;

	size_t stringSize = strlen( szString );

	// 2- Make lower case.
	_MakeLower( szString, stringSize );

	// 3- Hash the resulting string.
	return HashName( szString, stringSize );
}
#endif //AK_SUPPORT_WCHAR

AkUInt32 GetIDFromString( const char* in_pszString )
{
	if( !in_pszString )
		return AK_INVALID_UNIQUE_ID;

	char szString[ AK_MAX_STRING_SIZE ]; 
	AKPLATFORM::SafeStrCpy( szString, in_pszString, AK_MAX_STRING_SIZE );

	size_t stringSize = strlen( in_pszString );

	// 1- Make lower case.
	_MakeLower( szString, stringSize );

	// 2- Hash the resulting string.
	return HashName( szString, stringSize );
}

bool IsInitialized()
{
	return s_bInitialized;
}

AKRESULT Init(
    AkInitSettings *			in_pSettings,   		///< Sound engine settings. Can be NULL (use default values).
    AkPlatformInitSettings *	in_pPlatformSettings  	///< Platform specific settings. Can be NULL (use default values).
	)
{
	// g_eVolumeThresholdPriority must be reset at init, to pad the situation where the game would term and re-init the engine.
	g_eVolumeThresholdPriority = AK::SoundEngine::AkCommandPriority_None;
	g_eNumVoicesPriority = AK::SoundEngine::AkCommandPriority_None;

#if defined AK_WIN && defined AK_CPU_X86
	if ( !AkRuntimeEnvironmentMgr::Instance()->GetSIMDSupport(AK_SIMD_SSE) )
	{
		AKASSERT( !"SSE instruction set not supported." );
        return AK_SSEInstructionsNotSupported;
	}
#endif

    // Check Memory manager.
	if ( !MemoryMgr::IsInitialized() )
    {
        AKASSERT( !"Memory manager is not initialized" );
        return AK_MemManagerNotInitialized;
    }
    // Check Stream manager.
    if ( IAkStreamMgr::Get( ) == NULL )
    {
        AKASSERT( !"Stream manager does not exist" );
        return AK_StreamMgrNotInitialized;
    }

    // Store upper engine global settings.
    if ( in_pSettings == NULL )
    {
		GetDefaultInitSettings( g_settings );
    }
    else
    {
    	// TODO. Check values, clamp to min or max.
        g_settings = *in_pSettings;
    }

	g_pAssertHook = g_settings.pfnAssertHook;

	// Store lower engine global settings.
	CAkLEngine::ApplyGlobalSettings( in_pPlatformSettings );
    
    // Instantiate, initialize and assign global pointer.

	AKRESULT eResult = AK_Fail;

	AKASSERT( !s_bInitialized || !"SoundEngine::Init should be called only once" );
    if ( !s_bInitialized )
    {
		eResult = CreateDefaultMemPools();
        if ( eResult == AK_Success )
        {
            AKASSERT( g_DefaultPoolId != AK_INVALID_POOL_ID );

			eResult = PreInit( in_pSettings );
			if( eResult == AK_Success )
				eResult = InitRenderer();
			
			if ( eResult != AK_Success )
			{
                Term();
			}
			else
			{
				s_bInitialized = true;
			}
        }

        // If instantiation failed, need to destroy pools. 
        if ( !s_bInitialized )
        {
            DestroyDefaultMemPools();
        }
    }

	// WG-6434
	{
		static const char s_szRandomSeed[] = { "779AD1D9-3419-4cbf-933B-B038DF5A2818" };
		char szRandomSeed[36] = { 0 };
		::strncpy( szRandomSeed, s_szRandomSeed, sizeof szRandomSeed );
	}

    return eResult;
}

void GetDefaultInitSettings(
    AkInitSettings & out_settings   		///< Default sound engine settings returned
	)
{
	out_settings.pfnAssertHook = NULL;
	out_settings.uMaxNumPaths = DEFAULT_MAX_NUM_PATHS;
    out_settings.uMaxNumTransitions = DEFAULT_MAX_NUM_TRANSITIONS;
    out_settings.uDefaultPoolSize = DEFAULT_POOL_SIZE;
	out_settings.fDefaultPoolRatioThreshold = 1.0f;// 1.0f == means 100% == disabled by default
	out_settings.uCommandQueueSize = COMMAND_QUEUE_SIZE;
	out_settings.uPrepareEventMemoryPoolID = AK_INVALID_POOL_ID;
	out_settings.bEnableGameSyncPreparation = false;
	out_settings.uContinuousPlaybackLookAhead = DEFAULT_CONTINUOUS_PLAYBACK_LOOK_AHEAD;

    out_settings.uMonitorPoolSize = MONITOR_POOL_SIZE;		
    out_settings.uMonitorQueuePoolSize = MONITOR_QUEUE_POOL_SIZE;

	out_settings.eMainOutputType = AkSink_Main;

	// Preparing global default settings; client does not care about the speaker angles at this point.
	CAkLEngine::GetDefaultOutputSettings(AkSink_Main, out_settings.settingsMainOutput);
}

void GetDefaultPlatformInitSettings(
    AkPlatformInitSettings &	out_platformSettings  	///< Default platform specific settings returned
	)
{
	CAkLEngine::GetDefaultPlatformInitSettings( out_platformSettings );
}

void Term()
{
#ifdef AK_MOTION
	//Audiomanager must be stopped, then the renderer must be destroyed, then the audiomanager gets destroyed
	CAkFeedbackDeviceMgr* pMgr = CAkFeedbackDeviceMgr::Get();
	if (pMgr != NULL)
		pMgr->Stop();
#endif // AK_MOTION

	//Stop the audio and bank threads before completely destroying the objects.
	if (g_pAudioMgr)
	{
		g_pAudioMgr->Stop();
	}

	if(g_pBankManager)
	{
		g_pBankManager->StopThread();
	}

	// Need to remove all playback objects before doing the last call on behavioral extensions
	CAkLEngine::Stop();

	for ( int i = (int) g_aBehavioralExtensions.Length() - 1; i >= 0; --i ) // from end in case an extension unregisters itself in callback.
		g_aBehavioralExtensions[ i ]( true );
	g_aBehavioralExtensions.Term();

	CAkURenderer::Term();

	if (g_pAudioMgr)
	{
		g_pAudioMgr->Term();
		AkDelete(g_DefaultPoolId,g_pAudioMgr);
		g_pAudioMgr = NULL;
	}

	if(g_pBankManager)
	{
		AKVERIFY(g_pBankManager->Term() == AK_Success);
		AkDelete(g_DefaultPoolId,g_pBankManager);
		g_pBankManager = NULL;
	}

	if (g_pIndex)
	{
		// Must be done after unloading banks and killing the bank manager and before releasing others managers.
		g_pIndex->ReleaseTempObjects();
		g_pIndex->ReleaseDynamicSequences();
	}

#if defined AK_WII_FAMILY
	CAkWiimoteMgr::Term();
#endif

	if (g_pStateMgr)
	{
		g_pStateMgr->Term();
		AkDelete(g_DefaultPoolId,g_pStateMgr);
		g_pStateMgr = NULL;
	}

	if(g_pPathManager)
	{
		g_pPathManager->Term();
		AkDelete(g_DefaultPoolId,g_pPathManager);
		g_pPathManager = NULL;
	}

	if(g_pTransitionManager)
	{
		g_pTransitionManager->Term();
		AkDelete(g_DefaultPoolId,g_pTransitionManager);
		g_pTransitionManager = NULL;
	}

	if (g_pRegistryMgr)
	{
		g_pRegistryMgr->Term();
		AkDelete(g_DefaultPoolId,g_pRegistryMgr);
		g_pRegistryMgr = NULL;
	}

	if(g_pPlayingMgr)
	{
		g_pPlayingMgr->Term();
		AkDelete(g_DefaultPoolId,g_pPlayingMgr);
		g_pPlayingMgr = NULL;
	}

	if(g_pPositionRepository)
	{
		g_pPositionRepository->Term();
		AkDelete(g_DefaultPoolId,g_pPositionRepository);
		g_pPositionRepository = NULL;
	}

	if(g_pEnvironmentMgr)
	{
		g_pEnvironmentMgr->Term();
		AkDelete(g_DefaultPoolId,g_pEnvironmentMgr);
		g_pEnvironmentMgr = NULL;
	}

	if(g_pRTPCMgr)
	{
		AKVERIFY(g_pRTPCMgr->Term() == AK_Success);
		AkDelete(g_DefaultPoolId,g_pRTPCMgr);
		g_pRTPCMgr = NULL;
	}

	if (g_pIndex)//IMPORTANT : g_pIndex MUST STAY ANTE-PENULTIEME DELETION OF AKINIT()!!!!!!!!!!!!!!!!!!
	{
		g_pIndex->Term();
		AkDelete( g_DefaultPoolId, g_pIndex );
		g_pIndex = NULL;
	}

#ifndef AK_OPTIMIZED
	AkMonitor * pMonitor = AkMonitor::Get();
	if ( pMonitor )
	{
		pMonitor->StopMonitoring(); 
		AkMonitor::Destroy();
	}
#endif

	g_aBehavioralExtensions.Term();
	
	AK_PERF_TERM();
	AK_TERM_TIMERS();

    DestroyDefaultMemPools();

	s_bInitialized = false;
}

//////////////////////////////////////////////////////////////////////////////////
// Tell the Audiolib it may now process all the events in the queue
//////////////////////////////////////////////////////////////////////////////////
AKRESULT RenderAudio()
{
	AKASSERT(g_pAudioMgr);

	return g_pAudioMgr->RenderAudio();
}

AKRESULT RegisterPlugin( 
	AkPluginType in_eType,
	AkUInt32 in_ulCompanyID,						// Company identifier (as declared in plugin description XML)
	AkUInt32 in_ulPluginID,							// Plugin identifier (as declared in plugin description XML)
    AkCreatePluginCallback	in_pCreateFunc,			// Pointer to the effect's Create() function.
    AkCreateParamCallback	in_pCreateParamFunc		// Pointer to the effect param's Create() function.
    )
{
	return CAkEffectsMgr::RegisterPlugin( in_eType, in_ulCompanyID, in_ulPluginID, in_pCreateFunc, in_pCreateParamFunc);
}

AKRESULT RegisterCodec( 
	AkUInt32 in_ulCompanyID,						// Company identifier (as declared in plugin description XML)
	AkUInt32 in_ulPluginID,							// Plugin identifier (as declared in plugin description XML)
	AkCreateFileSourceCallback in_pFileCreateFunc,	// File source creation function
	AkCreateBankSourceCallback in_pBankCreateFunc	// Bank source creation function
    )
{
	return CAkEffectsMgr::RegisterCodec( in_ulCompanyID, in_ulPluginID, in_pFileCreateFunc, in_pBankCreateFunc);
}

///////////////////////////////////////////////////////////////////////////
// RTPC
///////////////////////////////////////////////////////////////////////////
AKRESULT SetPosition( 
	AkGameObjectID in_GameObj, 
	const AkSoundPosition & in_Position
	)
{
	if ( in_GameObj == 0 ) // omni
		return AK_Fail;
	
	return SetPositionInternal( in_GameObj, in_Position );
}

AKRESULT SetPositionInternal( 
	AkGameObjectID in_GameObj, 
	const AkSoundPosition & in_Position
	)
{
	AKASSERT(g_pAudioMgr);
#ifdef CHECK_SOUND_ENGINE_INPUT_VALID
	if ( !(IsValidFloatVector( in_Position.Orientation ) && IsValidFloatVector( in_Position.Position ) ) )
		MONITOR_ERRORMSG( AKTEXT("AK::SoundEngine::SetPosition : Invalid Float in in_Position") );
#endif

	AkQueuedMsg Item( QueuedMsgType_GameObjPosition );

	Item.gameobjpos.GameObjID = in_GameObj;
	Item.gameobjpos.Position = in_Position;

	return g_pAudioMgr->Enqueue(Item, AkQueuedMsg::Sizeof_GameObjPosition() );
}

//Maximum position coordinate that can be passed.  This is simply sqrt(FLX_MAX/3) which will give FLT_MAX when used in distance calculation.
#define AK_MAX_DISTANCE 1.065023233e+19F

AKRESULT SetMultiplePositions( 
	AkGameObjectID in_GameObjectID,
	const AkSoundPosition * in_pPositions,
	AkUInt16 in_NumPositions,
	MultiPositionType in_eMultiPositionType /*= MultiPositionType_MultiDirection*/
    )
{
	if( in_eMultiPositionType > MultiPositionType_MultiDirections ||
		(in_NumPositions > 0 && in_pPositions == NULL))
		return AK_InvalidParameter;

	AKASSERT(g_pAudioMgr);

	// Here, we need to create the complete variable size object on the stack.
	// And then use the placement new for the object.
	// We have to copy the array over since the enqueue function can only do one write and this write must be atomic.
	AkUInt32 uAllocSize = AkQueuedMsg::Sizeof_GameObjMultiPositionBase() + in_NumPositions * sizeof( AkSoundPosition );
	if (uAllocSize > g_pAudioMgr->GetMaximumMsgSize())
	{
		MONITOR_ERRORMSG( AKTEXT("AK::SoundEngine::SetMultiplePositions: too many positions.") );
		return AK_Fail;
	}

	AkQueuedMsg* pItem = (AkQueuedMsg*)AkAlloca( uAllocSize );
	AkPlacementNew( pItem ) AkQueuedMsg( QueuedMsgType_GameObjMultiPosition );

	pItem->gameObjMultiPos.eMultiPositionType = in_eMultiPositionType;
	pItem->gameObjMultiPos.GameObjID = in_GameObjectID;
	pItem->gameObjMultiPos.uNumPositions = in_NumPositions;
	for( AkUInt16 i = 0; i < in_NumPositions; ++i )
	{
#ifdef CHECK_SOUND_ENGINE_INPUT_VALID
		if ( !(IsValidFloatVector( in_pPositions[i].Orientation ) && IsValidFloatVector( in_pPositions[i].Position ) ) )
			MONITOR_ERRORMSG( AKTEXT("AK::SoundEngine::SetMultiplePositions : Invalid Float in in_pPositions") );
#endif

		pItem->gameObjMultiPos.aMultiPosition[i] = in_pPositions[i];
	}

	return g_pAudioMgr->Enqueue( *pItem, uAllocSize );
}

AKRESULT SetAttenuationScalingFactor(
	AkGameObjectID in_GameObj,
	AkReal32 in_fAttenuationScalingFactor
	)
{
	if( in_fAttenuationScalingFactor <= 0.0f )// negative and zero are not valid scaling factor.
	{
		return AK_InvalidParameter;
	}
	AKASSERT( g_pAudioMgr );
#ifdef CHECK_SOUND_ENGINE_INPUT_VALID
	if ( !(AkMath::IsValidFloatInput( in_fAttenuationScalingFactor ) ) )
		MONITOR_ERRORMSG( AKTEXT("AK::SoundEngine::SetAttenuationScalingFactor : Invalid Float in in_fAttenuationScalingFactor") );
#endif

	AkQueuedMsg Item( QueuedMsgType_GameObjScalingFactor );

	Item.gameobjscalingfactor.GameObjID = in_GameObj;
	Item.gameobjscalingfactor.fValue = in_fAttenuationScalingFactor;

	return g_pAudioMgr->Enqueue(Item, AkQueuedMsg::Sizeof_GameObjScalingFactor() );
}

AKRESULT SetListenerScalingFactor(
	AkUInt32 in_listenerIndex,
	AkReal32 in_fListenerScalingFactor
	)
{
	if( in_fListenerScalingFactor <= 0.0f )// negative and zero are not valid scaling factor.
	{
		return AK_InvalidParameter;
	}
	AKASSERT( g_pAudioMgr );
#ifdef CHECK_SOUND_ENGINE_INPUT_VALID
	if ( !(AkMath::IsValidFloatInput( in_fListenerScalingFactor ) ) )
		MONITOR_ERRORMSG( AKTEXT("AK::SoundEngine::SetListenerScalingFactor : Invalid Float in in_fListenerScalingFactor") );
#endif

	AkQueuedMsg Item( QueuedMsgType_ListenerScalingFactor );

	Item.listenerscalingfactor.uListenerIndex = in_listenerIndex;
	Item.listenerscalingfactor.fValue = in_fListenerScalingFactor;

	return g_pAudioMgr->Enqueue(Item, AkQueuedMsg::Sizeof_ListenerScalingFactor() );
}

AKRESULT SetActiveListeners(
	AkGameObjectID in_GameObj,					///< Game object.
	AkUInt32 in_uListenerMask					///< Bitmask representing active listeners. LSB = Listener 0, set to 1 means active.
	)
{
	AKASSERT(g_pAudioMgr);
	AKASSERT(in_uListenerMask <= AK_ALL_LISTENERS_MASK);
	if (in_uListenerMask > AK_ALL_LISTENERS_MASK)
		return AK_InvalidParameter;	//Invalid listener.

	AkQueuedMsg Item( QueuedMsgType_GameObjActiveListeners );

	Item.gameobjactlist.GameObjID = in_GameObj;
	Item.gameobjactlist.uListenerMask = in_uListenerMask;

	return g_pAudioMgr->Enqueue(Item, AkQueuedMsg::Sizeof_GameObjActiveListeners() );
}

#if defined AK_WII_FAMILY
namespace Wii
{
#if defined AK_WII
AKRESULT SetActiveControllers(
	AkGameObjectID in_GameObj,
	AkUInt32 in_uActiveControllersMask
	)
{
	AKASSERT(g_pAudioMgr);

	AkQueuedMsg Item( QueuedMsgType_GameObjActiveControllers );

	Item.gameobjactcontroller.GameObjID = in_GameObj;
	Item.gameobjactcontroller.uActiveControllerMask = in_uActiveControllersMask;

	return g_pAudioMgr->Enqueue(Item, AkQueuedMsg::Sizeof_GameObjActiveControllers() );
}

#endif //AK_WII
} // namespace Wii
#endif //AK_WII_FAMILY

AKRESULT SetListenerPosition( 
		    const AkListenerPosition & in_Position,
		    AkUInt32 in_ulIndex /*= 0*/ //actually unused, only there in the situation we would start using miltiple listeners
		    )
{
	AKASSERT(g_pAudioMgr);

#ifdef CHECK_SOUND_ENGINE_INPUT_VALID
	bool bValid = true;
	if ( !(	IsValidFloatVector( in_Position.OrientationFront ) && 
			IsValidFloatVector( in_Position.OrientationTop ) && 
			IsValidFloatVector( in_Position.Position ) ) )
	{
		MONITOR_ERRORMSG( AKTEXT("AK::SoundEngine::SetListenerPosition : Invalid Float in in_Position") );
		bValid = false;
	}
#endif

	AkQueuedMsg Item( QueuedMsgType_ListenerPosition );

	// adjust front anyway as chances are we will rarely
	// get fDot spot on 0.0f
	AkReal32 fDot = AkMath::DotProduct( in_Position.OrientationFront, in_Position.OrientationTop );

	Item.listpos.Position.OrientationFront.X = in_Position.OrientationFront.X - in_Position.OrientationTop.X * fDot;
	Item.listpos.Position.OrientationFront.Y = in_Position.OrientationFront.Y - in_Position.OrientationTop.Y * fDot;
	Item.listpos.Position.OrientationFront.Z = in_Position.OrientationFront.Z - in_Position.OrientationTop.Z * fDot;

	Item.listpos.Position.OrientationTop = in_Position.OrientationTop;

	// normalise them
	AkMath::Normalise(Item.listpos.Position.OrientationFront);
	AkMath::Normalise(Item.listpos.Position.OrientationTop);

#ifdef CHECK_SOUND_ENGINE_INPUT_VALID
	if ( bValid &&
		!(	IsValidFloatVector( Item.listpos.Position.OrientationFront ) && 
			IsValidFloatVector( Item.listpos.Position.OrientationTop ) ) )
	{
		MONITOR_ERRORMSG( AKTEXT("AK::SoundEngine::SetListenerPosition : Invalid Float in normalized orientation") );
	}
#endif

	Item.listpos.uListenerIndex = in_ulIndex;
	Item.listpos.Position.Position = in_Position.Position;

	return g_pAudioMgr->Enqueue(Item, AkQueuedMsg::Sizeof_ListenerPosition() );
}

AKRESULT SetListenerSpatialization(
	AkUInt32 in_ulIndex,						///< Listener index. 
	bool in_bSpatialized,
	AkSpeakerVolumes * in_pVolumeOffsets
	)
{
	AKASSERT(g_pAudioMgr);

	AkQueuedMsg Item( QueuedMsgType_ListenerSpatialization );

	Item.listspat.uListenerIndex = in_ulIndex;
	Item.listspat.bSpatialized = in_bSpatialized;

	if ( in_pVolumeOffsets )
	{		
#ifdef CHECK_SOUND_ENGINE_INPUT_VALID
	if ( !(IsValidFloatVolumes( *in_pVolumeOffsets ) ) )
		MONITOR_ERRORMSG( AKTEXT("AK::SoundEngine::SetListenerSpatialization : Invalid Float in in_pVolumeOffsets") );
#endif
		
		Item.listspat.bSetVolumes = true;
		Item.listspat.Volumes = *in_pVolumeOffsets;
	}
	else
	{
		Item.listspat.bSetVolumes = false;
	}

	return g_pAudioMgr->Enqueue(Item, AkQueuedMsg::Sizeof_ListenerSpatialization() );
}

AKRESULT SetListenerPipeline(
	AkUInt32 in_uIndex,						///< Listener index (0: first listener, 7: 8th listener)
	bool in_bAudio,							///< True=Listens to audio events (by default it is true)
	bool in_bFeedback						///< True=Listens to feedback events (by default it is false)
	)
{
	AKASSERT(g_pAudioMgr);

	AkQueuedMsg Item( QueuedMsgType_ListenerPipeline );

	Item.listpipe.uListenerIndex = in_uIndex;
	Item.listpipe.bAudio = in_bAudio;
	Item.listpipe.bFeedback = in_bFeedback;

	return g_pAudioMgr->Enqueue(Item, AkQueuedMsg::Sizeof_ListenerPipeline() );
}

AKRESULT SetRTPCValue( 
		    AkRtpcID in_rtpcID, 
		    AkReal32 in_value, 
		    AkGameObjectID in_gameObjectID /*= AK_INVALID_GAME_OBJECT*/,
			AkTimeMs in_uValueChangeDuration /*= 0*/,
			AkCurveInterpolation in_eFadeCurve /*= AkCurveInterpolation_Linear*/
		    )
{
	AKASSERT(g_pAudioMgr);
#ifdef CHECK_SOUND_ENGINE_INPUT_VALID
	if ( !(AkMath::IsValidFloatInput( in_value ) ) )
		MONITOR_ERRORMSG( AKTEXT("AK::SoundEngine::SetRTPCValue : Invalid Float in in_Value") );
#endif

	if ( in_uValueChangeDuration == 0 )
	{
		AkQueuedMsg Item( QueuedMsgType_RTPC );

		Item.rtpc.ID = in_rtpcID;
		Item.rtpc.Value = in_value;
		Item.rtpc.GameObjID = in_gameObjectID;

		return g_pAudioMgr->Enqueue(Item, AkQueuedMsg::Sizeof_Rtpc() );
	}
	else
	{
		AkQueuedMsg Item( QueuedMsgType_RTPCWithTransition );
		Item.rtpcWithTransition.ID = in_rtpcID;
		Item.rtpcWithTransition.Value = in_value;
		Item.rtpcWithTransition.GameObjID = in_gameObjectID;
		Item.rtpcWithTransition.transParams.TransitionTime = in_uValueChangeDuration;
		Item.rtpcWithTransition.transParams.eFadeCurve = in_eFadeCurve;

		return g_pAudioMgr->Enqueue(Item, AkQueuedMsg::Sizeof_RtpcWithTransition() );
	}
}

#ifdef AK_SUPPORT_WCHAR
AKRESULT SetRTPCValue( 
		        const wchar_t* in_pszRtpcName, 
		        AkReal32 in_value, 
		        AkGameObjectID in_gameObjectID /*= AK_INVALID_GAME_OBJECT */,
				AkTimeMs in_uValueChangeDuration /*= 0*/,
				AkCurveInterpolation in_eFadeCurve /*= AkCurveInterpolation_Linear*/
		        )
{
	AkRtpcID id = GetIDFromString( in_pszRtpcName );
	if ( id == AK_INVALID_RTPC_ID )
		return AK_IDNotFound;

	return SetRTPCValue( id, in_value, in_gameObjectID, in_uValueChangeDuration, in_eFadeCurve );
}
#endif //AK_SUPPORT_WCHAR

AKRESULT SetRTPCValue( 
		        const char* in_pszRtpcName, 
		        AkReal32 in_value, 
		        AkGameObjectID in_gameObjectID /*= AK_INVALID_GAME_OBJECT */,
				AkTimeMs in_uValueChangeDuration /*= 0*/,
				AkCurveInterpolation in_eFadeCurve /*= AkCurveInterpolation_Linear*/
		        )
{
	AkRtpcID id = GetIDFromString( in_pszRtpcName );
	if ( id == AK_INVALID_RTPC_ID )
		return AK_IDNotFound;

	return SetRTPCValue( id, in_value, in_gameObjectID, in_uValueChangeDuration, in_eFadeCurve );
}

AKRESULT SetSwitch( 
		    AkSwitchGroupID in_SwitchGroup, 
		    AkSwitchStateID in_SwitchState, 
		    AkGameObjectID in_GameObj /*= AK_INVALID_GAME_OBJECT*/
		    )
{
	AKASSERT(g_pAudioMgr);

	AkQueuedMsg Item( QueuedMsgType_Switch );

	Item.setswitch.GameObjID = in_GameObj;
	Item.setswitch.SwitchGroupID = in_SwitchGroup;
	Item.setswitch.SwitchStateID = in_SwitchState;

	return g_pAudioMgr->Enqueue(Item, AkQueuedMsg::Sizeof_Switch() );
}

#ifdef AK_SUPPORT_WCHAR
AKRESULT SetSwitch( 
		                const wchar_t* in_pszSwitchGroup, 
		                const wchar_t* in_pszSwitchState, 
		                AkGameObjectID in_GameObj /*= AK_INVALID_GAME_OBJECT*/ 
		                )
{
	AkSwitchGroupID l_SwitchGroup = GetIDFromString( in_pszSwitchGroup );
	AkSwitchStateID l_SwitchState = GetIDFromString( in_pszSwitchState );

	if( l_SwitchGroup != AK_INVALID_RTPC_ID && l_SwitchState != AK_INVALID_RTPC_ID )
	{
		return SetSwitch( l_SwitchGroup, l_SwitchState, in_GameObj );
	}
	else
	{
		return AK_IDNotFound;
	}
}
#endif //AK_SUPPORT_WCHAR

AKRESULT SetSwitch( 
		                const char* in_pszSwitchGroup, 
		                const char* in_pszSwitchState, 
		                AkGameObjectID in_GameObj /*= AK_INVALID_GAME_OBJECT*/ 
		                )
{
	AkSwitchGroupID l_SwitchGroup = GetIDFromString( in_pszSwitchGroup );
	AkSwitchStateID l_SwitchState = GetIDFromString( in_pszSwitchState );

	if( l_SwitchGroup != AK_INVALID_RTPC_ID && l_SwitchState != AK_INVALID_RTPC_ID )
	{
		return SetSwitch( l_SwitchGroup, l_SwitchState, in_GameObj );
	}
	else
	{
		return AK_IDNotFound;
	}
}

AKRESULT PostTrigger( 
			AkTriggerID in_Trigger, 
		    AkGameObjectID in_GameObj /*= AK_INVALID_GAME_OBJECT*/
		    )
{
	AKASSERT(g_pAudioMgr);

	AkQueuedMsg Item( QueuedMsgType_Trigger );

	Item.trigger.GameObjID = in_GameObj;
	Item.trigger.TriggerID = in_Trigger;

	return g_pAudioMgr->Enqueue( Item, AkQueuedMsg::Sizeof_Trigger() );
}

#ifdef AK_SUPPORT_WCHAR
AKRESULT PostTrigger( 
			const wchar_t* in_pszTrigger, 
			AkGameObjectID in_GameObj /*= AK_INVALID_GAME_OBJECT*/ 
		                )
{
	AkTriggerID l_TriggerID = GetIDFromString( in_pszTrigger );

	if( l_TriggerID != AK_INVALID_UNIQUE_ID  )
	{
		return PostTrigger( l_TriggerID, in_GameObj );
	}
	else
	{
		return AK_IDNotFound;
	}
}
#endif //AK_SUPPORT_WCHAR

AKRESULT PostTrigger( 
			const char* in_pszTrigger, 
			AkGameObjectID in_GameObj /*= AK_INVALID_GAME_OBJECT*/ 
		                )
{
	AkTriggerID l_TriggerID = GetIDFromString( in_pszTrigger );

	if( l_TriggerID != AK_INVALID_UNIQUE_ID  )
	{
		return PostTrigger( l_TriggerID, in_GameObj );
	}
	else
	{
		return AK_IDNotFound;
	}
}

AKRESULT SetState( AkStateGroupID in_StateGroup, AkStateID in_State )
{
	return SetState( in_StateGroup, in_State, false, false );
}

AKRESULT SetState( AkStateGroupID in_StateGroup, AkStateID in_State, bool in_bSkipTransitionTime, bool in_bSkipExtension )
{
	AKASSERT(g_pAudioMgr);

	AkQueuedMsg Item( QueuedMsgType_State );

	// none is a reserved name in Wwise and no name can be named none, except the none state.
	#define AK_HASH_STATE_NONE 748895195 // This is the hash of "none" by GetIDFromString( "none" )
	// We still accept state = 0, for backward compatibility.
	// Letting this assert to catch errors if the hash algorithm changes
	AKASSERT( HashName( "none", 4 ) == AK_HASH_STATE_NONE );

	if( in_State == AK_HASH_STATE_NONE )
		in_State = 0;

	Item.setstate.StateGroupID = in_StateGroup;
	Item.setstate.StateID = in_State;
	Item.setstate.bSkipTransition = in_bSkipTransitionTime;
    Item.setstate.bSkipExtension = in_bSkipExtension;

	return g_pAudioMgr->Enqueue(Item, AkQueuedMsg::Sizeof_State() );
}

#ifdef AK_SUPPORT_WCHAR
AKRESULT SetState( const wchar_t* in_pszStateGroup, const wchar_t* in_pszState )
{
	AkStateGroupID	l_StateGroup	= GetIDFromString( in_pszStateGroup );
	AkStateID		l_State			= GetIDFromString( in_pszState );

	if( l_StateGroup != AK_INVALID_UNIQUE_ID && l_State != AK_INVALID_UNIQUE_ID )
	{
		return SetState( l_StateGroup, l_State );
	}
	else
	{
		return AK_IDNotFound;
	}
}
#endif //AK_SUPPORT_WCHAR

AKRESULT SetState( const char* in_pszStateGroup, const char* in_pszState )
{
	AkStateGroupID	l_StateGroup	= GetIDFromString( in_pszStateGroup );
	AkStateID		l_State			= GetIDFromString( in_pszState );

	if( l_StateGroup != AK_INVALID_UNIQUE_ID && l_State != AK_INVALID_UNIQUE_ID )
	{
		return SetState( l_StateGroup, l_State );
	}
	else
	{
		return AK_IDNotFound;
	}
}

#ifndef AK_OPTIMIZED
AKRESULT ResetSwitches( AkGameObjectID in_GameObjID )
{
	AKASSERT( g_pAudioMgr );

	AkQueuedMsg Item( QueuedMsgType_ResetSwitches );

	Item.resetswitches.GameObjID = in_GameObjID;

	return g_pAudioMgr->Enqueue(Item, AkQueuedMsg::Sizeof_ResetSwitches() );
}

AKRESULT ResetRTPC( AkGameObjectID in_GameObjID )
{
	AKASSERT( g_pAudioMgr );

	AkQueuedMsg Item( QueuedMsgType_ResetRTPC );

	Item.resetrtpc.GameObjID = in_GameObjID;

	return g_pAudioMgr->Enqueue(Item, AkQueuedMsg::Sizeof_ResetRTPC() );
}
#endif

AKRESULT ResetRTPCValue(AkUInt32 in_rtpcID, 
				AkGameObjectID in_gameObjectID,
				AkTimeMs in_uValueChangeDuration /*= 0*/,
				AkCurveInterpolation in_eFadeCurve /*= AkCurveInterpolation_Linear*/
				)
{
	AKASSERT( g_pAudioMgr );

	if ( in_uValueChangeDuration == 0 )
	{
		AkQueuedMsg Item( QueuedMsgType_ResetRTPCValue );
		Item.resetrtpcvalue.GameObjID = in_gameObjectID;
		Item.resetrtpcvalue.ParamID = in_rtpcID;
		return g_pAudioMgr->Enqueue(Item, AkQueuedMsg::Sizeof_ResetRTPCValue() );
	}
	else
	{
		AkQueuedMsg Item( QueuedMsgType_ResetRTPCValueWithTransition );
		Item.resetrtpcvalueWithTransition.GameObjID = in_gameObjectID;
		Item.resetrtpcvalueWithTransition.ParamID = in_rtpcID;
		Item.resetrtpcvalueWithTransition.transParams.TransitionTime = in_uValueChangeDuration;
		Item.resetrtpcvalueWithTransition.transParams.eFadeCurve = in_eFadeCurve;
		return g_pAudioMgr->Enqueue(Item, AkQueuedMsg::Sizeof_ResetRTPCValueWithTransition() );
	}
}

#ifdef AK_SUPPORT_WCHAR
AKRESULT ResetRTPCValue(const wchar_t* in_pszRtpcName, 
				AkGameObjectID in_gameObjectID,
				AkTimeMs in_uValueChangeDuration /*= 0*/,
				AkCurveInterpolation in_eFadeCurve /*= AkCurveInterpolation_Linear*/
				)
{
	AkRtpcID id = GetIDFromString( in_pszRtpcName );
	if ( id == AK_INVALID_RTPC_ID )
		return AK_IDNotFound;

	return ResetRTPCValue(id, in_gameObjectID, in_uValueChangeDuration, in_eFadeCurve);
}
#endif //AK_SUPPORT_WCHAR

AKRESULT ResetRTPCValue(const char* in_pszRtpcName, 
				AkGameObjectID in_gameObjectID,
				AkTimeMs in_uValueChangeDuration /*= 0*/,
				AkCurveInterpolation in_eFadeCurve /*= AkCurveInterpolation_Linear*/
				)
{
	AkRtpcID id = GetIDFromString( in_pszRtpcName );
	if ( id == AK_INVALID_RTPC_ID )
		return AK_IDNotFound;

	return ResetRTPCValue(id, in_gameObjectID, in_uValueChangeDuration, in_eFadeCurve);
}

AKRESULT SetGameObjectAuxSendValues( 
		AkGameObjectID		in_GameObj,				///< the unique object ID
		AkAuxSendValue*	in_aEnvironmentValues,		///< variable-size array of AkAuxSendValue(s)
		AkUInt32			in_uNumEnvValues		///< number of elements in struct
		)
{
	AKASSERT(g_pAudioMgr);

	if( in_uNumEnvValues > AK_MAX_AUX_PER_OBJ )
		return AK_InvalidParameter;

	if( in_uNumEnvValues )
	{
#ifdef CHECK_SOUND_ENGINE_INPUT_VALID
		for ( AkUInt32 i = 0; i < in_uNumEnvValues; ++i )
		{
			if ( !(AkMath::IsValidFloatInput( in_aEnvironmentValues[i].fControlValue ) ) )
				MONITOR_ERRORMSG( AKTEXT("AK::SoundEngine::SetGameObjectAuxSendValues : Invalid Float in in_aEnvironmentValues") );
		}
#endif
	}

	AkQueuedMsg Item( QueuedMsgType_GameObjEnvValues );

	Item.gameobjenvvalues.GameObjID = in_GameObj;
	Item.gameobjenvvalues.uNumValues = in_uNumEnvValues;
	AKPLATFORM::AkMemCpy( Item.gameobjenvvalues.EnvValues, in_aEnvironmentValues, sizeof( AkAuxSendValue ) * in_uNumEnvValues );

	return g_pAudioMgr->Enqueue(Item, AkQueuedMsg::Sizeof_GameObjEnvValues() );
}

AKRESULT RegisterBusVolumeCallback( 
			AkUniqueID in_busID,
			AkBusCallbackFunc in_pfnCallback
			)
{
	if( g_pBusCallbackMgr )
	{
		return g_pBusCallbackMgr->SetCallback( in_busID, in_pfnCallback );
	}
	return AK_Fail;
}

AKRESULT SetGameObjectOutputBusVolume( 
		AkGameObjectID		in_GameObj,			///< the unique object ID
		AkReal32			in_fControlValue	///< control value for dry level
		)
{
	AKASSERT( g_pAudioMgr );
#ifdef CHECK_SOUND_ENGINE_INPUT_VALID
	if ( !(AkMath::IsValidFloatInput( in_fControlValue )) )
		MONITOR_ERRORMSG( AKTEXT("AK::SoundEngine::SetGameObjectOutputBusVolume : Invalid Float in in_fControlValue") );
#endif

	AkQueuedMsg Item( QueuedMsgType_GameObjDryLevel );

	Item.gameobjdrylevel.GameObjID = in_GameObj;
	Item.gameobjdrylevel.fValue = in_fControlValue;

	return g_pAudioMgr->Enqueue(Item, AkQueuedMsg::Sizeof_GameObjDryLevel() );
}

AKRESULT SetObjectObstructionAndOcclusion(  
	AkGameObjectID in_ObjectID,           ///< Game object ID.
	AkUInt32 in_uListener,             ///< Listener ID.
	AkReal32 in_fObstructionLevel,		///< ObstructionLevel : [0.0f..1.0f]
	AkReal32 in_fOcclusionLevel			///< OcclusionLevel   : [0.0f..1.0f]
	)
{
	AKASSERT( g_pAudioMgr );

	AkQueuedMsg Item( QueuedMsgType_GameObjObstruction );

	Item.gameobjobstr.GameObjID = in_ObjectID;
	Item.gameobjobstr.uListenerIndex = in_uListener;
	Item.gameobjobstr.fObstructionLevel = in_fObstructionLevel;
	Item.gameobjobstr.fOcclusionLevel = in_fOcclusionLevel;

	return g_pAudioMgr->Enqueue(Item, AkQueuedMsg::Sizeof_GameObjObstruction() );
}

AKRESULT SetVolumeThreshold( AkReal32 in_fVolumeThresholdDB )
{
#ifdef CHECK_SOUND_ENGINE_INPUT_VALID
	if ( !(AkMath::IsValidFloatInput( in_fVolumeThresholdDB ) ) )
		MONITOR_ERRORMSG( AKTEXT("AK::SoundEngine::SetVolumeThreshold : Invalid Float in in_fVolumeThresholdDB") );
#endif

	return SetVolumeThresholdInternal( in_fVolumeThresholdDB, AkCommandPriority_Game );
}

AKRESULT SetVolumeThresholdInternal( AkReal32 in_fVolumeThresholdDB, AkCommandPriority in_uReserved )
{
	if( in_fVolumeThresholdDB < AK_MINIMUM_VOLUME_DBFS
		|| in_fVolumeThresholdDB > AK_MAXIMUM_VOLUME_DBFS )
		return AK_InvalidParameter;

	// First, check if the provided value must be considered.
	// the rule is : If the games sets something, we accept it.
	// if Wwise sets something, we accept it, unless the game previously set one.
	// the init bank sets it only if neither the game of Wwise did set a different value.
	if( in_uReserved <= g_eVolumeThresholdPriority )
	{
		g_eVolumeThresholdPriority = in_uReserved;

		// To protect against numerical errors, use our three methods of doing the dB-to-Lin conversion
		// and pick the highest mark as our linear threshold.

		AkReal32 fThresholdLinA = powf( 10.0f, in_fVolumeThresholdDB * 0.05f );
		AkReal32 fThresholdLinB = AkMath::dBToLin( in_fVolumeThresholdDB );

		AkSIMDSpeakerVolumes vLin;
		vLin.Set( in_fVolumeThresholdDB );
		vLin.dBToLin();
		AkReal32 fThresholdLinC = vLin.volumes.fFrontLeft;

		g_fVolumeThreshold = AkMath::Max( AkMath::Max( fThresholdLinA, fThresholdLinB ), fThresholdLinC );
		g_fVolumeThresholdDB = in_fVolumeThresholdDB;
		AKASSERT( g_fVolumeThreshold <= 1.0f && g_fVolumeThreshold >=0.0f );
	}
	return AK_Success;
}

AKRESULT SetMaxNumVoicesLimit( AkUInt16 in_maxNumberVoices )
{
	return SetMaxNumVoicesLimitInternal( in_maxNumberVoices, AkCommandPriority_Game );
}

// Get the output speaker configuration.
AkChannelMask GetSpeakerConfiguration(
	AkSinkType 			in_eSinkType,		///< Output sink type. Pass AkSink_Main for main output.
	AkUInt32 			in_iOutputID		///< Player number or device-unique identifier. Pass 0 for main.
	)
{
	AkDevice * pDevice = CAkOutputMgr::GetDevice( AK_MAKE_DEVICE_KEY( in_eSinkType, in_iOutputID ) );
	if ( pDevice && pDevice->pSink )
		return pDevice->pSink->GetSpeakerConfig();
	return AK_CHANNEL_MASK_INVALID;
}

AKRESULT GetPanningRule(
	AkPanningRule &		out_ePanningRule,	///< Returned panning rule (AkPanningRule_Speakers or AkPanningRule_Headphone) for given output.
	AkSinkType 			in_eSinkType,		///< Output sink type. Pass AkSink_Main for main output.
	AkUInt32 			in_iOutputID		///< Player number or device-unique identifier. Pass 0 for main.	
	)
{
	AkDevice * pDevice = CAkOutputMgr::GetDevice( AK_MAKE_DEVICE_KEY( in_eSinkType, in_iOutputID ) );
	if ( pDevice )
	{
		out_ePanningRule = pDevice->ePanningRule;
		return AK_Success;
	}
	return AK_Fail;
}

AKRESULT SetPanningRule( 
	AkPanningRule		in_ePanningRule,	///< Panning rule.
	AkSinkType 			in_eSinkType,		///< Output sink type. Pass AkSink_Main for main output.
	AkUInt32 			in_iOutputID		///< Player number or device-unique identifier. Pass 0 for main.
	)
{
	AkQueuedMsg Item( QueuedMsgType_SetPanningRule );
	Item.setPanningRule.iOutputID = in_iOutputID;
	Item.setPanningRule.eSinkType = in_eSinkType;
	Item.setPanningRule.panRule = in_ePanningRule;
	return g_pAudioMgr->Enqueue(Item, AkQueuedMsg::Sizeof_SetPanningRule() );
}

AKRESULT GetSpeakerAngles(
	AkReal32 *			io_pfSpeakerAngles,			///< Returned array of loudspeaker pair angles, in degrees relative to azimuth [0,180]. Pass NULL to get the required size of the array.
	AkUInt32 &			io_uNumAngles,				///< Returned number of angles in io_pfSpeakerAngles, which is the minimum between the value that you pass in, and the number of angles corresponding to the output configuration, or just the latter if io_pfSpeakerAngles is NULL.
	AkSinkType 			in_eSinkType,				///< Output sink type. Pass AkSink_Main for main output.
	AkUInt32 			in_iOutputID				///< Player number or device-unique identifier. Pass 0 for main.
	)
{
	AkDevice * pDevice = CAkOutputMgr::GetDevice( AK_MAKE_DEVICE_KEY( in_eSinkType, in_iOutputID ) );
	if ( pDevice && pDevice->pSink )
	{
		AkUInt32 uDesiredNumberOfAngles = AK::GetNumberOfAnglesForConfig( AK_SUPPORTED_SPEAKER_SETUP & pDevice->pSink->GetSpeakerConfig() );
		if ( uDesiredNumberOfAngles == 0 )
			return AK_Fail;

		if ( !io_pfSpeakerAngles )
		{
			// Just querying the number of angles required.
			io_uNumAngles = uDesiredNumberOfAngles;
		}
		else
		{
			io_uNumAngles = AkMin( io_uNumAngles, uDesiredNumberOfAngles );
			CAkSpeakerPan::ConvertSpeakerAngles( pDevice->puSpeakerAngles, io_uNumAngles, io_pfSpeakerAngles );
		}
		return AK_Success;
	}
	return AK_Fail;
}

AKRESULT SetSpeakerAngles(
	AkReal32 *			in_pfSpeakerAngles,		///< Array of loudspeaker pair angles, in degrees relative to azimuth [0,180].
	AkUInt32			in_uNumAngles,			///< Number of elements in in_pfSpeakerAngles. It should correspond to the output configuration.
	AkSinkType 			in_eSinkType,			///< Output sink type. Pass AkSink_Main for main output.
	AkUInt32 			in_iOutputID			///< Player number or device-unique identifier. Pass 0 for main.
	)
{
	if ( !in_pfSpeakerAngles || in_uNumAngles == 0 )
	{
		AKASSERT( !"Invalid angles" );
		return AK_InvalidParameter;
	}

	AkQueuedMsg item( QueuedMsgType_SetSpeakerAngles );

	size_t uSize = in_uNumAngles * sizeof( AkReal32 );
	item.setSpeakerAngles.pfSpeakerAngles = (AkReal32*)AkAlloc( g_DefaultPoolId, uSize );
	if ( !item.setSpeakerAngles.pfSpeakerAngles )
		return AK_InsufficientMemory;

	memcpy( item.setSpeakerAngles.pfSpeakerAngles, in_pfSpeakerAngles, uSize );
	item.setSpeakerAngles.uNumAngles = in_uNumAngles;
	item.setSpeakerAngles.eSinkType = in_eSinkType;
	item.setSpeakerAngles.iOutputID = in_iOutputID;

	return g_pAudioMgr->Enqueue( item, AkQueuedMsg::Sizeof_SetSpeakerAngles() );
}

AKRESULT SetMaxNumVoicesLimitInternal( AkUInt16 in_maxNumberVoices, AkCommandPriority in_uReserved )
{
	if( in_maxNumberVoices == 0 )
		return AK_InvalidParameter;

	// First, check if the provided value must be considered.
	// the rule is : If the games sets something, we accept it.
	// if Wwise sets something, we accept it, unless the game previously set one.
	// the init bank sets it only if neither the game of Wwise did set a different value.
	if( in_uReserved <= g_eNumVoicesPriority )
	{
		g_eNumVoicesPriority = in_uReserved;

		CAkURenderer::GetGlobalLimiter().UpdateMax( in_maxNumberVoices );
	}
	return AK_Success;
}

#ifndef AK_OPTIMIZED
AkLoudnessFrequencyWeighting g_eLoudnessFrequencyWeighting;
AkLoudnessFrequencyWeighting GetLoudnessFrequencyWeighting()
{
	return g_eLoudnessFrequencyWeighting;
}
void SetLoudnessFrequencyWeighting( AkLoudnessFrequencyWeighting in_eLoudnessFrequencyWeighting )
{
	g_eLoudnessFrequencyWeighting = in_eLoudnessFrequencyWeighting;
}
#endif


//////////////////////////////////////////////////////////////////////////////////
//Monitoring
//////////////////////////////////////////////////////////////////////////////////
#ifndef AK_OPTIMIZED
IALMonitor* GetMonitor()
{
	return AkMonitor::Get();
}
#else
IALMonitor* GetMonitor()
{
	// No monitoring in optimized mode
	// Get Monitor should be removed from the SDK...
	return NULL;
}
#endif

//////////////////////////////////////////////////////////////////////////////////
// Event Management
//////////////////////////////////////////////////////////////////////////////////

AkPlayingID PostEvent(
	AkUniqueID in_ulEventID,
	AkGameObjectID in_GameObjID,
	AkUInt32 in_uiFlags,// = 0
	AkCallbackFunc in_pfnCallback, // = NULL
	void* in_pCookie, // = NULL
	AkUInt32 in_cExternals, // = 0	
	AkExternalSourceInfo *in_pExternalSources, // = NULL
	AkPlayingID	in_PlayingID //= AK_INVALID_PLAYING_ID
	)
{
	AkCustomParamType temp;
	AkCustomParamType *pParam = NULL;
	if ( AK_EXPECT_FALSE( in_cExternals != 0 ) )
	{
		temp.customParam = 0;
		temp.ui32Reserved = 0;
		temp.pExternalSrcs = AkExternalSourceArray::Create(in_cExternals, in_pExternalSources);
		if (temp.pExternalSrcs == NULL)
			return AK_INVALID_PLAYING_ID ;

		pParam = &temp;
	}
	AkPlayingID playingID = PostEvent( in_ulEventID, in_GameObjID, in_uiFlags, in_pfnCallback, in_pCookie, pParam, in_PlayingID );

	if (playingID == AK_INVALID_PLAYING_ID && in_cExternals != 0)
		temp.pExternalSrcs->Release();

	return playingID;
}

AkPlayingID PostEvent(
	AkUniqueID in_ulEventID,
	AkGameObjectID in_GameObjID,
	AkUInt32 in_uiFlags,
	AkCallbackFunc in_pfnCallback,
	void* in_pCookie,
	AkCustomParamType * in_pCustomParam,
	AkPlayingID	in_PlayingID /*= AK_INVALID_PLAYING_ID*/
	)
{
	AKASSERT(g_pIndex);
	AKASSERT(g_pAudioMgr);
	AkQueuedMsg Item( QueuedMsgType_Event );

    if ( in_pCustomParam != NULL )
    {
	    Item.event.CustomParam = *in_pCustomParam;		
    }
    else
    {
        Item.event.CustomParam.customParam = 0;
	    Item.event.CustomParam.ui32Reserved = 0;
		Item.event.CustomParam.pExternalSrcs = NULL;
    }

	// This function call does get the pointer and addref it in an atomic call. 
	// Item.event.Event will be filled with NULL if it was not possible. 
	Item.event.Event = g_pIndex->m_idxEvents.GetPtrAndAddRef( in_ulEventID ); 

	if( Item.event.Event )
	{
		Item.event.PlayingID = AKPLATFORM::AkInterlockedIncrement((AkInt32*)&g_PlayingID);
		Item.event.TargetPlayingID = in_PlayingID;
		Item.event.GameObjID = in_GameObjID;

		AKASSERT( g_pPlayingMgr );

		AKRESULT eResult = g_pPlayingMgr->AddPlayingID( Item.event, in_pfnCallback, in_pCookie, in_uiFlags, Item.event.Event->ID() );
		if( eResult != AK_Success )
		{
			Item.event.Event->Release();
			return AK_INVALID_PLAYING_ID;
		}

		eResult = g_pAudioMgr->Enqueue(Item, AkQueuedMsg::Sizeof_Event() );
		AKASSERT(eResult == AK_Success);

		return Item.event.PlayingID;
	}
	else
	{
		MONITOR_ERROR_PARAM( AK::Monitor::ErrorCode_EventIDNotFound, in_ulEventID, AK_INVALID_PLAYING_ID, in_GameObjID, in_ulEventID, false );
		return AK_INVALID_PLAYING_ID;
	}
}

#ifdef AK_SUPPORT_WCHAR
AkPlayingID PostEvent(
	const wchar_t* in_pszEventName,
	AkGameObjectID in_GameObjID,
	AkUInt32 in_uiFlags,
	AkCallbackFunc in_pfnCallback,
	void* in_pCookie,
	AkUInt32 in_cExternals, // = 0	
	AkExternalSourceInfo *in_pExternalSources, // = NULL
	AkPlayingID	in_PlayingID //= AK_INVALID_PLAYING_ID
	)
{
	AkCustomParamType temp;
	AkCustomParamType *pParam = NULL;
	if ( AK_EXPECT_FALSE( in_cExternals != 0 ) )
	{
		temp.customParam = 0;
		temp.ui32Reserved = 0;
		temp.pExternalSrcs = AkExternalSourceArray::Create(in_cExternals, in_pExternalSources);
		if (temp.pExternalSrcs == NULL)
			return AK_INVALID_PLAYING_ID ;

		pParam = &temp;
	}

	AkUniqueID EventID = GetIDFromString( in_pszEventName );
    AkPlayingID returnedPlayingID = PostEvent( EventID, in_GameObjID, in_uiFlags, in_pfnCallback, in_pCookie, pParam, in_PlayingID );
	if( returnedPlayingID == AK_INVALID_PLAYING_ID )
	{
		MONITOR_ERRORMSG2( L"Failed posting event: ", in_pszEventName );

		if (in_cExternals != 0)
			temp.pExternalSrcs->Release();
	}
	return returnedPlayingID;
}
#endif //AK_SUPPORT_WCHAR

AkPlayingID PostEvent(
	const char* in_pszEventName,
	AkGameObjectID in_GameObjID,
	AkUInt32 in_uiFlags,
	AkCallbackFunc in_pfnCallback,
	void* in_pCookie,
	AkUInt32 in_cExternals, // = 0	
	AkExternalSourceInfo *in_pExternalSources, // = NULL
	AkPlayingID	in_PlayingID //= AK_INVALID_PLAYING_ID
	)
{
	AkCustomParamType temp;
	AkCustomParamType *pParam = NULL;
	if ( AK_EXPECT_FALSE( in_cExternals != 0 ) )
	{
		temp.customParam = 0;
		temp.ui32Reserved = 0;
		temp.pExternalSrcs = AkExternalSourceArray::Create(in_cExternals, in_pExternalSources);
		if (temp.pExternalSrcs == NULL)
			return AK_INVALID_PLAYING_ID ;

		pParam = &temp;
	}

	AkUniqueID EventID = GetIDFromString( in_pszEventName );
    AkPlayingID returnedPlayingID = PostEvent( EventID, in_GameObjID, in_uiFlags, in_pfnCallback, in_pCookie, pParam, in_PlayingID );
	if( returnedPlayingID == AK_INVALID_PLAYING_ID )
	{
		MONITOR_ERRORMSG2( "Failed posting event: ", in_pszEventName );
		if (in_cExternals != 0)
			temp.pExternalSrcs->Release();
	}
	return returnedPlayingID;
}

// This function Was added to allow a very specific and special way to playback something.
// An instance of the plug-in will be created
AkPlayingID PlaySourcePlugin( AkUInt32 in_plugInID, AkUInt32 in_CompanyID, AkGameObjectID in_GameObjID )
{
	AKASSERT(g_pAudioMgr);

	AkQueuedMsg Item( QueuedMsgType_SourcePluginAction );

    Item.sourcePluginAction.CustomParam.customParam = 0;
	Item.sourcePluginAction.CustomParam.ui32Reserved = 0;
	Item.sourcePluginAction.CustomParam.pExternalSrcs = 0;

    Item.sourcePluginAction.PluginID = in_plugInID;
    Item.sourcePluginAction.CompanyID = in_CompanyID;

    Item.sourcePluginAction.ActionType = AkSourcePluginActionType_Play;

	Item.sourcePluginAction.PlayingID = AKPLATFORM::AkInterlockedIncrement((AkInt32*)&g_PlayingID);
	Item.sourcePluginAction.TargetPlayingID = AK_INVALID_PLAYING_ID;
	Item.sourcePluginAction.GameObjID = in_GameObjID;
	AKRESULT eResult = g_pAudioMgr->Enqueue(Item, AkQueuedMsg::Sizeof_PlaySourcePlugin() );
	if(eResult != AK_Success)
	{
		return AK_INVALID_PLAYING_ID;
	}

	return Item.sourcePluginAction.PlayingID;

}

AKRESULT StopSourcePlugin( AkUInt32 in_plugInID, AkUInt32 in_CompanyID, AkPlayingID in_playingID )
{
    AKASSERT(g_pAudioMgr);

	AkQueuedMsg Item( QueuedMsgType_SourcePluginAction );

    Item.sourcePluginAction.CustomParam.customParam = 0;
	Item.sourcePluginAction.CustomParam.ui32Reserved = 0;
	Item.sourcePluginAction.CustomParam.pExternalSrcs = 0;

    Item.sourcePluginAction.PluginID = in_plugInID;
    Item.sourcePluginAction.CompanyID = in_CompanyID;

    Item.sourcePluginAction.ActionType = AkSourcePluginActionType_Stop;

	Item.sourcePluginAction.PlayingID = in_playingID;
    Item.sourcePluginAction.GameObjID = AK_INVALID_GAME_OBJECT;
	return g_pAudioMgr->Enqueue(Item, AkQueuedMsg::Sizeof_PlaySourcePlugin() );
}

AKRESULT ExecuteActionOnEvent(
	AkUniqueID in_eventID,
	AkActionOnEventType in_ActionType,
    AkGameObjectID in_gameObjectID, /*= AK_INVALID_GAME_OBJECT*/
	AkTimeMs in_uTransitionDuration /*= 0*/,
	AkCurveInterpolation in_eFadeCurve, /*= AkCurveInterpolation_Linear*/
	AkPlayingID in_PlayingID /*= AK_INVALID_PLAYING_ID*/
	)
{
	AKASSERT(g_pIndex);
	AkQueuedMsg Item( QueuedMsgType_EventAction );

	// This function call does get the pointer and addref it in an atomic call. 
	// Item.event.Event will be filled with NULL if it was not possible. 
	Item.eventAction.Event = g_pIndex->m_idxEvents.GetPtrAndAddRef( in_eventID ); 

	if( Item.eventAction.Event )
	{
		Item.eventAction.GameObjID = in_gameObjectID;
		Item.eventAction.eActionToExecute = in_ActionType;
		Item.eventAction.uTransitionDuration = in_uTransitionDuration;
		Item.eventAction.eFadeCurve = in_eFadeCurve;
		Item.eventAction.TargetPlayingID = in_PlayingID;
		return g_pAudioMgr->Enqueue(Item, AkQueuedMsg::Sizeof_EventAction() );
	}
	else
	{
		MONITOR_ERROR_PARAM( AK::Monitor::ErrorCode_EventIDNotFound, in_eventID, AK_INVALID_PLAYING_ID, in_gameObjectID, in_eventID, false );
		return AK_Fail;
	}
}

#ifdef AK_SUPPORT_WCHAR
AKRESULT ExecuteActionOnEvent(
	const wchar_t* in_pszEventName,
	AkActionOnEventType in_ActionType,
    AkGameObjectID in_gameObjectID, /*= AK_INVALID_GAME_OBJECT*/
	AkTimeMs in_uTransitionDuration /*= 0*/,
	AkCurveInterpolation in_eFadeCurve, /*= AkCurveInterpolation_Linear*/
	AkPlayingID in_PlayingID /*= AK_INVALID_PLAYING_ID*/
	)
{
	AkUniqueID EventID = GetIDFromString( in_pszEventName );
	return ExecuteActionOnEvent( EventID, in_ActionType, in_gameObjectID, in_uTransitionDuration, in_eFadeCurve, in_PlayingID );
}
#endif //AK_SUPPORT_WCHAR

AKRESULT ExecuteActionOnEvent(
	const char* in_pszEventName,
	AkActionOnEventType in_ActionType,
    AkGameObjectID in_gameObjectID, /*= AK_INVALID_GAME_OBJECT*/
	AkTimeMs in_uTransitionDuration /*= 0*/,
	AkCurveInterpolation in_eFadeCurve, /*= AkCurveInterpolation_Linear*/
	AkPlayingID in_PlayingID /*= AK_INVALID_PLAYING_ID*/
	)
{
	AkUniqueID EventID = GetIDFromString( in_pszEventName );
	return ExecuteActionOnEvent( EventID, in_ActionType, in_gameObjectID, in_uTransitionDuration, in_eFadeCurve, in_PlayingID );
}

AKRESULT SeekOnEvent( 
	AkUniqueID in_eventID, 
	AkGameObjectID in_gameObjectID, 
	AkTimeMs in_iPosition,
	bool in_bSeekToNearestMarker
	)
{
	AKASSERT(g_pIndex);
	AkQueuedMsg Item( QueuedMsgType_Seek );

	// This function call does get the pointer and addref it in an atomic call. 
	// Item.event.Event will be filled with NULL if it was not possible. 
	Item.seek.Event = g_pIndex->m_idxEvents.GetPtrAndAddRef( in_eventID ); 

	if( Item.seek.Event )
	{
		Item.seek.GameObjID = in_gameObjectID;
		Item.seek.bIsSeekRelativeToDuration = false;
		Item.seek.iPosition = in_iPosition;
		Item.seek.bSnapToMarker = in_bSeekToNearestMarker;
		return g_pAudioMgr->Enqueue(Item, AkQueuedMsg::Sizeof_Seek() );
	}
	else
	{
		MONITOR_ERROR_PARAM( AK::Monitor::ErrorCode_EventIDNotFound, in_eventID, AK_INVALID_PLAYING_ID, in_gameObjectID, in_eventID, false );
		return AK_Fail;
	}
}

#ifdef AK_SUPPORT_WCHAR
AKRESULT SeekOnEvent( 
	const wchar_t* in_pszEventName,
	AkGameObjectID in_gameObjectID,
	AkTimeMs in_iPosition,
	bool in_bSeekToNearestMarker
	)
{
	AkUniqueID EventID = GetIDFromString( in_pszEventName );
	return SeekOnEvent( EventID, in_gameObjectID, in_iPosition, in_bSeekToNearestMarker );
}
#endif //AK_SUPPORT_WCHAR

AKRESULT SeekOnEvent( 
	const char* in_pszEventName,
	AkGameObjectID in_gameObjectID,
	AkTimeMs in_iPosition,
	bool in_bSeekToNearestMarker
	)
{
	AkUniqueID EventID = GetIDFromString( in_pszEventName );
	return SeekOnEvent( EventID, in_gameObjectID, in_iPosition, in_bSeekToNearestMarker );
}

AKRESULT SeekOnEvent( 
	AkUniqueID in_eventID, 
	AkGameObjectID in_gameObjectID, 
	AkReal32 in_fPercent,
	bool in_bSeekToNearestMarker
	)
{
	AKASSERT(g_pIndex);
	AkQueuedMsg Item( QueuedMsgType_Seek );

	// This function call does get the pointer and addref it in an atomic call. 
	// Item.event.Event will be filled with NULL if it was not possible. 
	Item.seek.Event = g_pIndex->m_idxEvents.GetPtrAndAddRef( in_eventID ); 

	if( Item.seek.Event )
	{
		Item.seek.GameObjID = in_gameObjectID;
		Item.seek.bIsSeekRelativeToDuration	= true;
		Item.seek.fPercent	= in_fPercent;
		Item.seek.bSnapToMarker = in_bSeekToNearestMarker;
		return g_pAudioMgr->Enqueue(Item, AkQueuedMsg::Sizeof_Seek() );
	}
	else
	{
		MONITOR_ERROR_PARAM( AK::Monitor::ErrorCode_EventIDNotFound, in_eventID, AK_INVALID_PLAYING_ID, in_gameObjectID, in_eventID, false );
		return AK_Fail;
	}
}

#ifdef AK_SUPPORT_WCHAR
AKRESULT SeekOnEvent( 
	const wchar_t* in_pszEventName,
	AkGameObjectID in_gameObjectID,
	AkReal32 in_fPercent,
	bool in_bSeekToNearestMarker
	)
{
	AkUniqueID EventID = GetIDFromString( in_pszEventName );
	return SeekOnEvent( EventID, in_gameObjectID, in_fPercent, in_bSeekToNearestMarker );
}
#endif //AK_SUPPORT_WCHAR

AKRESULT SeekOnEvent( 
	const char* in_pszEventName,
	AkGameObjectID in_gameObjectID,
	AkReal32 in_fPercent,
	bool in_bSeekToNearestMarker
	)
{
	AkUniqueID EventID = GetIDFromString( in_pszEventName );
	return SeekOnEvent( EventID, in_gameObjectID, in_fPercent, in_bSeekToNearestMarker );
}

void CancelEventCallbackCookie( void* in_pCookie )
{
	if( g_pPlayingMgr )
		g_pPlayingMgr->CancelCallbackCookie( in_pCookie );
}


void CancelEventCallback( AkPlayingID in_playingID )
{
	if( g_pPlayingMgr )
		g_pPlayingMgr->CancelCallback( in_playingID );
}

AKRESULT GetSourcePlayPosition( 
	AkPlayingID		in_PlayingID,				///< PlayingID returned by PostEvent
	AkTimeMs*		out_puPosition,				///< Position (in ms) of the source associated with that PlayingID
	bool			in_bExtrapolate /*= true*/	///< Real position is extrapolated based on the difference between when this function is called and when the real sound's position was updated by the sound engine.
	)
{
	if( out_puPosition == NULL )
		return AK_InvalidParameter;

	return g_pPositionRepository->GetCurrPosition( in_PlayingID, out_puPosition, in_bExtrapolate );
}

//////////////////////////////////////////////////////////////////////////////////
// Dynamic Dialog
//////////////////////////////////////////////////////////////////////////////////
namespace DynamicDialogue
{

AkUniqueID ResolveDialogueEvent(
		AkUniqueID			in_eventID,					///< Unique ID of the dialog event
		AkArgumentValueID*	in_aArgumentValues,			///< Array of argument value IDs
		AkUInt32			in_uNumArguments,			///< Number of argument value IDs in in_aArguments
		AkPlayingID			in_idSequence				///< Optional sequence ID in which the token will be inserted (for profiling purposes)
	)
{
	CAkDialogueEvent * pDialogueEvent = g_pIndex->m_idxDialogueEvents.GetPtrAndAddRef( in_eventID );
	if ( !pDialogueEvent )
		return AK_INVALID_UNIQUE_ID;

	AkUniqueID audioNodeID = pDialogueEvent->ResolvePath( in_aArgumentValues, in_uNumArguments, in_idSequence );

	pDialogueEvent->Release();

	return audioNodeID;
}

#ifdef AK_SUPPORT_WCHAR
AkUniqueID ResolveDialogueEvent(
		const wchar_t*		in_pszEventName,			///< Name of the dialog event
		const wchar_t**		in_aArgumentValueNames,		///< Array of argument value names
		AkUInt32			in_uNumArguments,			///< Number of argument value names in in_aArguments
		AkPlayingID			in_idSequence				///< Optional sequence ID in which the token will be inserted (for profiling purposes)
	)
{
	AkUniqueID eventID = GetIDFromString( in_pszEventName );

	CAkDialogueEvent * pDialogueEvent = g_pIndex->m_idxDialogueEvents.GetPtrAndAddRef( eventID );
	if ( !pDialogueEvent )
	{
		MONITOR_ERRORMSG2( L"Unknown Dialogue Event: ", in_pszEventName );
		return AK_INVALID_UNIQUE_ID;
	}

	AkArgumentValueID * pArgumentValues = (AkArgumentValueID *) AkAlloca( in_uNumArguments * sizeof( AkArgumentValueID ) );

	AkUniqueID audioNodeID = AK_INVALID_UNIQUE_ID;

	AKRESULT eResult = pDialogueEvent->ResolveArgumentValueNames( in_aArgumentValueNames, pArgumentValues, in_uNumArguments );
	if ( eResult == AK_Success )
	{
		audioNodeID = pDialogueEvent->ResolvePath( pArgumentValues, in_uNumArguments, in_idSequence );
	}

	pDialogueEvent->Release();

	return audioNodeID;
}
#endif //AK_SUPPORT_WCHAR

AkUniqueID ResolveDialogueEvent(
		const char*			in_pszEventName,			///< Name of the dialog event
		const char**		in_aArgumentValueNames,		///< Array of argument value names
		AkUInt32			in_uNumArguments,			///< Number of argument value names in in_aArguments
		AkPlayingID			in_idSequence				///< Optional sequence ID in which the token will be inserted (for profiling purposes)
	)
{
	AkUniqueID eventID = GetIDFromString( in_pszEventName );

	CAkDialogueEvent * pDialogueEvent = g_pIndex->m_idxDialogueEvents.GetPtrAndAddRef( eventID );
	if ( !pDialogueEvent )
	{
		MONITOR_ERRORMSG2( "Unknown Dialogue Event: ", in_pszEventName );
		return AK_INVALID_UNIQUE_ID;
	}

	AkArgumentValueID * pArgumentValues = (AkArgumentValueID *) AkAlloca( in_uNumArguments * sizeof( AkArgumentValueID ) );

	AkUniqueID audioNodeID = AK_INVALID_UNIQUE_ID;

	AKRESULT eResult = pDialogueEvent->ResolveArgumentValueNames( in_aArgumentValueNames, pArgumentValues, in_uNumArguments );
	if ( eResult == AK_Success )
	{
		audioNodeID = pDialogueEvent->ResolvePath( pArgumentValues, in_uNumArguments, in_idSequence );
	}

	pDialogueEvent->Release();

	return audioNodeID;
}
} // namespace DynamicDialogue

//////////////////////////////////////////////////////////////////////////////////
// Dynamic Sequence
//////////////////////////////////////////////////////////////////////////////////
namespace DynamicSequence
{
AkPlayingID Open(
	AkGameObjectID	in_gameObjectID,
	AkUInt32		in_uiFlags /* = 0 */,
	AkCallbackFunc	in_pfnCallback /* = NULL*/,
	void* 			in_pCookie	   /* = NULL */,
	DynamicSequenceType in_eDynamicSequenceType /*= DynamicSequenceType_SampleAccurate*/
	)
{
	AKASSERT(g_pIndex);
	AKASSERT(g_pAudioMgr);
	AkQueuedMsg Item( QueuedMsgType_OpenDynamicSequence );

	Item.opendynamicsequence.PlayingID = AKPLATFORM::AkInterlockedIncrement((AkInt32*)&g_PlayingID);
	Item.opendynamicsequence.TargetPlayingID = AK_INVALID_PLAYING_ID;
	Item.opendynamicsequence.pDynamicSequence = CAkDynamicSequence::Create( Item.opendynamicsequence.PlayingID, in_eDynamicSequenceType );

	if( Item.opendynamicsequence.pDynamicSequence )
	{
		Item.opendynamicsequence.GameObjID = in_gameObjectID;
		Item.opendynamicsequence.CustomParam.customParam = 0;
		Item.opendynamicsequence.CustomParam.ui32Reserved = 0;		
		Item.opendynamicsequence.CustomParam.pExternalSrcs = 0;

		AKASSERT( g_pPlayingMgr );

		AKRESULT eResult = g_pPlayingMgr->AddPlayingID( Item.opendynamicsequence, in_pfnCallback, in_pCookie, in_uiFlags, AK_INVALID_UNIQUE_ID );
		if( eResult != AK_Success )
		{
			Item.opendynamicsequence.pDynamicSequence->Release();
			return AK_INVALID_PLAYING_ID;
		}

		eResult = g_pAudioMgr->Enqueue(Item, AkQueuedMsg::Sizeof_OpenDynamicSequence() );
		AKASSERT(eResult == AK_Success);

		return Item.opendynamicsequence.PlayingID;
	}
	else
	{
		return AK_INVALID_PLAYING_ID;
	}
}

static AKRESULT _DynamicSequenceCommand( 
	AkPlayingID in_playingID, 
	AkQueuedMsg_DynamicSequenceCmd::Command in_eCommand, 
	AkTimeMs in_uTransitionDuration,
	AkCurveInterpolation in_eFadeCurve )
{
	AKASSERT(g_pIndex);
	AKASSERT(g_pAudioMgr);
	AkQueuedMsg Item( QueuedMsgType_DynamicSequenceCmd );

	Item.dynamicsequencecmd.pDynamicSequence = g_pIndex->m_idxDynamicSequences.GetPtrAndAddRef( in_playingID ); 

	if( Item.dynamicsequencecmd.pDynamicSequence )
	{
		if ( !Item.dynamicsequencecmd.pDynamicSequence->WasClosed() )
		{
			if( in_eCommand == AkQueuedMsg_DynamicSequenceCmd::Close )
			{
				// Must do it here. to prevent another call to succeed.
				// this operation can be processed from here without locking
				Item.dynamicsequencecmd.pDynamicSequence->Close();
				
				// Then we still have to push the close command so that the release is done to free the resources.
			}

			Item.dynamicsequencecmd.eCommand = in_eCommand;
			Item.dynamicsequencecmd.uTransitionDuration = in_uTransitionDuration;
			Item.dynamicsequencecmd.eFadeCurve = in_eFadeCurve;
			return g_pAudioMgr->Enqueue(Item, AkQueuedMsg::Sizeof_DynamicSequenceCmd() );
		}

		Item.dynamicsequencecmd.pDynamicSequence->Release();

#ifndef AK_OPTIMIZED
		AkOSChar szMsg[ 64 ];
		AK_OSPRINTF( szMsg, 64, AKTEXT("Dynamic Sequence already closed: %u"), in_playingID );
		MONITOR_ERRORMSG( szMsg );
#endif
		return AK_Fail;
	}

#ifndef AK_OPTIMIZED
	AkOSChar szMsg[ 64 ];
	AK_OSPRINTF( szMsg, 64, AKTEXT("Dynamic Sequence ID not found: %u"), in_playingID );
	MONITOR_ERRORMSG( szMsg );
#endif
	return AK_Fail;
}

AKRESULT Play( AkPlayingID in_playingID, AkTimeMs in_uTransitionDuration, AkCurveInterpolation in_eFadeCurve )
{
	return _DynamicSequenceCommand( in_playingID, AkQueuedMsg_DynamicSequenceCmd::Play, in_uTransitionDuration, in_eFadeCurve );
}

AKRESULT Pause( AkPlayingID in_playingID, AkTimeMs in_uTransitionDuration, AkCurveInterpolation in_eFadeCurve )
{
	return _DynamicSequenceCommand( in_playingID, AkQueuedMsg_DynamicSequenceCmd::Pause, in_uTransitionDuration, in_eFadeCurve );
}

AKRESULT Resume( AkPlayingID in_playingID, AkTimeMs in_uTransitionDuration, AkCurveInterpolation in_eFadeCurve )
{
	return _DynamicSequenceCommand( in_playingID, AkQueuedMsg_DynamicSequenceCmd::Resume, in_uTransitionDuration, in_eFadeCurve );
}

AKRESULT Stop( AkPlayingID	in_playingID, AkTimeMs in_uTransitionDuration, AkCurveInterpolation in_eFadeCurve )
{
	return _DynamicSequenceCommand( in_playingID, AkQueuedMsg_DynamicSequenceCmd::Stop, in_uTransitionDuration, in_eFadeCurve );
}

AKRESULT Break( AkPlayingID	in_playingID )
{
	return _DynamicSequenceCommand( in_playingID, AkQueuedMsg_DynamicSequenceCmd::Break, 0, AkCurveInterpolation_Linear );
}

AKRESULT Close( AkPlayingID in_playingID  )
{
	return _DynamicSequenceCommand( in_playingID, AkQueuedMsg_DynamicSequenceCmd::Close, 0, AkCurveInterpolation_Linear );
}

Playlist * LockPlaylist(
	AkPlayingID in_playingID						///< AkPlayingID returned by DynamicSequence::Open
	)
{
	CAkDynamicSequence * pDynaSeq = g_pIndex->m_idxDynamicSequences.GetPtrAndAddRef( in_playingID ); 
	if ( !pDynaSeq )
	{
#ifndef AK_OPTIMIZED
		AkOSChar szMsg[ 64 ];
		AK_OSPRINTF( szMsg, 64, AKTEXT("Dynamic Sequence ID not found: %u"), in_playingID );
		MONITOR_ERRORMSG( szMsg );
#endif
		return NULL;
	}

	Playlist * pPlaylist = pDynaSeq->LockPlaylist();

	pDynaSeq->Release();

	return pPlaylist;
}

AKRESULT UnlockPlaylist(
	AkPlayingID in_playingID						///< AkPlayingID returned by DynamicSequence::Open
	)
{
	CAkDynamicSequence * pDynaSeq = g_pIndex->m_idxDynamicSequences.GetPtrAndAddRef( in_playingID ); 
	if ( !pDynaSeq )
	{
#ifndef AK_OPTIMIZED
		AkOSChar szMsg[ 64 ];
		AK_OSPRINTF( szMsg, 64, AKTEXT("Dynamic Sequence ID not found: %u"), in_playingID );
		MONITOR_ERRORMSG( szMsg );
#endif
		return AK_Fail;
	}

	pDynaSeq->UnlockPlaylist();

	pDynaSeq->Release();

	return AK_Success;
}

} // namespace DynamicSequence
//////////////////////////////////////////////////////////////////////////////////
// Game Objects
//////////////////////////////////////////////////////////////////////////////////
AKRESULT RegisterGameObj( AkGameObjectID in_GameObj, AkUInt32 in_uListenerMask )
{
	if ( in_GameObj == 0 || in_GameObj == AK_INVALID_GAME_OBJECT ) // omni
		return AK_InvalidParameter;

	AKASSERT(in_uListenerMask <= AK_ALL_LISTENERS_MASK);
	if (in_uListenerMask > AK_ALL_LISTENERS_MASK)
		return AK_InvalidParameter;	//Invalid listener.

	AkQueuedMsg Item( QueuedMsgType_RegisterGameObj );

	Item.reggameobj.GameObjID = in_GameObj;
	Item.reggameobj.uListenerMask = in_uListenerMask;
	Item.reggameobj.pMonitorData = NULL;

	return g_pAudioMgr->Enqueue(Item, AkQueuedMsg::Sizeof_RegisterGameObj() );
}

AKRESULT RegisterGameObj( AkGameObjectID in_GameObj, const char* in_pszObjName, AkUInt32 in_uListenerMask )
{
	if ( in_GameObj == 0 || in_GameObj == AK_INVALID_GAME_OBJECT ) // omni
		return AK_InvalidParameter;

	AKASSERT(in_uListenerMask <= AK_ALL_LISTENERS_MASK);
	if (in_uListenerMask > AK_ALL_LISTENERS_MASK)
		return AK_InvalidParameter;	//Invalid listener.

	AkQueuedMsg Item( QueuedMsgType_RegisterGameObj );

	Item.reggameobj.GameObjID = in_GameObj;
	Item.reggameobj.uListenerMask = in_uListenerMask;

#ifndef AK_OPTIMIZED
	if ( in_pszObjName )
		Item.reggameobj.pMonitorData = AkMonitor::Monitor_AllocateGameObjNameString( in_GameObj, in_pszObjName );
	else
		Item.reggameobj.pMonitorData = NULL;
#else
	Item.reggameobj.pMonitorData = NULL;
#endif

	return g_pAudioMgr->Enqueue(Item, AkQueuedMsg::Sizeof_RegisterGameObj() );
}

AKRESULT UnregisterGameObj( AkGameObjectID in_GameObj )
{
	if ( in_GameObj == 0 ) // omni
		return AK_Fail;

	AkQueuedMsg Item( QueuedMsgType_UnregisterGameObj );

	Item.unreggameobj.GameObjID = in_GameObj;

	return g_pAudioMgr->Enqueue(Item, AkQueuedMsg::Sizeof_UnregisterGameObj() );
}

AKRESULT UnregisterAllGameObj()
{
	AkQueuedMsg Item( QueuedMsgType_UnregisterGameObj );

	Item.unreggameobj.GameObjID = AK_INVALID_GAME_OBJECT;

	return g_pAudioMgr->Enqueue(Item, AkQueuedMsg::Sizeof_UnregisterGameObj() );
}

//////////////////////////////////////////////////////////////////////////////////
// Bank Management
//////////////////////////////////////////////////////////////////////////////////

void DefaultBankCallbackFunc(
                    AkBankID    /*in_bankID*/,
					const void*	/*in_pInMemoryBankPtr*/,
                    AKRESULT	in_eLoadResult,
                    AkMemPoolId in_memPoolId,
					void *		in_pCookie )
{
	AKASSERT( in_pCookie );
	AkBankSyncLoader * pReturnInfo = (AkBankSyncLoader*)in_pCookie;
	
	// Fill status info.
	pReturnInfo->m_eResult	  = in_eLoadResult;
	pReturnInfo->memPoolId    = in_memPoolId;

	pReturnInfo->Done();
}

AKRESULT ClearBanks()
{
	if( !g_pBankManager )
		return AK_Fail;

	// We must first clear prepared events prior clearing all banks.
	AKRESULT eResult = ClearPreparedEvents();

	if( eResult == AK_Success )
	{
		AkBankSyncLoader syncLoader;
		eResult = syncLoader.Init();
		if( eResult != AK_Success )
			return eResult;

		CAkBankMgr::AkBankQueueItem item;
		item.eType						 = CAkBankMgr::QueueItemClearBanks;
		item.callbackInfo.pfnBankCallback = DefaultBankCallbackFunc;
		item.callbackInfo.pCookie		 = &syncLoader;
		item.bankLoadFlag				= AkBankLoadFlag_None;

		eResult = g_pBankManager->QueueBankCommand( item );

		return syncLoader.Wait( eResult );
	}

	return eResult;
}

// Bank loading I/O settings.
AKRESULT SetBankLoadIOSettings(
    AkReal32            in_fThroughput,         // Average throughput of bank data streaming. Default is AK_DEFAULT_BANKLOAD_THROUGHPUT.
    AkPriority          in_priority             // Priority of bank streaming. Default is AK_DEFAULT_PRIORITY.
    )
{
    AKRESULT eResult = AK_Fail;

	AKASSERT( g_pBankManager );

	if( g_pBankManager )
	{
		eResult = g_pBankManager->SetBankLoadIOSettings(
            in_fThroughput,
            in_priority );
	}

	return eResult;
}

////////////////////////////////////////////////////////////////////////////
// Banks
////////////////////////////////////////////////////////////////////////////

// Load/Unload Bank Internal.
static AKRESULT LoadBankInternal(
	AkBankID            in_bankID,              // ID of the bank to load.
	AkBankLoadFlag		in_flag,
	CAkBankMgr::AkBankQueueItemType	in_eType,	// load or unload
    AkBankCallbackFunc  in_pfnBankCallback,	    // Callback function that will be called.
	void *              in_pCookie,				// Callback cookie.
	AkMemPoolId         in_memPoolId,           // Memory pool ID. Pool is created inside if AK_INVALID_POOL_ID.
	const void *		in_pInMemoryBank = NULL,	// Pointer to an in-memory bank
	AkUInt32			in_ui32InMemoryBankSize = 0 // Size of the specified in-memory bank
	)
{
    CAkBankMgr::AkBankQueueItem item;
	item.eType					= in_eType;
	item.load.BankID			= in_bankID;
    item.load.memPoolId			= in_memPoolId;
    item.callbackInfo.pfnBankCallback	= in_pfnBankCallback;
    item.callbackInfo.pCookie			= in_pCookie;
	item.load.pInMemoryBank		= in_pInMemoryBank;
	item.load.ui32InMemoryBankSize = in_ui32InMemoryBankSize;
	item.bankLoadFlag			= in_flag;

	return g_pBankManager->QueueBankCommand( item );
}

static AKRESULT PrepareEventInternal(
	PreparationType		in_PreparationType,
    AkBankCallbackFunc  in_pfnBankCallback,	// Callback function that will be called.
	void *              in_pCookie,			// Callback cookie.
	AkUniqueID*			in_pEventID,
	AkUInt32			in_uNumEvents,
	bool				in_bDoAllocAndCopy = true // When set to false, the provided array can be used as is and must be freed.
	)
{
	if( in_uNumEvents == 0 )
		return AK_InvalidParameter;

    CAkBankMgr::AkBankQueueItem item;
	item.eType						= ( in_PreparationType == Preparation_Load ) ? CAkBankMgr::QueueItemPrepareEvent : CAkBankMgr::QueueItemUnprepareEvent;
	item.bankLoadFlag				= AkBankLoadFlag_None;
	item.callbackInfo.pCookie			= in_pCookie;
	item.callbackInfo.pfnBankCallback	= in_pfnBankCallback;

	item.prepare.numEvents = in_uNumEvents;

	if( in_uNumEvents == 1 )
	{
		item.prepare.eventID = *in_pEventID;
	}
	else if( in_bDoAllocAndCopy )
	{
		item.prepare.pEventID = (AkUniqueID*)AkAlloc( g_DefaultPoolId, in_uNumEvents * sizeof( AkUniqueID ) );
		if( !item.prepare.pEventID )
		{
			return AK_InsufficientMemory;
		}
		memcpy( item.prepare.pEventID, in_pEventID, in_uNumEvents * sizeof( AkUniqueID ) );
	}
	else
	{
		item.prepare.pEventID = in_pEventID;
	}

	AKRESULT eResult = g_pBankManager->QueueBankCommand( item );

	if( eResult != AK_Success && in_uNumEvents != 1 )
	{
		AKASSERT( item.prepare.pEventID );
		AkFree( g_DefaultPoolId, item.prepare.pEventID );
	}

	return eResult;
}

static AKRESULT PrepareGameSyncsInternal(
    AkBankCallbackFunc  in_pfnBankCallback,	    // Callback function that will be called.
	void *              in_pCookie,				// Callback cookie.
	bool				in_bSupported,
	AkGroupType			in_eGroupType,
	AkUInt32			in_GroupID,
	AkUInt32*			in_pGameSyncID,
	AkUInt32			in_uNumGameSync,
	bool				in_bDoAllocAndCopy = true // When set to false, the provided array can be used as is and must be freed.
	)
{
	if( in_uNumGameSync == 0 || in_pGameSyncID == NULL )
		return AK_InvalidParameter;

	CAkBankMgr::AkBankQueueItem item;
	item.eType						= CAkBankMgr::QueueItemSupportedGameSync;
	item.bankLoadFlag				= AkBankLoadFlag_None;
    item.callbackInfo.pfnBankCallback	= in_pfnBankCallback;
    item.callbackInfo.pCookie			= in_pCookie;
	item.gameSync.bSupported		= in_bSupported;
	item.gameSync.eGroupType		= in_eGroupType;
	item.gameSync.uGroupID			= in_GroupID;

	item.gameSync.uNumGameSync = in_uNumGameSync;

	if( in_uNumGameSync == 1 )
	{
		item.gameSync.uGameSyncID = *in_pGameSyncID;
	}
	else if( in_bDoAllocAndCopy )
	{
		item.gameSync.pGameSyncID = (AkUInt32*)AkAlloc( g_DefaultPoolId, in_uNumGameSync * sizeof( AkUInt32 ) );
		if( !item.gameSync.pGameSyncID )
		{
			return AK_InsufficientMemory;
		}
		memcpy( item.gameSync.pGameSyncID, in_pGameSyncID, in_uNumGameSync * sizeof( AkUInt32 ) );
	}
	else
	{
		item.gameSync.pGameSyncID = in_pGameSyncID;
	}

	AKRESULT eResult = g_pBankManager->QueueBankCommand( item );

	if( eResult != AK_Success && in_uNumGameSync != 1 )
	{
		AKASSERT( item.gameSync.pGameSyncID );
		AkFree( g_DefaultPoolId, item.gameSync.pGameSyncID );
	}

	return eResult;
}

#ifdef AK_SUPPORT_WCHAR
void RemoveFileExtension( wchar_t* in_pstring );// propotype to avoid no prototype warning with some compilers.
void RemoveFileExtension( wchar_t* in_pstring )
{
	while( *in_pstring != 0 )
	{
		if( *in_pstring == L'.' )
		{
			*in_pstring = 0;
			return;
		}
		++in_pstring;
	}
}
#endif //AK_SUPPORT_WCHAR

static void RemoveFileExtension( char* in_pstring )
{
	while( *in_pstring != 0 )
	{
		if( *in_pstring == '.' )
		{
			*in_pstring = 0;
			return;
		}
		++in_pstring;
	}
}

static AkBankID GetBankIDFromString( const char* in_pszString )
{
	// Remove the file extension of it was provided.

	char szStringWithoutExtension[ AK_MAX_STRING_SIZE ]; 
	AKPLATFORM::SafeStrCpy( szStringWithoutExtension, in_pszString, AK_MAX_STRING_SIZE );

	RemoveFileExtension( szStringWithoutExtension );

	// Get the ID from the resulting string
	AkBankID bankID = GetIDFromString( szStringWithoutExtension );

	// Call UpdateBankName(...)
	g_pBankManager->UpdateBankName( bankID, szStringWithoutExtension );

	return bankID;
}

#ifdef AK_SUPPORT_WCHAR
static AkBankID GetBankIDFromString( const wchar_t* in_pszString )
{
	// Convert to char* so it is conform with UpdateBankName(...)
	char szString[ AK_MAX_STRING_SIZE ];
	AkWideCharToChar( in_pszString, AK_MAX_STRING_SIZE-1, szString );
	szString[ AK_MAX_STRING_SIZE-1 ] = 0;

	return GetBankIDFromString( szString );
}
#endif //AK_SUPPORT_WCHAR

#ifdef AK_SUPPORT_WCHAR
// Synchronous bank load (by string).
AKRESULT LoadBank(
	    const wchar_t*      in_pszString,		    // Name/path of the bank to load
        AkMemPoolId         in_memPoolId, 			// Memory pool ID. Pool is created inside if AK_INVALID_POOL_ID.
        AkBankID &          out_bankID				// Returned bank ID.
	    )
{
	out_bankID = GetBankIDFromString( in_pszString );

	AkBankSyncLoader syncLoader;
	AKRESULT eResult = syncLoader.Init();
	if( eResult != AK_Success )
		return eResult;

    eResult = LoadBankInternal(
	                        out_bankID,             // ID of the bank to load.
							AkBankLoadFlag_None,
                            CAkBankMgr::QueueItemLoad,
                            DefaultBankCallbackFunc,// Callback function that will be called.
							&syncLoader,			// Callback cookie.
	                        in_memPoolId            // Pool ID.
	                        );

	return syncLoader.Wait( eResult );
}
#endif //AK_SUPPORT_WCHAR

AKRESULT LoadBank(
	    const char*         in_pszString,		    // Name/path of the bank to load
        AkMemPoolId         in_memPoolId, 			// Memory pool ID. Pool is created inside if AK_INVALID_POOL_ID.
        AkBankID &          out_bankID				// Returned bank ID.
	    )
{
	out_bankID = GetBankIDFromString( in_pszString );

	AkBankSyncLoader syncLoader;
	AKRESULT eResult = syncLoader.Init();
	if( eResult != AK_Success )
		return eResult;

    eResult = LoadBankInternal(
	                        out_bankID,             // ID of the bank to load.
							AkBankLoadFlag_None,
                            CAkBankMgr::QueueItemLoad,
                            DefaultBankCallbackFunc,// Callback function that will be called.
							&syncLoader,			// Callback cookie.
	                        in_memPoolId            // Pool ID.
	                        );

	return syncLoader.Wait( eResult );
}

// Synchronous bank load (by id).
AKRESULT LoadBank(
	    AkBankID			in_bankID,              // Bank ID of the bank to load
        AkMemPoolId         in_memPoolId			// Memory pool ID. Pool is created inside if AK_INVALID_POOL_ID.
	    )
{
    AkBankSyncLoader syncLoader;
	AKRESULT eResult = syncLoader.Init();
	if( eResult != AK_Success )
		return eResult;

    eResult = LoadBankInternal(
	                        in_bankID,             // ID of the bank to load.
							AkBankLoadFlag_UsingFileID,
                            CAkBankMgr::QueueItemLoad,
                            DefaultBankCallbackFunc,// Callback function that will be called.
							&syncLoader,		    // Callback cookie.
	                        in_memPoolId,           // Pool ID.
							NULL,
							0 ); 

	return syncLoader.Wait( eResult );
}

AKRESULT CheckBankAlignment( const void* in_pMem, AkUInt32 in_uMemSize );
AKRESULT CheckBankAlignment( const void* in_pMem, AkUInt32 in_uMemSize )
{
	if( ((uintptr_t)in_pMem) % AK_BANK_PLATFORM_DATA_ALIGNMENT != 0 )
	{
		return AK_InvalidParameter;
	}

	if( in_pMem == NULL || in_uMemSize < AK_MINIMUM_BANK_SIZE )
	{
		return AK_InvalidParameter;
	}

	return AK_Success;
}

// Synchronous bank load (by in-memory Bank).
AKRESULT LoadBank(
	    const void *		in_pInMemoryBankPtr,	// Pointer to the in-memory bank to load
		AkUInt32			in_ui32InMemoryBankSize,// Size of the in-memory bank to load
        AkBankID &          out_bankID				// Returned bank ID.
	    )
{
	AKRESULT eResult = CheckBankAlignment( in_pInMemoryBankPtr, in_ui32InMemoryBankSize );
	if( eResult != AK_Success )
		return eResult;

	out_bankID = g_pBankManager->GetBankIDFromInMemorySpace( in_pInMemoryBankPtr, in_ui32InMemoryBankSize );

    AkBankSyncLoader syncLoader;
	eResult = syncLoader.Init();
	if( eResult != AK_Success )
		return eResult;

    eResult = LoadBankInternal(
	                        out_bankID,					 // ID of the bank to load.
							AkBankLoadFlag_InMemory,
                            CAkBankMgr::QueueItemLoad,
                            DefaultBankCallbackFunc,	// Callback function that will be called.
							&syncLoader,				// Callback cookie.
	                        AK_INVALID_POOL_ID,			// Pool ID unused when from memory.
							in_pInMemoryBankPtr,
							in_ui32InMemoryBankSize
	                        ); 

	return syncLoader.Wait( eResult );
}

#ifdef AK_SUPPORT_WCHAR
// Asynchronous bank load (by string).
AKRESULT LoadBank(
	    const wchar_t*      in_pszString,           // Name/path of the bank to load.
		AkBankCallbackFunc  in_pfnBankCallback,	    // Callback function.
		void *              in_pCookie,				// Callback cookie.
        AkMemPoolId         in_memPoolId,			// Memory pool ID. Pool is created inside if AK_INVALID_POOL_ID.
		AkBankID &          out_bankID				// Returned bank ID.
	    )
{
    out_bankID = GetBankIDFromString( in_pszString );
    return LoadBankInternal(		out_bankID,                 // ID of the bank to load.
									AkBankLoadFlag_None,
                                    CAkBankMgr::QueueItemLoad,	// true = load, false = unload
                                    in_pfnBankCallback,			// Callback function that will be called.
	                                in_pCookie,					// Callback cookie.
                                    in_memPoolId );				// Custom parameter. Currently not used.
}
#endif //AK_SUPPORT_WCHAR

AKRESULT LoadBank(
	    const char*         in_pszString,           // Name/path of the bank to load.
		AkBankCallbackFunc  in_pfnBankCallback,	    // Callback function.
		void *              in_pCookie,				// Callback cookie.
        AkMemPoolId         in_memPoolId,			// Memory pool ID. Pool is created inside if AK_INVALID_POOL_ID.
		AkBankID &          out_bankID				// Returned bank ID.
	    )
{
    out_bankID = GetBankIDFromString( in_pszString );
    return LoadBankInternal(		out_bankID,                 // ID of the bank to load.
									AkBankLoadFlag_None,
                                    CAkBankMgr::QueueItemLoad,	// true = load, false = unload
                                    in_pfnBankCallback,			// Callback function that will be called.
	                                in_pCookie,					// Callback cookie.
                                    in_memPoolId );				// Custom parameter. Currently not used.
}

// Asynchronous bank load (by id).
AKRESULT LoadBank(
	    AkBankID			in_bankID,              // Bank ID of the bank to load.
		AkBankCallbackFunc  in_pfnBankCallback,	    // Callback function.
		void *              in_pCookie,				// Callback cookie.
	    AkMemPoolId         in_memPoolId			// Memory pool ID. Pool is created inside if AK_INVALID_POOL_ID.
	    )
{
    return LoadBankInternal( in_bankID,             // ID of the bank to load.
							 AkBankLoadFlag_UsingFileID,
                             CAkBankMgr::QueueItemLoad,
                             in_pfnBankCallback,	 // Callback function that will be called.
	                         in_pCookie,             // Callback cookie.
                             in_memPoolId,           // Custom parameter. Currently not used.
							 NULL,
							 0
							 );
}

// Asynchronous bank load (by in-memory bank).
AKRESULT LoadBank(
	    const void *		in_pInMemoryBankPtr,	// Pointer to the in-memory bank to load
		AkUInt32			in_ui32InMemoryBankSize,// Size of the in-memory bank to load
		AkBankCallbackFunc  in_pfnBankCallback,	    // Callback function.
		void *              in_pCookie,				// Callback cookie.
		AkBankID &          out_bankID				// Returned bank ID.
	    )
{
	AKRESULT eResult = CheckBankAlignment( in_pInMemoryBankPtr, in_ui32InMemoryBankSize );
	if( eResult != AK_Success )
		return eResult;

	out_bankID = g_pBankManager->GetBankIDFromInMemorySpace( in_pInMemoryBankPtr, in_ui32InMemoryBankSize );

    return LoadBankInternal( out_bankID,			// ID of the bank to load.
							 AkBankLoadFlag_InMemory,
                             CAkBankMgr::QueueItemLoad,
                             in_pfnBankCallback,	// Callback function that will be called.
	                         in_pCookie,			// Callback cookie.
                             AK_INVALID_POOL_ID,	// Custom parameter. Currently not used.
							 in_pInMemoryBankPtr,
							 in_ui32InMemoryBankSize
							 );
}

#ifdef AK_SUPPORT_WCHAR
// Synchronous bank unload (by string).
AKRESULT UnloadBank(
	    const wchar_t*      in_pszString,				// Name/path of the bank to unload.
		const void *		in_pInMemoryBankPtr,
	    AkMemPoolId *       out_pMemPoolId /*= NULL*/	// Returned memory pool ID used with LoadBank(). Can pass NULL.
	    )
{
    AkBankID bankID = GetBankIDFromString( in_pszString );
    return UnloadBank( bankID, in_pInMemoryBankPtr, out_pMemPoolId );
}
#endif //AK_SUPPORT_WCHAR

AKRESULT UnloadBank(
	    const char*         in_pszString,				// Name/path of the bank to unload.
		const void *		in_pInMemoryBankPtr,
	    AkMemPoolId *       out_pMemPoolId /*= NULL*/	// Returned memory pool ID used with LoadBank(). Can pass NULL.
	    )
{
    AkBankID bankID = GetBankIDFromString( in_pszString );
    return UnloadBank( bankID, in_pInMemoryBankPtr, out_pMemPoolId );
}

// Synchronous bank unload (by id).
AKRESULT UnloadBank(
	    AkBankID            in_bankID,              // ID of the bank to unload.
		const void *		in_pInMemoryBankPtr,
        AkMemPoolId *       out_pMemPoolId /*= NULL*/   // Returned memory pool ID used with LoadBank(). Can pass NULL.
	    )
{
	AkBankSyncLoader syncLoader;
	AKRESULT eResult = syncLoader.Init();
	if( eResult != AK_Success )
		return eResult;

    eResult = LoadBankInternal(
	                        in_bankID,              // ID of the bank to load.
							AkBankLoadFlag_InMemory,
                            CAkBankMgr::QueueItemUnload,
                            DefaultBankCallbackFunc,// Callback function that will be called.
							&syncLoader,		    // Callback cookie.
	                        AK_INVALID_POOL_ID,      // Pool: ignored on unload.
							in_pInMemoryBankPtr
	                        );

	eResult = syncLoader.Wait( eResult );

    if ( out_pMemPoolId != NULL )
    {
		*out_pMemPoolId = syncLoader.memPoolId;
    }

	return eResult;
}

#ifdef AK_SUPPORT_WCHAR
// Asynchronous bank unload (by string).
AKRESULT UnloadBank(
	    const wchar_t*      in_pszString,           // Name/path of the bank to load.
		const void *		in_pInMemoryBankPtr,
		AkBankCallbackFunc  in_pfnBankCallback,	    // Callback function.
		void *              in_pCookie 				// Callback cookie.
	    )
{
    AkBankID bankID = GetBankIDFromString( in_pszString );

    return LoadBankInternal(bankID,                 // ID of the bank to load.
							(in_pInMemoryBankPtr) ? AkBankLoadFlag_InMemory : AkBankLoadFlag_None,
                            CAkBankMgr::QueueItemUnload,
                            in_pfnBankCallback,	    // Callback function that will be called.
                            in_pCookie,             // Callback cookie.
                            AK_INVALID_POOL_ID,      // Pool: ignored on unload.
							in_pInMemoryBankPtr
                            );
}
#endif //AK_SUPPORT_WCHAR

AKRESULT UnloadBank(
	    const char*         in_pszString,           // Name/path of the bank to load.
		const void *		in_pInMemoryBankPtr,
		AkBankCallbackFunc  in_pfnBankCallback,	    // Callback function.
		void *              in_pCookie 				// Callback cookie.
	    )
{
    AkBankID bankID = GetBankIDFromString( in_pszString );

    return LoadBankInternal(bankID,                 // ID of the bank to load.
							(in_pInMemoryBankPtr) ? AkBankLoadFlag_InMemory : AkBankLoadFlag_None,
                            CAkBankMgr::QueueItemUnload,
                            in_pfnBankCallback,	    // Callback function that will be called.
                            in_pCookie,             // Callback cookie.
                            AK_INVALID_POOL_ID,     // Pool: ignored on unload.
							in_pInMemoryBankPtr
                            );
}

// Asynchronous bank unload (by id and Memory Ptr).
AKRESULT UnloadBank(
	    AkBankID            in_bankID,              // ID of the bank to unload.
		const void *		in_pInMemoryBankPtr,
		AkBankCallbackFunc  in_pfnBankCallback,	    // Callback function.
		void *              in_pCookie 				// Callback cookie.
	    )
{
    return LoadBankInternal( in_bankID,              // ID of the bank to load.
							 (in_pInMemoryBankPtr) ? AkBankLoadFlag_InMemory : AkBankLoadFlag_None,
							 CAkBankMgr::QueueItemUnload,
                             in_pfnBankCallback,	 // Callback function that will be called.
	                         in_pCookie,             // Callback cookie.
                             AK_INVALID_POOL_ID,     // Pool: ignored on unload.
							 in_pInMemoryBankPtr
                             );
}

void CancelBankCallbackCookie( 
		void * in_pCookie
		)
{
	if( g_pBankManager )
	{
		g_pBankManager->CancelCookie( in_pCookie );
	}
}

#ifdef AK_SUPPORT_WCHAR
AKRESULT PrepareBank(
			PreparationType		in_PreparationType,					///< Preparation type ( Preparation_Load or Preparation_Unload )
			const wchar_t*      in_pszString,						///< Name of the bank to Prepare/Unprepare.
			AkBankContent		in_uFlags /*= AkBankContent_All*/	///< Media, structure or both (Events are included in structural)
			)
{
	AkBankID bankID = GetBankIDFromString( in_pszString );
	return PrepareBank( in_PreparationType, bankID, in_uFlags );
}
#endif //AK_SUPPORT_WCHAR

AKRESULT PrepareBank(
			PreparationType		in_PreparationType,					///< Preparation type ( Preparation_Load or Preparation_Unload )
			const char*         in_pszString,						///< Name of the bank to Prepare/Unprepare.
			AkBankContent		in_uFlags /*= AkBankContent_All*/	///< Media, structure or both (Events are included in structural)
			)
{
	AkBankID bankID = GetBankIDFromString( in_pszString );
	return PrepareBank( in_PreparationType, bankID, in_uFlags );
}

AKRESULT PrepareBank(
			PreparationType		in_PreparationType,					///< Preparation type ( Preparation_Load or Preparation_Unload )
			AkBankID            in_bankID,							///< ID of the bank to Prepare/Unprepare.
			AkBankContent		in_uFlags /*= AkBankContent_All*/	///< Media, structure or both (Events are included in structural)
			)
{
	AkBankSyncLoader syncLoader;
	AKRESULT eResult = syncLoader.Init();
	if( eResult != AK_Success )
		return eResult;

    eResult = PrepareBank( in_PreparationType, in_bankID, DefaultBankCallbackFunc, &syncLoader, in_uFlags );

	return syncLoader.Wait( eResult );
}

#ifdef AK_SUPPORT_WCHAR
AKRESULT PrepareBank(
			PreparationType		in_PreparationType,					///< Preparation type ( Preparation_Load or Preparation_Unload )
			const wchar_t*      in_pszString,						///< Name of the bank to Prepare/Unprepare.
			AkBankCallbackFunc	in_pfnBankCallback,					///< Callback function
			void *              in_pCookie,							///< Callback cookie (reserved to user, passed to the callback function)
			AkBankContent		in_uFlags /*= AkBankContent_All*/	///< Media, structure or both (Events are included in structural)
			)
{
	AkBankID bankID = GetBankIDFromString( in_pszString );
	return PrepareBank( in_PreparationType, bankID, in_pfnBankCallback, in_pCookie, in_uFlags );
}
#endif //AK_SUPPORT_WCHAR

AKRESULT PrepareBank(
			PreparationType		in_PreparationType,					///< Preparation type ( Preparation_Load or Preparation_Unload )
			const char*         in_pszString,						///< Name of the bank to Prepare/Unprepare.
			AkBankCallbackFunc	in_pfnBankCallback,					///< Callback function
			void *              in_pCookie,							///< Callback cookie (reserved to user, passed to the callback function)
			AkBankContent		in_uFlags /*= AkBankContent_All*/	///< Media, structure or both (Events are included in structural)
			)
{
	AkBankID bankID = GetBankIDFromString( in_pszString );
	return PrepareBank( in_PreparationType, bankID, in_pfnBankCallback, in_pCookie, in_uFlags );
}

AKRESULT PrepareBank(
			PreparationType		in_PreparationType,					///< Preparation type ( Preparation_Load or Preparation_Unload )
			AkBankID            in_bankID,							///< ID of the bank to Prepare/Unprepare.
			AkBankCallbackFunc	in_pfnBankCallback,					///< Callback function
			void *              in_pCookie,							///< Callback cookie (reserved to user, passed to the callback function)
			AkBankContent		in_uFlags /*= AkBankContent_All*/	///< Media, structure or both (Events are included in structural)
			)
{
	CAkBankMgr::AkBankQueueItem item;
	item.eType							= in_PreparationType == Preparation_Load ? CAkBankMgr::QueueItemPrepareBank : CAkBankMgr::QueueItemUnprepareBank;
	item.bankLoadFlag					= AkBankLoadFlag_None;
    item.callbackInfo.pfnBankCallback	= in_pfnBankCallback;
    item.callbackInfo.pCookie			= in_pCookie;
	item.bankPreparation.BankID = in_bankID;
	item.bankPreparation.uFlags = in_uFlags;

	return g_pBankManager->QueueBankCommand( item );
}

// Synchronous PrepareEvent (by id).
AKRESULT PrepareEvent( 
	PreparationType		in_PreparationType,		///< Preparation type ( Preparation_Load or Preparation_Unload )
	AkUniqueID*			in_pEventID,			///< Array of Event ID
	AkUInt32			in_uNumEvent			///< number of Event ID in the array
	)
{
	AkBankSyncLoader syncLoader;
	AKRESULT eResult = syncLoader.Init();
	if( eResult != AK_Success )
		return eResult;

	eResult = PrepareEventInternal( in_PreparationType, DefaultBankCallbackFunc, &syncLoader, in_pEventID, in_uNumEvent );

	return syncLoader.Wait( eResult );
}

#ifdef AK_SUPPORT_WCHAR
// Synchronous PrepareEvent (by string).
AKRESULT PrepareEvent( 
	PreparationType		in_PreparationType,		///< Preparation type ( Preparation_Load or Preparation_Unload )
	const wchar_t**		in_ppszString,			///< Array of EventName
	AkUInt32			in_uNumEvent			///< Number of Events in the array
	)
{
	AKRESULT eResult;
	switch ( in_uNumEvent )
	{
	case 0:
		eResult = AK_InvalidParameter;
		break;

	case 1:
		{
			AkUniqueID eventID = GetIDFromString( in_ppszString[0] );
			eResult = PrepareEvent( in_PreparationType, &eventID, 1 );
		}
		break;

	default:
		{
			AkUInt32* pEventIDArray = (AkUInt32*)AkAlloc( g_DefaultPoolId, in_uNumEvent * sizeof( AkUInt32 ) );
			if( pEventIDArray )
			{
				for( AkUInt32 i = 0; i < in_uNumEvent; ++i )
				{
					pEventIDArray[i] = 	GetIDFromString( in_ppszString[i] );
				}
				AkBankSyncLoader syncLoader;
				eResult = syncLoader.Init();
				if( eResult == AK_Success )
				{
					eResult = PrepareEventInternal( in_PreparationType, DefaultBankCallbackFunc, &syncLoader, pEventIDArray, in_uNumEvent, false );
					eResult = syncLoader.Wait( eResult );
				}
			}
			else
			{
				eResult = AK_InsufficientMemory;
			}
		}
		break;
	}

	return eResult;
}
#endif //AK_SUPPORT_WCHAR

AKRESULT PrepareEvent( 
	PreparationType		in_PreparationType,		///< Preparation type ( Preparation_Load or Preparation_Unload )
	const char**		in_ppszString,			///< Array of EventName
	AkUInt32			in_uNumEvent			///< Number of Events in the array
	)
{
	AKRESULT eResult;
	switch ( in_uNumEvent )
	{
	case 0:
		eResult = AK_InvalidParameter;
		break;

	case 1:
		{
			AkUniqueID eventID = GetIDFromString( in_ppszString[0] );
			eResult = PrepareEvent( in_PreparationType, &eventID, 1 );
		}
		break;

	default:
		{
			AkUInt32* pEventIDArray = (AkUInt32*)AkAlloc( g_DefaultPoolId, in_uNumEvent * sizeof( AkUInt32 ) );
			if( pEventIDArray )
			{
				for( AkUInt32 i = 0; i < in_uNumEvent; ++i )
				{
					pEventIDArray[i] = 	GetIDFromString( in_ppszString[i] );
				}
				AkBankSyncLoader syncLoader;
				eResult = syncLoader.Init();
				if( eResult == AK_Success )
				{
					eResult = PrepareEventInternal( in_PreparationType, DefaultBankCallbackFunc, &syncLoader, pEventIDArray, in_uNumEvent, false );
					eResult = syncLoader.Wait( eResult );
				}
			}
			else
			{
				eResult = AK_InsufficientMemory;
			}
		}
		break;
	}

	return eResult;
}

// Asynchronous PrepareEvent (by id).
AKRESULT PrepareEvent( 
	PreparationType		in_PreparationType,		///< Preparation type ( Preparation_Load or Preparation_Unload )
	AkUniqueID*			in_pEventID,			///< Array of Event ID
	AkUInt32			in_uNumEvent,			///< number of Event ID in the array
	AkBankCallbackFunc	in_pfnBankCallback,		///< Callback function
	void *              in_pCookie				///< Callback cookie
	)
{
	return PrepareEventInternal( in_PreparationType, in_pfnBankCallback, in_pCookie, in_pEventID, in_uNumEvent );
}

#ifdef AK_SUPPORT_WCHAR
// Asynchronous PrepareEvent (by string).
AKRESULT PrepareEvent( 
	PreparationType		in_PreparationType,		///< Preparation type ( Preparation_Load or Preparation_Unload )
	const wchar_t**		in_ppszString,			///< Array of EventName
	AkUInt32			in_uNumEvent,			///< Number of Events in the array
	AkBankCallbackFunc	in_pfnBankCallback,		///< Callback function
	void *              in_pCookie				///< Callback cookie
	)
{
	AKRESULT eResult;
	switch ( in_uNumEvent )
	{
	case 0:
		eResult = AK_InvalidParameter;
		break;

	case 1:
		{
			AkUniqueID eventID = GetIDFromString( in_ppszString[0] );
			eResult = PrepareEventInternal( in_PreparationType, in_pfnBankCallback, in_pCookie, &eventID, 1 );
		}
		break;

	default:
		{
			AkUInt32* pEventIDArray = (AkUInt32*)AkAlloc( g_DefaultPoolId, in_uNumEvent * sizeof( AkUInt32 ) );
			if( pEventIDArray )
			{
				for( AkUInt32 i = 0; i < in_uNumEvent; ++i )
				{
					pEventIDArray[i] = 	GetIDFromString( in_ppszString[i] );
				}
				eResult = PrepareEventInternal( in_PreparationType, in_pfnBankCallback, in_pCookie, pEventIDArray, in_uNumEvent, false);
			}
			else
			{
				eResult = AK_InsufficientMemory;
			}
		}
		break;
	}

	return eResult;
}
#endif //AK_SUPPORT_WCHAR

AKRESULT PrepareEvent( 
	PreparationType		in_PreparationType,		///< Preparation type ( Preparation_Load or Preparation_Unload )
	const char**		in_ppszString,			///< Array of EventName
	AkUInt32			in_uNumEvent,			///< Number of Events in the array
	AkBankCallbackFunc	in_pfnBankCallback,		///< Callback function
	void *              in_pCookie				///< Callback cookie
	)
{
	AKRESULT eResult;
	switch ( in_uNumEvent )
	{
	case 0:
		eResult = AK_InvalidParameter;
		break;

	case 1:
		{
			AkUniqueID eventID = GetIDFromString( in_ppszString[0] );
			eResult = PrepareEventInternal( in_PreparationType, in_pfnBankCallback, in_pCookie, &eventID, 1 );
		}
		break;

	default:
		{
			AkUInt32* pEventIDArray = (AkUInt32*)AkAlloc( g_DefaultPoolId, in_uNumEvent * sizeof( AkUInt32 ) );
			if( pEventIDArray )
			{
				for( AkUInt32 i = 0; i < in_uNumEvent; ++i )
				{
					pEventIDArray[i] = 	GetIDFromString( in_ppszString[i] );
				}
				eResult = PrepareEventInternal( in_PreparationType, in_pfnBankCallback, in_pCookie, pEventIDArray, in_uNumEvent, false);
			}
			else
			{
				eResult = AK_InsufficientMemory;
			}
		}
		break;
	}

	return eResult;
}

AKRESULT ClearPreparedEvents()
{
	AkBankSyncLoader syncLoader;
	AKRESULT eResult = syncLoader.Init();
	if( eResult != AK_Success )
		return eResult;

	CAkBankMgr::AkBankQueueItem item;
	item.eType						 = CAkBankMgr::QueueItemUnprepareAllEvents;
	item.bankLoadFlag				= AkBankLoadFlag_None;
    item.callbackInfo.pfnBankCallback = DefaultBankCallbackFunc;
    item.callbackInfo.pCookie		 = &syncLoader;

	eResult = g_pBankManager->QueueBankCommand( item );

	return syncLoader.Wait( eResult );
}

/// Async IDs
AKRESULT PrepareGameSyncs(
	PreparationType	in_PreparationType,			///< Preparation type ( Preparation_Load or Preparation_Unload )
	AkGroupType			in_eGameSyncType,		///< The type of game sync.
	AkUInt32			in_GroupID,				///< The state group ID or the Switch Group ID.
	AkUInt32*			in_paGameSyncID,		///< Array of ID of the gamesyncs to either support or not support.
	AkUInt32			in_uNumGameSyncs,		///< The number of game sync ID in the array.
	AkBankCallbackFunc	in_pfnBankCallback,		///< Callback function
	void *				in_pCookie				///< Callback cookie
	)
{
	return PrepareGameSyncsInternal( in_pfnBankCallback, in_pCookie, (in_PreparationType == Preparation_Load), in_eGameSyncType, in_GroupID, in_paGameSyncID, in_uNumGameSyncs );
}

#ifdef AK_SUPPORT_WCHAR
/// Async strings
AKRESULT PrepareGameSyncs(
	PreparationType	in_PreparationType,			///< Preparation type ( Preparation_Load or Preparation_Unload )
	AkGroupType			in_eGameSyncType,		///< The type of game sync.
	const wchar_t*		in_pszGroupName,		///< The state group Name or the Switch Group Name.
	const wchar_t**		in_ppszGameSyncName,	///< The specific ID of the state to either support or not support.
	AkUInt32			in_uNumGameSyncNames,	///< The number of game sync in the string array.
	AkBankCallbackFunc	in_pfnBankCallback,		///< Callback function
	void *				in_pCookie				///< Callback cookie
	)
{
	AKRESULT eResult;
	
	if( !in_ppszGameSyncName || !in_uNumGameSyncNames )
		return AK_InvalidParameter;

	AkUInt32 groupID = GetIDFromString( in_pszGroupName );

	if ( in_uNumGameSyncNames == 1 )
	{
		AkUInt32 gameSyncID = GetIDFromString( in_ppszGameSyncName[0] );

		eResult = PrepareGameSyncsInternal( in_pfnBankCallback, in_pCookie, (in_PreparationType == Preparation_Load), in_eGameSyncType, groupID, &gameSyncID, 1 );
	}
	else
	{
		AkUInt32* pGameSyncIDArray = (AkUInt32*)AkAlloc( g_DefaultPoolId, in_uNumGameSyncNames * sizeof( AkUInt32 ) );
		if( pGameSyncIDArray )
		{
			for( AkUInt32 i = 0; i < in_uNumGameSyncNames; ++i )
			{
				pGameSyncIDArray[i] = 	GetIDFromString( in_ppszGameSyncName[i] );
			}
			eResult = PrepareGameSyncsInternal( in_pfnBankCallback, in_pCookie, (in_PreparationType == Preparation_Load), in_eGameSyncType, groupID, pGameSyncIDArray, in_uNumGameSyncNames, false );
		}
		else
		{
			eResult = AK_InsufficientMemory;
		}
	}

	return eResult;
}
#endif //AK_SUPPORT_WCHAR

AKRESULT PrepareGameSyncs(
	PreparationType	in_PreparationType,			///< Preparation type ( Preparation_Load or Preparation_Unload )
	AkGroupType			in_eGameSyncType,		///< The type of game sync.
	const char*			in_pszGroupName,		///< The state group Name or the Switch Group Name.
	const char**		in_ppszGameSyncName,	///< The specific ID of the state to either support or not support.
	AkUInt32			in_uNumGameSyncNames,	///< The number of game sync in the string array.
	AkBankCallbackFunc	in_pfnBankCallback,		///< Callback function
	void *				in_pCookie				///< Callback cookie
	)
{
	AKRESULT eResult;
	
	if( !in_ppszGameSyncName || !in_uNumGameSyncNames )
		return AK_InvalidParameter;

	AkUInt32 groupID = GetIDFromString( in_pszGroupName );

	if ( in_uNumGameSyncNames == 1 )
	{
		AkUInt32 gameSyncID = GetIDFromString( in_ppszGameSyncName[0] );

		eResult = PrepareGameSyncsInternal( in_pfnBankCallback, in_pCookie, (in_PreparationType == Preparation_Load), in_eGameSyncType, groupID, &gameSyncID, 1 );
	}
	else
	{
		AkUInt32* pGameSyncIDArray = (AkUInt32*)AkAlloc( g_DefaultPoolId, in_uNumGameSyncNames * sizeof( AkUInt32 ) );
		if( pGameSyncIDArray )
		{
			for( AkUInt32 i = 0; i < in_uNumGameSyncNames; ++i )
			{
				pGameSyncIDArray[i] = 	GetIDFromString( in_ppszGameSyncName[i] );
			}
			eResult = PrepareGameSyncsInternal( in_pfnBankCallback, in_pCookie, (in_PreparationType == Preparation_Load), in_eGameSyncType, groupID, pGameSyncIDArray, in_uNumGameSyncNames, false );
		}
		else
		{
			eResult = AK_InsufficientMemory;
		}
	}

	return eResult;
}

/// Sync IDs
AKRESULT PrepareGameSyncs(
	PreparationType	in_PreparationType,			///< Preparation type ( Preparation_Load or Preparation_Unload )
	AkGroupType		in_eGameSyncType,			///< The type of game sync.
	AkUInt32		in_GroupID,					///< The state group ID or the Switch Group ID.
	AkUInt32*		in_paGameSyncID,			///< Array of ID of the gamesyncs to either support or not support.
	AkUInt32		in_uNumGameSyncs			///< The number of game sync ID in the array.
	)
{
	AkBankSyncLoader syncLoader;
	AKRESULT eResult = syncLoader.Init();
	if( eResult != AK_Success )
		return eResult;

	eResult = PrepareGameSyncsInternal( DefaultBankCallbackFunc, &syncLoader, (in_PreparationType == Preparation_Load), in_eGameSyncType, in_GroupID, in_paGameSyncID, in_uNumGameSyncs );

	return syncLoader.Wait( eResult );
}

#ifdef AK_SUPPORT_WCHAR
/// Sync strings
AKRESULT PrepareGameSyncs(
	PreparationType	in_PreparationType,			///< Preparation type ( Preparation_Load or Preparation_Unload )
	AkGroupType		in_eGameSyncType,			///< The type of game sync.
	const wchar_t*	in_pszGroupName,			///< The state group Name or the Switch Group Name.
	const wchar_t**	in_ppszGameSyncName,		///< The specific ID of the state to either support or not support.
	AkUInt32		in_uNumGameSyncNames		///< The number of game sync in the string array.
	)
{
	AKRESULT eResult;
	
	if( !in_ppszGameSyncName || !in_uNumGameSyncNames )
		return AK_InvalidParameter;

	AkUInt32 groupID = GetIDFromString( in_pszGroupName );

	if ( in_uNumGameSyncNames == 1 )
	{
		AkUInt32 gameSyncID = GetIDFromString( in_ppszGameSyncName[0] );

		eResult = PrepareGameSyncs( in_PreparationType, in_eGameSyncType, groupID, &gameSyncID, 1 );
	}
	else
	{
		AkUInt32* pGameSyncIDArray = (AkUInt32*)AkAlloc( g_DefaultPoolId, in_uNumGameSyncNames * sizeof( AkUInt32 ) );
		if( pGameSyncIDArray )
		{
			for( AkUInt32 i = 0; i < in_uNumGameSyncNames; ++i )
			{
				pGameSyncIDArray[i] = 	GetIDFromString( in_ppszGameSyncName[i] );
			}

			AkBankSyncLoader syncLoader;
			eResult = syncLoader.Init();
			if( eResult == AK_Success )
			{
				eResult = PrepareGameSyncsInternal( DefaultBankCallbackFunc, &syncLoader, (in_PreparationType == Preparation_Load), in_eGameSyncType, groupID, pGameSyncIDArray, in_uNumGameSyncNames, false );
				eResult = syncLoader.Wait( eResult );
			}
		}
		else
		{
			eResult = AK_InsufficientMemory;
		}
	}

	return eResult;
}
#endif //AK_SUPPORT_WCHAR

AKRESULT PrepareGameSyncs(
	PreparationType	in_PreparationType,			///< Preparation type ( Preparation_Load or Preparation_Unload )
	AkGroupType		in_eGameSyncType,			///< The type of game sync.
	const char*		in_pszGroupName,			///< The state group Name or the Switch Group Name.
	const char**	in_ppszGameSyncName,		///< The specific ID of the state to either support or not support.
	AkUInt32		in_uNumGameSyncNames		///< The number of game sync in the string array.
	)
{
	AKRESULT eResult;
	
	if( !in_ppszGameSyncName || !in_uNumGameSyncNames )
		return AK_InvalidParameter;

	AkUInt32 groupID = GetIDFromString( in_pszGroupName );

	if ( in_uNumGameSyncNames == 1 )
	{
		AkUInt32 gameSyncID = GetIDFromString( in_ppszGameSyncName[0] );

		eResult = PrepareGameSyncs( in_PreparationType, in_eGameSyncType, groupID, &gameSyncID, 1 );
	}
	else
	{
		AkUInt32* pGameSyncIDArray = (AkUInt32*)AkAlloc( g_DefaultPoolId, in_uNumGameSyncNames * sizeof( AkUInt32 ) );
		if( pGameSyncIDArray )
		{
			for( AkUInt32 i = 0; i < in_uNumGameSyncNames; ++i )
			{
				pGameSyncIDArray[i] = 	GetIDFromString( in_ppszGameSyncName[i] );
			}

			AkBankSyncLoader syncLoader;
			eResult = syncLoader.Init();
			if( eResult == AK_Success )
			{
				eResult = PrepareGameSyncsInternal( DefaultBankCallbackFunc, &syncLoader, (in_PreparationType == Preparation_Load), in_eGameSyncType, groupID, pGameSyncIDArray, in_uNumGameSyncNames, false );
				eResult = syncLoader.Wait( eResult );
			}
		}
		else
		{
			eResult = AK_InsufficientMemory;
		}
	}

	return eResult;
}

////////////////////////////////////////////////////////////////////////////
// Banks new API END.
////////////////////////////////////////////////////////////////////////////

#ifndef PROXYCENTRAL_CONNECTED
#ifdef AK_SUPPORT_WCHAR
AKRESULT LoadMediaFileSync( AkUniqueID in_MediaID, const wchar_t* in_szFileName, LoadMediaFile_ActionType in_eActionType )
{
	AkBankSyncLoader syncLoader;
	AKRESULT eResult = syncLoader.Init();
	if( eResult != AK_Success )
		return eResult;

    CAkBankMgr::AkBankQueueItem item;
	switch( in_eActionType )
	{
	case LoadMediaFile_Load:
		item.eType = CAkBankMgr::QueueItemLoadMediaFile;
		break;
	case LoadMediaFile_Unload:
		item.eType = CAkBankMgr::QueueItemUnloadMediaFile;
		break;
	case LoadMediaFile_Swap:
		item.eType = CAkBankMgr::QueueItemLoadMediaFileSwap;
		break;
	}

	item.bankLoadFlag						= AkBankLoadFlag_None;
	item.callbackInfo.pCookie				= &syncLoader;
	item.callbackInfo.pfnBankCallback		= DefaultBankCallbackFunc;
	item.loadMediaFile.MediaID				= in_MediaID;

	if( in_eActionType == LoadMediaFile_Unload )
	{
		// No string attached, simplier.
		item.loadMediaFile.pszAllocatedFileName = NULL;
		eResult = g_pBankManager->QueueBankCommand( item );
	}
	else
	{
		AkUInt32 stringSize = (AkUInt32)wcslen( in_szFileName );
		item.loadMediaFile.pszAllocatedFileName = (char*)AkAlloc( g_DefaultPoolId, (stringSize+1) * sizeof( char ) );
		if( item.loadMediaFile.pszAllocatedFileName )
		{
			AkWideCharToChar( in_szFileName, stringSize, item.loadMediaFile.pszAllocatedFileName );
			item.loadMediaFile.pszAllocatedFileName[ stringSize ] = 0;

			eResult = g_pBankManager->QueueBankCommand( item );
			if( eResult != AK_Success )
			{
				AkFree( g_DefaultPoolId, item.loadMediaFile.pszAllocatedFileName );
				item.loadMediaFile.pszAllocatedFileName = NULL;
			}
		}
		else
		{
			eResult = AK_InsufficientMemory;
		}
	}

	return syncLoader.Wait( eResult );
}
#endif //AK_SUPPORT_WCHAR
#endif //PROXYCENTRAL_CONNECTED

#ifdef AK_SUPPORT_WCHAR
AKRESULT SetBusEffect( 
			const wchar_t* in_pszBusName, 
			AkUInt32 in_uFXIndex,
			AkUniqueID in_shareSetID
		                )
{
	AkTriggerID l_busID = GetIDFromString( in_pszBusName );

	if( l_busID != AK_INVALID_UNIQUE_ID  )
	{
		return SetBusEffect( l_busID, in_uFXIndex, in_shareSetID );
	}
	else
	{
		return AK_IDNotFound;
	}
}
#endif //AK_SUPPORT_WCHAR

AKRESULT SetBusEffect( 
			const char* in_pszBusName, 
			AkUInt32 in_uFXIndex,
			AkUniqueID in_shareSetID
		                )
{
	AkTriggerID l_busID = GetIDFromString( in_pszBusName );

	if( l_busID != AK_INVALID_UNIQUE_ID  )
	{
		return SetBusEffect( l_busID, in_uFXIndex, in_shareSetID );
	}
	else
	{
		return AK_IDNotFound;
	}
}

AKRESULT SetBusEffect( 
	AkUniqueID in_audioNodeID,
	AkUInt32 in_uFXIndex,
	AkUniqueID in_shareSetID )
{
	AKASSERT( in_audioNodeID != AK_INVALID_UNIQUE_ID );
	AKASSERT( in_uFXIndex < AK_NUM_EFFECTS_PER_OBJ );

	AkQueuedMsg Item( QueuedMsgType_SetEffect );

	Item.setEffect.audioNodeID = in_audioNodeID;
	Item.setEffect.uFXIndex = in_uFXIndex;
	Item.setEffect.shareSetID = in_shareSetID;
	Item.setEffect.eNodeType = AkNodeType_Bus;

	return g_pAudioMgr->Enqueue(Item, AkQueuedMsg::Sizeof_SetEffect() );
}

AKRESULT SetActorMixerEffect( 
	AkUniqueID in_audioNodeID,
	AkUInt32 in_uFXIndex,
	AkUniqueID in_shareSetID )
{
	AKASSERT( in_audioNodeID != AK_INVALID_UNIQUE_ID );
	AKASSERT( in_uFXIndex < AK_NUM_EFFECTS_PER_OBJ );

	AkQueuedMsg Item( QueuedMsgType_SetEffect );

	Item.setEffect.audioNodeID = in_audioNodeID;
	Item.setEffect.uFXIndex = in_uFXIndex;
	Item.setEffect.shareSetID = in_shareSetID;
	Item.setEffect.eNodeType = AkNodeType_Default;

	return g_pAudioMgr->Enqueue(Item, AkQueuedMsg::Sizeof_SetEffect() );
}

////////////////////////////////////////////////////////////////////////////
// Behavioral extensions registration
////////////////////////////////////////////////////////////////////////////
AKRESULT AddBehavioralExtension( 
    AkGlobalCallbackFunc in_pCallback
    )
{
	return g_aBehavioralExtensions.AddLast( in_pCallback ) ? AK_Success : AK_InsufficientMemory;
}

AKRESULT RemoveBehavioralExtension( 
    AkGlobalCallbackFunc in_pCallback
    )
{
	return g_aBehavioralExtensions.Remove( in_pCallback );
}

AKRESULT RegisterGlobalCallback( AkGlobalCallbackFunc in_pCallback )
{
	CAkFunctionCritical GlobalLock;
	return AddBehavioralExtension( in_pCallback );
}

AKRESULT UnregisterGlobalCallback( AkGlobalCallbackFunc in_pCallback )
{
	CAkFunctionCritical GlobalLock;
	return RemoveBehavioralExtension( in_pCallback );
}

void AddExternalStateHandler( 
    AkExternalStateHandlerCallback in_pCallback
    )
{
    g_pExternalStateHandler = in_pCallback;
}
void AddExternalBankHandler(
	AkExternalBankHandlerCallback in_pCallback
	)
{
	g_pExternalBankHandlerCallback = in_pCallback;
}
void AddExternalProfileHandler(
	AkExternalProfileHandlerCallback in_pCallback
	)
{
	g_pExternalProfileHandlerCallback = in_pCallback;
}

///////////////////////////////////////////////////////////////////////////
// Output Capture
///////////////////////////////////////////////////////////////////////////

AKRESULT StartOutputCapture( const AkOSChar* in_CaptureFileName )
{
#if defined(AK_WII)
	if( !AK_PERF_OFFLINE_RENDERING )
		return AK_NotCompatible;
#endif

	if (in_CaptureFileName == NULL)
		return AK_InvalidParameter;

	AkQueuedMsg Item( QueuedMsgType_StartStopOutputCapture );

	size_t uStrLen = AKPLATFORM::OsStrLen(in_CaptureFileName) + 1;
	Item.outputCapture.szFileName = (AkOSChar*)AkAlloc( g_DefaultPoolId, uStrLen * sizeof(AkOSChar) );
	if (Item.outputCapture.szFileName == NULL)
		return AK_InsufficientMemory;

	memcpy( (void*)Item.outputCapture.szFileName, (void*)in_CaptureFileName, uStrLen * sizeof(AkOSChar));

	Item.outputCapture.bCaptureMotion = AK_PERF_OFFLINE_RENDERING;

	AKRESULT eResult = g_pAudioMgr->Enqueue(Item, AkQueuedMsg::Sizeof_StartStopCapture() );
	if (eResult != AK_Success)
		AkFree(g_DefaultPoolId, Item.outputCapture.szFileName);

	return eResult;
}

AKRESULT StopOutputCapture()
{
#if defined(AK_WII)
	if( !AK_PERF_OFFLINE_RENDERING )
		return AK_NotCompatible;
#endif
	AkQueuedMsg Item( QueuedMsgType_StartStopOutputCapture );
	Item.outputCapture.szFileName = NULL;	//Setting no file name signals the end.
	return  g_pAudioMgr->Enqueue(Item, AkQueuedMsg::Sizeof_StartStopCapture() );
}

AKRESULT StartProfilerCapture( 
	const AkOSChar* in_CaptureFileName				///< Name of the output profiler file
	)
{
#ifdef AK_OPTIMIZED
	return AK_NotCompatible;
#else
	if (in_CaptureFileName == NULL)
		return AK_InvalidParameter;

	return AkMonitor::Get()->StartProfilerCapture( in_CaptureFileName );
#endif
}

AKRESULT StopProfilerCapture()
{
#ifdef AK_OPTIMIZED
	return AK_NotCompatible;
#else
	if (AkMonitor::Get())
		return AkMonitor::Get()->StopProfilerCapture();

	return AK_Success;
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// BELOW ARE GENERAL, BUT ****INTERNAL**** FUNCTIONS
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

AKRESULT CreateDefaultMemPools()
{
    // The Memory Pool(s) should, of course, be created before
	// any other that will need memory.
    if ( g_DefaultPoolId == AK_INVALID_POOL_ID )
    {
	    // default pool
		g_DefaultPoolId = AK::MemoryMgr::CreatePool( 
			NULL, 
			g_settings.uDefaultPoolSize > DEFAULT_POOL_BLOCK_SIZE ? g_settings.uDefaultPoolSize : DEFAULT_POOL_SIZE,
			DEFAULT_POOL_BLOCK_SIZE,
			UENGINE_DEFAULT_ALLOCATION_TYPE // use same settings as lengine pool (for PrepareEvent media)
			);

		AkFXMemAlloc::GetUpper()->SetPoolId(g_DefaultPoolId);
		AK_SETPOOLNAME(g_DefaultPoolId,AKTEXT("Default"));
    }

    if ( g_DefaultPoolId == AK_INVALID_POOL_ID )
    {
        return AK_InsufficientMemory;
    }

	return AK_Success;
}

void DestroyDefaultMemPools()
{
	if(g_DefaultPoolId != AK_INVALID_POOL_ID)
    {
	    AKVERIFY( AK::MemoryMgr::DestroyPool(g_DefaultPoolId) == AK_Success );
	    g_DefaultPoolId = AK_INVALID_POOL_ID;
    }
}

//====================================================================================================
// Internal INITS
//====================================================================================================
AKRESULT PreInit( AkInitSettings * io_pSettings )
{
	// make sure you get rid of things in the reverse order
	// you created them

	AKRESULT eResult = AK_Success;

	#define CHECK_PREINIT_FAILURE {if( eResult != AK_Success ) goto preinit_failure;}

	#define CHECK_ALLOCATION_PREINIT_FAILURE( _IN_PTR_ ) {if( _IN_PTR_ == NULL ) {eResult = AK_InsufficientMemory; goto preinit_failure;}}

	//Initialise the timers for performance measurement.
	AK_INIT_TIMERS();
	AK_PERF_INIT();

#ifndef AK_OPTIMIZED
	CAkParameterNodeBase::ResetMonitoringMuteSolo();

	AkMonitor * pMonitor = AkMonitor::Instance(); 
	CHECK_ALLOCATION_PREINIT_FAILURE( pMonitor );

	pMonitor->StartMonitoring();
#endif

	//IMPORTANT : g_pIndex MUST STAY SECOND CREATION OF AKINIT()!!!!!!!!!!!!!!!!!!
	if(!g_pIndex)
	{
		g_pIndex = AkNew( g_DefaultPoolId, CAkAudioLibIndex() );
		CHECK_ALLOCATION_PREINIT_FAILURE( g_pIndex );

		g_pIndex->Init();
	}

	CHECK_PREINIT_FAILURE;

	if(!g_pRTPCMgr)
	{
		g_pRTPCMgr = AkNew( g_DefaultPoolId, CAkRTPCMgr() );
		CHECK_ALLOCATION_PREINIT_FAILURE( g_pRTPCMgr );

		eResult = g_pRTPCMgr->Init();
		CHECK_PREINIT_FAILURE;
	}

	if(!g_pEnvironmentMgr)
	{
		g_pEnvironmentMgr = AkNew( g_DefaultPoolId, CAkEnvironmentsMgr() );
		CHECK_ALLOCATION_PREINIT_FAILURE( g_pEnvironmentMgr );

		eResult = g_pEnvironmentMgr->Init();
		CHECK_PREINIT_FAILURE;
	}

	if(!g_pBankManager)
	{
		g_pBankManager = AkNew( g_DefaultPoolId, CAkBankMgr() );
		CHECK_ALLOCATION_PREINIT_FAILURE( g_pBankManager );

		eResult = g_pBankManager->Init();
		CHECK_PREINIT_FAILURE;
	}

	if(!g_pPlayingMgr)
	{
		g_pPlayingMgr = AkNew( g_DefaultPoolId, CAkPlayingMgr() );
		CHECK_ALLOCATION_PREINIT_FAILURE( g_pPlayingMgr );

		eResult = g_pPlayingMgr->Init();
		CHECK_PREINIT_FAILURE;
	}

	if(!g_pPositionRepository)
	{
		g_pPositionRepository = AkNew( g_DefaultPoolId, CAkPositionRepository() );
		CHECK_ALLOCATION_PREINIT_FAILURE( g_pPositionRepository );

		eResult = g_pPositionRepository->Init();
		CHECK_PREINIT_FAILURE;
	}

	if(!g_pRegistryMgr)
	{
		g_pRegistryMgr = AkNew( g_DefaultPoolId, CAkRegistryMgr() );
		CHECK_ALLOCATION_PREINIT_FAILURE( g_pRegistryMgr );

		eResult = g_pRegistryMgr->Init();
		CHECK_PREINIT_FAILURE;
	}

	// this one has to be ready before StateMgr is
	if(!g_pTransitionManager)
	{
		g_pTransitionManager = AkNew( g_DefaultPoolId, CAkTransitionManager() );
		CHECK_ALLOCATION_PREINIT_FAILURE( g_pTransitionManager );

		eResult = g_pTransitionManager->Init( g_settings.uMaxNumTransitions );
		CHECK_PREINIT_FAILURE;
	}

	// this one needs math and transitions to be ready
	if(!g_pPathManager)
	{
		g_pPathManager = AkNew( g_DefaultPoolId, CAkPathManager() );
		CHECK_ALLOCATION_PREINIT_FAILURE( g_pPathManager );

		eResult = g_pPathManager->Init( g_settings.uMaxNumPaths );
		CHECK_PREINIT_FAILURE;
	}

	if(!g_pStateMgr)
	{
		g_pStateMgr = AkNew( g_DefaultPoolId, CAkStateMgr() );
		CHECK_ALLOCATION_PREINIT_FAILURE( g_pStateMgr );

		eResult = g_pStateMgr->Init();
		CHECK_PREINIT_FAILURE;
	}

#if defined AK_WII_FAMILY
	CAkWiimoteMgr::Init();
#endif

preinit_failure:

    // Update client settings with (possibly) modified values.
    if ( io_pSettings )
        *io_pSettings = g_settings;

    return eResult;
}

AKRESULT InitRenderer()
{
	AKRESULT eResult = CAkURenderer::Init();

	if (!g_pAudioMgr && eResult == AK_Success )
	{
		g_pAudioMgr = AkNew(g_DefaultPoolId,CAkAudioMgr());
		if ( g_pAudioMgr )
		{
			eResult = g_pAudioMgr->Init();
			if( eResult == AK_Success )
			{
				eResult = g_pAudioMgr->Start();
			}
		}
		else
		{
			eResult = AK_InsufficientMemory;
		}
	}
	return eResult;
}

void StopAll( AkGameObjectID in_gameObjectID /*= AK_INVALID_GAME_OBJECT*/ )
{
	AkQueuedMsg Item( QueuedMsgType_StopAll );

	Item.stopAll.GameObjID = in_gameObjectID;

	g_pAudioMgr->Enqueue(Item, AkQueuedMsg::Sizeof_StopAll() );
}

void StopPlayingID( 
				   AkPlayingID in_playingID,
				   AkTimeMs in_uTransitionDuration /*= 0*/,					
				   AkCurveInterpolation in_eFadeCurve /*= AkCurveInterpolation_Linear*/
				   )
{
	// WG-20735, must handle invalid ID.
	if( in_playingID == AK_INVALID_PLAYING_ID )
		return;

	AKASSERT(g_pIndex);
	AKASSERT(g_pAudioMgr);

	AkQueuedMsg Item( QueuedMsgType_StopPlayingID );

	Item.stopEvent.playingID = in_playingID;
	Item.stopEvent.uTransitionDuration = in_uTransitionDuration;
	Item.stopEvent.eFadeCurve = in_eFadeCurve;
	g_pAudioMgr->Enqueue(Item, AkQueuedMsg::Sizeof_StopPlayingID() );
}

#ifndef AK_OPTIMIZED

CAkIndexable* GetIndexable( AkUniqueID in_IndexableID, AkIndexType in_eIndexType )
{
	CAkIndexable* pReturnedIndexable = NULL;

	//Ensure the Index Was initialized
	AKASSERT(g_pIndex);

	switch(in_eIndexType)
	{
	case AkIdxType_AudioNode:
		pReturnedIndexable = g_pIndex->GetNodePtrAndAddRef(in_IndexableID, AkNodeType_Default);
		break;
	case AkIdxType_BusNode:
		pReturnedIndexable = g_pIndex->GetNodePtrAndAddRef(in_IndexableID, AkNodeType_Bus);
		break;
	case AkIdxType_Action:
		pReturnedIndexable = g_pIndex->m_idxActions.GetPtrAndAddRef(in_IndexableID);
		break;
	case AkIdxType_Event:
		pReturnedIndexable = g_pIndex->m_idxEvents.GetPtrAndAddRef(in_IndexableID);
		break;
	case AkIdxType_DialogueEvent:
		pReturnedIndexable = g_pIndex->m_idxDialogueEvents.GetPtrAndAddRef(in_IndexableID);
		break;
	case AkIdxType_Layer:
		pReturnedIndexable = g_pIndex->m_idxLayers.GetPtrAndAddRef(in_IndexableID);
		break;
	case AkIdxType_Attenuation:
		pReturnedIndexable = g_pIndex->m_idxAttenuations.GetPtrAndAddRef(in_IndexableID);
		break;
	case AkIdxType_DynamicSequence:
		pReturnedIndexable = g_pIndex->m_idxDynamicSequences.GetPtrAndAddRef(in_IndexableID);
		break;
	case AkIdxType_FxShareSet:
		pReturnedIndexable = g_pIndex->m_idxFxShareSets.GetPtrAndAddRef(in_IndexableID);
		break;
	case AkIdxType_FxCustom:
		pReturnedIndexable = g_pIndex->m_idxFxCustom.GetPtrAndAddRef(in_IndexableID);
		break;
	case AkIdxType_CustomState:
		pReturnedIndexable = g_pIndex->m_idxCustomStates.GetPtrAndAddRef( in_IndexableID );
		break;
	default:
		AKASSERT(!"Invalid Index Type");
		break;
	}

	return pReturnedIndexable;
}

#endif // AK_OPTIMIZED

#ifdef AK_WIIU
namespace Wii
{
	void SendMainOutputToDevice(AkOutputSelector in_eDevice)
	{
		AkQueuedMsg Item( QueuedMsgType_SendMainOutputToDevice );
		Item.mainOutputDevice.eDevice = in_eDevice;
		g_pAudioMgr->Enqueue(Item, AkQueuedMsg::Sizeof_SendMainOutputToDevice() );	
	}

	void SetProcessModeFadeCallback(void* in_pEvent )
	{
		AKPLATFORM::AkSignalEvent(*(AkEvent*)in_pEvent);
	}

	void SetProcessMode(ProcUIStatus in_eStatus, AkTimeMs in_msFade, AkCurveInterpolation in_eCurve, void (*in_pCallback)(void*), void* in_pCookie)
	{	
		if (in_eStatus == PROCUI_STATUS_BACKGROUND)
		{
			if (in_pCallback)
				in_pCallback(in_pCookie);
			return;	//Should not be called, but still nothing to do.
		}

		AkQueuedMsg Item( QueuedMsgType_SetProcessMode );

		Item.processMode.eStatus = (AkUInt32)in_eStatus;
		Item.processMode.msFade = in_msFade;
		Item.processMode.eCurve = in_eCurve;
		Item.processMode.Callback = in_pCallback;
		Item.processMode.pUserData = in_pCookie;
		
		g_pAudioMgr->Enqueue(Item, AkQueuedMsg::Sizeof_SetProcessMode() );		
	}

	void SetProcessModeBlocking(ProcUIStatus in_eStatus, AkTimeMs in_msFade, AkCurveInterpolation in_eCurve)
	{	
		if (in_eStatus == PROCUI_STATUS_BACKGROUND)
			return;	//Should not be called, but still nothing to do.

		AkEvent eEvent;
		AKPLATFORM::AkCreateEvent(eEvent);
		SetProcessMode(in_eStatus, in_msFade, in_eCurve, SetProcessModeFadeCallback, &eEvent);
		
		RenderAudio();
		AKPLATFORM::AkWaitForEvent(eEvent);		
		AKPLATFORM::AkDestroyEvent(eEvent);
	}
}
#endif

AKRESULT AddSecondaryOutput(AkUInt32 in_iPlayerID, AkSinkType in_iDeviceType, AkUInt32 in_uListeners)
{	
	if (in_iDeviceType == AkSink_Main || AK_MAKE_DEVICE_KEY(in_iDeviceType, in_iPlayerID) == AK_MAIN_OUTPUT_DEVICE || in_iDeviceType >= AkSink_NumSinkTypes)
		return AK_InvalidParameter;	//Can't add the main device.

	AKASSERT(in_uListeners <= AK_ALL_LISTENERS_MASK);
	if (in_uListeners > AK_ALL_LISTENERS_MASK)
		return AK_InvalidParameter;	//Invalid listener.

#if defined AK_WIN
	if (in_iDeviceType != AkSink_MergeToMain && in_iDeviceType != AkSink_Dummy)
		return AK_InvalidParameter;
#elif defined AK_XBOXONE
	if (in_iDeviceType == AkSink_Main_Wasapi || in_iDeviceType == AkSink_Main_XAudio2 || in_iPlayerID != 0)
		return AK_InvalidParameter;
#elif defined AK_WIIU
	if (in_iPlayerID > 3 || (in_iDeviceType == AkSink_DRC && in_iPlayerID != 0))
		return AK_InvalidParameter;
#elif defined AK_PS4
	if ((in_iDeviceType == AkSink_BGM || in_iDeviceType == AkSink_BGM_NonRecordable || in_iDeviceType == AkSink_Main_NonRecordable) && in_iPlayerID != 0)
		return AK_InvalidParameter;
#endif

#if defined AK_WIN || defined AK_XBOXONE || defined AK_WIIU || defined AK_PS4

	AKASSERT(g_pAudioMgr);
	AkQueuedMsg Item( QueuedMsgType_AddRemovePlayerDevice );

	Item.playerdevice.iPlayer = in_iPlayerID;
	Item.playerdevice.uListeners = in_uListeners;
	Item.playerdevice.idDevice = in_iDeviceType;
	Item.playerdevice.pDevice = NULL;
	Item.playerdevice.bAdd = true;

	return g_pAudioMgr->Enqueue(Item, AkQueuedMsg::Sizeof_AddRemovePlayerDevice() );
#else
	return AK_NotImplemented;	//Not implemented on this platform.
#endif
}

AKRESULT RemoveSecondaryOutput(AkUInt32 in_iPlayerID, AkSinkType in_iDeviceType)
{
	if (AK_MAKE_DEVICE_KEY(in_iDeviceType, in_iPlayerID) == AK_MAIN_OUTPUT_DEVICE)
		return AK_Fail;	//Can't remove the main device.

	AKASSERT(g_pAudioMgr);
	AkQueuedMsg Item( QueuedMsgType_AddRemovePlayerDevice );

	Item.playerdevice.iPlayer = in_iPlayerID;
	Item.playerdevice.idDevice = in_iDeviceType;
	Item.playerdevice.pDevice = NULL;
	Item.playerdevice.bAdd = false;

	return g_pAudioMgr->Enqueue(Item, AkQueuedMsg::Sizeof_AddRemovePlayerDevice() );
}

AKRESULT SetSecondaryOutputVolume(AkUInt32 in_iPlayerID, AkSinkType in_iDeviceType, AkReal32 in_fVolume )
{
	AKASSERT(g_pAudioMgr);

	if (AK_MAKE_DEVICE_KEY(in_iDeviceType, in_iPlayerID) == AK_MAIN_OUTPUT_DEVICE)
		return AK_Fail;	//Can't set the main device volume.

#ifdef CHECK_SOUND_ENGINE_INPUT_VALID
	if ( !(AkMath::IsValidFloatInput( in_fVolume ) ) )
		MONITOR_ERRORMSG( AKTEXT("AK::SoundEngine::SetSecondaryOutputVolume : Invalid Float in in_fVolume") );
#endif

	AkQueuedMsg Item( QueuedMsgType_SetSecondaryOutputVolume );

	Item.secondaryoutputvolume.iPlayer = in_iPlayerID;
	Item.secondaryoutputvolume.idDevice = in_iDeviceType;
	Item.secondaryoutputvolume.fVolume = in_fVolume;

	return g_pAudioMgr->Enqueue(Item, AkQueuedMsg::Sizeof_SetSecondaryOutputVolume() );
}

} // namespace SoundEngine

#ifdef AK_MOTION
namespace MotionEngine
{
	AKRESULT AddPlayerMotionDevice(AkUInt8 in_iPlayerID, AkUInt32 in_iCompanyID, AkUInt32 in_iDevicePluginID, void * in_pDevice /* = NULL */)
	{
#ifdef AK_WIIU
		if (in_pDevice == AK_MOTION_DRC_DEVICE)
		{
			//DRC, players 0-1 are reserved for DRC.
			if (in_iPlayerID >= 2)
				return AK_Fail;
		}
		else
		{
			//Wiimotes, player 2 to 5 allowed.
			if (in_iPlayerID < 2 || in_iPlayerID > 5)
				return AK_Fail;
		}
#else
		if (in_iPlayerID > 3)
			return AK_Fail;
#endif

		AKASSERT(g_pAudioMgr);
		AkQueuedMsg Item( QueuedMsgType_AddRemovePlayerDevice );

		Item.playerdevice.iPlayer = in_iPlayerID;
		Item.playerdevice.idDevice = (AkUInt16)in_iDevicePluginID;
		Item.playerdevice.pDevice = in_pDevice;
		Item.playerdevice.bAdd = true;

		return g_pAudioMgr->Enqueue(Item, AkQueuedMsg::Sizeof_AddRemovePlayerDevice() );
	}

	void RemovePlayerMotionDevice(AkUInt8 in_iPlayerID, AkUInt32 in_iCompanyID, AkUInt32 in_iDevicePluginID)
	{
#ifdef AK_WIIU
		if (in_iPlayerID > 5)
			return;
#else
		if (in_iPlayerID > 3)
			return;
#endif
		AKASSERT(g_pAudioMgr);
		AkQueuedMsg Item( QueuedMsgType_AddRemovePlayerDevice );

		Item.playerdevice.iPlayer = in_iPlayerID;
		Item.playerdevice.idDevice = (AkUInt16)in_iDevicePluginID;
		Item.playerdevice.pDevice = NULL;
		Item.playerdevice.bAdd = false;
		g_pAudioMgr->Enqueue(Item, AkQueuedMsg::Sizeof_AddRemovePlayerDevice() );
	}

	void RegisterMotionDevice(AkUInt32 in_ulCompanyID, AkUInt32 in_ulPluginID, AkCreatePluginCallback	in_pCreateFunc)
	{
		CAkEffectsMgr::RegisterFeedbackBus(in_ulCompanyID, in_ulPluginID, in_pCreateFunc);
	}

	void SetPlayerListener(AkUInt8 in_iPlayerID, AkUInt8 in_iListener)
	{
		AKASSERT(g_pAudioMgr);

		AkQueuedMsg Item( QueuedMsgType_SetPlayerListener );

		Item.playerlistener.iPlayer = in_iPlayerID;
		Item.playerlistener.iListener = in_iListener;

		g_pAudioMgr->Enqueue(Item, AkQueuedMsg::Sizeof_SetPlayerListener() );
	}

	void SetPlayerVolume(AkUInt8 in_iPlayerID, AkReal32 in_fVolume)
	{
		AkQueuedMsg Item( QueuedMsgType_SetPlayerVolume );

		Item.playervolume.iPlayer = in_iPlayerID;
		Item.playervolume.fVolume = in_fVolume;

		g_pAudioMgr->Enqueue(Item, AkQueuedMsg::Sizeof_SetPlayerVolume() );
	}

} // namespace MotionEngine
#else // AK_MOTION
namespace MotionEngine
{
	AKRESULT AddPlayerMotionDevice(AkUInt8, AkUInt32, AkUInt32, void * /* = NULL */)
	{
		return AK_NotCompatible;
	}
	void RemovePlayerMotionDevice(AkUInt8, AkUInt32, AkUInt32)
	{
	}
	void RegisterMotionDevice(AkUInt32, AkUInt32, AkCreatePluginCallback)
	{
	}
	void SetPlayerListener(AkUInt8, AkUInt8)
	{
	}
	void SetPlayerVolume(AkUInt8, AkReal32)
	{
	}
} // namespace MotionEngine
#endif // AK_MOTION

namespace Monitor
{

AKRESULT PostCode( 
	ErrorCode in_eErrorCode,
	ErrorLevel in_eErrorLevel )
{
#ifndef AK_OPTIMIZED
	if ( AkMonitor::Get() )
		AkMonitor::Monitor_PostCode( in_eErrorCode, in_eErrorLevel );

	return AK_Success;
#else
	return AK_NotCompatible;
#endif
}

#ifdef AK_SUPPORT_WCHAR
AKRESULT PostString( 
	const wchar_t* in_pszError,
	ErrorLevel in_eErrorLevel
	)
{
#ifndef AK_OPTIMIZED
	if ( AkMonitor::Get() )
		AkMonitor::Monitor_PostString( in_pszError, in_eErrorLevel );

	return AK_Success;
#else
	return AK_NotCompatible;
#endif
}
#endif //AK_SUPPORT_WCHAR

AKRESULT PostString( 
	const char* in_pszError,
	ErrorLevel in_eErrorLevel
	)
{
#ifndef AK_OPTIMIZED
	if ( AkMonitor::Get() )
		AkMonitor::Monitor_PostString( in_pszError, in_eErrorLevel );

	return AK_Success;
#else
	return AK_NotCompatible;
#endif
}

AKRESULT SetLocalOutput(
	AkUInt32 in_uErrorLevel,
	LocalOutputFunc in_pMonitorFunc
	)
{
#ifndef AK_OPTIMIZED
	AkMonitor::SetLocalOutput( in_uErrorLevel, in_pMonitorFunc );
	return AK_Success;
#else
	return AK_NotCompatible;
#endif
}

AkTimeMs GetTimeStamp()
{
#ifndef AK_OPTIMIZED
	return AkMonitor::GetThreadTime();
#else
	return 0;
#endif
}

} // namespace Monitor

} // namespace AK


AkExternalSourceArray* AkExternalSourceArray::Create(AkUInt32 in_nCount, AkExternalSourceInfo* in_pSrcs)
{
	AkUInt32 iSize = (in_nCount - 1/*One is already in the struct*/) * sizeof(AkExternalSourceInfo) + sizeof(AkExternalSourceArray);
	AkExternalSourceArray* pArray = (AkExternalSourceArray *)AkAlloc(g_DefaultPoolId, iSize);
	if (AK_EXPECT_FALSE(pArray == NULL))
		return NULL;

	pArray->m_cRefCount = 1;
	pArray->m_nCount = in_nCount;

	for(AkUInt32 i = 0; i < in_nCount; ++i)
	{
		AkExternalSourceInfo& rInfo = in_pSrcs[i];
		AkExternalSourceInfo& rCopy = pArray->m_pSrcs[i];

		rCopy = rInfo;

		if (rInfo.szFile != NULL)
		{
			//Copy the paths. 
			AkUInt32 len = (AkUInt32)(AKPLATFORM::OsStrLen(rInfo.szFile) + 1) * sizeof(AkOSChar);
			rCopy.szFile = (AkOSChar*)AkAlloc(g_DefaultPoolId, len);
			if (rCopy.szFile != NULL)
			{
				memcpy((void*)rCopy.szFile, rInfo.szFile, len);
			}
			else
			{
				//Free everything allocated up to now
				pArray->m_nCount = i;
				pArray->Release();
				return NULL; 
			}
		}
	}

	return pArray;
}

void AkExternalSourceArray::Release()
{
	m_cRefCount--;
	if (m_cRefCount == 0)
	{
		for(AkUInt32 i = 0; i < m_nCount; i++)
		{
			if (m_pSrcs[i].szFile != NULL) 
				AkFree(g_DefaultPoolId, (void*)m_pSrcs[i].szFile);
		}

		AkFree(g_DefaultPoolId, (void*)this);		
	}
}
