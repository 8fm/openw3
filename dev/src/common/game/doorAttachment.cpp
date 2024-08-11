/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "doorAttachment.h"
#include "doorComponent.h"
#include "../engine/visualDebug.h"
#include "../engine/externalProxy.h"
#include "../engine/rigidMeshComponent.h"
#include "../engine/behaviorIncludes.h"
#include "../engine/renderCommands.h"
#include "../engine/umbraScene.h"

IMPLEMENT_ENGINE_CLASS( SDoorSoundsEvents );
IMPLEMENT_ENGINE_CLASS( IDoorAttachment );
IMPLEMENT_ENGINE_CLASS( CDoorAttachment_AngleAnimation );
IMPLEMENT_ENGINE_CLASS( CDoorAttachment_PropertyAnimation );
IMPLEMENT_ENGINE_CLASS( CDoorAttachment_GameplayPush );

RED_DEFINE_STATIC_NAME( Open )
RED_DEFINE_STATIC_NAME( OnOpenningDoor )
RED_DEFINE_STATIC_NAME( IsUsingHorse )

//////////////////////////////////////////////////////////////////////////////////////////////
Bool IDoorAttachment::Init( CNode* parent, CNode* child, const AttachmentSpawnInfo* info )
{
	// is the stuff good?
	Bool parentIsADoor = parent->IsA<CDoorComponent>() || parent->IsA<CExternalProxyComponent>();
	Bool childIsAMesh = child->IsA<CMeshTypeComponent>() || child->IsA<CExternalProxyComponent>();
	
	Bool success = parentIsADoor && childIsAMesh && TBaseClass::Init( parent, child, info );

	if( !success )
		return false;

	UpdateIsTrapdoor();	
	
	if( child != nullptr )
	{
		// calculate original yaw [-180, 180]
		if( m_isTrapdoor )
		{
			m_originalAngle = EulerAngles::NormalizeAngle( child->GetRotation().Pitch );
		}
		else
		{
			m_originalAngle = EulerAngles::NormalizeAngle( child->GetRotation().Yaw );
		}		

		if( m_originalAngle > 180.0f )
		{
			m_originalAngle -= 360.0f;
		}		
	}

	CDoorComponent* door = Cast< CDoorComponent >( parent );
	if( door &&	GGame->IsActive() )
	{
		EDoorState targetState = door->GetDesiredState();
		if( targetState == Door_Open )
		{
			door->Open( true );
		}
		else
		{
			door->Close( true );
		}
	}

#ifdef USE_UMBRA
	// cache umbraGateId
	if ( CMeshComponent* meshComponent = Cast< CMeshComponent >( child ) )
	{
		Uint32 modelId = GetHash( meshComponent->GetMeshResourcePath() );
		Uint32 transformHash = UmbraHelpers::CalculateTransformHash( meshComponent->GetLocalToWorld() );
		TObjectCacheKeyType objectKey = UmbraHelpers::CompressToKeyType( modelId, transformHash );
		if ( meshComponent->GetWorld() && meshComponent->GetWorld()->GetUmbraScene() )
		{
			meshComponent->GetWorld()->GetUmbraScene()->GetObjectCache().Find( objectKey, m_umbraGateId );
		}
	}
#endif // USE_UMBRA
	
	return true;
}

void IDoorAttachment::OnAttached() 
{ 
	UpdateIsTrapdoor();
	UpdateDoorState();
}

void IDoorAttachment::UpdateIsTrapdoor()
{
	CDoorComponent* door = GetDoor(); 
	m_isTrapdoor = door && door->IsTrapdoor();
}

//////////////////////////////////////////////////////////////////////////////////////////////
CDoorAttachment_AngleAnimation::CDoorAttachment_AngleAnimation()
	: m_openAngle( 90.0f )
	, m_openTime( 1.5f )
	, m_openPercentage( 0.0f )	
	, m_stateForced( false )
{
}

void CDoorAttachment_AngleAnimation::OnAttached()
{
	CDoorComponent* door = GetDoor();
	if( door )
	{
		// set initial door position
		m_openPercentage = door->IsOpen() ? 1.0f : 0.0f;
		Update( 0.0f );
	}
}

Bool CDoorAttachment_AngleAnimation::Update( Float timeDelta )
{
	// attachment broken?
	if( m_isBroken )
	{
		return false;
	}

	// got door & child?
	CDoorComponent* door = GetDoor();
	CComponent* child = Cast<CComponent>( GetChild() );
	if( !door || !child )
	{
		return false;
	}

	// open / close door?
	EDoorState sourceState = door->GetCurrentState();
	EDoorState targetState = door->GetDesiredState();

	if( targetState == Door_Closed && ( !m_stateForced && !CanBeClosed( door )  ) )
	{//we have to waint with closing
		return true;
	}

	m_openPercentage += ( timeDelta / m_openTime ) * ( targetState == Door_Open ? 1.0f : -1.0f );

	Bool isDone = ( targetState == Door_Open ? m_openPercentage >= 1.0f :
											   m_openPercentage <= 0.0f );

	// cap percentage
	m_openPercentage = Clamp( m_openPercentage, 0.0f, 1.0f );

	Float weight = 0;

	if( m_openPercentage <= 0.5f )
	{
		weight = 2 * Red::Math::MSqr( m_openPercentage ); // 2* (x^2)
	}
	else
	{		
		weight = -2  * Red::Math::MSqr( m_openPercentage - 1 ) + 1; // -2*(( x - 1 )^2) + 1
	}
	// update door angle
	EulerAngles newAngle = child->GetRotation();
	if( m_isTrapdoor )
	{
		newAngle.Pitch = m_originalAngle + weight * m_openAngle;
		newAngle.Normalize();
	}	
	else
	{
		newAngle.Yaw = m_originalAngle + weight * m_openAngle;
		newAngle.Normalize();
	}

	if ( !child->GetRotation().AlmostEquals( newAngle ) )
	{
		child->SetRotation( newAngle );
		child->ForceUpdateTransformNodeAndCommitChanges();
	}

	if( !isDone )
	{
		if( targetState == Door_Open )
		{
			PlayStartOpeningSounds( );
		}
		else
		{
			PlayStartClosingSounds( );
		}		
	}
	else
	{
		if( targetState == Door_Open )
		{
			PlayStopOpeningSounds( );
		}
		else
		{
			PlayStopClosingSounds( );
		}
		m_stateForced = false;
	}

	UpdateDoorState();

	// need more ticks?
	return !isDone;
}

Bool CDoorAttachment_AngleAnimation::CanBeClosed( CDoorComponent* door )
{	
	const TDynArray< THandle< CActor > >& doorUsers = door->GetDoorUsers();

	CRigidMeshComponent* rigidMesh = Cast<CRigidMeshComponent>( GetChild() );
	if( !rigidMesh )
		return true;

	Vector doorCenterPos = rigidMesh->GetBoundingBox().CalcCenter();
	const Vector& doorCornerPos = door->GetWorldPositionRef();

	Vector doorFrameDirection = door->GetWorldRight();
	Float doorSize = ( doorCenterPos - doorCornerPos ).Mag2();
	Float doorSizeSqrt = 4 * doorSize * doorSize;

	Vector doorFrameCenter = doorCornerPos + doorFrameDirection * doorSize;

	for( Int32 i=0; i<doorUsers.SizeInt(); ++i )
	{
		CNewNPC* npc = Cast< CNewNPC >( doorUsers[ i ] );
		if( npc )
		{
			Vector distance = npc->GetWorldPositionRef() - doorFrameCenter;
			distance.Z = 0;

			if( distance.SquareMag2() < doorSizeSqrt )
			{
				return false;
			}
		}
	}

	return true;
}

void CDoorAttachment_AngleAnimation::InstantClose()
{
	CDoorComponent* door = GetDoor();
	CComponent* child = Cast<CComponent>( GetChild() );

	InstantClose( door, child );
}

void CDoorAttachment_AngleAnimation::InstantOpen()
{
	CDoorComponent* door = GetDoor();
	CComponent* child = Cast<CComponent>( GetChild() );	

	InstantOpen( door, child );

}

void CDoorAttachment_AngleAnimation::InstantClose( CDoorComponent* door, CComponent* child )
{
	if( !door || !child )
	{
		return;
	}

	EulerAngles newAngle = child->GetRotation();
	if( m_isTrapdoor )
	{
		newAngle.Pitch = m_originalAngle;
		newAngle.Normalize();
	}
	else
	{
		newAngle.Yaw = m_originalAngle;
		newAngle.Normalize();
	}	
	if ( !child->GetRotation().AlmostEquals( newAngle ) )
	{
		child->SetRotation( newAngle );
		child->ForceUpdateTransformNodeAndCommitChanges();
	}

	m_openPercentage = 0;
	UpdateDoorState();
	PlayStopClosingSounds();
}

void CDoorAttachment_AngleAnimation::InstantOpen( CDoorComponent* door, CComponent* child )
{
	if( !door || !child )
	{
		return;
	}

	EulerAngles newAngle = child->GetRotation();
	if( m_isTrapdoor )
	{
		newAngle.Pitch = m_originalAngle + m_openAngle;
		newAngle.Normalize();
	}
	else
	{
		newAngle.Yaw = m_originalAngle + m_openAngle;
		newAngle.Normalize();
	}
	if ( !child->GetRotation().AlmostEquals( newAngle ) )
	{
		child->SetRotation( newAngle );
		child->ForceUpdateTransformNodeAndCommitChanges();
	}

	m_openPercentage = 1;
	UpdateDoorState();
	PlayStopOpeningSounds();
}

Bool CDoorAttachment_AngleAnimation::IsClosed() const
{
	return MAbs( m_openPercentage ) <= 0.05f;
}

Bool CDoorAttachment_AngleAnimation::IsOpened() const
{
	return !IsClosed();
}

//////////////////////////////////////////////////////////////////////////////////////////////
CDoorAttachment_PropertyAnimation::CDoorAttachment_PropertyAnimation()
	: m_animDuration( 0.0f )
	, m_openPercentage( 0.0f )
{
}

Bool CDoorAttachment_PropertyAnimation::Init( CNode* parent, CNode* child, const AttachmentSpawnInfo* info )
{
	if( !TBaseClass::Init( parent, child, info ) )
	{
		return false;
	}

	if( !parent || !parent->AsComponent() )
	{
		return false;
	}

	// make sure owner is CGameplayEntity	
	if( GetGameplayEntity() == nullptr )
	{
		//GFeedback->ShowWarn( TXT("Parent entity must be a CGameplayEntity") );
		return false;
	}

	// find 'Open' animation
	if( GetAnimationByName( CNAME( Open ) ) == nullptr )
	{
		//GFeedback->ShowWarn( TXT("Parent gameplay entity must have a property animation called 'Open'") );
		return false;
	}

	return true;
}

void CDoorAttachment_PropertyAnimation::OnAttached() 
{
	CDoorComponent* door = GetDoor();
	if( door )
	{
		// set initial door position
		m_openPercentage = door->IsOpen() ? 1.0f : 0.0f;
		Update( 0.0f );
	}

	// figure out max animation duration
	m_animDuration = 0.0f;
	CGameplayEntity* owner = GetGameplayEntity();
	CPropertyAnimationSet* animSet = ( owner != nullptr ? owner->GetPropertyAnimationSet() : nullptr );
	if( animSet )
	{
		for( Uint32 i=0; i<animSet->GetPropertyAnimationsCount(); ++i )
		{
			SPropertyAnimation* anim = animSet->GetPropertyAnimationByIndex( i );
			if( anim->m_animationName == CNAME( Open ) )
			{
				m_animDuration = Max( m_animDuration, anim->m_curve.GetTotalTime() );
			}
		}
	}
}

Bool CDoorAttachment_PropertyAnimation::Update( Float timeDelta )
{
	// attachment broken? and got animation?
	if( m_isBroken || m_animDuration < 0.001f )
	{
		return false;
	}

	// got door & child?
	CDoorComponent* door = GetDoor();
	CComponent* child = Cast<CComponent>( GetChild() );
	if( !door || !child )
	{
		return false;
	}

	// open / close door?
	EDoorState sourceState = door->GetCurrentState();
	EDoorState targetState = door->GetDesiredState();
	m_openPercentage += ( timeDelta / m_animDuration ) * ( targetState == Door_Open ? 1.0f : -1.0f );

	Bool isDone = ( targetState == Door_Open ? m_openPercentage >= 1.0f :
											   m_openPercentage <= 0.0f );

	UpdateDoorState();
	
	// set gameplay entity animtime
	GetGameplayEntity()->RewindPropertyAnimation( CNAME( Open ), m_openPercentage * m_animDuration );

	// need more ticks?
	return !isDone;
}

void CDoorAttachment_PropertyAnimation::InstantClose()
{
	CDoorComponent* door = GetDoor();
	CComponent* child = Cast<CComponent>( GetChild() );
	
	InstantClose( door, child );
}

void CDoorAttachment_PropertyAnimation::InstantOpen()
{
	CDoorComponent* door = GetDoor();
	CComponent* child = Cast<CComponent>( GetChild() );	

	InstantOpen( door, child );
}

void CDoorAttachment_PropertyAnimation::InstantClose( CDoorComponent* door, CComponent* child )
{
	if( !door || !child )
	{
		return;
	}

	m_openPercentage = 0;
	GetGameplayEntity()->RewindPropertyAnimation( CNAME( Open ), 0 );
}

void CDoorAttachment_PropertyAnimation::InstantOpen( CDoorComponent* door, CComponent* child )
{
	if( !door || !child )
	{
		return;
	}
	m_openPercentage = 1;
	GetGameplayEntity()->RewindPropertyAnimation( CNAME( Open ), m_animDuration );
}

SPropertyAnimation* CDoorAttachment_PropertyAnimation::GetAnimationByName( const CName& name ) const
{
	CGameplayEntity* owner = GetGameplayEntity();
	CPropertyAnimationSet* animSet = ( owner != nullptr ? owner->GetPropertyAnimationSet() : nullptr );
	if( animSet )
	{
		for( Uint32 i=0; i<animSet->GetPropertyAnimationsCount(); ++i )
		{
			SPropertyAnimation* anim = animSet->GetPropertyAnimationByIndex( i );
			if( anim->m_animationName == name )
			{
				return anim;
			}
		}
	}

	return nullptr;
}

Bool CDoorAttachment_PropertyAnimation::IsClosed() const
{
	return MAbs( m_openPercentage ) <= 0.05f;
}

Bool CDoorAttachment_PropertyAnimation::IsOpened() const
{
	return !IsClosed();
}

//////////////////////////////////////////////////////////////////////////////////////////////
CDoorAttachment_GameplayPush::CDoorAttachment_GameplayPush()
	: m_openAngle( 90.0f )	
	, m_autoCloseForce( 60.0f )
	, m_openingSpeed( 400 )
	, m_accumulatedYaw( 0 ) 	
	, m_stateForced( false )
	, m_playerNotificationkCalled( false )
	, m_flipForward( false )
	, m_invertedPivot( false )
{

}

Bool CDoorAttachment_GameplayPush::Init( CNode* parent, CNode* child, const AttachmentSpawnInfo* info )
{
	// make sure owner is CGameplayEntity	
	if( !child->IsA<CRigidMeshComponent>() && !child->IsA<CExternalProxyComponent>() )
	{
		//GFeedback->ShowWarn( TXT("Child must be a CRigidMeshComponent") );		
		return false;
	}
	Vector forward( 1.0f, 0.0f, 0.0f );
	
	m_flipForward = Vector::Dot3( child->GetRotation().TransformVector( forward ), parent->GetRotation().TransformVector( forward ) ) < 0.0f;
	//m_flipForward =  Vector::Dot3( child->GetWorldForward(), parent->GetWorldForward() ) < 0;
	if( m_invertedPivot )
	{
		m_flipForward = ! m_flipForward;
	}
	m_playerNotificationkCalled = false;

	Bool ret = TBaseClass::Init( parent, child, info );

	// Saved "accumulated yaw" can be wrong if door component was streamed-out before saving its state.
	// Thus, to avoid cases when door has wrong rotation and it cannot be changed (cause it is disabled)
	// we apply "accumulated yaw" only for enabled door (this has another explanation: accumulated yaw is used
	// to store state of the door which is "moving" when saving its state - disabled door cannot be moved so
	// its rotation should be taken directly from the current state).
	CDoorComponent* door = Cast< CDoorComponent >( parent );
	if( door &&	GGame->IsActive() && door->IsDoorEnabled() )
	{				
		Float accumulatedYaw = door->SavedPushableAccumulatedYaw();
		if ( IsAngleValidForState( accumulatedYaw, static_cast< Uint32 >( door->GetDesiredState() ) ) )
		{
			ForceAccumulatedYaw( accumulatedYaw );
		}
	}

	return ret;
}

void  CDoorAttachment_GameplayPush::ForceAccumulatedYaw( Float yaw )
{
	CRigidMeshComponent* child = Cast<CRigidMeshComponent>( GetChild() );
	if( !child )
	{
		return;
	}
	EulerAngles newAngle = child->GetRotation();
	newAngle.Yaw = m_originalAngle + yaw;
	m_accumulatedYaw = yaw;
	child->SetRotation( newAngle );
	child->ForceUpdateTransformNodeAndCommitChanges();

	m_yawSpeed = 0;
}

void CDoorAttachment_GameplayPush::AddForceImpulse( const Vector& origin, Float force )
{
	CRigidMeshComponent* rigidMesh = Cast<CRigidMeshComponent>( GetChild() );
	CDoorComponent* door = GetDoor();

	if( !rigidMesh || !door )
	{
		return;
	}

	// don't add impulses to locked doors
	if( door->IsLocked() )
	{
		return;
	}

	if( GGame->GetPlayerEntity() )
	{
		CPlayer* player = Cast< CPlayer >( GGame->GetPlayerEntity() );
		if( player && player->IsInCombat() && ( player->GetWorldPositionRef() - rigidMesh->GetWorldPositionRef() ).SquareMag3() < COMBAT_LOCK_DISTANCE_SQRT  )
		{
			return;
		}
	}

	// get door plane
	Plane doorPlane;
	if( !GetDoorPlane( doorPlane ) )
	{
		return;
	}

	Vector doorPos = rigidMesh->GetBoundingBox().CalcCenter();
	Float distance = ( doorPos - origin ).Mag3();
	
	// modify yaw speed
	m_yawSpeed -= ( force / distance ) * ( doorPlane.GetSide( origin ) == Plane::PS_Front ? 1.0f : -1.0f );

	// cap speed
	m_yawSpeed = Clamp( m_yawSpeed, -500.0f, 500.0f );
}

const float CDoorAttachment_GameplayPush::YAW_EPS = 0.01f;
const float CDoorAttachment_GameplayPush::ANG_EPS = 0.01f;
const float CDoorAttachment_GameplayPush::COMBAT_LOCK_DISTANCE_SQRT = 15 * 15; 
const float CDoorAttachment_GameplayPush::HORSE_BONUS_RADIUS = 2.0f;

Bool CDoorAttachment_GameplayPush::Update( Float timeDelta ) 
{
	static const float DOORS_SAFE_CLOSING_DISTANCE = 1.5f;

	// get player, door & door mesh
	CRigidMeshComponent* rigidMesh = Cast<CRigidMeshComponent>( GetChild() );
	if( !rigidMesh )
		return true;

	Bool inCombat = false;

	if( GGame->GetPlayerEntity() )
	{
		CPlayer* player = Cast< CPlayer >( GGame->GetPlayerEntity() );
		if( player && player->IsInCombat() && ( player->GetWorldPositionRef() - rigidMesh->GetWorldPositionRef() ).SquareMag3() < COMBAT_LOCK_DISTANCE_SQRT  )
		{// lock doors when player is in combat
			inCombat = true;
		}
	}

	CDoorComponent* door = GetDoor();
	const TDynArray< THandle< CActor > >& doorUsers = door->GetDoorUsers();

	// grab a few useful values
	Vector doorPos = rigidMesh->GetBoundingBox().CalcCenter();
	doorPos.Z = rigidMesh->GetWorldPosition().Z;// + 1;	
	Bool hasCollision = false;
	Bool isLocked = door->IsLocked();
	Float smallestDist = FLT_MAX;
	

	const Vector& doorCmpPos	= door->GetWorldPositionRef();
	Vector doorSideDirection = doorPos - doorCmpPos;
	Vector doorsEndPos = doorCmpPos + doorSideDirection * 2; 
	Float doorLen = ( doorsEndPos - doorCmpPos ).Mag3();

	Float hugeDoorsBonus = doorLen > 2.5f ? 1.0f : 0.0f;

	doorSideDirection.Normalize3();
	Plane doorPlane;
	if( !GetDoorPlane( doorPlane, doorSideDirection ) )
	{
		return false;
	}

	if( !isLocked && !inCombat && !m_stateForced )
	{
		Vector doorsForward = rigidMesh->GetWorldForward();		
		if( m_flipForward )
		{
			doorsForward = -doorsForward;
		}
		for( Uint32 i=0; i<doorUsers.Size(); ++i )
		{
			CActor* doorUser = doorUsers[ i ].Get();

			if( !doorUser || doorUser->IsInNonGameplayScene() )
				continue;

			Vector userPos = doorUser->GetWorldPosition() ;//+ Vector3( 0.0f, 0.0f, 1.0f );
			Float distToUser = ( doorPos - userPos ).Mag2();							// simple point-to-point distance
			CMovingAgentComponent* mac = doorUser->GetMovingAgentComponent();
			Float userRadius = mac ?  mac->GetRadius() : 0.25f;		// why is this too small?
			
			Vector userFrward = doorUser->GetWorldForward();
			// player within radius? player within height?
			if( distToUser < 4.0f && Abs( doorPos.Z - userPos.Z ) < 1.5f )
			{				

				// player within door frame?															
				Vector userPosOnPlane	= doorPlane.Project( userPos );				
								
				Float reverseLerp = ( userPosOnPlane - doorCmpPos ).Mag3() / doorLen;
				reverseLerp -= ( userRadius*0.75f ) / doorLen;

				Float userProjDistance		= ( userPos - userPosOnPlane ).Mag3();
			//	Float userDoorDistaceSqrt	= ( userPosOnPlane - doorPos ).SquareMag3();

				Float dotResult = Vector::Dot3( doorSideDirection, doorUser->GetWorldPositionRef() - doorCmpPos );
								
				Bool isOnHorse = false;
				if( CallFunctionRet< Bool >( doorUser, CNAME( IsUsingHorse ), true, isOnHorse ) && isOnHorse )
				{
					userRadius += HORSE_BONUS_RADIUS;
				}

				if( 
					userProjDistance < 0.4f + userRadius + hugeDoorsBonus
					&& dotResult > 0 
					&& reverseLerp <= 1
				)
				{
					distToUser = Abs( doorPlane.DistanceTo( userPos ) );		// update distance to more accurate user-to-door-plane distance

					NotifyPlayerOpened( doorUser );

					//distToUser = ( doorPosOnPlane - userPos ).Mag3();
					if( distToUser < smallestDist )
					{					
						if( distToUser < userRadius + 0.4f )
						{
							m_yawSpeed = ( Vector::Dot3( userPosOnPlane - userPos, doorsForward ) > 0 ? m_openingSpeed: -m_openingSpeed );
							hasCollision = true;
						}
					}
				}
				else
				{
					distToUser = Min( distToUser, ( doorCmpPos - userPos).Mag2() );				
				}
				smallestDist = Min( smallestDist, distToUser );


				// debug stuffs
				
				//GCommonGame->GetVisualDebug()->AddLine( CName( TXT( "DL1" ) ), userPos	 , userPosOnPlane			 , true, Color::BLUE, 0.1f );			
				//GCommonGame->GetVisualDebug()->AddLine( CName( TXT( "DL2" ) ), doorCmpPos + Vector(0 ,0, 1 ),  doorCmpPos + Vector(0 ,0, 1 ) + doorsForward, true, Color::GREEN, 0.1f );

				//GCommonGame->GetVisualDebug()->AddSphere( CName( TXT( "DS1" ) ), 0.2f, userPosOnPlane, true, Color::GREEN, 0.1f );
				//GCommonGame->GetVisualDebug()->AddSphere( CName( TXT( "DS2" ) ), 0.2f, doorCmpPos + doorSideDirection, true, Color::RED, 0.1f );
				//GCommonGame->GetVisualDebug()->AddSphere( CName( TXT( "DS3" ) ), 0.2f, doorCmpPos, true, Color::BLACK, 0.1f );
				//GCommonGame->GetVisualDebug()->AddSphere( CName( TXT( "DS3" ) ), 0.2f, doorCmpPos + doorsForward, true, Color::RED, 0.1f );
				//GCommonGame->GetVisualDebug()->AddSphere( CName( TXT( "DS4" ) ), 0.2f, doorsEndPos, true, Color::GREEN, 0.1f );
				
			}
		}
	}

	// get current door rotation [-180, 180]
	EulerAngles doorRotation = rigidMesh->GetRotation();
	float dYaw = timeDelta * m_yawSpeed;	
	doorRotation.Yaw = EulerAngles::NormalizeAngle( doorRotation.Yaw + dYaw );
	
	if( doorRotation.Yaw > 180.0f )
	{
		doorRotation.Yaw -= 360.0f;
	}

	if( m_yawSpeed * m_accumulatedYaw < 0 && Abs( m_accumulatedYaw ) < ANG_EPS )
	{//closing
		doorRotation.Yaw = m_originalAngle;
		rigidMesh->SetRotation( doorRotation );
		//HACK Should be converted to rigidMesh->ScheduleUpdateTransformNode();
		rigidMesh->ForceUpdateTransformNodeAndCommitChanges();

		PlayStopClosingSounds();
		m_yawSpeed = 0;
		m_accumulatedYaw = 0;
	}

	if( Abs( m_yawSpeed ) > YAW_EPS )
	{
		// dampen the speed
		m_yawSpeed *= ( 1.0f - timeDelta * 3.0f );		

		if( Abs( m_accumulatedYaw + dYaw ) < Abs( m_openAngle ) )
		{
			if( Abs( m_accumulatedYaw ) > ANG_EPS && Red::Math::MSign( m_accumulatedYaw ) != Red::Math::MSign( m_accumulatedYaw + dYaw ) )
			{
				//0 crossed
				doorRotation.Yaw = m_originalAngle;
				rigidMesh->SetRotation( doorRotation );
				rigidMesh->ForceUpdateTransformNodeAndCommitChanges();

				PlayStopClosingSounds();
				m_yawSpeed = 0;
				m_accumulatedYaw = 0;
			}
			else
			{			
				// update door from rotation speed
				m_accumulatedYaw += dYaw;
				rigidMesh->SetRotation( doorRotation );
				//HACK Should be converted to rigidMesh->ScheduleUpdateTransformNode();
				rigidMesh->ForceUpdateTransformNodeAndCommitChanges();
			}
		}
		else if( Abs( m_accumulatedYaw ) < Abs( m_openAngle ) )
		{
			m_accumulatedYaw = Red::Math::MSign( m_accumulatedYaw ) *  Abs( m_openAngle );
			doorRotation.Yaw = m_originalAngle + m_accumulatedYaw;
			rigidMesh->SetRotation( doorRotation );
			rigidMesh->ForceUpdateTransformNodeAndCommitChanges();
		}
		/*else if( !hasCollision && smallestDist > DOORS_SAFE_CLOSING_DISTANCE )
		{
			// reflect speed if door hits max / min angle (and users are not too close)
			m_yawSpeed *= -0.5f;

			PlayStartClosingSounds();
			//m_accumulatedYaw = m_openAngle;
		}*/
		else 
		{
			// stop door if player is running against it
			m_yawSpeed = 0.0f;
			PlayStopOpeningSounds();
		}
	}	

	// auto close door?
	if( ( m_stateForced || smallestDist > DOORS_SAFE_CLOSING_DISTANCE + hugeDoorsBonus ) && m_autoCloseForce > 0.001f  )
	{
		EDoorState targetState = door->GetDesiredState();
		if( targetState == Door_Closed && !IsClosed() )
		{
			m_yawSpeed += ( m_accumulatedYaw < 0.0f ? 1.0f : -1.0f ) * m_autoCloseForce * timeDelta;
			PlayStartClosingSounds();
		}
		else if( targetState == Door_Open && !IsOpened() )
		{
			if( Abs( m_accumulatedYaw ) > 5 )
			{//if it is near zero, then open to closes limit
				m_yawSpeed += ( m_accumulatedYaw > 0.0f ? 1.0f : -1.0f ) * m_autoCloseForce * timeDelta;
			}
			else
			{
				m_yawSpeed += ( m_openAngle > 0.0f ? 1.0f : -1.0f ) * m_autoCloseForce * timeDelta;
			}
		}
		else
		{
			m_yawSpeed = 0;
		}
	}

	// keep ticking if the player is inside the activation radius or the door is moving
	Float activationRadius = door->GetRangeMax() + 1.0f;
	Bool doorMoving = IsDoorMoving();	

	if( doorMoving && m_accumulatedYaw * m_yawSpeed > 0 )
	{
		PlayStartOpeningSounds();
		NotifyOpenningDoors();
	}
	
	if( !doorMoving )
	{
		PlayStopClosingSounds();
		
		if ( doorUsers.Empty() )
		{
			m_stateForced = false;
		}
	}

	UpdateDoorState();

	bool inDesiredState = IsInDesiredState();
	if ( inDesiredState )
	{
		m_stateForced = false;
	}

	Bool notDone = !inDesiredState || doorMoving;
	if( !notDone )
	{
		m_playerNotificationkCalled = false;
	}

	return notDone;
}

Bool CDoorAttachment_GameplayPush::IsClosed() const
{
	return Abs( m_accumulatedYaw ) <= 0.5f;
}

Bool CDoorAttachment_GameplayPush::IsOpened() const
{
	return ( Abs( m_openAngle ) - Abs( m_accumulatedYaw ) ) <= 0.5f;
}

Bool CDoorAttachment_GameplayPush::IsDoorMoving()
{
	return Abs( m_yawSpeed ) > 0.001f;
}

Bool CDoorAttachment_GameplayPush::IsInDesiredState()
{
	CDoorComponent* door = GetDoor();
	EDoorState targetState = door->GetDesiredState();
	return ( ( targetState == Door_Closed && IsClosed() ) || ( targetState == Door_Open && IsOpened() ) );
}

Bool CDoorAttachment_GameplayPush::IsAngleValidForState( Float angle, Uint32 doorState ) const
{
	if ( doorState == static_cast< Uint32 >( Door_Closed ) )
	{
		// when closed angle needs to be almost 0
		return Abs( angle ) <= 0.5f;
	}
	else // doorState == Door_Open
	{
		// when open or opening, the difference should be less than 90 degrees
		return ( Abs( m_openAngle ) - Abs( angle ) ) <= 89.5f;
	}
}

void IDoorAttachment::PlayStartOpeningSounds()
{
	if( !m_isOpeningPlaying )
	{
		PlayStopClosingSounds( true );

		CDoorComponent* door = GetDoor();
		if( CSoundEmitterComponent* soundEmitterComponent = door->GetEntity()->GetSoundEmitterComponent() )
		{
			soundEmitterComponent->SoundEvent( UNICODE_TO_ANSI( m_soundsEvents.m_openingStart.AsChar() ) );
			soundEmitterComponent->SoundEvent( UNICODE_TO_ANSI( m_soundsEvents.m_open.AsChar() ) );
		}
		m_isOpeningPlaying = true;
	}
}

void IDoorAttachment::PlayStopOpeningSounds( Bool _onlyLoop )
{
	if( m_isOpeningPlaying )
	{
		CDoorComponent* door = GetDoor();
		if( CSoundEmitterComponent* soundEmitterComponent = door->GetEntity()->GetSoundEmitterComponent() )
		{
			soundEmitterComponent->SoundEvent( UNICODE_TO_ANSI( m_soundsEvents.m_openingStop.AsChar() ) );
			if( !_onlyLoop )
			{
				soundEmitterComponent->SoundEvent( UNICODE_TO_ANSI( m_soundsEvents.m_openFully.AsChar() ) );
			}		
		}
		m_isOpeningPlaying = false;
	}
}

void IDoorAttachment::PlayStartClosingSounds()
{
	if( !m_isClosingPlaying )
	{		
		PlayStopOpeningSounds( true );
		CDoorComponent* door = GetDoor();
		if( CSoundEmitterComponent* soundEmitterComponent = door->GetEntity()->GetSoundEmitterComponent() )
		{
			soundEmitterComponent->SoundEvent( UNICODE_TO_ANSI( m_soundsEvents.m_closingStart.AsChar() ) );
			soundEmitterComponent->SoundEvent( UNICODE_TO_ANSI( m_soundsEvents.m_close.AsChar() ) );
		}
		m_isClosingPlaying = true;
	}
}

void IDoorAttachment::PlayStopClosingSounds( Bool _onlyLoop )
{
	if( m_isClosingPlaying )
	{
		CDoorComponent* door = GetDoor();
		if( CSoundEmitterComponent* soundEmitterComponent = door->GetEntity()->GetSoundEmitterComponent() )
		{
			soundEmitterComponent->SoundEvent( UNICODE_TO_ANSI( m_soundsEvents.m_closingStop.AsChar() ) );
			if( !_onlyLoop )
			{
				soundEmitterComponent->SoundEvent( UNICODE_TO_ANSI( m_soundsEvents.m_closeFully.AsChar() ) );
			}
		}
		m_isClosingPlaying = false;
	}
}

void IDoorAttachment::UpdateDoorState()
{
#ifdef USE_UMBRA
	Bool state = !IsClosed();
	if ( m_cachedState != state && m_umbraGateId != INVALID_UMBRA_OBJECT_ID )
	{
		CDoorComponent* door = GetDoor();
		if ( !door || !door->GetWorld() )
		{
			// something went wrong
			return;
		}

		if ( IRenderScene* renderScene = door->GetWorld()->GetRenderSceneEx() )
		{
			( new CRenderCommand_SetDoorState( renderScene, m_umbraGateId, state ) )->Commit();
			m_cachedState = state;
		}
	}
#endif // USE_UMBRA
}

void CDoorAttachment_GameplayPush::NotifyPlayerOpened( CActor* doorUser )
{	
	if( !m_playerNotificationkCalled )
	{		
		if( doorUser->IsA< CPlayer >() )
		{
			CDoorComponent* door = GetDoor();
			door->GetEntity()->CallEvent( CNAME( OnPlayerOpenedDoors ) );
			m_playerNotificationkCalled = true;
		}
	}
}

void CDoorAttachment_GameplayPush::NotifyOpenningDoors()
{
	if( ! m_openningNotificationCalled )
	{
		CDoorComponent* door = GetDoor();
		const TDynArray< THandle< CActor > >& doorUsers = door->GetDoorUsers();

		for( Int32 i=0; i<doorUsers.SizeInt(); ++i )
		{
			if( !doorUsers[ i ].Get() )
			{
				continue;
			}
			doorUsers[ i ]->CallEvent( CNAME( OnOpenningDoor ) );
			m_openningNotificationCalled = true;
		}
	}
}

void CDoorAttachment_GameplayPush::InstantClose()
{
	CDoorComponent* door = GetDoor();
	CComponent* child = Cast<CComponent>( GetChild() );
	
	InstantClose( door, child );
}

void CDoorAttachment_GameplayPush::InstantOpen()
{
	CDoorComponent* door = GetDoor();
	CComponent* child = Cast<CComponent>( GetChild() );
	
	InstantOpen( door, child );
}

void CDoorAttachment_GameplayPush::InstantClose( CDoorComponent* door, CComponent* child )
{
	if( !door || !child )
	{
		return;
	}

	EulerAngles newAngle = child->GetRotation();
	newAngle.Yaw = m_originalAngle;
	m_accumulatedYaw = 0;
	child->SetRotation( newAngle );
	child->ForceUpdateTransformNodeAndCommitChanges();

	m_yawSpeed = 0;
}

void CDoorAttachment_GameplayPush::InstantOpen( CDoorComponent* door, CComponent* child )
{
	if( !door || !child )
	{
		return;
	}


	EulerAngles newAngle = child->GetRotation();
	newAngle.Yaw = m_originalAngle + m_openAngle;
	m_accumulatedYaw = m_openAngle;
	child->SetRotation( newAngle );
	child->ForceUpdateTransformNodeAndCommitChanges();

	m_yawSpeed = 0;
}

Bool CDoorAttachment_GameplayPush::GetDoorPlane( Plane& doorPlane )
{
	CMeshComponent* mesh = Cast< CMeshComponent >( GetChild() );
	if( !mesh )
	{
		return false;
	}
	
	doorPlane = Plane( mesh->GetWorldPosition(), 
		mesh->GetWorldPosition() -  mesh->GetWorldRight(), 
		mesh->GetWorldPosition() + mesh->GetWorldUp() );

	return true;
}

Bool CDoorAttachment_GameplayPush::GetDoorPlane( Plane& doorPlane, Vector& sideDirection )
{
	CMeshComponent* mesh = Cast< CMeshComponent >( GetChild() );
	if( !mesh )
	{
		return false;
	}

	// calculate door plane
	doorPlane = Plane( mesh->GetWorldPosition(), 
					   mesh->GetWorldPosition() - sideDirection, 
					   mesh->GetWorldPosition() + mesh->GetWorldUp() );

	return true;
}
