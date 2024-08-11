#include "build.h"
#include "../core/depot.h"
#include "../core/mathUtils.h"
#include "soundSettings.h"
#include "actorInterface.h"
#include "layer.h"
#ifdef USE_WWISE

#include <AK/SoundEngine/Common/AkSoundEngine.h>
#include <AK/SoundEngine/Common/AkQueryParameters.h>
#include <AK/SoundEngine/Common/AkDynamicSequence.h>
#include <AK/SoundEngine/Common/AkDynamicDialogue.h>
#include "soundAmbientAreaComponent.h"
#include "soundAdditionalListener.h"

extern Vector ToVector( const AkVector& akVector );
extern AkVector ToAk( const Vector& vector );
#endif
#include "../physics/physicsWrapper.h"
#include "soundSystem.h"
#include "renderFrame.h"
#include "entity.h"
#include "world.h"
#include "tickManager.h"
#include "bitmapTexture.h"
#include "drawableComponent.h"

#include "../physics/physicsWorldPhysxImplBatchTrace.h"

#ifndef RED_FINAL_BUILD

namespace Config
{
	TConfigVar<String, Validation::Always> cvBreakOnSoundEvent( "Audio", "BreakOnSoundEvent", String::EMPTY, eConsoleVarFlag_Developer );

};
#endif

TFastMuiltiStreamList< Uint64, Uint64, 0 > CLightweightSoundEmitter::m_gameObjects;
#define UNOCCLUDED_GAME_OBJECT_SLOT -3

#ifdef RED_FINAL_BUILD 
#define  DEV_ONLY(x) 
#else
#define DEV_ONLY(x) x
#endif

#ifndef RED_FINAL_BUILD
Red::Threads::CAtomic< Bool > CLightweightSoundEmitter::m_swapping;

#endif

static Uint64 GetNextGameObjectId(	const char* DEV_ONLY(objectName))
{
#ifdef USE_WWISE
	static Uint64 lastGameObjectId = 666;
	DEV_ONLY(AnsiChar tmpName[256] = {0};)
	do 
	{
		++lastGameObjectId;
		if( lastGameObjectId > 0x00000000FFFFFFFF )
		{
			lastGameObjectId = 666;
		}
#ifndef RED_FINAL_BUILD
		StringAnsi::ASprintf(tmpName, "%u::%s", lastGameObjectId, objectName);
#endif
	} 
#ifndef RED_FINAL_BUILD
	while( AK::SoundEngine::RegisterGameObj( lastGameObjectId, tmpName ) != AKRESULT::AK_Success );
#else
	while( AK::SoundEngine::RegisterGameObj( lastGameObjectId ) != AKRESULT::AK_Success );
#endif

	//If we want to assign sounds to seperate/multiple listeners for a game object, do that here
	//AK::SoundEngine::SetActiveListeners(lastGameObjectId, FLAG(0) | FLAG(1));

	return lastGameObjectId;
#else
	return 0;
#endif
}

void CLightweightSoundEmitter::Process()
{
#ifndef RED_FINAL_BUILD
	m_swapping.SetValue( true );
#endif

	m_gameObjects.Swap();
	for( ;; )
	{
#ifdef USE_WWISE
		AkGameObjectID gameObject = ( AkGameObjectID ) m_gameObjects.Get();
		if( !gameObject ) break;

		AkUInt32 count = 0;
		AKRESULT result = AK::SoundEngine::Query::GetPlayingIDsFromGameObject( gameObject, count, 0 );
		if( count == 0 )
		{
			result = AK::SoundEngine::UnregisterGameObj( gameObject );
		}
		else
		{
			m_gameObjects.Put( gameObject );
		}
#endif
	}
#ifndef RED_FINAL_BUILD
	m_swapping.SetValue( false );
#endif

}

CLightweightSoundEmitter::CLightweightSoundEmitter( const StringAnsi& objectName )
{
	m_gameObject = GetNextGameObjectId( objectName.AsChar() );
}

CLightweightSoundEmitter::CLightweightSoundEmitter( const Vector& position, const StringAnsi& objectName )
{
	m_gameObject = GetNextGameObjectId( objectName.AsChar() );
#ifdef USE_WWISE
	AkSoundPosition soundPosition;
	soundPosition.Position = ToAk( position );
	soundPosition.Orientation.X = 1.0f;
	soundPosition.Orientation.Y = 0.0f;
	soundPosition.Orientation.Z = 0.0f;
	AK::SoundEngine::SetPosition( m_gameObject, soundPosition );
#endif
}

static const AkUInt32 s_MusicCallbackFlags = AK_MusicSyncUserCue | AK_MusicSyncGrid | AK_MusicSyncBar | AK_MusicSyncBeat;

CMusicSystem::EMusicResponseEventType convertWwiseMusicFlags(AkCallbackType wwiseType)
{
	if(wwiseType == AK_MusicSyncUserCue)
	{
		return CMusicSystem::User;
	}
	if(wwiseType == AK_MusicSyncGrid)
	{
		return CMusicSystem::Grid;
	}
	if(wwiseType == AK_MusicSyncBar)
	{
		return CMusicSystem::Bar;
	}
	if(wwiseType == AK_MusicSyncBeat)
	{
		return CMusicSystem::Beat;
	}
	
	return CMusicSystem::InvalidResponseType;
}

#ifdef USE_WWISE

void eventCallback( AkCallbackType in_eType, AkCallbackInfo* in_pCallbackInfo )
{

	CMusicSystem::EMusicResponseEventType eventType = convertWwiseMusicFlags(in_eType);

	if( eventType == CMusicSystem::InvalidResponseType )
		return;

	if( GSoundSystem != nullptr) 
	{
		AkMusicSyncCallbackInfo *info = (AkMusicSyncCallbackInfo*)in_pCallbackInfo;
		Float eventTime = 0.f;
		Float engineTime = GGame->GetEngineTime();

		Uint32 barBeatsRemaining = (Uint32)(info->fBarDuration/info->fBeatDuration + 0.1f);
		Uint32 gridBeatsRemaining = (Uint32)(info->fGridDuration/info->fBeatDuration + 0.1f);


		switch (eventType)
		{
		case CMusicSystem::InvalidResponseType:
			break;
		case CMusicSystem::Bar:
			eventTime = info->fBarDuration; 
			GSoundSystem->GetMusicSystem().NotifyBar(barBeatsRemaining);
			break;
		case CMusicSystem::Grid:
			eventTime = info->fGridDuration;
			GSoundSystem->GetMusicSystem().NotifyGrid(gridBeatsRemaining);
			break;
		case CMusicSystem::User:
			break;
		case CMusicSystem::Beat:
			eventTime = info->fBeatDuration;
			GSoundSystem->GetMusicSystem().NotifyBeat(info->fBeatDuration);
			break;
		default:
			break;
		}
	}

}
#endif

CLightweightSoundEmitter::~CLightweightSoundEmitter()
{
#ifndef RED_FINAL_BUILD
	if( m_swapping.GetValue() )
	{
		RED_FATAL_ASSERT( SIsMainThread(), "Config loading has to be done on main thread only." );
	}
#endif
	m_gameObjects.PutChecked( m_gameObject );
}

CLightweightSoundEmitter& CLightweightSoundEmitter::Event( const char* eventName )
{

#ifndef RED_FINAL_BUILD
	if(!Config::cvBreakOnSoundEvent.Get().Empty() && StringAnsi(eventName).ContainsSubstring(UNICODE_TO_ANSI(Config::cvBreakOnSoundEvent.Get().AsChar())))
	{
		RED_BREAKPOINT();
	}
#endif // !RED_FINAL_BUILD

	if( !eventName ) return *this;
#ifdef USE_WWISE
	if( GSoundSystem != nullptr && GSoundSystem->GetMusicSystem( ).IsEnabled( ) )
	{
		AkPlayingID result = AK::SoundEngine::PostEvent( eventName, ( AkGameObjectID ) m_gameObject, s_MusicCallbackFlags, eventCallback );
	}
	else
	{
		AkPlayingID result = AK::SoundEngine::PostEvent( eventName, ( AkGameObjectID ) m_gameObject );
	}
#endif
	return *this;
}

#ifndef NO_EDITOR
CLightweightSoundEmitter& CLightweightSoundEmitter::ListenerSwitch( Int32 listenerBitmask )
{
#ifdef USE_WWISE
	AkUInt32 listenerMask = listenerBitmask;

	if( m_gameObject )
	{
		AKRESULT result = AK::SoundEngine::SetActiveListeners( ( AkGameObjectID ) m_gameObject, listenerMask );
		result = AK_NotImplemented;
	}
#endif
	return *this;
}
#endif

CLightweightSoundEmitter& CLightweightSoundEmitter::OcclusionParams(Float occlusion, Float obstruction)
{
	AKRESULT eResult = AK::SoundEngine::SetObjectObstructionAndOcclusion( ( AkGameObjectID ) m_gameObject, 0, obstruction, occlusion );
	return *this;
}

CLightweightSoundEmitter& CLightweightSoundEmitter::Parameter( const char* name, Float value, Float duration, ESoundParameterCurveType curveType )
{
	if( !name ) return *this;
#ifdef USE_WWISE
	AKRESULT result = AK::SoundEngine::SetRTPCValue( name, value, ( AkGameObjectID ) m_gameObject, ( AkInt32 ) duration * 1000, ( AkCurveInterpolation ) curveType );
	result = AK_NotImplemented;
#endif
	return *this;
}

CLightweightSoundEmitter& CLightweightSoundEmitter::Switch( const char* name, const char* value )
{
	if( !name || !value ) return *this;
#ifdef USE_WWISE
	AKRESULT result = AK::SoundEngine::SetSwitch( name, value, ( AkGameObjectID ) m_gameObject );
	result = AK_NotImplemented;
#endif
	return *this;
}

CLightweightSoundEmitter& CLightweightSoundEmitter::Reverb( class ITriggerManager* triggerManager, const Vector& position )
{
	if( !triggerManager ) return *this;

	CSoundListenerComponent* listner = GSoundSystem->GetSoundListner();
	if( !listner ) return *this;

	PC_SCOPE_PIX( CLightweightSoundEmitter Reverb );

	ITriggerManager::TResultTriggers triggers;

	triggerManager->GetTriggersAtPoint( position, TC_SoundReverbArea, triggers );

	Uint32 triggersCount = triggers.Size();
	if( !triggersCount) return *this; 

	Uint32 reverbSentCount = 0;
	AkAuxSendValue aEnvs[ 4 ];

	struct SSoundAmbientAreaComponentVector
	{
		CSoundAmbientAreaComponent* m_area;
		float m_listenerTriggerPenetration;
		float m_soundTriggerPenetration;

		SSoundAmbientAreaComponentVector() : m_area( 0 ), m_listenerTriggerPenetration( -1.0f ), m_soundTriggerPenetration( -1.0f ) {}
		SSoundAmbientAreaComponentVector( CSoundAmbientAreaComponent* area ) : m_area( area ), m_listenerTriggerPenetration( -1.0f ), m_soundTriggerPenetration( -1.0f ) {}
		SSoundAmbientAreaComponentVector( CSoundAmbientAreaComponent* area, float listnerTriggerPenetration ) : m_area( area ), m_listenerTriggerPenetration( listnerTriggerPenetration ), m_soundTriggerPenetration( -1.0f ) {}
		SSoundAmbientAreaComponentVector( CSoundAmbientAreaComponent* area, float listnerTriggerPenetration, float soundTriggerPenetration ) : m_area( area ), m_listenerTriggerPenetration( listnerTriggerPenetration ), m_soundTriggerPenetration( soundTriggerPenetration ) {}

		RED_INLINE bool operator==( const SSoundAmbientAreaComponentVector& entry ) const
		{
			return m_area == entry.m_area;
		}

		RED_INLINE SSoundAmbientAreaComponentVector& operator+=( const SSoundAmbientAreaComponentVector& entry )
		{
			m_soundTriggerPenetration += entry.m_soundTriggerPenetration;
			return *this;
		}
	};


	//this may be optimized of course...

	TDynArray< SSoundAmbientAreaComponentVector > ambientAreas;
	ambientAreas.Reserve( triggersCount );

	CSoundAmbientAreaComponent* highestPriorityListnerAmbient = 0;
	if( listner )
	{
		Vector listenerPosition = listner->GetPosition();
		for( Uint32 index = 0; index != listner->GetNumOccupiedTrigger(); ++index )
			if( CSoundAmbientAreaComponent* ambientComponent = Cast< CSoundAmbientAreaComponent >( listner->GetOccupiedTrigger( index )->GetComponent() ) )
			{
				if( ambientComponent->GetReverbName().Empty() ) continue;
				Int32 priority = ambientComponent->GetAmbientPriority();
				if( !highestPriorityListnerAmbient || highestPriorityListnerAmbient->GetAmbientPriority() < priority )
				{
					highestPriorityListnerAmbient = ambientComponent;
				}
				float listenerTriggerPenetration = ambientComponent->CalcPenetrationFraction( listenerPosition );
				ambientAreas.PushBack( SSoundAmbientAreaComponentVector( ambientComponent, listenerTriggerPenetration ) );
			}
	}

	for( Uint32 index = 0; index != triggersCount; ++index )
		if( CSoundAmbientAreaComponent* ambientComponent = Cast< CSoundAmbientAreaComponent >( triggers[ index ]->GetComponent() ) )
		{
			if( ambientComponent->GetReverbName().Empty() ) continue;
			float soundTriggerPenetration = ambientComponent->CalcPenetrationFraction( position );
			SSoundAmbientAreaComponentVector* result = ambientAreas.FindPtr( SSoundAmbientAreaComponentVector( ambientComponent ) );
			if( result )
			{
				if( result->m_soundTriggerPenetration == -1 )
				{
					result->m_soundTriggerPenetration = soundTriggerPenetration;
				}
			}
			else
			{
				ambientAreas.PushBack( SSoundAmbientAreaComponentVector( ambientComponent, 0.0f, soundTriggerPenetration ) );
			}
		}


		for( Uint32 j = 0; j != ambientAreas.Size() && reverbSentCount < 4; ++j )
		{
			const SSoundAmbientAreaComponentVector& soundAmbientAreaComponentVector = ambientAreas[ j ];
			const char* name = soundAmbientAreaComponentVector.m_area->GetReverbName().AsChar();
			AkSwitchStateID reverbId = AK::SoundEngine::GetIDFromString( name );

			for( Uint32 existingIndex = 0; existingIndex != reverbSentCount; ++existingIndex )
				if( aEnvs[ existingIndex ].auxBusID == reverbId )
				{
					reverbId = 0;
					break;
				}

				if( reverbId )
				{
					aEnvs[ reverbSentCount ].auxBusID = reverbId;
					float resultWetFilter = 0.0f;
					float soundPenetration = soundAmbientAreaComponentVector.m_soundTriggerPenetration;
					float listenerPenetration = soundAmbientAreaComponentVector.m_listenerTriggerPenetration;
					float outerReverbRatio = soundAmbientAreaComponentVector.m_area->GetOuterReverbRatio();
					if( soundAmbientAreaComponentVector.m_soundTriggerPenetration <= 0.0f )
					{
						resultWetFilter = listenerPenetration;
					}
					else if( soundAmbientAreaComponentVector.m_listenerTriggerPenetration <= 0.0f )
					{
						resultWetFilter = soundPenetration * outerReverbRatio;
					}
					else
					{
						resultWetFilter = soundPenetration * outerReverbRatio;
						resultWetFilter += ( 1.0f - outerReverbRatio ) * listenerPenetration;
					}

					aEnvs[ reverbSentCount ].fControlValue = resultWetFilter;

					reverbSentCount++;
				}
		}

		AkAuxSendValue* aEnvsResult = reverbSentCount ? &aEnvs[ 0 ] : 0;
		AKRESULT eResult = AK::SoundEngine::SetGameObjectOutputBusVolume( m_gameObject, 1.0f );
		eResult = AK::SoundEngine::SetGameObjectAuxSendValues( m_gameObject, aEnvsResult, reverbSentCount );

	return *this;
}

#define INENSITY_GAME_OBJECT_SLOT -2

IMPLEMENT_ENGINE_CLASS( CSoundEmitterComponent );
IMPLEMENT_ENGINE_CLASS( SSoundProperty );
IMPLEMENT_ENGINE_CLASS( SSoundSwitch );

void CSoundEmitterOneShot::Flush()
{
#ifdef USE_WWISE
	if( m_gameObject )
	{
		AkPlayingID ids[ 128 ];
		AkUInt32 count = 128;
		Red::System::MemorySet( &ids, 0, sizeof( ids ) );

		AKRESULT result = AK::SoundEngine::Query::GetPlayingIDsFromGameObject( ( AkGameObjectID ) m_gameObject, count, ids );

		for( AkUInt32 i = 0; i != count; i++ )
		{
			AK::SoundEngine::StopPlayingID( ids[ i ], ( AkInt32 ) 1000 );
		}

		AK::SoundEngine::UnregisterGameObj( ( AkGameObjectID ) m_gameObject );
	}
	m_gameObject = 0;
#endif
}

Bool CSoundEmitterOneShot::SoundFlagGet( ESoundEmitterFlags flag ) const
{
	return ( m_flags & flag ) > 0;
}

void CSoundEmitterOneShot::SoundFlagSet( ESoundEmitterFlags flag, Bool decision )
{
	if( decision )
	{
		m_flags |= flag;
	}
	else
	{
		m_flags &= ( 0xFF ^ flag );
	}
}

Int32 CSoundEmitterOneShot::GetHighestTime( Uint64 objectid )
{
	if( !objectid ) return -1;

	Int32 resultTime = -1;

#ifdef USE_WWISE
	AkUInt32 size = 0;
	AK::SoundEngine::Query::GetPlayingIDsFromGameObject( ( AkGameObjectID ) objectid, size, 0 );
	if( size == 0 ) return -1;
	TDynArray< AkUInt32 > ids;
	ids.Grow( size );
	AK::SoundEngine::Query::GetPlayingIDsFromGameObject( ( AkGameObjectID ) objectid, size, &ids[ 0 ] );
	for( Uint32 i = 0; i != size; ++i )
	{
		AkUInt32 playingId = ids[ i ];
		AkTimeMs currentTime = 0;
		if( AK::SoundEngine::GetSourcePlayPosition( playingId, &currentTime ) == AK_Success )
		{
			if( resultTime < currentTime )
			{
				resultTime = currentTime;
			}
		}
	}
#endif
	return resultTime;
}

Float CSoundEmitterOneShot::GetHighestTime()
{
	return GetHighestTime( m_gameObject ) * 0.001f;
}

Uint64 CSoundEmitterOneShot::SoundEvent( const char* eventName )
{
	if( !m_gameObject )
	{
		m_gameObject = GetGameObjectId();
		if( !m_gameObject ) return 0;
	}

#ifdef USE_WWISE

#ifndef RED_FINAL_BUILD
	if(!Config::cvBreakOnSoundEvent.Get().Empty() && StringAnsi(eventName).ContainsSubstring(UNICODE_TO_ANSI(Config::cvBreakOnSoundEvent.Get().AsChar())))
	{
		RED_BREAKPOINT();
	}
#endif // !RED_FINAL_BUILD


	AkPlayingID result = AK_INVALID_PLAYING_ID;
	if( GSoundSystem != nullptr && GSoundSystem->GetMusicSystem( ).IsEnabled( ) )
	{
		const AkUInt32 flags = s_MusicCallbackFlags | ( SoundFlagGet( SEF_ProcessTiming ) ? AK_EnableGetSourcePlayPosition : 0 );
		result = AK::SoundEngine::PostEvent( eventName, ( AkGameObjectID ) m_gameObject, flags, eventCallback );
	}
	else
	{
		result = AK::SoundEngine::PostEvent( eventName, ( AkGameObjectID ) m_gameObject, SoundFlagGet( SEF_ProcessTiming ) ? AK_EnableGetSourcePlayPosition : NULL );
	}
	if( result == AK_INVALID_PLAYING_ID )
	{
		return 0;
	}
#endif

	return m_gameObject | ( ( Uint64 ) result << 32 );
}

#ifdef USE_WWISE
void sequenceCallback( AkCallbackType in_eType, AkCallbackInfo* in_pCallbackInfo )
{
	if ( in_eType == AK_Duration )
	{
		AkDurationCallbackInfo* info = reinterpret_cast< AkDurationCallbackInfo* > ( in_pCallbackInfo );
		if ( nullptr == info )
		{
			ASSERT( false );
			return;
		}

		SSoundSequenceCallbackData* data = reinterpret_cast< SSoundSequenceCallbackData* > ( info->pCookie );
		if ( nullptr == data )
		{
			ASSERT( false );
			return;
		}

		ASSERT( data->m_duartion == 0, TXT("OMG, mazacz, DEBUG NOW!") ); // it means accessing someone else memory

		Red::Threads::AtomicOps::TAtomic32 newDuration = *reinterpret_cast< Red::Threads::AtomicOps::TAtomic32* > ( info->fDuration > 0.f ? &info->fDuration : &info->fEstimatedDuration );
		Red::Threads::AtomicOps::Exchange32( &data->m_duartion, newDuration ); 
	}
	else if ( in_eType == AK_EndOfDynamicSequenceItem )
	{
		SSoundSequenceCallbackData* data = reinterpret_cast< SSoundSequenceCallbackData* > ( in_pCallbackInfo->pCookie );
		if ( nullptr == data )
		{
			ASSERT( false );
			return;
		}

		Red::Threads::AtomicOps::Increment32( &data->m_itemsCompleted ); 
	}
	else if ( in_eType == AK_EndOfEvent )
	{
		SSoundSequenceCallbackData* data = reinterpret_cast< SSoundSequenceCallbackData* > ( in_pCallbackInfo->pCookie );
		if ( nullptr == data )
		{
			ASSERT( false );
			return;
		}

		Red::Threads::AtomicOps::Increment32( &data->m_eventCompleted ); 
	}
}
#endif

CSoundEmitterOneShot::TSoundSequenceID CSoundEmitterOneShot::PlaySoundSequence( const wchar_t* sequenceName, const wchar_t** sequenceElements, Uint32 sequenceElementsCount, SSoundSequenceCallbackData* callbackData /*= nullptr*/ )
{
	if ( !m_gameObject )
	{
		m_gameObject = GetGameObjectId();
		if ( !m_gameObject )
		{
			return false;
		}
	}

#ifdef USE_WWISE
	AkPlayingID playingID = AK::SoundEngine::DynamicSequence::Open( 
		( AkGameObjectID ) m_gameObject,
		callbackData ? AK_EndOfEvent | AK_EndOfDynamicSequenceItem | AK_Duration : 0,
		callbackData ? sequenceCallback : nullptr,
		callbackData,
		AK::SoundEngine::DynamicSequence::DynamicSequenceType::DynamicSequenceType_NormalTransition 
	);

	AkUniqueID audioNodeID = 0;

	// Locking the playlist for editing
	AK::SoundEngine::DynamicSequence::Playlist* playlist = AK::SoundEngine::DynamicSequence::LockPlaylist( playingID );
	if( playlist )
	{
		audioNodeID = AK::SoundEngine::DynamicDialogue::ResolveDialogueEvent( sequenceName, sequenceElements, sequenceElementsCount );
		playlist->Enqueue( audioNodeID );

		AK::SoundEngine::DynamicSequence::UnlockPlaylist( playingID );
	}

	AK::SoundEngine::DynamicSequence::Play( playingID );
	AK::SoundEngine::DynamicSequence::Close( playingID );
	return playingID;
#else
	return 0;
#endif
}

void CSoundEmitterOneShot::StopSoundSequence( CSoundEmitterOneShot::TSoundSequenceID id )
{
#ifdef USE_WWISE
	AK::SoundEngine::StopPlayingID( id );
#endif
}

void CSoundEmitterOneShot::SoundParameter( const char* name, Float parameterNewValue, Float duration, ESoundParameterCurveType curveType )
{
	if( !m_gameObject )
	{
		m_gameObject = GetGameObjectId();
		if( !m_gameObject ) return;
	}

#ifdef USE_WWISE
	Uint32 parameterName = AK::SoundEngine::GetIDFromString( name );

	CSwitchOrParameterNameHashValue* previousParameter = m_switchesAndParameters.FindPtr( CSwitchOrParameterNameHashValue( false, parameterName ) );
	if( !previousParameter )
	{
		m_switchesAndParameters.PushBack( CSwitchOrParameterNameHashValue( parameterName, parameterNewValue ) );
	}
	else if( previousParameter->getParameterValue() == parameterNewValue )
	{
		return;
	}
	else
	{
		previousParameter->setParameterValue( parameterNewValue );
	}

	
	AKRESULT result = AK::SoundEngine::SetRTPCValue( parameterName, parameterNewValue, ( AkGameObjectID ) m_gameObject, ( AkInt32 ) duration * 1000, ( AkCurveInterpolation ) curveType );
	result = AK_NotImplemented;
#endif
}

Uint64 CSoundEmitterOneShot::GetGameObjectId()
{
	StringAnsi objectName;
#ifndef RED_FINAL_BUILD
	 objectName = GetSoundObjectName();
#endif

	return GetNextGameObjectId(objectName.AsChar());
}

void CSoundEmitterOneShot::SoundSwitch( const char* name, const char* value )
{
	if( !m_gameObject )
	{
		m_gameObject = GetGameObjectId();
		if( !m_gameObject ) return;
	}


#ifdef USE_WWISE
	Uint32 switchName = AK::SoundEngine::GetIDFromString( name );
	Uint32 switchNewValue = AK::SoundEngine::GetIDFromString( value );

	CSwitchOrParameterNameHashValue* previousSwitch = m_switchesAndParameters.FindPtr( CSwitchOrParameterNameHashValue( true, switchName ) );
	if( !previousSwitch )
	{
		m_switchesAndParameters.PushBack( CSwitchOrParameterNameHashValue( switchName, switchNewValue ) );
	}
	else if( previousSwitch->getSwitchValue() == switchNewValue )
	{
		return;
	}
	else
	{
		previousSwitch->setSwitchValue( switchNewValue );
	}
	
	AKRESULT result = AK::SoundEngine::SetSwitch( switchName, switchNewValue, ( AkGameObjectID ) m_gameObject );
	result = AK_NotImplemented;
#endif
}

void CSoundEmitterOneShot::SoundPause()
{
	if( !m_gameObject ) return;

#ifdef USE_WWISE
	AkUInt32 count = 128;
	AkPlayingID ids[128];

	AKRESULT result = AK::SoundEngine::Query::GetPlayingIDsFromGameObject( ( AkGameObjectID ) m_gameObject, count, &ids[ 0 ] );

	for( Uint32 i = 0; i != count; ++i )
	{
		AkUniqueID eventId = AK::SoundEngine::Query::GetEventIDFromPlayingID( ids[ i ] );
		result = AK::SoundEngine::ExecuteActionOnEvent( eventId, AK::SoundEngine::AkActionOnEventType_Pause, ( AkGameObjectID ) m_gameObject );
	}
#endif
}

void CSoundEmitterOneShot::SoundResume()
{
	if( !m_gameObject ) return;

#ifdef USE_WWISE
	AkUInt32 count = 128;
	AkPlayingID ids[128];

	AKRESULT result = AK::SoundEngine::Query::GetPlayingIDsFromGameObject( ( AkGameObjectID ) m_gameObject, count, &ids[ 0 ] );

	for( Uint32 i = 0; i != count; ++i )
	{
		AkUniqueID eventId = AK::SoundEngine::Query::GetEventIDFromPlayingID( ids[ i ] );
		result = AK::SoundEngine::ExecuteActionOnEvent( eventId, AK::SoundEngine::AkActionOnEventType_Resume, ( AkGameObjectID ) m_gameObject );
	}
#endif
}

void CSoundEmitterOneShot::SoundState( const char* name, const char* value )
{
#ifdef USE_WWISE
	AK::SoundEngine::SetState( name, value );
	SOUND_LOG( TXT( "Sound State Group %" ) RED_PRIWas TXT( "to value %" ) RED_PRIWas, name, value );
#endif
}

void CSoundEmitterOneShot::SoundGlobalParameter( const char* name, Float value, Float duration, ESoundParameterCurveType curveType )
{
#ifdef USE_WWISE

	AkUInt32 id = AK::SoundEngine::GetIDFromString( name );
	AKRESULT result = AK::SoundEngine::SetRTPCValue( id, value, AK_INVALID_GAME_OBJECT, ( AkInt32 ) duration * 1000, ( AkCurveInterpolation ) curveType );
	result = AK_NotImplemented;
#endif
}

TFastMuiltiStreamList< CSoundEmitter::SEmitterComponent, void*, nullptr > CSoundEmitter::m_playingEmitters;
double CSoundEmitter::m_lastProcessingMarker = 0.0f;


CSoundEmitter::~CSoundEmitter()
{
	Flush();
}

void CSoundEmitter::Acquire( bool async )
{
	if( m_banksAquired ) return;
	m_banksAquired = true;
	Uint32 banksCount = m_banksDependency.Size();
	for( Uint32 i = 0; i != banksCount; ++i )
	{
		CSoundBank* soundBank = CSoundBank::FindSoundBank( m_banksDependency[ i ] );
		if( !soundBank ) continue;

		soundBank->QueueLoading();
		if( !async ) 
		{
			while( !soundBank->IsLoadingFinished() )	{}
			RED_ASSERT( soundBank->IsLoaded(), TXT("CSoundEmitter::Acquire - sound bank didn't load properly - bank: [%ls] - result [%ls]"), 
				soundBank->GetFileName().AsChar(), soundBank->GetLoadingResultString().AsChar() );
		}
	}
}

Bool CSoundEmitter::LoadedFully()
{
	// If there is no banks to load or acquire, means we are loaded fully
	if( m_banksDependency.Empty() ) return true;
	if( !m_banksAquired ) return false;
	Uint32 banksCount = m_banksDependency.Size();
	for( Uint32 i = 0; i != banksCount; ++i )
	{
		CSoundBank* soundBank = CSoundBank::FindSoundBank( m_banksDependency[ i ] );
		if( !soundBank ) continue;

		if( !soundBank->IsLoaded() ) return false;
	}

	return true;
}

void CSoundEmitter::UnAcquire()
{
	if( !m_banksAquired ) return;
	m_banksAquired = false;
	Uint32 banksCount = m_banksDependency.Size();
	for( Uint32 i = 0; i != banksCount; ++i )
	{
		CSoundBank* soundBank = CSoundBank::FindSoundBank( m_banksDependency[ i ] );
		if( !soundBank ) continue;

		soundBank->Unload();
	}
}

Uint64 CSoundEmitter::SoundEvent( const char* eventName )
{
	if( !m_gameObject )
	{
		m_gameObject = GetGameObjectId();
		if( !m_gameObject ) return 0;
	}

#ifdef USE_WWISE
	AkSoundPosition soundPosition;
	soundPosition.Position = ToAk( GetSoundPlacementMatrix().GetTranslation() );
	soundPosition.Orientation = ToAk( GetSoundPlacementMatrix().GetAxisY() );

	AKRESULT eResult = AK::SoundEngine::SetPosition( ( AkGameObjectID ) m_gameObject, soundPosition );
	eResult = AK_NotImplemented;

	if( m_occlusionEnabled )
	{
		if( SoundIsActive() )
		{
			eResult = AK::SoundEngine::SetObjectObstructionAndOcclusion( ( AkGameObjectID ) m_gameObject, 0, m_currentObstruction, m_currentOcclusion );
		}
		else
		{
			CSoundEmitterOneShot::SoundFlagSet( SEF_WaitForImmediateOcclusionUpdate, true );
		}
	}

#endif
	Uint64 result = CSoundEmitterOneShot::SoundEvent( eventName );

	SoundFlagSet( SEF_WhileStopping, false );

	return result;
}

#ifdef RED_PLATFORM_DURANGO

#ifdef USE_WWISE
extern void * AK::APUAllocHook( size_t in_size, unsigned int in_alignment );
extern void AK::APUFreeHook( void * in_pMemAddress );
void callbackFunc( AkCallbackType in_eType, AkCallbackInfo* in_pCallbackInfo )
{
	if( in_eType == AK_EndOfEvent )
	{
		if( in_pCallbackInfo->pCookie )
		{
			AK::APUFreeHook( in_pCallbackInfo->pCookie );
		}
	}
	eventCallback(in_eType, in_pCallbackInfo);
}
#endif
#endif

void CSoundEmitter::SoundStop( Float duration )
{
	if ( !m_gameObject ) 
	{
		return;
	}

	if ( SoundFlagGet( SEF_WhileStopping ) )
	{
		return;
	}

#ifdef USE_WWISE
	AkPlayingID ids[ 128 ];
	AkUInt32 count = 128;
	Red::System::MemorySet( &ids, 0, sizeof( ids ) );

	AKRESULT result = AK::SoundEngine::Query::GetPlayingIDsFromGameObject( ( AkGameObjectID ) m_gameObject, count, ids );

	for( AkUInt32 i = 0; i != count; i++ )
	{
		AK::SoundEngine::StopPlayingID( ids[ i ], ( AkInt32 ) duration * 1000 );
	}
#endif
}

void CSoundEmitter::SoundStop( Uint64 handle, Float duration )
{
	if( !handle )
	{
		return;
	}
	if ( SoundFlagGet( SEF_WhileStopping ) )
	{
		return;
	}

#ifdef USE_WWISE

	Uint64 gameobject = handle & 0x00000000FFFFFFFF;
	Uint64 playingId = handle >> 32;

	if( m_gameObject != gameobject )
	{
		return;;
	}

	AK::SoundEngine::StopPlayingID( ( AkInt32 ) playingId, ( AkInt32 ) duration * 1000 );

#endif
}

void CSoundEmitter::SoundStopAll()
{
	if ( !m_gameObject ) 
	{
		return;
	}

	if ( SoundFlagGet( SEF_WhileStopping ) )
	{
		return;
	}

#ifdef USE_WWISE
	AK::SoundEngine::StopAll( ( AkGameObjectID ) m_gameObject );
#endif
}

void CSoundEmitter::SoundStop( const char* eventName, Float duration )
{
	if ( !m_gameObject ) 
	{
		return;
	}

	if ( SoundFlagGet( SEF_WhileStopping ) )
	{
		return;
	}

#ifdef USE_WWISE
	AkUInt32 eventId = AK::SoundEngine::GetIDFromString( eventName );

	AkPlayingID ids[ 128 ];
	AkUInt32 count = 128;
	Red::System::MemorySet( &ids, 0, sizeof( ids ) );

	AKRESULT result = AK::SoundEngine::Query::GetPlayingIDsFromGameObject( ( AkGameObjectID ) m_gameObject, count, ids );

	for( AkUInt32 i = 0; i != count; i++ )
	{
		AkUniqueID playingEventId = AK::SoundEngine::Query::GetEventIDFromPlayingID( ids[ i ] );
		if( eventId != playingEventId ) continue;

		AK::SoundEngine::StopPlayingID( ids[ i ], ( AkInt32 ) duration * 1000 );
	}
#endif
}

Uint32 CSoundEmitter::GetIdFromName( const char* eventName )
{
	return AK::SoundEngine::GetIDFromString( eventName );
}

Bool CSoundEmitter::SoundIsActive() const
{
	if ( !m_gameObject ) return false;

#ifdef USE_WWISE
	AkUInt32 count = 0;
	AKRESULT result = AK::SoundEngine::Query::GetPlayingIDsFromGameObject( ( AkGameObjectID ) m_gameObject, count, 0 );
	return count > 0;
#else
	return false;
#endif
}

void CSoundEmitter::SoundSeek( const char* eventName, float percent )
{
	if( !m_gameObject )
	{
		m_gameObject = GetGameObjectId();
		if( !m_gameObject ) return;
	}

#ifdef USE_WWISE
	AkSoundPosition soundPosition;
	soundPosition.Position = ToAk( GetSoundPlacementMatrix().GetTranslation() );
	soundPosition.Orientation = ToAk( GetSoundPlacementMatrix().GetAxisY() );

	AKRESULT result = AK::SoundEngine::SetPosition( ( AkGameObjectID ) m_gameObject, soundPosition );

	if( m_occlusionEnabled )
	{
		if( SoundIsActive() )
		{
			result = AK::SoundEngine::SetObjectObstructionAndOcclusion( ( AkGameObjectID ) m_gameObject, 0, m_currentObstruction, m_currentOcclusion );
		}
		else
		{
			CSoundEmitterOneShot::SoundFlagSet( SEF_WaitForImmediateOcclusionUpdate, true );
		}
	}

	result = AK::SoundEngine::SeekOnEvent( eventName, ( AkGameObjectID ) m_gameObject, percent, false );
#endif

	SoundFlagSet( SEF_WhileStopping, false );
}

void CSoundEmitter::SetOcclusionParameters( Float obstruction, Float occlusion )
{
	m_targetObstruction = obstruction;
	m_targetOcclusion = occlusion;

	if( SoundFlagGet( SEF_WaitForImmediateOcclusionUpdate ) )
	{
		SoundFlagSet( SEF_WaitForImmediateOcclusionUpdate, false );
		m_currentObstruction = m_targetObstruction;
		m_currentOcclusion = m_targetOcclusion;
		UpdateOcclusionParameters();
	}

	m_occlusionUpdateRequired = m_targetObstruction != m_currentObstruction || m_targetOcclusion != m_targetOcclusion;
}

void CSoundEmitter::UpdateOcclusionParameters()
{
	if( !m_gameObject ) return;
#ifdef USE_WWISE
	AKRESULT eResult = AK::SoundEngine::SetObjectObstructionAndOcclusion( ( AkGameObjectID ) m_gameObject, 0, m_currentObstruction, m_currentOcclusion );
#endif
}

void CSoundEmitter::ProcessPlayingEmitters( class CSoundListenerComponent* listener )
{
	PC_SCOPE_PIX( CSoundEmitter ProcessPlayingEmitters )

	m_playingEmitters.Swap();
	m_lastProcessingMarker = Red::System::Clock::GetInstance().GetTimer().GetSeconds();

	CPhysicsBatchQueryManager* physicsBatchManager = nullptr;
	if( GGame->GetActiveWorld() && GGame->GetActiveWorld()->GetPhysicsBatchQueryManager() )
	{
		physicsBatchManager = GGame->GetActiveWorld()->GetPhysicsBatchQueryManager();
	}

	for( ;; )
	{
		SEmitterComponent emitterComponent = m_playingEmitters.Get();
		if( emitterComponent.IsDefault() ) break;
		if( !emitterComponent.IsValid() ) continue;

		CSoundEmitter* emitter = emitterComponent.m_emitter;
		if( !emitter->SoundIsActive() )
		{
			continue;
		}

		emitter->PendProcessing( emitterComponent.m_emitter, emitterComponent.m_parentComponent );

		emitter->ProcessOcclusionRaycast( physicsBatchManager );
		if( listener && !GGame->IsPaused() )
		{
			emitter->PrepareOcclusionRaycast( physicsBatchManager, listener->GetPosition() );			
		}
	}
}

void CSoundEmitter::PrepareOcclusionRaycast( CPhysicsBatchQueryManager* physicsBatchManager, const Vector& listenerPosition )
{
	PC_SCOPE_PIX( CSoundEmitter PrepareOcclusionRaycast );

	if( physicsBatchManager && m_occlusionEnabled )
	{
		const Float occlusionDistanceLimiterSquared = SSoundSettings::m_occlusionDistanceLimiter * SSoundSettings::m_occlusionDistanceLimiter;
		const Float maxDistanceSquared = m_maxDistance * m_maxDistance;
		Vector occlusionPlacement = GetOcclusionPosition();
		const Float newDistanceFromCameraSquared = occlusionPlacement.DistanceSquaredTo( listenerPosition );
		if ( newDistanceFromCameraSquared <= maxDistanceSquared && newDistanceFromCameraSquared <= occlusionDistanceLimiterSquared )
		{
			Vector dir = occlusionPlacement - listenerPosition;
			const float distance = dir.Normalize3();
			if ( distance < m_maxDistance && distance > 0.0f )
			{
				m_occlusionQueryId = physicsBatchManager->SubmitRaycastQuery( 
					listenerPosition,
					dir,
					distance,
					0, 0, SPhysicalFilterData::EPFDF_SoundOccludable,
					EBatchQueryQueryFlag::EQQF_DISTANCE | EBatchQueryQueryFlag::EQQF_BLOCKING_HIT,
					this );
			}
		}
	}
}

void CSoundEmitter::ProcessOcclusionRaycast( CPhysicsBatchQueryManager* physicsBatchManager )
{
	PC_SCOPE_PIX( CSoundEmitter ProcessOcclusionRaycast );

	if( physicsBatchManager && m_occlusionQueryId.IsValid() )
	{
		TDynArray< SRaycastHitResult > results;
		EBatchQueryState state = physicsBatchManager->GetRaycastQueryState( m_occlusionQueryId, results );
		switch ( state )
		{
		case EBatchQueryState::BQS_Processed:
			{
				float resultObstruction = 0.0f;
				float resultOcclusion = 0.0f;
				for( auto result : results )
				{
					CComponent* component = result.m_component.Get();
					if( result.m_wrapper && result.m_physicalMaterial )
					{
						Float attenuation = -1.f;
						result.m_wrapper->GetOcclusionParameters( result.m_actorShapeIndex.m_actorIndex, 0, &attenuation );
						float obstruction = 0.0f;
						float occlusion = 0.0f;
						if( attenuation >= 0.f ) 
						{
							obstruction = result.m_physicalMaterial->m_soundDirectFilter;
							occlusion = result.m_physicalMaterial->m_soundReverbFilter;
							Float hitDistance = result.m_distance;
							if( attenuation > 0.f )
							{
								if( hitDistance > attenuation )
								{
									obstruction = 0.0f;
								}
								else
								{
									obstruction -= obstruction * ( hitDistance / attenuation );
								}
							}
						}

						resultObstruction = Max( resultObstruction, obstruction );
						resultOcclusion = Max( resultOcclusion, occlusion );
					}
				}

				SetOcclusionParameters( resultObstruction, resultOcclusion );
				m_occlusionQueryId = CPhysicsBatchQueryManager::SQueryId::INVALID;
			}
			break;

		case EBatchQueryState::BQS_NotReady:
			break;

		case EBatchQueryState::BQS_NotFound:
		default:
			m_occlusionQueryId = CPhysicsBatchQueryManager::SQueryId::INVALID;
			break;
		}
	}
}

Bool CSoundEmitter::PendProcessing( CSoundEmitter* emitter, THandle< CComponent > component )
{
	if( m_lastProcessMarker >= m_lastProcessingMarker ) return false;
	m_lastProcessMarker = m_lastProcessingMarker;
	m_playingEmitters.Put( SEmitterComponent( emitter, component ) );
	return true;
}

Bool MoveValueTowardsTarget( Float& value, const Float target, const Float step )
{
	if( target < value )
	{
		value -= step;
		if( target > value )
		{
			value = target;
		}
	}
	else if( target > value )
	{
		value += step;
		if( target < value )
		{
			value = target;
		}
	}
	return value == target;
}

void CSoundEmitter::OnTick( Float timeDelta )
{
	if( m_occlusionUpdateRequired )
	{
		Float step = timeDelta * ( 1 / SSoundSettings::m_occlusionInterpolationSpeed );
		Bool obstructionReachedTarget	= MoveValueTowardsTarget( m_currentObstruction, m_targetObstruction, step );
		Bool occlusionReachedTarget		= MoveValueTowardsTarget( m_currentOcclusion, m_targetOcclusion, step );

		m_occlusionUpdateRequired = !obstructionReachedTarget || !occlusionReachedTarget;

		UpdateOcclusionParameters();
	}

#ifdef USE_WWISE
	if ( 0 != m_gameObject )
	{
		AkSoundPosition soundPosition;
		Vector position = GetSoundPlacementMatrix().GetTranslation();

		soundPosition.Position = ToAk( position );
		soundPosition.Orientation = ToAk( GetSoundPlacementMatrix().GetAxisY() );

		AK::SoundEngine::SetPosition( ( AkGameObjectID ) m_gameObject, soundPosition );

	}
#endif
}

TDynArray< CSoundEmitterComponent::SProcessingContext< CSoundEmitterComponent > > CSoundEmitterComponent::m_processingContextPool;

Float CSoundEmitterComponent::sm_inGameMusicListenerDistance = FLT_MAX;

CSoundEmitterComponent::CSoundEmitterComponent() 
	: m_processingContextPoolIndex( -1 )
	, m_intensityBasedLoopStart()
	, m_intensityBasedLoopStop()
	, m_onAttachProcessingStatus( EOAPS_NotAttached )
	, m_activator( 0 )
	, m_isReady( false )
	, m_isInGameMusic( false )
	, m_updateAzimuth( false )

#ifndef NO_EDITOR
	, m_listenerBitmask( 0 )
#endif
{
	SetStreamed( true );
}

CSoundEmitterComponent::~CSoundEmitterComponent()
{
	Flush();
}

Uint64 CSoundEmitterComponent::SoundEvent( const char* eventName, Int32 boneNum )
{
	PendProcessing( this, this );
	if( !m_isReady )
	{
		m_eventsPosponedTilReady.PushBack( SStringAnsiInt32( eventName, boneNum ) );
		return 0;
	}

	SoundFlagSet( ESoundEmitterFlags::SEF_ForceUpdateReverb, true );

	if( boneNum == -1 )
	{
		return CSoundEmitter::SoundEvent( eventName );
	}

	Uint64 gameObject = 0;
	if( !m_gameObjects.Find( boneNum, gameObject ) )
	{
//#ifdef RED_FINAL_BUILD
		gameObject = GetGameObjectId();
/*#else
		StringAnsi name = GetSoundObjectName(); 
		CEntity* entity = GetEntity();
		if( entity )
		{
			CAnimatedComponent* component = entity->GetRootAnimatedComponent();;
			if( component )
			{
				const ISkeletonDataProvider* provider = component->QuerySkeletonDataProvider();
				if( provider )
				{
					TDynArray< ISkeletonDataProvider::BoneInfo > bones;
					provider->GetBones( bones );
					if( ( Int32 ) bones.Size() > boneNum )
					{
						AnsiChar buffer[ RED_NAME_MAX_LENGTH ];
						bones[ boneNum ].m_name.ToChar( buffer, RED_NAME_MAX_LENGTH );
						name += buffer;
					}
				}
			}
		}
		gameObject = GetNewGameObjectId( name );
#endif*/
		if( !gameObject ) return 0;
		m_gameObjects.Insert( boneNum, gameObject );
	}

#ifdef USE_WWISE

	AkSoundPosition soundPosition;

	Matrix mat;
	if( !GetEntity()->GetSubObjectWorldMatrix( boneNum, mat ) )
	{
		mat = GetSoundPlacementMatrix();
	}

	Vector position = mat.GetTranslation();
	soundPosition.Position = ToAk( position );
	Vector direction = mat.GetAxisY();
	soundPosition.Orientation = ToAk( direction );


	AKRESULT eResult = AK::SoundEngine::SetPosition( ( AkGameObjectID ) gameObject, soundPosition );

	if( m_occlusionEnabled )
	{
		if( SoundIsActive() )
		{
			eResult = AK::SoundEngine::SetObjectObstructionAndOcclusion( ( AkGameObjectID ) gameObject, 0, m_currentObstruction, m_currentOcclusion );
		}
		else
		{
			CSoundEmitterOneShot::SoundFlagSet( SEF_WaitForImmediateOcclusionUpdate, true );
		}

		eResult = AK_NotImplemented;
	}

#ifndef RED_FINAL_BUILD
	if(!Config::cvBreakOnSoundEvent.Get().Empty() && StringAnsi(eventName).ContainsSubstring(UNICODE_TO_ANSI(Config::cvBreakOnSoundEvent.Get().AsChar())))
	{
		RED_BREAKPOINT();
	}
#endif // !RED_FINAL_BUILD

	AkPlayingID id = AK_INVALID_PLAYING_ID;
	if( GSoundSystem != nullptr && GSoundSystem->GetMusicSystem( ).IsEnabled( ) )
	{
		const AkUInt32 flags = s_MusicCallbackFlags | ( SoundFlagGet( SEF_ProcessTiming ) ? AK_EnableGetSourcePlayPosition : 0 );
		id = AK::SoundEngine::PostEvent( eventName, ( AkGameObjectID ) gameObject, flags, eventCallback );
	}
	else
	{
		id = AK::SoundEngine::PostEvent( eventName, ( AkGameObjectID ) gameObject, SoundFlagGet( SEF_ProcessTiming ) ? AK_EnableGetSourcePlayPosition : NULL );
	}

	if( id == AK_INVALID_PLAYING_ID )
	{
		return 0;
	}

#endif
	SoundFlagSet( SEF_WhileStopping, false );

	return gameObject | ( ( Uint64 ) id << 32 );

}

Uint64 CSoundEmitterComponent::SoundEvent( const char* eventName, void* data, Uint32 compresion, Uint32 size, float percent )
{
	return SoundEvent( eventName, data, compresion, size, UNOCCLUDED_GAME_OBJECT_SLOT, percent );
}

Uint64 CSoundEmitterComponent::SoundEvent( const char* eventName, void* data, Uint32 compresion, Uint32 size, Int32 boneNum, float percent )
{
	PendProcessing( this, this );

	Uint64 gameObject = 0;
	if( boneNum == -1 )
	{
		if( !m_gameObject )
		{
			m_gameObject = GetGameObjectId();
			if( !m_gameObject ) return 0;
			gameObject = m_gameObject;
		}

	}
	else if( !m_gameObjects.Find( boneNum, gameObject ) )
	{
		gameObject = GetGameObjectId();
		if( !gameObject ) return 0;
		m_gameObjects.Insert( boneNum, gameObject );

	}

	SoundFlagSet( ESoundEmitterFlags::SEF_ForceUpdateReverb, true );

#ifdef USE_WWISE
	AkExternalSourceInfo source;
	source.iExternalSrcCookie = AK::SoundEngine::GetIDFromString( eventName );
	source.idCodec = compresion;

#ifdef RED_PLATFORM_DURANGO
	source.pInMemory = AK::APUAllocHook( size, 2048 );
	if( !source.pInMemory )
	{
		SOUND_LOG( TXT( "OUT OF APUAllocHook MEMORY for %i size" ) , size );
		return 0;
	}
	Red::System::MemoryCopy( source.pInMemory, data, size );
#else
	source.pInMemory = data;
#endif

	source.uiMemorySize = size;

	AkSoundPosition soundPosition;

	Matrix mat;
	if(boneNum < 0 || !GetEntity()->GetSubObjectWorldMatrix( boneNum, mat ) )
	{
		mat = GetSoundPlacementMatrix();
	}

	Vector position = mat.GetTranslation();
	soundPosition.Position = ToAk( position );
	Vector direction = mat.GetAxisY();
	soundPosition.Orientation = ToAk( direction );

	AKRESULT eResult = AK::SoundEngine::SetPosition( ( AkGameObjectID ) gameObject, soundPosition );
	eResult = AK_NotImplemented;

	if( m_occlusionEnabled && boneNum != UNOCCLUDED_GAME_OBJECT_SLOT )
	{
		if( SoundIsActive() )
		{
			eResult = AK::SoundEngine::SetObjectObstructionAndOcclusion( ( AkGameObjectID ) gameObject, 0, m_currentObstruction, m_currentOcclusion );
		}
		else
		{
			CSoundEmitterOneShot::SoundFlagSet( SEF_WaitForImmediateOcclusionUpdate, true );
		}

	}

#ifndef RED_FINAL_BUILD
	if(!Config::cvBreakOnSoundEvent.Get().Empty() && StringAnsi(eventName).ContainsSubstring(UNICODE_TO_ANSI(Config::cvBreakOnSoundEvent.Get().AsChar())))
	{
		RED_BREAKPOINT();
	}
#endif // !RED_FINAL_BUILD

	AkPlayingID id = AK_INVALID_PLAYING_ID;
	if( GSoundSystem != nullptr && GSoundSystem->GetMusicSystem( ).IsEnabled( ) )
	{
#ifdef RED_PLATFORM_DURANGO
		const AkUInt32 flags = ( SoundFlagGet( SEF_ProcessTiming ) ? AK_EnableGetSourcePlayPosition : NULL ) | AK_EndOfEvent | AK_MusicSyncUserCue;
		id = AK::SoundEngine::PostEvent( eventName, ( AkGameObjectID ) gameObject, flags, callbackFunc, source.pInMemory, 1, &source );
#else
		const AkUInt32 flags = s_MusicCallbackFlags | ( SoundFlagGet( SEF_ProcessTiming ) ? AK_EnableGetSourcePlayPosition : 0 );
		id = AK::SoundEngine::PostEvent( eventName, ( AkGameObjectID ) gameObject, flags, eventCallback, nullptr, 1, &source );
#endif
	}
	else
	{
#ifdef RED_PLATFORM_DURANGO
		id = AK::SoundEngine::PostEvent( eventName, ( AkGameObjectID ) gameObject, ( SoundFlagGet( SEF_ProcessTiming ) ? AK_EnableGetSourcePlayPosition : NULL ) | AK_EndOfEvent, callbackFunc, source.pInMemory, 1, &source );
#else
		id = AK::SoundEngine::PostEvent( eventName, ( AkGameObjectID ) gameObject, SoundFlagGet( SEF_ProcessTiming ) ? AK_EnableGetSourcePlayPosition : NULL , nullptr, nullptr, 1, &source );
#endif
	}
	if( id == AK_INVALID_PLAYING_ID )
	{
		return 0;
	}

	if( percent >= 0.05f )
	{
		AKRESULT result = AK::SoundEngine::SeekOnEvent( eventName, ( AkGameObjectID ) gameObject, percent, false );
	}
#endif
	SoundFlagSet( SEF_WhileStopping, false );

	return gameObject | ( ( Uint64 ) id << 32 );
}

Uint64 CSoundEmitterComponent::AuxiliarySoundEvent(const char * eventName, Float speed /*= -1.f*/, Float decelDist /*=0.f*/, Int32 boneNum /*= -1*/)
{
	CAuxiliarySoundEmitter * aux = m_auxiliaryEmitters.FindPtr(boneNum);
	if(!aux)
	{
		m_auxiliaryEmitters.Insert(boneNum, CAuxiliarySoundEmitter(this));
		aux = m_auxiliaryEmitters.FindPtr(boneNum);
	}

	aux->SetSpeed(speed, decelDist);
	
	return aux->SoundEvent(eventName);
}

CAuxiliarySoundEmitter* CSoundEmitterComponent::GetAuxiliaryEmitter(Uint32 boneNum)
{
	CAuxiliarySoundEmitter * aux = m_auxiliaryEmitters.FindPtr(boneNum);
	if(!aux)
	{
		m_auxiliaryEmitters.Insert(boneNum, CAuxiliarySoundEmitter(this));
		aux = m_auxiliaryEmitters.FindPtr(boneNum);
	}

	return aux;
}

void CSoundEmitterComponent::SetParameterOnAllObjects(const char* name, Float value, Float duration)
{
	CAuxiliarySoundEmitter * auxEmitter = m_auxiliaryEmitters.FindPtr(-1);
	if(auxEmitter)
	{
		auxEmitter->SoundParameter(name, value);
	}

	CSoundEmitter::SoundParameter( name, value, duration );

	for(auto entry : m_gameObjects)
	{
		Uint64 gameObject = entry.m_second;
		CAuxiliarySoundEmitter * auxEmitter = m_auxiliaryEmitters.FindPtr((Int32)gameObject);
		if(auxEmitter)
		{
			auxEmitter->SoundParameter(name, value);
		}
		AKRESULT result = AK::SoundEngine::SetRTPCValue( name, value, ( AkGameObjectID ) gameObject, ( AkInt32 ) duration * 1000);
	}
}

void CSoundEmitterComponent::SoundParameter( const char* name, Float value, Float duration, Int32 boneNum, ESoundParameterCurveType curveType )
{
	CAuxiliarySoundEmitter * auxEmitter = m_auxiliaryEmitters.FindPtr(boneNum);
	if(auxEmitter)
	{
		auxEmitter->SoundParameter(name, value);
	}

	if( boneNum == -1 )
	{
		return CSoundEmitter::SoundParameter( name, value, duration );
	}

	Uint64 gameObject = 0;
	if( !m_gameObjects.Find( boneNum, gameObject ) )
	{
//#ifdef RED_FINAL_BUILD
		gameObject = GetGameObjectId();
/*#else
		StringAnsi name = GetSoundObjectName(); 
		CEntity* entity = GetEntity();
		if( entity )
		{
			CAnimatedComponent* component = entity->GetRootAnimatedComponent();;
			if( component )
			{
				const ISkeletonDataProvider* provider = component->QuerySkeletonDataProvider();
				if( provider )
				{
					TDynArray< ISkeletonDataProvider::BoneInfo > bones;
					provider->GetBones( bones );
					if( ( Int32 ) bones.Size() > boneNum )
					{
						AnsiChar buffer[ RED_NAME_MAX_LENGTH ];
						bones[ boneNum ].m_name.ToChar( buffer, RED_NAME_MAX_LENGTH );
						name += buffer;
					}
				}
			}
		}
		gameObject = GetNewGameObjectId( name );
#endif*/
		if( !gameObject ) return;
		m_gameObjects.Insert( boneNum, gameObject );
	}
#ifdef USE_WWISE

	AKRESULT result = AK::SoundEngine::SetRTPCValue( name, value, ( AkGameObjectID ) gameObject, ( AkInt32 ) duration * 1000, ( AkCurveInterpolation ) curveType );
	result = AK_NotImplemented;
#endif
}

void CSoundEmitterComponent::SoundSwitch( const char* name, const char* value, Int32 boneNum )
{
	CAuxiliarySoundEmitter * auxEmitter = m_auxiliaryEmitters.FindPtr(boneNum);
	if(auxEmitter)
	{
		auxEmitter->SoundSwitch(name, value);
	}

	if( boneNum == -1 )
	{
		return CSoundEmitter::SoundSwitch( name, value );
	}

	Uint64 gameObject = 0;
	if( !m_gameObjects.Find( boneNum, gameObject ) )
	{
//#ifdef RED_FINAL_BUILD
		gameObject = GetGameObjectId();
/*#else
		StringAnsi name = GetSoundObjectName(); 
		CEntity* entity = GetEntity();
		if( entity )
		{
			CAnimatedComponent* component = entity->GetRootAnimatedComponent();;
			if( component )
			{
				const ISkeletonDataProvider* provider = component->QuerySkeletonDataProvider();
				if( provider )
				{
					TDynArray< ISkeletonDataProvider::BoneInfo > bones;
					provider->GetBones( bones );
					if( ( Int32 ) bones.Size() > boneNum )
					{
						AnsiChar buffer[ RED_NAME_MAX_LENGTH ];
						bones[ boneNum ].m_name.ToChar( buffer, RED_NAME_MAX_LENGTH );
						name += buffer;
					}
				}
			}
		}
		gameObject = GetNewGameObjectId( name );
#endif*/
		if( !gameObject ) return;
		m_gameObjects.Insert( boneNum, gameObject );
	}

#ifdef USE_WWISE
	AKRESULT result = AK::SoundEngine::SetSwitch( name, value, ( AkGameObjectID ) gameObject );
	result = AK_NotImplemented;
#endif

}

Bool CSoundEmitterComponent::CopySwitchFromRoot( const char* name, Int32 boneNum )
{
	if( !m_gameObject ) return false;
	if( boneNum == -1 ) return false;

#ifdef USE_WWISE
	Uint32 switchNameHash = AK::SoundEngine::GetIDFromString( name );

	AKRESULT result = AK_Success;
	if( CSwitchOrParameterNameHashValue* previousSwitch = m_switchesAndParameters.FindPtr( CSwitchOrParameterNameHashValue( true, switchNameHash ) ) )
	{
		Uint64 gameObject = 0;
		if( !m_gameObjects.Find( boneNum, gameObject ) )
		{
			gameObject = GetGameObjectId();
			if( !gameObject ) return false;
			m_gameObjects.Insert( boneNum, gameObject );
		}
		result = AK::SoundEngine::SetSwitch( switchNameHash, previousSwitch->getSwitchValue(), ( AkGameObjectID ) gameObject );
	}
	return result == AK_Success;
#else
	return false;
#endif
}

Bool CSoundEmitterComponent::CopyPrameterFromRoot( const char* paramterName, Int32 boneNum )
{
	if( !m_gameObject ) return false;
	if( boneNum == -1 ) return false;

#ifdef USE_WWISE
	Uint32 parameterNameHash = AK::SoundEngine::GetIDFromString( paramterName );
	
	AKRESULT result = AK_Success;
	if( CSwitchOrParameterNameHashValue* previousSwitch = m_switchesAndParameters.FindPtr( CSwitchOrParameterNameHashValue( false, parameterNameHash ) ) )
	{
		Uint64 gameObject = 0;
		if( !m_gameObjects.Find( boneNum, gameObject ) )
		{
			gameObject = GetGameObjectId();
			if( !gameObject ) return false;
			m_gameObjects.Insert( boneNum, gameObject );
		}
		result = AK::SoundEngine::SetRTPCValue( parameterNameHash, previousSwitch->getParameterValue(), ( AkGameObjectID ) gameObject );
	}
	return result == AK_Success;
#else
	return false;
#endif
}

Bool CSoundEmitterComponent::SoundIsActive() const
{
	THashMap< Int32, Uint64 >::const_iterator i = m_gameObjects.Begin();
	for ( i = m_gameObjects.Begin(); i != m_gameObjects.End(); ++i )
	{
#ifdef USE_WWISE
		AkUInt32 count = 0;
		AK::SoundEngine::Query::GetPlayingIDsFromGameObject( ( AkGameObjectID ) i->m_second, count, 0 );
		if ( count>0 )
		{
			return true;
		}
#endif
	}

	return CSoundEmitter::SoundIsActive();
}

void CSoundEmitterComponent::SetOcclusionEnable( Bool enable )
{
	m_occlusionEnabled = enable;
	m_targetObstruction = 0.0f;
	m_targetOcclusion = 0.0f;
	m_occlusionUpdateRequired = m_targetObstruction != m_currentObstruction || m_targetOcclusion != m_targetOcclusion;
}

Bool CSoundEmitterComponent::SoundIsActive( const char* eventName ) const
{
#ifdef USE_WWISE
	AkUInt32 id = AK::SoundEngine::GetIDFromString( eventName );

	AkPlayingID ids[ 128 ];
	AkUInt32 count = 128;
	Red::System::MemorySet( &ids, 0, sizeof( ids ) );

	THashMap< Int32, Uint64 >::const_iterator i = m_gameObjects.Begin();
	for( i = m_gameObjects.Begin(); i != m_gameObjects.End(); ++i )
	{
		AKRESULT result = AK::SoundEngine::Query::GetPlayingIDsFromGameObject( ( AkGameObjectID ) i->m_second, count, ids );

		for( AkUInt32 i = 0; i != count; i++ )
		{
			AkUniqueID plyingId = AK::SoundEngine::Query::GetEventIDFromPlayingID( ids[ i ] );
			if( plyingId == id ) return true;
		}
	}

	if( !m_gameObject ) return false;

	AKRESULT result = AK::SoundEngine::Query::GetPlayingIDsFromGameObject( ( AkGameObjectID ) m_gameObject, count, ids );
	for( AkUInt32 i = 0; i != count; i++ )
	{
		AkUniqueID plyingId = AK::SoundEngine::Query::GetEventIDFromPlayingID( ids[ i ] );
		if( plyingId == id ) return true;
	}
#endif
	return false;
}

Bool CSoundEmitterComponent::SoundIsActive( Int32 boneNum ) const
{
#ifdef USE_WWISE
	if( m_gameObjects.Empty() ) return false;

	const Uint64* i = m_gameObjects.FindPtr( boneNum );
	if( i )
	{
		AkUInt32 count = 0;

		AKRESULT result = AK::SoundEngine::Query::GetPlayingIDsFromGameObject( ( AkGameObjectID ) *i, count, 0 );

		return count > 0 ;
	}
#endif
	return false;

}

Bool CSoundEmitterComponent::SoundIsActive( Uint64 handle, Int32 boneNum )
{
#ifdef USE_WWISE
	Uint64 gameobject = handle & 0x00000000FFFFFFFF;
	Uint64 playingId = handle >> 32;

	AkPlayingID ids[ 128 ];
	AkUInt32 count = 128;
	Red::System::MemorySet( &ids, 0, sizeof( ids ) );

	if( m_gameObject == gameobject )
	{
		AKRESULT result = AK::SoundEngine::Query::GetPlayingIDsFromGameObject( gameobject, count, ids );

		for( AkUInt32 i = 0; i != count; i++ )
		{
			if( playingId == ids[ i ] )
			{
				return true;
			}
		}
	}

	THashMap< Int32, Uint64 >::const_iterator i = m_gameObjects.Begin();
	for( i = m_gameObjects.Begin(); i != m_gameObjects.End(); ++i )
	{
		if( gameobject != ( AkGameObjectID ) i->m_second )
		{
			continue;
		}

		AKRESULT result = AK::SoundEngine::Query::GetPlayingIDsFromGameObject( gameobject, count, ids );

		for( AkUInt32 i = 0; i != count; i++ )
		{
			if( playingId == ids[ i ] )
			{
				return true;
			}
		}
	}
#endif

	return false;

}

Uint32 CSoundEmitterComponent::GetSoundsActiveCount()
{
	Uint32 soundActiveCOunt = 0;
#ifdef USE_WWISE
	AkUInt32 count = 0;
	if( m_gameObject )
	{
		AKRESULT result = AK::SoundEngine::Query::GetPlayingIDsFromGameObject( ( AkGameObjectID ) m_gameObject, count, nullptr );
		soundActiveCOunt += count;
	}
	THashMap< Int32, Uint64 >::const_iterator i = m_gameObjects.Begin();
	for( i = m_gameObjects.Begin(); i != m_gameObjects.End(); ++i )
	{
		AKRESULT result = AK::SoundEngine::Query::GetPlayingIDsFromGameObject( ( AkGameObjectID ) i->m_second, count, nullptr );
		soundActiveCOunt += count;
	}
#endif
	return soundActiveCOunt;
}

#ifndef NO_EDITOR
void CSoundEmitterComponent::ListenerSwitch( unsigned char listnerIndex )
{
#ifdef USE_WWISE
	AkUInt32 listenerMask = 1 << listnerIndex;
	THashMap< Int32, Uint64 >::iterator i = m_gameObjects.Begin();
	for( i = m_gameObjects.Begin(); i != m_gameObjects.End(); ++i )
	{
		AKRESULT result = AK::SoundEngine::SetActiveListeners( ( AkGameObjectID ) i->m_second, listenerMask );
		result = AK_NotImplemented;
	}

	if( m_gameObject )
	{
		AKRESULT result = AK::SoundEngine::SetActiveListeners( ( AkGameObjectID ) m_gameObject, listenerMask );
		result = AK_NotImplemented;
	}
#endif
#ifndef NO_EDITOR
	m_listenerBitmask = listenerMask;
#endif
}
#endif

#ifndef NO_EDITOR
Bool CSoundEmitterComponent::IsMaxDistanceReached( const Vector& emitterPosition, float maxDistance, Int32 listenerBitmask )
{
	CSoundListenerComponent * soundListener = nullptr;
	CSoundAdditionalListener * soundAddListener = nullptr;
	if( listenerBitmask )
	{
		soundAddListener = GSoundSystem->GetAdditionalListener( listenerBitmask );
		if ( soundAddListener )
		{
			const Vector& listenerPosition = soundAddListener->GetPosition();
			return listenerPosition.DistanceSquaredTo( emitterPosition ) >= (maxDistance*maxDistance);
		}
	}
	else
	{
		soundListener = GSoundSystem->GetSoundListner( );
		if ( soundListener )
		{
			const Vector& listenerPosition = soundListener->GetPosition();
			return listenerPosition.DistanceSquaredTo( emitterPosition ) >= (maxDistance*maxDistance);
		}
	}

	// litener position unknown, assume max distance not reached yet
	return false;
}
#else
Bool CSoundEmitterComponent::IsMaxDistanceReached( const Vector& emitterPosition, float maxDistance )
{
	CSoundListenerComponent * soundListener = nullptr;
	soundListener = GSoundSystem->GetSoundListner();
	if ( soundListener )
	{
		const Vector& listenerPosition = soundListener->GetPosition();
		return listenerPosition.DistanceSquaredTo( emitterPosition ) >= (maxDistance*maxDistance);
	}
	// litener position unknown, assume max distance not reached yet
	return false;
}
#endif
CSoundEmitterComponent* CSoundEmitterComponent::GetSoundEmitterIfMaxDistanceIsntReached( CEntity* entity, float maxDistance )
{
#ifndef NO_EDITOR
	if( CLayer* layer = entity->GetLayer() )
	{
		if( CWorld* world = layer->GetWorld() )
		{
			if( world->GetPreviewWorldFlag() ) 
			{
				return entity->GetSoundEmitterComponent();
			}
		}
	}
#endif

	CSoundEmitterComponent* soundEmitterComponent = entity->GetSoundEmitterComponent( false );
	if( soundEmitterComponent )
	{
#ifndef NO_EDITOR
		if( CSoundEmitterComponent::IsMaxDistanceReached( soundEmitterComponent->GetLocalToWorld().GetTranslationRef(), maxDistance, soundEmitterComponent->GetListenerMask() ) )
#else
		if( CSoundEmitterComponent::IsMaxDistanceReached( soundEmitterComponent->GetLocalToWorld().GetTranslationRef(), maxDistance ) )
#endif
		{
			return nullptr;
		}
	}
	if( !soundEmitterComponent )
	{
		if( CSoundEmitterComponent::IsMaxDistanceReached( entity->GetLocalToWorld().GetTranslationRef(), maxDistance ) )
		{
			return nullptr;
		}

		soundEmitterComponent = entity->GetSoundEmitterComponent();
		if( !soundEmitterComponent ) return nullptr;
	}


	return soundEmitterComponent;
}

void CSoundEmitterComponent::OnAttached( CWorld* world )
{
	PC_SCOPE_PIX( CSoundEmitterComponent_OnAttached );

	TBaseClass::OnAttached( world );

	{
		const Vector& position = GetLocalToWorld().GetTranslationRef();

		//locate empty index;
		RED_FATAL_ASSERT( m_processingContextPoolIndex == -1, "attaching sound emitter component twice");
		for( Uint32 i = 0; i != m_processingContextPool.Size(); ++i )
		{
			SProcessingContext< CSoundEmitterComponent >& context = m_processingContextPool[ i ];
			if( context.m_componentHandle ) continue;
			m_processingContextPoolIndex = i;
			m_processingContextPool[ m_processingContextPoolIndex ] = SProcessingContext< CSoundEmitterComponent >( this, position, m_maxDistance );
			break;
		}
		if( m_processingContextPoolIndex == -1 )
		{
			m_processingContextPoolIndex = m_processingContextPool.Size();
			m_processingContextPool.PushBack( SProcessingContext< CSoundEmitterComponent >( this, position, m_maxDistance ) );
		}
	}

#ifndef NO_EDITOR_FRAGMENTS
	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_SoundReverb );
	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_Sound );
	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_SoundListener );
#endif
	if(!m_listenerOverride.Empty())
	{
		const Matrix& mat = GetLocalToWorld();
		GSoundSystem->RegisterListenerOverride(m_listenerOverride, mat.GetTranslation(), mat.GetAxisZ(), mat.GetAxisY());
	}

	if( m_banksDependency.Empty() )
	{
		m_isReady = true;
	}
	if( m_eventsOnAttach.Size() )
	{
		const Float maxDistanceSquared = m_maxDistance * m_maxDistance;
		const Float newDistanceFromCameraSquared = GetSoundPlacementMatrix().GetTranslation().DistanceSquaredTo( GSoundSystem->GetListenerPosition() );
		if( newDistanceFromCameraSquared <= maxDistanceSquared )
		{
			Acquire( true );
			ProcessOnAttach();
		}

		if( m_onAttachProcessingStatus != EOAPS_Processed )
		{
			m_onAttachProcessingStatus = EOAPS_Postponed;
		}
	}
	else
	{
		m_onAttachProcessingStatus = EOAPS_Processed;
	}
}

void CSoundEmitterComponent::ProcessOnAttach()
{
	if( m_isReady )
	{
		ProcessOnAttachEvents();
		ProcessOnAttachProperties();
		ProcessOnAttachSwitches();

		m_onAttachProcessingStatus = EOAPS_Processed;
	}
}

void CSoundEmitterComponent::ProcessOnAttachEvents()
{	
	for( Uint32 i = 0; i != m_eventsOnAttach.Size(); ++i )
	{
		const StringAnsi& string = m_eventsOnAttach[ i ];
		if( !string.Empty() )
		{
			SSoundEventConversionHelper seConversion;
			if( !GSoundSystem->CheckSoundEventConversion( string, seConversion ) )
			{
				SoundEvent( string.AsChar() );
			}
			else
			{
				// DEPRECATED - implemented to fix issues with entities that can't be changed anymore
				switch( seConversion.m_type )
				{
				case SSoundEventConversionHelper::ECT_Switch:
					SoundSwitch( seConversion.m_name.AsChar(), seConversion.m_switchValue.AsChar() );
					break;
				case SSoundEventConversionHelper::ECT_RTPC:
					SoundParameter( seConversion.m_name.AsChar(), seConversion.m_rtpcValue );
					break;
				default:
					break;
				}
			}

		}
	}
}

void CSoundEmitterComponent::ProcessOnAttachSwitches()
{
	for( Uint32 i = 0; i != m_switchesOnAttach.Size(); ++i )
	{
		SSoundSwitch sw = m_switchesOnAttach[i];
		SoundSwitch( sw.m_name.AsChar(), sw.m_value.AsChar() );
	}
}

void CSoundEmitterComponent::ProcessOnAttachProperties()
{
	for( Uint32 i = 0; i != m_rtpcsOnAttach.Size(); ++i )
	{
		SSoundProperty sw = m_rtpcsOnAttach[i];
		SoundParameter( sw.m_name.AsChar(), sw.m_value );
	}
}

void CSoundEmitterComponent::OnDetached( CWorld* world )
{
	TBaseClass::OnDetached( world );

	RED_FATAL_ASSERT( m_processingContextPoolIndex != -1, "detaching sound emitter component which wherent on processing pool");
	RED_FATAL_ASSERT( ( Int32 ) m_processingContextPool.Size() >= m_processingContextPoolIndex, "sound emitter component processin pool index higher than pool size");

	m_processingContextPool[ m_processingContextPoolIndex ].Clear();
	m_processingContextPoolIndex = -1;

	if ( m_activator )
	{
		m_activator->Remove();
		m_activator->Release();
		m_activator = 0;
	}

#ifndef NO_EDITOR_FRAGMENTS
	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_SoundReverb );
	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_Sound );
	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_SoundListener );
#endif

	if(!m_listenerOverride.Empty())
	{
		GSoundSystem->UnregisterListenerOverride(m_listenerOverride);
	}

	if( !SoundFlagGet( SEF_WhileStopping ) )
	{
		SoundFlagSet( SEF_WhileStopping, true );
#ifdef USE_WWISE
		if ( m_gameObject ) 
		{
			AkPlayingID ids[ 128 ];
			AkUInt32 count = 128;
			Red::System::MemorySet( &ids, 0, sizeof( ids ) );
			AKRESULT result = AK::SoundEngine::Query::GetPlayingIDsFromGameObject( ( AkGameObjectID ) m_gameObject, count, ids );

			for( AkUInt32 i = 0; i != count; i++ )
			{
				AK::SoundEngine::StopPlayingID( ids[ i ], ( AkInt32 ) 1000 );
			}

		}
		for( THashMap< Int32, Uint64 >::iterator i = m_gameObjects.Begin(); i != m_gameObjects.End(); ++i )
		{
			if( i->m_second == INENSITY_GAME_OBJECT_SLOT ) continue;
			AkPlayingID ids[ 128 ];
			AkUInt32 count = 128;
			Red::System::MemorySet( &ids, 0, sizeof( ids ) );
			AKRESULT result = AK::SoundEngine::Query::GetPlayingIDsFromGameObject( ( AkGameObjectID ) i->m_second, count, ids );

			for( AkUInt32 i = 0; i != count; i++ )
			{
				AK::SoundEngine::StopPlayingID( ids[ i ], ( AkInt32 ) 1000 );
			}
		}

#endif
	}

	for( Uint32 i = 0; i != m_eventsOnDetach.Size(); ++i )
	{
		StringAnsi string = m_eventsOnDetach[ i ];
		if( !string.Empty() )
		{
			SoundEvent( string.AsChar() );
		}
	}

	if( m_intensityBasedLoopStart.Size() && SoundIsActive() )
	{
		if( m_intensityBasedLoopStop.Empty() )
		{
			SoundStopAll();
		}
		else
		{
			SoundEvent( m_intensityBasedLoopStop.AsChar(), INENSITY_GAME_OBJECT_SLOT );
		}
	}

	Flush();

	for(auto &auxEmitter : m_auxiliaryEmitters)
	{
		auxEmitter.m_second.Flush();
	}

	m_onAttachProcessingStatus = EOAPS_NotAttached;
	m_isReady = false;
}

void CSoundEmitterComponent::UpdateOcclusionParameters()
{
	CSoundEmitter::UpdateOcclusionParameters();
#ifdef USE_WWISE
	for ( auto i = m_gameObjects.Begin(); i != m_gameObjects.End(); ++i )
	{
		if( i->m_first == UNOCCLUDED_GAME_OBJECT_SLOT ) continue;
		AKRESULT eResult = AK::SoundEngine::SetObjectObstructionAndOcclusion( ( AkGameObjectID ) i->m_second, 0, m_currentObstruction, m_currentOcclusion );
	}
#endif
}

void CSoundEmitterComponent::OnUpdateTransformComponent( SUpdateTransformContext& context, const Matrix& prevLocalToWorld )
{
	TBaseClass::OnUpdateTransformComponent( context, prevLocalToWorld );

	if ( m_processingContextPoolIndex >= 0 )
	{
		const Vector& pos = GetLocalToWorld().GetTranslationRef();
		SProcessingContext< CSoundEmitterComponent >& context = m_processingContextPool[ m_processingContextPoolIndex ];
		context.m_x = pos.X;
		context.m_y = pos.Y;
		context.m_z = pos.Z;
	}
}

void CSoundEmitterComponent::OnMaxDistanceEntered()
{
	if( m_banksDependency.Size() )
	{
		Acquire( true );
	}

	if( m_onAttachProcessingStatus == EOAPS_Postponed )
	{
		ProcessOnAttach();
	}

}

void CSoundEmitterComponent::OnMaxDistanceExited()
{
	if( m_intensityBasedLoopStart.Size() && m_isReady )
	{
		Bool intensityGameObjectActive = SoundIsActive( INENSITY_GAME_OBJECT_SLOT );
		if( intensityGameObjectActive )
		{
			if( m_intensityBasedLoopStop.Empty() )
			{
				SoundStopAll();
			}
			else
			{
				SoundEvent( m_intensityBasedLoopStop.AsChar(), INENSITY_GAME_OBJECT_SLOT );
			}
		}
	}

#ifdef NO_EDITOR
#ifdef USE_WWISE
/*	THashMap< Int32, Uint64 >::iterator i = m_gameObjects.Begin();
	while( i != m_gameObjects.End() )
	{
		AkUInt32 count = 0;
		AK::SoundEngine::Query::GetPlayingIDsFromGameObject( ( AkGameObjectID ) i->m_second, count, 0 );

		if( count )
		{
			++i;
			continue;
		}

		AK::SoundEngine::UnregisterGameObj( ( AkGameObjectID ) i->m_second );
		m_gameObjects.Erase( i );
		i = m_gameObjects.Begin();
	}*/
#endif
#endif

	if( m_banksDependency.Size() )
	{
		UnAcquire();
	}
}

void CSoundEmitterComponent::OnTick( SProcessingContext< CSoundEmitterComponent >* context, Float timeDelta )
{
	CSoundEmitter::OnTick( timeDelta );

	TDynArray<STimedSoundEvent*> eventsToRemove;
	for (auto &timedEvent : m_timedSoundEvents)
	{
		timedEvent.currentTime += timeDelta;
		if(timedEvent.updateTimeParameter && timedEvent.startEventId != AK_INVALID_PLAYING_ID)
		{
			SoundParameter("eventTime", timedEvent.currentTime, 0.f, timedEvent.boneNum);
		}

		if(timedEvent.currentTime > timedEvent.duration)
		{
			SoundEvent(UNICODE_TO_ANSI(timedEvent.onStopEvent.AsChar()), timedEvent.boneNum);
			eventsToRemove.PushBack(&timedEvent);
		}
		
	}

	for(auto event : eventsToRemove)
	{
		m_timedSoundEvents.Erase(event);
	}

	if( !m_isReady )
	{
		if( LoadedFully() )
		{
			m_isReady = true;
			for( auto i = m_eventsPosponedTilReady.Begin(); i != m_eventsPosponedTilReady.End(); ++i )
			{
				SStringAnsiInt32& posponed = *i;
				SoundEvent( posponed.m_eventName.AsChar(), posponed.m_boneId );
			}
			m_eventsPosponedTilReady.Clear();
		}
	}
	else
	{
		if( m_onAttachProcessingStatus == EOAPS_Postponed )
		{
			ProcessOnAttach();
		}
	}

	for(auto &auxEmitter : m_auxiliaryEmitters)
	{
		auxEmitter.m_second.Update(timeDelta);
	}

	CEntity* entity = GetEntity();
#ifdef USE_WWISE
	{
		Int32 subObjectsCount = entity->GetSoundSubObjectCount();
		for( THashMap< Int32, Uint64 >::iterator i = m_gameObjects.Begin(); i != m_gameObjects.End(); ++i )
		{
			Matrix mat;
			Int32 bone = i->m_first;
			if( bone >= subObjectsCount ) continue;
			if( bone < 0 )
			{
				mat = GetSoundPlacementMatrix();
			}
			else if( !entity->GetSubObjectWorldMatrix( bone, mat ) )
			{
				mat = GetSoundPlacementMatrix();
			}

			AkSoundPosition soundPosition;

			Vector position = mat.GetTranslation();
			soundPosition.Position = ToAk( position );
			Vector direction = mat.GetAxisY();
			soundPosition.Orientation = ToAk( direction );

			AKRESULT eResult = AK::SoundEngine::SetPosition( ( AkGameObjectID ) i->m_second, soundPosition );
			eResult = AK_NotImplemented;
		}
	}

	Bool positionChanged = context->m_previousResultDistanceSquared != context->m_resultDistanceSquared;

	if(positionChanged && m_updateAzimuth)
	{
		Float azimuth = MathUtils::VectorUtils::GetAngleDegBetweenVectors( GSoundSystem->GetListenerDirection(), GetPosition() - GSoundSystem->GetPanningListenerPosition() );
		SoundParameter("azimuth", azimuth);
	}

	if( SoundIsActive() )
	{
		Bool shouldCheckReverbs = positionChanged;
		if( SoundFlagGet( ESoundEmitterFlags::SEF_ForceUpdateReverb ) )
		{
			SoundFlagSet( ESoundEmitterFlags::SEF_ForceUpdateReverb, false );
			shouldCheckReverbs = true;
		}

		if( !m_activator )
		{
			CTriggerActivatorInfo initInfo;
			initInfo.m_channels = TC_SoundReverbArea;
			initInfo.m_component = this;
#ifndef RED_TRIGGER_SYSTEM_NO_DEBUG_NAMES
			initInfo.m_debugName = TXT("SoundEmitter_"); // please keep the debug names
			initInfo.m_debugName += GetName();
#endif
			initInfo.m_extents = Vector::ZEROS;
			initInfo.m_localToWorld = Matrix::IDENTITY; // todo: it would be nice to set the initial position correctly
			m_activator = GetWorld()->GetTriggerManager()->CreateActivator(initInfo);
			shouldCheckReverbs = true;
		}

		CSoundListenerComponent* listner = GSoundSystem->GetSoundListner();

		if(m_isInGameMusic)
		{
			Float distance = (GetWorldPosition() - GSoundSystem->GetListenerPosition()).Mag3();
			if(distance < sm_inGameMusicListenerDistance)
			{
				sm_inGameMusicListenerDistance = distance;
			}
		}

		if( !shouldCheckReverbs && listner )
		{
			shouldCheckReverbs = listner->GetInfluencedTriggerCount() > 0;
		}

		if( shouldCheckReverbs )
		{
			Uint32 reverbSentCount = 0;
			AkAuxSendValue aEnvs[ 4 ];

			Vector3 position( context->m_x, context->m_y, context->m_z );
			m_activator->Move( IntegerVector4( position ) );

			struct SSoundAmbientAreaComponentVector
			{
				CSoundAmbientAreaComponent* m_area;
				float m_listenerTriggerPenetration;
				float m_soundTriggerPenetration;

				SSoundAmbientAreaComponentVector() : m_area( 0 ), m_listenerTriggerPenetration( -1.0f ), m_soundTriggerPenetration( -1.0f ) {}
				SSoundAmbientAreaComponentVector( CSoundAmbientAreaComponent* area ) : m_area( area ), m_listenerTriggerPenetration( -1.0f ), m_soundTriggerPenetration( -1.0f ) {}
				SSoundAmbientAreaComponentVector( CSoundAmbientAreaComponent* area, float listnerTriggerPenetration ) : m_area( area ), m_listenerTriggerPenetration( listnerTriggerPenetration ), m_soundTriggerPenetration( -1.0f ) {}
				SSoundAmbientAreaComponentVector( CSoundAmbientAreaComponent* area, float listnerTriggerPenetration, float soundTriggerPenetration ) : m_area( area ), m_listenerTriggerPenetration( listnerTriggerPenetration ), m_soundTriggerPenetration( soundTriggerPenetration ) {}

				RED_INLINE bool operator==( const SSoundAmbientAreaComponentVector& entry ) const
				{
					return m_area == entry.m_area;
				}

				RED_INLINE SSoundAmbientAreaComponentVector& operator+=( const SSoundAmbientAreaComponentVector& entry )
				{
					m_soundTriggerPenetration += entry.m_soundTriggerPenetration;
					return *this;
				}
			};

			static TDynArray< SSoundAmbientAreaComponentVector > ambientAreas;
			Uint32 ambientAreasCount = m_activator ? m_activator->GetNumOccupiedTrigger() : 0 + listner ? listner->GetNumOccupiedTrigger() : 0;
			ambientAreas.ClearFast();
			ambientAreas.Reserve( ambientAreasCount );

			CSoundAmbientAreaComponent* highestPriorityListnerAmbient = 0;
			if( listner )
			{
				Vector listenerPosition = GSoundSystem->GetSoundListner()->GetPosition();
				for( Uint32 index = 0; index != listner->GetNumOccupiedTrigger(); ++index )
					if( CSoundAmbientAreaComponent* ambientComponent = Cast< CSoundAmbientAreaComponent >( listner->GetOccupiedTrigger( index )->GetComponent() ) )
					{
						if( ambientComponent->GetReverbName().Empty() ) continue;
						Int32 priority = ambientComponent->GetAmbientPriority();
						if( !highestPriorityListnerAmbient || highestPriorityListnerAmbient->GetAmbientPriority() < priority )
						{
							highestPriorityListnerAmbient = ambientComponent;
						}
						float listenerTriggerPenetration = ambientComponent->CalcPenetrationFraction( listenerPosition );
						ambientAreas.PushBack( SSoundAmbientAreaComponentVector( ambientComponent, listenerTriggerPenetration ) );
					}
			}

			const Uint32 numObjects = m_activator ? m_activator->GetNumOccupiedTrigger() : 0;
			for( Uint32 index = 0; index != numObjects; ++index )
				if( CSoundAmbientAreaComponent* ambientComponent = Cast< CSoundAmbientAreaComponent >( m_activator->GetOccupiedTrigger( index )->GetComponent() ) )
				{
					if( ambientComponent->GetReverbName().Empty() ) continue;
					float soundTriggerPenetration = ambientComponent->CalcPenetrationFraction( position );
					SSoundAmbientAreaComponentVector* result = ambientAreas.FindPtr( SSoundAmbientAreaComponentVector( ambientComponent ) );
					if( result )
					{
						if( result->m_soundTriggerPenetration == -1 )
						{
							result->m_soundTriggerPenetration = soundTriggerPenetration;
						}
					}
					else
					{
						ambientAreas.PushBack( SSoundAmbientAreaComponentVector( ambientComponent, 0.0f, soundTriggerPenetration ) );
					}
				}


#ifndef NO_EDITOR_FRAGMENTS
				if( m_debugBuffer.Size() )
				{
					m_debugBuffer.Clear();
					m_debugBuffer += TXT("\n");
				}
#endif

				for( Uint32 j = 0; j != ambientAreas.Size() && reverbSentCount < 4; ++j )
				{
					const SSoundAmbientAreaComponentVector& soundAmbientAreaComponentVector = ambientAreas[ j ];
					const char* name = soundAmbientAreaComponentVector.m_area->GetReverbName().AsChar();
					AkSwitchStateID reverbId = AK::SoundEngine::GetIDFromString( name );

					for( Uint32 existingIndex = 0; existingIndex != reverbSentCount; ++existingIndex )
						if( aEnvs[ existingIndex ].auxBusID == reverbId )
						{
							reverbId = 0;
							break;
						}

						if( reverbId )
						{
							aEnvs[ reverbSentCount ].auxBusID = reverbId;
							float resultWetFilter = 0.0f;
							float soundPenetration = soundAmbientAreaComponentVector.m_soundTriggerPenetration;
							float listenerPenetration = soundAmbientAreaComponentVector.m_listenerTriggerPenetration;
							float outerReverbRatio = soundAmbientAreaComponentVector.m_area->GetOuterReverbRatio();
							if( soundAmbientAreaComponentVector.m_soundTriggerPenetration <= 0.0f )
							{
								resultWetFilter = listenerPenetration;
							}
							else if( soundAmbientAreaComponentVector.m_listenerTriggerPenetration <= 0.0f )
							{
								resultWetFilter = soundPenetration * outerReverbRatio;
							}
							else
							{
								resultWetFilter = soundPenetration * outerReverbRatio;
								resultWetFilter += ( 1.0f - outerReverbRatio ) * listenerPenetration;
							}

							aEnvs[ reverbSentCount ].fControlValue = resultWetFilter;

#ifndef NO_EDITOR_FRAGMENTS
							if( m_debugBuffer.Size() )
							{
								m_debugBuffer += String::Printf( TXT( "%s %f\n" ), ANSI_TO_UNICODE( name ), aEnvs[ reverbSentCount ].fControlValue );
							}
#endif
							reverbSentCount++;
						}
				}

				AKRESULT eResult = AK::SoundEngine::SetGameObjectAuxSendValues( ( AkGameObjectID ) m_gameObject, 0, 0 );

				for( THashMap< Int32, Uint64 >::iterator i = m_gameObjects.Begin(); i != m_gameObjects.End(); ++i )
				{
					eResult = AK::SoundEngine::SetGameObjectAuxSendValues( ( AkGameObjectID ) i->m_second, 0, 0 );
					eResult = AK_NotImplemented;
				}

				AkAuxSendValue* aEnvsResult = reverbSentCount ? &aEnvs[ 0 ] : 0;
				if( m_gameObject )
				{
					AkGameObjectID gameObject = ( AkGameObjectID ) m_gameObject;
					eResult = AK::SoundEngine::SetGameObjectOutputBusVolume( gameObject, 1.0f );
					eResult = AK::SoundEngine::SetGameObjectAuxSendValues( gameObject, aEnvsResult, reverbSentCount );
					eResult = AK_NotImplemented;
				}

				for( THashMap< Int32, Uint64 >::iterator i = m_gameObjects.Begin(); i != m_gameObjects.End(); ++i )
				{
					AkGameObjectID gameObject = ( AkGameObjectID ) i->m_second;
					eResult = AK::SoundEngine::SetGameObjectOutputBusVolume( gameObject, 1.0f );
					eResult = AK::SoundEngine::SetGameObjectAuxSendValues( gameObject, aEnvsResult, reverbSentCount );
					eResult = AK_NotImplemented;
				}

		}

	}
	else if( m_activator )
	{
		m_activator->Remove();
		m_activator->Release();
		m_activator = 0;
	}
#endif

	if( m_intensityBasedLoopStart.Empty() || !m_isReady ) return;

	Bool intensityGameObjectActive = SoundIsActive( INENSITY_GAME_OBJECT_SLOT );
	if( !intensityGameObjectActive )
	{
		SoundEvent( m_intensityBasedLoopStart.AsChar(), INENSITY_GAME_OBJECT_SLOT );
	}

	if( positionChanged )
	{
#ifdef USE_WWISE
		const Matrix& mat = GetLocalToWorld();

		if(!m_listenerOverride.Empty())
		{
			GSoundSystem->RegisterListenerOverride(m_listenerOverride, mat.GetTranslation(), mat.GetAxisZ(), mat.GetAxisY());
		}


		Uint64* gameObject = m_gameObjects.FindPtr( INENSITY_GAME_OBJECT_SLOT );
		if( gameObject )
		{
			AkSoundPosition soundPosition;
			soundPosition.Position = ToAk( mat.GetTranslation() );
			Vector direction = mat.GetAxisY();
			soundPosition.Orientation = ToAk( direction );			
			AKRESULT eResult = AK::SoundEngine::SetPosition( ( AkGameObjectID ) *gameObject, soundPosition );
			eResult = AK_NotImplemented;

		}
#endif
	}

	if( m_intensityParameter.Size() )
	{
		Float maxForce = 0.0f;
		CEntity* entity = GetEntity();
		const TDynArray< CComponent* >& components = entity->GetComponents();
		for( Uint32 i = 0; i != components.Size(); ++i )
		{
			CPhysicsWrapperInterface* wrapper = components[ i ]->GetPhysicsRigidBodyWrapper();
			if( !wrapper ) continue;
			Float force = wrapper->GetMotionIntensity();
			if( force <= maxForce ) continue;
			maxForce = force;
		}
		SoundParameter( m_intensityParameter.AsChar(), maxForce, 0.0f, INENSITY_GAME_OBJECT_SLOT );
	}
}

void CSoundEmitterComponent::ProcessTick( Float timeDelta, const Vector& position )
{
	PC_SCOPE_PIX( ProcessTick );

	static TDynArray< SProcessingContext< CSoundEmitterComponent >* > willBeToProcessed;
	willBeToProcessed.ClearFast();
	static TDynArray< SProcessingContext< CSoundEmitterComponent >* > isToProcess;
	isToProcess.ClearFast();
	static TDynArray< SProcessingContext< CSoundEmitterComponent >* > wasToProcessed;
	wasToProcessed.ClearFast();

	Uint32 size = m_processingContextPool.Size();

	float x = position.X;
	float y = position.Y;
	float z = position.Z;

	SProcessingContext< CSoundEmitterComponent >* context = m_processingContextPool.TypedData();
	for( Uint32 i = 0; i != size; ++i)
	{
		if( context->m_desiredDistanceSquared >= 0 )
		{
			Bool wasProcessedFlag = context->m_previousResultDistanceSquared <= context->m_desiredDistanceSquared;
			context->m_previousResultDistanceSquared = context->m_resultDistanceSquared;
#ifndef NO_EDITOR
			if( context->m_componentHandle )
			{
				Int32 addListenerBitmask = context->m_componentHandle->GetListenerMask();
				if( addListenerBitmask )
				{
					CSoundAdditionalListener* addList = GSoundSystem->GetAdditionalListener( addListenerBitmask );
					if( addList )
					{
						Vector posAdd = addList->GetPosition();
						x = posAdd.X;
						y = posAdd.Y;
						z = posAdd.Z;
					}
				}
			}

#endif
			
			Float pos = context->m_x - x;
			context->m_resultDistanceSquared = pos * pos;
			pos = context->m_y - y;
			context->m_resultDistanceSquared += pos * pos;
			pos = context->m_z - z;
			context->m_resultDistanceSquared += pos * pos;
			Bool willBeProcessedFlag = context->m_resultDistanceSquared <= context->m_desiredDistanceSquared;
			if( !wasProcessedFlag && !willBeProcessedFlag )
			{
			}
			else if( wasProcessedFlag && willBeProcessedFlag )
			{
				isToProcess.PushBack( context );
			}
			else if( wasProcessedFlag )
			{
				wasToProcessed.PushBack( context );
			}
			else //if( willBeProcessedFlag )
			{
				willBeToProcessed.PushBack( context );
			}
		}
		++context;
	}


	{
		PC_SCOPE_PIX( ProcessTickwillBeToProcessed );
		for( SProcessingContext< CSoundEmitterComponent >* context : willBeToProcessed )
		{
			if( CSoundEmitterComponent* component = context->m_componentHandle.Get() )
			{
				component->OnMaxDistanceEntered();
			}
		}
	}

	{
		PC_SCOPE_PIX( ProcessTickisToProcess );
		sm_inGameMusicListenerDistance = FLT_MAX;
		for( SProcessingContext< CSoundEmitterComponent >* context : isToProcess )
		{
			if( CSoundEmitterComponent* component = context->m_componentHandle.Get() )
			{
				component->OnTick( context, timeDelta );
			}
		}
		GSoundSystem->SoundParameter("inGameMusicDistance", sm_inGameMusicListenerDistance);
	}


	{
		PC_SCOPE_PIX( ProcessTickwillBeToProcessed );
		for( SProcessingContext< CSoundEmitterComponent >* context : wasToProcessed )
		{
			if( CSoundEmitterComponent* component = context->m_componentHandle.Get() )
			{
				component->OnMaxDistanceExited();
			}
		}
	}
}

Uint32 CSoundEmitterComponent::GetMinimumStreamingDistance() const
{
	return (Uint32)Red::Math::MRound( m_maxDistance*1.1f );
}

void CSoundEmitterComponent::NotifyFootstepEvent(StringAnsi eventName, Int64 bone)
{
	m_previousFootstep.bone = bone;
	m_previousFootstep.eventName = eventName;
	m_previousFootstep.time = GGame->GetEngineTime();
}

void CSoundEmitterComponent::NotifyAnimEvent(StringAnsi eventName, Int64 bone)
{
	m_previousAnimEvent.bone = bone;
	m_previousAnimEvent.eventName = eventName;
	m_previousAnimEvent.time = GGame->GetEngineTime();
}

Float CSoundEmitterComponent::GetDistanceFromCameraSquared() const
{
	if( m_processingContextPoolIndex == -1 )
	{
		return 0.0f;
	}

	SProcessingContext< CSoundEmitterComponent >& processingContext = m_processingContextPool[ m_processingContextPoolIndex ];
	return processingContext.m_resultDistanceSquared;
}

void CSoundEmitterComponent::Flush()
{
	if ( m_activator )
	{
		m_activator->Remove();
		m_activator->Release();
		m_activator = 0;
	}

#ifdef USE_WWISE
	for( auto iter = m_gameObjects.Begin(); iter != m_gameObjects.End(); ++iter )
	{
		AkPlayingID ids[ 128 ];
		AkUInt32 count = 128;
		Red::System::MemorySet( &ids, 0, sizeof( ids ) );

		AKRESULT result = AK::SoundEngine::Query::GetPlayingIDsFromGameObject( ( AkGameObjectID ) iter->m_second, count, ids );

		for( AkUInt32 i = 0; i != count; i++ )
		{
			AK::SoundEngine::StopPlayingID( ids[ i ], ( AkInt32 ) 1000 );
		}

		AK::SoundEngine::UnregisterGameObj( ( AkGameObjectID ) iter->m_second );
	}
#endif
	m_gameObjects.Clear();

	CSoundEmitter::Flush();

	for(auto &auxEmitter : m_auxiliaryEmitters)
	{
		auxEmitter.m_second.Flush();
	}

	UnAcquire();
}

Uint64 CSoundEmitterComponent::FindGameObjectIdForBone(Int32 boneNum)
{
	 Uint64 gameObjectId;
	 if(m_gameObjects.Find(boneNum, gameObjectId))
	 {
		 return gameObjectId;
	 }

	 return m_gameObject;
}

void CSoundEmitterComponent::TimedSoundEvent(String startEvent, String stopEvent, Float duration, Bool updateTimeParameter /*= false*/, Int32 boneNum /*= -1 */)
{
	STimedSoundEvent timedEvent;
	timedEvent.startEventId = SoundEvent(UNICODE_TO_ANSI(startEvent.AsChar()), boneNum);

	if(timedEvent.updateTimeParameter && (AkPlayingID)timedEvent.startEventId != AK_INVALID_PLAYING_ID)
	{
		SoundParameter("eventTime", 0.f, 0.f, boneNum);
	}

	timedEvent.onStopEvent = stopEvent;
	timedEvent.duration = duration;
	timedEvent.updateTimeParameter = updateTimeParameter;
	timedEvent.boneNum = boneNum;
	timedEvent.currentTime = 0.f;

	m_timedSoundEvents.PushBack(timedEvent);
}

#ifndef NO_EDITOR_FRAGMENTS
void CSoundEmitterComponent::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flags )
{
	// Pass to base class
	TBaseClass::OnGenerateEditorFragments( frame, flags );

	if( flags == SHOW_Sound )
	{
		CBitmapTexture* icon = m_icon.Get();
		if( !icon )
		{
			m_icon = LoadResource< CBitmapTexture >(TXT("engine\\textures\\icons\\soundicon.xbm"));
		}
		if ( icon )
		{
			// Draw editor icons
			Float screenScale = frame->GetFrameInfo().CalcScreenSpaceScale( GetWorldPosition() );
			const Float size = 0.25f; 

#ifndef NO_COMPONENT_GRAPH
			frame->AddSprite( GetLocalToWorld().GetTranslation(), size, Color::WHITE, GetHitProxyID(), icon, false );
#else
			frame->AddSprite( GetLocalToWorld().GetTranslation(), size, Color::WHITE, CHitProxyID(), icon, false );
#endif
		}

		const Bool isPlaying = SoundIsActive();
		Vector pos = GetSoundPlacementMatrix().GetTranslation();
		// draw ambient sound position marker
		if( m_processingContextPoolIndex >= 0 )
		{
			SProcessingContext< CSoundEmitterComponent >& processingContext = m_processingContextPool[ m_processingContextPoolIndex ];
			const Float dist = sqrtf( processingContext.m_resultDistanceSquared );
			if( SoundIsActive() )
			{
				frame->AddDebugSphere( pos, 0.15f, Matrix::IDENTITY, Color::YELLOW, true );
				frame->AddDebugText( pos, String::Printf( TXT("D:%1.0f/%1.0f, P:%d"), dist, m_maxDistance, isPlaying ? 1 : 0 ), 0, 0, true );

				pos = GetOcclusionPosition();
				frame->AddDebugSphere( pos, 0.15f, Matrix::IDENTITY, Color::RED, true );
			}
		}
	}
	if ( flags == SHOW_SoundReverb && m_activator )
	{
		if( m_debugBuffer.Empty() ) m_debugBuffer += TXT("\n");
		else
		{
			Vector position = GetLocalToWorld().GetTranslation();
			frame->AddDebugText( position, m_debugBuffer, false, Color::GREEN, Color::GREEN );
		}
	}
	else if( !m_debugBuffer.Empty() ) m_debugBuffer.Clear();

	if( flags == SHOW_SoundListener )
	{
		Vector position = Vector::ZEROS;
		Vector forward = Vector::ZEROS;
		Vector up = Vector::ZEROS;
		GSoundSystem->GetListenerVectors(position, up, forward);
		frame->AddDebugSphere( position, 0.1f, Matrix::IDENTITY, Color::CYAN );
		frame->AddDebug3DArrow(position, forward.Normalized3(), 0.25f, 0.02f,0.04, 0.05f, Color::CYAN);
	}		
}
#endif

#ifndef RED_FINAL_BUILD
StringAnsi CSoundEmitterComponent::GetSoundObjectName() const 
{
	CEntity* entity = GetEntity();
	if( !entity ) return 0;
	String result = GetEntity()->GetName();
	if( result.Empty() ) return 0;

	return UNICODE_TO_ANSI( result.AsChar() );
}

#endif

void CSoundEmitterComponent::SoundPause()
{
#ifdef USE_WWISE
	for( auto iter = m_gameObjects.Begin(); iter != m_gameObjects.End(); ++iter )
	{
		AkUInt32 count = 128;
		AkPlayingID ids[128];

		AKRESULT result = AK::SoundEngine::Query::GetPlayingIDsFromGameObject( ( AkGameObjectID ) iter->m_second, count, &ids[ 0 ] );

		for( Uint32 i = 0; i != count; ++i )
		{
			AkUniqueID eventId = AK::SoundEngine::Query::GetEventIDFromPlayingID( ids[ i ] );
			result = AK::SoundEngine::ExecuteActionOnEvent( eventId, AK::SoundEngine::AkActionOnEventType_Pause, ( AkGameObjectID ) iter->m_second );
		}
	}
#endif
	CSoundEmitterOneShot::SoundPause();
}

void CSoundEmitterComponent::SoundResume()
{
#ifdef USE_WWISE
	for( auto iter = m_gameObjects.Begin(); iter != m_gameObjects.End(); ++iter )
	{
		AkUInt32 count = 128;
		AkPlayingID ids[128];

		AKRESULT result = AK::SoundEngine::Query::GetPlayingIDsFromGameObject( ( AkGameObjectID ) iter->m_second, count, &ids[ 0 ] );

		for( Uint32 i = 0; i != count; ++i )
		{
			AkUniqueID eventId = AK::SoundEngine::Query::GetEventIDFromPlayingID( ids[ i ] );
			result = AK::SoundEngine::ExecuteActionOnEvent( eventId, AK::SoundEngine::AkActionOnEventType_Resume, ( AkGameObjectID ) iter->m_second );
		}
	}
#endif
	CSoundEmitterOneShot::SoundResume();
}

void CSoundEmitterComponent::SoundStopAll()
{
	if ( SoundFlagGet( SEF_WhileStopping ) )
	{
		return;
	}

	CSoundEmitter::SoundStopAll();

#ifdef USE_WWISE
	for( auto iter = m_gameObjects.Begin(); iter != m_gameObjects.End(); ++iter )
	{
		AK::SoundEngine::StopAll( ( AkGameObjectID ) iter->m_second );
	}
#endif
}

void CSoundEmitterComponent::SoundStop( Float duration )
{
	if ( SoundFlagGet( SEF_WhileStopping ) )
	{
		return;
	}

	CSoundEmitter::SoundStop( duration );

#ifdef USE_WWISE
	for( auto iter = m_gameObjects.Begin(); iter != m_gameObjects.End(); ++iter )
	{
		AkPlayingID ids[ 128 ];
		AkUInt32 count = 128;
		Red::System::MemorySet( &ids, 0, sizeof( ids ) );

		AKRESULT result = AK::SoundEngine::Query::GetPlayingIDsFromGameObject( ( AkGameObjectID ) iter->m_second, count, ids );

		for( AkUInt32 i = 0; i != count; i++ )
		{
			AK::SoundEngine::StopPlayingID( ids[ i ], ( AkInt32 ) duration * 1000 );
		}
	}
#endif
}

void CSoundEmitterComponent::SoundStop( Uint64 handle, Float duration )
{
	if( !handle )
	{
		return;
	}
	if ( SoundFlagGet( SEF_WhileStopping ) )
	{
		return;
	}

	CSoundEmitter::SoundStop( handle, duration );
#ifdef USE_WWISE

	Uint64 gameobject = handle & 0x00000000FFFFFFFF;
	Uint64 playingId = handle >> 32;

	for( auto iter = m_gameObjects.Begin(); iter != m_gameObjects.End(); ++iter )
	{
		if( iter->m_second != gameobject )
		{
			continue;
		}
		AK::SoundEngine::StopPlayingID( ( AkInt32 ) playingId, ( AkInt32 ) duration * 1000 );
	}
#endif

}

void CSoundEmitterComponent::SoundStop( const char* eventName, Float duration )
{
	if ( SoundFlagGet( SEF_WhileStopping ) )
	{
		return;
	}

	CSoundEmitter::SoundStop( eventName, duration );

#ifdef USE_WWISE
	AkUInt32 eventId = AK::SoundEngine::GetIDFromString( eventName );

	for( auto iter = m_gameObjects.Begin(); iter != m_gameObjects.End(); ++iter )
	{
		AkPlayingID ids[ 128 ];
		AkUInt32 count = 128;
		Red::System::MemorySet( &ids, 0, sizeof( ids ) );

		AKRESULT result = AK::SoundEngine::Query::GetPlayingIDsFromGameObject( ( AkGameObjectID ) iter->m_second, count, ids );

		for( AkUInt32 i = 0; i != count; i++ )
		{
			AkUniqueID playingEventId = AK::SoundEngine::Query::GetEventIDFromPlayingID( ids[ i ] );
			if( eventId != playingEventId ) continue;

			AK::SoundEngine::StopPlayingID( ids[ i ], ( AkInt32 ) duration * 1000 );
		}
	}
#endif
}

void CSoundEmitterComponent::SoundSeek( const char* eventName, float percent )
{
	for( auto iter = m_gameObjects.Begin(); iter != m_gameObjects.End(); ++iter )
	{
#ifdef USE_WWISE
		AKRESULT result = AK::SoundEngine::SeekOnEvent( eventName, ( AkGameObjectID ) ( AkGameObjectID ) iter->m_second, percent, false );
#endif
	}
	CSoundEmitter::SoundSeek( eventName, percent );
}

void CSoundEmitterComponent::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );

	if ( property->GetName() == TXT( "occlusionEnabled" ) )
	{
		SetOcclusionEnable( m_occlusionEnabled );
	}

	if ( property->GetName() == TXT( "loopStart" ) )
	{
		if( SoundIsActive( INENSITY_GAME_OBJECT_SLOT ) )
		{
			SoundStop();
			SoundEvent( m_intensityBasedLoopStart.AsChar(), INENSITY_GAME_OBJECT_SLOT );
		}
	}
}

void CSoundEmitterComponent::AddSoundBankDependency( CName soundBank )
{
	m_banksDependency.PushBackUnique( soundBank );
}

Vector CSoundEmitterComponent::GetOcclusionPosition()
{
	if( GetTransform().GetPosition() == Vector::ZERO_3D_POINT )
	{
		if( CEntity* entity = GetEntity() )
			if( IActorInterface* actorInterface = entity->QueryActorInterface() )
			{
				return actorInterface->GetHeadPosition();
			}
	}

	return GetSoundPlacementMatrix().GetTranslation();
}

Bool CSoundEmitterComponent::ShouldWriteToDisk() const
{
	// Only save non-default sound emitter components and default sound emitter components without entity templates
	if ( IsDefault() && GetEntity()->GetEntityTemplate() == nullptr )
	{
		return false;
	}

	return TBaseClass::ShouldWriteToDisk();
}

Bool CSoundEmitterComponent::IsDefault() const
{
	if( !m_occlusionEnabled ) return false;
	if( m_maxDistance != 30.0f ) return false;
	if( m_banksDependency.Size() || m_eventsOnAttach.Size() || m_eventsOnDetach.Size() ) return false;
	if( m_intensityBasedLoopStart.Size() || m_intensityBasedLoopStop.Size() || m_intensityParameter.Size() ) return false;
	if( !m_transform.IsIdentity() ) return false;

	return true;
}

#ifndef NO_EDITOR
Bool CSoundEmitterComponent::CheckMassActionCondition( const Char* condition ) const
{
	if ( Red::System::StringCompare( condition, TXT("default-parameters") ) == 0 )
	{
		return IsDefault();
	}
	return false;
}

#endif

Float CSoundEmitterComponent::GetCurrentOcclusion()
{
	return m_currentOcclusion;
}

Float CSoundEmitterComponent::GetCurrentObstruction()
{
	return m_currentObstruction;
}

CAuxiliarySoundEmitter::CAuxiliarySoundEmitter(const CSoundEmitterComponent *component)
	:m_speed(-1.f)
	,m_component(component)
{
	m_position = m_component->GetSoundPlacementMatrix().GetTranslation();
}

#ifndef RED_FINAL_BUILD

StringAnsi CAuxiliarySoundEmitter::GetSoundObjectName() const 
{
	return m_component->GetSoundObjectName() + "::aux";
}
#endif // !RED_FINAL

void CAuxiliarySoundEmitter::Update(Float timeDelta)
{
#ifdef USE_WWISE
	if ( 0 != m_gameObject && !GGame->IsPaused())
	{
		AkSoundPosition soundPosition;

		Vector pos = m_component->GetSoundPlacementMatrix().GetTranslation();

		soundPosition.Orientation = ToAk( m_component->GetSoundPlacementMatrix().GetAxisY() );


		Vector posDelta = pos - m_position;

		Float distance = posDelta.Mag3();

		if(distance > 0.1f)
		{
			Vector direction = posDelta.Normalized3();

			Float speed = m_speed;

			if(distance < m_decelDistance)
			{
				speed = speed * distance / m_decelDistance;
			}

			//Move the current position along the path towards the target by the speed until it reaches the target
			Vector newPosition = m_position + (direction * Min(timeDelta*speed, distance));

			soundPosition.Position = ToAk( newPosition );

			AK::SoundEngine::SetPosition( ( AkGameObjectID ) m_gameObject, soundPosition );

			m_position = newPosition;
		}
	}
#endif


}


