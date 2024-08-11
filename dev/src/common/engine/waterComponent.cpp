/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "waterComponent.h"
#include "globalWater.h"
#include "../core/dataError.h"
#include "game.h"
#include "world.h"
#include "entity.h"
#include "layer.h"
#include "utils.h"


IMPLEMENT_ENGINE_CLASS( CWaterComponent );

CWaterComponent::CWaterComponent()
	:m_world(nullptr)
	, m_transformNeedsUpdate( false )
{
}

void CWaterComponent::OnAttached( CWorld* world )
{
	ASSERT( world );		
	m_world = world;

	TBaseClass::OnAttached( world );
	NotifyGlobalWater( false );
}

void CWaterComponent::OnDetached( CWorld* world )
{
	ASSERT( world );	
	// Base detach
	TBaseClass::OnDetached( world );	
	NotifyGlobalWater( false );
}

void CWaterComponent::OnPropertyPostChange( CProperty* prop )
{
	TBaseClass::OnPropertyPostChange( prop );	
	NotifyGlobalWater( true );
}

void CWaterComponent::OnUpdateTransformComponent( SUpdateTransformContext& context, const Matrix& prevLocalToWorld )
{
	TBaseClass::OnUpdateTransformComponent( context, prevLocalToWorld );
	m_transformNeedsUpdate = true;
}

void CWaterComponent::NotifyGlobalWater( Bool propertyChanged )
{
	if( m_world && m_world->IsWaterShaderEnabled() )
	{
		CGlobalWater* gw = m_world->GetGlobalWater();
		if( gw != nullptr )
		{			
			gw->OnLocalShapeAttached( this, propertyChanged );
		}
	}
}

void CWaterComponent::HandleOldFileVersion() const
{
	RED_HALT(  "CWaterComponent data is from an unsupported version, and can not be loaded! This must be fixed!" );
	DATA_HALT( DES_Uber, CResourceObtainer::GetResource( this ), TXT( "Water" ), TXT( "This CWaterComponent needs to be removed! The file version is no longer supported" ) );
}

// Since the CWaterComponent base type changed at some point (and thus, all the data), we need to manually handle
// the data when the serialised file is too old. Fix for TTP W3#13716
void CWaterComponent::OnSerialize( IFile& file )
{
	if( file.GetVersion() < VER_IDLINES_REFACTOR )
	{
		HandleOldFileVersion();
	}
	else
	{
		TBaseClass::OnSerialize( file );
	}

	if( m_transformNeedsUpdate ) 
	{
		NotifyGlobalWater( true );
		m_transformNeedsUpdate = false;
	}
}

#ifndef NO_EDITOR
void CWaterComponent::OnNavigationCookerInitialization( CWorld* world, CNavigationCookingContext* context )
{
	CGlobalWater* globalWater = world->GetGlobalWater();
	if ( globalWater )
	{
		globalWater->Cooker_IncrementalShapeAddition( this );
	}
}
#endif

// Since the CWaterComponent base type changed at some point (and thus, all the data), we need to manually handle
// the data when the serialised file is too old. Fix for TTP W3#13716
void CWaterComponent::OnSerializeAdditionalData( IFile& file )
{
	if( file.GetVersion() < VER_IDLINES_REFACTOR )
	{
		HandleOldFileVersion();
	}
	else
	{
		TBaseClass::OnSerializeAdditionalData( file );		
	}
}


#ifndef NO_EDITOR
void CWaterComponent::EditorOnTransformChanged()
{
	TBaseClass::EditorOnTransformChanged();
	NotifyGlobalWater( true );
}

void CWaterComponent::OnEditorEndVertexEdit()
{
	TBaseClass::OnEditorEndVertexEdit();	
	NotifyGlobalWater( true );
}

Bool CWaterComponent::OnEditorVertexInsert( Int32 edge, const Vector& wishedPosition, Vector& allowedPosition, Int32& outInsertPos )
{
	Bool res = TBaseClass::OnEditorVertexInsert( edge, wishedPosition, allowedPosition, outInsertPos );
	NotifyGlobalWater( true );
	return res;
}

Bool CWaterComponent::OnEditorVertexDestroy( Int32 vertexIndex )
{
	Bool res = TBaseClass::OnEditorVertexDestroy( vertexIndex );
	NotifyGlobalWater( true );
	return res;
}

void CWaterComponent::OnEditorNodeMoved( Int32 vertexIndex, const Vector& oldPosition, const Vector& wishedPosition, Vector& allowedPosition )
{
	TBaseClass::OnEditorNodeMoved( vertexIndex, oldPosition, wishedPosition, allowedPosition );
	NotifyGlobalWater( true );
}
#endif
