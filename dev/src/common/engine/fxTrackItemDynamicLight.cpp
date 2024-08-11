/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "fxTrackItemDynamicLight.h"
#include "lightComponent.h"
#include "spotLightComponent.h"
#include "pointLightComponent.h"
#include "fxState.h"
#include "entity.h"
#include "layer.h"

IMPLEMENT_ENGINE_CLASS( CFXTrackItemDynamicLight );

RED_DEFINE_STATIC_NAME( DynamicLight );

//////////////////////////////////////////////////////////////////////////////////////////////////

/// Runtime player for particles
class CFXTrackItemDynamicLightPlayData : public IFXTrackItemPlayData
{
public:
	const CFXTrackItemDynamicLight*		m_trackItem;		//!< Data
	THandle< CLightComponent >			m_light;			//!< Controlled particles component
	THandle< CEntity >					m_entity;			//!< Created entity
	Float								m_baseRadius;		//!< Starting radius
	Float								m_baseBrightness;	//!< Starting brightness

public:
	CFXTrackItemDynamicLightPlayData( const CFXTrackItemDynamicLight* trackItem, CLightComponent* light, CEntity* entity )
		: IFXTrackItemPlayData( light, trackItem )
		, m_baseRadius( light->GetRadius() )
		, m_baseBrightness( light->GetBrightness() )
		, m_trackItem( trackItem )
		, m_light( light )
		, m_entity( entity )
	{
	};

	~CFXTrackItemDynamicLightPlayData()
	{
		CLightComponent* light = m_light.Get();
		if ( light )
		{
			light->SetBrightness( 0.0f );
			light->SetRadius( 0.0f );
		}

		// Destroy entity
		CEntity* entity = m_entity.Get();
		if ( entity )
		{
			entity->Destroy();
			entity = NULL;
		}
	}

	virtual void OnStop()
	{
	}

	virtual void OnTick( const CFXState& fxState, Float timeDelta )
	{
		CLightComponent* lc = m_light;
		if ( lc )
		{
			// Evaluate curve value
			const Float brightness = m_trackItem->GetCurveValue( 0, fxState.GetCurrentTime() ) * m_baseBrightness;
			const Float radius = m_trackItem->GetCurveValue( 1, fxState.GetCurrentTime() ) * m_baseRadius;

			lc->SetBrightness( Max< Float >( 0.0f, brightness ) );
			lc->SetRadius( Max< Float >( 0.0f, radius ) );
		}
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////

CFXTrackItemDynamicLight::CFXTrackItemDynamicLight()
	: CFXTrackItemCurveBase( 2, CNAME( DynamicLight ) )
	, m_color( Color::WHITE )
	, m_radius( 5.0f )
	, m_attenuation( 2.0f )
	, m_brightness( 1.0f )
	, m_specularScale( 1.0f )
	, m_lightChannels( LC_Default | LC_Characters )
	, m_autoHideDistance( 15.0f )
	, m_autoHideRange( 5.0f )
	, m_colorGroup( ECG_Default )
	, m_isSpotlight( false )
	, m_spotInnerAngle( 80.0f )
	, m_spotOuterAngle( 90.0f )
{
}

String CFXTrackItemDynamicLight::GetCurveName( Uint32 i /*= 0*/ ) const
{
	if ( i == 0 ) return TXT("BrightnessMul");
	if ( i == 1 ) return TXT("RadiusMul");
	return String::EMPTY;
}

Bool CFXTrackItemDynamicLight::UsesComponent( const CName& componentName ) const
{
	return m_spawner && m_spawner->UsesComponent( componentName );
}

IFXTrackItemPlayData* CFXTrackItemDynamicLight::OnStart( CFXState& fxState ) const
{
	// No spawner
	if ( !m_spawner )
	{
		WARN_ENGINE( TXT("No spawner for dynamic light track '%ls' in effect '%ls' in '%ls'"), GetName().AsChar(), fxState.GetDefinition()->GetName().AsString().AsChar(), fxState.GetEntity()->GetFriendlyName().AsChar() );
		return NULL;
	}

	// Calculate spawn position and rotation
	EntitySpawnInfo einfo;
	if ( !m_spawner->Calculate( fxState.GetEntity(), einfo.m_spawnPosition, einfo.m_spawnRotation, 0 ) )
	{
		WARN_ENGINE( TXT("Invalid spawner for dynamic light track '%ls' in effect '%ls' in '%ls'"), GetName().AsChar(), fxState.GetDefinition()->GetName().AsString().AsChar(), fxState.GetEntity()->GetFriendlyName().AsChar() );
		return NULL;
	}

	// Create entity
	CEntity* entity = fxState.CreateDynamicEntity( einfo );
	if ( !entity )
	{
		WARN_ENGINE( TXT("Unable to create dynamic light for track '%ls' in effect '%ls' in '%ls'"), GetName().AsChar(), fxState.GetDefinition()->GetName().AsString().AsChar(), fxState.GetEntity()->GetFriendlyName().AsChar() );
		return NULL;
	}
	
	// Create particles component
	CLightComponent* lc = NULL;
	if ( m_isSpotlight )
	{
		lc = Cast< CLightComponent >( entity->CreateComponent( ClassID< CSpotLightComponent >(), SComponentSpawnInfo() ) );
		((CSpotLightComponent*)lc)->SetInnerAngle( m_spotInnerAngle );
		((CSpotLightComponent*)lc)->SetOuterAngle( m_spotOuterAngle );
	}
	else
	{
		lc = Cast< CLightComponent >( entity->CreateComponent( ClassID< CPointLightComponent >(), SComponentSpawnInfo() ) );
	}
	if ( !lc )
	{
		ASSERT( lc && "Particles component not created" );
		entity->Destroy();
		return NULL;
	}

	// Initialize the light
	lc->SetCastingShadows( m_isCastingShadows );
	lc->SetBrightness( m_brightness );
	lc->SetColor( m_color );
	lc->SetRadius( m_radius );
	//dex++
	lc->SetAutoHideDistance( m_autoHideDistance, m_autoHideRange );
	//dex--
	lc->m_envColorGroup = m_colorGroup;
	lc->m_lightFlickering = m_lightFlickering;

	// Update post spawn
	m_spawner->PostSpawnUpdate( fxState.GetEntity(), lc, 0 );
	entity->ForceUpdateTransformNodeAndCommitChanges();
	lc->ForceUpdateTransformNodeAndCommitChanges();
	lc->RefreshRenderProxies();

	// Done
	return new CFXTrackItemDynamicLightPlayData( this, lc, entity );
}

//////////////////////////////////////////////////////////////////////////////////////////////////
