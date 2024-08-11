/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "switchableFoliageComponent.h"
#include "foliageInstance.h"
#include "foliageDynamicInstanceService.h"
#include "baseTree.h"
#include "entity.h"
#include "layer.h"
#include "renderFrame.h"
#include "manualStreamingHelper.h"
#include "../core/scriptStackFrame.h"
#include "../core/factory.h"

class CSwitchableFoliageResourceFactory : public IFactory
{
	DECLARE_ENGINE_CLASS( CSwitchableFoliageResourceFactory, IFactory, 0 );

public:
	CSwitchableFoliageResourceFactory()
	{
		m_resourceClass = ClassID< CSwitchableFoliageResource >();
	}

	CResource* DoCreate( const FactoryOptions& options ) override
	{
		return ::CreateObject< CSwitchableFoliageResource >( options.m_parentObject );		
	}
};

BEGIN_CLASS_RTTI( CSwitchableFoliageResourceFactory )
	PARENT_CLASS( IFactory )
END_CLASS_RTTI()

IMPLEMENT_ENGINE_CLASS( CSwitchableFoliageResourceFactory );

// CSwitchableFoliageResource

IMPLEMENT_ENGINE_CLASS( SSwitchableFoliageEntry );
IMPLEMENT_ENGINE_CLASS( CSwitchableFoliageResource )

Uint32 CSwitchableFoliageResource::GetEntryIndex( CName name )
{
	for ( Uint32 i = 0; i < m_entries.Size(); ++i )
	{
		if ( m_entries[ i ].m_name == name )
		{
			return i;
		}
	}
	return InvalidEntryIndex;
}

CName CSwitchableFoliageResource::GetEntryName( Uint32 index )
{
	return ( index < m_entries.Size() ) ? m_entries[ index ].m_name : CName::NONE;
}

CSRTBaseTree* CSwitchableFoliageResource::GetAsync( Uint32 index )
{
	return ( index < m_entries.Size() && m_entries[ index ].m_tree.GetAsync() == BaseSoftHandle::ALR_Loaded ) ? m_entries[ index ].m_tree.Get() : nullptr;
}

// CSwitchableFoliageComponent

IMPLEMENT_ENGINE_CLASS( CSwitchableFoliageComponent )

CSwitchableFoliageComponent::CSwitchableFoliageComponent(void)
	: m_minimumStreamingDistance( 90 )
	, m_isCreated( false )
	, m_isSuppressed( true )
	, m_entryIndex( CSwitchableFoliageResource::InvalidEntryIndex )
	, m_pendingEntryIndex( CSwitchableFoliageResource::InvalidEntryIndex )
{
}

CSwitchableFoliageComponent::~CSwitchableFoliageComponent(void)
{
}

void CSwitchableFoliageComponent::OnAttached(CWorld* world)
{
	ASSERT( m_entryIndex == CSwitchableFoliageResource::InvalidEntryIndex );
	ASSERT( m_pendingEntryIndex == CSwitchableFoliageResource::InvalidEntryIndex );

	TBaseClass::OnAttached( world );

	const Float distance = ( Float ) GetMinimumStreamingDistance();
	world->GetManualStreamingHelper().Register( this, distance, distance + 5.0f );

	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_Foliage );
}

void CSwitchableFoliageComponent::OnDetached(CWorld* world)
{
	DestroyFoliageInstance();

	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_Foliage );

	world->GetManualStreamingHelper().Unregister( this );

	TBaseClass::OnDetached( world );
}

Bool CSwitchableFoliageComponent::OnCreateEngineRepresentation()
{
	m_isSuppressed = false;
	return CreateFoliageInstance();
}

void CSwitchableFoliageComponent::OnDestroyEngineRepresentation()
{
	m_isSuppressed = true;
	DestroyFoliageInstance();
}

void CSwitchableFoliageComponent::OnDestroyed()
{
	DestroyFoliageInstance();
	TBaseClass::OnDestroyed();
}

void CSwitchableFoliageComponent::SetEntry( CName name )
{
	CSwitchableFoliageResource* resource = m_resource.Get();
	if ( !resource )
	{
		return;
	}

	// Do we need to switch entry at all?

	const Uint32 newPendingEntryIndex = resource->GetEntryIndex( name );
	if ( newPendingEntryIndex == m_pendingEntryIndex )
	{
		return;
	}

	if ( newPendingEntryIndex == m_entryIndex )
	{
		m_pendingEntryIndex = m_entryIndex;
		return;
	}

	// Set new pending entry index

	m_pendingEntryIndex = newPendingEntryIndex;

	// If suppressed, don't do anything

	if ( m_isSuppressed )
	{
		return;
	}

	// Attempt to switch immediately if was created

	if ( m_isCreated )
	{
		// If was created and is to disappear, then destroy immediately

		if ( m_pendingEntryIndex == CSwitchableFoliageResource::InvalidEntryIndex )
		{
			DestroyFoliageInstance();
		}

		// If was created and is to change into different entry, then wait until that new entry is ready (to apply immediate switch)

		else if ( !CreateFoliageInstance() )
		{
			// If we didn't succeed straight away, then make the streaming system aware of it, so it will keep calling OnCreateEngineRepresentation() on us until succeeded 

			GetWorld()->GetManualStreamingHelper().ResetCreateSucceeded( this );
		}
	}
}

CSRTBaseTree* CSwitchableFoliageComponent::GetBaseTree( Uint32 entryIndex )
{
	CSwitchableFoliageResource* resource = m_resource.Get();
	return ( entryIndex != CSwitchableFoliageResource::InvalidEntryIndex && resource ) ? resource->GetAsync( entryIndex ) : nullptr;
}

Bool CSwitchableFoliageComponent::CreateFoliageInstance()
{
	if ( m_isSuppressed || ( m_isCreated && m_pendingEntryIndex == m_entryIndex ) )
	{
		return true;
	}

	// Is tree resource available?

	CSRTBaseTree* baseTree = GetBaseTree( m_pendingEntryIndex );
	if ( !baseTree )
	{
		return false;
	}

	// Destroy old instance

	DestroyFoliageInstance();

	// Create new instance

	if ( !m_foliageController.IsValid() )
	{
		m_foliageController = GetLayer()->GetWorld()->CreateFoliageDynamicInstanceService();
	}

	SFoliageInstance inst;
	inst.SetPosition( GetWorldPosition() );
	inst.SetQuaternion( GetWorldRotation().ToQuat() );
	inst.SetScale( GetLocalToWorld().GetScale33().Z );

	m_foliageController.AddInstance( baseTree, inst );

#ifndef NO_EDITOR
	m_oldInstancePosition = inst.GetPosition();
	m_oldBaseTree = baseTree;
#endif

	m_entryIndex = m_pendingEntryIndex;
	m_isCreated = true;
	return true;
}

void CSwitchableFoliageComponent::DestroyFoliageInstance()
{
	if ( !m_isCreated )
	{
		return;
	}
	m_isCreated = false;

	CSRTBaseTree* baseTree = GetBaseTree( m_entryIndex );
	if ( !baseTree )
	{
		return;
	}

	m_foliageController.RemoveInstance( baseTree, GetWorldPosition() );

#ifndef NO_EDITOR
	if ( m_oldBaseTree )
	{
		m_foliageController.RemoveInstance( m_oldBaseTree, m_oldInstancePosition );
		m_oldBaseTree = nullptr;
	}
#endif

	m_entryIndex = m_pendingEntryIndex;
}

#ifndef NO_EDITOR
void CSwitchableFoliageComponent::EditorOnTransformChanged()
{
	CComponent::EditorOnTransformChanged();
	UpdateInstancePosition();
}

void CSwitchableFoliageComponent::EditorOnTransformChangeStop()
{
	CComponent::EditorOnTransformChangeStop();
	UpdateInstancePosition();
}
#endif

void CSwitchableFoliageComponent::UpdateInstancePosition()
{
#ifndef NO_EDITOR
	if ( m_foliageController.IsValid() && !static_cast<CEntity*>( GetParent() )->IsInGame() )
	{
		DestroyFoliageInstance();
		if ( !CreateFoliageInstance() )
		{
			GetWorld()->GetManualStreamingHelper().ResetCreateSucceeded( this );
		}
	}
#endif
}

void CSwitchableFoliageComponent::OnGenerateEditorFragments(CRenderFrame* frame, EShowFlags flag)
{
	// Handle hit proxy mode
#ifndef NO_COMPONENT_GRAPH
	if ( frame->GetFrameInfo().m_renderingMode == RM_HitProxies )
	{
		if ( CSRTBaseTree* baseTree = GetBaseTree( m_entryIndex ) )
		{
			// Get bounding box

			Matrix localToWorld;
			GetLocalToWorld( localToWorld );

			Box boundingBox = baseTree->GetBBox();
			frame->AddDebugSolidBox( boundingBox, localToWorld, m_hitProxyId.GetColor() );
		}
	}
#endif
}

Uint32 CSwitchableFoliageComponent::GetMinimumStreamingDistance() const
{
	return m_minimumStreamingDistance;
}

void CSwitchableFoliageComponent::funcSetEntry( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, name, CName::NONE );
	FINISH_PARAMETERS;

	SetEntry( name );
	RETURN_VOID();
}
