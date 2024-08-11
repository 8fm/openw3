/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "morphedMeshManagerComponent.h"

#include "morphedMeshComponent.h"
#include "../core/scriptStackFrame.h"
#include "curve.h"
#include "world.h"
#include "layer.h"
#include "tickManager.h"
#include "entity.h"


IMPLEMENT_ENGINE_CLASS( CMorphedMeshManagerComponent );

CMorphedMeshManagerComponent::CMorphedMeshManagerComponent()
	: m_morphRatio( 0.0f )
	, m_defaultMorphRatio( 0.0f )
	, m_blendTimeLeft( 0.0f )
	, m_blendTime( 0.0f )
{
}

void CMorphedMeshManagerComponent::SetMorphRatio( Float morphRatio, Bool force )
{
	InternalSetMorphRatio( morphRatio, force );
	// not blending anymore
	m_blendTimeLeft = 0.0f;
	m_blendTime = 0.0f;
}

void CMorphedMeshManagerComponent::ActivateInTickGroups()
{
	if ( CLayer* layer = GetLayer() )
	{
		CWorld* world = layer->GetWorld();
		if ( world && world->GetTickManager() )
		{
			if ( ! m_addedToTickGroups )
			{
				m_addedToTickGroups = true;
				world->GetTickManager()->AddToGroupDelayed( this, TICK_Main );
			}
		}
	}
}

void CMorphedMeshManagerComponent::DeactivateInTickGroups()
{
	if ( CLayer* layer = GetLayer() )
	{
		CWorld* world = layer->GetWorld();
		if ( world && world->GetTickManager() )
		{
			if ( m_addedToTickGroups )
			{
				m_addedToTickGroups = false;
				world->GetTickManager()->RemoveFromGroupDelayed( this, TICK_Main );
			}
		}
	}
}


void CMorphedMeshManagerComponent::OnMorphedMeshAttached( CMorphedMeshComponent* component )
{
	RED_ASSERT( component != nullptr, TXT("Attaching null morphed component to manager") );
	if ( component != nullptr )
	{
		if ( m_morphedMeshComponents.Insert( component ) )
		{
			component->SetMorphRatio( m_morphRatio );
		}
	}
}

void CMorphedMeshManagerComponent::OnMorphedMeshDetached( CMorphedMeshComponent* component )
{
	RED_ASSERT( component != nullptr, TXT("Detaching null morphed component from manager") );
	if ( component != nullptr )
	{
		m_morphedMeshComponents.Erase( component );
	}
}


void CMorphedMeshManagerComponent::SetMorphBlend( Float morphTarget, Float blendTime )
{
	Float currentBlendThrough = m_blendTime != 0.0f? m_blendTimeLeft / m_blendTime : 0.0f;
	m_blendingTo = morphTarget < 0.5f? 0.0f : 1.0f;
	m_blendTime = Min( 10.0f, blendTime ); // NOTE: disallow longer blends
	m_blendTimeLeft = (1.0f - currentBlendThrough) * m_blendTime; // how far are we
	if ( m_blendTime == 0.0f )
	{
		SetMorphRatio( m_blendingTo );
	}
	else
	{
		ActivateInTickGroups();
	}
}

void CMorphedMeshManagerComponent::InternalSetMorphRatio( Float morphRatio, Bool force )
{
	if ( m_morphRatio != morphRatio || force )
	{
		m_morphRatio = morphRatio;

		// update all mesh components
		for ( auto& mmc : m_morphedMeshComponents )
		{
			RED_ASSERT( mmc.Get() != nullptr, TXT("Null cached morphed mesh component") );
			if ( CMorphedMeshComponent* component = mmc.Get() )
			{
				component->SetMorphRatio( m_morphRatio );
			}
		}
	}
}

void CMorphedMeshManagerComponent::OnTick( Float timeDelta )
{
	TBaseClass::OnTick( timeDelta );

	if ( m_blendTimeLeft > 0.0f && m_blendTime > 0.0f )
	{
		m_blendTimeLeft = Max( 0.0f, m_blendTimeLeft - timeDelta );
		const Float atTarget = 1.0f - m_blendTimeLeft / m_blendTime;
		if ( m_blendTimeLeft == 0.0f )
		{
			InternalSetMorphRatio( m_blendingTo ); // just force to 0 or 1
			DeactivateInTickGroups();
		}
		else
		{
			Float morphRatio = m_blendingTo * atTarget + ( 1.0f - m_blendingTo ) * ( 1.0f - atTarget );
			if ( CCurve* morphCurve = m_morphCurve.Get() )
			{
				morphRatio = morphCurve->GetFloatValue( morphRatio * morphCurve->GetDuration() );
			}
			InternalSetMorphRatio( morphRatio );
		}
	}
}

void CMorphedMeshManagerComponent::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );

	PC_SCOPE_PIX( CMorphedMeshManagerComponent_OnAttached );

	// In case we are being attached _after_ any morphed mesh components, we'll scan the entity for any already attached ones. These
	// would have failed to register with the manager.
	if ( CEntity* entity = GetEntity() )
	{
		for ( ComponentIterator< CMorphedMeshComponent > iter( entity ); iter; ++iter )
		{
			CMorphedMeshComponent* morphedComp = *iter;
			if ( morphedComp->IsAttached() )
			{
				OnMorphedMeshAttached( morphedComp );
			}
		}
	}

	SetMorphRatio( m_defaultMorphRatio, true );
}

void CMorphedMeshManagerComponent::OnSerialize( IFile& file )
{
	TBaseClass::OnSerialize( file );
}

void CMorphedMeshManagerComponent::OnPostLoad()
{
	m_morphRatio = m_defaultMorphRatio;
	CreateMorphCurveIfNonExistent();
}

void CMorphedMeshManagerComponent::OnSpawned( const SComponentSpawnInfo& spawnInfo )
{
	TBaseClass::OnSpawned( spawnInfo );

	CreateMorphCurveIfNonExistent();
}

void CMorphedMeshManagerComponent::OnPropertyPostChange( IProperty* property )
{
#ifndef NO_EDITOR
	if ( property->GetName() == TXT("testBlend") )
	{
		SetMorphBlend( m_testBlend? 1.0f : 0.0f, 2.0f );
	}
#endif

	if ( property->GetName() == TXT("defaultMorphRatio") )
	{
		m_defaultMorphRatio = m_defaultMorphRatio < 0.5? 0.0f : 1.0f;
		InternalSetMorphRatio( m_defaultMorphRatio );
	}

	if ( property->GetName() == TXT("morphRatio") )
	{
		InternalSetMorphRatio( m_morphRatio, true );
	}

	TBaseClass::OnPropertyPostChange( property );

	CreateMorphCurveIfNonExistent();
}

void CMorphedMeshManagerComponent::CreateMorphCurveIfNonExistent()
{
	if ( ! m_morphCurve.Get() )
	{
		m_morphCurve = CreateObject< CCurve >( this );
		m_morphCurve->GetCurveData().AddPoint( 0.f, 0.f );
		m_morphCurve->GetCurveData().AddPoint( 1.f, 1.f );
	}
}

void CMorphedMeshManagerComponent::funcSetMorphBlend( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, morphRatio, 0.0f );
	GET_PARAMETER( Float, blendTime, 0.0f );
	FINISH_PARAMETERS;
	SetMorphBlend( morphRatio, blendTime );
}

void CMorphedMeshManagerComponent::funcGetMorphBlend( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	
	RETURN_FLOAT( m_morphRatio );
}

