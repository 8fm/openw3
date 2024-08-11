/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "dropPhysicsComponent.h"
#include "curveEntity.h"
#include "../physics/physicsRagdollWrapper.h"
#include "mesh.h"
#include "particleSystem.h"
#include "../core/scriptStackFrame.h"
#include "collisionShape.h"
#include "collisionMesh.h"
#include "animatedComponent.h"
#include "rigidMeshComponent.h"
#include "node.h"
#include "particleComponent.h"
#include "componentIterator.h"
#include "layer.h"
#include "world.h"
#include "tickManager.h"
#include "dynamicLayer.h"
#include "baseEngine.h"
#include "renderFrame.h"
#include "animatedComponent.h"
#include "meshSkinningAttachment.h"
#include "skeleton.h"
#include "../physics/physXEngine.h"
#include "dropPhysicsComponentModifier.h"

IMPLEMENT_ENGINE_CLASS( CDropPhysicsComponent );
IMPLEMENT_ENGINE_CLASS( CDropPhysicsSetup );
IMPLEMENT_ENGINE_CLASS( SDropPhysicsCurves );
IMPLEMENT_RTTI_ENUM( EFixBonesHierarchyType );

//////////////////////////////////////////////////////////////////////////

void SDropPhysicsCurves::Init()
{
	if ( m_trajectory.GetCurveType() == ECurveType_Uninitialized )
	{
		m_trajectory.Reset();
		m_trajectory.SetCurveType( ECurveType_Vector, NULL, false );
		m_trajectory.SetPositionInterpolationMode( ECurveInterpolationMode_Automatic );
		m_trajectory.SetTransformationRelativeMode( ECurveRelativeMode_None );
		m_trajectory.SetShowFlags( SHOW_CurveAnimations );
		m_trajectory.SetLooping( false );
		m_trajectory.SetColor( Color::BLUE );
		m_trajectory.AddControlPoint( 0.0f, Vector( 0.0f, 0.0f, 0.0f ) );
		m_trajectory.AddControlPoint( 0.5f, Vector( 0.75f, 0.75f, 1.0f ) );
		m_trajectory.AddControlPoint( 1.0f, Vector( 1.0f, 1.0f, 0.75f ) );
		m_trajectory.SetTotalTime( 0.5f );
		m_trajectory.EnableAutomaticTimeByDistanceRecalculation( true );
	}

	if ( m_rotation.GetCurveType() == ECurveType_Uninitialized )
	{
		m_rotation.Reset();
		m_rotation.SetCurveType( ECurveType_Vector, NULL, false );
		m_rotation.SetPositionInterpolationMode( ECurveInterpolationMode_Automatic );
		m_rotation.SetTransformationRelativeMode( ECurveRelativeMode_None );
		m_rotation.SetShowFlags( SHOW_CurveAnimations );
		m_rotation.SetLooping( false );
		m_rotation.SetColor( Color::RED );
		m_rotation.AddControlPoint( 0.0f, Vector( 0.0f, 0.0f, 0.0f ) );
		m_rotation.AddControlPoint( 1.0f, Vector( 720.0f, 720.0f, 720.0f ) );
		m_rotation.SetTotalTime( 0.5f );
		m_rotation.EnableAutomaticTimeByDistanceRecalculation( true );
	}
}

//////////////////////////////////////////////////////////////////////////

const CName& CDropPhysicsSetup::GetName() const
{
	return m_name;
}

SDropPhysicsCurves* CDropPhysicsSetup::SelectRandomCurves()
{
	if ( m_curves.Size() == 0 )
	{
		return nullptr;
	}
	Uint32 index = GEngine->GetRandomNumberGenerator().Get< Uint32 >( 0, m_curves.Size() );
	return &m_curves[ index ];
}

void CDropPhysicsSetup::AttachCurves( CNode* parent )
{
	TDynArray< SDropPhysicsCurves >::iterator it = m_curves.Begin();
	TDynArray< SDropPhysicsCurves >::iterator itEnd = m_curves.End();
	for ( ; it != itEnd; ++it )
	{
		it->m_trajectory.SetParent( parent );
		it->m_rotation.SetParent( parent );
	}
}

void CDropPhysicsSetup::DeleteCurveEditors( CWorld* world )
{
	TDynArray< SDropPhysicsCurves >::iterator it = m_curves.Begin();
	TDynArray< SDropPhysicsCurves >::iterator itEnd = m_curves.End();
	for ( ; it != itEnd; ++it )
	{
		CCurveEntity::DeleteEditor( world, &it->m_trajectory );
		CCurveEntity::DeleteEditor( world, &it->m_rotation );
	}
}

void CDropPhysicsSetup::OnPropertyPostChange( IProperty* property )
{
	if ( property->GetName() == TXT( "curves" ) )
	{
		InitCurves();
		CComponent* parentComponent = Cast< CComponent >( GetParent() );
		if ( parentComponent != nullptr && parentComponent->IsAttached() )
		{
			CCurveEntity::DeleteEditors( parentComponent->GetLayer()->GetWorld(), false );
			AttachCurves( parentComponent->GetEntity() );
		}
	}
}

void CDropPhysicsSetup::InitCurves()
{
	TDynArray< SDropPhysicsCurves >::iterator it = m_curves.Begin();
	TDynArray< SDropPhysicsCurves >::iterator itEnd = m_curves.End();
	for ( ; it != itEnd; ++it )
	{
		it->Init();
	}
}

//////////////////////////////////////////////////////////////////////////

CDropPhysicsComponent::SDisableRagdollInfo::SDisableRagdollInfo()
	: m_type( DRT_Both )
	, m_spawnedEntity( nullptr )
	, m_baseEntity( nullptr )
	, m_spawnedEntityBones( nullptr )
	, m_fixBaseBonesHierarchyType( FBHTAddMissingBones )
	, m_fixSpawnedBonesHierarchyType( FBHTAddMissingBones )
{
}

//////////////////////////////////////////////////////////////////////////

CDropPhysicsComponent::SDroppedEntityPhysicsWrapper::SDroppedEntityPhysicsWrapper( CEntity* entity )
	: m_entity( entity )
	, m_physicsInterface( nullptr )
	, m_flags( DEPWF_WaitingForComponents )
	, m_linearVelocity( Vector::ZEROS )
	, m_angularVelocity( Vector::ZEROS )
	, m_validActorIndex( -1 )
{
	Init();
}

void CDropPhysicsComponent::SDroppedEntityPhysicsWrapper::SetEntity( CEntity* entity )
{
	m_entity = entity;
	m_flags = 0;
	Init();
}

void CDropPhysicsComponent::SDroppedEntityPhysicsWrapper::Init()
{
	RED_ASSERT( m_physicsInterface == nullptr );

	if ( m_entity == nullptr )
	{
		return;
	}

	// .. and we're still waiting for components to load
	if ( m_flags & DEPWF_WaitingForComponents )
	{
		CAnimatedComponent* ac = m_entity->GetRootAnimatedComponent();
		if ( ac != nullptr  )
		{
			m_flags &= ~DEPWF_WaitingForComponents;
			// We check if animated component has a ragdoll resource.
			// We need this info to obtain its physics interface in the next frame(s)
			// (cause probably it won't be accessible right after entity's spawn).		
			if ( ac->HasRagdoll() )
			{
				m_flags |= DEPWF_HasRagdoll;
				// There's a slight chance that we already have ragdoll's physics interface.
				m_physicsInterface = ac->GetRagdollPhysicsWrapper();
				if ( m_physicsInterface == nullptr )
				{
					m_flags |= DEPWF_WaitingForRagdollWrapper;
				}
			}		
		}
		else
		{
			CRigidMeshComponent* rigidMesh = m_entity->FindComponent< CRigidMeshComponent >();
			if ( rigidMesh != nullptr )
			{
				m_flags &= ~DEPWF_WaitingForComponents;
				m_physicsInterface = rigidMesh->GetPhysicsRigidBodyWrapper();
				if ( m_physicsInterface == nullptr )
				{
					m_flags |= DEPWF_WaitingForRigidBodyWrapper;
				}
			}
		}
	}
	if ( m_flags & DEPWF_WaitingForRagdollWrapper )
	{
		m_flags &= ~DEPWF_WaitingForRagdollWrapper;
		CAnimatedComponent* ac = m_entity->GetRootAnimatedComponent();
		if ( ac != nullptr )
		{
			m_physicsInterface = ac->GetRagdollPhysicsWrapper();
			if ( m_physicsInterface == nullptr )
			{
				m_flags |= DEPWF_WaitingForRagdollWrapper;
			}
		}
	}
	else if ( m_flags & DEPWF_WaitingForRigidBodyWrapper )
	{
		m_flags &= ~DEPWF_WaitingForRigidBodyWrapper;
		CRigidMeshComponent* rigidMesh = m_entity->FindComponent< CRigidMeshComponent >();
		if ( rigidMesh != nullptr )
		{
			m_physicsInterface = rigidMesh->GetPhysicsRigidBodyWrapper();
			if ( m_physicsInterface == nullptr )
			{
				m_flags |= DEPWF_WaitingForRigidBodyWrapper;
			}
		}
	}

	// If we have finally got an interface we need to update its state.
	if ( m_physicsInterface != nullptr )
	{
		SwitchToKinematicInternal();
		ApplyForcesInternal();
	}
}

void CDropPhysicsComponent::SDroppedEntityPhysicsWrapper::Update()
{
	// If there's still no physics interface...
	if ( m_physicsInterface == nullptr )
	{
		Init();
	}
	else
	{
		if ( ShouldSwitchToKinematic() )
		{
			SwitchToKinematicInternal();
		}
		if ( ShouldApplyForces() )
		{
			ApplyForcesInternal();
		}
	}
}

void CDropPhysicsComponent::SDroppedEntityPhysicsWrapper::SwitchToKinematic( Bool kinematic )
{
	Bool isKinematic = ( m_flags & DEPWF_Kinematic ) != 0;
	if ( isKinematic == kinematic )
	{
		return;
	}
	if ( kinematic )
	{
		m_flags |= DEPWF_Kinematic;
	}
	else
	{
		m_flags &= ~DEPWF_Kinematic;
	}
	// marking as "not switched yet"
	m_flags &= ~DEPWF_SwitchedToKinematic;
	if ( m_physicsInterface != nullptr )
	{
		SwitchToKinematicInternal();
	}
}

Bool CDropPhysicsComponent::SDroppedEntityPhysicsWrapper::ShouldSwitchToKinematic() const
{
	return ( m_flags & DEPWF_SwitchedToKinematic ) == 0;
}

void CDropPhysicsComponent::SDroppedEntityPhysicsWrapper::SwitchToKinematicInternal()
{
	RED_ASSERT( m_physicsInterface != nullptr );

	const Bool isKinematic = ( m_flags & DEPWF_Kinematic );
	m_physicsInterface->SwitchToKinematic( isKinematic );
	m_physicsInterface->SetFlag( PRBW_CollisionDisabled, isKinematic );

	m_flags |= DEPWF_SwitchedToKinematic;
}

void CDropPhysicsComponent::SDroppedEntityPhysicsWrapper::ApplyForces( const Vector& linearVelocity, const Vector& angularVelocity )
{
	m_linearVelocity = linearVelocity;
	m_angularVelocity = angularVelocity;
	// marking as "not applied yet"
	m_flags &= ~DEPWF_ForcesApplied;
	if ( m_physicsInterface != nullptr )
	{
		ApplyForcesInternal();
	}
}

Bool CDropPhysicsComponent::SDroppedEntityPhysicsWrapper::ShouldApplyForces() const
{
	return ( m_flags & DEPWF_ForcesApplied ) == 0;
}

void CDropPhysicsComponent::SDroppedEntityPhysicsWrapper::ApplyForcesInternal()
{
	RED_ASSERT( m_physicsInterface != nullptr );

	if ( m_linearVelocity != Vector::ZEROS || m_angularVelocity != Vector::ZEROS )
	{
		if ( !m_physicsInterface->IsReady() || m_physicsInterface->IsKinematic() )
		{
			return;
		}

		Uint32 defaultActorIndex = GetValidActorIndex();
		// we try to apply velocity to an actor only to check if it is possible (wrapper is loaded and ready)
		Bool res = m_physicsInterface->SetVelocityLinear( m_linearVelocity, defaultActorIndex );
		if ( res )
		{
			Uint32 usedActorsCount = 0;

			// angular velocity denotes velocity in local space, but we will apply it in the global space
			// thus, we need to transform "local" Euler angles to global ones
			const Float dt = 0.001f;
			const Float _1_dt = 1.0f / dt;
			const Vector angularVelocityDtLS = m_angularVelocity * dt;
			const Matrix rotationDtLS = EulerAngles( angularVelocityDtLS.X, angularVelocityDtLS.Y, angularVelocityDtLS.Z ).ToMatrix();
			Matrix entityLocalToWorld;
			m_entity->GetLocalToWorld( entityLocalToWorld );
			entityLocalToWorld.SetTranslation( Vector::ZEROS );
			const Matrix entityLocalToWorldInv = entityLocalToWorld.Transposed(); // we will use only rotations so transpose is sufficient
			const Matrix rotationDtWS = entityLocalToWorldInv * rotationDtLS * entityLocalToWorld;
			const EulerAngles angularVelocityDtWS = rotationDtWS.ToEulerAngles();
			const Vector angularVelocityRadiansWS = ( Vector( angularVelocityDtWS.Pitch, angularVelocityDtWS.Roll, angularVelocityDtWS.Yaw ) * M_PI / 180.0f ) * _1_dt;
			const Float maxAngularVelocityRadians = angularVelocityRadiansWS.Mag3();

			if ( m_entity.Get() != nullptr && ( m_flags & DEPWF_HasRagdoll ) )
			{
				// for entities with ragdolls, we do a trick and compute forces for all actors
				// relative to its center of mass
				CAnimatedComponent* ac = m_entity->GetRootAnimatedComponent();
				if ( ac != nullptr && ac->GetSkeleton() != nullptr && ac->GetRagdollPhysicsWrapper() == m_physicsInterface )
				{
					RED_ASSERT( ac->HasRagdoll() );
					Vector centerOfMass = GetCenterOfMassPosition( m_entity.Get(), m_physicsInterface, defaultActorIndex );
					CSkeleton* skeleton = ac->GetSkeleton();
					const Uint32 size = skeleton->GetBonesNum();
					for ( Uint32 i = 0; i < size; i++ )
					{
						Int32 actorIndex = m_physicsInterface->GetIndex( skeleton->GetBoneNameAnsi( i ) );
						if ( actorIndex >= 0 )
						{
#ifdef USE_PHYSX
							physx::PxActor* actor = ( physx::PxActor* ) m_physicsInterface->GetActor( actorIndex );
							if ( actor != nullptr && actor->getScene() != nullptr )
							{
								usedActorsCount++;
								defaultActorIndex = actorIndex;								
								const Vector actorCenterOfMass = m_physicsInterface->GetCenterOfMassPosition( actorIndex );
								const Vector offsetWS = actorCenterOfMass - centerOfMass;
								const Vector rotatedOffsetWS = rotationDtWS.TransformVector( offsetWS );
								const Vector approxAngularVelocity = ( centerOfMass + rotatedOffsetWS - actorCenterOfMass ) * _1_dt;
								Vector velocity = m_linearVelocity + approxAngularVelocity;
								velocity.W = 0.0f;
								m_physicsInterface->SetVelocityLinear( velocity, actorIndex );
								m_physicsInterface->SetMaxVelocityAngular( maxAngularVelocityRadians, actorIndex );
								m_physicsInterface->SetVelocityAngular( angularVelocityRadiansWS, actorIndex );
							}
#endif
						}
					}
				}
			}
			// for rigid bodies or ragdolls with only one actor let's just apply angular velocity and tweak maximal velocity if needed
			if ( usedActorsCount < 2 )
			{
				m_physicsInterface->SetMaxVelocityAngular( maxAngularVelocityRadians, defaultActorIndex );
				m_physicsInterface->SetVelocityAngular( angularVelocityRadiansWS, defaultActorIndex );
			}
			m_linearVelocity = Vector::ZEROS;
			m_angularVelocity = Vector::ZEROS;
			m_flags |= DEPWF_ForcesApplied;
		}
	}
	else
	{
		m_flags |= DEPWF_ForcesApplied;
	}
}

void CDropPhysicsComponent::SDroppedEntityPhysicsWrapper::InitCenterOfMassOffsets( const SDropPhysicsInfo& info )
{
	if ( info.m_centerOfMassOffsetLS != Vector::ZEROS )
	{
		m_flags |= DEPWF_OffsetsCached;
	}
}

Vector CDropPhysicsComponent::SDroppedEntityPhysicsWrapper::GetCenterOfMassOffsetLS( SDropPhysicsInfo& info )
{
	if ( m_flags & DEPWF_OffsetsCached )
	{
		return info.m_centerOfMassOffsetLS;
	}

	if ( m_flags & DEPWF_WaitingForWrapper )
	{
		return Vector::ZEROS;
	}

	if ( CDropPhysicsComponent::GetCenterOfMassOffsets( m_entity.Get(), m_physicsInterface, info.m_centerOfMassOffsetLS, info.m_initialCenterOfMassOffsetWS, GetValidActorIndex(), nullptr, &info.m_initialRotation ) )
	{
		m_flags |= DEPWF_OffsetsCached;
	}

	return info.m_centerOfMassOffsetLS;
}

Vector CDropPhysicsComponent::SDroppedEntityPhysicsWrapper::GetInitialCenterOfMassOffsetWS( SDropPhysicsInfo& info )
{
	if ( m_flags & DEPWF_OffsetsCached )
	{
		return info.m_initialCenterOfMassOffsetWS;
	}

	if ( m_flags & DEPWF_WaitingForWrapper )
	{
		return Vector::ZEROS;
	}

	if ( CDropPhysicsComponent::GetCenterOfMassOffsets( m_entity.Get(), m_physicsInterface, info.m_centerOfMassOffsetLS, info.m_initialCenterOfMassOffsetWS, GetValidActorIndex(), nullptr, &info.m_initialRotation ) )
	{
		m_flags |= DEPWF_OffsetsCached;
	}

	return info.m_initialCenterOfMassOffsetWS;
}

Uint32 CDropPhysicsComponent::SDroppedEntityPhysicsWrapper::GetValidActorIndex()
{
	if ( m_validActorIndex >= 0 )
	{
		return static_cast< Uint32 >( m_validActorIndex );
	}

	if ( m_flags & DEPWF_WaitingForWrapper )
	{
		return 0;
	}

	// From now on we are able to cache actorIndex
	// and if this won't happen then there's nothing more we can do,
	// so let's leave the index equal to 0 (valid for rigid body).
	m_validActorIndex = 0;

	if ( m_entity.Get() != nullptr && m_physicsInterface != nullptr && ( m_flags & DEPWF_HasRagdoll ) )
	{
		CAnimatedComponent* ac = m_entity->GetRootAnimatedComponent();
		CSkeleton* skeleton = nullptr;
		if ( ac != nullptr && ac->HasRagdoll() && ac->GetRagdollPhysicsWrapper() == m_physicsInterface )
		{
			skeleton = ac->GetSkeleton();
		}
		if ( skeleton == nullptr )
		{
			return 0;
		}
		const Uint32 size = skeleton->GetBonesNum();
		for ( Uint32 i = 0; i < size; i++ )
		{
			Int32 actorIndex = m_physicsInterface->GetIndex( skeleton->GetBoneNameAnsi( i ) );
			if ( actorIndex >= 0 )
			{
#ifdef USE_PHYSX
				physx::PxActor* actor = ( physx::PxActor* ) m_physicsInterface->GetActor( actorIndex );
				if ( actor != nullptr && actor->getScene() != nullptr )
				{
					m_validActorIndex = actorIndex;
					break;
				}
#endif
			}
		}
	}
	return static_cast< Uint32 >( m_validActorIndex );
}

//////////////////////////////////////////////////////////////////////////

const Float CDropPhysicsComponent::SDroppedEntity::DESTROY_RANGE_SQUARE = 50.0f * 50.0f;

CDropPhysicsComponent::SDroppedEntity::SDroppedEntity()
	: m_entity( nullptr )
	, m_physicsWrapper( nullptr )
	, m_timer( 0.0f )
	, m_curveSpace( Matrix::IDENTITY )
	, m_positionAt0( Vector::ZEROS )
	, m_rotationAt0( Vector::ZEROS )
{
}

CDropPhysicsComponent::SDroppedEntity::SDroppedEntity( CEntity* entity, const SDropPhysicsInfo& dropInfo )
	: m_entity( entity )
	, m_physicsWrapper( entity )
	, m_timer( 0.0f )
	, m_curveSpace( Matrix::IDENTITY )
	, m_dropInfo( dropInfo )
	, m_positionAt0( Vector::ZEROS )
	, m_rotationAt0( Vector::ZEROS )
{
	if ( !( dropInfo.m_flags & SDropPhysicsInfo::DPIF_DoNotUpdate ) )
	{
		Init();
	}
}

CDropPhysicsComponent::SDroppedEntity::SDroppedEntity( CRigidMeshComponent* rigidMesh, const SDropPhysicsInfo& dropInfo )
	: m_entity( nullptr )
	, m_physicsWrapper( nullptr )
	, m_timer( 0.0f )
	, m_curveSpace( Matrix::IDENTITY )
	, m_dropInfo( dropInfo )
	, m_positionAt0( Vector::ZEROS )
	, m_rotationAt0( Vector::ZEROS )
{
	RED_ASSERT( rigidMesh != nullptr );
	m_entity = rigidMesh->GetEntity();
	if ( !( dropInfo.m_flags & SDropPhysicsInfo::DPIF_DoNotUpdate ) )
	{
		m_physicsWrapper.SetEntity( rigidMesh->GetEntity() );
		Init();
	}
}

void CDropPhysicsComponent::SDroppedEntity::Init()
{
	Float len = m_dropInfo.m_direction.Mag3();
	if ( len > 0.001f )
	{
		Vector y = m_dropInfo.m_direction * ( 1.0f / len );
		Vector x = Vector::Cross( y, Vector::EZ );
		x.Normalize3();
		Vector z = Vector::Cross( x, y );
		m_curveSpace.SetRows( x, y, z, Vector::EW );
	}

	m_physicsWrapper.SwitchToKinematic( true );
	m_physicsWrapper.InitCenterOfMassOffsets( m_dropInfo );

	// curves normalization
	if ( m_dropInfo.m_flags & SDropPhysicsInfo::DPIF_NormalizeCurves )
	{
		if ( m_dropInfo.m_trajectoryCurve != nullptr )
		{
			m_dropInfo.m_trajectoryCurve->GetPosition( 0.0f, m_positionAt0 );
		}
		if ( m_dropInfo.m_rotationCurve != nullptr )
		{
			m_dropInfo.m_rotationCurve->GetPosition( 0.0f, m_rotationAt0 );
		}
	}
}

Bool CDropPhysicsComponent::SDroppedEntity::Update( const Vector& baseEntityWorldPos, Float timeDelta )
{	
	if ( m_dropInfo.m_flags & SDropPhysicsInfo::DPIF_DoNotUpdate )
	{
		return true;
	}

	CEntity* entity = m_entity.Get();	
	// if there's no entity or entity is too far from base entity
	// (probably we will need to check it's visibility as well)
	if ( entity == nullptr )
	{
		return false;
	}
	if ( ( entity->GetWorldPosition() - baseEntityWorldPos ).SquareMag3() > DESTROY_RANGE_SQUARE )
	{
		entity->Destroy();	
		return false;
	}

	m_timer += timeDelta;
	m_physicsWrapper.Update();

	// if we're still animated by the curve
	if ( m_timer < GetCurveDuration() )
	{
		Vector entityPosition = Vector::ZEROS;
		EulerAngles entityRotation = EulerAngles::ZEROS;
		GetCoordinatesAtTime( m_timer, entityPosition, entityRotation );
		entity->SetPosition( entityPosition );
		entity->SetRotation( entityRotation );
	}
	// otherwise if collision wasn't enabled -> enable it
	else if ( m_physicsWrapper.IsKinematic() )
	{
		m_physicsWrapper.SwitchToKinematic( false );

		// also apply initial forces (velocities)
		const Float dt = 0.05f;
		Float now = GetCurveDuration();
		Float prev = Max( 0.0f, now - dt );
		if ( now > prev )
		{
			Float f = 1.0f / ( now - prev );
			Vector linearVelocity = m_curveSpace.TransformVector( GetCurvePosition( now ) - GetCurvePosition( prev ) ) * f;
			Vector angularVelocity = ( GetCurveRotation( now ) - GetCurveRotation( prev ) ) * f;
			m_physicsWrapper.ApplyForces( linearVelocity, angularVelocity );
		}
	}

	return true;
}

void CDropPhysicsComponent::SDroppedEntity::GetCoordinatesAtTime( Float time, Vector& outEntityPosition, EulerAngles& outEntityRotation,
																  Vector* outCenterOfMassPosition /* = nullptr */,
																  EulerAngles* outRotationDelta /* = nullptr */ )
{
	Vector rotVect = GetCurveRotation( time );
	EulerAngles currentAngles = EulerAngles( rotVect.X, rotVect.Y, rotVect.Z );
	outEntityRotation = m_dropInfo.m_initialRotation + currentAngles;
	if ( outRotationDelta != nullptr )
	{
		*outRotationDelta = currentAngles;
	}

	Vector centerOfMassPosition = Vector::ZEROS;
	if ( m_dropInfo.m_initialPositionType == SDropPhysicsInfo::DPIPT_Entity )
	{
		const Vector initialOffsetWS = m_physicsWrapper.GetInitialCenterOfMassOffsetWS( m_dropInfo );
		centerOfMassPosition = m_dropInfo.m_initialPosition + initialOffsetWS + m_curveSpace.TransformVector( GetCurvePosition( time ) );
	}
	else // if ( m_dropInfo.m_initialPositionType == SDropPhysicsInfo::DPIPT_CenterOfMass )
	{
		centerOfMassPosition = m_dropInfo.m_initialPosition + m_curveSpace.TransformVector( GetCurvePosition( time ) );
	}

	if ( outCenterOfMassPosition != nullptr )
	{
		*outCenterOfMassPosition = centerOfMassPosition;
	}
	const Vector offsetLS = m_physicsWrapper.GetCenterOfMassOffsetLS( m_dropInfo );
	const Vector rotatedOffsetWS = outEntityRotation.ToMatrix().TransformVector( offsetLS );
	outEntityPosition = centerOfMassPosition - rotatedOffsetWS;
}

void CDropPhysicsComponent::SDroppedEntity::OnBaseComponentDetached()
{
	if ( m_dropInfo.m_flags & SDropPhysicsInfo::DPIF_DespawnAlongWithBase )
	{
		CEntity* entity = m_entity.Get();	
		if ( entity != nullptr )
		{
			entity->Destroy();	
		}
	}
}

Vector CDropPhysicsComponent::SDroppedEntity::GetCurvePosition( Float time )
{
	Vector pos = Vector::ZEROS;
	if ( m_dropInfo.m_trajectoryCurve != nullptr )
	{
		m_dropInfo.m_trajectoryCurve->GetAbsolutePosition( time, pos );
	}
	return pos - m_positionAt0;
}

Vector CDropPhysicsComponent::SDroppedEntity::GetCurveRotation( Float time )
{
	Vector rot = Vector::ZEROS;
	if ( m_dropInfo.m_rotationCurve != nullptr )
	{
		m_dropInfo.m_rotationCurve->GetPosition( time, rot );
	}
	rot = rot - m_rotationAt0;
	::Swap( rot.X, rot.Y );
	return rot;
}

Float CDropPhysicsComponent::SDroppedEntity::GetCurveDuration()
{
	if ( m_dropInfo.m_trajectoryCurve != nullptr && m_dropInfo.m_rotationCurve != nullptr )
	{
		return Min( m_dropInfo.m_trajectoryCurve->GetTotalTime(), m_dropInfo.m_rotationCurve->GetTotalTime() );
	}
	else if ( m_dropInfo.m_trajectoryCurve != nullptr )
	{
		return m_dropInfo.m_trajectoryCurve->GetTotalTime();
	}
	else if ( m_dropInfo.m_rotationCurve != nullptr )
	{
		return m_dropInfo.m_rotationCurve->GetTotalTime();
	}
	return 0.0f;
}

void CDropPhysicsComponent::SDroppedEntity::DrawDebug( CRenderFrame* frame, EShowFlags flag )
{
	CEntity* entity = m_entity.Get();
	if ( entity == nullptr || m_timer > GetCurveDuration() || m_physicsWrapper.Get() == nullptr )
	{
		return;
	}

	const Uint32 points = 20;
	Vector position = Vector::ZEROS;
	EulerAngles rotation = EulerAngles::ZEROS;
	Vector initCenterOfMassPosition = Vector::ZEROS;
	GetCoordinatesAtTime( 0.0f, position, rotation, &initCenterOfMassPosition );
	Vector prev = m_curveSpace.TransformPoint( GetCurvePosition( 0.0f ) );
	for ( Uint32 i = 1; i <= points; ++i )
	{
		Vector p = m_curveSpace.TransformPoint( GetCurvePosition( static_cast< Float >( i ) / points ) );
		frame->AddDebugLine( initCenterOfMassPosition + prev, initCenterOfMassPosition + p, Color::RED, true );
		prev = p;
	}
		
	// initial position + initial offset WS
	const Vector initialPosition = m_dropInfo.m_initialPosition;
	frame->AddDebugSphere( initialPosition, 0.135f, Matrix::IDENTITY, Color::RED, true );
	frame->AddDebugLine( initialPosition, initialPosition + m_physicsWrapper.GetInitialCenterOfMassOffsetWS( m_dropInfo ), Color::RED, true );

	// current computed center of mass
	Vector centerOfMassPosition = Vector::ZEROS;
	EulerAngles currentAngles = EulerAngles::ZEROS;
	GetCoordinatesAtTime( m_timer, position, rotation, &centerOfMassPosition, &currentAngles );
	frame->AddDebugSphere( centerOfMassPosition, 0.223f, Matrix::IDENTITY, Color::BLUE, true );

	// current real center of mass
	frame->AddDebugSphere( GetCenterOfMassPosition( entity, m_physicsWrapper.Get() ), 0.2f, Matrix::IDENTITY, Color::YELLOW, true );

	// current computed entity position and center of mass offset in WS
	Vector offsetWS = rotation.ToMatrix().TransformVector( m_physicsWrapper.GetCenterOfMassOffsetLS( m_dropInfo ) );
	frame->AddDebugSphere( position, 0.15f, Matrix::IDENTITY, Color::MAGENTA, true );
	frame->AddDebugLine( position, position + offsetWS, Color::MAGENTA, true );
}

//////////////////////////////////////////////////////////////////////////

CDropPhysicsComponent::SRagdollTask::SRagdollTask()
	: m_animatedComponent( nullptr )
	, m_physicsWrapper( nullptr )
{}

CDropPhysicsComponent::SRagdollTask::SRagdollTask( CAnimatedComponent* animatedComponent )
	: m_animatedComponent( animatedComponent )
	, m_physicsWrapper( nullptr )
{}

Bool CDropPhysicsComponent::SRagdollTask::Update()
{
	if ( m_animatedComponent == nullptr )
	{
		return false;
	}
	if ( m_physicsWrapper == nullptr )
	{
		m_physicsWrapper = m_animatedComponent->GetRagdollPhysicsWrapper();
		if ( m_physicsWrapper == nullptr )
		{
			return true;
		}
	}
	if ( !m_physicsWrapper->IsReady() )
	{
		return true;
	}
	return UpdateRagdoll();
}

CDropPhysicsComponent::SDisableRagdollParts::SDisableRagdollParts()
{
}

CDropPhysicsComponent::SDisableRagdollParts::SDisableRagdollParts( CAnimatedComponent* animatedComponent,
																  const THashSet< String > & allExceptBones )
	: SRagdollTask( animatedComponent )
{
	RED_ASSERT( m_animatedComponent != nullptr );

	CSkeleton* skeleton = m_animatedComponent->GetSkeleton();
	if ( skeleton == nullptr )
	{
		return;
	}

	Uint32 size = skeleton->GetBonesNum();
	// starting from 1 cause we don't want to disable root bone
	for ( Uint32 i = 1; i < size; i++ )
	{
		String boneName = skeleton->GetBoneName( i );
		if ( !allExceptBones.Exist( boneName ) )
		{
			m_boneIndices.PushBack( i );
		}
	}
}

Bool CDropPhysicsComponent::SDisableRagdollParts::UpdateRagdoll()
{
	RED_ASSERT( m_physicsWrapper != nullptr );
	m_physicsWrapper->DisableRagdollParts( m_boneIndices );
	return false;
}

//////////////////////////////////////////////////////////////////////////

CDropPhysicsComponent::SDropPhysicsInfo::SDropPhysicsInfo()
	: m_direction( Vector::ZEROS )
	, m_curveName( CName::NONE )
	, m_flags( 0 )
	, m_trajectoryCurve( nullptr )
	, m_rotationCurve( nullptr )
	, m_initialPosition( Vector::ZERO_3D_POINT )
	, m_initialRotation( EulerAngles::ZEROS )
	, m_initialPositionType( DPIPT_Entity )
	, m_centerOfMassOffsetLS( Vector::ZEROS )
	, m_initialCenterOfMassOffsetWS( Vector::ZEROS )
{
}

//////////////////////////////////////////////////////////////////////////

CDropPhysicsComponent::CDropPhysicsComponent()
{

}

void CDropPhysicsComponent::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );

	if ( property->GetName() == TXT( "dropSetups") )
	{
		// Handle case when curve has been deleted

		CCurveEntity::DeleteEditors( GetLayer()->GetWorld(), false );
	}
}

void CDropPhysicsComponent::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );

	PC_SCOPE_PIX( CDropPhysicsComponent_OnAttached );

	// Set up curves parent so the curves calculations can use it when determining absolute curves coordinates
	CEntity* parentEntity = GetEntity();
	RED_ASSERT( parentEntity != nullptr );
	TDropSetups::iterator it = m_dropSetups.Begin();
	TDropSetups::iterator itEnd = m_dropSetups.End();
	for ( ; it != itEnd; ++it )
	{
		if ( *it != nullptr )
		{
			(*it)->AttachCurves( parentEntity );
		}
	}

	m_droppedEntities.Clear();

	world->GetTickManager()->AddToGroup( this, TICK_Main );	
	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_Dismemberment );
}

void CDropPhysicsComponent::OnDetached( CWorld* world )
{
#ifndef NO_EDITOR

	// Remove editable curves
	if ( GIsEditor )
	{
		TDropSetups::iterator it = m_dropSetups.Begin();
		TDropSetups::iterator itEnd = m_dropSetups.End();
		for ( ; it != itEnd; ++it )
		{
			if ( *it != nullptr )
			{
				(*it)->DeleteCurveEditors( world );
			}
		}
	}

#endif

	TDynArray< SDroppedEntity >::iterator itEnd = m_droppedEntities.End();
	for ( TDynArray< SDroppedEntity >::iterator it = m_droppedEntities.Begin(); it != itEnd; ++it )
	{
		it->OnBaseComponentDetached();
	}
	m_droppedEntities.Clear();

	m_ragdollTasks.ClearPtr();

	world->GetTickManager()->RemoveFromGroup( this, TICK_Main );	
	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_Dismemberment );
	
	TBaseClass::OnDetached( world );
}

void CDropPhysicsComponent::OnTick( Float timeDelta )
{
	UpdateDroppedEntities( timeDelta );
	UpdateRagdollTasks();
}

void CDropPhysicsComponent::UpdateDroppedEntities( Float timeDelta )
{
	Vector baseEntityWorldPos = GetEntity()->GetWorldPosition();
	Uint32 index = 0;
	while ( index < m_droppedEntities.Size() )
	{
		SDroppedEntity& entity = m_droppedEntities[ index ];
		if ( entity.Update( baseEntityWorldPos, timeDelta ) )
		{
			index++;
		}
		else
		{
			m_droppedEntities.RemoveAt( index );
		}
	}
}

void CDropPhysicsComponent::UpdateRagdollTasks()
{
	Uint32 index = 0;
	while ( index < m_ragdollTasks.Size() )
	{
		if ( !m_ragdollTasks[ index ]->Update() )
		{
			delete m_ragdollTasks[ index ];
			m_ragdollTasks.RemoveAt( index );
		}
		else
		{
			index++;
		}
	}
}

Bool CDropPhysicsComponent::DropMeshByName( const String& name, const SDropPhysicsInfo& dropInfo )
{
	TDynArray< CMeshComponent* > meshes;
	for ( ComponentIterator< CMeshComponent > it( GetEntity() ) ; it ; ++it )
	{
		if ( (*it)->GetName() == name )
		{
			meshes.PushBack( *it );
		}
	}
	Bool res = false;
	TDynArray< CMeshComponent* >::iterator itEnd = meshes.End();
	for ( TDynArray< CMeshComponent* >::iterator it = meshes.Begin(); it != itEnd; ++it )
	{
		res |= DetachAndDropMesh( *it, dropInfo );
	}
	return res;
}

Bool CDropPhysicsComponent::DropMeshByTag( const CName& tag, const SDropPhysicsInfo& dropInfo )
{
	TDynArray< CMeshComponent* > meshes;
	for ( ComponentIterator< CRigidMeshComponent > it( GetEntity() ) ; it ; ++it )
	{
		if ( (*it)->GetTags().HasTag( tag ) )
		{
			meshes.PushBack( *it );
		}
	}
	Bool res = false;
	TDynArray< CMeshComponent* >::iterator itEnd = meshes.End();
	for ( TDynArray< CMeshComponent* >::iterator it = meshes.Begin(); it != itEnd; ++it )
	{
		res |= DetachAndDropMesh( *it, dropInfo );
	}
	return res;
}

Bool CDropPhysicsComponent::DropExternalEntity( CEntity* entity, const SDropPhysicsInfo& dropInfo )
{
	RED_ASSERT( entity != nullptr );

	UpdateCurves( dropInfo );

	m_droppedEntities.PushBack( SDroppedEntity( entity, dropInfo ) );
	return true;
}

Bool CDropPhysicsComponent::DetachAndDropMesh( CMeshComponent* meshComponent, const SDropPhysicsInfo& dropInfo )
{
	RED_ASSERT( meshComponent != nullptr );
	RED_ASSERT( meshComponent->GetParent()->IsA< CEntity >() );

	CEntity* baseEntity = meshComponent->GetEntity();
	if ( baseEntity == nullptr )
	{
		return false;
	}

	CPhysicsWrapperInterface* physicsInterface = meshComponent->GetPhysicsRigidBodyWrapper();

	// initializing data for dropping entity
	dropInfo.m_initialPosition = ( physicsInterface != nullptr ) ? GetCenterOfMassPosition( baseEntity, physicsInterface ) : meshComponent->GetWorldPositionRef();
	dropInfo.m_initialRotation = meshComponent->GetWorldRotation();
	dropInfo.m_initialPositionType = SDropPhysicsInfo::DPIPT_CenterOfMass;

	// spawning new entity
	EntitySpawnInfo spawnInfo;
	spawnInfo.m_spawnPosition = dropInfo.m_initialPosition;
	spawnInfo.m_spawnRotation = dropInfo.m_initialRotation;
	CEntity* newEntity = GetWorld()->GetDynamicLayer()->CreateEntitySync( spawnInfo );
	if ( newEntity == nullptr )
	{
		return false;
	}

	// move mesh component to the new entity
	newEntity->MoveComponent( meshComponent );

	// reset mesh local transform
	meshComponent->SetPosition( Vector::ZEROS );
	meshComponent->SetRotation( EulerAngles::ZEROS );
	meshComponent->ForceUpdateTransformNodeAndCommitChanges();

	//newEntity->Attach( meshComponent, HardAttachmentSpawnInfo() );

	DropExternalEntity( newEntity, dropInfo );
	CreateParticles( dropInfo );

	return true;
}

Bool CDropPhysicsComponent::DisableRagdollParts( const SDisableRagdollInfo& info )
{
	CEntity* baseEntity = ( info.m_baseEntity != nullptr ) ? info.m_baseEntity : GetEntity();
	CAnimatedComponent* baseAnimatedComponent = baseEntity->GetRootAnimatedComponent();
	CAnimatedComponent* spawnedAnimatedComponent = info.m_spawnedEntity->GetRootAnimatedComponent();
	if ( baseAnimatedComponent == nullptr || spawnedAnimatedComponent == nullptr )
	{
		return false;
	}


	SMappedBones localCollectedBones;
	SMappedBones* spawnedEntityBones = info.m_spawnedEntityBones;
	if ( spawnedEntityBones == nullptr )
	{
		CollectMappedBones( spawnedAnimatedComponent, localCollectedBones, info.m_fixSpawnedBonesHierarchyType );
		spawnedEntityBones = &localCollectedBones;
	}

	// We add two tasks (cause probably ragdolls aren't ready yet).
	// First one will disable base character's ragdoll parts that are contained in spawned entity mesh' bones list.
	// Second one will disable spawned entity ragdoll parts that are not contained in mesh' bones list.
	if ( info.m_type == DRT_Base || info.m_type == DRT_Both )
	{
		// We need to get a list of bones that need to be left in base character
		// and then fix this list (cause spawned entity may contain some bones
		// that are important for base ragdoll hierarchy)
		SMappedBones baseEntityBones;
		for ( const String& bone : info.m_baseEntityBonesToSkip )
		{
			baseEntityBones.m_bones.Insert( bone );
		}
		CSkeleton* baseSkeleton = baseAnimatedComponent->GetSkeleton();
		if ( baseSkeleton != nullptr )
		{
			const Int32 bonesCount = baseSkeleton->GetBonesNum();
			for ( Int32 i = 0; i < bonesCount; i++ )
			{
				String boneName = baseSkeleton->GetBoneName( i );
				if ( !spawnedEntityBones->m_bones.Exist( boneName ) )
				{
					baseEntityBones.m_bones.Insert( boneName );
				}
			}
			FixMappedBonesHierarchy( baseAnimatedComponent, baseEntityBones, nullptr, info.m_fixBaseBonesHierarchyType );
		}
		m_ragdollTasks.PushBackUnique( new SDisableRagdollParts( baseAnimatedComponent, baseEntityBones.m_bones ) );
	}
	if ( info.m_type == DRT_Spawned || info.m_type == DRT_Both )
	{
		m_ragdollTasks.PushBackUnique( new SDisableRagdollParts( spawnedAnimatedComponent, spawnedEntityBones->m_bones ) );
	}

	return true;
}

void CDropPhysicsComponent::CollectMappedBones( CAnimatedComponent* animatedComponent, CDropPhysicsComponent::SMappedBones& mappedBones, EFixBonesHierarchyType fixType )
{
	if ( animatedComponent == nullptr )
	{
		return;
	}
	CSkeleton* skeleton = animatedComponent->GetSkeleton();
	if ( skeleton == nullptr )
	{
		return;
	}

	THashSet< String > & bones = mappedBones.m_bones;
	THashSet< Int32 > usedBonesIndicies;

	// We take all used bones from animated meshes.
	const TList< IAttachment* >& childAttachments = animatedComponent->GetChildAttachments();
	TList< IAttachment* >::const_iterator attItEnd = childAttachments.End();
	for ( TList< IAttachment* >::const_iterator it = childAttachments.Begin(); it != attItEnd; ++it )
	{
		CNode* child = ( *it )->GetChild();
		if ( child == nullptr )
		{
			continue;
		}
		CMeshComponent *meshComponent = Cast< CMeshComponent >( child );
		CMesh* mesh = ( meshComponent != nullptr ) ? meshComponent->GetMeshNow() : nullptr;
		if ( mesh == nullptr )
		{
			continue;
		}
		Uint32 size = mesh->GetBoneCount();
		const CName* names = mesh->GetBoneNames();
		for ( Uint32 i = 0; i < size; i++ )
		{
			Int32 boneIndex = skeleton->FindBoneByName( names[ i ] );
			if ( boneIndex != -1 )
			{
				usedBonesIndicies.Insert( boneIndex );
				bones.Insert( names[ i ].AsString() );
			}
		}
	}

	if ( bones.Size() == 0 )
	{
		return;
	}

	FixMappedBonesHierarchy( animatedComponent, mappedBones, &usedBonesIndicies, fixType );
}

void CDropPhysicsComponent::FixMappedBonesHierarchy( CAnimatedComponent* animatedComponent,
												     SMappedBones& mappedBones,
													 THashSet< Int32 > * usedBonesIndicies /* = nullptr */,
													 EFixBonesHierarchyType fixType /* = FBHTAddMissingBones */ )
{
	if ( animatedComponent == nullptr )
	{
		return;
	}
	CSkeleton* skeleton = animatedComponent->GetSkeleton();
	if ( skeleton == nullptr )
	{
		return;
	}
	if ( mappedBones.m_bones.Size() == 0 )
	{
		return;
	}

	THashSet< String > & bones = mappedBones.m_bones;

	// 0. First, let's try to collect mapped bones' indices (if weren't passed as argument)
	THashSet< Int32 > localUsedBonesIndices;
	if ( usedBonesIndicies == nullptr )
	{
		localUsedBonesIndices.Clear();
		THashSet< String >::iterator itEnd = mappedBones.m_bones.End();
		for ( THashSet< String >::iterator it = mappedBones.m_bones.Begin(); it != itEnd; ++it )
		{
			Int32 index = skeleton->FindBoneByName( ( *it ).AsChar() );
			if ( index >= 0 )
			{
				localUsedBonesIndices.Insert( index );
			}
		}
		usedBonesIndicies = &localUsedBonesIndices;
	}

	// 1. We collect "roots" - the bones that don't have parents in the set of collected indices.
	THashMap< Int32, TDynArray< Int32 > > roots;
	THashSet< Int32 >::iterator ubItEnd = usedBonesIndicies->End();
	for ( THashSet< Int32 >::iterator it = usedBonesIndicies->Begin(); it != ubItEnd; ++it )
	{
		Int32 parentIndex = skeleton->GetParentBoneIndex( *it );
		if ( parentIndex == -1 || !usedBonesIndicies->Exist( parentIndex ) )
		{
			roots.Insert( *it, TDynArray< Int32 >() );
		}
	}

	// 2. If there's only one root - everything is connected and there's
	// nothing more we need to do.
	if ( roots.Size() < 2 )
	{
		RED_ASSERT( roots.Size() == 1, TXT( "No roots found in mapped bones hierarchy.") );
		return;
	}

	// if we want to fix hierarchy bya adding missing bones
	if ( fixType == FBHTAddMissingBones )
	{
		// 3. We will consider all the roots as the leaves of the tree we need to find.
		// First, we find a path from every leaf-root to THE root.
		THashMap< Int32, TDynArray< Int32 > >::iterator rootItEnd = roots.End();
		for ( THashMap< Int32, TDynArray< Int32 > >::iterator it = roots.Begin(); it != rootItEnd; ++it )
		{
			TDynArray< Int32 > & path = it->m_second;
			path.PushBack( it->m_first );
			Int32 parentIndex = skeleton->GetParentBoneIndex( it->m_first );
			while ( parentIndex != -1 )
			{
				path.PushBack( parentIndex );
				parentIndex = skeleton->GetParentBoneIndex( parentIndex );
			}
		}

		// 4. Now, we find the lowest common root for all paths.
		THashMap< Int32, TDynArray< Int32 > >::iterator rootIt = roots.Begin();
		TDynArray< Int32 > & firstPath = rootIt->m_second;
		// We will iterate leaves-roots starting from the second one.
		++rootIt;
		const Uint32 size = firstPath.Size();
		RED_ASSERT( size > 0, TXT( "There should be anything on the path to the root, even the bone itself." ) );
		Int32 commonRoot = firstPath[ size - 1 ];
		for ( Uint32 i = 1; i < size; ++i )
		{
			Int32 commonRootCandidate = firstPath[ size - 1 - i ];
			Bool foundInAll = true;
			for ( THashMap< Int32, TDynArray< Int32 > >::iterator it = rootIt; it != rootItEnd; ++it )
			{
				if ( i >= it->m_second.Size() || it->m_second[ it->m_second.Size() - 1 - i ] != commonRootCandidate )
				{
					foundInAll = false;
					break;
				}
			}
			if ( foundInAll )
			{
				commonRoot = commonRootCandidate;
			}
			else
			{
				break;
			}
		}

		// 5. Finally, we add common root and all bones from paths up to the common root.
		bones.Insert( skeleton->GetBoneName( commonRoot ) );
		usedBonesIndicies->Insert( commonRoot );
		for ( THashMap< Int32, TDynArray< Int32 > >::iterator it = roots.Begin(); it != rootItEnd; ++it )
		{
			// The only special situation we need to deal with
			// is when one of the leaves is the common root.
			if ( it->m_first == commonRoot )
			{
				// it's already added to the tree (and list of used bones)
				continue; 
			}
			Int32 parentIndex = skeleton->GetParentBoneIndex( it->m_first );
			while ( !usedBonesIndicies->Exist( parentIndex ) )
			{
				if ( parentIndex == -1 )
				{
					RED_ASSERT( parentIndex != -1, TXT( "There should be a common root in the skeleton - is it really disconnected?" ) );
					break;
				}
				bones.Insert( skeleton->GetBoneName( parentIndex ) );
				usedBonesIndicies->Insert( parentIndex );
				parentIndex = skeleton->GetParentBoneIndex( parentIndex );
			}
		}
	}
	// the other fix type would be to remove all disjoint roots which are not main 'Root' bone.
	else // ( fixType == FBHTRemoveDisconnectedBones )
	{
		RED_ASSERT( roots.KeyExist( 0 ), TXT( "Root bone needs to be mapped if you want to fix hierarchy bones by FBHTRemoveDisconnectedBones" ) );

		// 3. Mark each bone either connected or disconnected to the 'Root'
		const Uint32 bonesCount = static_cast< Uint32 >( skeleton->GetBonesNum() );
		TDynArray< Uint8 > connectedBones( bonesCount );
		TDynArray< Uint8 > disconnectedBones( bonesCount ) ;
		memset( connectedBones.Data(), 0, bonesCount );
		memset( disconnectedBones.Data(), 0, bonesCount );
		// 'Root' bone is obviously connected to the 'Root'
		connectedBones[ 0 ] = 1;
		for ( Int32 i = 1; i < static_cast< Int32 >( bonesCount ); i++ )
		{
			if ( roots.KeyExist( i ) )
			{
				disconnectedBones[ i ] = 1;
			}
			else
			{
				Int32 parentIndex = skeleton->GetParentBoneIndex( i );
				RED_ASSERT( parentIndex != -1, TXT( "All bones but Root should have parent" ) );
				if ( connectedBones[ parentIndex ] )
				{
					connectedBones[ i ] = 1;
				}
				else if ( disconnectedBones[ parentIndex ] )
				{
					disconnectedBones[ i ] = 1;
				}
				else
				{
					RED_ASSERT( false, TXT( "Parent node should be already marked as either connected or disconnected" ) );
				}
			}
		}

		// 4. Finally, remove used disconnected bones.
		THashSet< Int32 >::iterator itEnd = usedBonesIndicies->End();
		for ( THashSet< Int32 >::iterator it = usedBonesIndicies->Begin(); it != itEnd; ++it )
		{
			if ( disconnectedBones[ *it ] )
			{
				bones.Erase( skeleton->GetBoneName( static_cast< Uint32 >( *it ) ) );
			}
		}
	}
}

CDropPhysicsSetup* CDropPhysicsComponent::GetDropSetup( const CName& setupName ) const
{
	TDropSetups::const_iterator itEnd = m_dropSetups.End();
	for ( TDropSetups::const_iterator it = m_dropSetups.Begin(); it != itEnd; ++it )
	{
		if ( *it != nullptr && (*it)->GetName() == setupName )
		{
			return *it;
		}
	}
	return nullptr;
}

Bool CDropPhysicsComponent::UpdateCurves( const SDropPhysicsInfo& dropInfo ) const
{
	CDropPhysicsSetup* dropSetup = GetDropSetup( dropInfo.m_curveName );
	if ( dropSetup != nullptr )
	{
		SDropPhysicsCurves *curves = dropSetup->SelectRandomCurves();
		if ( curves != nullptr )
		{
			dropInfo.m_trajectoryCurve = &curves->m_trajectory;
			dropInfo.m_rotationCurve = &curves->m_rotation;
			return true;
		}
	}
	return false;
}

Bool CDropPhysicsComponent::CreateParticles( const SDropPhysicsInfo& dropInfo ) const
{
	const CDropPhysicsSetup* dropSetup = GetDropSetup( dropInfo.m_curveName );
	if ( dropSetup == nullptr )
	{
		return false;
	}

	CParticleSystem* particles = dropSetup->m_particles.Get();
	if ( particles == nullptr )
	{
		return false;
	}

	EntitySpawnInfo psInfo;
	psInfo.m_spawnPosition = dropInfo.m_initialPosition;
	CEntity* particlesEntity = GetWorld()->GetDynamicLayer()->CreateEntitySync( psInfo );
	if ( particlesEntity == nullptr )
	{
		return false;

	}

	CParticleComponent* pc = Cast< CParticleComponent >( particlesEntity->CreateComponent( ClassID< CParticleComponent >(), SComponentSpawnInfo() ) );
	particlesEntity->Attach( pc, HardAttachmentSpawnInfo() );
	pc->ForceUpdateTransformNodeAndCommitChanges();
	pc->SetParticleSystem( particles );
	pc->RefreshRenderProxies();

	return true;
}

Vector CDropPhysicsComponent::GetCenterOfMassPosition( const CEntity* entity, const CPhysicsWrapperInterface* physicsInterface,
													   Uint32 defaultActorIndex /* = 0 */,
													   THashSet< String > * usedBones /* = nullptr */ )
{
	if ( entity != nullptr && physicsInterface != nullptr )
	{
		CAnimatedComponent* ac = entity->GetRootAnimatedComponent();
		if ( ac != nullptr && ac->HasRagdoll() && ac->GetRagdollPhysicsWrapper() == physicsInterface )
		{
			CSkeleton* skeleton = ac->GetSkeleton();
			if ( skeleton != nullptr )
			{
				Float totalMass = 0.0f;
				Vector center = Vector::ZEROS;
				const Uint32 size = skeleton->GetBonesNum();
				for ( Uint32 i = 0; i < size; i++ )
				{
					if ( usedBones != nullptr && !usedBones->Exist( skeleton->GetBoneName( i ) ) )
					{
						continue;
					}
					Int32 actorIndex = const_cast< CPhysicsWrapperInterface* >( physicsInterface )->GetIndex( skeleton->GetBoneNameAnsi( i ) );
					if ( actorIndex >= 0 )
					{
#ifdef USE_PHYSX
						physx::PxActor* actor = ( physx::PxActor* ) physicsInterface->GetActor( actorIndex );
						if ( actor != nullptr && actor->getScene() != nullptr )
						{
							const Float actorMass = physicsInterface->GetMass( actorIndex );
							const Vector actorCenterOfMass = physicsInterface->GetCenterOfMassPosition( actorIndex );
							totalMass += actorMass;
							center += ( actorCenterOfMass * actorMass );
						}
#endif
					}
				}
				if ( totalMass > NumericLimits< Float >::Epsilon() )
				{
					center *= ( 1.0f / totalMass );
					center.W = 1.0f;
					return center;
				}
			}
		}
	}
	if ( physicsInterface != nullptr )
	{
		return physicsInterface->GetCenterOfMassPosition( defaultActorIndex );
	}
	if ( entity != nullptr )
	{
		return entity->GetWorldPosition();
	}
	return Vector::ZEROS;
}

Bool CDropPhysicsComponent::GetCenterOfMassOffsets( const CEntity* entity, const CPhysicsWrapperInterface* physicsInterface, Vector& centerOfMassOffsetLS, Vector& centerOfMassOffsetWS,
													Uint32 actorIndex /* = 0 */, THashSet< String > * usedBones /* = nullptr */, EulerAngles* relativeRotation /* = nullptr */ )
{
	if ( entity != nullptr && physicsInterface != nullptr )
	{
		if ( !const_cast< CPhysicsWrapperInterface* >( physicsInterface )->IsReady() )
		{
			return false;
		}

		const Matrix entityLocalToWorld = entity->GetLocalToWorld();
		const Vector centerOfMassWS = GetCenterOfMassPosition( entity, physicsInterface, actorIndex, usedBones );
		const Vector entityPositionWS = entityLocalToWorld.GetTranslation();
		centerOfMassOffsetWS = centerOfMassWS - entityPositionWS;

		centerOfMassOffsetLS.X = centerOfMassOffsetWS.Dot3( entityLocalToWorld.GetAxisX() );
		centerOfMassOffsetLS.Y = centerOfMassOffsetWS.Dot3( entityLocalToWorld.GetAxisY() );
		centerOfMassOffsetLS.Z = centerOfMassOffsetWS.Dot3( entityLocalToWorld.GetAxisZ() );
		centerOfMassOffsetLS.W = 0.0f;

		// if we need another rotation in global space than the "current one"
		if ( relativeRotation != nullptr )
		{
			const Matrix rotation = relativeRotation->ToMatrix();
			centerOfMassOffsetWS = rotation.TransformVector( centerOfMassOffsetLS );
		}
	}

	return true;
}

void CDropPhysicsComponent::funcDropMeshByName( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, name, String::EMPTY );
	GET_PARAMETER_OPT( Vector, direction, Vector::ZEROS );
	GET_PARAMETER_OPT( CName, curveName, CName::NONE );
	FINISH_PARAMETERS;
	Bool res = false;
	if ( name != String::EMPTY )
	{
		SDropPhysicsInfo dropInfo;
		dropInfo.m_direction = direction;
		dropInfo.m_curveName = curveName;
		res = DropMeshByName( name, dropInfo );
	}
	RETURN_BOOL( res );
}

void CDropPhysicsComponent::funcDropMeshByTag( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, tag, CName::NONE );
	GET_PARAMETER_OPT( Vector, direction, Vector::ZEROS );
	GET_PARAMETER_OPT( CName, curveName, CName::NONE );
	FINISH_PARAMETERS;
	Bool res = false;
	if ( tag != CName::NONE )
	{
		if ( curveName == CName::NONE )
		{
			curveName = tag;
		}
		SDropPhysicsInfo dropInfo;
		dropInfo.m_direction = direction;
		dropInfo.m_curveName = curveName;
		res = DropMeshByTag( tag, dropInfo );
	}
	RETURN_BOOL( res );
}

void CDropPhysicsComponent::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag )
{
	TBaseClass::OnGenerateEditorFragments( frame, flag );

	TDynArray< SDroppedEntity >::iterator it = m_droppedEntities.Begin();
	TDynArray< SDroppedEntity >::iterator itEnd = m_droppedEntities.End();
	for ( ; it != itEnd; ++it )
	{
		it->DrawDebug( frame, flag );
	}
}

void CDropPhysicsComponent::OnInitialized()
{
	TBaseClass::OnInitialized();
	SDropPhysicsComponentModifierManager::GetInstance().DropPhysicsComponentOnInitialized( this );
}

void CDropPhysicsComponent::OnFinalize()
{
	SDropPhysicsComponentModifierManager::GetInstance().DropPhysicsComponentOnFinalize( this );
	TBaseClass::OnFinalize();
}

void CDropPhysicsComponent::OnDetachFromEntityTemplate()
{
	SDropPhysicsComponentModifierManager::GetInstance().DropPhysicsComponentOnFinalize( this );
	TBaseClass::OnDetachFromEntityTemplate();
}

void CDropPhysicsComponent::AddDropPhysicsSetup( CDropPhysicsSetup* dropPhysicsSetup )
{
	m_dropSetups.PushBack( dropPhysicsSetup );
}

void CDropPhysicsComponent::RemoveDropPhysicsSetup( const CName& dropPhysicsSetupName )
{
	TDropSetups::const_iterator endDropSetups = m_dropSetups.End();
	for( TDropSetups::iterator iterDropSetups = m_dropSetups.Begin(); iterDropSetups != endDropSetups; ++iterDropSetups )
	{
		if( iterDropSetups->Get() )
		{
			if( iterDropSetups->Get()->m_name == dropPhysicsSetupName )
			{
				m_dropSetups.Erase( iterDropSetups );
				return;
			}
		}

	}
}