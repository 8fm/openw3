/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "soundAmbientManager.h"
#include "soundStartData.h"
#include "soundAmbientAreaComponent.h"
#include "game.h"
#include "triggerManager.h"
#include "world.h"
#include "AK\SoundEngine\Common\AkTypes.h"
#include "AK\SoundEngine\Common\AkQueryParameters.h"
#include "AK\SoundEngine\Common\AkSoundEngine.h"

CSoundAmbientManager::CSoundAmbientManager()
	: m_ambientActivator( NULL )
{
}

CSoundAmbientManager::~CSoundAmbientManager()
{
}

void CSoundAmbientManager::Init()
{
	Reset();
}

void CSoundAmbientManager::Shutdown()
{
	if ( NULL != m_ambientActivator )
	{
		m_ambientActivator->Remove();
		m_ambientActivator->Release();
		m_ambientActivator = NULL;
	}
	m_currentAmbients.Clear();
}

void CSoundAmbientManager::Reset()
{
	if( m_listenerCurrentMusicArea )
	{
		CSoundEmitterOneShot::SoundGlobalParameter( "music_priority\0", 0.0f );
		m_listenerCurrentMusicArea = nullptr;
	}
	if( m_listenerCurrentAmbientArea )
	{
		CSoundEmitterOneShot::SoundGlobalParameter( "amb_priority\0", 0.0f );
		m_listenerCurrentAmbientArea = nullptr;
	}

	for( auto i : m_currentParameters )
	{
		CSoundEmitterOneShot::SoundGlobalParameter( i.m_gameParameterName.AsChar(), 0.0f );
	}
	m_currentParameters.Clear();
	
	if ( nullptr != m_ambientActivator )
	{
		m_ambientActivator->Remove();
		m_ambientActivator->Release();
		m_ambientActivator = nullptr;
	}

	m_currentAmbients.Clear();
}

void CSoundAmbientManager::Tick( const Vector& listenerPosition, Float dt )
{
	PC_SCOPE( SoundAmbientManagerTick );

	// Create ambient activator (for trigger system) if not yet created
	if ( NULL == m_ambientActivator )
	{
		CWorld* world = GGame->GetActiveWorld();
		if ( NULL != world )
		{
			CTriggerActivatorInfo info;
			info.m_channels = TC_SoundAmbientArea;
			info.m_component = NULL; // external
#ifndef RED_TRIGGER_SYSTEM_NO_DEBUG_NAMES
			info.m_debugName = TXT("AmbientAreaActivator");
#endif
			info.m_enableCCD = false;
			info.m_extents = Vector::ZEROS;
			info.m_localToWorld.SetIdentity();
			info.m_localToWorld.SetTranslation( listenerPosition );
			m_ambientActivator = world->GetTriggerManager()->CreateActivator( info );
		}
	}
	
	// Move ambient area activator to new listener location.
	// This will (in next trigger manager tick) update the list of active ambient areas
	if ( NULL != m_ambientActivator )
	{
		const IntegerVector4 pos( listenerPosition );
		m_ambientActivator->Move( pos, true );
	}

	UpdateAmbientPriorities();

	UpdateWorldSounds();
}

void CSoundAmbientManager::UpdateWorldSounds()
{
	TDynArray<StringAnsi> &worldSounds = GGame->GetActiveWorld()->GetSoundEventsOnAttach();

	if(!worldSounds.Empty())
	{
		AkUInt32 size = 0;
		AK::SoundEngine::Query::GetPlayingIDsFromGameObject( ( AkGameObjectID ) m_gameObject, size, 0 );
		if( size == 0 )
		{
			for(auto soundEvent : worldSounds)
			{
				SoundEvent( soundEvent.AsChar() );
			}
		}
		else
		{

			Bool retriggerEvent = true;
			TDynArray< AkUInt32 > ids;
			ids.Grow( size );
			AK::SoundEngine::Query::GetPlayingIDsFromGameObject( ( AkGameObjectID ) m_gameObject, size, &ids[ 0 ] );
			
			for(auto soundEvent : worldSounds)
			{
				AkUInt32 eventId = AK::SoundEngine::GetIDFromString(soundEvent.AsChar());


				for( Uint32 i = 0; i != size; ++i )
				{
					if(eventId == AK::SoundEngine::Query::GetEventIDFromPlayingID(ids[i]))
					{
						retriggerEvent = false;
						break;
					}
				}

				if(retriggerEvent)
				{
					SoundEvent(soundEvent.AsChar());
				}
			}
			
		}
	}
}

static Bool RED_FORCE_INLINE comp( const THandle< CSoundAmbientAreaComponent > a, const THandle< CSoundAmbientAreaComponent > b)
{
	return a.Get()->GetAmbientPriority() < b.Get()->GetAmbientPriority();
}

void CSoundAmbientManager::DetachAmbientArea(const CSoundAmbientAreaComponent * areaComponent)
{
	if(m_listenerCurrentAmbientArea.Get() == areaComponent)
	{
		for( Uint32 j = 0; j != m_currentParameters.Size(); ++j )
		{
			const char* parameterName = m_currentParameters[ j ].m_gameParameterName.AsChar();
			CSoundEmitterOneShot::SoundGlobalParameter( parameterName, 0.0f, ( Float ) areaComponent->GetParameterExitingTime(), areaComponent->GetParameterExitingCurve() );
		}

		m_currentParameters.Clear();
	}
}

void CSoundAmbientManager::UpdateAmbientPriorities()
{
	PC_SCOPE_PIX( UpdateAmbients );

	if( !m_ambientActivator ) return;

	Uint32 occupiedTriggerCount = m_ambientActivator->GetNumOccupiedTrigger();

	TDynArray< THandle< CSoundAmbientAreaComponent > > newAmbients;
	newAmbients.Reserve( occupiedTriggerCount );

	for( Uint32 i = 0; i != occupiedTriggerCount; ++i )
	{
		const ITriggerObject* trigger = m_ambientActivator->GetOccupiedTrigger( i );

		CComponent* component = trigger->GetComponent();
		if( !component ) continue;

		CSoundAmbientAreaComponent* ambient = Cast< CSoundAmbientAreaComponent >( component );
		if( !ambient ) continue;

		if( ambient->GetAmbientPriority() > -1 )
		{
			newAmbients.PushBackUnique( ambient );
		}
	}

	if( m_currentAmbients.Size() == newAmbients.Size() )
	{
		Bool match = true;
		for( auto i = newAmbients.Begin(); i != newAmbients.End(); ++i )
		{
			if( m_currentAmbients.FindPtr( *i ) ) continue;
			match = false;
			break;
		}

		if( match ) return;
	}

	Int32 ambientHighestPriority = -1;
	Int32 musicHighestPriority = -1;

	CSoundAmbientAreaComponent* newAmbientArea = 0;
	CSoundAmbientAreaComponent* newMusicArea = 0;

	Sort( newAmbients.Begin(), newAmbients.End(), comp );	

	for( Uint32 i = 0; i != newAmbients.Size(); ++i )
	{
		// Calculate highest priority
		CSoundAmbientAreaComponent* component = newAmbients[ i ].Get();
		Int32 priority = component->GetAmbientPriority();

		if( component->IsPriorityParameterMusic() )
		{
			if( priority > musicHighestPriority )
			{
				musicHighestPriority = priority;
				newMusicArea = component;
			}
		}
		else
		{
			if( priority == ambientHighestPriority )
			{
				THandle< CSoundAmbientAreaComponent >* newAlreadyExisting = m_currentAmbients.FindPtr( component );
				
 				if( !newAlreadyExisting )
				{
					ambientHighestPriority = priority;
					newAmbientArea = component;
				}
			}
			else if( priority > ambientHighestPriority )
			{
				ambientHighestPriority = priority;
				newAmbientArea = component;
			}
		}
	}

	{
		float currentValue = m_listenerCurrentMusicArea.Get() ? m_listenerCurrentMusicArea.Get()->GetTriggerPriority() : 0.0f;
		float newValue = newMusicArea ? newMusicArea->GetTriggerPriority() : 0.0f;
		UpdateParameter( m_listenerCurrentMusicArea.Get(), newMusicArea, "music_priority\0", currentValue, newValue );
		m_listenerCurrentMusicArea = newMusicArea;
	}

	TDynArray< SSoundGameParameterValue > currentParameters;
	TDynArray< SSoundGameParameterValue > newParameters;

	float ambientPriorityCurrentValue = 0.0f;
	if( m_listenerCurrentAmbientArea.Get() )
	{
		ambientPriorityCurrentValue = ( float ) m_listenerCurrentAmbientArea.Get()->GetTriggerPriority();
		currentParameters = m_currentParameters;
	}

	float ambientPriorityNewValue = 0.0f;
	if( newAmbientArea )
	{
		ambientPriorityNewValue = ( float ) newAmbientArea->GetTriggerPriority();
		newParameters = newAmbientArea->GetGameParameters();
		for( auto i = newAmbients.Begin(); i != newAmbients.End(); ++i )
		{
			const TDynArray< SSoundGameParameterValue >& otherParameters = ( *i )->GetGameParameters();
			for( auto j = otherParameters.Begin(); j != otherParameters.End(); ++j )
			{
				if( newParameters.FindPtr( ( *j ) ) ) continue;

				newParameters.PushBack( *j );
			}
		}

		for( Uint32 i = 0; i != newParameters.Size(); ++i )
		{
			float currentValue = 0.0f;
			float newValue = newParameters[ i ].m_gameParameterValue;
			const char* parameterName = newParameters[ i ].m_gameParameterName.AsChar();
			for( Uint32 j = 0; j != currentParameters.Size(); ++j )
			{
				if( !( newParameters[ i ] == currentParameters[ j ] ) ) continue;
				currentValue = currentParameters[ j ].m_gameParameterValue;
				currentParameters.RemoveAtFast( j );
				break;
			}
	
			UpdateParameter( m_listenerCurrentAmbientArea.Get(), newAmbientArea, parameterName, currentValue, newValue );
		}
	}

	float newValue = 0.0f;
	for( Uint32 j = 0; j != currentParameters.Size(); ++j )
	{
		float currentValue = currentParameters[ j ].m_gameParameterValue;
		const char* parameterName = currentParameters[ j ].m_gameParameterName.AsChar();
		UpdateParameter( m_listenerCurrentAmbientArea.Get(), newAmbientArea, parameterName, currentValue, newValue );
	}

	UpdateParameter( m_listenerCurrentAmbientArea.Get(), newAmbientArea, "amb_priority\0", ambientPriorityCurrentValue, ambientPriorityNewValue );
	m_listenerCurrentAmbientArea = newAmbientArea;

	//yes, this is need becouse if those ambient on enter/exit would be called normaly from triggers...order would we unsored, and we need to call every enter/exit in trigger priority risinh order.
	for( auto i = newAmbients.Begin(); i != newAmbients.End(); ++i )
	{
		if( THandle< CSoundAmbientAreaComponent >* existing = m_currentAmbients.FindPtr( *i ) )
		{
			*existing = nullptr;
		}
		else
		{
			( *i )->OnEnterEvents();
		}
	}

	for( auto i = m_currentAmbients.Begin(); i != m_currentAmbients.End(); ++i )
	{
		if( ( *i ) == nullptr )
		{
			continue;
		}
		if( THandle< CSoundAmbientAreaComponent >* existing = newAmbients.FindPtr( *i ) )
		{
		}
		else
		{
			( *i )->OnExitEvents();
		}
	}
	m_currentAmbients = newAmbients;
	m_currentParameters = newParameters;
}

void CSoundAmbientManager::UpdateParameter( CSoundAmbientAreaComponent* currentArea, CSoundAmbientAreaComponent* newArea, const char* parameterName, float value, float newValue )
{
	if( currentArea && !newArea )
	{
		CSoundEmitterOneShot::SoundGlobalParameter( parameterName, 0.0f, ( Float ) currentArea->GetParameterExitingTime(), currentArea->GetParameterExitingCurve() );
		return;
	}

	if( !newArea ) return;

	Float newPriority = ( Float ) newArea->GetTriggerPriority();
	if( !currentArea && newArea )
	{
		CSoundEmitterOneShot::SoundGlobalParameter( parameterName, newValue, ( Float ) newArea->GetParameterEnteringTime(), newArea->GetParameterEnteringCurve() );
	}
	else if( newValue > value )
	{
		CSoundEmitterOneShot::SoundGlobalParameter( parameterName, newValue, ( Float ) newArea->GetParameterEnteringTime(), newArea->GetParameterEnteringCurve() );
	}
	else if( newValue < value )
	{
		CSoundEmitterOneShot::SoundGlobalParameter( parameterName, newValue, ( Float ) currentArea->GetParameterExitingTime(), currentArea->GetParameterExitingCurve() );
	}
}
