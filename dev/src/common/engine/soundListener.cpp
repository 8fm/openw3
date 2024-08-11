#include "build.h"
#include "soundListener.h"
#include "softTriggerAreaComponent.h"
#include "game.h"

IMPLEMENT_ENGINE_CLASS( CSoundListenerComponent )

#ifdef USE_WWISE
#include <AK/SoundEngine/Common/AkSoundEngine.h>    // Sound engine
#endif
#include "triggerManager.h"
#include "world.h"


CSoundListenerComponent::CSoundListenerComponent() : m_activator( 0 ), m_position( Vector::ZERO_3D_POINT )
{

}

CSoundListenerComponent::~CSoundListenerComponent()
{
}

void CSoundListenerComponent::Flush()
{
	if( m_activator )
	{
		m_activator->Remove();
		m_activator->Release();
		m_activator = 0;
	}
}

void CSoundListenerComponent::UpdatePosition( const Vector& forward, const Vector& up, const Vector& position )
{
	CWorld* world = GGame->GetActiveWorld();
	if ( !m_activator && world )
	{
		CTriggerActivatorInfo initInfo;
		initInfo.m_channels = TC_SoundReverbArea;
		initInfo.m_component = this;
#ifndef RED_TRIGGER_SYSTEM_NO_DEBUG_NAMES
		initInfo.m_debugName = TXT("SoundListener_");
		initInfo.m_debugName += GetName();
#endif
		initInfo.m_extents = Vector::ZEROS; // point activator
		initInfo.m_localToWorld.SetIdentity(); 
		initInfo.m_localToWorld.SetTranslation( position ); // setup initial position
		m_activator = world->GetTriggerManager()->CreateActivator( initInfo );
	}

	m_position = position;
	m_influencedTriggerObjectsIds.ClearFast();

	if( m_activator )
	{
		const IntegerVector4 newPosI( position );
		m_activator->Move( newPosI );

		Uint32 triggersCount = m_activator->GetNumOccupiedTrigger();
		for( Uint32 i = 0; i != triggersCount; ++i )
		{
			const ITriggerObject* trigger = m_activator->GetOccupiedTrigger( i );
			CSoftTriggerAreaComponent* softTrigger = Cast< CSoftTriggerAreaComponent >( trigger->GetComponent() );
			if( softTrigger )
			{
				float ratio = softTrigger->CalcPenetrationFraction( m_position );
				STriggerRatio triggerRatio( trigger, ratio );

				STriggerRatio* existingTriggerRatio = m_state.FindPtr( triggerRatio );
				if( existingTriggerRatio )
				{
					m_state.RemoveFast( triggerRatio );
				}
				else
				{
					m_influencedTriggerObjectsIds.PushBack( triggerRatio.m_trigger );
				}
			}
		}

		for( Uint32 i = 0; i != m_state.Size(); ++i )
		{
			m_influencedTriggerObjectsIds.PushBack( m_state[ i ].m_trigger );
		}

		m_state.ClearFast();

		triggersCount = m_activator->GetNumOccupiedTrigger();
		m_state.Reserve( triggersCount );
		for( Uint32 i = 0; i != triggersCount; ++i )
		{
			const ITriggerObject* trigger = m_activator->GetOccupiedTrigger( i );
			CSoftTriggerAreaComponent* softTrigger = Cast< CSoftTriggerAreaComponent >( trigger->GetComponent() );
			if( softTrigger )
			{
				float ratio = softTrigger->CalcPenetrationFraction( m_position );
				m_state.PushBackUnique( STriggerRatio( trigger, ratio ) );

			}
		}

	}


#ifdef USE_WWISE
	AkListenerPosition listenerPosition;
	listenerPosition.OrientationFront.X = forward.X;
	listenerPosition.OrientationFront.Y = forward.Z;
	listenerPosition.OrientationFront.Z = forward.Y;
	listenerPosition.OrientationTop.X = up.X;
	listenerPosition.OrientationTop.Y = up.Z;
	listenerPosition.OrientationTop.Z = up.Y;
	listenerPosition.Position.X = m_position.X;
	listenerPosition.Position.Y = m_position.Z;
	listenerPosition.Position.Z = m_position.Y;
	AKRESULT RESULT = AK::SoundEngine::SetListenerPosition( listenerPosition );
#endif

}

void CSoundListenerComponent::PushInfluencedTrigger( const void* ptr )
{
	m_influencedTriggerObjectsIds.PushBack( ptr );
}

const void* CSoundListenerComponent::GetInfluencedTrigger( Uint32 index )
{
	if( m_influencedTriggerObjectsIds.Size() <= index ) return nullptr;

	return m_influencedTriggerObjectsIds[ index ];
}
