/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "lightComponent.h"

#include "renderCommands.h"
#include "../../common/core/gatheredResource.h"
#include "../core/dataError.h"
#include "layer.h"
#include "world.h"
#include "renderProxy.h"
#include "bitmapTexture.h"
#include "entity.h"
#include "utils.h"

IMPLEMENT_ENGINE_CLASS( CLightComponent );
IMPLEMENT_ENGINE_CLASS( SLightFlickering );
IMPLEMENT_RTTI_BITFIELD( ELightUsageMask);

//dex++
IMPLEMENT_RTTI_ENUM( ELightShadowCastingMode );
//dex--

RED_DEFINE_STATIC_NAME( brightness );
RED_DEFINE_STATIC_NAME( attenuation );
RED_DEFINE_STATIC_NAME( autoHideDistance );
RED_DEFINE_STATIC_NAME( autoHideRange );

CGatheredResource resLightIcon( TXT("engine\\textures\\icons\\lighticon.xbm"), RGF_NotCooked );

CLightComponent::CLightComponent()
	: m_shadowCastingMode( LSCM_None )
	, m_shadowFadeRange( 5.0f )
	, m_shadowFadeDistance( 0.0f )
	, m_color( Color::WHITE )
	, m_radius( 5.0f )
	, m_brightness( 1.0f )
	, m_attenuation( 1.0f )
	, m_envColorGroup( ECG_Default )
	, m_autoHideDistance( 15.0f )
	, m_autoHideRange( 5.0f )
	, m_isEnabled( true )
	, m_renderProxy( nullptr )
	, m_allowDistantFade( true )
	, m_shadowBlendFactor( 1.f )
	, m_isFadeOnToggle( false )
{
}

CBitmapTexture* CLightComponent::GetSpriteIcon() const
{
	return resLightIcon.LoadAndGet< CBitmapTexture >();
}

Color CLightComponent::CalcSpriteColor() const
{
	//return m_color;
	if ( IsSelected() )
	{
		return Color::GREEN;
	}
	else
	{
		if ( !IsEnabled() )
		{
			return Color::GRAY;
		}
		if ( GetShadowCastingMode() == LSCM_None )
		{
			return Color::WHITE;
		}
		if ( GetShadowCastingMode() == LSCM_Normal )
		{
			return Color::RED;
		}
		if ( GetShadowCastingMode() == LSCM_OnlyDynamic )
		{
			//return Color::DARK_RED;
			return Color(255,63,63);
		}
		if ( GetShadowCastingMode() == LSCM_OnlyStatic )
		{
			return Color::LIGHT_RED;
		}
		return Color::WHITE;
	}
}

void CLightComponent::SetColor( const Color& color )
{
	// Set new color
	m_color = color;

	// Recreate
	RefreshRenderProxies();
}

void CLightComponent::SetEnabled( Bool enabled )
{
	if ( m_isEnabled != enabled )
	{
		// Change flag
		m_isEnabled = enabled;

		// Recreate
		RefreshRenderProxies();
	}
}

void CLightComponent::RefreshRenderProxies()
{
	// Recreate the render proxy if it is attached
	if ( IsAttached() )
	{
		CWorld* attachedWorld = GetLayer()->GetWorld();
		ConditionalAttachToRenderScene( attachedWorld );
	}
}

void CLightComponent::SetCastingShadows( Bool enabled )
{
	//dex++: reimplemented old behaviour
	if ( enabled != IsCastingShadows() )
	{
		if ( enabled )
		{
			m_shadowCastingMode = LSCM_Normal;
		}
		else
		{
			m_shadowCastingMode = LSCM_None;
		}

		// Recreate rendering proxies
		RefreshRenderProxies();
	}
	//dex--
}

//dex++: new interface
void CLightComponent::SetShadowCastingMode( ELightShadowCastingMode mode )
{
	if ( m_shadowCastingMode != mode )
	{
		m_shadowCastingMode = mode;

		// Recreate rendering proxies
		RefreshRenderProxies();
	}
}

void CLightComponent::SetShadowFadeDistance( Float shadowFadeDistance )
{
	if ( m_shadowFadeDistance != shadowFadeDistance )
	{
		m_shadowFadeDistance = shadowFadeDistance;

		// Recreate rendering proxies
		RefreshRenderProxies();
	}
}

void CLightComponent::SetShadowFadeRange( Float shadowFadeRange )
{
	if ( m_shadowFadeRange != shadowFadeRange )
	{
		m_shadowFadeRange = shadowFadeRange;

		// Recreate rendering proxies
		RefreshRenderProxies();
	}
}
//dex--

void CLightComponent::SetRadius( Float radius )
{
	// Limit
	radius = Clamp< Float >( radius, 0.0f, 100.0f );
	if ( m_radius != radius )
	{
		// Change radius
		m_radius = radius;

		// Schedule transform update
		ScheduleUpdateTransformNode();
	}
}

void CLightComponent::SetBrightness( Float brightness )
{
	if ( m_brightness != brightness )
	{
		// Change
		m_brightness = brightness;

		// Recreate
		if ( IsAttached() )
		{
			if ( m_renderProxy )
			{
				// Faster way
				const Vector data( m_brightness, 0.0f, 0.0f, 0.0f );
				( new CRenderCommand_UpdateLightProxyParameter( m_renderProxy, data, RLP_Brightness ) )->Commit();
			}
			else
			{
				RefreshRenderProxies();
			}
		}
	}
}

void CLightComponent::SetAttenuation( Float attenuation )
{
	if ( m_attenuation != attenuation )
	{
		// Change
		m_attenuation = attenuation;

		// Recreate
		if ( IsAttached() )
		{
			if ( m_renderProxy )
			{
				// Faster way
				const Vector data( m_attenuation, 0.0f, 0.0f, 0.0f );
				( new CRenderCommand_UpdateLightProxyParameter( m_renderProxy, data, RLP_Attenuation ) )->Commit();
			}
			else
			{
				RefreshRenderProxies();
			}
		}
	}
}

void CLightComponent::SetShadowBlendFactor( Float shadowBlendFactor )
{
	m_shadowBlendFactor = shadowBlendFactor;
}

void CLightComponent::SetFadeOnToggle( Bool enableFade )
{
	m_isFadeOnToggle = enableFade;
}

void CLightComponent::SetAllowDistantFade( bool allowDistantFade )
{
	if ( m_allowDistantFade != allowDistantFade )
	{
		// Change
		m_allowDistantFade = allowDistantFade;

		// Recreate
		if ( IsAttached() )
		{
			if ( m_renderProxy )
			{
				// Faster way
				const Vector data( m_allowDistantFade, 0.0f, 0.0f, 0.0f );
				( new CRenderCommand_UpdateLightProxyParameter( m_renderProxy, data, RLP_AllowDistantFade ) )->Commit();
			}
			else
			{
				RefreshRenderProxies();
			}
		}
	}
}

//dex++: moved
void CLightComponent::SetAutoHideDistance( Float autoHideDistance, Float autoHideRange )
{
	m_autoHideDistance = autoHideDistance;
	m_autoHideRange = autoHideRange;

	// Inform render proxy
	if ( m_renderProxy )
	{
		( new CRenderCommand_UpdateLightParameter( m_renderProxy, CNAME( autoHideDistance ), autoHideDistance ) )->Commit();
		( new CRenderCommand_UpdateLightParameter( m_renderProxy, CNAME( autoHideRange ), autoHideRange ) )->Commit();
	}
}
//dex--

void CLightComponent::SetLightFlickering( const SLightFlickering& lightFlickering )
{
	m_lightFlickering = lightFlickering;
	RefreshRenderProxies();
}

void CLightComponent::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );

	// Create a new render proxy if there isn't one
	if ( !m_renderProxy )
	{
		ConditionalAttachToRenderScene( GetEntity()->GetLayer()->GetWorld() );
	}
	else // ...otherwise update the existing property
	{
		const CName& name = property->GetName();

		if ( name == CNAME( color ) || name == CNAME( Color ) )
		{
			( new CRenderCommand_UpdateLightColor( m_renderProxy, m_color ) )->Commit();
		}
		else if ( property->GetType()->GetType() == RT_Fundamental && property->GetType()->GetName() == CNAME( Float ) )
		{
			Float value;
			property->Get( this, &value );
			( new CRenderCommand_UpdateLightParameter( m_renderProxy, name, value ) )->Commit();
		}
		else
		{
			// TODO: handle other cases without reconstructing the proxy
			RefreshRenderProxies();
		}
	}

#if 0
    if ( property->GetName() == TXT("color") )
    {
        // Reattach to rendering scene
        ConditionalAttachToRenderScene( GetEntity()->GetLayer()->GetWorld() );
    }
    else
    {
        PerformFullRecreation();
    }   
#endif
}

#ifndef NO_DATA_VALIDATION
void CLightComponent::OnCheckDataErrors( Bool isInTemplate ) const
{
	// Pass to base class
	TBaseClass::OnCheckDataErrors( isInTemplate );

	// No brightness for an entity without a template
	if ( GetEntity() && !GetEntity()->GetTemplate() )
	{
		if ( m_brightness < 0.01f && !isInTemplate )
		{
			CResource* resource = static_cast< CResource* > ( GetEntity()->GetLayer() );
			DATA_HALT( DES_Minor, resource, TXT("World"), TXT("Light '%ls' in entity has no brightness"), GetName().AsChar() );
		}
	}

	// No radius
	if ( m_radius < 0.01f && !isInTemplate )
	{
		if ( m_radius < 0.0f )
		{
			DATA_HALT( DES_Major, CResourceObtainer::GetResource( this ), TXT("World"), TXT("Light '%ls' in entity has negative radius"), GetName().AsChar() );
		}
		else
		{
			DATA_HALT( DES_Major, CResourceObtainer::GetResource( this ), TXT("World"), TXT("Light '%ls' in entity has no radius"), GetName().AsChar() );
		}
	}
}
#endif

void CLightComponent::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );

	PC_SCOPE_PIX( CLightComponent_OnAttached );

	// Attach to rendering scene
	ConditionalAttachToRenderScene( world );

	// Register in editor fragment list
	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_Lights );
	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_LightsBBoxes );
}

void CLightComponent::OnDetached( CWorld* world )
{
	TBaseClass::OnDetached( world );

	// Unregister from editor fragment list
	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_Lights );
	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_LightsBBoxes );

	// Detach from rendering scene
	ConditionalAttachToRenderScene( world );
}

#ifdef USE_UMBRA
Bool CLightComponent::ShouldBeCookedAsOcclusionData() const
{
	// if lightComponent is attached to entity via HardAttachment it will be transformed along with the parent,
	// then do not include it in occlusion data
	return !GetTransformParent();
}
#endif

#ifdef USE_UMBRA
Uint32 CLightComponent::GetOcclusionId() const
{
	return UmbraHelpers::CalculateBoundingBoxHash( Box( GetWorldPositionRef(), GetRadius() ) );
}
#endif // USE_UMBRA

void CLightComponent::OnUpdateTransformComponent( SUpdateTransformContext& context, const Matrix& prevLocalToWorld )
{
	PC_SCOPE( CLightComponent );

	// Pass to base class
	TBaseClass::OnUpdateTransformComponent( context, prevLocalToWorld );

	// Update in scene
	if ( IsAttached() && m_renderProxy )
	{
		// Prepare some fake bounding box
		Box fakeLightBox( GetLocalToWorld().GetTranslation(), m_radius * 0.8f );

		// Simple bounds around center point
		RenderProxyUpdateInfo info;
		info.m_localToWorld = &m_localToWorld;
		info.m_boundingBox = &fakeLightBox;

		// Relink the position and bounding box of this proxy on the rendering thread
		context.m_skinningContext.AddCommand_Relink( m_renderProxy, info );

		// Update the light's radius
		const Vector data( m_radius, 0.0f, 0.0f, 0.0f );
		context.m_skinningContext.AddCommand_UpdateLightProxyParameter( m_renderProxy, data, RLP_Radius );
	}
}

void CLightComponent::ConditionalAttachToRenderScene( CWorld* world )
{
	// Destroy current proxy
	if ( m_renderProxy )
	{
		// Detach if attached
		if ( world->GetRenderSceneEx() )
		{
 			if ( m_isFadeOnToggle )
 			{
 				// Fade out proxy from rendering scene
 				( new CRenderCommand_SetAutoFade( world->GetRenderSceneEx(), m_renderProxy, FT_FadeOutAndDestroy ) )->Commit();
 			}
 			else
			{
				// Detach proxy from rendering scene
				( new CRenderCommand_RemoveProxyFromScene( world->GetRenderSceneEx(), m_renderProxy ) )->Commit();
			}
		}

		// Free proxy
		m_renderProxy->Release();
		m_renderProxy = NULL;
	}

	// Add to scene
	const Bool shouldAdd = m_isEnabled && IsAttached() && !GRender->IsDeviceLost();
	if ( shouldAdd )
	{
		RenderProxyInitInfo initInfo;
		initInfo.m_component = this;

		// Create proxy
		m_renderProxy = GRender->CreateProxy( initInfo );
		if ( m_renderProxy )
		{
			// Attach to scene
			( new CRenderCommand_AddProxyToScene( world->GetRenderSceneEx(), m_renderProxy ) )->Commit();

			// Fade in
 			if ( m_isFadeOnToggle )
 			{
 				( new CRenderCommand_SetAutoFade( world->GetRenderSceneEx(), m_renderProxy, FT_FadeInStart ) )->Commit();
 			}
		}
	}
}

Bool CLightComponent::GetEffectParameterValue( CName paramName, EffectParameterValue &value /* out */ ) const
{
	if ( paramName == CNAME( brightness ) || paramName == CNAME( Brightness ) )
	{
		value.SetFloat( m_brightness );
		return true;
	}
	else if ( paramName == CNAME( radius ) || paramName == CNAME( Radius ) )
	{
		value.SetFloat( m_radius );
		return true;
	}
	else if ( paramName == CNAME( color ) || paramName == CNAME( Color ) )
	{
		value.SetColor( m_color );
		return true;
	}

	return false; // parameter not found
}

Bool CLightComponent::SetEffectParameterValue( CName paramName, const EffectParameterValue &value )
{
	if ( m_renderProxy )
	{
		if ( (paramName == CNAME( color ) || paramName == CNAME( Color )) && value.IsColor() )
		{
			( new CRenderCommand_UpdateLightColor( m_renderProxy, value.GetColor() ) )->Commit();
			return true;
		}

		if ( (paramName == CNAME( brightness ) || paramName == CNAME( Brightness )) && value.IsFloat() )
		{
			( new CRenderCommand_UpdateLightParameter( m_renderProxy, paramName, value.GetFloat() ) )->Commit();
			return true;
		}

		if ( (paramName == CNAME( radius ) || paramName == CNAME( Radius )) && value.IsFloat() )
		{
			( new CRenderCommand_UpdateLightParameter( m_renderProxy, paramName, value.GetFloat() ) )->Commit();
			return true;
		}
	}

	return false;
}

void CLightComponent::EnumEffectParameters( CFXParameters &effectParams /* out */ )
{
	effectParams.AddParameter< Float >( CNAME( brightness ) );
	effectParams.AddParameter< Float >( CNAME( radius ) );
	effectParams.AddParameter< Color >( CNAME( color ) );
}

void CLightComponent::RefreshInRenderSceneIfAttached()
{
	// Reattach
	if ( IsAttached() && GetLayer()->IsAttached() )
	{
		CWorld* attachedWorld = GetLayer()->GetWorld();
		ConditionalAttachToRenderScene( attachedWorld );
	}
	else
	{
		ASSERT( !m_renderProxy );
	}
}


Bool CLightComponent::OnPropertyMissing( CName propertyName, const CVariant& readValue )
{
	// Read property
	if ( propertyName == TXT("isCastingShadows") )
	{
		Bool isCastingShadows = false;
		readValue.AsType<Bool>( isCastingShadows );

		if ( isCastingShadows )
		{
			m_shadowCastingMode = LSCM_Normal;
		}
		else
		{
			m_shadowCastingMode = LSCM_None;
		}

		return true;
	}

	// Pass to base class
	return CSpriteComponent::OnPropertyMissing( propertyName, readValue );
}

void CLightComponent::OnPropertyExternalChanged( const CName& propertyName )
{
	TBaseClass::OnPropertyExternalChanged( propertyName );

	if ( m_renderProxy && propertyName == CNAME( brightness ) )
	{
		// Faster way
		const Vector data( m_brightness, 0.0f, 0.0f, 0.0f );
		( new CRenderCommand_UpdateLightProxyParameter( m_renderProxy, data, RLP_Brightness ) )->Commit();
	}
	else if ( m_renderProxy && propertyName == CNAME( attenuation ) )
	{
		// Faster way
		const Vector data( m_attenuation, 0.0f, 0.0f, 0.0f );
		( new CRenderCommand_UpdateLightProxyParameter( m_renderProxy, data, RLP_Attenuation ) )->Commit();
	}
	else if ( m_renderProxy && propertyName == CNAME( radius ) )
	{
		// Faster way
		const Vector data( m_radius, 0.0f, 0.0f, 0.0f );
		( new CRenderCommand_UpdateLightProxyParameter( m_renderProxy, data, RLP_Radius ) )->Commit();
	}
	else if ( m_renderProxy && propertyName == CNAME( color ) )
	{
		( new CRenderCommand_UpdateLightProxyParameter( m_renderProxy, m_color.ToVector(), RLP_Color ) )->Commit();
	}
}
