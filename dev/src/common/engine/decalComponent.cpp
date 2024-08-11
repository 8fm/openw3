/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "decalComponent.h"
#include "renderCommands.h"
#include "../core/dataError.h"
#include "hardAttachment.h"
#include "renderFrame.h"
#include "renderObject.h"
#include "drawableComponent.h"
#include "renderProxy.h"
#include "world.h"
#include "layer.h"
#include "umbraScene.h"
#include "bitmapTexture.h"
#include "fxDefinition.h"
#include "entity.h"
#include "utils.h"
#include "../core/gatheredResource.h"

namespace Config
{
	TConfigVar< Int32 >	cvDecalMinimumStreamingDistance( "Streaming/Components", "DecalMinimumStreamingDistance", 60 );
}

IMPLEMENT_ENGINE_CLASS( CDecalComponent );

Box CDecalComponent::m_unitBox = Box( Vector::ZEROS, 0.5f );

const Float CDecalComponent::AUTOHIDE_MARGIN = 5.0f;

namespace
{
	CGatheredResource resDecalIcon( TXT("engine\\textures\\icons\\particleicon.xbm"), RGF_NotCooked );
}

CDecalComponent::CDecalComponent()
	: CDrawableComponent()
	, m_diffuseTexture( NULL )
	, m_autoHideDistance( -1.f )			// -1.0 means it will take value from Config
	, m_horizontalFlip( false )
	, m_verticalFlip( false )
	, m_specularity( -1.0f )
	, m_specularColor( 48, 48, 48, 255 )
	, m_fadeTime( 0.25f )
{
}

CDecalComponent::~CDecalComponent()
{
}

void CDecalComponent::OnUpdateBounds()
{
	// No skinning, use static bounding box
	if ( !m_transformParent || !m_transformParent->ToSkinningAttachment() )
	{
		// Use default bounding box from mesh
		m_boundingBox = GetLocalToWorld().TransformBox( m_unitBox );
	}
	else
	{
		TBaseClass::OnUpdateBounds();
	}
}

void CDecalComponent::OnAttached( CWorld* world )
{
	// Pass to base class
	TBaseClass::OnAttached( world );

	PC_SCOPE_PIX( CDecalComponent_OnAttached );

	// Add to editor fragments group
	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_Bboxes );
	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_BboxesDecals );
	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_SpritesDecals );
}

void CDecalComponent::OnDetached( CWorld* world )
{
	// Pass to base class
	TBaseClass::OnDetached( world );
	// Remove from editor fragments group
	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_Bboxes );
	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_BboxesDecals );
	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_SpritesDecals );
}

#ifdef USE_UMBRA
Bool CDecalComponent::OnAttachToUmbraScene( CUmbraScene* umbraScene, const VectorI& bounds )
{
#if !defined(NO_UMBRA_DATA_GENERATION) && defined(USE_UMBRA_COOKING)
	if ( umbraScene && umbraScene->IsDuringSyncRecreation() && CUmbraScene::ShouldAddComponent( this ) )
	{
		return umbraScene->AddDecal( this, bounds );
	}
#endif // NO_UMBRA_DATA_GENERATION

	return false;
}
#endif // USE_UMBRA

Uint32 CDecalComponent::GetOcclusionId() const
{
	RED_ASSERT( GetDiffuseTexture() != nullptr, TXT("Diffuse texture not available.") );
	return GetDiffuseTexture() ? GetHash( GetDiffuseTexture()->GetDepotPath() ) : 0;
}

void CDecalComponent::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag )
{
	// Pass to base class
	TBaseClass::OnGenerateEditorFragments( frame, flag );

	if ( flag == SHOW_Bboxes && IsVisible() )
	{
		frame->AddDebugBox( m_boundingBox, Matrix::IDENTITY, Color::LIGHT_YELLOW );
	}

	if ( flag == SHOW_SpritesDecals && IsVisible() && IsAttached() )
	{
		// Sprite icon
		if ( CBitmapTexture* icon = resDecalIcon.LoadAndGet< CBitmapTexture >() )
		{
			// Draw editor icons
			Float size = 0.25f * frame->GetFrameInfo().CalcScreenSpaceScale( GetWorldPosition() );;
#ifndef NO_COMPONENT_GRAPH
			frame->AddSprite( GetLocalToWorld().GetTranslation(), size, IsSelected() ? Color::GREEN : Color::WHITE, GetHitProxyID(), icon, false );
#else
			frame->AddSprite( GetLocalToWorld().GetTranslation(), size, IsSelected() ? Color::GREEN : Color::WHITE, CHitProxyID(), icon, false );
#endif
		}
	}

	// Generate bounding boxes
	if ( flag == SHOW_BboxesDecals && IsVisible() )
	{
//#define USE_DECAL_FAT_LINES

#ifndef USE_DECAL_FAT_LINES

		Float userThickness = frame->GetFrameInfo().GetRenderingDebugOption( VDCommon_DebugLinesThickness );
		if ( frame->GetFrameInfo().m_renderingMode == RM_HitProxies )
		{
#ifndef NO_COMPONENT_GRAPH
			frame->AddDebugFatBox( m_unitBox, m_localToWorld, GetHitProxyID().GetColor(), userThickness );
#endif
		}
		else
		{
			frame->AddDebugFatBox( m_unitBox, m_localToWorld, IsSelected() ? Color::GREEN : Color::RED, userThickness );
		}

#else
		Vector corners[8];
		unitBox.CalcCorners( corners );

		Vector transformedCorners[8];
		for ( Uint32 i = 0; i < 8; i++ )
		{
			transformedCorners[i] = m_localToWorld.TransformPoint( corners[i] );
		}

		Color color = IsSelected() ? Color::GREEN : Color:RED;
		Float width = 0.1f;

		// Draw lines
		frame->AddDebugFatLine( transformedCorners[0], transformedCorners[4], color, width );
		frame->AddDebugFatLine( transformedCorners[1], transformedCorners[5], color, width );
		frame->AddDebugFatLine( transformedCorners[2], transformedCorners[6], color, width );
		frame->AddDebugFatLine( transformedCorners[3], transformedCorners[7], color, width );
		frame->AddDebugFatLine( transformedCorners[0], transformedCorners[1], color, width );
		frame->AddDebugFatLine( transformedCorners[1], transformedCorners[3], color, width );
		frame->AddDebugFatLine( transformedCorners[3], transformedCorners[2], color, width );
		frame->AddDebugFatLine( transformedCorners[2], transformedCorners[0], color, width );
		frame->AddDebugFatLine( transformedCorners[4], transformedCorners[5], color, width );
		frame->AddDebugFatLine( transformedCorners[5], transformedCorners[7], color, width );
		frame->AddDebugFatLine( transformedCorners[7], transformedCorners[6], color, width );
		frame->AddDebugFatLine( transformedCorners[6], transformedCorners[4], color, width );
#endif
	}
}

#ifndef NO_DATA_VALIDATION
void CDecalComponent::OnCheckDataErrors( Bool isInTemplate ) const
{
	// Pass to base class
	TBaseClass::OnCheckDataErrors( isInTemplate );
	if ( !m_diffuseTexture )
	{
		DATA_HALT( DES_Minor, CResourceObtainer::GetResource( this ), TXT("Rendering"), TXT("DecalComponent '%ls' in entity has no diffuse texture."), GetName().AsChar() );
	}
}
#endif // NO_DATA_VALIDATION

Bool CDecalComponent::GetEffectParameterValue( CName paramName, EffectParameterValue &value /* out */ ) const
{
	if ( !TBaseClass::GetEffectParameterValue( paramName, value ) )
	{
		if ( paramName == CNAME( NormalThreshold ) )
		{
			// setting value IN DEGREES
			value.SetFloat( m_normalThreshold );
			return true;
		}
		return false;
	}

	return true;
}

Bool CDecalComponent::SetEffectParameterValue( CName paramName, const EffectParameterValue &value )
{
	// Pass to base class first
	if ( TBaseClass::SetEffectParameterValue( paramName, value ) )
	{
		return true;
	}

	if ( m_renderProxy )
	{
		if ( paramName == CNAME( NormalThreshold ) && value.IsFloat() )
		{
			// value that comes in is in degrees, convert to radians before sending to renderer
			Float normalThresholdRadians = DEG2RAD( value.GetFloat() );
			( new CRenderCommand_UpdateEffectParameters( m_renderProxy, Vector( normalThresholdRadians, 0.0f, 0.0f, 0.0f ), 0 ) )->Commit();
			return true;
		}
	}

	// Not processed
	return false;
}

void CDecalComponent::EnumEffectParameters( CFXParameters &effectParams /* out */ )
{
	TBaseClass::EnumEffectParameters( effectParams );

	effectParams.AddParameter< Float >( CNAME( NormalThreshold ) );
}

Uint32 CDecalComponent::GetMinimumStreamingDistance() const
{
	// hack to cover varous platforms ini files, and we are taking the highest possible here
	return m_autoHideDistance < 0.0001f ? (Uint32)Config::cvDecalMinimumStreamingDistance.Get() : (Uint32)Red::Math::MRound( m_autoHideDistance * 1.1f );
}
