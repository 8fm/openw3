/*
 * Copyright © 2016 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "soundAmbientAreaComponent.h"
#include "soundStartData.h"
#include "../core/gatheredResource.h"
#include "animatedComponent.h"
#include "game.h"
#include "soundSystem.h"
#include "renderFrame.h"
#include "tickManager.h"
#include "world.h"
#include "bitmapTexture.h"
#include "entity.h"
#include "AK/SoundEngine/Common/AkSoundEngine.h"
#include "soundWallaSystem.h"
#include "AK/SoundEngine/Common/AkQueryParameters.h"
#include "baseEngine.h"

IMPLEMENT_RTTI_ENUM( ESoundParameterCurveType )
IMPLEMENT_RTTI_ENUM( ESoundAmbientDynamicParameter )
IMPLEMENT_ENGINE_CLASS( SReverbDefinition )
IMPLEMENT_ENGINE_CLASS( SSoundGameParameterValue )
IMPLEMENT_ENGINE_CLASS( SSoundParameterCullSettings )
IMPLEMENT_ENGINE_CLASS( SSoundAmbientDynamicSoundEvents )
IMPLEMENT_ENGINE_CLASS( CSoundAmbientAreaComponent )

RED_DEFINE_NAME( SoundAmbientArea )

CGatheredResource resSoundSourceIcon( TXT("engine\\textures\\icons\\waypointicon.xbm"), RGF_NotCooked );

CSoundAmbientAreaComponent::CSoundAmbientAreaComponent()
	: m_soundEvents()
	, m_isRegistered( false )
	, m_innerActivated( false )
	, m_intensityParameter( -1 )
	, m_intensityParameterFadeTime( 0.0f )
	, m_parameterEnteringTime( 1.0f )
	, m_parameterEnteringCurve( ESPCT_Linear )
	, m_parameterExitingTime( 1.0f )
	, m_parameterExitingCurve( ESPCT_Linear )
	, m_useListernerDistance( false )
	, m_enterExitEventsUsePosition( false )
	, m_fitWaterShore( false )
	, m_waterGridCellCount( 1024 )
	, m_waterLevelOffset( -0.5f )
	, m_fitFoliage( false )
	, m_foliageMaxDistance( 25.0f )
	, m_foliageStepNeighbors( 8 )
	, m_foliageVitalAreaRadius( 0.0f )
	, m_foliageVitalAreaPoints( 10 )
	, m_lastListenerPosition( 0, 0,0 )
	, m_outerListnerReverbRatio( 1.0f )
	, m_maxDistanceVertical( 0.0f )
	, m_state( ESoundAmbientAreaState::Outside )
	, m_gatewayState( Gateway_None )
	, m_gatewayRotation( 0.f )
	, m_isGate( false )
	, m_isWalla( false)
	, m_wallaEmitterSpread(5.f)
	, m_wallaOmniFactor(0.f)
	, m_wallaMinDistance(0.f)
	, m_wallaMaxDistance(1000.f)
	, m_wallaBoxExtention(0.f)
	, m_wallaRotation(0.f)
	, m_wallaAfraidRetriggerTime(10.f)
	, m_wallaAfraidDecreaseRate(1.f)
	, m_wallaAfraidTimer(0.f)
#ifdef SOUND_DEBUG
	, m_decoyPosition()
	, m_playerDistance( 0.0f )
#endif
{
	// only player or camera can trigger this area
	m_includedChannels = TC_SoundReverbArea;

	// outer area can be activated only by ambient activator
	m_outerIncludedChannels = TC_SoundAmbientArea;

	m_color = m_priorityParameterMusic ? Color::BLUE : Color::YELLOW;
}

CSoundAmbientAreaComponent::~CSoundAmbientAreaComponent()
{
}

void CSoundAmbientAreaComponent::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );

	// changed the "enabled" flag - restart is required
	if ( property->GetName() == TXT( "isEnabled" ) )
	{
		if ( m_isEnabled )
		{
			RegisterAudioSource();
		}
		else
		{
			UnregisterAudioSource();
		}
	}

	// changed the maxRadius property - recompute beveling
	if ( property->GetName() == TXT("maxDistance") || property->GetName() == TXT("maxDistanceVertical") )
	{
		UnregisterAudioSource();
		RecreateOuterShape();
	}

	if ( property->GetName() == TXT("priorityParameterMusic") )
	{
		m_color = m_priorityParameterMusic ? Color::BLUE : Color::YELLOW;
	}

#ifndef NO_EDITOR
	if( property->GetName() == TXT( "fitWaterShore" ) )
	{
		if( m_fitWaterShore )
			GenerateWaterAreas( );
		else
			RestoreAreaToDefaultBox( );
	}
	else if( property->GetName() == TXT( "fitFoliage" ) )
	{
		if( m_fitFoliage )
			GenerateFoliageAreas( );
		else
			RestoreAreaToDefaultBox( );
	}
#endif
}

void CSoundAmbientAreaComponent::RegisterAudioSource()
{
	if ( !m_isRegistered && IsEnabled()  )
	{
		CWorld* attachedWorld = GetWorld();
		if ( NULL != attachedWorld )
		{
			attachedWorld->GetTickManager()->AddToGroup( this, TICK_PostUpdateTransform );	
		}

		m_lastListenerPosition.SetZeros();
		m_lastSourcePosition.SetZeros();

		m_isRegistered = 1;

		Acquire( true );
	}
}

void CSoundAmbientAreaComponent::UnregisterAudioSource()
{
	if ( !m_isRegistered ) return;
	CWorld* attachedWorld = GetWorld();
	if ( NULL != attachedWorld )
	{
		attachedWorld->GetTickManager()->RemoveFromGroup( this, TICK_PostUpdateTransform );	
	}

	Stop();

	m_isPlaying = false;
	m_isRegistered = 0;
	UnAcquire();
}

void CSoundAmbientAreaComponent::OnAttached( CWorld* world )
{
	m_includedChannels = TC_SoundReverbArea;
	m_outerIncludedChannels = TC_SoundAmbientArea;

	TBaseClass::OnAttached( world );
}

void CSoundAmbientAreaComponent::OnDetached( CWorld* world )
{
	// HACK! This functionality/bug fix should be probably moved to CTriggerAreaComponent HACK! 
	// When area is despawned and player is still inside it , we need to trigger ExitedArea manually.
	if ( m_isRegistered )
	{
		// TODO: evaluate if this hack is needed
		if ( GGame && GGame->GetPlayerEntity() )
		{
			ExitedArea( GGame->GetPlayerEntity()->GetRootAnimatedComponent() );
		}
	}

	GSoundSystem->GetAmbientManager().DetachAmbientArea(this);

	// Always unregister sound source when detaching
	UnregisterAudioSource();

	// Pass to the base class
	TBaseClass::OnDetached( world );
}

Bool CSoundAmbientAreaComponent::ShouldCullFromParameters() const 
{
	for(auto & parameterCulling : m_parameterCulling)
	{
		Float value;
		if(GSoundSystem->GetRtpcValue(parameterCulling.m_gameParameterName.AsChar(), value, m_gameObject))
		{
			if(parameterCulling.m_invertCullCheck ? value >= parameterCulling.m_gameParameterCullValue : value <= parameterCulling.m_gameParameterCullValue)
			{
				return true;
			}
		}
	}

	return false;
}

Bool CSoundAmbientAreaComponent::SoundIsActive( const char* eventName ) const
{
#ifdef USE_WWISE
	AkUInt32 id = AK::SoundEngine::GetIDFromString( eventName );

	AkPlayingID ids[ 128 ];
	AkUInt32 count = 128;
	Red::System::MemorySet( &ids, 0, sizeof( ids ) );

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

Bool CSoundAmbientAreaComponent::SoundIsActive() const
{
	return CSoundEmitter::SoundIsActive();
}

void CSoundAmbientAreaComponent::ProcessDynamicEvents()
{
	Float time = GGame->GetEngineTime();
	for(auto &dynamicEvent : m_dynamicEvents)
	{
		if(!SoundIsActive(dynamicEvent.m_eventName.AsChar()))
		{
			if(time - dynamicEvent.m_lastTimeWasPlaying > (dynamicEvent.m_repeatTime + dynamicEvent.m_nextVariance))
			{
				SoundEvent(dynamicEvent.m_eventName.AsChar());
				dynamicEvent.m_lastTimeWasPlaying = time;
				dynamicEvent.m_nextVariance = (GEngine->GetRandomNumberGenerator().Get< Float >(2.f) -1.f)*dynamicEvent.m_repeatTimeVariance;

			}
		}
		else
		{
			dynamicEvent.m_lastTimeWasPlaying = time;
		}
	}
}

void CSoundAmbientAreaComponent::TickSounds()
{
	if(m_isPlaying && !m_soundEvents.Empty())
	{
		AkUInt32 size = 0;
		AK::SoundEngine::Query::GetPlayingIDsFromGameObject( ( AkGameObjectID ) m_gameObject, size, 0 );
		if( size == 0 )
		{
			SoundEvent( m_soundEvents.AsChar() );
		}
		else
		{
			Bool retriggerEvent = true;
			AkUInt32 eventId = AK::SoundEngine::GetIDFromString(m_soundEvents.AsChar());
			TDynArray< AkUInt32 > ids;
			ids.Grow( size );
			AK::SoundEngine::Query::GetPlayingIDsFromGameObject( ( AkGameObjectID ) m_gameObject, size, &ids[ 0 ] );
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
				SoundEvent(m_soundEvents.AsChar());
			}
		}
	}
	
}

Bool CSoundAmbientAreaComponent::Play( Bool _loadReasourcesImmediately )
{
	// we can't play when disabled
	if ( !m_isEnabled )
	{
		return false;
	}

	if( SoundIsActive() || WallaIsActive() ) return true;

	// execute sound event
	if ( !m_soundEvents.Empty() )
	{
		PendProcessing( this, this );
		SoundEvent( m_soundEvents.AsChar() );
	}

	if(m_isWalla)
	{
		for (Uint32 i=0; (i < Num_WallaDirections) && (i < m_wallaSoundEvents.Size()); i++)
		{
			if(!m_wallaSoundEvents[i].Empty() && !m_wallaSoundEmitters[i].SoundIsActive())
			{
				PendProcessing( &m_wallaSoundEmitters[i], this );
				m_wallaSoundEmitters[i].SoundEvent(m_wallaSoundEvents[i].AsChar());
			}
		}
	}

	for(auto &dynamicEvent : m_dynamicEvents)
	{
		if(dynamicEvent.m_triggerOnActivation)
		{
			SoundEvent(dynamicEvent.m_eventName.AsChar());
		}
		dynamicEvent.m_lastTimeWasPlaying = GGame->GetEngineTime();
		dynamicEvent.m_nextVariance = (GEngine->GetRandomNumberGenerator().Get< Float >(2.f) -1.f)*dynamicEvent.m_repeatTimeVariance;
	}

	// modify the intensity parameter ( what ever it is )
	if ( m_intensityParameter > -1 )
	{
		GSoundSystem->GetAmbientManager().SoundParameter( "intensity", m_intensityParameter, m_intensityParameterFadeTime );
	}

	return true;
}

void CSoundAmbientAreaComponent::Stop()
{
	SoundStopAll();

	if(m_isWalla)
	{
		for (int i=0; i < Num_WallaDirections; i++)
		{
			m_wallaSoundEmitters[i].SoundStopAll();
		}
	}
}

Float CSoundAmbientAreaComponent::GetGatewayRotationInRad() const
{
	return DEG2RAD(m_gatewayRotation);
}

Vector CSoundAmbientAreaComponent::GetGatewayDirection() const
{
	Vector direction = Vector(Red::Math::MCos(GetGatewayRotationInRad()), Red::Math::MSin(GetGatewayRotationInRad()), 0.f);
	return GetLocalToWorld().TransformVector(direction).Normalized3();

}

Vector CSoundAmbientAreaComponent::GetUpDirection() const
{
	return GetLocalToWorld().GetAxisZ().Normalized3();
}

#ifdef SOUND_DEBUG

void CSoundAmbientAreaComponent::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag )
{
	TBaseClass::OnGenerateEditorFragments( frame, flag );

	if ( frame->GetFrameInfo().IsShowFlagOn( SHOW_Sound ) )
	{		
	#ifndef NO_COMPONENT_GRAPH
		CBitmapTexture* icon = resSoundSourceIcon.LoadAndGet< CBitmapTexture >();
		if ( icon != NULL )
		{
			// Draw editor icons
			frame->AddSprite( m_decoyPosition, 0.15f, Color::WHITE, GetHitProxyID(), icon );
		}
	#endif

		if ( m_isRegistered )
		{
			// Overlap string
			const String text0 = String::Printf( TXT( "Dist: %.2f m" ), m_playerDistance );	
			frame->AddDebugText( Vector( m_decoyPosition.X, m_decoyPosition.Y, m_decoyPosition.Z - 0.3f ), text0, 0, 0, false );

			// Draw the sound position
			frame->AddDebugSphere( m_decoyPosition, 0.2f, Matrix::IDENTITY, Color::GREEN, true );

			Vector pos = GetOcclusionPosition();
			frame->AddDebugSphere( pos, 0.15f, Matrix::IDENTITY, Color::RED, true );

		}
	}

	if( frame->GetFrameInfo().IsShowFlagOn(SHOW_SoundAmbients) )
	{
		if(m_isGate)
		{
			Vector direction = GetGatewayDirection();

			Vector crossDirection = Vector::Cross(direction, GetUpDirection());
			Vector centre = GetBoundingBox().CalcCenter();
			Float crossLength = GetBoudingAreaRadius();
			Float arrowLength = crossLength/2.f;
			Vector arrowPos = centre - (direction*arrowLength/2.f);
			Vector crossPos = centre - (crossDirection*crossLength/2.f);
			Float arrowThickness = Min(0.05f, 0.01f * arrowLength);

			frame->AddDebug3DArrow(arrowPos, direction, arrowLength, arrowThickness, arrowThickness*2.f, arrowLength/5.f, Color::LIGHT_GREEN, Color::LIGHT_GREEN);
			frame->AddDebug3DArrow(crossPos, crossDirection, crossLength, arrowThickness, 0.f, 0.f, Color::LIGHT_GREEN, Color::LIGHT_GREEN);
		}
	
		if(m_isWalla)
		{
			String debugText = String::Printf(TXT("Num Actors "));
			Vector zonePos  = GetSoundPosition();
			Vector direction = GSoundSystem->GetListenerDirection();

			String dirNames[Num_WallaDirections] = 
			{
				String(TXT("FR")),
				String(TXT("BR")),
				String(TXT("BL")),
				String(TXT("FL"))
			};

			auto addText = [frame, this, &debugText, &direction, &zonePos](ESoundWallaDirection dir, String dirName)
			{
				frame->AddDebugText(zonePos + direction + CSoundWallaSystem::GetDirectionVector(dir, DEG2RAD(m_wallaRotation)) * 0.3f, 
					debugText + 
					String::Printf(TXT("%s: Out: %u, In: %u, Afr: %u"),
					dirName.AsChar(),
					m_wallaMetrics.numActors[dir], 
					m_wallaMetrics.numInteriorActors[dir],
					m_wallaMetrics.numAfraidActors[dir]), 0, 0, true, Color::WHITE, Color::BLACK, nullptr, true);
			};

		
			frame->AddDebugSphere(zonePos + direction, 0.1f, Matrix::IDENTITY, Color::GREEN, true, true);
		
		
			for(int i=0; i<Num_WallaDirections; i++)
			{
				if(m_wallaSoundEmitters[i].SoundIsActive())
				{
					addText((ESoundWallaDirection)i, dirNames[i]);
				}
			}
		}
	}
}

void CSoundAmbientAreaComponent::UpdateDebug( const Vector& position, Float distance )
{
	m_decoyPosition = position;
	m_playerDistance = distance;
}

#endif

void CSoundAmbientAreaComponent::EnteredOuterArea( CComponent* component, const class ITriggerActivator* activator )
{
	if( CSoundEmitterComponent* soundEmitterComponent = Cast< CSoundEmitterComponent >( component ) )
	{
		soundEmitterComponent->SoundFlagSet( ESoundEmitterFlags::SEF_ForceUpdateReverb, true );
		return;
	}

	// An ambient activator has entered the outer area, add to the ambient list
	if ( ( NULL != activator ) && ( 0 != (activator->GetMask() & TC_SoundAmbientArea) ) )
	{
		RegisterAudioSource();
	}
}

void CSoundAmbientAreaComponent::ExitedOuterArea( CComponent* component, const class ITriggerActivator* activator )
{
	if( CSoundEmitterComponent* soundEmitterComponent = Cast< CSoundEmitterComponent >( component ) )
	{
		soundEmitterComponent->SoundFlagSet( ESoundEmitterFlags::SEF_ForceUpdateReverb, true );
		return;
	}

	// An ambient activator has exited the outer area, remove from the ambient list
	if ( ( NULL != activator ) && ( 0 != (activator->GetMask() & TC_SoundAmbientArea) ) )
	{
		UnregisterAudioSource();
	}
}

void CSoundAmbientAreaComponent::EnteredArea( CComponent* component )
{
	if( CSoundEmitterComponent* soundEmitterComponent = Cast< CSoundEmitterComponent >( component ) )
	{
		soundEmitterComponent->SoundFlagSet( ESoundEmitterFlags::SEF_ForceUpdateReverb, true );
		return;
	}

	if(m_state != Inside)
	{
		m_state = Entering;
	}

	m_innerActivated = true;
}

void CSoundAmbientAreaComponent::ExitedArea( CComponent* component )
{
	if( CSoundEmitterComponent* soundEmitterComponent = Cast< CSoundEmitterComponent >( component ) )
	{
		soundEmitterComponent->SoundFlagSet( ESoundEmitterFlags::SEF_ForceUpdateReverb, true );
		return;
	}

	if(m_state != Outside)
	{
		m_state = Exiting;
	}
	m_innerActivated = false;
}

void CSoundAmbientAreaComponent::OnEnterEvents()
{
	if(m_state == Inside)
	{
		return;
	}

	if(IsPriorityParameterMusic() && GSoundSystem->MusicTriggersAreBlocked())
	{
		m_state = Entering;
		return;
	}

	//Events are triggered through the gate mechanism instead
	if(!m_isGate)
	{
		ProcessEnterEvents();
	}

	m_state = Inside;
}

void CSoundAmbientAreaComponent::OnExitEvents()
{
	if(m_state == Outside)
	{
		return;
	}

	if(IsPriorityParameterMusic() && GSoundSystem->MusicTriggersAreBlocked())
	{
		//Only go into the exiting state if we've already triggered entry events for the area
		if(m_state == Inside)
		{
			m_state = Exiting;
		}
		else
		{
			m_state = Outside;
		}
		return;
	}

	//Events are triggered through the gate mechanism instead
	if(!m_isGate)
	{
		ProcessExitEvents();
	}
	
	m_state = Outside;
}

CSoundAmbientAreaComponent::EGatewayState CSoundAmbientAreaComponent::CalcListenerGatewayState()
{
	if(!m_innerActivated)
	{
		return Gateway_None;
	}

	Vector center = GetBoundingBox().CalcCenter();
	center.SetZ(0.f);

	Vector listenerPos = GSoundSystem->GetListenerPosition();
	listenerPos.SetZ(0.f);

	Vector direction = GetGatewayDirection();

	if(Vector::Dot2(direction, listenerPos - center) < 0.f)
	{
		return Gateway_Exit;
	}

	return Gateway_Enter;
}

void CSoundAmbientAreaComponent::OnTickPostUpdateTransform( Float timeDelta )
{
	PC_SCOPE_PIX( SoundAmbientAreaComponent_OnTickPostUpdateTransform );

	if(m_isGate)
	{
		m_gatewayRotation = Max(0.f, Min(m_gatewayRotation, 360.f));
		EGatewayState newState = CalcListenerGatewayState();
		switch(newState)
		{
		case Gateway_Enter:
			if(m_gatewayState == Gateway_Exit)
			{
				ProcessEnterEvents();
				m_gatewayState = newState;
			}
			break;
		case Gateway_Exit:
			if(m_gatewayState == Gateway_Enter)
			{
				ProcessExitEvents();
				m_gatewayState = newState;
			}
			break;
		}
		m_gatewayState = newState;
	}

	switch(m_state)
	{
		case Entering :
			OnEnterEvents(); 
			break;
		case Exiting :
			OnExitEvents();
			break;
		default :
			break;
	}


	//Clamp the walla rotation
	m_wallaRotation = Max(0.f, Min(360.f, m_wallaRotation));

	
	CTriggerAreaComponent::OnTickPostUpdateTransform( timeDelta );

	// update only when registered
	if ( !m_isRegistered ) return;
	UpdateSoundPosition( GSoundSystem->GetListenerPosition() );
	CSoundEmitter::OnTick( timeDelta );


	if(ShouldCullFromParameters())
	{
		if(m_isPlaying)
		{
			Stop();
		}
		m_isPlaying = false;
		return;
	}

	for(auto dynamicParamter : m_dynamicParameters)
	{
		ProcessDynamicParameter(dynamicParamter);
	}

	if(m_isWalla)
	{
		ProcessWalla(timeDelta);
	}

	ProcessDynamicEvents();

	if( m_isPlaying )
	{
		TickSounds();
		return;
	}

	if( SoundIsActive() || WallaIsActive() )
	{
		m_isPlaying = true;
	}
	
	if( !LoadedFully() ) return;

	Play();
}

const Vector& CSoundAmbientAreaComponent::GetSoundPosition() const
{
	return m_lastSourcePosition;
}

const Matrix CSoundAmbientAreaComponent::GetSoundPlacementMatrix() const
{
	Matrix ret = Matrix::IDENTITY;
	ret.SetTranslation( m_lastSourcePosition );
	return ret;
}

void CSoundAmbientAreaComponent::ProcessExitEvents()
{
	const Uint32 count = m_soundEventsOnExit.Size();
	for( Uint32 i = 0; i != count; ++i )
	{
		PendProcessing( this, this );
		if(m_enterExitEventsUsePosition)
		{
			SoundEvent( m_soundEventsOnExit[ i ].AsChar() );
		}
		else
		{
			GSoundSystem->SoundEvent( m_soundEventsOnExit[ i ].AsChar() );
		}
	}
}

void CSoundAmbientAreaComponent::ProcessEnterEvents()
{
	const Uint32 count = m_soundEventsOnEnter.Size();
	for( Uint32 i = 0; i != count; ++i )
	{
		PendProcessing( this, this );
		if(m_enterExitEventsUsePosition)
		{
			SoundEvent( m_soundEventsOnEnter[ i ].AsChar() );
		}
		else
		{
			GSoundSystem->SoundEvent( m_soundEventsOnEnter[ i ].AsChar() );
		}
	}
}

void CSoundAmbientAreaComponent::ProcessWalla(Float timeDelta)
{
	PC_SCOPE_PIX( CSoundAmbientAreaComponent ProcessWalla )


	Vector position = GetBoundingBox().CalcCenter();


	SoundWallaProcessParams wallaParams;
	wallaParams.minDistance = m_wallaMinDistance;
	wallaParams.maxDistance = m_wallaMaxDistance;
	wallaParams.wallaRotation = DEG2RAD(m_wallaRotation);
	Box wallaBox = GetBoundingBox() - GetBoundingBox().CalcCenter();
	wallaBox.Extrude(m_wallaBoxExtention);

	SoundWallaMetrics oldMetrics = m_wallaMetrics;

	m_wallaMetrics.UpdateFromMetrics(GSoundSystem->GetWallaSystem().CalculateSoundWallaMetrics(position, wallaBox, wallaParams));

	Float omniWallaContribution = m_wallaOmniFactor * m_wallaMetrics.totalNumActors;

	Uint32 totalInteriorActors = 0;
	Float averageDistance = 0.f, averageInteriorDistance = 0.f;

	auto computeDistContribution = [](const SoundWallaMetrics &metrics, Uint32 direction)
	{
		if(metrics.totalNumActors == 0)
		{
			return 0.f;
		}
		return Float(metrics.numActors[direction])/metrics.totalNumActors;
	};

	auto computeInteriorDistContribution = [](const SoundWallaMetrics &metrics, Uint32 direction)
	{
		if(metrics.totalNumInteriorActors == 0)
		{
			return 0.f;
		}
		return Float(metrics.numInteriorActors[direction])/metrics.totalNumInteriorActors;
	};

	for (Uint32 direction =0; direction < Num_WallaDirections; direction++)
	{
		if(m_wallaAfraidTimer > 0.f)
		{
			m_wallaAfraidTimer -= timeDelta;
		}
		else if(m_wallaMetrics.numAfraidActors[direction] > (oldMetrics.numAfraidActors[direction] + 2))
		{
			m_wallaSoundEmitters[direction].SoundEvent("AfraidWallaOneShot");
			m_wallaAfraidTimer = m_wallaAfraidRetriggerTime;
		}
		m_wallaSoundEmitters[direction].SoundParameter("WallaActors", Float(m_wallaMetrics.numActors[direction] + omniWallaContribution));
		m_wallaSoundEmitters[direction].SoundParameter("WallaDistance", m_wallaMetrics.avgDistance[direction]);
		m_wallaSoundEmitters[direction].SoundParameter("AfraidActors", Float(m_wallaMetrics.numAfraidActors[direction]));
		m_wallaSoundEmitters[direction].Reverb(GetWorld()->GetTriggerManager());
		m_wallaSoundEmitters[direction].OnTick(timeDelta);

		if(m_wallaMetrics.numAfraidActors[direction] > 0.f)
		{
			m_wallaMetrics.numAfraidActors[direction] -= timeDelta*m_wallaAfraidDecreaseRate;
			m_wallaMetrics.numAfraidActors[direction] = Max(m_wallaMetrics.numAfraidActors[direction], 0.f);
		}

		totalInteriorActors += m_wallaMetrics.numInteriorActors[direction];
		averageDistance += m_wallaMetrics.avgDistance[direction]*computeDistContribution(m_wallaMetrics, direction);
		averageInteriorDistance += m_wallaMetrics.avgInteriorDistance[direction]*computeInteriorDistContribution(m_wallaMetrics, direction);
	}

	SoundParameter("WallaActors", Float(m_wallaMetrics.totalNumActors));
	SoundParameter("WallaInteriorActors", Float(totalInteriorActors));
	SoundParameter("WallaDistance", averageDistance);
	SoundParameter("WallaInteriorDistance", averageInteriorDistance);
}

bool CSoundAmbientAreaComponent::WallaIsActive()
{
	for(auto& sound : m_wallaSoundEmitters)
	{
		if(sound.SoundIsActive())
		{
			return true;
		}
	}
	return false;
}

void CSoundAmbientAreaComponent::ProcessDynamicParameter(ESoundAmbientDynamicParameter dynamicParamter)
{
	switch(dynamicParamter)
	{
	case ESADP_None :
	default:
		return;
	}
}

void CSoundAmbientAreaComponent::UpdateSoundPosition( const Vector& listenerPosition )
{
	// Recalculate only if the listener position moved
	if ( listenerPosition != m_lastListenerPosition )
	{
		m_lastListenerPosition = listenerPosition;

		// Max search distance is the maximum of vertical and horizontal bevel sizes, also there's a small epsilon added for "safety"
		const Float maxDistanceEpsilon = 1.0f;
		const Float maxSearchDistance = Max< Float >( m_maxDistance, m_maxDistanceVertical ) + maxDistanceEpsilon;

		// NOTE: this should be called only when we are known to be inside beleved region 
		// m_maxDistance from the original area shape
		Vector closestPoint;
		Float closestDistance;
		if ( !FindClosestPoint( listenerPosition, maxSearchDistance, closestPoint, closestDistance ) )
		{
			// out of range, should not happen under normal circumstances use some default position
			WARN_ENGINE( TXT("Sound Area '%ls': GetSoundPosition queried with point outside maxRange"), GetFriendlyName().AsChar() );
			closestPoint = GetWorldPosition();
		}

		const Float distance = listenerPosition.DistanceTo( closestPoint );
	#ifdef SOUND_DEBUG
		// Update internal debug information
		UpdateDebug( closestPoint, distance );
	#endif

		// Update sound placement matrix
		m_lastSourcePosition = closestPoint;

		if(m_useListernerDistance)
		{
			SoundParameter("distance", distance);
		}

		if(m_isWalla)
		{
			for(Uint32 direction=0; direction < Num_WallaDirections; direction++)
			{
				Vector position = closestPoint + CSoundWallaSystem::GetDirectionVector((ESoundWallaDirection)direction, DEG2RAD(m_wallaRotation))*m_wallaEmitterSpread;
				m_wallaSoundEmitters[direction].SetSoundPosition(position);
			}
		}
	}
}

Bool CSoundAmbientAreaComponent::IsPlaying() const
{
	return SoundIsActive();
}

Int32 CSoundAmbientAreaComponent::GetAmbientPriority() const
{	
	return m_innerActivated ? GetTriggerPriority() : -1;
}

const StringAnsi& CSoundAmbientAreaComponent::GetReverbName()
{
	return m_reverb.m_enabled ? m_reverb.m_reverbName : StringAnsi::EMPTY;
}

Float CSoundAmbientAreaComponent::CalcBevelRadius() const
{
	return m_maxDistance;
}

Float CSoundAmbientAreaComponent::CalcVerticalBevelRadius() const
{
	return m_maxDistanceVertical;
}

const Matrix CWallaSoundEmitter::GetSoundPlacementMatrix() const
{
	Matrix ret = Matrix::IDENTITY;
	ret.SetTranslation( m_soundPosition );
	return ret;
}

CWallaSoundEmitter& CWallaSoundEmitter::Reverb( class ITriggerManager* triggerManager)
{
	if( !triggerManager ) return *this;

	CSoundListenerComponent* listner = GSoundSystem->GetSoundListner();
	if( !listner ) return *this;

	PC_SCOPE_PIX( CLightweightSoundEmitter Reverb );

	ITriggerManager::TResultTriggers triggers;

	triggerManager->GetTriggersAtPoint( GetSoundPosition(), TC_SoundReverbArea, triggers );

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
			float soundTriggerPenetration = ambientComponent->CalcPenetrationFraction( GetSoundPosition() );
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