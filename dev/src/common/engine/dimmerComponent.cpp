/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "bitmapTexture.h"
#include "dimmerComponent.h"
#include "entity.h"
#include "layer.h"
#include "renderCommands.h"
#include "renderFrame.h"
#include "renderProxy.h"
#include "umbraScene.h"
#include "world.h"
#include "../../common/core/gatheredResource.h"

namespace Config
{
	TConfigVar< Int32 >	cvDimmerMinimumStreamingDistance( "Streaming/Components", "DimmerMinimumStreamingDistance", 100 );
}

IMPLEMENT_ENGINE_CLASS( CDimmerComponent );

IMPLEMENT_RTTI_ENUM( EDimmerType );

CGatheredResource resDimmerIcon( TXT("engine\\textures\\icons\\dimmericon.xbm"), RGF_NotCooked );

CDimmerComponent::CDimmerComponent()
	: m_dimmerType ( DIMMERTYPE_Default )
	, m_isAreaMarker ( false )
	, m_ambientLevel ( 0.f )
	, m_marginFactor ( 1.f )
	, m_autoHideDistance( -1.0f )			// -1.0 means it will take value from Dimmer bounding box size
{
	// Setting the default value to false, since it was this way before we changed the base class from SpriteComponent to DrawableComponent.
	// DrawableComponent has streamed true by default, and keeping it this way fucked up existing data loading.
	SetStreamed( false ); 
}

CDimmerComponent::~CDimmerComponent()
{	
}

void CDimmerComponent::OnAttached( CWorld* world )
{
	PC_SCOPE_PIX( CDimmerComponent_OnAttached );

	TBaseClass::OnAttached( world );

	// Register in editor fragment list
	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_Dimmers );
	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_DimmersBBoxes );
	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_Sprites );
}

void CDimmerComponent::OnDetached( CWorld* world )
{
	TBaseClass::OnDetached( world );

	// Unregister from editor fragment list
	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_Dimmers );
	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_DimmersBBoxes );
	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_Sprites );
}

void CDimmerComponent::OnSerialize( IFile& file )
{
	TBaseClass::OnSerialize( file );
}

Float CDimmerComponent::GetAutoHideDistance() const 
{ 
	return CalculateAutoHideDistance( m_autoHideDistance, GetBoundingBox() ); 
}

Float CDimmerComponent::GetDefaultAutohideDistance() const 
{ 
	return Config::cvDimmerHideDistance.Get(); 
}

Float CDimmerComponent::GetMaxAutohideDistance() const 
{ 
	return 300.0f; 
}

#ifdef USE_UMBRA
Bool CDimmerComponent::OnAttachToUmbraScene( CUmbraScene* umbraScene, const VectorI& bounds )
{
#if !defined(NO_UMBRA_DATA_GENERATION) && defined(USE_UMBRA_COOKING)
	if ( umbraScene && umbraScene->IsDuringSyncRecreation() && CUmbraScene::ShouldAddComponent( this ) )
	{
		return umbraScene->AddDimmer( this, bounds );
	}
#endif // NO_UMBRA_DATA_GENERATION

	return false;
}

Uint32 CDimmerComponent::GetOcclusionId() const
{
	return UmbraHelpers::CalculateBoundingBoxHash( GetBoundingBox() );
}
#endif // USE_UMBRA

void CDimmerComponent::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag )
{
	// Icon
	TBaseClass::OnGenerateEditorFragments( frame, flag );

	// Dimmer shape
	if ( flag == SHOW_DimmersBBoxes )
	{
		frame->AddDebugBox( GetBoundingBox(), Matrix::IDENTITY, Color::LIGHT_YELLOW );
	}

	// Dimmer local shape (shown when selected)
	if ( IsSelected() && flag == SHOW_Dimmers )
	{
		frame->AddDebugBox( Box( Vector( -1.f, -1.f, -1.f ), Vector( 1.f, 1.f, 1.f ) ), GetLocalToWorld(), Color::LIGHT_GREEN );
	}

	// Draw sprite
	if ( IsVisible() && flag == SHOW_Sprites )
	{
		// Sprite icon
		CBitmapTexture* icon = resDimmerIcon.LoadAndGet< CBitmapTexture >();
		Color iconColor = IsSelected() ? Color::GREEN : Color::WHITE;

		if ( icon )
		{
			// Draw editor icons
			Float screenScale = frame->GetFrameInfo().CalcScreenSpaceScale( GetWorldPosition() );
			const Float size = 0.25f*screenScale;

#ifndef NO_COMPONENT_GRAPH
			frame->AddSprite( GetLocalToWorld().GetTranslation(), size, iconColor, GetHitProxyID(), icon, false );
#else
			frame->AddSprite( GetLocalToWorld().GetTranslation(), size, iconColor, CHitProxyID(), icon, false );
#endif
		}
	}
}

void CDimmerComponent::OnUpdateBounds()
{
	m_boundingBox = GetLocalToWorld().TransformBox( Box ( Vector ( -1.f, -1.f, -1.f ), Vector ( 1.f, 1.f, 1.f ) ) );
}

Uint32 CDimmerComponent::GetMinimumStreamingDistance() const
{
	// hack to cover varous platforms ini files, and we are taking the highest possible here
	return m_autoHideDistance < 0.0001f ? (Uint32)Config::cvDimmerMinimumStreamingDistance.Get() : (Uint32)Red::Math::MRound( m_autoHideDistance * 1.1f );
}
