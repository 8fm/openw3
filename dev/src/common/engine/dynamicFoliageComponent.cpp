/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "dynamicFoliageComponent.h"
#include "foliageInstance.h"
#include "foliageDynamicInstanceService.h"
#include "baseTree.h"
#include "entity.h"
#include "layer.h"
#include "renderFrame.h"

IMPLEMENT_ENGINE_CLASS( CDynamicFoliageComponent )

CDynamicFoliageComponent::CDynamicFoliageComponent(void)
	: m_baseTree( nullptr )
	, m_minimumStreamingDistance( 30 )
#ifndef NO_EDITOR
	, m_oldBaseTree( nullptr )
#endif
{
}

CDynamicFoliageComponent::~CDynamicFoliageComponent(void)
{
}

void CDynamicFoliageComponent::OnAttached(CWorld* world)
{
	TBaseClass::OnAttached( world );
	m_foliageController = world->CreateFoliageDynamicInstanceService();
	CreateFoliageInstance();
	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_Foliage );
}

void CDynamicFoliageComponent::OnDetached(CWorld* world)
{
	DestroyFoliageInstance();
	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_Foliage );
	TBaseClass::OnDetached( world );
}

void CDynamicFoliageComponent::OnDestroyed()
{
	DestroyFoliageInstance();
	TBaseClass::OnDestroyed();
}

void CDynamicFoliageComponent::CreateFoliageInstance()
{
	if( m_foliageController.IsValid() )
	{
		m_foliageController = GetLayer()->GetWorld()->CreateFoliageDynamicInstanceService();
	}

	if( m_baseTree != nullptr )
	{
		SFoliageInstance inst;
		inst.SetPosition( GetWorldPosition() );
		inst.SetQuaternion( GetWorldRotation().ToQuat() );
		inst.SetScale( GetLocalToWorld().GetScale33().Z );

		m_foliageController.AddInstance( m_baseTree.Get(), inst );

#ifndef NO_EDITOR
		m_oldBaseTree = m_baseTree.Get();
		m_instancePosition = inst.GetPosition();
#endif
	}
}

void CDynamicFoliageComponent::DestroyFoliageInstance()
{
	if( m_baseTree != nullptr )
	{
		m_foliageController.RemoveInstance( m_baseTree.Get(), GetWorldPosition() );
	}

#ifndef NO_EDITOR
	if( m_oldBaseTree != nullptr && m_oldBaseTree != m_baseTree )
	{
		m_foliageController.RemoveInstance( m_oldBaseTree, GetWorldPosition() );
	}
#endif
}

#ifndef NO_EDITOR
void CDynamicFoliageComponent::EditorOnTransformChanged()
{
	CComponent::EditorOnTransformChanged();
	UpdateInstancePosition();
	
}

void CDynamicFoliageComponent::EditorOnTransformChangeStop()
{
	CComponent::EditorOnTransformChangeStop();
	UpdateInstancePosition();
}
#endif

void CDynamicFoliageComponent::UpdateInstancePosition()
{

#ifndef NO_EDITOR
	CEntity* parent = static_cast<CEntity*>( GetParent() );

	if( m_baseTree != nullptr && m_foliageController.IsValid() && parent->IsInGame() == false )
	{
		m_foliageController.RemoveInstance( m_baseTree.Get(), m_instancePosition );

		CreateFoliageInstance();
	}

#endif
}

void CDynamicFoliageComponent::OnGenerateEditorFragments(CRenderFrame* frame, EShowFlags flag)
{
	// Handle hit proxy mode
#ifndef NO_COMPONENT_GRAPH
	if ( frame->GetFrameInfo().m_renderingMode == RM_HitProxies )
	{
		// Get bounding box
		if( CSRTBaseTree* baseTree = m_baseTree.Get() )
		{
			Matrix localToWorld;
			GetLocalToWorld( localToWorld );

			Box boundingBox = baseTree->GetBBox();
			frame->AddDebugSolidBox( boundingBox, localToWorld, m_hitProxyId.GetColor() );
		}
	}
#endif
}

Uint32 CDynamicFoliageComponent::GetMinimumStreamingDistance() const
{
	return m_minimumStreamingDistance;
}
