/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "fxTrackItemSoundEvent.h"
#include "soundStartData.h"
#include "fxTrackGroup.h"
#include "animatedComponent.h"
#include "soundAmbientManager.h"
#include "soundSystem.h"
#include "soundEmitter.h"
#include "entity.h"

IMPLEMENT_ENGINE_CLASS( CFXTrackItemSoundEvent );

////////////////////////////////////////////////////////////////////////

class CFXTrackItemSoundPlayData : public IFXTrackItemPlayData, public IAmbientSound
{
public:
	CFXTrackItemSoundPlayData( const CFXTrackItemSoundEvent* trackItem, CNode* node,
		const CName& boneName, const StringAnsi& soundEventName, Bool isAmbient, Float maxDistance )
		: IFXTrackItemPlayData( node, trackItem )
		, m_nodeHandle( node )
		, m_isStopped( false )
		, m_hasFailed( true )
		, m_isAmbient( isAmbient )
		, m_maxDistance( maxDistance )
		, m_isPlaying( false )
		, m_soundInstanceHandle( 0 )
		, m_useDistanceParameter(trackItem->GetUseDistanceParameter())
		, m_latchDistanceParameterBelow( trackItem->GetDistanceLatchBelow() )
		, m_invertLatchDistance(trackItem->ShouldInvertDistanceLatch())
		, m_latchEvent(trackItem->GetLatchEvent())
		, m_speed(trackItem->GetSpeed())
		, m_decelDist(trackItem->GetDecelDistance())
	{ 
		if( m_isAmbient ) return;

		m_isPlaying = Play();
	} 

	~CFXTrackItemSoundPlayData()
	{
		Stop();
	}

	virtual void OnTick( const CFXState& fxState, Float timeDelta )
	{
		CNode* node = m_nodeHandle.Get();
		if( !node ) return;
		CEntity* entity = node->AsEntity();
		if( !entity ) return;

		CSoundEmitterComponent* soundEmitterComponent = entity->GetSoundEmitterComponent( false );
		if( !soundEmitterComponent ) return;

		HandleDistanceBehaviour(entity, soundEmitterComponent);

		if( !m_isAmbient && m_isPlaying ) return;

		Float currentDistnce = soundEmitterComponent->GetDistanceFromCameraSquared();
		
		Float maxDistance = GetMaxDistance();
		maxDistance *= maxDistance;
		if( currentDistnce > maxDistance && m_isPlaying )
		{
			Stop();
		}
		else if( currentDistnce <= maxDistance && !m_isPlaying )
		{
			m_isPlaying = Play();
		}
		else
		{
			m_isPlaying = IsPlaying();
		}
	}

	virtual void OnStop()
	{
		m_isStopped = true;
		Stop();
	}

	virtual Bool Play( )
	{
		if( m_isStopped )
		{
			return false;
		}


		CNode* node = m_nodeHandle.Get();
		if( !node ) return false;

		CFXTrackItemSoundEvent* trackItem = static_cast< CFXTrackItemSoundEvent* >( GetTrackItem() );

		CEntity* entity = node->AsEntity();
		if( !entity ) return false;

		CSoundEmitterComponent* soundEmitterComponent = CSoundEmitterComponent::GetSoundEmitterIfMaxDistanceIsntReached( entity, m_maxDistance );
		if( !soundEmitterComponent || !soundEmitterComponent->IsLoadedAndReady() ) return false;

		if( IsPlaying() )
		{
			return true;
		}

		CAnimatedComponent* component = entity->GetRootAnimatedComponent();

		Float speed = m_speed, decelDist = m_decelDist;

		auto soundEvent = [&soundEmitterComponent, speed, decelDist](const char *name, Int32 bone)
		{
			if(speed >= 0.f)
			{
				return soundEmitterComponent->AuxiliarySoundEvent(name, speed, decelDist, bone);
			}
			else
			{
				return soundEmitterComponent->SoundEvent(name, bone);
			}
		};

		Int32 bone = -1;
		if( component )
		{
			bone = component->FindBoneByName( trackItem->GetBoneName() );
		}
		m_soundInstanceHandle = soundEvent( trackItem->GetSoundEventName().AsChar(), bone );

		return true;
	}

	virtual void Stop()
	{
		m_isPlaying = false;
		CNode* node = m_nodeHandle.Get();
		if( !node ) return;

		CEntity* entity = node->AsEntity();
		if( !entity ) return;

		Uint32 bone = -1;
		if( CAnimatedComponent* component = entity->GetRootAnimatedComponent() )
		{
			CFXTrackItemSoundEvent* trackItem = static_cast< CFXTrackItemSoundEvent* >( GetTrackItem() );
			bone = component->FindBoneByName( trackItem->GetBoneName() );
		}

		CSoundEmitterComponent* soundEmitterComponent = entity->GetSoundEmitterComponent( false );
		if( soundEmitterComponent && soundEmitterComponent->SoundIsActive( m_soundInstanceHandle, bone ) )
		{
			CFXTrackItemSoundEvent* trackItem = static_cast< CFXTrackItemSoundEvent* >( GetTrackItem() );
			soundEmitterComponent->SoundStop( m_soundInstanceHandle, trackItem->GetStopFadeTime() );
		}

	}

	void HandleDistanceBehaviour(const CEntity* entity, CSoundEmitterComponent *soundEmitterComponent)
	{
		if(m_useDistanceParameter)
		{
			Float distanceToListener = (GetSoundPosition() - GSoundSystem->GetListenerPosition()).Mag3();
			soundEmitterComponent->SoundParameter("distance", distanceToListener);
			
			Bool playLatchEvent = false;
			
			
			if(m_invertLatchDistance)
			{
				if(distanceToListener >= m_latchDistanceParameterBelow)
				{
					m_useDistanceParameter = false;
					playLatchEvent = true;
				}
			}
			else
			{
				if(distanceToListener <= m_latchDistanceParameterBelow)
				{
					m_useDistanceParameter = false;
					playLatchEvent = true;
				}
			}

			if(playLatchEvent)
			{
				Int32 bone = -1;
				if(entity->GetRootAnimatedComponent())
				{
					CFXTrackItemSoundEvent* trackItem = static_cast< CFXTrackItemSoundEvent* >( GetTrackItem() );
					bone = entity->GetRootAnimatedComponent()->FindBoneByName(trackItem->GetBoneName());
				}
				soundEmitterComponent->SoundEvent(m_latchEvent.AsChar(), bone);
			}
		}
	}

	virtual Bool IsPlaying() const
	{
		CNode* node = m_nodeHandle.Get();
		if( !node ) return false;
		CEntity* entity = node->AsEntity();
		if( !entity ) return false;
		CSoundEmitterComponent* soundEmitterComponent = entity->GetSoundEmitterComponent( false );
		if( !soundEmitterComponent ) return false;
		CFXTrackItemSoundEvent* trackItem = static_cast< CFXTrackItemSoundEvent* >( GetTrackItem() );
		Uint32 bone = -1;
		if( CAnimatedComponent* component = entity->GetRootAnimatedComponent() )
		{
			CFXTrackItemSoundEvent* trackItem = static_cast< CFXTrackItemSoundEvent* >( GetTrackItem() );
			bone = component->FindBoneByName( trackItem->GetBoneName() );
		}
		return soundEmitterComponent->SoundIsActive( m_soundInstanceHandle, bone );
	}

	virtual Bool HasFailed() const { return m_hasFailed; }

	virtual Float GetMaxDistance() const
	{
		return m_maxDistance;
	}

	virtual const Vector& GetSoundPosition() const
	{
		CNode* node = m_nodeHandle.Get();
		if( !node ) return Vector::ZERO_3D_POINT;

		return node->GetWorldPositionRef();
	}

	virtual EAmbientSoundType GetType() const { return EAST_FX; }

private:
	StringAnsi				m_latchEvent;		//!< event to trigger when the latch activates
	THandle< CNode >		m_nodeHandle;
	Uint64					m_soundInstanceHandle;
	Bool					m_isStopped;
	Bool					m_hasFailed;
	Bool					m_isAmbient;
	Bool					m_isPlaying;
	Bool					m_useDistanceParameter;
	Float					m_stopFadeTime;
	Float					m_maxDistance;
	Float					m_latchDistanceParameterBelow; //!< When distance is below this value, we no longer update it
	Bool					m_invertLatchDistance; //!< latch works on distances above m_latchDistanceParameterBelow instead
	Float					m_speed;				//!< when the fx has a target to move to, use this to control how fast the sound moves
	Float					m_decelDist;			//!< when a speed is being used the speed will decrease linearly wrt dist when below this distance

};

////////////////////////////////////////////////////////////////////////

CFXTrackItemSoundEvent::CFXTrackItemSoundEvent()
	: m_soundEventName()
	, m_enabled( true )
	, m_maxDistance( 20.0f )
	, m_isAmbient( false )
	, m_boneName( CNAME( Trajectory ) )
	, m_stopFadeTime( 1.0f )
	, m_useDistanceParameter( false )
	, m_latchDistanceParameterBelow( -1.f )
	, m_invertLatchDistance(false)
	, m_speed(-1.f)
	, m_decelDist(0.f)
{
	// A tick
	m_timeDuration = 0.0f;
}

IFXTrackItemPlayData* CFXTrackItemSoundEvent::OnStart( CFXState& fxState ) const
{
	if ( ! (m_soundEventName.Empty() && m_latchEvent.Empty()) && m_enabled )
	{
		CComponent * component = GetTrack()->GetTrackGroup()->GetAffectedComponent( fxState );
		if ( component && component->AsEntity() )
		{
			return new CFXTrackItemSoundPlayData( this, component, m_boneName, m_soundEventName, m_isAmbient, m_maxDistance );
		}
		else
		{
			// Get target entity
			CEntity *entity = fxState.GetEntity();
			if ( entity )
			{
				// Raise sound event
				return new CFXTrackItemSoundPlayData( this, entity, m_boneName, m_soundEventName, m_isAmbient, m_maxDistance );
			}
		}
	}
	
	// No runtime data spawned
	return NULL;
}
