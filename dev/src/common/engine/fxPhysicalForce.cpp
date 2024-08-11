/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "fxphysicalforce.h"
#include "fxTrackItemGlobalSpacePhysicalForce.h"
#include "dynamicCollisionCollector.h"
#include "phantomComponent.h"
#include "../physics/physicsWrapper.h"
#include "destructionSystemComponent.h"
#include "hardAttachment.h"
#include "destructionComponent.h"

IMPLEMENT_ENGINE_CLASS( IFXPhysicalForce );
IMPLEMENT_ENGINE_CLASS( CFXExplosionImplosionPhysicalForce );
IMPLEMENT_RTTI_ENUM( EFieldType );

TDynArray< THandle< CForceFieldEntity > > CForceFieldEntity::m_elements;
IMPLEMENT_ENGINE_CLASS( CForceFieldEntity );

IFXPhysicalForce::IFXPhysicalForce()
	: m_fieldType( FT_Const )
	, m_radius( 5.0f )
	, m_simulateLocalyInEntity( false )
{
}

CPhantomComponent* IFXPhysicalForce::OnSpawn( CForceFieldEntity* entity, CComponent* component, const CFXTrackItemGlobalSpacePhysicalForce* trackItem, const CFXState& fxState ) const
{
	CPhantomComponent* pc = Cast< CPhantomComponent >( entity->CreateComponent( ClassID< CPhantomComponent >(), SComponentSpawnInfo() ) );
	pc->SetParent( entity );

	entity->m_parentEntityTemplate = component->GetEntity();

	HardAttachmentSpawnInfo attachmentInfo;
	CPhantomComponent* orginPhantomComponent = Cast< CPhantomComponent >( component );
	if( orginPhantomComponent )
	{
		component->Attach( pc, attachmentInfo );
		pc->Fill( orginPhantomComponent );
	}
	else
	{
		component->Attach( pc, attachmentInfo );
		pc->DefineManualy( PS_Sphere, Vector( m_radius, m_radius, m_radius ) );

	}
	return pc;
}

CFXExplosionImplosionPhysicalForce::CFXExplosionImplosionPhysicalForce()
	: m_forceScale( 1.0f ),
	m_applyFractureDamage( 100.0f )
{
	m_parameterNames.PushBack( TXT( "Force" ) );
}

void CFXExplosionImplosionPhysicalForce::OnTick( CForceFieldEntity* entity, const TDynArray< STriggeringInfo >& triggeringInfo, const CFXTrackItemGlobalSpacePhysicalForce* trackItem, const CFXState& fxState, Float timeDelta ) const
{

	if( !timeDelta ) return;
    
	entity->m_currentForce = trackItem->GetCurveValue( fxState.GetCurrentTime() );

	Uint32 triggeredCount = triggeringInfo.Size();
	if( !triggeredCount )
        return;

	if( entity->m_currentForce == 0.0f )
        return;

	CPhantomComponent* triggerComponent = entity->m_phantomComponent.Get();
	if( !triggerComponent ) return;

	CPhysicsWrapperInterface* triggerWrapper = triggerComponent->GetPhysicsRigidBodyWrapper();
	if( !triggerWrapper ) return;

	Vector triggerPosition = triggerWrapper->GetPosition();

	CEntity* parentEntity = m_simulateLocalyInEntity ? entity->m_parentEntityTemplate.Get() : 0;

	for( Uint32 i = 0; i != triggeredCount; ++i )
	{
		const STriggeringInfo& info = triggeringInfo[ i ];

		IScriptable* triggeredScriptable = info.m_triggeredObject.Get();
		if( !triggeredScriptable ) continue;

		CComponent* triggeredComponent = Cast< CComponent >( triggeredScriptable );
		if( !triggeredComponent ) continue;

		if( parentEntity )
		{
			if( parentEntity != triggeredComponent->GetEntity() )
			{
				continue;
			}
		}

		CPhysicsWrapperInterface* triggeredWrapper = triggeredComponent->GetPhysicsRigidBodyWrapper();
		if( !triggeredWrapper ) continue;

		Uint16 triggeredActorIndex = info.m_triggeredBodyIndex.m_actorIndex;

		Float mass = triggeredWrapper->GetMass( triggeredActorIndex );
		if ( mass == 0.0f )
		{
			continue;
		}

		Vector triggeredPosition = triggeredWrapper->GetCenterOfMassPosition( triggeredActorIndex );

		float radius = m_radius;
		radius += triggeredWrapper->GetDiagonal( triggeredActorIndex );
		Vector forceVector = Process( entity, triggeredPosition, radius );
		if( forceVector == Vector::ZERO_3D_POINT ) 
		{
			continue;
		}

		forceVector *= m_forceScale;
		triggeredWrapper->ApplyForce( forceVector, triggerPosition, triggeredActorIndex );

#ifdef USE_APEX
		CDestructionSystemComponent* destruction = Cast< CDestructionSystemComponent >( triggeredComponent );
		if( destruction ) 
		{
			CApexDestructionWrapper* wrapper = destruction->GetDestructionBodyWrapper();
			wrapper->ApplyFractureByVolume( triggeredActorIndex, m_applyFractureDamage, triggerComponent );
		}
#endif
		CDestructionComponent* destructionCmp = Cast< CDestructionComponent >( triggeredComponent );
		if( destructionCmp )
		{
			CPhysicsDestructionWrapper* wrapperDest = destructionCmp->GetDestructionBodyWrapper();
			wrapperDest->ApplyFractureByVolume( triggeredActorIndex, m_applyFractureDamage, triggerComponent, forceVector );
		}
	}
}

Vector CFXExplosionImplosionPhysicalForce::Process( CForceFieldEntity* entity, const Vector& targetPosition, float radius ) const
{
	if( entity->m_currentForce == 0.0f ) return Vector::ZERO_3D_POINT;

	CPhantomComponent* triggerComponent = entity->m_phantomComponent.Get();
	if( !triggerComponent ) return Vector::ZERO_3D_POINT;

	Vector triggerPosition = triggerComponent->GetLocalToWorld().GetTranslationRef();

	Vector diff = targetPosition - triggerPosition;
	Float mag = diff.Mag3();
	if( mag == 0.0f ) return Vector::ZERO_3D_POINT;

	if( mag > radius ) return Vector::ZERO_3D_POINT;

	if( diff.Z <= 0.0f )
	{
		diff.Z = 0.0f;
	}
	diff.Normalize3();

	Float resultForce = entity->m_currentForce;

	EFieldType fieldType = m_fieldType;
	switch ( fieldType )
	{
	case FT_Const:
		break;
	case FT_Linear:
		resultForce = entity->m_currentForce * ( 1.0f - (mag / radius) );
		break;
	case FT_Square:
		Float d = (mag / radius);
		resultForce = entity->m_currentForce * ( 1.0f - d*d );
		break;
	}

	Vector forceVector = Vector::Mul4( diff, resultForce );
	return forceVector;

}



IMPLEMENT_ENGINE_CLASS( CFXFractureDesctruction );


CFXFractureDesctruction::CFXFractureDesctruction()
{
	m_parameterNames.PushBack( TXT( "Fracture Desctruction" ) );
}

void CFXFractureDesctruction::OnTick( CForceFieldEntity* entity, const TDynArray< STriggeringInfo >& triggeringInfo, const CFXTrackItemGlobalSpacePhysicalForce* trackItem, const CFXState& fxState, Float timeDelta ) const
{
	if( !timeDelta ) return;

	Uint32 triggeredCount = triggeringInfo.Size();

	if( !triggeredCount ) return;

	entity->m_currentForce = trackItem->GetCurveValue( fxState.GetCurrentTime() );

#ifdef USE_APEX
	if( entity->m_currentForce == 0.0f ) return;

	CPhantomComponent* triggerComponent = entity->m_phantomComponent.Get();
	if( !triggerComponent ) return;

	CPhysicsWrapperInterface* triggerWrapper = triggerComponent->GetPhysicsRigidBodyWrapper();
	if( !triggerWrapper ) return;

	CEntity* parentEntity = m_simulateLocalyInEntity ? entity->m_parentEntityTemplate.Get() : 0;

	for( Uint32 i = 0; i != triggeredCount; ++i )
	{
		const STriggeringInfo& info = triggeringInfo[ i ];

		IScriptable* triggeredScriptable = info.m_triggeredObject.Get();
		if( !triggeredScriptable ) continue;

		if( parentEntity )
		{
			CComponent* triggeredComponent = Cast< CComponent >( triggeredScriptable );
			if( !triggeredComponent || parentEntity != triggeredComponent->GetEntity() )
			{
				continue;
			}
		}

		CDestructionSystemComponent* destruction = Cast< CDestructionSystemComponent >( triggeredScriptable );
		if( !destruction ) continue;

		CApexDestructionWrapper* wrapper = destruction->GetDestructionBodyWrapper();
		wrapper->ApplyFracture();
	}
#endif
}

