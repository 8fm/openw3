/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "pointLightComponent.h"
#include "layer.h"
#include "renderFrame.h"
#include "bitmapTexture.h"
#include "umbraScene.h"

#include "../core/gatheredResource.h"

IMPLEMENT_ENGINE_CLASS( CPointLightComponent );
//dex++
IMPLEMENT_RTTI_BITFIELD( ELightCubeSides );
//dex--

CPointLightComponent::CPointLightComponent()
	//dex++
	: m_cacheStaticShadows( false )
	, m_dynamicShadowsFaceMask( 0x3F ) // all
	//dex--
{
}

CGatheredResource resPointLightIcon( TXT("engine\\textures\\icons\\lightpointicon.xbm"), RGF_NotCooked );

CBitmapTexture* CPointLightComponent::GetSpriteIcon() const
{
	return resPointLightIcon.LoadAndGet< CBitmapTexture >();
}

void CPointLightComponent::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag )
{
	// Icon
	TBaseClass::OnGenerateEditorFragments( frame, flag );

	// Radius sphere
	if ( (IsSelected() && flag == SHOW_Lights) || flag == SHOW_LightsBBoxes ) 
	{
		frame->AddDebugSphere( GetWorldPosition(), GetRadius(), Matrix::IDENTITY, Color::WHITE );

		if( IsCastingShadows() )
		{
			frame->AddDebugSphere( GetWorldPosition(), GetShadowFadeDistance() + GetShadowFadeRange(), Matrix::IDENTITY, Color::LIGHT_RED );
		}
	}
}

#ifdef USE_UMBRA
Bool CPointLightComponent::OnAttachToUmbraScene( CUmbraScene* umbraScene, const VectorI& bounds )
{
#if !defined(NO_UMBRA_DATA_GENERATION) && defined(USE_UMBRA_COOKING)
	if ( umbraScene && umbraScene->IsDuringSyncRecreation() && CUmbraScene::ShouldAddComponent( this ) )
	{
		return umbraScene->AddPointLight( this, bounds );
	}
#endif // NO_UMBRA_DATA_GENERATION

	return false;
}
#endif
