/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "bitmapTexture.h"
#include "layer.h"
#include "spotLightComponent.h"
#include "renderCommands.h"
#include "renderFrame.h"
#include "umbraScene.h"
#include "../../common/core/gatheredResource.h"

IMPLEMENT_ENGINE_CLASS( CSpotLightComponent );

RED_DEFINE_STATIC_NAME( projectionTexureAngle );
RED_DEFINE_STATIC_NAME( projectionTexureFactor );
RED_DEFINE_STATIC_NAME( projectionTexureUBias );
RED_DEFINE_STATIC_NAME( projectionTexureVBias );

CSpotLightComponent::CSpotLightComponent()
	: m_innerAngle( 30.0f )
	, m_outerAngle( 45.0f )
	, m_softness( 2.0f )
	, m_projectionTextureAngle( 90.0f )
	, m_projectionTexureUBias( 0.0f )
	, m_projectionTexureVBias( 0.0f )
{
}

CGatheredResource resSpotLightIcon( TXT("engine\\textures\\icons\\lightspoticon.xbm"), RGF_NotCooked );

CBitmapTexture* CSpotLightComponent::GetSpriteIcon() const
{
	return resSpotLightIcon.LoadAndGet< CBitmapTexture >();
}

void CSpotLightComponent::SetInnerAngle( Float angle )
{
	m_innerAngle = angle;
	ScheduleUpdateTransformNode(); // FIX: this doesn't update the angle in the proxy
}

void CSpotLightComponent::SetOuterAngle( Float angle )
{
	m_outerAngle = angle;
	ScheduleUpdateTransformNode(); // FIX: this doesn't update the angle in the proxy
}

void CSpotLightComponent::SetSoftness( Float attenuation )
{
	m_softness = attenuation;
}

void CSpotLightComponent::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag )
{
	// Icon
	TBaseClass::OnGenerateEditorFragments( frame, flag );

	// Radius sphere
	if ( (IsSelected() && flag == SHOW_Lights) || flag == SHOW_LightsBBoxes ) 
	{
		frame->AddDebugCone( GetLocalToWorld(), m_radius, m_innerAngle, m_outerAngle );
	}
}

Bool CSpotLightComponent::GetEffectParameterValue( CName paramName, EffectParameterValue &value /* out */ ) const
{
	if ( !TBaseClass::GetEffectParameterValue( paramName, value ) )
	{
		if ( paramName == CNAME( projectionTexureAngle ) || paramName == CNAME( ProjectionTexureAngle ) )
		{
			value.SetFloat( m_projectionTextureAngle );
			return true;
		}
		else if ( paramName == CNAME( projectionTexureUBias ) || paramName == CNAME( ProjectionTexureUBias ) )
		{
			value.SetFloat( m_projectionTexureUBias );
			return true;
		}
		else if ( paramName == CNAME( projectionTexureVBias ) || paramName == CNAME( ProjectionTexureVBias ) )
		{
			value.SetFloat( m_projectionTexureVBias );
			return true;
		}

		return false;

	}

	return true;
}

Bool CSpotLightComponent::SetEffectParameterValue( CName paramName, const EffectParameterValue &value )
{
	// Pass to base class first
	if ( TBaseClass::SetEffectParameterValue( paramName, value ) )
	{
		return true;
	}

	if ( m_renderProxy )
	{
		if ( (paramName == CNAME( projectionTexureAngle ) || paramName == CNAME( ProjectionTexureAngle )) && value.IsFloat() )
		{
			( new CRenderCommand_UpdateLightParameter( m_renderProxy, paramName, value.GetFloat() ) )->Commit();
			return true;
		}
		else if ( (paramName == CNAME( projectionTexureFactor ) || paramName == CNAME( ProjectionTexureFactor )) && value.IsFloat() )
		{
			( new CRenderCommand_UpdateLightParameter( m_renderProxy, paramName, value.GetFloat() ) )->Commit();
			return true;
		}
		else if ( (paramName == CNAME( projectionTexureUBias ) || paramName == CNAME( ProjectionTexureUBias )) && value.IsFloat() )
		{
			( new CRenderCommand_UpdateLightParameter( m_renderProxy, paramName, value.GetFloat() ) )->Commit();
			return true;
		}
		else if ( (paramName == CNAME( projectionTexureVBias ) || paramName == CNAME( ProjectionTexureVBias )) && value.IsFloat() )
		{
			( new CRenderCommand_UpdateLightParameter( m_renderProxy, paramName, value.GetFloat() ) )->Commit();
			return true;
		}
	}

	// Not processed
	return false;
}

void CSpotLightComponent::EnumEffectParameters( CFXParameters &effectParams /* out */ )
{
	TBaseClass::EnumEffectParameters( effectParams );

	effectParams.AddParameter< Float >( CNAME( projectionTexureAngle ) );
	effectParams.AddParameter< Float >( CNAME( projectionTexureFactor ) );
	effectParams.AddParameter< Float >( CNAME( projectionTexureUBias ) );
	effectParams.AddParameter< Float >( CNAME( projectionTexureVBias ) );
}

#ifdef USE_UMBRA
Bool CSpotLightComponent::OnAttachToUmbraScene( CUmbraScene* umbraScene, const VectorI& bounds )
{
#if !defined(NO_UMBRA_DATA_GENERATION) && defined(USE_UMBRA_COOKING)
	if ( umbraScene && umbraScene->IsDuringSyncRecreation() && CUmbraScene::ShouldAddComponent( this ) )
	{
		return umbraScene->AddSpotLight( this, bounds );
	}
#endif // NO_UMBRA_DATA_GENERATION

	return false;
}
#endif
