/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "renderProxyDimmer.h"
#include "renderCollector.h"
#include "renderScene.h"
#include "renderVisibilityQueryManager.h"
#include "../engine/dimmerComponent.h"
#include "../engine/renderSettings.h"

extern SceneRenderingStats GRenderingStats;

#define DIMMER_FADE_SPEED	(2.f)

CRenderProxy_Dimmer::CRenderProxy_Dimmer( const RenderProxyInitInfo& initInfo )
	: IRenderProxyDrawable( RPT_Dimmer, initInfo )
	, m_dimmerType ( DIMMERTYPE_Default )
	, m_ambientLevel ( 0.f )
	, m_marginFactor ( 0.f )
	, m_budgetLastFrameIndex ( -1 )
	, m_fadeAlphaBudget ( 1.f )
{
	if ( initInfo.m_component && initInfo.m_component->IsA< CDimmerComponent >() )
	{
		const CDimmerComponent *dc = static_cast< const CDimmerComponent* >( initInfo.m_component );
		m_dimmerType = dc->GetDimmerType();
		m_isMarker = dc->IsAreaMarker();
		m_ambientLevel = dc->GetAmbientLevel();
		m_marginFactor = dc->GetMarginFactor();

#ifdef USE_UMBRA
		m_umbraProxyId = GlobalVisID( dc->GetOcclusionId(), GetLocalToWorld() );
#endif
	}
	else if ( initInfo.m_packedData && initInfo.m_packedData->GetType() == RPT_Dimmer )
	{
		const RenderProxyDimmerInitData* data = static_cast< const RenderProxyDimmerInitData* >( initInfo.m_packedData );
		m_dimmerType = (EDimmerType) data->m_dimmerType;
		m_isMarker = data->m_areaMarker;
		m_ambientLevel = data->m_ambientLevel;
		m_marginFactor = data->m_marginFactor;

#ifdef USE_UMBRA
		m_umbraProxyId = GlobalVisID( data->m_occlusionId, GetLocalToWorld() );
#endif
	}
}

CRenderProxy_Dimmer::~CRenderProxy_Dimmer()
{}

const EFrameUpdateState CRenderProxy_Dimmer::UpdateOncePerFrame( const CRenderCollector& collector )
{
	const auto ret = IRenderProxyDrawable::UpdateOncePerFrame( collector );
	if ( ret == FUS_AlreadyUpdated )
		return ret;

	const Bool wasVisibleLastFrame = ( ret == FUS_UpdatedLastFrame );
	UpdateFade( collector, wasVisibleLastFrame );

	return ret;
}

void CRenderProxy_Dimmer::UpdateFade( const CRenderCollector& collector, const Bool wasVisibleLastFrame )
{
	// Hide/show near autohide distance
	const Float maxMargin = 5.f;
	const Float hideDistFull = GetAutoHideDistance();
	const Float hideDistLimited = Max( 1.f, hideDistFull - maxMargin );
	const Float hideDistLimitedSquared = hideDistLimited * hideDistLimited;

	UpdateDistanceFade( GetCachedDistanceSquared() , hideDistLimitedSquared, wasVisibleLastFrame && !collector.WasLastFrameCameraInvalidated() );
}


void CRenderProxy_Dimmer::OnNotVisibleFromAutoHide( CRenderCollector& collector )
{
	if ( !collector.m_frame->GetFrameInfo().IsShowFlagOn( SHOW_Dimmers ) ) return;

	UpdateOncePerFrame( collector );
}


void CRenderProxy_Dimmer::CollectElements( CRenderCollector& collector )
{
	if ( !collector.m_frame->GetFrameInfo().IsShowFlagOn( SHOW_Dimmers ) ) return;

	// Update once per frame
	UpdateOncePerFrame( collector );

	// check fade visibility
	if ( !IsFadeVisible() )
		return;

	// Update the scene visibility query
	UpdateVisibilityQueryState( CRenderVisibilityQueryManager::VisibleScene );

	// Actual collect
	collector.m_renderCollectorData->m_dimmers.PushBack( this );

#ifndef RED_FINAL_BUILD
	++GRenderingStats.m_numDimmers;
#endif
}

void CRenderProxy_Dimmer::UpdateBudgetFadeAlpha( Float frameTime, Bool isInMainRange )
{
	const Bool inBudgetLastFrame = (m_budgetLastFrameIndex + 1) >= m_frameTracker.GetLastUpdateFrameIndex();	
	m_budgetLastFrameIndex = m_frameTracker.GetLastUpdateFrameIndex();

	if ( isInMainRange )
	{
		m_fadeAlphaBudget = inBudgetLastFrame ? Min( 1.f, m_fadeAlphaBudget + DIMMER_FADE_SPEED * frameTime ) : 1.f;
	}
	else
	{
		m_fadeAlphaBudget = inBudgetLastFrame ? Max( 0.f, m_fadeAlphaBudget - DIMMER_FADE_SPEED * frameTime ) : 0.f;
	}
}
