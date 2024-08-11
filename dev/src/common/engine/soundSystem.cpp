/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "soundSystem.h"
#include "soundFileLoader.h"
#include "scriptSoundSystem.h"
#include "localizationManager.h"
#include "soundStartData.h"
#include "soundAdditionalListener.h"
#include "soundAmbientAreaComponent.h"
#include "mesh.h"
#include "game.h"
#include "meshTypeComponent.h"
#include "../physics/physicsWrapper.h"
#include "soundListener.h"
#include "world.h"
#include "entity.h"
#include "../core/2darray.h"
#include "../core/depot.h"
#include "../core/scriptStackFrame.h"
#include "../core/configFileManager.h"
#include "../core/scriptingSystem.h"
#include "soundSettings.h"
#include "../core/configVar.h"
#include "renderCommands.h"
#include "scaleformConfig.h" // for the hack function

#include "../physics/physicsWorldPhysxImplBatchTrace.h"

#pragma warning( disable: 4505 )

#include "animatedComponent.h"
#include "../core/taskRunner.h"
#include "../core/taskDispatcher.h"

#ifdef USE_WWISE
#include <AK/SoundEngine/Common/AkTypes.h>
#include <AK/SoundEngine/Common/AkQueryParameters.h>
#include <AK/SoundEngine/Common/AkMemoryMgr.h>		// Memory Manager
#include <AK/SoundEngine/Common/AkModule.h>			// Default memory and stream managers
#include <AK/SoundEngine/Common/IAkStreamMgr.h>		// Streaming Manager
#include <AK/SoundEngine/Common/AkSoundEngine.h>    // Sound engine
#include <AK/MusicEngine/Common/AkMusicEngine.h>	// Music Engine
#include <AK/Tools/Common/AkMonitorError.h>
#include <AK/SoundEngine/Common/AkStreamMgrModule.h>	// AkStreamMgrModule
#include <AK/Plugin/AllPluginsRegistrationHelpers.h>
#include <AK/Comm/AkCommunication.h>			
#include "../core/loadingProfiler.h"
#include "../core/gameSave.h"
#ifdef RED_PLATFORM_WINPC
#include <combaseapi.h>
#endif

#include "globalWater.h"

using namespace AK;
 
#ifdef RED_PLATFORM_WIN64
	#define LIB_EXT ".lib"
	#ifdef _DEBUG
		#define WWISE_LIB_PATH "..\\..\\..\\external\\wwise\\lib\\x64_vc110\\Debug(StaticCRT)\\lib\\"
	#elif defined(RED_FINAL_BUILD) && !defined(LOG_IN_FINAL)
		#define WWISE_LIB_PATH "..\\..\\..\\external\\wwise\\lib\\x64_vc110\\Release(StaticCRT)\\lib\\"
	#else
		#define WWISE_LIB_PATH "..\\..\\..\\external\\wwise\\lib\\x64_vc110\\Profile(StaticCRT)\\lib\\"
	#endif
#elif defined( RED_PLATFORM_DURANGO )
	#define LIB_EXT ".lib"
	#pragma comment(lib, "xaudio2" )
	#pragma comment(lib, "MMDevApi.lib" )
	#ifdef _DEBUG
		#define WWISE_LIB_PATH "..\\..\\..\\external\\wwise\\lib\\XboxOne_vc110\\Debug\\lib\\"
	#elif defined(RED_FINAL_BUILD) && !defined(LOG_IN_FINAL)
		#define WWISE_LIB_PATH "..\\..\\..\\external\\wwise\\lib\\XboxOne_vc110\\Release\\lib\\"
	#else
		#define WWISE_LIB_PATH "..\\..\\..\\external\\wwise\\lib\\XboxOne_vc110\\Profile\\lib\\"
	#endif
#elif defined( RED_PLATFORM_ORBIS )
	#define LIB_EXT ".a"
	#ifdef _DEBUG
		#define WWISE_LIB_PATH "..\\..\\..\\external\\wwise\\lib\\PS4\\Debug\\lib\\"
	#elif defined(RED_FINAL_BUILD) && !defined(LOG_IN_FINAL)
		#define WWISE_LIB_PATH "..\\..\\..\\external\\wwise\\lib\\PS4\\Release\\lib\\"
	#else
		#define WWISE_LIB_PATH "..\\..\\..\\external\\wwise\\lib\\PS4\\Profile\\lib\\"
	#endif
#else
	#define LIB_EXT ".lib"
	#ifdef _DEBUG
		#define WWISE_LIB_PATH "..\\..\\..\\external\\wwise\\lib\\Win32_vc110\\Debug(StaticCRT)\\lib\\"
	#elif defined(RED_FINAL_BUILD) && !defined(LOG_IN_FINAL)
		#define WWISE_LIB_PATH "..\\..\\..\\external\\wwise\\lib\\Win32_vc110\\Release(StaticCRT)\\lib\\"
	#else
		#define WWISE_LIB_PATH "..\\..\\..\\external\\wwise\\lib\\Win32_vc110\\Profile(StaticCRT)\\lib\\"
	#endif
#endif

#pragma comment(lib, WWISE_LIB_PATH "AkSoundEngine" LIB_EXT )
#pragma comment(lib, WWISE_LIB_PATH "AkMemoryMgr" LIB_EXT )
#pragma comment(lib, WWISE_LIB_PATH "AkStreamMgr" LIB_EXT )
#pragma comment(lib, WWISE_LIB_PATH "AkMusicEngine" LIB_EXT )
#pragma comment(lib, WWISE_LIB_PATH "AkVorbisDecoder" LIB_EXT )
#pragma comment(lib, WWISE_LIB_PATH "AkAudioInputSource" LIB_EXT )
#pragma comment(lib, WWISE_LIB_PATH "AkToneSource" LIB_EXT )
#pragma comment(lib, WWISE_LIB_PATH "AkSilenceSource" LIB_EXT )
#pragma comment(lib, WWISE_LIB_PATH "AkSineSource" LIB_EXT )
#pragma comment(lib, WWISE_LIB_PATH "AkMatrixReverbFX" LIB_EXT )
#pragma comment(lib, WWISE_LIB_PATH "AkMeterFX" LIB_EXT )
#pragma comment(lib, WWISE_LIB_PATH "AkParametricEQFX" LIB_EXT )
#pragma comment(lib, WWISE_LIB_PATH "AkGainFX" LIB_EXT )
#pragma comment(lib, WWISE_LIB_PATH "AkDelayFX" LIB_EXT )
#pragma comment(lib, WWISE_LIB_PATH "AkCompressorFX" LIB_EXT )
#pragma comment(lib, WWISE_LIB_PATH "AkExpanderFX" LIB_EXT )
#pragma comment(lib, WWISE_LIB_PATH "AkPeakLimiterFX" LIB_EXT )
#pragma comment(lib, WWISE_LIB_PATH "AkRoomVerbFX" LIB_EXT )
#pragma comment(lib, WWISE_LIB_PATH "AkGuitarDistortionFX" LIB_EXT )
#pragma comment(lib, WWISE_LIB_PATH "AkFlangerFX" LIB_EXT )
#pragma comment(lib, WWISE_LIB_PATH "AkStereoDelayFX" LIB_EXT )
#pragma comment(lib, WWISE_LIB_PATH "AkTimeStretchFX" LIB_EXT )
#pragma comment(lib, WWISE_LIB_PATH "AkPitchShifterFX" LIB_EXT )
#pragma comment(lib, WWISE_LIB_PATH "AkTremoloFX" LIB_EXT )
#pragma comment(lib, WWISE_LIB_PATH "AkHarmonizerFX" LIB_EXT )
#pragma comment(lib, WWISE_LIB_PATH "AkRumble" LIB_EXT )
//#pragma comment(lib, WWISE_LIB_PATH "AkMP3Source" LIB_EXT )
#pragma comment(lib, WWISE_LIB_PATH "AkMotionGenerator" LIB_EXT )
/*#pragma comment(lib, WWISE_LIB_PATH "AkSoundSeedWind" LIB_EXT )
#pragma comment(lib, WWISE_LIB_PATH "AkSoundSeedWoosh" LIB_EXT )
#pragma comment(lib, WWISE_LIB_PATH "AkSoundSeedImpactFX" LIB_EXT )
#pragma comment(lib, WWISE_LIB_PATH "McDSPFutzBoxFX" LIB_EXT )
#pragma comment(lib, WWISE_LIB_PATH "McDSPLimiterFX" LIB_EXT )*/
#if !defined(RED_FINAL_BUILD) || defined(LOG_IN_FINAL)
	#pragma comment(lib, WWISE_LIB_PATH "CommunicationCentral" LIB_EXT )
#endif
#ifndef RED_PLATFORM_CONSOLE
/*#pragma comment(lib, WWISE_LIB_PATH "iZHybridReverbFX")
#pragma comment(lib, WWISE_LIB_PATH "iZTrashBoxModelerFX")
#pragma comment(lib, WWISE_LIB_PATH "iZTrashDelayFX")
#pragma comment(lib, WWISE_LIB_PATH "iZTrashDistortionFX")
#pragma comment(lib, WWISE_LIB_PATH "iZTrashDynamicsFX")
#pragma comment(lib, WWISE_LIB_PATH "iZTrashFiltersFX")
#pragma comment(lib, WWISE_LIB_PATH "iZTrashMultibandDistortionFX")*/
#endif
//////////////////////////////////////////////////////////////////////////

#ifndef NO_LOG
RED_DEFINE_NAME( Sound )
#endif

Vector ToVector( const AkVector& akVector )
{
	return Vector( akVector.X, akVector.Z, akVector.Y );
}

AkVector ToAk( const Vector& vector )
{
	AkVector akVector = { vector.X, vector.Z, vector.Y };
	return akVector;
}
//////////////////////////////////////////////////////////////////////////

#endif

IMPLEMENT_RTTI_ENUM( ESoundType )
IMPLEMENT_RTTI_ENUM( ESoundTypeFlag )

CSoundSystem* GSoundSystem = NULL;

const AnsiChar* CSoundSystem::PARAM_SURFACE = "ground_materials\0";


Bool			CSoundSystem::m_initialized = false;
#ifdef RED_PLATFORM_WINPC
Bool			CSoundSystem::m_initializedCom = false;
#endif
CSoundFileLoader CSoundSystem::sm_fileLoader;

CGatheredResource resSoundEventConversion( TXT("gameplay\\globals\\soundevents_to_switch_conversion.csv"), RGF_Startup );

namespace Config
{
	TConfigVar<Float, Validation::FloatRange<0,100,1>> cvMasterVolume( "Audio", "MasterVolume", 100.0f, eConsoleVarFlag_Save );
	TConfigVar<Float, Validation::FloatRange<0,100,1>> cvMusicVolume( "Audio", "MusicVolume", 100.0f, eConsoleVarFlag_Save );
	TConfigVar<Float, Validation::FloatRange<0,100,1>> cvSoundsVolume( "Audio", "SoundVolume", 100.0f, eConsoleVarFlag_Save );
	TConfigVar<Float, Validation::FloatRange<0,100,1>> cvVoiceoversVolume( "Audio", "VoiceoversVolume", 100.0f, eConsoleVarFlag_Save );
}

CTickSoundEmittersTask::CTickSoundEmittersTask( Float dt )
	: m_delta( dt )
{

}

CTickSoundEmittersTask::~CTickSoundEmittersTask()
{

}

void CTickSoundEmittersTask::Run()
{
	GSoundSystem->PreTick( m_delta );
}

void CTickSoundEmittersTask::FinalizeSoundEmittersTick()
{	
	CTaskDispatcher taskDispatcher( *GTaskManager );
	CTaskRunner taskRunner;
	taskRunner.RunTask( *this, taskDispatcher );

	while( !this->IsFinished() ){RED_BUSY_WAIT();}
}


#ifndef NO_DEBUG_PAGES

const Char* CTickSoundEmittersTask::GetDebugName() const
{
	return TXT( "CTickSoundEmittersTask" );
}

Uint32 CTickSoundEmittersTask::GetDebugColor() const
{
	return Color::LIGHT_BLUE.ToUint32();
}

#endif // NO_DEBUG_PAGES

CSoundSystem::CSoundSystem()
	: m_ambientManager()
	, m_soundCurrentVolume( 100.0f )
	, m_musicCurrentVolume( 100.0f )
	, m_masterCurrentVolume( 100.0f )
	, m_voiceoversCurrentVolume( 100.0f )
	, m_cameraIsFree( false )
	, m_cameraPosition( Vector::ZERO_3D_POINT )
	, m_controllerPosition( Vector::ZEROS )
	, m_up( Vector::ZERO_3D_POINT )
	, m_forward( Vector::ZERO_3D_POINT )
	, m_manuallListner( false )
	, m_isBlackscreen( false )
	, m_callOnBlackscreenEnd( false )
	, m_SoundListener( nullptr )
	, m_PanningListener( nullptr )
	, m_delta( 0.1f )
	, m_MusicBlockedCount(0)
	, m_underwaterSpeechMutingCount(0)
	, m_soundEmittersTask( nullptr )
	, m_muteSpeechUnderWater( false )
{
	GUserProfileManager->RegisterListener( &CSoundSystem::OnUserEvent, this );
	m_savedSoundEvents.Clear();
}

CSoundSystem::~CSoundSystem()
{
	if( m_scriptWrapper )
	{
		m_scriptWrapper->RemoveFromRootSet();
		m_scriptWrapper->Discard();
	}
}

#define DEFAULT_POOL_SIZE 15*1024*1024
#define ENGINE_DEFAULT_POOL_SIZE 5*1024*1024
#define APU_DEFAULT_POOL_SIZE_CACHED 240 * 1024 * 1024
#define APU_DEFAULT_POOL_SIZE_NONCACHED 1 * 1024 * 1024

#ifdef USE_WWISE
namespace AK
{
	void * AllocHook( size_t in_size )
	{
		return RED_MEMORY_ALLOCATE( MemoryPool_Audio, MC_WWise, in_size );
	}
	void FreeHook( void * in_ptr )
	{
		RED_MEMORY_FREE( MemoryPool_Audio, MC_WWise, in_ptr );
	}
#ifndef RED_PLATFORM_ORBIS
	void * VirtualAllocHook( void * in_pMemAddress, size_t in_size, unsigned long in_dwAllocationType, unsigned long in_dwProtect )
	{
		return VirtualAlloc( in_pMemAddress, in_size, in_dwAllocationType, in_dwProtect );
	}
	void VirtualFreeHook( void * in_pMemAddress, size_t in_size, unsigned long in_dwFreeType )
	{
		VirtualFree( in_pMemAddress, in_size, in_dwFreeType );
	}
#endif
#ifdef RED_PLATFORM_DURANGO
	#include <apu.h>
	#include <mmdeviceapi.h>
	UINT32 SHAPE_XMA_INPUT_BUFFER_SIZE_ALIGNMENT = 2048;

	BOOL FixBlockAlign(DWORD* pBYTES,DWORD BLOCKALIGN,DWORD* pChange)
	{
		if(pBYTES && (BLOCKALIGN > 1))
		{
			DWORD bytes = (*pBYTES);
			(*pBYTES) /=  BLOCKALIGN;
			(*pBYTES) *=  BLOCKALIGN;
			if(pChange)
			{
				(*pChange) = bytes - (*pBYTES);
				if((*pChange) > 0)
				{
					return TRUE;
				}
			}
		}
		return FALSE;
	}

	void * APUAllocHook( size_t in_size, unsigned int in_alignment )
	{
		DWORD dwsize = (DWORD)in_size;
		DWORD change = 0;
		if(FixBlockAlign((DWORD*)&in_size,SHAPE_XMA_INPUT_BUFFER_SIZE_ALIGNMENT,&change))
		{  
			dwsize += change;
		}

		void* ptr = 0;
		Uint32 ptr2 = 0;
		HRESULT result = ApuAlloc(&ptr, &ptr2,dwsize,in_alignment);
		return ptr;
	}
	void APUFreeHook( void * in_pMemAddress )
	{
		if(!in_pMemAddress) return;
		ApuFree(in_pMemAddress);
	}
#endif
}
#endif

void TempAssertHook( const char * in_pszExpression,	const char * in_pszFileName, int in_lineNumber )
{
	int a = 0;
}

#if defined(USE_WWISE)
void OutputFunc( AK::Monitor::ErrorCode in_eErrorCode, const AkOSChar* in_pszError, AK::Monitor::ErrorLevel in_eErrorLevel, AkPlayingID in_playingID, AkGameObjectID in_gameObjID )
{
#ifdef RED_PLATFORM_ORBIS
	return;
#endif
	if( in_gameObjID != AK_INVALID_GAME_OBJECT )
	{
#ifdef RED_PLATFORM_ORBIS
		RED_LOG_ERROR( WWise_GameObject, TXT( "%s at %i game object" ), ANSI_TO_UNICODE( in_pszError ), in_gameObjID );
#else
		RED_LOG_ERROR( WWise_GameObject, TXT( "%s at %i game object" ), in_pszError, in_gameObjID );
#endif
	}
	else
	{
#ifdef RED_PLATFORM_ORBIS
		RED_LOG_ERROR( WWise_Global, ANSI_TO_UNICODE( in_pszError ) );
#else
		RED_LOG_ERROR( WWise_Global, in_pszError );
#endif
	}
}
#endif

void CSoundSystem::Init( const String& rootPath )
{
	if (m_initialized)
		return;

#ifdef USE_WWISE
	AkMemSettings memSettings;
	AkStreamMgrSettings stmSettings;
	AkInitSettings initSettings;
	AkPlatformInitSettings platformInitSettings;
	AkMusicSettings musicInit;

	memSettings.uMaxNumPools = 512;
	AK::StreamMgr::GetDefaultSettings( stmSettings );

	stmSettings.uMemorySize = 1024 * 512; 

	AK::SoundEngine::GetDefaultInitSettings( initSettings );
	initSettings.uDefaultPoolSize = DEFAULT_POOL_SIZE;
	initSettings.uCommandQueueSize = ENGINE_DEFAULT_POOL_SIZE;  
	initSettings.pfnAssertHook = TempAssertHook;

	AK::SoundEngine::GetDefaultPlatformInitSettings( platformInitSettings );
	platformInitSettings.uLEngineDefaultPoolSize = ENGINE_DEFAULT_POOL_SIZE;
#ifdef RED_PLATFORM_DURANGO
	// #tbd: set to the same affinity as tasks. May be overkill.
	DWORD audioAffinityMask = 1 << 2 | 1 << 3 | 1 << 4 | 1 << 5;
	
	// The bank manager is causing synch CSoundFileLoaderHook::Open to be called
	// #tbd: set it to I/O cores #5 and #6 too or expand it to there as well?
	DWORD ioAffinityMask = 1 << 6;

	platformInitSettings.uShapeDefaultPoolSize = 2 * 1024 * 1024;
	platformInitSettings.threadLEngine.dwAffinityMask = audioAffinityMask;
	platformInitSettings.threadLEngine.nPriority = THREAD_PRIORITY_ABOVE_NORMAL;
	platformInitSettings.threadBankManager.dwAffinityMask = ioAffinityMask;
	platformInitSettings.threadBankManager.nPriority = THREAD_PRIORITY_HIGHEST;
	platformInitSettings.threadMonitor.dwAffinityMask = audioAffinityMask;
	HRESULT result = ApuCreateHeap( APU_DEFAULT_POOL_SIZE_CACHED, APU_DEFAULT_POOL_SIZE_NONCACHED );
	platformInitSettings.uMaxXMAVoices = 128;

	// Call the System API to move the audio DLL's onto cores 4 and 5
	SetWasapiThreadAffinityMask( audioAffinityMask );
#elif defined( RED_PLATFORM_ORBIS )
	platformInitSettings.threadLEngine.dwAffinityMask = 1 << 0 | 1 << 1;
	platformInitSettings.threadLEngine.nPriority = SCE_KERNEL_PRIO_FIFO_HIGHEST + 5;
	platformInitSettings.threadBankManager.dwAffinityMask = 1 << 0 | 1 << 1;
	platformInitSettings.threadBankManager.nPriority = SCE_KERNEL_PRIO_FIFO_HIGHEST + 20; // Go easy on bumpers at startup. Don't need highest priority on this core.
	platformInitSettings.threadMonitor.dwAffinityMask = 1 << 0 | 1 << 1;
#endif
	
#if !defined(RED_FINAL_BUILD) || defined(LOG_IN_FINAL)
	if( NULL != Red::System::StringSearch( SGetCommandLine(), TXT("-nosound") ) )
	{
		initSettings.uDefaultPoolSize = 1024 * 128;
		platformInitSettings.uLEngineDefaultPoolSize = 1024 * 128;
	}
#endif	

	AK::MusicEngine::GetDefaultInitSettings( musicInit );

	AK::Monitor::SetLocalOutput( AK::Monitor::ErrorLevel_Error, OutputFunc );  

	AKRESULT res = AK::MemoryMgr::Init( &memSettings );
	if ( res != AK_Success )
	{
		return;
	}

	if ( !AK::StreamMgr::Create( stmSettings ) )
	{
		return;
	}

#ifdef RED_PLATFORM_WINPC
	HRESULT hr;
	m_initializedCom = false;
	hr = CoInitializeEx( nullptr, COINIT_APARTMENTTHREADED );
	if ( FAILED(hr) )
	{
		hr = CoInitializeEx( nullptr, COINIT_MULTITHREADED );
	}
	if( SUCCEEDED(hr) )
	{
		m_initializedCom = true;
	}
#endif

	res = AK::SoundEngine::Init( &initSettings, &platformInitSettings );
	if ( res != AK_Success )
	{
		return;
	}

	res = AK::MusicEngine::Init( &musicInit );
	if ( res != AK_Success )
	{
		return;
	}

#if !defined(RED_FINAL_BUILD) || defined(LOG_IN_FINAL)
	AkCommSettings commSettings;
	AK::Comm::GetDefaultInitSettings( commSettings );
	commSettings.ports.uDiscoveryBroadcast = 24024; 
	commSettings.ports.uCommand = 24025; 
	commSettings.ports.uNotification = 24026; 
	res = AK::Comm::Init( commSettings );
#endif
	
	res = AK::SoundEngine::RegisterAllBuiltInPlugins();
	if ( res != AK_Success )
	{
		return;
	}

	m_initialized = true;
#endif

	sm_fileLoader.PreInit( rootPath );
}

void CSoundSystem::PostInit()
{
	if( ! IsInitialized() )
	{
		return;
	}

#ifdef RED_PLATFORM_DURANGO
	// #tbd: why is this called a second time exactly?
	DWORD audioAffinityMask = 1 << 2 | 1 << 3 | 1 << 4 | 1 << 5;
	SetWasapiThreadAffinityMask( audioAffinityMask );

	m_depotBanksPath = TXT( "soundbanks/xboxone/" );
#elif defined( RED_PLATFORM_ORBIS )
	m_depotBanksPath = TXT( "soundbanks/ps4/" );
#else
	m_depotBanksPath = TXT( "soundbanks\\pc\\" );
#endif

	sm_fileLoader.Init();

	
	// Load conversion array
	C2dArray* soundEventsToConvert = resSoundEventConversion.LoadAndGet< C2dArray >();

	Uint32 setcCount = 0;
	if( soundEventsToConvert )
	{
		setcCount += ( unsigned char ) soundEventsToConvert->GetNumberOfRows();
	}

	m_soundEventConversionMap.Clear();
	m_soundEventConversionMap.Reserve( setcCount );

	for( Uint32 i = 0; i < setcCount; ++i)
	{
		SSoundEventConversionHelper soundEventConversion;
		StringAnsi caseInsensitiveName = StringAnsi( UNICODE_TO_ANSI( soundEventsToConvert->GetValue( TXT( "SoundEvent" ), i ).AsChar() ) );

		String switchValue, switchName, rtpcName;
		Float rtpcValue;
		C2dArray::ConvertValue( soundEventsToConvert->GetValue( TXT( "SwitchName" ), i ),	switchName );
		C2dArray::ConvertValue( soundEventsToConvert->GetValue( TXT( "SwitchValue" ), i ),	switchValue );
		C2dArray::ConvertValue( soundEventsToConvert->GetValue( TXT( "RTPCName" ), i ),		rtpcName );
		C2dArray::ConvertValue( soundEventsToConvert->GetValue( TXT( "RTPCValue" ), i ),	rtpcValue );

		if( switchName != String::EMPTY ) // conversion to swtich
		{
			soundEventConversion.m_type = SSoundEventConversionHelper::ECT_Switch;
			soundEventConversion.m_name = StringAnsi( UNICODE_TO_ANSI( switchName.AsChar() ) );
			soundEventConversion.m_switchValue = StringAnsi( UNICODE_TO_ANSI( switchValue.AsChar() ) );
		}
		else if( rtpcName != String::EMPTY)
		{
			soundEventConversion.m_type = SSoundEventConversionHelper::ECT_RTPC;
			soundEventConversion.m_name = StringAnsi( UNICODE_TO_ANSI( rtpcName.AsChar() ) );
			soundEventConversion.m_rtpcValue = rtpcValue;
		}
		else
		{
			ASSERT( false, TXT( "Incorrect line in soundevents_to_switch_conversion file! Line number: [%i]"), i );
		}

		m_soundEventConversionMap.Insert( caseInsensitiveName, soundEventConversion );
	}
	
#ifndef NO_DEBUG_WINDOWS	
	m_currentSoundBanks.Clear();
#endif
	GLoadingProfiler.FinishStage( TXT("SoundLoaderInit") );

	m_ambientManager.Init();

	//Let's not have this on by default just yet; it can be actived from scripts using SoundEnableMusicEvents
	//GSoundSystem->GetMusicSystem().Enable(true);
}

void CSoundSystem::FinalizeLoading()
{
	sm_fileLoader.Reset();

	CSoundEmitter::m_playingEmitters.Reset( 1024 );
	CLightweightSoundEmitter::m_gameObjects.Reset( 1024 );

	// Create script wrapper
	m_scriptWrapper = CreateObject< CScriptSoundSystem >();
	m_scriptWrapper->AddToRootSet();

	m_currentLocalization = SLocalizationManager::GetInstance().GetSpeechLocale();
	SoundSwitch("language", UNICODE_TO_ANSI(m_currentLocalization.ToLower().AsChar()));

}

void CSoundSystem::Shutdown()
{
	if( ! IsInitialized() )
	{
		return;
	}

#ifdef USE_WWISE

	Flush();

	m_ambientManager.Reset();
	m_ambientManager.SoundStopAll();
	m_ambientManager.Flush();

	m_initialized = false;

#if !defined(RED_FINAL_BUILD) || defined(LOG_IN_FINAL)
	AK::Comm::Term();
#endif

	// Terminate the music engine
	AK::MusicEngine::Term();

	CSoundBank::ShutDown();

	// Terminate the sound engine
	if ( AK::SoundEngine::IsInitialized() )
	{
		AK::SoundEngine::Term();
#ifdef RED_PLATFORM_WINPC
		if( m_initializedCom )
		{
			CoUninitialize();
		}
#endif
	}

	sm_fileLoader.Shutdown();
	
	// Terminate the Memory Manager
	if ( AK::MemoryMgr::IsInitialized() )
	{
		AK::MemoryMgr::Term();
	}

	m_cameraIsFree = false;
#endif
}

void CSoundSystem::Reset()
{
	if ( nullptr != m_ambientManager.m_ambientActivator )
	{
		m_ambientManager.m_ambientActivator->Remove();
		m_ambientManager.m_ambientActivator->Release();
		m_ambientManager.m_ambientActivator = nullptr;
	}
	if ( m_SoundListener != nullptr )
	{
		m_SoundListener->RemoveFromRootSet();
		m_SoundListener->Discard();
		m_SoundListener = nullptr;
	}
	if( m_PanningListener != nullptr )
	{
		m_PanningListener->RemoveFromRootSet();
		m_PanningListener->Discard();
		m_PanningListener = nullptr;
	}
}

namespace Hacks
{
	void RenderAudioWhileBlockingGame()
	{
#if defined(USE_WWISE)
		AK::SoundEngine::RenderAudio();
#endif
	}

	void RenderAudioForLoadingScreen()
	{
#if defined(USE_WWISE)
# if WITH_SCALEFORM_WWISE
		AK::SoundEngine::RenderAudio();
# endif
#endif
	}

	// Fading bleeds into the loading screen... nobody's fixing it on the soundswitch side. Currently not using Wwise for loading screen.
	// This fade really ruins them. Or hearing the wind, or other ambient sounds.
	void MuteAudioForLoadingScreen()
	{
#if defined(USE_WWISE)
# if !(WITH_SCALEFORM_WWISE)
		GSoundSystem->SoundGlobalParameter( "menu_volume_master", 0.f );
		AK::SoundEngine::RenderAudio();
# endif
#endif
	}

	void UnmuteAudioAfterLoadingScreen()
	{
#if defined(USE_WWISE)
# if !(WITH_SCALEFORM_WWISE)
		GSoundSystem->SoundGlobalParameter( "menu_volume_master", Config::cvMasterVolume.Get() );
		AK::SoundEngine::RenderAudio();
# endif
#endif
	}
}

void CSoundSystem::IssuePreTick( Float dt )
{
	PC_SCOPE_PIX( IssueSoundPreTick )

	// still not initialized
	if ( !IsInitialized() )
	{
		return;
	}
	m_delta = dt;

	GSoundSystem->PreTick( m_delta );

	// Once the contention gets better for PS4, tick this on task thread. It's thread safe.
	//m_soundEmittersTask = new (CTask::Root) CTickSoundEmittersTask( dt );
	//GTaskManager->Issue( *m_soundEmittersTask );
}

void CSoundSystem::PreTick( Float dt )
{
	PC_SCOPE_PIX( CSoundSystemPreTick )

	const Vector& position = m_SoundListener ? m_SoundListener->GetPosition() : Vector::ZERO_3D_POINT;
	{
		PC_SCOPE_PIX( CSoundEmitterComponent ProcessTick )
		CSoundEmitterComponent::ProcessTick( dt, position );
	}

	{
		PC_SCOPE_PIX( ProcessPlayingEmitters )
		CLightweightSoundEmitter::Process();
	}

	{
		PC_SCOPE_PIX( ProcessPlayingEmitters )
		CSoundEmitter::ProcessPlayingEmitters( m_PanningListener );
	}
}

void CSoundSystem::Tick( Float dt )
{
	PC_SCOPE_PIX( CSoundSystemTick )
	
	if ( !IsInitialized() )
	{
		return;
	}

	TDynArray<STimedSoundEvent*> eventsToRemove;
	for (auto &timedEvent : m_timedSoundEvents)
	{
		if(GGame->GetEngineTime() > timedEvent.lastEngineTime)
		{
			timedEvent.currentTime += dt;
			if(timedEvent.updateTimeParameter && timedEvent.startEventId != AK_INVALID_PLAYING_ID)
			{
				SoundParameter("eventTime", timedEvent.currentTime, 0.f);
			}

			if( timedEvent.currentTime > timedEvent.duration)
			{
				SoundEvent(UNICODE_TO_ANSI(timedEvent.onStopEvent.AsChar()));
				eventsToRemove.PushBack(&timedEvent);
			}
			timedEvent.lastEngineTime = GGame->GetEngineTime();
		}
		else if(GGame->GetEngineTime() < timedEvent.lastEngineTime) //If time has gone backwards we need to clear all timed events
		{
			SoundEvent(UNICODE_TO_ANSI(timedEvent.onStopEvent.AsChar()));
			eventsToRemove.PushBack(&timedEvent);
		}
	}

	for(auto event : eventsToRemove)
	{
		m_timedSoundEvents.Erase(event);
	}

	// Once the contention gets better for PS4, tick this on task thread. It's thread safe.
	//if( m_soundEmittersTask )
	//{
	//	PC_SCOPE_PIX( FinalizeSoundEmittersTick )

	//	m_soundEmittersTask->FinalizeSoundEmittersTick();
	//}

	sm_fileLoader.RefreshSoundBanks();

	// Refresh volumes if needed
	if( Abs( m_soundCurrentVolume - Config::cvSoundsVolume.Get() ) > 0.001f )
	{
		m_soundCurrentVolume = Config::cvSoundsVolume.Get();
		SoundGlobalParameter( "menu_volume_sounds", m_soundCurrentVolume );
		( new CRenderCommand_SetVideoEffectsVolume( m_soundCurrentVolume / 100.f ) )->Commit();
	}

	if( Abs( m_musicCurrentVolume - Config::cvMusicVolume.Get() ) > 0.001f )
	{
		m_musicCurrentVolume = Config::cvMusicVolume.Get();
		SoundGlobalParameter( "menu_volume_music", m_musicCurrentVolume );
	}

	if( Abs( m_masterCurrentVolume - Config::cvMasterVolume.Get() ) > 0.001f )
	{
		m_masterCurrentVolume = Config::cvMasterVolume.Get();
		SoundGlobalParameter( "menu_volume_master", m_masterCurrentVolume );
		( new CRenderCommand_SetVideoMasterVolume( m_masterCurrentVolume / 100.f ) )->Commit();
	}

	if(m_underwaterSpeechMutingCount == 0 && !m_muteSpeechUnderWater)
	{
		if( Abs( m_voiceoversCurrentVolume - Config::cvVoiceoversVolume.Get() ) > 0.001f )
		{
			m_voiceoversCurrentVolume = Config::cvVoiceoversVolume.Get();
			SoundGlobalParameter( "menu_volume_voiceovers", m_voiceoversCurrentVolume );
			( new CRenderCommand_SetVideoVoiceVolume( m_voiceoversCurrentVolume / 100.f ) )->Commit();
		}
	}
	else if(GGame->GetActiveWorld().Get())
	{
		CGlobalWater * water = GGame->GetActiveWorld().Get()->GetGlobalWater();
		if(water && water->GetWaterLevelApproximate(m_cameraPosition.X, m_cameraPosition.Y, 0) > m_cameraPosition.Z)
		{
			m_voiceoversCurrentVolume = 0.f;
			SoundGlobalParameter( "menu_volume_voiceovers", m_voiceoversCurrentVolume );
		}
		else if( Abs( m_voiceoversCurrentVolume - Config::cvVoiceoversVolume.Get() ) > 0.001f )
		{
			m_voiceoversCurrentVolume = Config::cvVoiceoversVolume.Get();
			SoundGlobalParameter( "menu_volume_voiceovers", m_voiceoversCurrentVolume );
			( new CRenderCommand_SetVideoVoiceVolume( m_voiceoversCurrentVolume / 100.f ) )->Commit();
		}
	}



	{
		// Blackscreen Off
		if( !GGame->IsLoadingScreenShown() && !m_isBlackscreen && m_callOnBlackscreenEnd )
		{
			m_callOnBlackscreenEnd = false;
			m_scriptWrapper->CallEvent( CNAME( OnBlackscreenEnd ) );
		}
	}

	m_musicSystem.Tick( dt );
	m_wallaSystem.Tick();

	String language = SLocalizationManager::GetInstance().GetSpeechLocale();
	if(language != m_currentLocalization)
	{
		m_currentLocalization = language;
		SoundSwitch("language", UNICODE_TO_ANSI(m_currentLocalization.ToLower().AsChar()));
	}

#if defined(USE_WWISE)
	AK::SoundEngine::RenderAudio();

	// Collect states from scripts
	{
		PC_SCOPE_PIX(CollectSoundStates)
		GScriptingSystem->CallGlobalExecFunction( TXT( "CollectSoundStates()"), true );
	}

	// Update the position of the audio listener component
	if( m_SoundListener )
	{
		m_ambientManager.Tick( m_cachedListenerPosition, dt );
	}
#if !defined(NO_EDITOR)
	{
		PC_SCOPE_PIX(AdditionalListeners)
		// Additional listeners
		for ( Uint32 i = 0; i != m_listeners.Size(); ++i )
		{
			CSoundAdditionalListener* listener = m_listeners[ i ];
			if ( NULL != listener )
			{
				AkListenerPosition listenerPositionAk;
				listenerPositionAk.OrientationFront = ToAk( listener->m_forward );
				listenerPositionAk.OrientationTop = ToAk( listener->m_up );
				listenerPositionAk.Position = ToAk( listener->m_position );
				AK::SoundEngine::SetListenerPosition( listenerPositionAk, i + 1 );
			}
		}
	}
#endif
#endif // USE_WWISE
}


#ifndef NO_EDITOR

TDynArray< String > CSoundSystem::GetDefinedEnvironments()
{
	if( m_definedEnvironments.Size() > 0 )
	{
		return m_definedEnvironments;
	}

	String path;
	GDepot->GetAbsolutePath( path );
	path += m_depotBanksPath;
	path += TXT( "*.txt" );
	for ( CSystemFindFile findFile( path.AsChar() ); findFile; ++findFile )
	{
		if ( !findFile.IsDirectory() )
		{
			String fileName;
			GDepot->GetAbsolutePath( fileName );
			fileName += m_depotBanksPath;
			fileName += findFile.GetFileName();
			String separator( TXT( "\t" ) );

			C2dArray* array = C2dArray::CreateFromString( fileName, separator );

			if( array )
			{
				for( Uint32 i = 0; i < array->GetNumberOfRows(); ++i )
				{
					String type( array->GetValue( 0, i ) );
					if( type.Size() && type == TXT( "Auxiliary Bus" ) )
					{
						i++;
						while( array->GetValue( 1, i ).Size() )
						{
							m_definedEnvironments.PushBackUnique( array->GetValue( 2, i ) );
							i++;
						}
						break;
					}
				}

			}
		}
	}
	return m_definedEnvironments;
}

TDynArray< String > CSoundSystem::GetDefinedEvents()
{
	if( m_definedEvents.Size() > 0 )
	{
		return m_definedEvents;
	}

	String path;
	GDepot->GetAbsolutePath( path );

	path += m_depotBanksPath;
	path += TXT( "*.txt" );
	for ( CSystemFindFile findFile( path.AsChar() ); findFile; ++findFile )
	{
		if ( !findFile.IsDirectory() )
		{
			String fileName;
			GDepot->GetAbsolutePath( fileName );
			fileName += m_depotBanksPath;
			fileName += findFile.GetFileName();
			String separator( TXT( "\t" ) );

			C2dArray* array = C2dArray::CreateFromString( fileName, separator );

			if( array )
			{
				Int32 i = 0;
				String header = array->GetHeader( 0 );
				if( header == TXT( "Event" ) || header == TXT( "Dialogue Event" ) )
				{
					while( array->GetValue( 1, i ).Size() )
					{
						m_definedEvents.PushBackUnique( array->GetValue( 2, i ) );
						i++;
					}
					i++;
				}
				for( Uint32 j = i; j < array->GetNumberOfRows(); ++j )
				{
					String type( array->GetValue( 0, i ) );
					if( type.Size() && ( type == TXT( "Event" ) || type == TXT( "Dialogue Event" ) ) )
					{
						while( array->GetValue( 1, i ).Size() )
						{
							m_definedEvents.PushBackUnique( array->GetValue( 2, i ) );
							i++;
						}
						i++;
					}
				}
			}
		}
	}

	return m_definedEvents;
}


TDynArray< String > CSoundSystem::GetDefinedSwitches()
{
	if( m_definedSwitches.Size() > 0 )
	{
		return m_definedSwitches;
	}

	String path;
	GDepot->GetAbsolutePath( path );
	path += m_depotBanksPath;
	path += TXT( "*.txt" );
	for ( CSystemFindFile findFile( path.AsChar() ); findFile; ++findFile )
	{
		if ( !findFile.IsDirectory() )
		{
			String fileName;
			GDepot->GetAbsolutePath( fileName );

			fileName += m_depotBanksPath;
			fileName.Append( findFile.GetFileName(), Red::System::StringLength( findFile.GetFileName() ) );
			String separator( TXT( "\t" ) );

			C2dArray* array = C2dArray::CreateFromString( fileName, separator );

			if( array )
			{
				for( Uint32 i = 0; i < array->GetNumberOfRows(); ++i )
				{
					String type( array->GetValue( 0, i ) );
					if( type.Size() && type == TXT( "Switch Group" ) )
					{
						i++;
						while( array->GetValue( 1, i ).Size() )
						{
							m_definedSwitches.PushBackUnique( array->GetValue( 2, i ).AsChar() );
							i++;
						}
						break;
					}
				}

			}
		}
	}

	return m_definedSwitches;
}

TDynArray< String > CSoundSystem::GetDefinedGameParameters()
{
	if( m_definedGameParameters.Size() > 0 )
	{
		return m_definedGameParameters;
	}

	String path;
	GDepot->GetAbsolutePath( path );
	path += m_depotBanksPath;
	path += TXT( "*.txt" );
	for ( CSystemFindFile findFile( path.AsChar() ); findFile; ++findFile )
	{
		if ( !findFile.IsDirectory() )
		{
			String fileName;
			GDepot->GetAbsolutePath( fileName );

			fileName += m_depotBanksPath;
			fileName.Append( findFile.GetFileName(), Red::System::StringLength( findFile.GetFileName() ) );
			String separator( TXT( "\t" ) );

			C2dArray* array = C2dArray::CreateFromString( fileName, separator );

			if( array )
			{
				for( Uint32 i = 0; i < array->GetNumberOfRows(); ++i )
				{
					String type( array->GetValue( 0, i ) );
					if( type.Size() && type == TXT( "Game Parameter" ) )
					{
						i++;
						while( array->GetValue( 1, i ).Size() )
						{
							m_definedGameParameters.PushBackUnique( array->GetValue( 2, i ).AsChar() );
							i++;
						}
						break;
					}
				}

			}
		}
	}

	return m_definedGameParameters;
}
#endif

void CSoundSystem::SetListenerVectorsFromCameraAndCharacter( const Vector& cameraPosition, const Vector& up, const Vector& forward, const Vector* controllerPosition )
{
	if ( m_manuallListner ) return;
	m_cameraPosition = cameraPosition;
	m_up = up;
	m_forward = forward;

	if ( NULL != controllerPosition )
	{
		m_controllerPosition = *controllerPosition;
		m_cameraIsFree = false;
	}
	else
	{
		m_cameraIsFree = true;
	}
	ProcessListener();
}

void CSoundSystem::SetListenerVectorsManually( const Vector& manualPosition, const Vector& up, const Vector& forward )
{
	m_cameraIsFree = true;
	m_cameraPosition = manualPosition;
	m_up = up;
	m_forward = forward;
	m_manuallListner = manualPosition != Vector::ZEROS & up != Vector::ZEROS & forward != Vector::ZEROS;
	ProcessListener();
}

void CSoundSystem::GetListenerVectors( Vector& position, Vector& up, Vector& forward )
{
	PC_SCOPE_PIX( CSoundSystem GetListenerVectors )

#ifdef USE_WWISE
	AkListenerPosition listenerPosition;
	AKRESULT RESULT = AK::SoundEngine::Query::GetListenerPosition( 0, listenerPosition );

	position = ToVector( listenerPosition.Position );
	forward = ToVector( listenerPosition.OrientationFront );
	up = ToVector(listenerPosition.OrientationTop);
#endif
}

const Vector CSoundSystem::GetListenerDirection() const
{
#ifdef USE_WWISE
	AkListenerPosition listenerPosition;
	AKRESULT RESULT = AK::SoundEngine::Query::GetListenerPosition( 0, listenerPosition );

	return ToVector(listenerPosition.OrientationFront);
#endif
}

const Vector& CSoundSystem::GetListenerPosition() const
{
	return m_SoundListener ? m_SoundListener->GetPosition() : Vector::ZERO_3D_POINT;
}

const Vector& CSoundSystem::GetPanningListenerPosition() const
{
	return m_PanningListener ? m_PanningListener->GetPosition() : Vector::ZERO_3D_POINT;
}

void CSoundSystem::OnGameStart(const CGameInfo& gameInfo)
{
	m_ambientManager.Reset();

	SGameSessionManager::GetInstance().RegisterGameSaveSection( this );
}

void CSoundSystem::OnGameEnd()
{
	m_ambientManager.Reset();

	SGameSessionManager::GetInstance().UnregisterGameSaveSection( this );	
}

#ifdef SOUND_EDITOR_STUFF

void CSoundSystem::MuteFromEditor( Bool mute )
{
	SoundEvent( mute ? "mute_all" : "unmute_all" );
}

#endif

void CSoundSystem::OnUserEvent( const EUserEvent& event )
{

}

void CSoundSystem::FlushListener()
{
	if ( m_SoundListener )
	{
		m_SoundListener->Flush();
		m_SoundListener->RemoveFromRootSet();
		m_SoundListener = nullptr;
	}
	if(m_PanningListener)
	{
		m_PanningListener->Flush();
		m_PanningListener->RemoveFromRootSet();
		m_PanningListener = nullptr;
	}
}

#ifndef NO_EDITOR
CSoundAdditionalListener* CSoundSystem::GetAdditionalListener( Int32 bitmask )
{
	for( Uint32 i = 0; i != m_listeners.Size(); ++i )
	{
		CSoundAdditionalListener* listener = m_listeners[ i ];
		if( !listener ) continue;
		if( ( 1 << listener->GetIndex() ) == bitmask ) return m_listeners[ i ];
	}
	return nullptr;
}
#endif

Bool CSoundSystem::OutOfMemoryCallback()
{
	if( !m_initialized ) return false;

#ifdef USE_WWISE
	AkInt32 prev, cur;
	prev = AK::MemoryMgr::GetNumPools();
	AKRESULT result = AK::SoundEngine::UnregisterAllGameObj();
	CSoundBank::ClearBanks();
	cur = AK::MemoryMgr::GetNumPools();

	if( cur < prev )
	{
		return true;
	}
	else
#endif
	{
		return false;
	}
}

void CSoundSystem::ProcessListener()
{
	// fade between camera and controller position based on the free camera flag
	if ( isFreeCamera() || ( GGame && ( GGame->IsPlayingCachetDialog() || GGame->IsPlayingCameraCutscene() ) ) ) 
	{
		if ( m_multipler > 0.0f )
		{
			m_multipler -= m_delta;
			if ( m_multipler < 0.0f )
			{
				m_multipler = 0.0f;
			}
		}
	}
	else
	{
		if ( m_multipler < SSoundSettings::m_listnerOnPlayerFromCamera )
		{
			m_multipler += m_delta;
			if ( m_multipler > SSoundSettings::m_listnerOnPlayerFromCamera )
			{
				m_multipler = SSoundSettings::m_listnerOnPlayerFromCamera;
			}
		}
		else if( m_multipler > SSoundSettings::m_listnerOnPlayerFromCamera )
		{
			m_multipler -= m_delta;
			if ( m_multipler < 0.0f )
			{
				m_multipler = 0.0f;
			}
		}
	}

	// Move listener in player direction by 2/3 of distance between player and camera.
	// It is due to sound distances misbehaving when calculated just from camera.
	const Vector cameraToPlayer( m_controllerPosition - m_cameraPosition );
	const Float cameraToPlayerDistance = cameraToPlayer.Mag3();
	const Vector listenerPosition = m_cameraPosition + (cameraToPlayer.Normalized3() * cameraToPlayerDistance * m_multipler);


	// Create listener component
	if ( nullptr == m_SoundListener )
	{
		CWorld* world = GGame->GetActiveWorld();
		m_SoundListener = CreateObject< CSoundListenerComponent >( world );
		m_SoundListener->AddToRootSet();
		AK::SoundEngine::SetListenerSpatialization(0, true);
	}
	if( nullptr == m_PanningListener )
	{
		CWorld* world = GGame->GetActiveWorld();
		m_PanningListener = CreateObject< CSoundListenerComponent >(world);
		m_PanningListener->AddToRootSet();
		AK::SoundEngine::SetListenerSpatialization(1, true);
	}

	Vector forward = m_forward;
	Vector up = m_up;
	Vector position = listenerPosition;
	Vector panningPosition = m_cameraPosition;

	if(!m_listenerOverrides.Empty() && m_listenerOverrideRequests.Size() > 0)
	{
		SListenerOverride * listenerOverride = m_listenerOverrides.FindPtr(m_listenerOverrideRequests.Back());
		if(listenerOverride)
		{
			forward = listenerOverride->forwards;
			up = listenerOverride->up;
			position = listenerOverride->position;
			panningPosition = position;
		}

	}

	m_cachedListenerPosition = position;

	if ( nullptr != m_SoundListener )
	{
		m_SoundListener->UpdatePosition( forward, up, position );
	}

	if( nullptr != m_PanningListener)
	{
		m_PanningListener->UpdatePosition(forward, up, panningPosition);
	}
}

void CSoundSystem::OnLoadGame( IGameLoader* loader )
{
	if( loader )
	{
		CGameSaverBlock block( loader, CNAME( SoundSystem ) );

		// Sound events
		{
			CGameSaverBlock entriesBlock( loader, CNAME( SoundEventSaveData ) );

			m_savedSoundEvents.Clear();

			Uint32 size = loader->ReadValue< Uint32 >( CNAME( Size ) );

			for( Uint32 i = 0; i < size; ++i )
			{
				StringAnsi soundEvent = StringAnsi::EMPTY;

				loader->ReadValue( CNAME( SavedSoundEvent ), soundEvent );

				if( soundEvent != StringAnsi::EMPTY )
				{
					m_savedSoundEvents.PushBack(soundEvent);
					SoundEvent( soundEvent.AsChar() );
				}
			}
		}
	}	
}
bool CSoundSystem::OnSaveGame( IGameSaver* saver )
{
	TIMER_BLOCK( time )

	CGameSaverBlock block( saver, CNAME( SoundSystem ) );

	// Sound events
	{
		CGameSaverBlock entriesBlock( saver, CNAME( SoundEventSaveData ) );

		saver->WriteValue( CNAME( Size ), m_savedSoundEvents.Size() );

		for( TDynArray< StringAnsi >::iterator iter = m_savedSoundEvents.Begin(), end = m_savedSoundEvents.End(); iter != end; ++iter )
		{
			saver->WriteValue( CNAME( SavedSoundEvent ), *iter );
		}
	}

	END_TIMER_BLOCK( time )

	return true;
}

void CSoundSystem::ClearSavedSoundEvents()
{
	m_savedSoundEvents.Clear();
}

void CSoundSystem::AddSoundEventToSave( StringAnsi eventName )
{
	m_savedSoundEvents.PushBack( eventName );
}


#ifndef NO_EDITOR
void CSoundSystem::ReloadSoundbanks()
{
	m_definedEnvironments.ClearFast();
	m_definedEvents.ClearFast();
	m_definedSwitches.ClearFast();
	m_definedGameParameters.ClearFast();

	CSoundBank::ReloadSoundbanks();
}

void CSoundSystem::SetMasterVolume(Float volume)
{
	Config::cvMasterVolume.Set( volume );
}

void CSoundSystem::SetMusicVolume(Float volume)
{
	Config::cvMusicVolume.Set( volume );
}

void CSoundSystem::SetVoiceoversVolume(Float volume)
{
	Config::cvVoiceoversVolume.Set( volume );
}

void CSoundSystem::SetSoundsVolume(Float volume)
{
	Config::cvSoundsVolume.Set( volume );
}

Float CSoundSystem::GetMasterVolume()
{
	return Config::cvMasterVolume.Get();
}

Float CSoundSystem::GetMusicVolume()
{
	return Config::cvMusicVolume.Get();
}

Float CSoundSystem::GetVoiceoversVolume()
{
	return Config::cvVoiceoversVolume.Get();
}

Float CSoundSystem::GetSoundsVolume()
{
	return Config::cvSoundsVolume.Get();
}

String CSoundSystem::GetCacheFileName()
{
#if defined (RED_PLATFORM_WINPC)
	String result = String( TXT("soundspc.cache") );
#elif defined(RED_PLATFORM_DURANGO)
	String result = String( TXT("soundsxboxone.cache") );
#elif defined(RED_PLATFORM_ORBIS)
	String result = String( TXT("soundsps4.cache") );
#else
	String result;
	RED_FATAL_ASSERT( false, "Unknown platform.");
#endif
	return result;
}

#endif	// NO_EDITOR

Bool CSoundSystem::CheckSoundEventConversion(const StringAnsi& soundEvent, SSoundEventConversionHelper& foundConversion)
{
	return m_soundEventConversionMap.Find( soundEvent, foundConversion );
}

void CSoundSystem::OnBlackscreenEnd()
{
	if( m_isBlackscreen )
	{
		m_isBlackscreen = false;
		m_callOnBlackscreenEnd = true;
	}
}

void CSoundSystem::OnBlackscreenStart()
{
	if( !m_isBlackscreen )
	{
		m_isBlackscreen = true;
		m_scriptWrapper->CallEvent( CNAME( OnBlackscreenStart ) );
	}
}
#ifndef NO_DEBUG_WINDOWS

void CSoundSystem::CheckQuestBank(String bnk, String phaseName )
{
	SSoundBankQuests val;
	if( !m_currentSoundBanks.Find(bnk, val ) )
	{
		val.m_refCount = 1500100900;
		val.m_blockPhases.PushBackUnique( TXT("ERROR - BNK REMOVED PREMATURELY") );
		m_currentSoundBanks.Set(bnk, val);
	}
}

void CSoundSystem::AddQuestBank(String bnk, String phaseName )
{
	SSoundBankQuests val;
	if( !m_currentSoundBanks.Find(bnk, val ) )
	{
		val.m_refCount = 1;
		val.m_blockPhases.PushBackUnique( phaseName );
		m_currentSoundBanks.Set(bnk, val);
	}
	else
	{
		val.m_refCount++;
		val.m_blockPhases.PushBackUnique( phaseName );
		m_currentSoundBanks.Set(bnk, val);
	}
}

void CSoundSystem::RemoveQuestBank(String bnk, String phaseName )
{
	SSoundBankQuests val;
	if( !m_currentSoundBanks.Find(bnk, val ) )
	{
		val.m_refCount = 37707;
		val.m_blockPhases.PushBackUnique( TXT("ERROR - REMOVING BNK THAT IS ALREDY UNLOADED") );
		m_currentSoundBanks.Set(bnk, val);
	}
	else
	{
		val.m_refCount--;
		val.m_blockPhases.Remove( phaseName );
		if( val.m_refCount )
		{
			m_currentSoundBanks.Set(bnk, val);
		}
		else
		{	// if refCount == 0 we remove bank
			m_currentSoundBanks.Erase( bnk );
		}
	}
}

THashMap< String, SSoundBankQuests > CSoundSystem::GetQuestBanks()
{
	return m_currentSoundBanks;
}



#endif // !NO_DEBUG_WINDOWS

void CSoundSystem::IncrementMusicBlockedCount()
{
	m_MusicBlockedCount++;
}

void CSoundSystem::DecrementMusicBlockedCount()
{
	if(m_MusicBlockedCount > 0)
	{
		m_MusicBlockedCount--;
	}
	else
	{
		SOUND_ERR(TXT("Attempted to decrement the music blocked count but it's already 0"));
	}
}

void CSoundSystem::IncrementUnderwaterSpeechMutingCount()
{
	m_underwaterSpeechMutingCount++;
}

void CSoundSystem::DecrementUnderwaterSpeechMutingCount()
{
	if(m_underwaterSpeechMutingCount > 0)
	{
		m_underwaterSpeechMutingCount--;
	}
	else
	{
		SOUND_ERR(TXT("Attempted to decrement the underwater speech muting count but it's already 0"));
	}
}

Bool CSoundSystem::GetRtpcValue(const char * name, Float &retVal, Uint64 gameObject /*= 0*/) const
{
	AkRtpcValue value;
	AK::SoundEngine::Query::RTPCValue_type type = SoundEngine::Query::RTPCValue_GameObject;
	AKRESULT result = AK::SoundEngine::Query::GetRTPCValue(name, (AkGameObjectID)gameObject, value, type);
	retVal = (Float) value;
	return result == AK_Success;
}

Float CSoundSystem::LinearToDb(Float linVol)
{
		Float dbVol;

		if (linVol >= 0.0f)
			dbVol = 20.0f * MLog10(linVol);
		else
			dbVol = -144.0f;  // effectively minus infinity

		return dbVol;
}

void CSoundSystem::PushListenerOverride(String overrideTarget)
{
	m_listenerOverrideRequests.PushBack(overrideTarget);
}

void CSoundSystem::PopListenerOverride()
{
	m_listenerOverrideRequests.PopBack();
}

void CSoundSystem::RegisterListenerOverride(String overrideName, Vector position, Vector up, Vector forwards)
{
	SListenerOverride * listenerOverride = m_listenerOverrides.FindPtr(overrideName); 
	SListenerOverride newOverride;
	if(!listenerOverride)
	{
		listenerOverride = &newOverride;
	}

	listenerOverride->overrideName = overrideName;
	listenerOverride->position = position;
	listenerOverride->up = up;
	listenerOverride->forwards = forwards;

	m_listenerOverrides.Insert(overrideName, *listenerOverride);
}

void CSoundSystem::UnregisterListenerOverride(String overrideName)
{
	m_listenerOverrides.Erase(overrideName);
}

void CSoundSystem::GenerateEditorFragments(CRenderFrame* frame)
{
	m_musicSystem.GenerateEditorFragments(frame);
}

//HACK this is only used for stagger going in at the end of ep2 so I'm writing it very custom for that
//it is NOT curently safe for general use
void CSoundSystem::TimedSoundEvent(String startEvent, String stopEvent, Float duration, Bool updateTimeParameter /*= false*/)
{
	STimedSoundEvent timedEvent;
	timedEvent.startEventId = SoundEvent(UNICODE_TO_ANSI(startEvent.AsChar()));

	timedEvent.onStopEvent = stopEvent;
	timedEvent.duration = duration;
	timedEvent.lastEngineTime = GGame->GetEngineTime();
	timedEvent.updateTimeParameter = updateTimeParameter;
	timedEvent.currentTime = 0.f;

	for(auto &existingEvent : m_timedSoundEvents)
	{
		if(existingEvent.onStopEvent == timedEvent.onStopEvent)
		{
			if((existingEvent.duration - existingEvent.currentTime) < timedEvent.duration)
			{
				existingEvent = timedEvent;
				if(timedEvent.updateTimeParameter && (AkPlayingID)timedEvent.startEventId != AK_INVALID_PLAYING_ID)
				{
					SoundParameter("eventTime", 0.f, 0.f);
				}
			}
			return;
		}
	}

	if(timedEvent.updateTimeParameter && (AkPlayingID)timedEvent.startEventId != AK_INVALID_PLAYING_ID)
	{
		SoundParameter("eventTime", 0.f, 0.f);
	}

	m_timedSoundEvents.PushBack(timedEvent);
}

void CSoundSystem::MuteSpeechUnderWater(Bool enable)
{
	m_muteSpeechUnderWater = enable;
}

void CEntity::funcSoundEvent( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, soundName, String() );
	GET_PARAMETER_OPT( CName, name, CName::NONE );
	GET_PARAMETER_OPT( Bool, isSlot, false );
	FINISH_PARAMETERS;

	if(soundName.Empty() ) 
		return;

	if( GGame->IsLoadingScreenShown() ) return;

	if( isSlot )
	{
		const CEntityTemplate* templ = GetEntityTemplate();
		Bool foundSlot = false;

		if ( !templ ) return;
		const EntitySlot* entitySlot = templ->FindSlotByName( name, true );

		if ( !entitySlot ) return;
		name = entitySlot->GetBoneName();
	}

	CSoundEmitterComponent* soundEmitterComponent = GetSoundEmitterComponent();
	if( !soundEmitterComponent ) return;

	CAnimatedComponent* component = GetRootAnimatedComponent();
	if( component && name != CName::NONE )
	{
		Int32 bone = component->FindBoneByName( name );
		soundEmitterComponent->SoundEvent( UNICODE_TO_ANSI( soundName.AsChar() ), bone );
	}
	else
	{
		soundEmitterComponent->SoundEvent( UNICODE_TO_ANSI( soundName.AsChar() ) );
	}
}

void CEntity::funcTimedSoundEvent( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, duration, 0.f);
	GET_PARAMETER_OPT( String, startEvent, String() );
	GET_PARAMETER_OPT( String, stopEvent, String() );
	GET_PARAMETER_OPT( Bool, updateTimeParameter, false );
	GET_PARAMETER_OPT( CName, name, CName::NONE );
	FINISH_PARAMETERS;

	CSoundEmitterComponent* soundEmitterComponent = GetSoundEmitterComponent();
	if( !soundEmitterComponent ) return;

	CAnimatedComponent* component = GetRootAnimatedComponent();
	if( component && name != CName::NONE )
	{
		Int32 bone = component->FindBoneByName( name );
		soundEmitterComponent->TimedSoundEvent( startEvent, stopEvent, duration, updateTimeParameter, bone );
	}
	else
	{
		soundEmitterComponent->TimedSoundEvent( startEvent, stopEvent, duration, updateTimeParameter );
	}

}

void CEntity::funcSoundSwitch( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, switchGroupName, String() );
	GET_PARAMETER( String, switchName, String() );
	GET_PARAMETER_OPT( CName, name, CName::NONE );
	GET_PARAMETER_OPT( Bool, isSlot, false );
	FINISH_PARAMETERS;	

	if( isSlot )
	{
		const CEntityTemplate* templ = GetEntityTemplate();
		Bool foundSlot = false;

		if ( !templ ) return;
		const EntitySlot* entitySlot = templ->FindSlotByName( name, true );

		if ( !entitySlot ) return;
		name = entitySlot->GetBoneName();
	}

	CSoundEmitterComponent* soundEmitterComponent = GetSoundEmitterComponent();
	if( soundEmitterComponent == NULL ) 
	{
		return;
	}

	CAnimatedComponent* component = GetRootAnimatedComponent();
	if( component && name != CName::NONE )
	{
		Int32 bone = component->FindBoneByName( name );
		soundEmitterComponent->SoundSwitch( UNICODE_TO_ANSI( switchGroupName.AsChar() ), UNICODE_TO_ANSI( switchName.AsChar() ), bone );
	}
	else
	{
		soundEmitterComponent->SoundSwitch( UNICODE_TO_ANSI( switchGroupName.AsChar() ), UNICODE_TO_ANSI( switchName.AsChar() ) );

	}

}


void CEntity::funcSoundParameter( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, parameterName, String() );
	GET_PARAMETER( Float, value, 0.0f );
	GET_PARAMETER_OPT( CName, name, CName::NONE );
	GET_PARAMETER_OPT( Float, duration, 0.0f );
	GET_PARAMETER_OPT( Bool, isSlot, false );
	FINISH_PARAMETERS;

	if( isSlot )
	{
		const CEntityTemplate* templ = GetEntityTemplate();
		Bool foundSlot = false;

		if ( !templ ) return;
		const EntitySlot* entitySlot = templ->FindSlotByName( name, true );

		if ( !entitySlot ) return;
		name = entitySlot->GetBoneName();
	}

	CSoundEmitterComponent* soundEmitterComponent = GetSoundEmitterComponent();
	if( soundEmitterComponent == NULL ) 
	{
		return;
	}

	CAnimatedComponent* component = GetRootAnimatedComponent();
	if( component && name != CName::NONE )
	{
		if(name == CName(TXT("all")))
		{
			soundEmitterComponent->SetParameterOnAllObjects(UNICODE_TO_ANSI( parameterName.AsChar() ), value, duration);
		}
		else
		{
			Int32 bone = component->FindBoneByName( name );
			soundEmitterComponent->SoundParameter( UNICODE_TO_ANSI( parameterName.AsChar() ), value, duration, bone );
		}
	}
	else
	{
		soundEmitterComponent->SoundParameter( UNICODE_TO_ANSI( parameterName.AsChar() ), value, duration );
	}
}

void CEntity::funcSoundIsActiveAny( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	CSoundEmitterComponent* soundEmitterComponent = GetSoundEmitterComponent();
	if( soundEmitterComponent == NULL ) 
	{
		return;
	}	

	Bool active = soundEmitterComponent->SoundIsActive();
	RETURN_BOOL( active );
}

void CEntity::funcSoundIsActiveName( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, name, CName::NONE );
	FINISH_PARAMETERS;

	CSoundEmitterComponent* soundEmitterComponent = GetSoundEmitterComponent();
	if( soundEmitterComponent == NULL ) 
	{
		return;
	}	

	Bool active = soundEmitterComponent->SoundIsActive( name.AsAnsiChar() );
	RETURN_BOOL( active );
}

void CEntity::funcSoundIsActive( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, name, CName::NONE );
	GET_PARAMETER_OPT( Bool, isSlot, false );
	FINISH_PARAMETERS;

	if( isSlot )
	{
		const CEntityTemplate* templ = GetEntityTemplate();
		Bool foundSlot = false;

		if ( !templ ) return;
		const EntitySlot* entitySlot = templ->FindSlotByName( name, true );

		if ( !entitySlot ) return;
		name = entitySlot->GetBoneName();
	}

	CSoundEmitterComponent* soundEmitterComponent = GetSoundEmitterComponent();
	if( soundEmitterComponent == NULL ) 
	{
		return;
	}	

	Bool active = false;
	CAnimatedComponent* component = GetRootAnimatedComponent();
	if( component )
	{
		Int32 bone = component->FindBoneByName( name );
		active = soundEmitterComponent->SoundIsActive( bone );
	}
	RETURN_BOOL( active );
}

static void funcGetMeshSoundTypeIdentification( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CComponent >, componentHandle, NULL );
	FINISH_PARAMETERS;

	CName resultName = CNAME( default );
	CComponent* component = componentHandle.Get();
	if( component )
	{
		const CMeshTypeComponent* meshComponent = Cast<  const CMeshTypeComponent >( component );

		if( meshComponent ) 
		{
			const CMesh* mesh = Cast< const CMesh >( meshComponent->GetMeshTypeResource() );
			if( mesh )
			{
				const SMeshSoundInfo* msi = mesh->GetMeshSoundInfo();
				if( msi )
				{
					resultName = msi->m_soundTypeIdentification;  			 
				}
			}
		}
	}
	RETURN_NAME( resultName );
}

static void funcGetMeshSoundSizeIdentification( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CComponent >, componentHandle, NULL );
	FINISH_PARAMETERS;

	CName resultName = CNAME( default );
	CComponent* component = componentHandle.Get();
	if( component )
	{
		const CMeshTypeComponent* meshComponent = Cast<  const CMeshTypeComponent >( component );

		if( meshComponent ) 
		{
			const CMesh* mesh = Cast< const CMesh >( meshComponent->GetMeshTypeResource() );
			if( mesh )
			{
				const SMeshSoundInfo* msi = mesh->GetMeshSoundInfo();
				if( msi )
				{
					resultName = msi->m_soundSizeIdentification;  
				}
			}
		}
	}
	RETURN_NAME( resultName );
}

void ExportSoundNatives()
{
	NATIVE_GLOBAL_FUNCTION( "GetMeshSoundTypeIdentification", funcGetMeshSoundTypeIdentification );
	NATIVE_GLOBAL_FUNCTION( "GetMeshSoundSizeIdentification", funcGetMeshSoundSizeIdentification );
}
