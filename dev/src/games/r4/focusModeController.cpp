/**
* Copyright c 2012 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "focusModeController.h"
#include "../../common/core/gatheredResource.h"
#include "../../common/engine/renderCommands.h"

RED_DEFINE_STATIC_NAME( OnUpdateFocus );
RED_DEFINE_STATIC_NAME( OnFocusModeDimmingFinished );
RED_DEFINE_STATIC_NAME( OnFocusModeSound );
RED_DEFINE_STATIC_NAME( OnFocusModeEnabled );
RED_DEFINE_STATIC_NAME( W3MonsterClue );
RED_DEFINE_STATIC_NAME( sound_clue );
RED_DEFINE_STATIC_NAME( SetCurrentSoundClue );
RED_DEFINE_STATIC_NAME( testLineOfSight );
RED_DEFINE_STATIC_NAME( SoundClues );
RED_DEFINE_STATIC_NAME( ScentClues );
RED_DEFINE_STATIC_NAME( FocusModeVisibility );
RED_DEFINE_STATIC_NAME( entityHandle );
RED_DEFINE_STATIC_NAME( activationTime );
RED_DEFINE_STATIC_NAME( intensity );
RED_DEFINE_STATIC_NAME( no_focus_glow );

namespace
{

const CName UPDATE_FOCUS_EVENT			= CNAME( OnUpdateFocus );
const CName ENABLE_FOCUS_SOUND_EVENT	= CNAME( OnFocusModeSound );
const char* FOCUS_SOUND_PARAM			= "focus_aim"; // sound system needs char
const CName FOCUS_ENTITY_CLASS			= CNAME( W3MonsterClue );
const CName CUSTOM_SOUND_CLUE_TAG		= CNAME( sound_clue );
const CName SET_ACTIVE_SOUND_CLUE		= CNAME( SetCurrentSoundClue );
const Float SOUND_CLUE_DURATION			= 1.0f;
const Float	SCENT_CLUE_FADE_IN_DURATION		= 0.0f;
const Float	SCENT_CLUE_FADE_OUT_DURATION	= 2.0f;
const CName SCENT_CLUE_EFFECT_PARAM			= CNAME( MeshEffectScalar3 );

}

CGatheredResource resFocusClueRanges( TXT("gameplay\\globals\\focus_clues_ranges.csv"), RGF_Startup );

const Float CFocusModeController::ACTIVATION_DURATION				= 1.0f;
const Float CFocusModeController::DIMMING_DURATION					= 1.5f;		// HACK: as long as we don't synchronize dimming speed/duration with renderer

// loaded from csv
Float CFocusModeController::CLUES_UPDATE_INTERVAL					= 10.0f;
Float CFocusModeController::CLUES_UPDATE_RANGE						= 50.f;
Float CFocusModeController::SOUND_CLUES_UPDATE_INTERVAL				= 10.f;
Float CFocusModeController::SOUND_CLUES_RANGE						= 200.f;
Float CFocusModeController::ENEMIES_UPDATE_INTERVAL					= 10.0f;
Float CFocusModeController::ENEMIES_RANGE							= 200.f;
Float CFocusModeController::CUSTOM_SOUND_CLUES_UPDATE_INTERVAL		= 10.0f;
Float CFocusModeController::CUSTOM_SOUND_CLUES_RANGE				= 200.f;

const Float CFocusModeController::EFFECT_DESATURATION				= 0.75f;	// How much to desaturate the screen. 0=full color; 1=gray
const Float CFocusModeController::EFFECT_HIGHLIGHT_BOOST			= 0.25f;	// Fade between gray*highlightColor and highlightColor. Higher can make it brighter, but lose surface details.
const Float CFocusModeController::EFFECT_FADE_NEAR					= 3.0f;						// Distance from camera where fading starts
const Float CFocusModeController::EFFECT_FADE_FAR					= 15.0f;					// Distance from camera where faded out entirely
const Float CFocusModeController::EFFECT_DIMMING_TIME				= 6.0f;					// How long it will take to dim marked elements
const Float CFocusModeController::EFFECT_DIMMING_SPEED				= 6.0f;					// How fast the focus will expand over distance

IMPLEMENT_ENGINE_CLASS( CFocusActionComponent );
IMPLEMENT_ENGINE_CLASS( CFocusModeController );
IMPLEMENT_ENGINE_CLASS( CFocusSoundParam );

//////////////////////////////////////////////////////////////////////////

CFocusModeController::SSoundClueParams::SSoundClueParams()
	: m_eventStart( CName::NONE )
	, m_eventStop( CName::NONE )
	, m_hearingAngleCos( -1.0f )
	, m_effectType( 0 )
{
}

//////////////////////////////////////////////////////////////////////////

CFocusModeController::SSoundClue::SSoundClue()
	: m_entity( nullptr )
	, m_soundEmitter( nullptr )
	, m_timeLeft( 0.0f )
{
}

CFocusModeController::SSoundClue::SSoundClue( CGameplayEntity* entity )
	: m_entity( entity )
	, m_soundEmitter( nullptr )
	, m_timeLeft( 0.0f )
{
}

void CFocusModeController::SSoundClue::Init( CFocusModeController* parent )
{
	if ( m_entity == nullptr )
	{
		return;
	}
	m_soundEmitter = m_entity->GetSoundEmitterComponent();
	if ( !parent->GetOverridenSoundClueParams( m_entity, m_params ) )
	{
		CEntityTemplate* templ = m_entity->GetEntityTemplate();
		if ( templ != nullptr )
		{
			CFocusSoundParam* param = templ->FindGameplayParamT< CFocusSoundParam >( true );
			RED_ASSERT( param != nullptr );
			if ( param != nullptr )
			{
				m_params.m_eventStart = param->GetEventStart();
				m_params.m_eventStop = param->GetEventStop();
				m_params.m_hearingAngleCos = MCos( DEG2RAD( param->GetHearingAngle() ) );
			}																	  
		}
	}

	if ( m_params.m_effectType != 0 )
	{
		CallFunction( m_entity, CName( TXT("SetFocusModeSoundEffectType") ), m_params.m_effectType );
	}
}

Bool CFocusModeController::SSoundClue::IsValid() const
{
	return ( m_entity != nullptr && m_soundEmitter != nullptr && m_params.m_eventStart != CName::NONE );
}

Bool CFocusModeController::SSoundClue::IsAlive() const
{
	if ( m_entity->IsA< CActor >() )
	{
		return static_cast< CActor* >( m_entity.Get() )->IsAlive();
	}
	// non-actors are always alive
	return true;
}

Bool CFocusModeController::SSoundClue::IsInHearingAngle( const SFocusPosition& focusPosition ) const
{
	RED_ASSERT( IsValid() );

	Vector focusToSound2D = m_entity->GetWorldPosition() - focusPosition.m_position;
	focusToSound2D.Z = 0;
	focusToSound2D.Normalize2();
	return Vector::Dot2( focusToSound2D, focusPosition.m_direction2D ) >= m_params.m_hearingAngleCos;
}

Bool CFocusModeController::SSoundClue::Update( Float deltaTime )
{
	RED_ASSERT( IsValid() );

	m_timeLeft = Max( 0.0f, m_timeLeft - deltaTime );
	if ( m_timeLeft == 0.0f )
	{
		Stop();
		return false;
	}
	else if ( !IsAlive() )
	{
		Stop();
		return false;
	}
	return true;
}

void CFocusModeController::SSoundClue::Start()
{
	RED_ASSERT( IsValid() );

	CSoundEmitterComponent* soundEmitterComponent = m_entity->GetSoundEmitterComponent();
	if( soundEmitterComponent )
	{
		soundEmitterComponent->SoundEvent( m_params.m_eventStart.AsAnsiChar() );
	}
#ifndef RED_FINAL_BUILD
	RED_LOG( FocusMode, TXT( "SoundClue started: %s" ), m_params.m_eventStart.AsChar() );
#endif

	ResetTimer();
}

void CFocusModeController::SSoundClue::Stop()
{
	RED_ASSERT( IsValid() );

	CSoundEmitterComponent* soundEmitterComponent = m_entity->GetSoundEmitterComponent();
	if( soundEmitterComponent )
	{
		soundEmitterComponent->SoundEvent( m_params.m_eventStop.AsAnsiChar() );
	}
	
#ifndef RED_FINAL_BUILD
	RED_LOG( FocusMode, TXT( "SoundClue stopped: %s" ), m_params.m_eventStop.AsChar() );
#endif

	m_timeLeft = 0.0f;
}

void CFocusModeController::SSoundClue::ResetTimer()
{
	m_timeLeft = SOUND_CLUE_DURATION;
}

void CFocusModeController::SSoundClue::UpdateAccuracy( const SFocusPosition& focusPosition )
{
	RED_ASSERT( IsValid() );
	m_soundEmitter->SoundParameter( FOCUS_SOUND_PARAM, CalcAccuracy( focusPosition ) );
}

Float CFocusModeController::SSoundClue::CalcAccuracy( const SFocusPosition& focusPosition ) const
{
	RED_ASSERT( IsValid() );

	// computed accuracy is an "approximated" angle between two vectors
	// so that 0 means that both vectors point the same direction
	// "angle" part is computed with dot product
	// "sign" part is computed with cross product (actually, since both vectors lie in XY plane only 'z' factor needs to be computed)

	// project 4D input vectors onto XY plane (remove Z and W components)
	const Vector& soundPosition = m_entity->GetWorldPositionRef();
	Vector focusToSound2D = soundPosition - focusPosition.m_position;
	focusToSound2D.Z = 0;
	focusToSound2D.Normalize2();

	// determining cos(angle) with dot
	Float acuracy = ( 1.0f - Vector::Dot2( focusToSound2D, focusPosition.m_direction2D ) ) * 0.5f;

	// determine sign by computing the 'z' factor of the normal vector (cross-product)
	Float z = focusToSound2D.X * focusPosition.m_direction2D.Y - focusToSound2D.Y * focusPosition.m_direction2D.X;
	if ( z < 0 )
	{
		acuracy = -acuracy;
	}

	return acuracy * 180.0f;
}

void CFocusModeController::SSoundClue::DrawDebug( CRenderFrame* frame, const SFocusPosition& focusPosition ) const
{
	CEntity* entity = m_entity.Get();
	if ( entity != nullptr )
	{
		Box entityBoundingBox = entity->CalcBoundingBox();
		Vector entityPosition = entity->GetWorldPosition();
		Float radius = entityBoundingBox.CalcExtents().Mag3() * 1.1f;
		Float accuracy = CalcAccuracy( focusPosition );
		String text = String::Printf( TXT("sound accuracy: %.2f"), accuracy );
		frame->AddDebugSphere( entityPosition, radius, Matrix::IDENTITY, Color::CYAN );
		frame->AddDebugText( entityPosition, text, 0, 1, true ); 
	}
}

Bool CFocusModeController::SSoundClue::Equal( const SSoundClue& a, const SSoundClue& b )
{
	return ( a.m_entity.Get() == b.m_entity.Get() );
}

//////////////////////////////////////////////////////////////////////////

CFocusModeController::SIsSoundClueEvaluator::SIsSoundClueEvaluator( CFocusModeController* parent )
	: m_parent( parent )
{
}

Bool CFocusModeController::SIsSoundClueEvaluator::operator()( const CGameplayEntity* entity ) const
{
	if ( m_parent->GetOverridenSoundClueParams( entity, m_params ) )
	{
		return m_params.m_eventStart != CName::NONE;
	}
	CEntityTemplate* templ = entity->GetEntityTemplate();
	if ( templ != nullptr )
	{
		CFocusSoundParam* param = templ->FindGameplayParamT< CFocusSoundParam >( true );
		if ( param != nullptr )
		{
			return param->GetEventStart() != CName::NONE;
		}
	}
	return false;
}

CFocusModeController::SScentClue::SScentClue()
{
}

CFocusModeController::SScentClue::SScentClue( const CName& effectName, CEntity* entity, Float duration, Uint32 worldHash )
	: m_effectName( effectName )
	, m_duration( duration )
	, m_isActive( false )
	, m_activationTime( 0.0f )
	, m_intensity( 0.0f )
	, m_worldHash( worldHash )
{
	m_entityHandle.Set( entity );
	Activate( true );
}

CFocusModeController::SScentClue::SScentClue( IGameLoader* loader )
{
	CGameSaverBlock block( loader, CNAME( p ) );

	loader->ReadValue( CNAME( effectName ), m_effectName );
	loader->ReadValue( CNAME( entityHandle ), m_entityHandle );
	loader->ReadValue( CNAME( duration ), m_duration );
	loader->ReadValue( CNAME( activationTime ), m_activationTime );
	loader->ReadValue( CNAME( intensity ), m_intensity );
	loader->ReadValue( CNAME( isActive ), m_isActive );
	loader->ReadValue( CNAME( hash ), m_worldHash );
}

CFocusModeController::SScentClue::~SScentClue()
{}


void CFocusModeController::SScentClue::Activate( Bool activate )
{
	if ( activate != m_isActive )
	{
		m_isActive = activate;

		if ( m_isActive )
		{
			m_activationTime = GGame->GetEngineTime() - m_intensity * SCENT_CLUE_FADE_IN_DURATION;
		}
		else
		{
			m_activationTime = GGame->GetEngineTime() - ( 1.0f - m_intensity ) * SCENT_CLUE_FADE_OUT_DURATION;
		}
	}
}

Bool CFocusModeController::SScentClue::Update( Float focusIntensity )
{
	if ( m_activationTime == 0.0f )
	{
		return false;
	}
	if ( m_entityHandle.Empty() || m_entityHandle.Get() == nullptr )
	{
		return false;
	}

	const Float currentTime = GGame->GetEngineTime();
	const Float deltaTime = currentTime - m_activationTime;
	if ( m_isActive )
	{
		if ( deltaTime < SCENT_CLUE_FADE_IN_DURATION )
		{
			const Float intensity = SCENT_CLUE_FADE_IN_DURATION * focusIntensity;
			if ( intensity != 0.0f )
			{
				SetIntensity( deltaTime / intensity );
			}
			else
			{
				SetIntensity( 0.0f );
			}

		}
		else if ( m_duration == -1.0f )
		{
			SetIntensity( focusIntensity );
		}
		else
		{
			SetIntensity( 0.0f );
			m_isActive = false;
			m_activationTime = 0.0f;
			return false;
		}
	}
	else
	{
		if ( deltaTime < SCENT_CLUE_FADE_OUT_DURATION )
		{
			SetIntensity( ( 1.0f - deltaTime / SCENT_CLUE_FADE_OUT_DURATION ) * focusIntensity );
		}
		else
		{
			SetIntensity( 0.0f );
			m_activationTime = 0.0f;
			return false;
		}
	}

	return true;
}

void CFocusModeController::SScentClue::SetIntensity( Float newIntensity )
{
	if ( newIntensity != m_intensity )
	{
		m_intensity = newIntensity;

		CEntity* entity = m_entityHandle.Get();
		if ( entity )
		{
			entity->SetEffectIntensity( m_effectName, m_intensity, CName::NONE, SCENT_CLUE_EFFECT_PARAM );
		}
	}
}

void CFocusModeController::SScentClue::Save( IGameSaver* saver ) const
{
	CGameSaverBlock block( saver, CNAME( p ) );

	saver->WriteValue( CNAME( effectName ), m_effectName );
	saver->WriteValue( CNAME( entityHandle ), m_entityHandle );
	saver->WriteValue( CNAME( duration ), m_duration );
	saver->WriteValue( CNAME( activationTime ), m_activationTime );
	saver->WriteValue( CNAME( intensity ), m_intensity );
	saver->WriteValue( CNAME( isActive ), m_isActive );
	saver->WriteValue( CNAME( hash ), m_worldHash );
}

//////////////////////////////////////////////////////////////////////////

CFocusModeController::SEntitiesDetector::SEntitiesDetector( CFocusModeController* parent )
	: m_parent( parent )
{}

//////////////////////////////////////////////////////////////////////////

CFocusModeController::SEntitiesWithStateDetector::SEntitiesWithStateDetector( CFocusModeController* parent, TClues* clues )
	: SEntitiesDetector( parent )
	, m_clues( clues )
{
}

void CFocusModeController::SEntitiesWithStateDetector::BeginEntitiesDetection()
{
	RED_ASSERT( m_oldClues.Size() == 0 );
	m_oldClues.PushBack( *m_clues );
	m_clues->ClearFast();
}

void CFocusModeController::SEntitiesWithStateDetector::EndEntitiesDetection()
{
	for ( const TClue& clue : m_oldClues )
	{
		CGameplayEntity* entity = clue.Get();
		if ( entity != nullptr )
		{
			OnClueStateChange( entity, false );
		}
	}
	m_oldClues.ClearFast();
}

void CFocusModeController::SEntitiesWithStateDetector::ProcessDetectedEntity( CGameplayEntity* entity )
{
	if ( entity != nullptr && CheckClueCondition( entity ) )
	{
		TClue clue = entity;
		// we check if clue state was previously enabled
		TClues::iterator it = Find( m_oldClues.Begin(), m_oldClues.End(), clue );
		// if yes, we just remove it from the list of "old" clues
		// (they will be deactivated while detection finishes)
		if ( it != m_oldClues.End() )
		{
			m_oldClues.Erase( it );
		}
		// if it's new clue, we enable it
		else
		{
			OnClueStateChange( entity, true );
		}
		m_clues->PushBack( clue );
	}
}

//////////////////////////////////////////////////////////////////////////

CFocusModeController::SGameplayEntitiesDetector::SGameplayEntitiesDetector( CFocusModeController* parent )
	: SEntitiesWithStateDetector( parent, &m_gameplayEntities )
{
}

Bool CFocusModeController::SGameplayEntitiesDetector::CheckClueCondition( TCluePtr clue )
{
	RED_ASSERT( clue != nullptr );
	return clue->IsA< CGameplayEntity >();
}

void CFocusModeController::SGameplayEntitiesDetector::OnClueStateChange( TCluePtr clue, Bool enabled )
{
	// we're enabling entities in focus mode only when focus mode is activated
	OnEnabledInFocusMode( clue, enabled );
}

void CFocusModeController::SGameplayEntitiesDetector::OnDeactivate()
{
	for ( const TClue& clue : m_gameplayEntities )
	{
		TCluePtr entity = clue.Get();
		if ( entity != nullptr )
		{
			OnEnabledInFocusMode( entity, false );
		}
	}
	m_gameplayEntities.ClearFast();
}

void CFocusModeController::SGameplayEntitiesDetector::OnEnabledInFocusMode( TCluePtr entity, Bool enable )
{
	RED_ASSERT( entity != nullptr );
	if ( enable )
	{
		// if focus mode visibility wasn't updated yet
		if ( !entity->GetInfoCache().Get( entity, GICT_Custom4 ) )
		{
			m_parent->RegisterForUpdateFocusModeVisibility( entity );
		}
	}
	entity->CallEvent( CNAME( OnFocusModeEnabled ), enable );
}

//////////////////////////////////////////////////////////////////////////

CFocusModeController::SFocusCluesDetector::SFocusCluesDetector( CFocusModeController* parent )
	: SEntitiesDetector( parent )
	, m_focusClueClass( nullptr )
{
}

void CFocusModeController::SFocusCluesDetector::BeginEntitiesDetection()
{
	m_focusClues.ClearFast();
	if ( m_focusClueClass == nullptr )
	{
		m_focusClueClass = SRTTI::GetInstance().FindClass( FOCUS_ENTITY_CLASS );
	}
}

void CFocusModeController::SFocusCluesDetector::ProcessDetectedEntity( CGameplayEntity* entity )
{
	if ( entity != nullptr && entity->IsA( m_focusClueClass ) )
	{
		m_focusClues.PushBack( entity );
	}
}

void CFocusModeController::SFocusCluesDetector::Update()
{
	SFocusPosition focusPosition;
	m_parent->GetFocusPosition( focusPosition );

	for ( const TClue& clue : m_focusClues )
	{
		TCluePtr entity = clue.Get();
		if ( entity != nullptr )
		{
			Vector directionToEntity = entity->GetWorldPositionRef() - focusPosition.m_cameraPosition;
			Float distance = ( entity->GetWorldPositionRef() - focusPosition.m_position ).Mag3();
			Float accuracy = m_parent->CalculateFocusAccuracy( focusPosition, entity, directionToEntity );
			entity->CallEvent( UPDATE_FOCUS_EVENT, distance, accuracy );
		}
	}
}

void CFocusModeController::SFocusCluesDetector::Clear()
{
	m_focusClues.ClearFast();
}

//////////////////////////////////////////////////////////////////////////

CFocusModeController::SSoundCluesDetector::SSoundCluesDetector( CFocusModeController* parent )
	: SEntitiesWithStateDetector( parent, &m_rawSoundClues )
	, m_player( nullptr )
{
}

void CFocusModeController::SSoundCluesDetector::BeginEntitiesDetection()
{
	SEntitiesWithStateDetector::BeginEntitiesDetection();
	m_player = GCommonGame->GetPlayer();
}

void CFocusModeController::SSoundCluesDetector::EndEntitiesDetection()
{
	SEntitiesWithStateDetector::EndEntitiesDetection();
	m_player = nullptr;
}

Bool CFocusModeController::SSoundCluesDetector::CheckClueCondition( TCluePtr clue )
{
	RED_ASSERT( clue != nullptr );
	if ( clue == nullptr )
	{
		return false;
	}
	if ( clue != m_player && clue->GetInfoCache().Get( clue, GICT_Custom0, m_parent->m_isSoundClueEvaluator ) )
	{
		if ( clue->IsA< CActor >() )
		{
			return static_cast< CActor* >( clue )->IsAlive();
		}
		return true;
	}
	return false;
}

void CFocusModeController::SSoundCluesDetector::OnClueStateChange( TCluePtr clue, Bool enabled )
{
	RED_ASSERT( clue != nullptr );
	EnableSoundClue( clue, enabled );
}

void CFocusModeController::SSoundCluesDetector::Update( Float timeDelta )
{
	RemoveInvalidClues( timeDelta );

	SFocusPosition focusPosition;
	m_parent->GetFocusPosition( focusPosition );

	// first, we find "best" clues
	m_bestSoundClues.ClearFast();
	SBestSoundClue bestSoundClue;
	for ( const auto& it : m_soundClues )
	{
		const SSoundClue& clue = it.m_second;

		// can we hear it
		const Vector& soundPosition = clue.GetWorldPosition();
		if ( !clue.IsInHearingAngle( focusPosition ) )
		{
			continue;
		}

		// is closer than "best" sound?
		const Float distanceSqr = ( soundPosition - focusPosition.m_position ).SquareMag3();
		if ( m_bestSoundClues.Find( clue.GetId(), bestSoundClue ) )
		{
			// if there is a clue for given "eventStart" and is closer that current one
			if ( distanceSqr > bestSoundClue.m_distanceSqr )
			{
				continue;
			}
		}
		bestSoundClue.m_clue = &clue;
		bestSoundClue.m_distanceSqr = distanceSqr;
		m_bestSoundClues[ clue.GetId() ] = bestSoundClue;
	}

	// then, we process "best" clues (check if can be activated and activate)
	for ( const auto& it : m_bestSoundClues )
	{
		TActiveSoundClues::iterator actIt = m_activeSounds.Find( it.m_first );
		if ( actIt != m_activeSounds.End() )
		{
			// if there's still an active sound clue that is different from
			// the current "best" one - we let it to "finish"
			if ( actIt->m_second != ( *it.m_second.m_clue ) )
			{
				continue;
			}
			// otherwise, reset its timer
			actIt->m_second.ResetTimer();
		}
		else
		{
			SSoundClue activeClue = *it.m_second.m_clue;
			activeClue.Start();
			m_activeSounds[ it.m_first ] = activeClue;
		}
	}

	// finally, we update focus sound param of all active clues
	for ( auto& it : m_activeSounds )
	{
		it.m_second.UpdateAccuracy( focusPosition );
	}
}

void CFocusModeController::SSoundCluesDetector::RemoveInvalidClues( Float timeDelta )
{
	// collect invalid active clues
	TDynArray< CName > invalidActiveClues;	
	for ( auto& it : m_activeSounds )
	{
		SSoundClue& clue = it.m_second;
		if ( !clue.IsValid() || !clue.Update( timeDelta ) )
		{
			invalidActiveClues.PushBack( it.m_first );
		}
	}

	// remove invalid active clues
	for ( CName name : invalidActiveClues )
	{
		m_activeSounds.Erase( name );
	}
	invalidActiveClues.ClearFast();

	// collect invalid clues
	TDynArray< CGUID > invalidClues;	
	for ( const auto& it : m_soundClues )
	{
		if ( !it.m_second.IsValid() || !it.m_second.IsAlive() )
		{
			invalidClues.PushBack( it.m_first );
		}
	}

	// remove invalid clues
	for ( const CGUID& guid : invalidClues )
	{
		m_soundClues.Erase( guid );
	}
	invalidClues.ClearFast();
}

void CFocusModeController::SSoundCluesDetector::OnDeactivate()
{
	for ( const TClue& clue : m_rawSoundClues )
	{
		TCluePtr entity = clue.Get();
		if ( entity != nullptr )
		{
			EnableSoundClue( entity, false );
		}
	}
	m_rawSoundClues.ClearFast();
}

void CFocusModeController::SSoundCluesDetector::OnSoundEventsChanged( TCluePtr clue )
{
	TSoundClues::iterator it = m_soundClues.Find( clue->GetGUID() );
	Bool isSoundClue = CheckClueCondition( clue );
	if ( isSoundClue && it == m_soundClues.End() )
	{
		EnableSoundClue( clue, true );
	}
	else if ( !isSoundClue && it != m_soundClues.End() )
	{
		EnableSoundClue( clue, false );
	}
}

void CFocusModeController::SSoundCluesDetector::EnableSoundClue( TCluePtr clue, Bool enable )
{
	RED_ASSERT( clue != nullptr );
	clue->CallEvent( ENABLE_FOCUS_SOUND_EVENT, enable, Config::cvColorblindFocusMode.Get() );

	const CGUID& guid = clue->GetGUID();
	TSoundClues::iterator it = m_soundClues.Find( guid );
	if ( enable && it == m_soundClues.End() )
	{
		SSoundClue newClue( clue );
		newClue.Init( m_parent );
		m_soundClues.Insert( guid, newClue );
	}
	if ( !enable && it != m_soundClues.End() )
	{
		m_soundClues.Erase( it );
	}
}

void CFocusModeController::SSoundCluesDetector::ForceClear()
{
	m_soundClues.Clear();
	m_activeSounds.Clear();
}

//////////////////////////////////////////////////////////////////////////

CFocusModeController::SEnemiesDetector::SEnemiesDetector( CFocusModeController* parent )
	: SEntitiesWithStateDetector( parent, &m_enemies )
	, m_player( nullptr )
{
}

void CFocusModeController::SEnemiesDetector::BeginEntitiesDetection()
{
	SEntitiesWithStateDetector::BeginEntitiesDetection();
	m_player = GCommonGame->GetPlayer();
}

void CFocusModeController::SEnemiesDetector::EndEntitiesDetection()
{
	SEntitiesWithStateDetector::EndEntitiesDetection();
	m_player = nullptr;
}

Bool CFocusModeController::SEnemiesDetector::CheckClueCondition( TCluePtr clue )
{
	RED_ASSERT( clue != nullptr );
	if ( m_player == nullptr )
	{
		return false;
	}
	CActor* actor = Cast< CActor >( clue );
	if ( actor != nullptr && m_player->GetAttitude( actor ) == AIA_Hostile && actor->IsAlive() )
	{
		return true;
	}
	return false;
}

void CFocusModeController::SEnemiesDetector::OnClueStateChange( TCluePtr clue, Bool enabled )
{
	RED_ASSERT( clue != nullptr );
	ShowInFocusMode( clue, enabled );
}

void CFocusModeController::SEnemiesDetector::OnDeactivate()
{
	for ( const TClue& clue : m_enemies )
	{
		TCluePtr entity = clue.Get();
		if ( entity != nullptr )		
		{
			ShowInFocusMode( entity, false );
		}
	}
	m_enemies.ClearFast();
}

void CFocusModeController::SEnemiesDetector::ShowInFocusMode( TCluePtr entity, Bool show )
{
	RED_ASSERT( entity != nullptr );

	// add all its DrawableComponents to light channel LC_Custom0
	for ( ComponentIterator< CDrawableComponent > it( entity ); it; ++it )
	{
		CDrawableComponent* dc = *it;
		dc->EnableLightChannels( show, LC_Custom0 );
	}
}

//////////////////////////////////////////////////////////////////////////

CFocusModeController::SCustomSoundCluesDetector::SCustomSoundCluesDetector( CFocusModeController* parent )
	: SEntitiesDetector( parent )
	, m_activeCustomSoundClue( nullptr )
{
}

void CFocusModeController::SCustomSoundCluesDetector::BeginEntitiesDetection()
{
	m_customSoundClues.ClearFast();
}

void CFocusModeController::SCustomSoundCluesDetector::ProcessDetectedEntity( CGameplayEntity* entity )
{
	if ( entity->GetTags().HasTag( CUSTOM_SOUND_CLUE_TAG ) )
	{
		m_customSoundClues.PushBack( entity );
	}
}

void CFocusModeController::SCustomSoundCluesDetector::Update()
{
	if ( m_customSoundClues.Size() == 0 )
	{
		return;
	}
	Float minDist = NumericLimits< Float >::Max();
	TCluePtr bestClue = nullptr;
	RED_ASSERT( GCommonGame->GetPlayer() != nullptr );
	Vector playerPos = GCommonGame->GetPlayer()->GetWorldPosition();
	for ( const TClue& clue : m_customSoundClues )
	{
		TCluePtr entity = clue.Get();
		if ( entity != nullptr )
		{
			Float dist = entity->GetWorldPosition().DistanceSquaredTo( playerPos );
			if ( dist < minDist )
			{
				minDist = dist;
				bestClue = entity;
			}
		}
	}
	SetActiveCustomSound( bestClue );
}

void CFocusModeController::SCustomSoundCluesDetector::OnDeactivate()
{
	SetActiveCustomSound( nullptr );
	m_customSoundClues.ClearFast();
}

void CFocusModeController::SCustomSoundCluesDetector::SetActiveCustomSound( TCluePtr entity )
{
	if ( entity != m_activeCustomSoundClue.Get() )
	{
		if ( m_activeCustomSoundClue.Get() != nullptr )
		{
			CallFunction( m_activeCustomSoundClue.Get(), SET_ACTIVE_SOUND_CLUE, false );
		}
		m_activeCustomSoundClue = entity;
		if ( entity != nullptr )
		{
			CallFunction( entity, SET_ACTIVE_SOUND_CLUE, true );
		}
	}
}

//////////////////////////////////////////////////////////////////////////

CFocusModeController::SUpdateFocusVisibilityEntity::SUpdateFocusVisibilityEntity()
	: m_entity( nullptr )
	, m_dist( 0.0f )
{}

CFocusModeController::SUpdateFocusVisibilityEntity::SUpdateFocusVisibilityEntity( CGameplayEntity* entity, Float dist )
	: m_entity( entity )
	, m_dist( dist )
{}

Bool CFocusModeController::SUpdateFocusVisibilityEntity::Less( const SUpdateFocusVisibilityEntity& key1, const SUpdateFocusVisibilityEntity& key2 )
{
	return key1.m_dist > key2.m_dist;
}

//////////////////////////////////////////////////////////////////////////

CFocusModeController::CFocusModeController()
	: m_gameplayEntitiesDetector( this )
	, m_focusCluesDetector( this )
	, m_soundCluesDetector( this )
	, m_enemiesDetector( this )
	, m_customSoundCluesDetector( this )
	, m_isActive( false )
	, m_activationTimer( 0.0f )
	, m_deactivationTimer( 0.0f )
	, m_dimming( false )
	, m_dimmingFactor( DIMMING_DURATION )
	, m_dimmingTime( 2.0f )
	, m_lostTestIgnoreEntities( 1 )
	, m_isSoundClueEvaluator( this )
{
}

void CFocusModeController::Initialize()
{
	SGameSessionManager::GetInstance().RegisterGameSaveSection( this );
}

void CFocusModeController::Shutdown()
{
	SGameSessionManager::GetInstance().UnregisterGameSaveSection( this );
}

void CFocusModeController::OnGameStart( const CGameInfo& gameInfo )
{
	m_isActive = false;
	m_activationTimer = 0.0f;
	m_deactivationTimer = 0.0f;

	LoadConfig();

	CEntitiesDetector* detector = GCommonGame->GetSystem< CEntitiesDetector >();
	if ( detector != nullptr )
	{
		detector->Register( &m_focusCluesDetector, CLUES_UPDATE_INTERVAL, CLUES_UPDATE_RANGE, true );
	}

	m_dynamicClueStorage.OnGameStart();

	if ( gameInfo.m_gameLoadStream )
	{
		LoadGame( gameInfo.m_gameLoadStream );
	}
}

void CFocusModeController::OnGameEnd( const CGameInfo& gameInfo )
{
	// force disable this effect
	EnableVisuals( false, 1.0f, 1.0f, true );

	CEntitiesDetector* detector = GCommonGame->GetSystem< CEntitiesDetector >();
	if ( detector != nullptr )
	{
		detector->Unregister( &m_focusCluesDetector );
	}
	m_focusCluesDetector.Clear();

	if ( !gameInfo.IsChangingWorld() )
	{
		m_scentClues.Clear();
	}

	SetActive( false );
	OnDeactivationFinished();

	m_dynamicClueStorage.OnGameEnd();
	m_storedFocusModeVisibility.Clear();
}

void CFocusModeController::OnWorldStart( const CGameInfo& gameInfo )
{
	m_currentWorldHash = GGame->GetActiveWorld()->DepotPath().CalcHash();
}

Bool CFocusModeController::OnSaveGame( IGameSaver* saver )
{
	{
		CGameSaverBlock block( saver, CNAME( SoundClues ) );

		const Uint32 size = m_overridenSoundClueParams.Size();
		saver->WriteValue( CNAME( count ), size );

		for ( const auto& it : m_overridenSoundClueParams )
		{
			CGameSaverBlock block( saver, CNAME( p ) );
			saver->WriteValue( CNAME( id ), it.m_first );
			saver->WriteValue( CNAME( 1 ), it.m_second.m_eventStart );
			saver->WriteValue( CNAME( 2 ), it.m_second.m_eventStop );
			saver->WriteValue( CNAME( a ), it.m_second.m_hearingAngleCos );
			saver->WriteValue( CNAME( type ), it.m_second.m_effectType );
		}
	}

	{
		CGameSaverBlock block( saver, CNAME( ScentClues ) );

		const Uint32 size = m_scentClues.Size();
		
		saver->WriteValue( CNAME( count ), size );

		for ( const auto& it : m_scentClues )
		{
			it.Save( saver );
		}
	}

	{
		CGameSaverBlock block( saver, CNAME( FocusModeVisibility ) );

		const Uint32 size = m_storedFocusModeVisibility.Size();
		saver->WriteValue( CNAME( count ), size );

		for ( const auto& it : m_storedFocusModeVisibility )
		{
			saver->WriteValue( CNAME( 1 ), it.m_first );
			saver->WriteValue( CNAME( 2 ), static_cast< Uint32 >( it.m_second ) );			
		}
	}

	return true;
}

void CFocusModeController::LoadGame( IGameLoader* loader )
{
	m_overridenSoundClueParams.Clear();

	{
		CGameSaverBlock block( loader, CNAME( SoundClues ) );

		Uint32 size = 0;
		loader->ReadValue( CNAME( count ), size );

		m_overridenSoundClueParams.Reserve( size );

		for ( Uint32 i = 0; i < size; ++i )
		{
			IdTag id;
			SSoundClueParams params;
			CGameSaverBlock block( loader, CNAME( p ) );
			loader->ReadValue( CNAME( id ), id );
			loader->ReadValue( CNAME( 1 ), params.m_eventStart );
			loader->ReadValue( CNAME( 2 ), params.m_eventStop );
			loader->ReadValue( CNAME( a ), params.m_hearingAngleCos );
			loader->ReadValue( CNAME( type ), params.m_effectType );

			m_overridenSoundClueParams.Insert( id, params );
		}
	}

	m_scentClues.Clear();

	{
		CGameSaverBlock block( loader, CNAME( ScentClues ) );

		Uint32 size = 0;
		loader->ReadValue( CNAME( count ), size );

		m_scentClues.Reserve( size );

		for ( Uint32 i = 0; i < size; ++i )
		{
			m_scentClues.PushBack( SScentClue( loader ) );
		}
	}
	
	m_storedFocusModeVisibility.Clear();

	{
		CGameSaverBlock block( loader, CNAME( FocusModeVisibility ) );

		Uint32 size = 0;
		loader->ReadValue( CNAME( count ), size );

		for ( Uint32 i = 0; i < size; ++i )
		{
			IdTag idTag;
			Uint32 visibility = 0;
			loader->ReadValue( CNAME( 1 ), idTag );
			loader->ReadValue( CNAME( 2 ), visibility );
			if ( idTag.IsValid() )
			{
				m_storedFocusModeVisibility.Insert( idTag, static_cast< EFocusModeVisibility >( visibility ) );
			}
		}
	}

}

void CFocusModeController::LoadConfig()
{
	C2dArray* rangesArray = resFocusClueRanges.LoadAndGet< C2dArray >();
	RED_ASSERT( rangesArray != nullptr, TXT( "Unable to open '%ls'" ), resFocusClueRanges.GetPath().ToString().AsChar() );
	if ( rangesArray == nullptr )
	{
		return;
	}

	Uint32 size = static_cast< Uint32 >( rangesArray->GetNumberOfRows() );
	for ( Uint32 i = 0; i < size; ++i )
	{
		String rangeName = rangesArray->GetValue( TXT( "Name" ), i );
		Float range = 0.0f;
		Float interval = 0.0f;
		if ( C2dArray::ConvertValue( rangesArray->GetValue( TXT( "Range" ), i ), range ) && 
			 C2dArray::ConvertValue( rangesArray->GetValue( TXT( "Interval" ), i ), interval ) )
		{
			SetRange( rangeName, range, interval );
		}
	}
}

void CFocusModeController::SetRange( const String& rangeName, Float range, Float interval )
{
	RED_ASSERT( range > 0.0f );
	RED_ASSERT( interval > 0.0f );

	if ( rangeName == TXT( "standard" ) )
	{
		CLUES_UPDATE_RANGE = range;
		CLUES_UPDATE_INTERVAL = interval;
	}
	else if ( rangeName == TXT( "sound" ) )
	{
		SOUND_CLUES_RANGE = range;
		SOUND_CLUES_UPDATE_INTERVAL = interval;
	}
	else if ( rangeName == TXT( "enemies" ) )
	{
		ENEMIES_RANGE = range;
		ENEMIES_UPDATE_INTERVAL = interval;
	}
	else if ( rangeName == TXT( "custom_sound" ) )
	{
		CUSTOM_SOUND_CLUES_RANGE = range;
		CUSTOM_SOUND_CLUES_UPDATE_INTERVAL = interval;
	}
	else
	{
		RED_ASSERT( false, TXT( "Unknown focus clue range name: '%ls'" ), rangeName.AsChar() );
	}
}

void CFocusModeController::Tick( Float timeDelta )
{
	PC_SCOPE_PIX( CFocusModeController );

	if ( GGame->IsBlackscreen() )
	{
		return; 
	}

	if ( m_activationTimer > 0.0f )
	{
		m_activationTimer -= timeDelta;
		if ( m_activationTimer <= 0 )
		{
			OnActivationFinished();
		}
	}
	if ( m_deactivationTimer > 0.0f )
	{
		m_deactivationTimer -= timeDelta;
		if ( m_deactivationTimer <= 0.0f )
		{
			OnDeactivationFinished();
		}
	}

	if ( m_isActive || m_deactivationTimer > 0.0f )
	{
		( new CRenderCommand_FocusModeSetPlayerPosition( GCommonGame->GetPlayer()->GetWorldPositionRef() ) )->Commit();
	}

	m_dynamicClueStorage.Update();
	m_focusCluesDetector.Update();
	m_soundCluesDetector.Update( timeDelta );
	m_customSoundCluesDetector.Update();
	UpdateScentClues();
	ProcessUpdateFocusModeVisibilityEntities();

	CallEvent( CNAME( OnTick ), timeDelta );

	// use atomics for sync with renderer instead
	if( m_dimming )
	{
		m_dimmingFactor -= timeDelta;

		if( m_dimmingFactor < 0.0f )
		{
			m_dimmingFactor = DIMMING_DURATION;
			m_dimming = false;
			CallEvent( CNAME( OnFocusModeDimmingFinished ), timeDelta );
		}
	}
}

void CFocusModeController::OnGenerateDebugFragments( CRenderFrame * frame )
{
#ifndef RED_FINAL_BUILD
	if ( !IsActive() || !frame->GetFrameInfo().IsShowFlagOn( SHOW_FocusMode ) )
	{
		return;
	}

	SFocusPosition focusPosition;
	GetFocusPosition( focusPosition );

	for ( const TClue clue : m_focusCluesDetector )
	{
		TCluePtr entity = clue.Get();
		if ( entity != nullptr )
		{
			Box entityBoundingBox = entity->CalcBoundingBox();
			Vector entityPosition = entity->GetWorldPosition();
			Vector directionToEntity = entityPosition - focusPosition.m_cameraPosition;
			Float radius = entityBoundingBox.CalcExtents().Mag3() * 0.9f;
			Float accuracy = CalculateFocusAccuracy( focusPosition, entity, directionToEntity );
			String text = String::Printf( TXT("clue accuracy: %.2f"), accuracy );
			frame->AddDebugSphere( entityPosition, radius, Matrix::IDENTITY, Color::CYAN );
			frame->AddDebugText( entityPosition, text, 0, 0, true ); 
		}
	}

	for ( const auto& it : m_soundCluesDetector )
	{
		it.m_second.DrawDebug( frame, focusPosition );
	}

#endif
}

void CFocusModeController::SetActive( Bool isActive )
{
	if ( m_isActive == isActive )
	{
		return;
	}
	m_isActive = isActive;
	if ( isActive )
	{
		CEntitiesDetector* detector = GCommonGame->GetSystem< CEntitiesDetector >();
		if ( detector != nullptr )
		{
			detector->Register( &m_gameplayEntitiesDetector, CLUES_UPDATE_INTERVAL, CLUES_UPDATE_RANGE );
			detector->Register( &m_soundCluesDetector, SOUND_CLUES_UPDATE_INTERVAL, SOUND_CLUES_RANGE );
			detector->Register( &m_enemiesDetector, ENEMIES_UPDATE_INTERVAL, ENEMIES_RANGE );
			detector->Register( &m_customSoundCluesDetector, CUSTOM_SOUND_CLUES_UPDATE_INTERVAL, CUSTOM_SOUND_CLUES_RANGE );
		}
		m_activationTimer = ACTIVATION_DURATION - m_deactivationTimer;
		m_deactivationTimer = 0.0f;
	}
	else
	{
		CEntitiesDetector* detector = GCommonGame->GetSystem< CEntitiesDetector >();
		if ( detector != nullptr )
		{
			detector->Unregister( &m_gameplayEntitiesDetector );
			detector->Unregister( &m_soundCluesDetector );
			detector->Unregister( &m_enemiesDetector );
			detector->Unregister( &m_customSoundCluesDetector );
		}
		m_gameplayEntitiesDetector.OnDeactivate();
		m_soundCluesDetector.OnDeactivate();
		m_customSoundCluesDetector.OnDeactivate();
		m_deactivationTimer = ACTIVATION_DURATION - m_activationTimer;
		m_activationTimer = 0.0f;
		m_updateFocusVisibilityEntities.Clear();
	}
}

Float CFocusModeController::GetIntensity() const
{
	if ( m_isActive )
	{
		if ( m_activationTimer > 0.0f )
		{
			return 1.0f - m_activationTimer / ACTIVATION_DURATION;
		}
		return 1.0f;
	}
	else
	{
		if ( m_deactivationTimer > 0.0f )
		{
			return m_deactivationTimer / ACTIVATION_DURATION;
		}
		return 0.0f;
	}
}

void CFocusModeController::OnActivationFinished()
{
	m_activationTimer = 0.0f;
}

void CFocusModeController::OnDeactivationFinished()
{
	m_enemiesDetector.OnDeactivate();
	m_deactivationTimer = 0.0f;
}

void CFocusModeController::EnableVisuals( Bool enable, float desaturation, float highlightBoost, Bool forceDisable )
{
	if ( enable )
	{
		( new CRenderCommand_AddFocusModePostFx( desaturation, highlightBoost ) )->Commit();
	}
	else
	{
		( new CRenderCommand_RemoveFocusModePostFx( forceDisable ) )->Commit();
	}
}

void CFocusModeController::EnableExtendedVisuals( Bool enable, float fadeTime )
{
	if ( enable )
	{
		( new CRenderCommand_EnableExtendedFocusModePostFx( fadeTime ) )->Commit();
	}
	else
	{
		( new CRenderCommand_DisableExtendedFocusModePostFx( fadeTime ) )->Commit();
	}
}
														
void CFocusModeController::SetFadeParameters( Float NearFadeDistance, Float FadeDistanceRange, Float dimmingTime, Float dimmingSpeed )
{													
	m_dimmingTime = dimmingTime;

	( new CRenderCommand_UpdateFocusHighlightFading( NearFadeDistance, FadeDistanceRange, dimmingTime, dimmingSpeed ) )->Commit();
}

void CFocusModeController::SetDimming( Bool dimming )
{													
	m_dimming = dimming;

	( new CRenderCommand_SetDimmingFocusMode( dimming ) )->Commit();
}

Bool CFocusModeController::GetOverridenSoundClueParams( const CGameplayEntity* entity, SSoundClueParams& params ) const
{
	if ( entity == nullptr || !entity->GetIdTag().IsValid() )
	{
		return false;
	}
	return m_overridenSoundClueParams.Find( entity->GetIdTag(), params );
}

Float CFocusModeController::CalculateFocusAccuracy( const SFocusPosition& focusPosition, TCluePtr entity, const Vector& directionToEntity ) const
{
	struct STestLineOfSightEvaluator
	{
		Bool operator()( const CGameplayEntity* entity ) const
		{
			Bool ret = false;
			CProperty* prop = entity->GetClass()->FindProperty( CNAME( testLineOfSight ) );
			if ( prop != nullptr )
			{
				prop->Get( entity, &ret );
			}
			return ret;
		}
	} TestLineOfSightEvaluator;

	RED_ASSERT( entity != nullptr );
	if ( !m_isActive )
	{
		return 0.0f;
	}
	if ( entity->GetInfoCache().Get( entity, GICT_Custom2, TestLineOfSightEvaluator ) )
	{
		m_lostTestIgnoreEntities[ 0 ] = entity;
		if ( !GGame->GetActiveWorld()->TestLineOfSight( focusPosition.m_cameraPosition, entity, true, &m_lostTestIgnoreEntities ) )
		{
			return 0.0f;
		}
	}
	// computing accuracy with dot product between view direction and direction to focus source
	Vector directionToEntity2D( directionToEntity.X, directionToEntity.Y, 0.f );
	directionToEntity2D.Normalize2();
	return Vector::Dot2( directionToEntity2D, focusPosition.m_direction2D );
}

void CFocusModeController::GetFocusPosition( SFocusPosition& focusPosition ) const
{
	focusPosition.m_position = ( GCommonGame->GetPlayer() != nullptr ) ? GCommonGame->GetPlayer()->GetWorldPosition() : Vector::ZEROS;
	focusPosition.m_direction2D = GGame->GetActiveWorld()->GetCameraDirector()->GetCameraForward();
	focusPosition.m_direction2D.Z = 0;
	focusPosition.m_direction2D.Normalize2();
	focusPosition.m_cameraPosition = GGame->GetActiveWorld()->GetCameraDirector()->GetCameraPosition();
}

void CFocusModeController::UpdateScentClues()
{
	const Float intensity = GetIntensity();
	TScentClues::iterator it = m_scentClues.Begin();
	while ( it != m_scentClues.End() )
	{
		// skip processing scent clues that are not on current world
		if ( !it->IsOnThisWorld( m_currentWorldHash ) )
		{
			++it;
			continue;
		}
		if ( it->Update( intensity ) )
		{
			++it;
		}
		else
		{
			m_scentClues.EraseFast( it );
		}
	}
}

void CFocusModeController::ProcessUpdateFocusModeVisibilityEntities()
{
	PC_SCOPE_PIX( FocusMode_UpdateVisibility );

	const Uint32 ENTITIES_TO_UPDATE = 5;
	const Float MOVE_AWAY_DIST_SQUARED = 100.0f;
	Uint32 updated = 0;
	TDynArray< SUpdateFocusVisibilityEntity > notUpdatedEntities;

	// update only small number of closest entities per frame
	while ( !m_updateFocusVisibilityEntities.Empty() && updated < ENTITIES_TO_UPDATE )
	{
		SUpdateFocusVisibilityEntity e = m_updateFocusVisibilityEntities.Top();
		m_updateFocusVisibilityEntities.Pop();
		if ( e.m_entity.IsValid() )
		{
			if ( UpdateFocusModeVisibility( e.m_entity.Get() ) )
			{
				updated++;
			}
			else
			{
				// to move this entity further in the queue
				e.m_dist += MOVE_AWAY_DIST_SQUARED;
				notUpdatedEntities.PushBack( e );
			}
		}
	}

	// enqueue not updated 
	for ( const SUpdateFocusVisibilityEntity& e : notUpdatedEntities )
	{
		m_updateFocusVisibilityEntities.Push( e );
	}
}

void CFocusModeController::RegisterForUpdateFocusModeVisibility( CGameplayEntity* entity )
{
	RED_ASSERT( GCommonGame != nullptr && GCommonGame->GetPlayer() != nullptr );
	m_updateFocusVisibilityEntities.Push( SUpdateFocusVisibilityEntity( entity, ( GCommonGame->GetPlayer()->GetWorldPositionRef() - entity->GetWorldPositionRef() ).SquareMag2() ) );
}

void CFocusModeController::StoreFocusModeVisibility( CGameplayEntity* entity, Bool persistent )
{
	IdTag idTag = entity->GetIdTag();
	if ( !idTag.IsValid() )
	{
		return;
	}
	if ( persistent )
	{
		m_storedFocusModeVisibility[ idTag ] = entity->GetFocusModeVisibility();
	}
	else
	{
		m_storedFocusModeVisibility.Erase( idTag );
	}
}

Bool CFocusModeController::UpdateFocusModeVisibility( CGameplayEntity* entity )
{
	if ( !entity->GetInfoCache().Get( entity, GICT_HasDrawableComponents ) )
	{
		return false;
	}
	EFocusModeVisibility visibility = FMV_None;

	// if tag is empty (entity is not "persistent") or visibility wasn't marked as "stored" we take it directly from the entity
	IdTag idTag = entity->GetIdTag();
	if ( !idTag.IsValid() || !m_storedFocusModeVisibility.Find( idTag, visibility ) )
	{
		visibility = entity->GetFocusModeVisibility();
	}		
	Uint8 orFlags = 0;
	Uint8 andFlags = 0xFF;
	if ( visibility == FMV_Interactive )
	{
		orFlags |= LC_Interactive;
	}
	else
	{
		andFlags &= ~LC_Interactive;
	}
	if ( visibility == FMV_Clue )
	{
		orFlags |= LC_Custom0;
	}
	else
	{
		andFlags &= ~LC_Custom0;
	}
	Bool wasUpdated = false;
	ComponentIterator< CDrawableComponent > it( entity );
	for ( ; it; ++it )
	{
		wasUpdated = true; // something was updated
		CDrawableComponent* dc = *it;
		RED_ASSERT( dc != nullptr );
		if ( !dc->HasTag( CNAME( no_focus_glow ) ) )
		{
			Uint8 lightChannels = ( dc->GetLightChannels() | orFlags ) & andFlags;
			dc->SetLightChannels( lightChannels );
		}
	}
	if ( wasUpdated )
	{
		entity->GetInfoCache().Set( GICT_Custom4, true );
	}
	return wasUpdated;
}

void CFocusModeController::funcSetActive( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, isActive, false );
	FINISH_PARAMETERS;
	SetActive( isActive );
	RETURN_VOID();
}

void CFocusModeController::funcIsActive( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_BOOL( IsActive() );
}

void CFocusModeController::funcGetIntensity( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_FLOAT( GetIntensity() );
}

void CFocusModeController::funcEnableVisuals( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, enable, false );	
	GET_PARAMETER_OPT( Float, desaturation, 0.0f );
	GET_PARAMETER_OPT( Float, highlightBoost, EFFECT_HIGHLIGHT_BOOST );
	FINISH_PARAMETERS;
	EnableVisuals( enable, desaturation, highlightBoost );
	RETURN_VOID();
}

void CFocusModeController::funcEnableExtendedVisuals( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, enable, false );
	GET_PARAMETER( Float, fadeTime, 0.0f );	
	FINISH_PARAMETERS;
	EnableExtendedVisuals( enable, fadeTime );
	RETURN_VOID();
}

void CFocusModeController::funcSetFadeParameters( CScriptStackFrame& stack, void* result )
{	
	GET_PARAMETER( Float, NearFadeDistance, EFFECT_FADE_NEAR );
	GET_PARAMETER( Float, FadeDistanceRange, EFFECT_FADE_FAR );
	GET_PARAMETER( Float, dimmingTime, EFFECT_DIMMING_TIME );	
	GET_PARAMETER( Float, dimmingSpeed, EFFECT_DIMMING_SPEED );	
	FINISH_PARAMETERS;	
	SetFadeParameters( NearFadeDistance, FadeDistanceRange, dimmingTime, dimmingSpeed );
	RETURN_VOID();
}

void CFocusModeController::funcSetDimming( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, enable, false );	
	FINISH_PARAMETERS;
	SetDimming( enable );
	RETURN_VOID();
}

void CFocusModeController::funcSetSoundClueEventNames( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CGameplayEntity >, entity, THandle< CGameplayEntity >() );
	GET_PARAMETER( CName, eventStart, CName::NONE );
	GET_PARAMETER( CName, eventStop, CName::NONE );
	GET_PARAMETER( Int32, soundEffectType, 0 );
	FINISH_PARAMETERS;

	Bool res = false;
	if ( entity != nullptr && entity->GetIdTag().IsValid() )
	{
		SSoundClueParams& params = m_overridenSoundClueParams.GetRef( entity->GetIdTag() );
		params.m_eventStart = eventStart;
		params.m_eventStop = eventStop;
		params.m_effectType = Uint8( Clamp( soundEffectType, 0, 255 ) );
		entity->GetInfoCache().Set( GICT_Custom0, m_isSoundClueEvaluator( entity.Get() ) );		
		// if focus mode is active and clue is within SOUND_RANGE -> update its state
		if ( IsActive() )
		{
			SFocusPosition focusPostion;
			GetFocusPosition( focusPostion );
			Vector diff = ( entity->GetWorldPositionRef() - focusPostion.m_position );
			if ( MAbs( diff.X ) < SOUND_CLUES_RANGE || MAbs( diff.Y ) < SOUND_CLUES_RANGE )
			{
				m_soundCluesDetector.OnSoundEventsChanged( entity );
			}
		}
		res = true;
	}

	RETURN_BOOL( res );
}

void CFocusModeController::funcActivateScentClue( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CEntity >, entity, THandle< CEntity >() );
	GET_PARAMETER( CName, effectName, CName::NONE );
	GET_PARAMETER( Float, duration, 0.0f );
	FINISH_PARAMETERS;

	if ( entity.IsValid() )
	{
		// colorblind mode smell focus mode hack
		if( Config::cvColorblindFocusMode.Get() ) m_scentClues.PushBack( SScentClue( CName( TXT("focus_smell_alt") ), entity.Get(), duration, m_currentWorldHash ) );
		else
			m_scentClues.PushBack( SScentClue( effectName, entity.Get(), duration, m_currentWorldHash ) );
	}

	RETURN_VOID();
}

void CFocusModeController::funcDeactivateScentClue( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CEntity >, entity, THandle< CEntity >() );
	FINISH_PARAMETERS;

	if ( entity.IsValid() )
	{
		for ( auto& scentClue : m_scentClues )
		{
			if ( scentClue == entity.Get() )
			{
				scentClue.Activate( false );
				break;
			}
		}
	}

	RETURN_VOID();
}

//////////////////////////////////////////////////////////////////////////////

CFocusSoundParam::CFocusSoundParam()
	: m_eventStart( CName::NONE )
	, m_eventStop( CName::NONE )
	, m_hearingAngle( 90.0f )
	, m_visualEffectBoneName( CName::NONE )
{
}

void CFocusSoundParam::funcGetEventStart( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_NAME( m_eventStart );
}

void CFocusSoundParam::funcGetEventStop( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_NAME( m_eventStop );
}

void CFocusSoundParam::funcGetHearingAngle( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_FLOAT( m_hearingAngle );
}

void CFocusSoundParam::funcGetVisualEffectBoneName( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_NAME( m_visualEffectBoneName );
}