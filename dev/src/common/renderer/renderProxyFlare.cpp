/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "renderElementMap.h"
#include "renderProxyFlare.h"
#include "renderQuery.h"
#include "renderScene.h"
#include "renderMaterial.h"
#include "renderSkybox.h"
#include "../engine/flareComponent.h"
#include "../engine/materialInstance.h"


Vector CRenderProxy_Flare::CalculateFlareSourcePosition( const CRenderProxy_Flare &flare, const CRenderFrameInfo &info )
{
	Vector flareSourcePosition;

	if ( FLARECAT_Sun == flare.GetParameters().m_category )
	{
		flareSourcePosition = info.m_camera.GetPosition() + info.m_envParametersDayPoint.m_sunDirection * CRenderSkybox::SUN_AND_MOON_RENDER_DIST;
	}
	else if ( FLARECAT_Moon == flare.GetParameters().m_category )
	{
		flareSourcePosition = info.m_camera.GetPosition() + info.m_envParametersDayPoint.m_moonDirection * CRenderSkybox::SUN_AND_MOON_RENDER_DIST;
	}
	else
	{
		flareSourcePosition = flare.GetLocalToWorld().GetTranslation();
	}

	flareSourcePosition.W = 1.f;
	return flareSourcePosition;
}

Float CRenderProxy_Flare::GetFlareCategoryScaleBase( EFlareCategory category )
{
	return FLARECAT_Sun == category || FLARECAT_Moon == category ? CRenderSkybox::SUN_AND_MOON_RENDER_SCALE_BASE : 1.f;
}

Matrix CRenderProxy_Flare::CalculateFlareOcclusionMatrix( const CRenderProxy_Flare &flare, const CRenderFrameInfo &info )
{
	Float extentScale = flare.GetParameters().m_occlusionExtent * GetFlareCategoryScaleBase( flare.GetParameters().m_category );

	Matrix matrix;
	matrix.SetIdentity();
	matrix.SetTranslation( CalculateFlareSourcePosition( flare, info ) );
	matrix.SetScale33( extentScale );
	return matrix;
}

Float CRenderProxy_Flare::GetFlareCategorySize( EFlareCategory category, const CRenderFrameInfo &info )
{
	Float size = 1.f;

	switch ( category )
	{
	case FLARECAT_Default:		size = 1.f; break;
	case FLARECAT_Sun:			size = info.m_envParametersArea.m_sunParams.m_sunFlareSize.GetScalarClampMin( 0.f ); break;
	case FLARECAT_Moon:			size = info.m_envParametersArea.m_sunParams.m_moonFlareSize.GetScalarClampMin( 0.f ); break;
	default:					ASSERT( !"Invalid" ); break;
	};

	size *= CRenderProxy_Flare::GetFlareCategoryScaleBase( category );

	return size;
}


CRenderProxy_Flare::SFlareState::SFlareState ()
	: m_currentAlpha( 0.f )
	, m_currentAlphaSpeed( 0.f )
	, m_occlusionTestActiveState( -2 )
	, m_targetAlpha( 0.f )
	, m_queryRefFull( NULL )
	, m_queryRefPart( NULL )
	, m_useMultipleQueryObjects( false )
{}

CRenderProxy_Flare::SFlareState::~SFlareState ()
{
	Shut();
}

Bool CRenderProxy_Flare::SFlareState::IsInit() const
{
	return m_queryRefFull && m_queryRefPart;
}

Bool CRenderProxy_Flare::SFlareState::Init( Bool useMultipleQueryObjects )
{
	if ( IsInit() && useMultipleQueryObjects == m_useMultipleQueryObjects )
	{
		return true;
	}

	Shut();

	m_queryRefFull = new CRenderQuery( GpuApi::QT_Occlusion, !useMultipleQueryObjects );
	m_queryRefPart = new CRenderQuery( GpuApi::QT_Occlusion, !useMultipleQueryObjects );

	if ( !(m_queryRefFull->IsValid() && m_queryRefPart->IsValid()) )
	{
		ASSERT( !"Failed to init flare state" );
		return false;
	}

	m_useMultipleQueryObjects = useMultipleQueryObjects;

	ASSERT( IsInit() );
	return true;
}

void CRenderProxy_Flare::SFlareState::Shut()
{
	SAFE_RELEASE( m_queryRefPart );
	SAFE_RELEASE( m_queryRefFull );

	m_currentAlpha = 0.f;
	m_currentAlphaSpeed = 0.f;
	m_targetAlpha = 0.f;
	m_occlusionTestActiveState = -2;
}

void CRenderProxy_Flare::SFlareState::NotifyOcclusionTestsActive( Bool isActive )
{
	if ( isActive )
	{
		if ( 0 == m_occlusionTestActiveState )
		{
			m_occlusionTestActiveState = -2;
		}
	}
	else
	{
		m_occlusionTestActiveState = 0;
	}
}

CRenderQuery* CRenderProxy_Flare::SFlareState::RequestQueryToDraw( Bool fullShapePass )
{
	if ( IsInit() )
	{
		// hacky: assumes that there will be two queries rendered one by one.

		if ( m_useMultipleQueryObjects )
		{
			if ( 0 != m_occlusionTestActiveState )
			{
				m_occlusionTestActiveState = 1;
				return fullShapePass ? m_queryRefFull : m_queryRefPart;
			}
		}
		else
		{
			if ( m_occlusionTestActiveState < 0 )
			{
				ASSERT( !(fullShapePass && -2 != m_occlusionTestActiveState) );
				ASSERT( !(!fullShapePass && -1 != m_occlusionTestActiveState) );
				++m_occlusionTestActiveState;
				if ( 0 == m_occlusionTestActiveState )
					m_occlusionTestActiveState = 1;

				return fullShapePass ? m_queryRefFull : m_queryRefPart;
			}
		}
	}

	return NULL;
}

void CRenderProxy_Flare::SFlareState::SimulateAlpha( const SFlareParameters &params, Float timeDelta )
{
	if ( timeDelta <= 0.f )
	{
		return;
	}

	if ( m_targetAlpha == m_currentAlpha )
	{
		m_currentAlphaSpeed = 0.f;
		return;
	}

	Float speed = m_currentAlphaSpeed;
	Float alpha = m_currentAlpha;

	if ( m_targetAlpha > alpha )
	{
		speed = Min( params.m_fadeInMaxSpeed, Max(0.f, speed) + params.m_fadeInAccel * timeDelta );
		alpha = Clamp( alpha + speed * timeDelta, 0.f, m_targetAlpha );
	}
	else if ( m_targetAlpha < alpha )
	{
		speed = Max( -params.m_fadeOutMaxSpeed, Min(0.f, speed) - params.m_fadeOutAccel * timeDelta );
		alpha = Clamp( alpha + speed * timeDelta, m_targetAlpha, 1.f );
	}
	else
	{
		speed = 0.f;
	}

	m_currentAlphaSpeed = speed;
	m_currentAlpha = alpha;
}

void CRenderProxy_Flare::SFlareState::Update( const SFlareParameters &params, Float timeDelta )
{
	if ( !IsInit() )
	{
		return;
	}

	if ( 1 == m_occlusionTestActiveState )
	{
		Uint64 fullCount = 0;
		Uint64 partCount = 0;
		EQueryResult queryResultFull = m_queryRefFull->GetQueryResult( fullCount, false );
		EQueryResult queryResultPart = m_queryRefPart->GetQueryResult( partCount, false );

		if ( EQR_Success == queryResultPart && EQR_Success == queryResultFull )
		{
			const Uint64 samplesThreshold = 15;
			const Float samplesInvRange = 1.f / 30.f;
			if ( partCount > samplesThreshold && fullCount > samplesThreshold )
			{
				m_targetAlpha = Clamp( partCount / (Float)fullCount, 0.f, 1.f );
				m_targetAlpha *= Min( 1.f, (fullCount - samplesThreshold) * samplesInvRange );
			}
			else
			{
				m_targetAlpha = 0.f;
			}

			if ( !m_useMultipleQueryObjects )
			{
				m_occlusionTestActiveState = -2;
			}
		}
	}
	else if ( 0 == m_occlusionTestActiveState )
	{
		m_targetAlpha = 0.f;
	}

	// Simulate alpha
	SimulateAlpha( params, timeDelta );
}


CRenderProxy_Flare::CRenderProxy_Flare( const RenderProxyInitInfo& initInfo )
	: IRenderProxyDrawable( RPT_Flare, initInfo )
	, m_materialParams( NULL )
	, m_material( NULL )
	, m_activeFlareIndex( -1 )
{
	if ( initInfo.m_component && initInfo.m_component->IsA< CFlareComponent >() )
	{
		const CFlareComponent* fc = static_cast< const CFlareComponent* >( initInfo.m_component );

		// Import material
		IMaterial *mat = fc->GetMaterial();
		if ( mat && mat->GetMaterialDefinition() && mat->GetMaterialDefinition()->GetRenderResource() && mat->GetRenderResource() )
		{
			m_material = static_cast< CRenderMaterial* >( mat->GetMaterialDefinition()->GetRenderResource() );
			m_material->AddRef();
			
			m_materialParams = static_cast< CRenderMaterialParameters* >( mat->GetRenderResource() );
			m_materialParams->AddRef();
		}

		// Import parameters
		m_parameters = fc->GetParameters();
	}
}

CRenderProxy_Flare::~CRenderProxy_Flare()
{	
	m_state.Shut();

	if ( m_materialParams )
	{
		m_materialParams->Release();
		m_materialParams = NULL;
	}

	if ( m_material )
	{
		m_material->Release();
		m_material = NULL;
	}
}


void CRenderProxy_Flare::Prefetch( CRenderFramePrefetch* prefetch ) const
{
	// When rendering, we use distance 0 because they're screenspace.
	// TODO : Check if that's proper, or if we can set better distance for better texture streaming.
	const Float distanceSq = 0.0f;
	prefetch->AddMaterialBind( m_material, m_materialParams, distanceSq );
}


void CRenderProxy_Flare::CollectElements( CRenderCollector& collector )
{
	// update stuff that should be updated only once per frame
	UpdateOncePerFrame( collector );

	// check auto hide distance visibility
	if ( !IsVisibleInCurrentFrame() )
		return;

	if ( collector.m_scene )
	{
		// ace_todo: ace_optimize: in case no lens flare or flare is configured, then don't take given factor into account.
		/*
		if ( GetFlareCategoryAlpha( GetParameters().m_colorGroup, collector.GetRenderFrameInfo() ) <= 0 && GetLensFlareCategoryAlpha( GetParameters().m_colorGroup, collector.GetRenderFrameInfo() ) <= 0 ) 
		{
			return;
		}
		*/

		// ace_todo: ace_optimize: xxxxxxxxxxxx don't collect if beyond distance where it's actually visible (e.g. no centered flare, only the lens flare with limited visibility)
		//m_lastCollectedFrame = collector.m_scene->GetLastAllocatedFrame();
		collector.m_scene->SetupActiveFlare( this, true );
	}
}

void CRenderProxy_Flare::DetachFromScene( CRenderSceneEx* scene )
{
	scene->SetupActiveFlare( this, false );
	m_state.Shut();

	IRenderProxyDrawable::DetachFromScene( scene );	
}

void CRenderProxy_Flare::DrawOcclusion( CRenderSceneEx* scene, Bool fullShapePass, const CRenderFrameInfo& info )
{
	if ( !m_state.IsInit() )
	{
		if ( !scene )
		{
			return;
		}

		const Bool useMultipleQueries = FLARECAT_Sun == GetParameters().m_category || FLARECAT_Moon == GetParameters().m_category; //< Better queries feedback for important queries (smoother feedback)
		if ( m_state.Init( useMultipleQueries ) )
		{
			// empty
		}
		else
		{
			return;
		}
	}

	//
	const Bool isPotentiallyVisible = m_frameTracker.GetLastUpdateFrameIndex() == scene->GetLastAllocatedFrame();
	m_state.NotifyOcclusionTestsActive( isPotentiallyVisible );

	// Get appropriate query (will return null in case flare is forced to be invisible by the func call above)
	CRenderQuery* query = m_state.RequestQueryToDraw( fullShapePass );

	// Run query
	if ( query )
	{
		query->BeginQuery();
		DrawOcclusionShape( info );
		query->EndQuery();
	}
}

void CRenderProxy_Flare::DrawOcclusionShape( const CRenderFrameInfo& info )
{
	Matrix matrix = CalculateFlareOcclusionMatrix( *this, info );
	GetRenderer()->GetStateManager().SetLocalToWorld( &matrix );

	GpuApi::DrawIndexedPrimitive( GpuApi::PRIMTYPE_TriangleList, 0, 0, 8, 0, 12 );
}

void CRenderProxy_Flare::Update( CRenderSceneEx* scene, const CRenderCamera &camera, const CFrustum &frustum, Float timeDelta )
{
	ASSERT( scene );

	if ( !m_state.IsInit() )
	{
		return;
	}

	m_state.Update( m_parameters, timeDelta );

	// Clamp alpha based on distance to camera frustum
	if ( FLARECAT_Default == m_parameters.m_category )
	{
		const Vector flarePos = GetLocalToWorld().GetTranslation();

		Float alphaLimit = 1.f;

		// Process visibility
		{
			Float distSq = camera.GetPosition().DistanceSquaredTo( flarePos );
			if ( distSq >= m_parameters.m_visibilityFullDist * m_parameters.m_visibilityFullDist )
			{
				alphaLimit *= Clamp( 1.f - (sqrtf( distSq ) - m_parameters.m_visibilityFullDist) / Max( 0.001f, m_parameters.m_visibilityFadeRange ), 0.f, 1.f );
			}
		}

		// Process frustum
		{
			const Float appearDist = 0.f;
			const Float appearInvRange = 1.f / Max( 0.001f, (1.75f * m_parameters.m_occlusionExtent * CRenderProxy_Flare::GetFlareCategoryScaleBase( m_parameters.m_category ) ) );
			const Float dist = ( frustum.GetPointMinDistance( flarePos ) );;
			alphaLimit *= Clamp( (dist -  appearDist) * appearInvRange, 0.f, 1.f );
		}

		m_state.m_targetAlpha = Min( m_state.m_targetAlpha, alphaLimit );
		m_state.m_currentAlpha = Min( m_state.m_currentAlpha, alphaLimit );
	}

	const Bool isPotentiallyVisible = m_frameTracker.GetLastUpdateFrameIndex() == scene->GetLastAllocatedFrame();
	if ( !m_state.IsInit() || ( !isPotentiallyVisible && 0 == m_state.m_currentAlpha ) )
	{
		scene->SetupActiveFlare( this, false );
	}
}

Bool CRenderProxy_Flare::Register( CRenderElementMap* reMap )
{
	if ( !(FLARECAT_Sun == GetParameters().m_category || FLARECAT_Moon == GetParameters().m_category) )
	{
		return IRenderProxyBase::Register( reMap );
	}

	return false;
}

Bool CRenderProxy_Flare::Unregister( CRenderElementMap* reMap )
{
	if ( !(FLARECAT_Sun == GetParameters().m_category || FLARECAT_Moon == GetParameters().m_category) )
	{
		return IRenderProxyBase::Unregister( reMap );
	}

	return false;
}
