/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "renderProxyLight.h"
#include "renderCollector.h"
#include "renderScene.h"
#include "renderDistantLightBatcher.h"
#include "../engine/baseEngine.h"
#include "../engine/lightComponent.h"
#include "../engine/spotLightComponent.h"
#include "../engine/pointLightComponent.h"
#include "../engine/renderSettings.h"

#ifndef RED_FINAL_BUILD
	extern SceneRenderingStats GRenderingStats;
#endif

RED_DEFINE_STATIC_NAME( attenuation );
RED_DEFINE_STATIC_NAME( brightness );
RED_DEFINE_STATIC_NAME( autoHideDistance );
RED_DEFINE_STATIC_NAME( autoHideRange );
RED_DEFINE_STATIC_NAME( shadowFadeDistance );
RED_DEFINE_STATIC_NAME( shadowFadeRange );
RED_DEFINE_STATIC_NAME( shadowBlendFactor );


// Create a continuous, pseudo noise, double bezier based - infinite - 8 like shape - movement 
// that on XY looks pretty much like this (Z also, but flattened):
//
//    Y	
//	^		  _
//	|		 / \
//	|		/   \
//	|		\   /
//	|		 \ /
//	|		 / \
//	|		/   \
//  |       \   /
//  |        \_/
//	|
//	|___________________>   X
//	
Vector DoubleCrossBezierFunction( Float t, Float sgn, Float range )
{
	Float XZ = range*( -sgn*3.0f*t*t + (sgn)*3.0f*t);
	Float Y = range*(6.0f*t*t*t-9.0f*t*t+3.0f*t);

	return Vector( XZ, Y, 0.5f*XZ );
}

IRenderProxyLight::IRenderProxyLight( ERenderProxyType type, const RenderProxyInitInfo& initInfo )
	: IRenderProxyFadeable( type, initInfo )
	, m_radius( 0.0f )
	, m_brightness( 0.0f )
	, m_attenuation( 1.f )
	, m_color( Color::BLACK )
	, m_envColorGroup( ECG_Default )
	, m_autoHideRange( 7.5f )			// keep in sync with autoHideInvRange
	, m_autoHideInvRange( 1.f / 7.5f )	// keep in sync with autoHideRange
	, m_shadowBlendFactor( 1.0f )
	, m_lightFlickerInfo( NULL )
	, m_allowDistantFade( true )
	, m_forcedDynamic( false )
	, m_isCastingStaticShadows( false )
	, m_isCastingDynamicShadows( false )
	, m_staticShadowsCached( false )
	, m_shadowFadeDistance( 0.0f )
	, m_shadowFadeInvRange( 0.0f )
	, m_currentShadowResolution( 0 )
	, m_currentShadowAlpha( 1.0f )
	, m_sceneEnvProbeLightIndex( -1 )
{
#ifdef USE_UMBRA
	Uint32 modelId = 0;
#endif // USE_UMBRA

	if ( initInfo.m_component && initInfo.m_component->IsA< CLightComponent >() )
	{
		// Copy light parameters
		const CLightComponent* lightComponent = static_cast< const CLightComponent* >( initInfo.m_component );
		m_radius = Max< Float >( 0.01f, lightComponent->GetRadius() );
		m_brightness = lightComponent->GetBrightness();
		m_attenuation = lightComponent->GetAttenuation();
		m_color = lightComponent->GetColor();
		m_forcedDynamic = lightComponent->GetTransformParent() != nullptr;
		m_shadowBlendFactor = Clamp( lightComponent->GetShadowBlendFactor(), 0.f, 1.f );

		COMPILE_ASSERT( sizeof(m_lightUsageMask) == sizeof(Uint16) );
		m_lightUsageMask = ( Uint16 )lightComponent-> GetLightUsageMask();
		ASSERT( m_lightUsageMask == lightComponent-> GetLightUsageMask(), TXT( "Unable to properly cast m_lightUsageMask from CLightComponent to IRenderProxyLight" ) );

		m_envColorGroup = lightComponent->GetEnvColorGroup();
		m_autoHideRange = Max( 0.001f, lightComponent->GetAutoHideRange() );
		m_autoHideDistance = Max( 0.f, lightComponent->GetAutoHideDistance() );
		m_autoHideDistanceSquared = m_autoHideDistance * m_autoHideDistance;
		m_autoHideInvRange = 1.f / m_autoHideRange;

		// read the casting options
		const ELightShadowCastingMode shadowMode = lightComponent->GetShadowCastingMode();
		m_isCastingDynamicShadows = (shadowMode == LSCM_OnlyDynamic) || (shadowMode == LSCM_Normal);
		m_isCastingStaticShadows = (shadowMode == LSCM_OnlyStatic) || (shadowMode == LSCM_Normal);

		// some spot lights are allowed to be "distant"
		Bool allowSpotLightDistantFade = false;

		// in case of points light the static shadows can be chached
		if ( lightComponent->IsA< CPointLightComponent >() )
		{
			const CPointLightComponent* plc = static_cast< const CPointLightComponent* >( lightComponent );
			if ( plc->IsCachingStaticShadows() )
			{
				// we can only cache lights if they are not moving :)
				if ( lightComponent->GetLightFlickering()->m_positionOffset <= 0.0f )
				{
					// use the static shadow caching if possible
					m_staticShadowsCached = m_isCastingStaticShadows;
				}
			}
		}
		else if ( lightComponent->IsA< CSpotLightComponent>() )
		{
			const CSpotLightComponent* slc = static_cast< const CSpotLightComponent* >( lightComponent );
			allowSpotLightDistantFade = slc->GetOuterAngle() > 120.0f ? true : false;
		}

		// process the shadow fade range
		m_shadowFadeDistance = lightComponent->GetShadowFadeDistance();
		if ( m_shadowFadeDistance <= 0.0f ) m_shadowFadeDistance = FLT_MAX; // extend
		m_shadowFadeInvRange = 1.0f / Max( 0.0001f, lightComponent->GetShadowFadeRange() );
		m_shadowFadeDistance += Max( 0.0001f, lightComponent->GetShadowFadeRange() );
		m_allowDistantFade = ( type == RPT_PointLight || allowSpotLightDistantFade ) && lightComponent->GetAllowDistantLight();
		
		// Add some flicker info if light needs it
		if ( lightComponent->GetLightFlickering() && lightComponent->GetLightFlickering()->m_flickerStrength > 0.0f )
		{
			m_lightFlickerInfo = new SLightFlickerInfo();
			m_lightFlickerInfo->m_flickeringStrength = Clamp( lightComponent->GetLightFlickering()->m_flickerStrength, 0.0f, 1.0f );
			m_lightFlickerInfo->m_flickerPeriod = lightComponent->GetLightFlickering()->m_flickerPeriod;			
			m_lightFlickerInfo->m_flickerTimeCounter = 0.0f;
			m_lightFlickerInfo->m_positionFlickerTimeCounter = 0.0f;
			m_lightFlickerInfo->m_maxOffset = lightComponent->GetLightFlickering()->m_positionOffset;

			const Float initialMultiplier = 1.0f - 0.5f * m_lightFlickerInfo->m_flickeringStrength;
			m_lightFlickerInfo->m_nextMultiplier = initialMultiplier;
			m_lightFlickerInfo->m_prevMultiplier = initialMultiplier;
			
			m_lightFlickerInfo->m_nextOffset = Vector::ZEROS;
			m_lightFlickerInfo->m_oscillatingTimeDelta = GEngine->GetRandomNumberGenerator().Get< Float >();			
		}

#ifdef USE_UMBRA
		modelId = lightComponent->GetOcclusionId();
#endif // USE_UMBRA
	}
	else if ( initInfo.m_packedData && (initInfo.m_packedData->GetType() == RPT_PointLight || initInfo.m_packedData->GetType() == RPT_SpotLight) )
	{
		// Copy light parameters
		const RenderProxyLightInitData* lightData = static_cast< const RenderProxyLightInitData* >( initInfo.m_packedData );
		m_radius = Max< Float >( 0.01f, lightData->m_radius );
		m_brightness = lightData->m_brightness;
		m_attenuation = lightData->m_attenuation;
		m_color = lightData->m_color;
		m_envColorGroup = (EEnvColorGroup) lightData->m_envColorGroup;
		m_forcedDynamic = false;
		m_shadowBlendFactor = Clamp( lightData->m_shadowBlendFactor, 0.f, 1.f );
		m_lightUsageMask = (Uint16) lightData->m_lightUsageMask;

		// fade
		m_autoHideDistance = Max( 0.f, lightData->m_autoHideDistance );
		m_autoHideDistanceSquared = m_autoHideDistance * m_autoHideDistance;
		m_autoHideRange = Max( 0.001f, lightData->m_autoHideRange );
		m_autoHideInvRange = 1.f / m_autoHideRange;
		m_allowDistantFade = lightData->m_allowDistanceFade;

		// process the shadow fade range
		m_shadowFadeDistance = Max( 0.0f, lightData->m_shadowFadeDistance );		

		m_shadowFadeInvRange = 1.0f / Max( 0.0001f, lightData->m_shadowFadeRange );
		m_shadowFadeDistance += Max( 0.0001f, lightData->m_shadowFadeRange );

		// flickering
		const SLightFlickering* flickerInfo = (const SLightFlickering*) &lightData->m_lightFlickering;
		if ( flickerInfo->m_flickerStrength > 0.0f )
		{
			m_lightFlickerInfo = new SLightFlickerInfo();
			m_lightFlickerInfo->m_flickeringStrength = Clamp( flickerInfo->m_flickerStrength, 0.0f, 1.0f );
			m_lightFlickerInfo->m_flickerPeriod = flickerInfo->m_flickerPeriod;			
			m_lightFlickerInfo->m_flickerTimeCounter = 0.0f;
			m_lightFlickerInfo->m_positionFlickerTimeCounter = 0.0f;
			m_lightFlickerInfo->m_maxOffset = flickerInfo->m_positionOffset;

			const Float initialMultiplier = 1.0f - 0.5f * m_lightFlickerInfo->m_flickeringStrength;
			m_lightFlickerInfo->m_nextMultiplier = initialMultiplier;
			m_lightFlickerInfo->m_prevMultiplier = initialMultiplier;
						
			m_lightFlickerInfo->m_nextOffset = Vector::ZEROS;						
			m_lightFlickerInfo->m_oscillatingTimeDelta = GEngine->GetRandomNumberGenerator().Get< Float >();
			
		}

		// read the casting options
		const ELightShadowCastingMode shadowMode = (const ELightShadowCastingMode) lightData->m_shadowCastingMode;
		m_isCastingDynamicShadows = (shadowMode == LSCM_OnlyDynamic) || (shadowMode == LSCM_Normal);
		m_isCastingStaticShadows = (shadowMode == LSCM_OnlyStatic) || (shadowMode == LSCM_Normal);

		COMPILE_ASSERT( sizeof(m_lightUsageMask) == sizeof(Uint16) );
		if ( initInfo.m_packedData->GetType() == RPT_PointLight )
		{
			// we can only cache lights if they are not moving :)
			if ( flickerInfo->m_positionOffset <= 0.0f )
			{
				// use the static shadow caching if possible
				m_staticShadowsCached = m_isCastingStaticShadows;
			}
		}

#ifdef USE_UMBRA
		modelId = initInfo.m_packedData->m_occlusionId;
#endif // USE_UMBRA
	}

	// Calculate world to light matrix
	m_lightToWorld = m_localToWorld;
	m_lightToWorld.SetScale33( m_radius );

	// Invert
	m_worldToLight = m_lightToWorld.Inverted();

#ifdef USE_UMBRA
	m_umbraProxyId = GlobalVisID( modelId, GetLocalToWorld() );
#endif // USE_UMBRA
}

IRenderProxyLight::~IRenderProxyLight()
{
	RED_ASSERT( -1 == GetSceneEnvProbeLightIndex() && "Wasn't it detached from scene? It would clear sceneEnvProbeLightIndex." );

	m_shadowProxies.Clear();

	if ( m_lightFlickerInfo )
	{
		delete m_lightFlickerInfo;
		m_lightFlickerInfo = NULL;
	}
}

void IRenderProxyLight::CollectElements( CRenderCollector& collector )
{
	if ( !IsRenderedToScene() )
	{
		return;
	}

	const auto& m_info = collector.m_info;

	SLightFactorInfo distanceInfo = GetDistanceInfo( m_info->m_camera.GetPosition(), m_info->m_worldRenderSettings, m_info->m_envParametersArea.m_colorGroups );

	bool collected = false;

	if ( distanceInfo.VisibleNear() )
	{
		collected = true;
		collector.m_renderCollectorData->m_lights.PushBack( this );
#ifndef RED_FINAL_BUILD
		if( !distanceInfo.VisibleDistance() ) ++GRenderingStats.m_numLights;
#endif
	}

	if( distanceInfo.VisibleDistance() )
	{
		collected = true;
		collector.m_renderCollectorData->m_distantLights.PushBack( this );
#ifndef RED_FINAL_BUILD
		if( !distanceInfo.VisibleNear() ) ++GRenderingStats.m_numDistantLights;
#endif
	}

	if( collected )
	{
		UpdateOncePerFrame( collector );
	}
	
}

const EFrameUpdateState IRenderProxyLight::UpdateOncePerFrame( const CRenderCollector& collector )
{
	const auto ret = IRenderProxyFadeable::UpdateOncePerFrame( collector );
	if ( ret == FUS_AlreadyUpdated )
		return ret;

	if ( m_lightFlickerInfo )
	{
		UpdateFlickering();
	}

	return ret;
}

Int32 IRenderProxyLight::GetSceneEnvProbeLightIndex() const
{
	return m_sceneEnvProbeLightIndex;
}

void IRenderProxyLight::SetSceneEnvProbeLightIndex( Int32 index )
{
	RED_ASSERT( index >= -1 );
	m_sceneEnvProbeLightIndex = index;
}

Bool IRenderProxyLight::AffectsBounds( const Box& bounds ) const
{
	// Direct BBox check
	return GetBoundingBox().Touches( bounds );
}

Bool IRenderProxyLight::IsRenderedToScene() const
{
	return 0 == (GetLightUsageMask() & LUM_ExcludeFromSceneRender);
}

Bool IRenderProxyLight::IsRenderedToEnvProbes() const
{
	return 0 != (GetLightUsageMask() & LUM_RenderToEnvProbe);
}

void IRenderProxyLight::Relink( const Box& boundingBox, const Matrix& localToWorld )
{
	// Pass to base class
	IRenderProxyBase::Relink( boundingBox, localToWorld );

	// Calculate world to light matrix
	m_lightToWorld = m_localToWorld;
	m_lightToWorld.SetScale33( m_radius );

	// Invert
	m_worldToLight = m_lightToWorld.Inverted();
}

void IRenderProxyLight::GatherDistantLight( const CRenderFrameInfo& info, CRenderDistantLightBatcher& batcher )
{
	const Vector finalPos = GetFinalPosition();

	const Vector2 fadeCoef = CalculateAutoHideFactor( info.m_camera.GetPosition(), info.m_worldRenderSettings );
	Vector finalColor = GetFinalColorNoFade( info.m_envParametersArea.m_colorGroups ) * ( fadeCoef.X * fadeCoef.Y );
	// Pack attenuation factor
	finalColor.A[3] = m_attenuation*m_attenuation; // It will be clamped by GetConvertedToUByte4Color
	
	batcher.AddLight( finalPos , m_radius, finalColor );
}

Vector2 IRenderProxyLight::CalculateAutoHideFactor( const Vector &cameraPosition, const SWorldRenderSettings &worldRenderSettings, Bool allowDistantLight ) const
{	
	Vector2 result( 1.0f , 0.0f ); // Full light, no distance fake
	
	const Bool useDistantLight = allowDistantLight && m_allowDistantFade;
	Float squareDistance = 0.0f;

	if( m_allowDistantFade )
	{
		const Vector position = m_localToWorld.GetTranslation();
		squareDistance = cameraPosition.DistanceSquaredTo( position );
	}
	
	const Float distThreshold = GetAutoHideDistance() + m_radius;

	if ( squareDistance < distThreshold * distThreshold )
	{
		const Float distance = Red::Math::MSqrt( squareDistance );
		result.X = ::Clamp( (distThreshold - distance) * m_autoHideInvRange , 0.0f , 1.0f );
	}
	else
	{
		result.X = 0.0f;
	}	

	if ( m_allowDistantFade )
	{
		const Float distThresholdDistant = worldRenderSettings.m_distantLightStartDistance + m_radius - worldRenderSettings.m_distantLightFadeDistance;

		if( squareDistance > distThresholdDistant * distThresholdDistant )
		{
			const Float distance = Red::Math::MSqrt( squareDistance );
			result.Y = ::Clamp( ( distance - distThresholdDistant ) * m_autoHideInvRange , 0.0f , 1.0f );
		}
	}

	return result;
}
	
SLightFactorInfo IRenderProxyLight::GetDistanceInfo( const Vector &cameraPosition, const SWorldRenderSettings &worldRenderSettings, const CEnvColorGroupsParametersAtPoint &colorGroupsParams, Bool allowDistantLight ) const
{
	const Vector2 fadeCoef = CalculateAutoHideFactor( cameraPosition, worldRenderSettings, allowDistantLight );

	SLightFactorInfo result;
	result.m_isDistantLight = fadeCoef.Y * fadeCoef.X > 0.0f;
	// No need to perform all this heavy vector-based computation if the distans simply is out ;)
	result.m_isBlack = fadeCoef.X * ( 1.0f - fadeCoef.Y ) < 0.01f;
	// Black light
	// if we have flickering info, we need to get collected, even if the light color is temporarily black because the update is in the collected tick
	if( !result.m_isBlack && !m_lightFlickerInfo )
	{
		result.m_isBlack = GetFinalColorNoFade( colorGroupsParams ).SquareMag3() <= 0.01f;
	}
	return result;
}

//! Update param
void IRenderProxyLight::UpdateParameter( const CName& name, Float param )
{
	Bool needsEnvProbeLightUpdate = false;

	if ( name == CNAME( brightness ) || name == CNAME( Brightness ) )
	{
		m_brightness = param;
	}
	else if ( name == CNAME( attenuation ) )
	{
		m_attenuation = param;
	}
	else if ( name == CNAME( shadowBlendFactor ) )
	{
		m_shadowBlendFactor = Clamp( param, 0.f, 1.f );
	}
	else if ( name == CNAME( radius ) || name == CNAME( Radius ) )
	{	
		needsEnvProbeLightUpdate = needsEnvProbeLightUpdate || (m_radius != param);
		m_radius = param;
	}
	else if ( name == CNAME( autoHideDistance ) )
	{
		m_autoHideDistance = Max( 0.f, param );
		m_autoHideDistanceSquared = m_autoHideDistance * m_autoHideDistance;
	}
	else if ( name == CNAME( autoHideRange ) )
	{
		m_autoHideRange = Max( 0.001f, param );
		m_autoHideInvRange = 1.f / m_autoHideRange;
	}
	else if ( name == CNAME( shadowFadeDistance ) )
	{
		m_shadowFadeDistance = Max( 0.f, param );
		if ( m_shadowFadeDistance <= 0.0f ) m_shadowFadeDistance = FLT_MAX; // extend
		m_shadowFadeDistance += Max( 0.0001f, 1.0f/m_shadowFadeInvRange );
	}
	else if ( name == CNAME( shadowFadeRange ) )
	{
		m_shadowFadeInvRange = 1.f / Max( 0.001f, param );
	}

	if ( needsEnvProbeLightUpdate && m_scene && -1 != m_sceneEnvProbeLightIndex )
	{
		m_scene->UpdateEnvProbeLightParameters( *this );
	}
}

void IRenderProxyLight::UpdateLightParameter( const Vector& data, ERenderLightParameter param )
{
	if ( param == RLP_Attenuation )
	{
		m_attenuation = data.X;
	}
	else if ( param == RLP_Brightness )
	{
		m_brightness = data.X;
	}
	else if ( param == RLP_Radius )
	{
		m_radius = data.X;
	}
	else if ( param == RLP_Color )
	{
		m_color = Color( data );
	}
	else if ( param == RLP_AllowDistantFade )
	{
		m_allowDistantFade = data.X > 0.5f;
	}
}

//! Update color
void IRenderProxyLight::UpdateColor( Color color )
{
	m_color = color;
}

void IRenderProxyLight::AttachToScene( CRenderSceneEx* scene )
{
	// if we are attached to a scene that is not a world scene we cannot cache static shadows
	if ( !scene->IsWorldScene() )
	{
		m_staticShadowsCached = false;
	}

	IRenderProxyBase::AttachToScene( scene );
}

void IRenderProxyLight::UpdateFlickering()
{
	ASSERT( m_lightFlickerInfo );

	const Float timeDeltaScaled = GetRenderer()->GetTimeScale() * GetRenderer()->GetLastTickDelta();

	if ( m_lightFlickerInfo )
	{
		PC_SCOPE_PIX( LightProxy_CollectedTick );

		m_lightFlickerInfo->m_flickerTimeCounter += timeDeltaScaled;	

		if ( m_lightFlickerInfo->m_flickerTimeCounter > m_lightFlickerInfo->m_flickerPeriod )
		{
			m_lightFlickerInfo->m_flickerTimeCounter = 0.f;

			m_lightFlickerInfo->m_prevMultiplier = m_lightFlickerInfo->m_nextMultiplier;
			m_lightFlickerInfo->m_nextMultiplier = 1.0f - GEngine->GetRandomNumberGenerator().Get< Float >( m_lightFlickerInfo->m_flickeringStrength );
		}	

		// Could be exposed to flickering properties, but tests shows that this value works pretty fine for our assets
		const Float speed = 0.25f;

		m_lightFlickerInfo->m_oscillatingTimeDelta += speed*timeDeltaScaled;
		if( m_lightFlickerInfo->m_oscillatingTimeDelta > 1.0f ) m_lightFlickerInfo->m_oscillatingTimeDelta = 0.0f;

		Float t = 2.0f*m_lightFlickerInfo->m_oscillatingTimeDelta;		
		
		if( t > 1.0f )
		{
			m_lightFlickerInfo->m_nextOffset = DoubleCrossBezierFunction( t-1.0f, 1.0f, m_lightFlickerInfo->m_maxOffset );
		}
		else
		{
			m_lightFlickerInfo->m_nextOffset = DoubleCrossBezierFunction( t, -1.0f, 2.0f*m_lightFlickerInfo->m_maxOffset );		
		}
		
	}
}

void IRenderProxyLight::CalcOffsetVector( Vector& outVector ) const
{
	if ( m_lightFlickerInfo )
	{
		outVector = m_lightFlickerInfo->m_nextOffset;
		return;
	}

	outVector = Vector::ZEROS;
}

Float IRenderProxyLight::CalcBrightnessMultiplier() const
{
	if ( m_lightFlickerInfo )
	{
		return Lerp( m_lightFlickerInfo->m_flickerTimeCounter / m_lightFlickerInfo->m_flickerPeriod, m_lightFlickerInfo->m_prevMultiplier, m_lightFlickerInfo->m_nextMultiplier );
	}

	return 1.0f;
}

Vector IRenderProxyLight::GetFinalColor( const Vector &cameraPosition, const SWorldRenderSettings &worldRenderSettings, const CEnvColorGroupsParametersAtPoint &colorGroupsParams, Bool allowDistantLight ) const
{
	Vector2 fadeCoef = CalculateAutoHideFactor( cameraPosition, worldRenderSettings, allowDistantLight );
	return GetFinalColorNoFade( colorGroupsParams ) * ( fadeCoef.X * ( 1.0f - fadeCoef.Y ) );
}

Vector IRenderProxyLight::GetFinalColorNoFade( const CEnvColorGroupsParametersAtPoint &colorGroupsParams ) const
{
	const Vector lightGroupColor = colorGroupsParams.GetCurveForColorGroup(m_envColorGroup).GetColorScaledGammaToLinear( true );
	const Vector lightColor = m_color.ToVector();
	const Float brightness = CalcBrightnessMultiplier() * m_brightness;
	const Float dissolve = GetGenericFadeFraction();

	return ( lightColor*lightColor * brightness * dissolve ) * lightGroupColor;
}

Vector IRenderProxyLight::GetBasePosition() const
{
	return m_localToWorld.GetTranslation();
}

Vector IRenderProxyLight::GetFinalPosition() const
{
	Vector offset;
	CalcOffsetVector( offset );
	return m_localToWorld.GetTranslation() + offset;
}

void IRenderProxyLight::ReduceDynamicShadowmapSize()
{
	// note: shadow border is taken into account here
	const Uint16 baseResolution = m_currentShadowResolution + 8;
	if ( baseResolution > 64 )
	{
		m_currentShadowResolution = (baseResolution/2) - 8;
	}
}

Bool IRenderProxyLight::UpdateShadowParams( const CRenderCollector& collector )
{
	// Should we troubble ourselves with dynamic shadows
	const Bool requiresDynamicShadows = m_isCastingDynamicShadows | m_isCastingStaticShadows;
	if ( requiresDynamicShadows )
	{
		// Get base resolution
		ASSERT( RPT_PointLight == GetType() || RPT_SpotLight == GetType() );
		Uint16 baseResolution = (Uint16)( RPT_PointLight == GetType() ? Config::cvMaxCubeShadowSize.Get() :  Config::cvMaxSpotShadowSize.Get() );
		ASSERT( baseResolution >= 16 );

		// Determine required shadow resolution
		const Vector position = GetLocalToWorld().GetTranslation();
		const Float realDistance = collector.GetLodRenderCamera().GetPosition().DistanceTo( position );

		// Is the shadow in range ?
		if ( realDistance < m_shadowFadeDistance )
		{
			// Calculate the resolution
			Float distance = realDistance / GetRadius();
			while ( distance > 1.2f && baseResolution > 64 )
			{
				baseResolution /= 2;
				distance /= 2.0f;
			}

			// Save
			RED_ASSERT( m_shadowBlendFactor > 0.f && m_shadowBlendFactor <= 1.f );
			m_currentShadowAlpha = m_shadowBlendFactor * ((m_shadowFadeDistance < FLT_MAX) ? Clamp<Float>( ( m_shadowFadeDistance - realDistance ) * m_shadowFadeInvRange, 0.0f, 1.0f ) : 1.0f);
			m_currentShadowResolution = baseResolution - 8; // border size
			return true;
		}
	}

	// No shadow casting
	m_currentShadowAlpha = 0.0f;
	m_currentShadowResolution = 0;
	return false;
}

bool TestSphereBoxIntersection( const Vector& sphereCenter, float sphereRadius, const Vector& boxCenter, const Vector& boxExtents )
{
	// Test for intersection in the coordinate system of the box by
	// transforming the sphere into that coordinate system.
	Vector kCDiff = sphereCenter - boxCenter;

	float fAx = fabs( kCDiff.X );
	float fAy = fabs( kCDiff.Y );
	float fAz = fabs( kCDiff.Z );
	float fDx = fAx - boxExtents.X;
	float fDy = fAy - boxExtents.Y;
	float fDz = fAz - boxExtents.Z;

	if (fAx <= boxExtents.X)
	{
		if (fAy <= boxExtents.Y)
		{
			if (fAz <= boxExtents.Z)
			{
				// sphere center inside box
				return true;
			}
			else
			{
				// potential sphere-face intersection with face z
				return fDz <= sphereRadius;
			}
		}
		else
		{
			if (fAz <= sphereRadius)
			{
				// potential sphere-face intersection with face y
				return fDy <= sphereRadius;
			}
			else
			{
				// potential sphere-edge intersection with edge formed
				// by faces y and z
				float fRSqr = sphereRadius*sphereRadius;
				return fDy*fDy + fDz*fDz <= fRSqr;
			}
		}
	}
	else
	{
		if (fAy <= boxExtents.Y)
		{
			if (fAz <= boxExtents.Z)
			{
				// potential sphere-face intersection with face x
				return fDx <= sphereRadius;
			}
			else
			{
				// potential sphere-edge intersection with edge formed
				// by faces x and z
				float fRSqr = sphereRadius * sphereRadius;
				return fDx*fDx + fDz*fDz <= fRSqr;
			}
		}
		else
		{
			if (fAz <= boxExtents.Z)
			{
				// potential sphere-edge intersection with edge formed
				// by faces x and y
				float fRSqr = sphereRadius*sphereRadius;
				return fDx*fDx + fDy*fDy <= fRSqr;
			}
			else
			{
				// potential sphere-vertex intersection at corner formed
				// by faces x,y,z
				float fRSqr = sphereRadius*sphereRadius;
				return fDx*fDx + fDy*fDy + fDz*fDz <= fRSqr;
			}
		}
	}
}

bool TestConeSphereIntersection( const Vector& coneVertex, const Vector& coneAxis, float coneCosSqr, float coneSinSqr, float coneInvSin, const Vector& sphereCenter, float sphereRadius )
{
	Vector U = coneVertex - coneAxis * ( sphereRadius * coneInvSin );
	Vector D = sphereCenter - U;
	float dsqr = Vector::Dot3( D, D );
	float e = Vector::Dot3( coneAxis, D );
	if ( e > 0.0f && e*e >= dsqr * coneCosSqr )
	{
		D = sphereCenter - coneVertex;
		dsqr = Vector::Dot3( D, D );
		e = -Vector::Dot3( coneAxis, D );
		if ( e > 0.0f && e*e >= dsqr * coneSinSqr )
		{
			return dsqr <= sphereRadius * sphereRadius;
		}
		else
		{
			return true;
		}
	}
	return false;
}

bool TestSphereSphereIntersection( const Vector& sphereCenter0, float sphereRadius0, const Vector& sphereCenter1, float sphereRadius1 )
{
	Float d    = (sphereCenter0 - sphereCenter1).SquareMag3();
	Float dmax = (sphereRadius0 + sphereRadius1) * (sphereRadius0 + sphereRadius1);
	return d <= dmax;
}



