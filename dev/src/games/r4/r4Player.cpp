/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "r4Player.h"
#include "dynamicTagsContainer.h"
#include "../../common/game/aiStorageExternalPtr.h"
#include "../../common/core/depot.h"
#include "../../common/engine/cutsceneInstance.h"
#include "../../common/engine/dynamicLayer.h"

#if !defined( NO_TELEMETRY )

#	include "r4telemetry.h"
#	include "../../common/core/redTelemetryServiceInterface.h"

#endif // NO_TELEMETRY

#if !defined( NO_SECOND_SCREEN ) 

#include "../../common/platformCommon/secondScreenManager.h"

#endif // NO_SECOND_SCREEN

#if !defined( NO_TELEMETRY ) || !defined( NO_SECOND_SCREEN )

#	define PLAYER_MOVEMENT_REPORTING_RESOLUTION_SQR 4.0
#	define PLAYER_POSITION_SAMPLING_TIME			5.0
#	define PLAYER_ROTATION_REPORTING_RESOLUTION		5.0
#	define PLAYER_ROTATION_SAMPLING_TIME			1.0

#endif

#include "../../common/game/gameplayStorage.h"

#include "r4PlayerController.h"
#include "../../common/engine/inputDeviceManager.h"
#include "../../common/engine/inputDeviceCommandsManager.h"

#include "customCamera.h"
#include "../../common/game/gameWorld.h"
#include "../../common/engine/visualDebug.h"
#include "../../common/core/gatheredResource.h"
#include "../../common/engine/appearanceComponent.h"
#include "../../common/engine/renderFrame.h"
#include "../../common/engine/behaviorGraphUtils.inl"
#include "../../common/engine/behaviorGraphContext.h"
#include "../../common/game/movableRepresentationPathAgent.h"
#include "../../common/engine/pathlibWorld.h"
#include "../../common/game/vehicle.h"
#include "../../common/game/movingAgentComponent.h"
#include "w3Boat.h"
#include "../../common/game/storyScenePlayer.h"

namespace Config
{
	TConfigVar< Int32 >	cvEnableUberMovement( "Gameplay", "EnableUberMovement", 0, eConsoleVarFlag_Save ); 
}

CGatheredResource resIntroCutscene( TXT("animations\\cutscenes\\additional\\cs_leaving_horse\\cs_leaving_horse.w2cutscene"), RGF_Startup );

IMPLEMENT_ENGINE_CLASS( CR4Player );

RED_DISABLE_WARNING_MSC( 4355 )

CR4Player::CR4Player()
	: CPlayer()
	, m_actionPlayerControl( this )
	, m_horseWithInventory()
	, m_isInInterior( 0 )
	, m_isInSettlement( 0 )
	, m_isInCombat( false )
#if !defined( NO_TELEMETRY ) || !defined( NO_SECOND_SCREEN )
	, m_lastReportedTime( 0 )
	, m_lastReportedPosition( Vector::ZEROS )
	, m_lastReportedRotation( 0 )
	, m_lastReportedRotationTime( 0 )
#endif // NO_TELEMETRY
{
}

CR4Player::~CR4Player()
{
	ClearTickets();
}

void CR4Player::ClearTickets()
{
	for ( auto it = m_tickets.Begin(), end = m_tickets.End(); it != end; ++it )
	{
		delete it->m_second;
	}
	m_tickets.ClearFast();
}

Bool CR4Player::Teleport( const Vector& position, const EulerAngles& rotation )
{
	Bool ret = TBaseClass::Teleport( position, rotation );

	CallEvent( CNAME( OnTeleported) );

	if ( CWorld* world = GR4Game ? GR4Game->GetActiveWorld() : nullptr )
	{
		if ( GR4Game->GetCamera() && GR4Game->GetCamera() == world->GetCameraDirector()->GetTopmostCameraObject().Get() )
		{
			GR4Game->GetCamera()->ResetCamera();
		}
	}

	return ret;
}

void CR4Player::GenerateDebugFragments( CRenderFrame* frame )
{
	TBaseClass::GenerateDebugFragments( frame );

	CVisualDebug* vd = GetVisualDebug();
	if ( vd )
	{
		static Bool toggle = true;
		static Uint64 lastDraw = 0;

		if ( lastDraw != hack_curr.m_tickNum )
		{
			const Float radius = toggle ? 0.05f : 0.08f;
			const Color color = toggle ? Color( 255, 255, 0 ) : Color( 255, 0, 0 );

			for ( Uint32 i=0; i<hack_curr.m_pointsWS.Size(); ++i )
			{
				const String str = String::Printf( TXT("hack_test_%d"), (Uint32)hack_curr.m_tickNum );
				const CName sname( str );
				const Vector point = hack_curr.m_pointsWS[ i ];

				vd->AddSphere( sname, radius, point, true, color, 1000.f );
			}

			lastDraw = hack_curr.m_tickNum;
			toggle = !toggle;
		}
	}
}

void CR4Player::Hack_SetSwordTrajectory( TDynArray< Vector >& dataWS, Float w )
{
	if ( hack_curr.m_pointsWS.Size() == 0 )
	{
		ASSERT( dataWS.Size() == 4 );
	}
	else
	{
		ASSERT( dataWS.Size() >= 2 );
	}

	hack_prev = hack_curr;

	hack_curr.m_pointsWS = dataWS;
	hack_curr.m_frameWeight = w;
	hack_curr.m_tickNum = GEngine->GetCurrentEngineTick();
}

void CR4Player::OnTick( Float timeDelta )
{
	PC_SCOPE( CR4Player );

	TBaseClass::OnTick( timeDelta );

	if( IsInGame() )
	{
		m_enemyData.Update( *this );

#if !defined( NO_TELEMETRY ) || !defined( NO_SECOND_SCREEN )
		
		const Vector& position = GetWorldPosition();
		const Float rotation = GetWorldYaw();		
		Double now = m_positionSamplingTimer.GetSeconds();
		if( ShouldReportMovement( position ) )
		{
			ReportMovement( position, rotation, now );
		}
		
		if ( ShouldReportRotation( rotation, now ) )
		{
			ReportRotation( position, rotation, now );
		}

		if( ShouldReportPosition( now ) )
		{
			ReportPosition( now );
		}
#endif
	}
}

#if !defined( NO_TELEMETRY ) || !defined( NO_SECOND_SCREEN )

Bool CR4Player::ShouldReportMovement( const Vector& position ) const
{
	return m_lastReportedPosition.DistanceSquaredTo2D( position ) > PLAYER_MOVEMENT_REPORTING_RESOLUTION_SQR;
}

Bool CR4Player::ShouldReportRotation( const Float rotation, const Double currentTime ) const
{
	return fabsf( m_lastReportedRotation - rotation ) > PLAYER_ROTATION_REPORTING_RESOLUTION && 
		   currentTime - m_lastReportedRotationTime > PLAYER_ROTATION_SAMPLING_TIME;
}

Bool CR4Player::ShouldReportPosition( const Double currentTime ) const
{
	return currentTime - m_lastReportedTime > PLAYER_POSITION_SAMPLING_TIME;
}

void CR4Player::ReportMovement( const Vector& position, const Float rotation, const Double currentTime )
{
	m_lastReportedPosition = position;
	m_lastReportedRotation = rotation;
	m_lastReportedRotationTime = currentTime;
#if !defined( NO_TELEMETRY )
	CR4Telemetry *telemetrySystem = GCommonGame->GetSystem< CR4Telemetry >();
	if ( telemetrySystem )
	{
		telemetrySystem->Log( TE_HERO_MOVEMENT );
	}
#endif // NO_TELEMETRY
#if !defined( NO_SECOND_SCREEN )
	SCSecondScreenManager::GetInstance().SendPlayerPosition( position.X, position.Y, position.Z, rotation );
#endif
}

void CR4Player::ReportRotation( const Vector& position, const Float rotation, const Double currentTime )
{
	m_lastReportedPosition = position;
	m_lastReportedRotation = rotation;
	m_lastReportedRotationTime = currentTime;
#if !defined( NO_SECOND_SCREEN )
	SCSecondScreenManager::GetInstance().SendPlayerPosition( position.X, position.Y, position.Z, rotation );
#endif
}

void CR4Player::ReportPosition( const Double currentTime )
{
	m_lastReportedTime = currentTime;
#if !defined( NO_TELEMETRY )
	CR4Telemetry *telemetrySystem = GCommonGame->GetSystem< CR4Telemetry >();
	if ( telemetrySystem )
	{
		telemetrySystem->Log( TE_HERO_POSITION );
	}
#endif // NO_TELEMETRY
}

#endif

void CR4Player::OnDetached( CWorld* world )
{
	if( GEngine && GEngine->GetInputDeviceManager() )
	{
		GEngine->GetInputDeviceManager()->Commands().ResetBacklight();
		GEngine->GetInputDeviceManager()->Commands().SetPadVibrate( 0.f, 0.f );
	}

	SetPlayerCombatTarget( NULL );

	ClearTickets();

	TBaseClass::OnDetached( world );
}

void CR4Player::SetPlayerCombatTarget( CActor* newTarget )
{
	CActor* currentTarget = m_combatTarget.Get();
	if ( currentTarget != newTarget )
	{
		CCombatDataComponent* currentTargetData = m_combatTargetData;
		if ( currentTargetData )
		{
			currentTargetData->UnregisterAttacker( this );
		}

		FreeTicketAtCombatTarget();

		m_combatTarget = newTarget;

		if ( newTarget )
		{
			CCombatDataComponent* newTargetData = (m_combatTargetData = newTarget);
			if ( newTargetData )
			{
				newTargetData->RegisterAttacker( newTarget );
			}
			m_combatTargetData->RegisterAttacker( this );
		}
	}
}

Bool CR4Player::ObtainTicketFromCombatTarget( CName ticketName, Uint32 ticketCount )
{
	CCombatDataComponent* combatTargetData = m_combatTargetData;
	if ( !combatTargetData )
	{
		return false;
	}
	CTicketSource* ticketSource = combatTargetData->GetTicketSource( ticketName );
	if ( !ticketSource )
	{
		return false;
	}
	CTicket* ticket = nullptr;
	auto itFind = m_tickets.Find( ticketName );
	if ( itFind == m_tickets.End() )
	{
		ticket = new CTicket( this );
		m_tickets.Insert( ticketName, ticket );
	}
	else
	{
		ticket = itFind->m_second;
	}
	ticket->Aquire( ticketSource, Int16( ticketCount ), 9999.f );
	ticket->Lock( true );
	return true;
}

void CR4Player::FreeTicketAtCombatTarget()
{
	for ( auto it = m_tickets.Begin(), end = m_tickets.End(); it != end; ++it )
	{
		it->m_second->Free();
	}
}

void CR4Player::SetIsInCombat( Bool b )
{
	if ( m_isInCombat != b )
	{
		m_isInCombat = b;
		OnCombatModeSet( b );
	}
}
Bool CR4Player::IsInCombat() const
{
	return m_isInCombat;
}

Bool CR4Player::ActionDirectControl( const THandle< CR4LocomotionDirectController >& playerController )
{
	// Check if current action may be canceled by other actions
	if ( m_action && ! m_action->IsCancelingAllowed() )
	{
		return false;
	}

	// Cancel actions
	ActionCancelAll();

	// Start move to action
	if ( !m_actionPlayerControl.Start( playerController ) )
	{
		return false;
	}

	// Start action
	m_action = &m_actionPlayerControl;
	m_latentActionResult = ActorActionResult_InProgress;
	m_latentActionType   = ActorAction_R4Reserved_PC;

	OnActionStarted( ActorAction_R4Reserved_PC );

	return true;
}

CActor* CR4Player::GetScriptTarget() const
{
	return m_scriptTarget.Get();
}
void CR4Player::SetScriptTarget( CActor* actor )
{
	m_scriptTarget = actor;
}

RED_DEFINE_STATIC_NAME( IsInCombatActionCameraRotationEnabled )
Bool CR4Player::IsInCombatActionCameraRotationEnabled()
{
	Bool result = false;
	CallFunctionRet( this, CNAME( IsInCombatActionCameraRotationEnabled ), result );

	return result;
}

RED_DEFINE_STATIC_NAME( IsGuarded )
Bool CR4Player::IsGuarded()
{
	Bool result = false;
	CallFunctionRet( this, CNAME( IsGuarded ), result );

	return result;
}

RED_DEFINE_STATIC_NAME( IsLockedToTarget )
Bool CR4Player::IsLockedToTarget()
{
	Bool result = false;
	CallFunctionRet( this, CNAME( IsLockedToTarget ), result );

	return result;
}

void CR4Player::GetVisibleEnemies( TDynArray< CActor* > & enemies ) const
{
	m_enemyData.GetVisibleEnemies( enemies );
}

void CR4Player::GetVisibleEnemies( TDynArray< THandle< CActor > > & enemies ) const
{
	m_enemyData.GetVisibleEnemies( enemies );
}

CAIStorageRiderData *const CR4Player::GetRiderData()
{
	return CAIStorageRiderData::Get( this );
}

Bool CR4Player::ShouldPlayPreDialogCutscene( THandle< CActor >& vehicle )
{
	// It must be Geralt (because the cutscene is made for Geralt specifically)

	if ( !HasTag( CName( TXT( "GERALT" ) ) ) )
	{
		return false;
	}

	// There must be mounted vehicle

	if ( CAIStorageRiderData* riderData = GetRiderData() )
	{
		if ( CHorseRiderSharedParams* riderParams = riderData->m_sharedParams.Get() )
		{
			if ( riderParams->m_mountStatus == VMS_mounted && riderParams->m_horse.IsValid() )
			{
				// Make sure this is horse (not a boat or something else)

				CVehicleComponent* vehicleComponent = riderParams->m_horse->FindComponent< CVehicleComponent >();
				if( !vehicleComponent || !vehicleComponent->IsHorse() )
				{
					return false;
				}

				vehicle = riderParams->m_horse;
				return true;
			}
		}
	}
	return false;
}

CCutsceneInstance* CR4Player::StartPreDialogCutscene( THandle< CActor >& vehicle )
{
	// Hide the horse not to interfere with the cutscene horse

	//if ( CActor* horse = vehicle.Get() )
	//{
		//GetTransformParent()->Break();
		//horse->SetHideInGame( true, false, CEntity::HR_Scene );
	//}

	// Load the cutscene resource

	CCutsceneTemplate* csTempl = resIntroCutscene.LoadAndGet< CCutsceneTemplate >();
	if ( !csTempl )
	{
		SCENE_WARN( TXT("StartPreDialogCutscene: Couldn't find resource for cutscene. Fail.") );
		return nullptr;
	}

	// Get layer to attach cutscene to

	CLayer* layer = GCommonGame->GetActiveWorld()->GetDynamicLayer();
	ASSERT( layer );

	// Determine good location to play the cutscene

	Matrix transform;
	GetLocalToWorld( transform );

	CMovingAgentComponent* actorMovingAgent = GetMovingAgentComponent();
	CPathAgent* pathAgent = actorMovingAgent->GetPathAgent();
	CPathLibWorld* pathlib = pathAgent->GetPathLib();

	const PathLib::AreaId areaId = pathAgent->GetCachedAreaId();
	const Float searchRadius = 6.0f;
	Float minZ = transform.GetTranslationRef().Z - 3.0f;
	Float maxZ = transform.GetTranslationRef().Z + 3.f;

	const Float personalSpace = 2.5f; // Larger to account for horse and the in-cutscene translation

	Vector3 safePosition;
	if ( pathlib->FindSafeSpot( areaId, transform.GetTranslationRef().AsVector3(), searchRadius, personalSpace, safePosition, &minZ, &maxZ, PathLib::CT_IGNORE_METAOBSTACLE | pathAgent->GetCollisionFlags() ) )
	{
		pathAgent->DoZCorrection( safePosition );
		transform.SetTranslation( Vector( safePosition.X, safePosition.Y, safePosition.Z, 1.0f ) );
	}

	// Create cutscene instance
	THashMap< String, CEntity* > actors;
	actors.Insert( TXT("geralt"), this );
	if ( CActor* horse = vehicle.Get() )
	{
		actors.Insert( TXT("horse"), horse );
	}

	String errors;

	if( GetTransformParent() )
	{
		BreakAttachment();
	}

	CCutsceneInstance* cs = csTempl->CreateInstance( layer, transform, errors, actors, true );
	if ( !cs )
	{
		SCENE_WARN( TXT("StartPreDialogCutscene: Couldn't create cutscene, reason: %s"), errors.AsChar() );
		return nullptr;
	}

	// Start cutscene

	cs->Play( true, false, false );

	return cs;
}

void CR4Player::OnPostPreDialogCutscene( THandle< CActor >& vehicle, CStoryScenePlayer* scene )
{
	if ( CActor* horse = vehicle.Get() )
	{
		//GetTransformParent()->Break();		
		if( !horse->IsInNonGameplayScene() )
		{
			scene->HideNonSceneActor( horse );
		}		
	}
}



Bool CR4Player::HandleWorldChangeOnBoat()
{
	CHardAttachment* parentAttachment = GetTransformParent();
	if( !parentAttachment )
	{
		return false;
	}

	CNode* parentNode = parentAttachment->GetParent();
	if( parentNode )
	{
		W3Boat* boat = Cast< W3Boat >( parentNode );
		if( boat )
		{
			boat->ForgetTheState();
			return true; // player on boat
		}
	}

	return false;
}


//////////////////////////////////////////////////////////////////////////

void CR4Player::funcIsInInterior( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_BOOL( IsInInterior() );
}

void CR4Player::funcIsInSettlement( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_BOOL( IsInSettlement() );
}

void CR4Player::funcEnterSettlement( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, enter, true );
	FINISH_PARAMETERS;
	SetInSettlement( enter ? 1 : -1 );

}

void CR4Player::funcGetEnemiesInRange( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( TDynArray< THandle< CActor > >, enemies, TDynArray< THandle< CActor > >() );
	FINISH_PARAMETERS;

	m_enemyData.GetAllEnemiesInRange( enemies );
}

void CR4Player::funcGetVisibleEnemies( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( TDynArray< THandle< CActor > >, enemies, TDynArray< THandle< CActor > >() );
	FINISH_PARAMETERS;

	GetVisibleEnemies( enemies );
}

void CR4Player::funcIsEnemyVisible( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CActor >, enemy, THandle< CActor >() );
	FINISH_PARAMETERS;

	RETURN_BOOL( IsEnemyVisible( enemy ) );
}

void CR4Player::funcSetupEnemiesCollection( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, range, 10.f );
	GET_PARAMETER( Float, heightTolerance, 5.f );
	GET_PARAMETER( Int32, maxEnemies, 16 );
	GET_PARAMETER_OPT( CName, tag, CName::NONE );
	GET_PARAMETER_OPT( Uint32, flags, 0 );
	FINISH_PARAMETERS;

	// make sure we exclude player
	m_enemyData.Setup( range, heightTolerance, (Uint32)maxEnemies, tag, flags | FLAG_ExcludePlayer );
}

void CR4Player::funcActionDirectControl( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CR4LocomotionDirectController >, handler, NULL );
	FINISH_PARAMETERS;
	Bool ret = false;
	if ( handler.IsValid() )
	{
		ret = ActionDirectControl( handler );
	}
	RETURN_BOOL( ret );
}

void CR4Player::funcGetCombatDataComponent( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	CCombatDataComponent* combatData = m_playerCombatData;
	if ( !combatData )
	{
		combatData = (m_playerCombatData = this);
	}
	THandle< CCombatDataComponent > handle( combatData );
	RETURN_HANDLE( CCombatDataComponent, handle );
}

void CR4Player::funcSaveLastMountedHorse( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CActor >, target, THandle< CActor >() );
	FINISH_PARAMETERS;

	if ( const CDynamicTagsContainer* const dynamicTagsContainer = GR4Game->GetDynamicTagsContainer() )
	{
		dynamicTagsContainer->UpdateDynamicTags( target, PLAYER_LAST_MOUNTED_VEHICLE_TAG );
	}	
}

void CR4Player::funcSetPlayerTarget( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CActor >, target, THandle< CActor >() );
	FINISH_PARAMETERS;
	m_scriptTarget = target;
}
void CR4Player::funcSetPlayerCombatTarget( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CActor >, target, THandle< CActor >() );
	FINISH_PARAMETERS;
	SetPlayerCombatTarget( target.Get() );
}

void CR4Player::funcObtainTicketFromCombatTarget( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, ticketName, CName::NONE );
	GET_PARAMETER( Uint32, ticketCount, 100 );
	FINISH_PARAMETERS;
	Bool ret = ObtainTicketFromCombatTarget( ticketName, ticketCount );
	RETURN_BOOL( ret );
}

void CR4Player::funcFreeTicketAtCombatTarget( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	FreeTicketAtCombatTarget();
}

void CR4Player::funcSetScriptMoveTarget( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CActor >, target, THandle< CActor >() );
	FINISH_PARAMETERS;
	m_moveTarget = target;
}

void CR4Player::funcGetRiderData( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_HANDLE( CAIStorageRiderData, GetRiderData() );
}
void CR4Player::funcSetIsInCombat( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, inCombat, false );
	FINISH_PARAMETERS;

	SetIsInCombat( inCombat );
}

namespace
{
	Uint8 ChannelLerp( Float frac, Uint8 from, Uint8 to )
	{
		Float tmp = Lerp( frac, ( Float )from, ( Float )to );
		tmp = Clamp( tmp, 0.0f, 255.0f );
		return static_cast< Uint8 >( tmp );
	}

	Color ColorLerp( Float frac, const Color& from, const Color& to )
	{
		Color toRet;
		toRet.R = ChannelLerp( frac, from.R, to.R );
		toRet.G = ChannelLerp( frac, from.G, to.G );
		toRet.B = ChannelLerp( frac, from.B, to.B );
		toRet.A = 255;
		return toRet;
	}
};

void CR4Player::funcSetBacklightFromHealth( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, healthPercentage, 1.0f );
	FINISH_PARAMETERS;

	IInputDeviceManager* inputDeviceManager = GEngine->GetInputDeviceManager();
	if ( !inputDeviceManager )
	{
		return;
	}

	healthPercentage = Clamp( healthPercentage, 0.0f, 1.0f );

	RED_ASSERT( GEngine && GEngine->GetInputDeviceManager() );

	if( healthPercentage >= 0.5f )
	{
		float lightCoef = ( healthPercentage - 0.5f ) * 2.0f;
		Color lightColor = ColorLerp( lightCoef, Color::YELLOW, Color::GREEN );
		inputDeviceManager->Commands().SetBacklight( lightColor );
	}
	else
	{
		float lightCoef = healthPercentage * 2.0f;
		Color lightColor = ColorLerp( lightCoef, Color::RED, Color::YELLOW );
		inputDeviceManager->Commands().SetBacklight( lightColor );
	}
}

void CR4Player::funcSetBacklightColor( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, c, Vector::ZEROS );
	FINISH_PARAMETERS;
	RED_ASSERT( GEngine && GEngine->GetInputDeviceManager() );

	IInputDeviceManager* inputDeviceManager = GEngine->GetInputDeviceManager();
	if ( !inputDeviceManager )
	{
		return;
	}

	c.W = 1.0f;
	inputDeviceManager->Commands().SetBacklight( Color( c ) );
}

void CR4Player::funcGetTemplatePathAndAppearance( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( String, templatePath, String::EMPTY );
	GET_PARAMETER_REF( CName, appearanceName, CName::NONE );
	FINISH_PARAMETERS;

	if ( m_template )
	{
		templatePath = m_template->GetDepotPath();
	}

	for ( ComponentIterator< CAppearanceComponent > it( this ); it; ++it )
	{
		CAppearanceComponent* component = Cast< CAppearanceComponent >( *it );
		if ( component )
		{
			appearanceName = component->GetAppearance();
			return;
		}
	}
}

void CR4Player::funcHACK_BoatDismountPositionCorrection( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, slotPos, Vector::ZEROS );
	FINISH_PARAMETERS;

	Float targetPosZ = slotPos.Z;
	Vector pos = GetWorldPosition();
	pos.Z = targetPosZ;
	CNode::SetPosition( pos );
}

void CR4Player::funcHACK_ForceGetBonePosition( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Uint32, boneIndex, 0 );
	FINISH_PARAMETERS;

	Vector bonePosWS( Vector::ZERO_3D_POINT );
	HACK_ForceGetBonePosWS( boneIndex, bonePosWS );

	RETURN_STRUCT( Vector, bonePosWS );
}
