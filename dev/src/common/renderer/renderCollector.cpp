/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "renderCollector.h"
#include "renderProxyParticles.h"
#include "renderProxyFlare.h"
#include "renderProxyFur.h"
#include "renderProxyMesh.h"
#include "renderProxyLight.h"
#include "renderProxyDimmer.h"
#include "renderProxyDecal.h"
#include "renderProxyApex.h"
#include "renderElementMap.h"
#include "renderShadowManager.h"
#include "renderTerrainShadows.h"
#include "renderElementApex.h"
#include "renderElementMeshChunk.h"
#include "renderScene.h"
#include "renderEntityGroup.h"
#include "renderMeshBatcher.h"
#include "renderApexBatcher.h"
#include "renderSwarmBatcher.h"
#include "renderParticleBatcher.h"
#include "renderFlaresBatcher.h"
#include "renderDynamicDecalBatcher.h"
#include "renderDynamicDecalChunk.h"
#include "renderMaterial.h"
#include "renderVisibilityQueryManager.h"
#include "renderDistantLightBatcher.h"
#include "renderShaderPair.h"
#include "../engine/umbraScene.h"
#include "../engine/baseEngine.h"
#include "../engine/renderFragment.h"
#include "../engine/renderSettings.h"

#ifdef USE_ANSEL
extern Bool isAnselSessionActive;
#endif // USE_ANSEL

namespace Config
{
	TConfigVar<Int32> cvHiResEntityShadowsForceFullRes		( "Rendering", "HiResEntityShadowsForceFullRes",		-1 );
}

#ifndef NO_DEBUG_PAGES

class CRenderVisibleLightsCounter : public IRenderDebugCounter
{
public:
	CRenderVisibleLightsCounter()
		: IRenderDebugCounter( TXT("Visible lights"), 100.f )
	{};

	//! Get the color for drawing the bar
	virtual Color GetColor() const
	{
		const Float max = 100.f;
		if ( m_current < max * 0.5f ) return Color( 0, 128, 0 );
		if ( m_current < max * 0.75f ) return Color( 128, 128, 0 );
		return Color( 128, 0, 0 );
	}

	// Get the text to display
	virtual String GetText() const
	{
		return String::Printf( TXT("Visible lights: %.0f"), m_current );
	}
};

#endif

CRenderCollector::CShadowCullingTask::~CShadowCullingTask()
{}


void CRenderCollector::CShadowCullingTask::Run()
{
	PC_SCOPE_PIX( CShadowCullingTask );

	// Collect drawables for each cascade from prepared data
	m_collector->BuildCascadesElements();
}


CRenderCollector::CSceneCullingTask::~CSceneCullingTask()
{}


void CRenderCollector::CSceneCullingTask::RunToOrWait( TaskPhase phase )
{
	// If we're already past the desired phase, don't need to do anything.
	if ( m_currentPhase.GetValue() > phase )
	{
		return;
	}

	// Try to acquire the mutex. If we succeed, we can run up to the desired phase. If we fail, we just wait until we reach the phase.
	if ( m_runningMutex.TryAcquire() )
	{
		// Run up to the desired phase
		while ( m_currentPhase.GetValue() <= phase )
		{
			RunCurrentPhaseAndAdvance();
		}

		m_runningMutex.Release();
	}
	else
	{
		Int32 currentPhase;
		while ( currentPhase = m_currentPhase.GetValue(), currentPhase <= phase && currentPhase != PHASE_Finished ) {}
	}
}

void CRenderCollector::CSceneCullingTask::Run()
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_runningMutex );

	// Run until the end.
	while ( m_currentPhase.GetValue() < PHASE_Finished )
	{
		RunCurrentPhaseAndAdvance();
	}
}

void CRenderCollector::CSceneCullingTask::RunCurrentPhaseAndAdvance()
{
	switch ( m_currentPhase.GetValue() )
	{
	case PHASE_NotStarted:
		// Nothing to do, just advance.
		break;

	case PHASE_OcclusionQuery:
		m_collector->RunSceneQuery();
		break;

	case PHASE_CollectStaticMeshes:
		m_collector->BuildSceneStatics();
		break;

	case PHASE_CollectScene:
		m_collector->BuildSceneElements();
		break;

	case PHASE_BuildDynamicShadowElements:
		m_collector->BuildDynamicShadowElements();
		break;

	case PHASE_Finished:
		// Finished also does nothing, just need to advance past it, so we can add or move phases around without needing to worry
		// about changing anywhere that makes assumptions about which one is last.
		break;

	default:
		RED_ASSERT( m_currentPhase.GetValue() == PHASE_Finished + 1, TXT("Invalid phase!") );
		// Don't advance anymore. If we've passed Finished, then we're completely done.
		return;
	}

	// Go to next phase.
	m_currentPhase.Increment();
}


void CRenderCollector::SRenderElementContainer::PushMeshChunk( CRenderElement_MeshChunk* meshChunk )
{
	RED_ASSERT( meshChunk->GetProxy()->GetType() == RPT_Mesh, TXT("Collecting mesh chunk, but its proxy is not RPT_Mesh (%i)! Have %i"), RPT_Mesh, meshChunk->GetProxy()->GetType() );
	const Bool isStatic = meshChunk->GetProxy()->GetType() == RPT_Mesh && static_cast< CRenderProxy_Mesh* >( meshChunk->GetProxy() )->IsStaticInRenderElementMap();

	if ( meshChunk->HasFlag( RMCF_IsMaskedMaterial ) || meshChunk->HasFlag( RMCF_UsesUVDissolve ) || meshChunk->GetProxy()->HasClippingEllipse() || meshChunk->HasFlag( RMCF_UsesDissolve ) )
	{
		m_discardMeshes.PushBack( meshChunk );
	}
	else if ( isStatic )
	{
		m_solidStaticMeshes.PushBack( meshChunk );
	}
	else
	{
		m_solidMeshes.PushBack( meshChunk );
	}
}


void CRenderCollector::SRenderElementContainer::Draw( const CRenderCollector &collector, const RenderingContext &context, Uint8 groups ) const
{
	const CRenderFrameInfo &info = collector.GetRenderFrameInfo();
	
	if ( groups & ( RECG_SolidStaticMesh | RECG_SolidMesh | RECG_DiscardMesh ) )
	{
		PC_SCOPE_RENDER_LVL0(RenderMeshes);

		// Render solid static meshes
		if ( groups & RECG_SolidStaticMesh )
		{
			GetRenderer()->GetMeshBatcher()->RenderMeshes( info, context, m_solidStaticMeshes, 0, collector.m_sceneStats );
		}

		// Render solid meshes
		if ( groups & RECG_SolidMesh )
		{
			GetRenderer()->GetMeshBatcher()->RenderMeshes( info, context, m_solidMeshes, 0, collector.m_sceneStats );
		}

		// Render meshes with discards
		if ( groups & RECG_DiscardMesh )
		{
			GetRenderer()->GetMeshBatcher()->RenderMeshes( info, context, m_discardMeshes, 0, collector.m_sceneStats );
		}
	}

#ifdef USE_APEX
	if ( groups & RECG_Apex )
	{
		PC_SCOPE_RENDER_LVL0(RenderApex);
		GetRenderer()->GetApexBatcher()->RenderApex( info, context, m_apex, RABF_All, collector.m_sceneStats );
	}
#endif
	
	if( !context.m_forceNoParticles && ( groups & RECG_Particle ) )
	{
		PC_SCOPE_RENDER_LVL0(RenderParticles);
		GetRenderer()->GetParticleBatcher()->RenderParticles( collector, context, m_particles );
	}

	if( !context.m_forceNoSwarms && ( groups & RECG_Swarm ) )
	{
		PC_SCOPE_RENDER_LVL1(RenderSwarms);
		GetRenderer()->GetSwarmBatcher()->RenderMeshes( info, context, m_swarmMeshes, 0, collector.m_sceneStats );
	}
}

static RED_INLINE Float CalcErrorMetric( const CRenderCamera& lodCamera, const CRenderFrameInfo& frameInfo )
{
	// Assume 720p as the height of the screen
	const Float height = ( Float ) frameInfo.m_height;

	// Get the half-fov
	const Float halfFov = 0.5f * DEG2RAD( lodCamera.GetFOV() );
	const Float nativeErrorMetric = MTan( halfFov ) / height;

	const Float multiplier = Config::cvTerrainErrorMetricMultiplier.Get();		// default is 10

	return nativeErrorMetric * multiplier;
}

static EPostProcessCategoryFlags MapConfigToPostProcessFlags( const CRenderFrameInfo& info )
{
	Uint32 flags = PPCF_DisableAllMask;

	flags |= Config::cvAllowAntialias.Get() ? PPCF_AntiAlias : 0;
	flags |= Config::cvAllowBloom.Get() ? PPCF_Bloom : 0;
	flags |= Config::cvAllowBlur.Get() ? PPCF_Blur : 0;
	flags |= Config::cvAllowDOF.Get() ? PPCF_DepthOfField : 0;
	flags |= Config::cvAllowDOF.Get() ? PPCF_CutsceneDOF : 0;		// AllowDOF if for both, cutsceneDOF and regular DOF
	flags |= Config::cvAllowShafts.Get() ? PPCF_Shafts : 0;
	flags |= Config::cvAllowSharpen.Get() ? PPCF_Sharpen : 0;
	flags |= Config::cvAllowVignette.Get() ? PPCF_Vignette : 0;
	flags |= (Config::cvSSAOVersion.Get()>0) ? PPCF_SSAO : 0;
	flags |= Config::cvAllowMotionBlur.Get() ? PPCF_MotionBlur : 0;
	flags |= Config::cvAllowFog.Get() ? PPCF_Fog : 0;
	flags |= (info.IsShowFlagOn( SHOW_Underwater ) && Config::cvAllowUnderwater.Get() ) ? PPCF_Underwater : 0;
	flags |= Config::cvAllowChromaticAberration.Get() ? PPCF_ChromaticAberration : 0;

	return static_cast< EPostProcessCategoryFlags >( flags );
}

CRenderCollector::CRenderCollector( )
	: m_frameIndex( 100 )
	, m_info( nullptr )
	, m_overlayInfo( nullptr )
	, m_camera( nullptr )
	, m_lodCamera( nullptr )
	, m_frame( nullptr )
	, m_scene( nullptr )
	, m_renderCollectorData( nullptr )
	, m_renderFoliage( true )
	, m_renderTerrain( true )
	, m_renderSunLighting( true )
	, m_renderSky( true )
	, m_hiResShadowLists( nullptr )
	, m_shadowCullingTask( nullptr )
	, m_sceneCullingTask( nullptr )
	, m_disolveFlushed( false )
	, m_usedLightChannels( 0 )
#ifdef USE_UMBRA
	, m_collectWithUmbra( false )
	, m_occlusionData( nullptr )
#endif // USE_UMBRA
{	
	
}

CRenderCollector::~CRenderCollector()
{
	// Validation. Pointers SHOULD be cleared during last Reset on RenderFrame. But if somehow it wont happen
	// and here there will be pointers, shutting down will cause crash due to dangling pointers

	Bool hasAnyPointers = false;
	hasAnyPointers |= ( m_frame != nullptr );
	hasAnyPointers |= ( m_scene != nullptr );
	hasAnyPointers |= ( m_renderCollectorData != nullptr );
	hasAnyPointers |= ( m_info != nullptr );
	hasAnyPointers |= ( m_overlayInfo != nullptr );
	hasAnyPointers |= ( m_camera != nullptr );
	hasAnyPointers |= ( m_lodCamera != nullptr );
	hasAnyPointers |= ( m_hiResShadowLists != nullptr );
#ifdef USE_UMBRA
	hasAnyPointers |= ( m_occlusionData != nullptr );
#endif // USE_UMBRA
		
	RED_FATAL_ASSERT( hasAnyPointers == false , "One of the poiters are still there. Please, investigae !" );

	Reset();
}

void CRenderCollector::Setup( CRenderFrame* frame, CRenderSceneEx* scene, CRenderCollectorData* data, Bool supressSceneRendering /* = false */ )
{
	PC_SCOPE_LV0( RenderCollector_Setup, PBC_CPU )

	m_frameIndex = 100;
	m_renderFoliage = nullptr;
	m_renderTerrain = true;
	m_renderSunLighting = true;
	m_renderSky = true;
	m_hiResShadowLists = nullptr;
	m_shadowCullingTask = nullptr;
	m_sceneCullingTask = nullptr;
#ifdef USE_UMBRA
	m_collectWithUmbra = false;
	m_occlusionData = nullptr;
#endif // USE_UMBRA

	m_frame = frame;
	m_scene = scene;
	m_renderCollectorData = data;
	m_info = &frame->GetFrameInfo();
	m_overlayInfo = &frame->GetFrameOverlayInfo();
	m_camera = &frame->GetFrameInfo().m_camera;
	m_lodCamera = &frame->GetFrameInfo().m_occlusionCamera;

	Red::System::MemorySet( &m_sceneStats, 0, sizeof(m_sceneStats) );
	
	if ( scene )
	{
		scene->AllocateFrame();

		m_disolveFlushed = scene->LatchFadeFinishRequested();

		m_frameIndex = scene->GetLastAllocatedFrame();
		m_dissolveSynchronizer = scene->GetDissolveSynchronizer();
	}	

#ifdef USE_SPEED_TREE
	// SpeedTree requires proper "frame index" for culling
	// We are adding enough here to account for the normal culling and the culling for shadow cascades
	// We could use separate frame for each render proxy but that would be a waste.
	extern Uint32 GGlobalSpeedTreeFrameIndex;
	GGlobalSpeedTreeFrameIndex += (1 + MAX_CASCADES);
#endif

	// read shadow detail settings
	const Float defaultFadeTreshold = 0.05f;
	m_cascadeShadowSizeLimit = defaultFadeTreshold * Config::cvCascadeShadowFadeTreshold.Get();

	// Reset collector data
	ASSERT( m_renderCollectorData );
	m_renderCollectorData->Reset();

	// Calculate error metric for terrain LOD
	if (m_scene)
	{
		m_terrainErrorMetric = CalcErrorMetric( *m_lodCamera, *m_info );
		m_terrainHoleDistance = 100.0f;
	}

#ifdef USE_UMBRA
	if ( m_scene )
	{
		m_occlusionData = m_scene->GetOcclusionData();
		if ( !m_scene->IsTomeCollectionValid() )
		{
			// data invalid - force frustum culling and inform about potential performance drop
			Int32 x = m_frame->GetFrameOverlayInfo().m_width / 2 - 320;
			Int32 y = 150;
			if ( frame->GetFrameInfo().IsShowFlagOn( SHOW_OnScreenMessages ) )
			{
				m_frame->AddDebugScreenText( x, y, TXT("Data from one or more Umbra tiles is corrupted, occlusion system is OFF, this may cause performance issues."), Color::RED );
			}
		}
		m_collectWithUmbra = m_scene->ShouldCollectWithUmbra();

#ifdef USE_ANSEL
		if ( isAnselSessionActive )
		{
			m_collectWithUmbra = false;
		}
#endif // USE_ANSEL
	}
#endif // USE_UMBRA

	m_renderFoliage = frame->GetFrameInfo().IsShowFlagOn( SHOW_Foliage );
	m_renderTerrain = frame->GetFrameInfo().IsShowFlagOn( SHOW_Terrain );
	m_renderSunLighting = frame->GetFrameInfo().IsShowFlagOn( SHOW_Lighting ) && RM_Shaded == m_info->m_renderingMode && (m_info->m_envParametersArea.m_globalLight.m_globalLightActivated > 0.05f);
	m_renderSky = frame->GetFrameInfo().IsShowFlagOn( SHOW_Lighting );

	// setup visibility filter
	if ( scene != nullptr )
	{
		scene->GetVisibilityExclusionMap()->PrepareForQueries();
		m_visibilityFilter.Setup( *scene->GetVisibilityExclusionMap() );
	}

	// cache some flags
	m_renderShadows = m_info->IsShowFlagOn( SHOW_Shadows );

	// LOD drop stuff
	if ( m_info->m_envParametersGame.m_cutsceneOrDialog )
	{
		m_numberOfMeshLodsToDrop = 0;
		m_numberOfCharacterLodsToDrop = 0;
	}
	else
	{
		m_numberOfMeshLodsToDrop = Config::cvMeshLodGameplayDownscale.Get();
		m_numberOfCharacterLodsToDrop = Config::cvCharacterLodGameplayDownscale.Get();
	}

	// always reset the shadow manager
	GetRenderer()->GetShadowManager()->ResetStaticPacking();
	GetRenderer()->GetShadowManager()->ResetDynamicPacking();


	// Collect scene elements to draw
	if ( scene && !supressSceneRendering )
	{
		// Update pending fadeout removals
		ASSERT( m_frameIndex == scene->GetLastAllocatedFrame() );
		scene->RemovePendingFadeOutRemovals( false );

		static Bool multithreadedSceneCollect = true;
		if ( multithreadedSceneCollect )
		{
			RED_ASSERT( m_sceneCullingTask == nullptr, TXT("Already have a scene culling task!") );
			m_sceneCullingTask = new ( CTask::Root ) CSceneCullingTask( this );
			GTaskManager->Issue( *m_sceneCullingTask );

			// prepare the cascade cameras and frustums data before collecting elements
			if ( m_renderSunLighting )
			{
				BuildCascadesElements();
			}
		}
		else
		{
			if ( m_renderSunLighting )
			{
				RED_ASSERT( m_sceneCullingTask == nullptr, TXT("Already have a shadow culling task!") );
				m_shadowCullingTask = new ( CTask::Root ) CShadowCullingTask( this );
				GTaskManager->Issue( *m_shadowCullingTask );
			}

			// Run occlusion query for main scene
			RunSceneQuery();

			BuildSceneStatics();

			// Build element list
			BuildSceneElements();

			// Collect shadows
			BuildDynamicShadowElements();
		}
	}

	m_postProcess = PPCF_Tonemapping;
	if ( m_info->IsShowFlagOn( SHOW_PostProcess ) && GGame && GGame->GetActiveWorld() )
	{
		Uint32 flags = MapConfigToPostProcessFlags( *m_info );
		flags |= PPCF_Tonemapping;
		m_postProcess = static_cast<EPostProcessCategoryFlags>( flags );
	}
	else if ( m_info->IsShowFlagOn( SHOW_DepthOfField ) )
	{
		// In preview enable only camera DOF
		m_postProcess = static_cast<EPostProcessCategoryFlags>( PPCF_Tonemapping | PPCF_DepthOfField | PPCF_CutsceneDOF );
	}
}

void CRenderCollector::Reset()
{
	FinishShadowCulling();
	FinishSceneCullingTask();

	// Clear when we're done with it. Otherwise the data could potentially end up holding references to stuff long after scene
	// rendering (e.g. during shutdown, if it's a global object).
	if (m_renderCollectorData)
	{
		m_renderCollectorData->Reset();
	}

	m_usedLightChannels = 0;

	m_cascades.Reset();

	// Clear pointers on the end of the frame. Not clearing pointers would lead to leave dangling pointers
	// and in destructor there could be crash
	m_frame					= nullptr;
	m_scene					= nullptr;
	m_renderCollectorData	= nullptr;
	m_info					= nullptr;
	m_overlayInfo			= nullptr;
	m_camera				= nullptr;
	m_lodCamera				= nullptr;
	m_hiResShadowLists		= nullptr;
	
#ifdef USE_UMBRA
	m_occlusionData			= nullptr;
#endif // USE_UMBRA

}

void CRenderCollector::RenderElements( enum ERenderingSortGroup sortGroup, const class RenderingContext& context, ERenderingPlane renderingPlane, Uint8 containerGroups )
{
	// Pass to rend
	ERenderingSortGroup sortGroups[] = { sortGroup };
	RenderElements( 1, sortGroups, context, renderingPlane, containerGroups );
}

void CRenderCollector::RenderElements( Int32 sortGroupsCount, const enum ERenderingSortGroup *sortGroups, const class RenderingContext& context, ERenderingPlane renderingPlane, Uint8 containerGroups )
{
	// Batch by type
	for ( Int32 i=0; i<sortGroupsCount; i++ )
	{
		//This is commented because it takes a substantial amount of time but it can be very useful in some situations
		//StringAnsi profileName = StringAnsi::Printf( "Draw - SortGroup:%d DepthPass:%d", sortGroups[i], GpuApi::GetDepthPass() );
		//PC_SCOPE_PIX_NAMED( profileName.AsChar() );
		SRenderElementContainer& container = m_renderCollectorData->m_elements[ renderingPlane ].m_elements[ sortGroups[i] ];
		container.Draw( *this, context, containerGroups );
	}
}


void CRenderCollector::RenderElementsAllPlanesBackFirst( enum ERenderingSortGroup sortGroup, const class RenderingContext& context, Uint8 containerGroups )
{
	for ( Uint32 i = RPl_Max; i > 0; --i )
	{
		ERenderingPlane renderingPlane = (ERenderingPlane)(i-1);
		if ( HasAnySortGroupElements( sortGroup, renderingPlane ))
		{
			RenderElements( sortGroup, context, renderingPlane, containerGroups );
		}
	}
}

void CRenderCollector::RenderElementsAllPlanesBackFirst( Int32 sortGroupsCount, const enum ERenderingSortGroup *sortGroups, const class RenderingContext& context, Uint8 containerGroups )
{
	for ( Uint32 i = RPl_Max; i > 0; --i )
	{
		ERenderingPlane renderingPlane = (ERenderingPlane)(i-1);
		RenderElements( sortGroupsCount, sortGroups, context, renderingPlane, containerGroups );
	}
}

void CRenderCollector::RenderElementsAllPlanesFrontFirst( enum ERenderingSortGroup sortGroup, const class RenderingContext& context, Uint8 containerGroups )
{
	for ( Uint32 i = 0; i < RPl_Max; ++i )
	{
		ERenderingPlane renderingPlane = (ERenderingPlane)i;
		if ( HasAnySortGroupElements( sortGroup, renderingPlane ))
		{
			RenderElements( sortGroup, context, renderingPlane, containerGroups );
		}
	}
}

void CRenderCollector::RenderElementsAllPlanesFrontFirst( Int32 sortGroupsCount, const enum ERenderingSortGroup *sortGroups, const class RenderingContext& context, Uint8 containerGroups )
{
	for ( Uint32 i = 0; i < RPl_Max; ++i )
	{
		ERenderingPlane renderingPlane = (ERenderingPlane)i;
		RenderElements( sortGroupsCount, sortGroups, context, renderingPlane, containerGroups );
	}
}

void CRenderCollector::RenderAccumulativeRefractionElements( const class RenderingContext& context )
{
	m_renderCollectorData->m_accumulativeRefractionElements.Draw( *this, context, RECG_ALL );
}

void CRenderCollector::RenderReflectiveMaskedElements( const class RenderingContext& context )
{
	m_renderCollectorData->m_reflectiveMaskedElements.Draw( *this, context, RECG_ALL );
}

void CRenderCollector::RenderForegroundElements( const class RenderingContext& context )
{
	m_renderCollectorData->m_foregroundElements.Draw( *this, context, RECG_ALL );
};

Bool CRenderCollector::HasAnyAccumulativeRefractionElements() const
{
	return !m_renderCollectorData->m_accumulativeRefractionElements.Empty();
}

Bool CRenderCollector::HasAnyReflectiveMaskedElements() const
{
	return !m_renderCollectorData->m_reflectiveMaskedElements.Empty();
}

Bool CRenderCollector::HasAnyForegroundElements() const
{
	return !m_renderCollectorData->m_foregroundElements.Empty();
}

Bool CRenderCollector::HasAnySortGroupElements( ERenderingSortGroup group, ERenderingPlane renderingPlane ) const
{
	return !m_renderCollectorData->m_elements[renderingPlane].m_elements[group].Empty();
}

Bool CRenderCollector::HasAnySortGroupElements( ERenderingSortGroup group ) const
{
	for ( Uint32 i = 0; i < RPl_Max; ++i )
	{
		ERenderingPlane renderingPlane = (ERenderingPlane)i;
		if ( !m_renderCollectorData->m_elements[renderingPlane].m_elements[group].Empty() )
		{
			return true;
		}
	}

	return false;
}

Bool CRenderCollector::HasAnySortGroupElements( Uint32 numGroups, const ERenderingSortGroup *groups ) const
{
	for ( Uint32 group_i=0; group_i<numGroups; ++group_i )
	{
		if ( HasAnySortGroupElements( groups[group_i] ) )
		{
			return true;
		}
	}

	return false;
}

Bool CRenderCollector::HasAnySortGroupElements( Uint32 numGroups, const ERenderingSortGroup *groups, ERenderingPlane renderingPlane ) const
{
	for ( Uint32 group_i=0; group_i<numGroups; ++group_i )
	{
		if ( HasAnySortGroupElements( groups[group_i], renderingPlane ) )
		{
			return true;
		}
	}

	return false;
}

// Based on CFrustum implementation, see perforce for details
class CHiResEntityShadowsClipper
{
public:
	enum { MAX_FRUSTUM_PLANES = 6 };	//!< Maximum number of clipping planes

protected:
	Vector		m_planes[ MAX_FRUSTUM_PLANES ];		//!< Planes
	Vector		m_center;							//!< View center
	Uint32		m_numPlanes;						//!< Number of planes

public:
	CHiResEntityShadowsClipper()
		: m_numPlanes(0)
	{}

	void Reset()
	{
		m_center = Vector ( 0, 0, 0, 1 );
		m_numPlanes = 0;
	}

	void InitFromCamera( const Matrix& tmatrix, const Vector& position )
	{
		Reset();

		m_center = position;

		Matrix matrix = tmatrix;
		matrix.Transpose();

		// Front clipping plane  
		{
			Float x = matrix.V[3].A[0] - matrix.V[2].A[0];
			Float y = matrix.V[3].A[1] - matrix.V[2].A[1];
			Float z = matrix.V[3].A[2] - matrix.V[2].A[2];
			Float w = matrix.V[3].A[3] - matrix.V[2].A[3];
			AddPlane( x, y, z, w );
		}

		// Back clipping plane  
		{
			Float x = matrix.V[3].A[0] + matrix.V[2].A[0];
			Float y = matrix.V[3].A[1] + matrix.V[2].A[1];
			Float z = matrix.V[3].A[2] + matrix.V[2].A[2];
			Float w = matrix.V[3].A[3] + matrix.V[2].A[3];
			AddPlane( x, y, z, w );
		}

		// Left clipping plane  
		{
			Float x = matrix.V[3].A[0] + matrix.V[0].A[0];
			Float y = matrix.V[3].A[1] + matrix.V[0].A[1];
			Float z = matrix.V[3].A[2] + matrix.V[0].A[2];
			Float w = matrix.V[3].A[3] + matrix.V[0].A[3];
			AddPlane( x, y, z, w );
		}

		// Right clipping plane  
		{
			Float x = matrix.V[3].A[0] - matrix.V[0].A[0];
			Float y = matrix.V[3].A[1] - matrix.V[0].A[1];
			Float z = matrix.V[3].A[2] - matrix.V[0].A[2];
			Float w = matrix.V[3].A[3] - matrix.V[0].A[3];
			AddPlane( x, y, z, w );
		}

		// Top clipping plane  
		{
			Float x = matrix.V[3].A[0] - matrix.V[1].A[0];
			Float y = matrix.V[3].A[1] - matrix.V[1].A[1];
			Float z = matrix.V[3].A[2] - matrix.V[1].A[2];
			Float w = matrix.V[3].A[3] - matrix.V[1].A[3];
			AddPlane( x, y, z, w );
		}

		// Bottom clipping plane
		{
			Float x = matrix.V[3].A[0] + matrix.V[1].A[0];
			Float y = matrix.V[3].A[1] + matrix.V[1].A[1];
			Float z = matrix.V[3].A[2] + matrix.V[1].A[2];
			Float w = matrix.V[3].A[3] + matrix.V[1].A[3];
			AddPlane( x, y, z, w );
		}
	}

	void AddPlane( Float x, Float y, Float z, Float w )
	{
		// Create optimized plane
		Vector& plane = m_planes[ m_numPlanes ];
		m_numPlanes++;

		// Normalize plane
		Float len = sqrt( x*x + y*y + z*z );
		plane.A[0] = x / len;
		plane.A[1] = y / len;
		plane.A[2] = z / len;
		plane.A[3] = w / len;
	}

	Bool ClampPoint( const Vector& srcPoint, Vector& destPoint ) const
	{
		// Clamp to planes
		Bool changed = false;
		Vector pos = srcPoint;
		pos.W = 1.0f;
		for ( Uint32 i=0; i<m_numPlanes; ++i )
		{
			Float dist = Vector::Dot4( m_planes[i], pos );
			if ( dist < 0.0f )
			{
				changed = true;
				pos -= (m_planes[i] * dist);
				pos.W = 1.0f;
			}
		}		

		// Reset (just to be sure, this is important)
#ifndef NO_ASSERTS
		if ( changed )
		{
			for ( Uint32 i=0; i<m_numPlanes; ++i )
			{
				Float dist = Vector::Dot4( m_planes[i], pos );
				ASSERT( dist >= -0.1f );
			}
		}
#endif

		// Write results
		destPoint = pos;
		return changed;
	}
};

Uint32 CollectEntityrGroupHiResShadowsElements( CRenderEntityGroup &entityGroup, const CFrustum &cameraFrustum, const CHiResEntityShadowsClipper &cameraClipper, const EulerAngles &shadowCameraRotation, CRenderCollector &collector, CRenderCollector::HiResShadowsCollector &shadowCollector, Vector *outBoxCenter, Float *outBoxExtents )
{
	shadowCollector.Reset();

	// Calcualte min/max projection bounds
	Float projMinU = FLT_MAX;
	Float projMinV = FLT_MAX;
	Float projMaxU = -FLT_MAX;
	Float projMaxV = -FLT_MAX;

	// Bounding box
	Box mergedBox;
	mergedBox.Clear();

	// Calculate projection vectors (U and V)
	Vector projU, projV;
	shadowCameraRotation.ToAngleVectors( NULL, &projU, &projV );

	// We do not use LOD0 here (we should but it would create differences between the GBuffer)
	// TODO: this can be enforced once the defered rendering is removed
	const Bool useHighestLOD = 
#ifdef USE_ANSEL
		isAnselSessionActive;
#else
		false;
#endif // USE_ANSEL

	Uint32 numCurrHiResShadowProxies = 0;
	for ( Uint32 i=0; i<entityGroup.GetMainProxies().Size(); ++i )
	{
		// Skip if not valid for hires shadows
		CRenderProxy_Mesh *proxy = entityGroup.GetMainProxies()[i]->AsMeshProxy();
		if ( !proxy )
		{
			continue;
		}

		// Collect elements
		{
			proxy->CollectHiResShadowsElements( collector, shadowCollector, useHighestLOD );
			++numCurrHiResShadowProxies;
		}

		// Calculate projection of proxy bounding box in shadow space
		if ( outBoxCenter || outBoxExtents )
		{
			const Box& box = proxy->GetBoundingBox();
			if ( 0 != cameraFrustum.TestBox( box ) )
			{
				// Get box center
				Vector corners[8];
				box.CalcCorners( corners );
				mergedBox.AddBox( box );
				const Vector center = box.CalcCenter();

				// process each corner
				for ( Uint32 j=0; j<8; ++j )
				{
					// clamp corner point to view frustum
					Vector clampedPoint = corners[j];
					cameraClipper.ClampPoint( corners[j], clampedPoint );

					// project
					const Float projValueU = Vector::Dot3( projU, clampedPoint );
					const Float projValueV = Vector::Dot3( projV, clampedPoint );
					projMinU = Min<Float>( projMinU, projValueU );
					projMaxU = Max<Float>( projMaxU, projValueU );
					projMinV = Min<Float>( projMinV, projValueV );
					projMaxV = Max<Float>( projMaxV, projValueV );
				}
			}
		}
	}

	// 
	if ( 0 == numCurrHiResShadowProxies || !shadowCollector.HasAnyCaster() || !shadowCollector.HasAnyReceiver() )
	{
		return 0;
	}

	// Output some projection params
	if ( outBoxCenter || outBoxExtents )
	{
		// Box is to large anyway to have any difference
		const Float extentsU = projMaxU - projMinU;
		const Float extentsV = projMaxV - projMinV;
		const Float boxExtents = Max<Float>( extentsU, extentsV );
		
		// Calcualte center
		const Float centerU = (projMaxU + projMinU) / 2.0f;
		const Float centerV = (projMaxV + projMinV) / 2.0f;
		Vector boxCenter = mergedBox.CalcCenter();
		boxCenter += projU * ( centerU - Vector::Dot3( boxCenter, projU ) );
		boxCenter += projV * ( centerV - Vector::Dot3( boxCenter, projV ) );

		//
		if ( outBoxCenter )
		{
			*outBoxCenter = boxCenter;
		}

		//
		if ( outBoxExtents )
		{
			*outBoxExtents = boxExtents;
		}
	}

	//
	return numCurrHiResShadowProxies;
}

void CRenderCollector::RenderHiResEntityShadows( const GpuApi::RenderTargetSetup& rtMainSetup )
{
	// Shadow map texture (shared with terrain shadows, why not)
	if ( !GetScene() || !GetScene()->GetTerrainShadows() )
	{
		return;
	}

	GpuApi::TextureRef shadowMap = GetRenderer()->GetHiResEntityShadowmap( *this );
	if ( !shadowMap )
	{
		return;
	}

	const CRenderFrameInfo &info = GetRenderFrameInfo();

	// Get some params
	const Uint32 shadowMapSize = GpuApi::GetTextureLevelDesc( shadowMap, 0 ).width;
	const Uint32 halfShadowMapSize = shadowMapSize / 2;
	ASSERT( shadowMapSize == GpuApi::GetTextureLevelDesc( shadowMap, 0 ).height );
	
	// Get viewport camera frustum planes ( to limit the extents of the bounding boxes )
	CFrustum cameraFrustum;
	CHiResEntityShadowsClipper cameraClipper;
	cameraFrustum.InitFromCamera( m_camera->GetWorldToScreen() );
	cameraClipper.InitFromCamera( m_camera->GetWorldToScreen(), m_camera->GetPosition() );

	// Calculate shadow camera rotation
	const EulerAngles shadowCameraRotation = LookMatrix2( -m_info->m_baseLightingParameters.m_lightDirection, Vector::EZ ).ToEulerAngles();

	// Mesh stats
	MeshDrawingStats meshStats;

	// Choose hires shadows quality
#ifdef RED_PLATFORM_WINPC
	Bool useFullResEntityShadows = true;
#else
	Bool useFullResEntityShadows = false;
#endif
	if ( Config::cvHiResEntityShadowsForceFullRes.Get() >= 0 )
	{
		useFullResEntityShadows = (Config::cvHiResEntityShadowsForceFullRes.Get() > 0) 
#ifdef USE_ANSEL
			|| isAnselSessionActive
#endif // USE_ANSEL
			;
	}

	// Declare some helpers
	const Uint32 packingMaxCapacity = 4;
	const Uint32 packingCapacity = useFullResEntityShadows ? 1 : packingMaxCapacity;
	CRenderEntityGroup *packedEntityGroups[packingMaxCapacity];
	Vector packedBoxCenters[packingMaxCapacity];
	Float packedBoxExtents[packingMaxCapacity];
	
	// Setup some parameters based on chosen quality
	Float biasSlopeScale = 1.f;
	Float biasConstScale = 1.f;
	Float filterTexelRadius = 0;
	GpuApi::ViewportDesc packedViewports[packingMaxCapacity];
	if ( 1 == packingCapacity )
	{
		// Filter radius
		filterTexelRadius = Max( 0.f, m_info->m_hiResShadowTexelRadius );

		// Viewports
		packedViewports[0].Set( shadowMapSize, shadowMapSize );
	}
	else
	{
		// Filter radius
		filterTexelRadius = 0.75f * Max( 0.f, m_info->m_hiResShadowTexelRadius );

		// Viewports
		const Int32 shadowBorder = 2 + (Int32)ceilf( filterTexelRadius );
		RED_ASSERT( 4 == packingCapacity );
		packedViewports[0].Set( halfShadowMapSize - 2 * shadowBorder, halfShadowMapSize - 2 * shadowBorder, 0, 0 );
		packedViewports[1].Set( halfShadowMapSize - 2 * shadowBorder, halfShadowMapSize - 2 * shadowBorder, halfShadowMapSize + shadowBorder, 0 );
		packedViewports[2].Set( halfShadowMapSize - 2 * shadowBorder, halfShadowMapSize - 2 * shadowBorder, 0, halfShadowMapSize + shadowBorder );
		packedViewports[3].Set( halfShadowMapSize - 2 * shadowBorder, halfShadowMapSize - 2 * shadowBorder, halfShadowMapSize + shadowBorder, halfShadowMapSize + shadowBorder );
	}

	//
	const Bool origReversedProjection = GpuApi::IsReversedProjectionState();

	// Process each entity on it's own
	Uint32 numPackedGroups = 0;
	CRenderEntityGroup *entityGroupIter = m_hiResShadowLists;
	while ( entityGroupIter )
	{
		// Skip if there are no shadow casting proxies
		Bool hasShadowCastingProxy = false;
		{
			for ( Uint32 i=0; i<entityGroupIter->GetMainProxies().Size(); ++i )
			{
				CRenderProxy_Mesh *meshProxy = entityGroupIter->GetMainProxies()[i]->AsMeshProxy();
				if ( meshProxy && meshProxy->CanCastShadows() )
				{
					hasShadowCastingProxy = true;
					break;
				}
			}
		}

		// Render to shadowmap and save entity group
		if ( hasShadowCastingProxy )
		{
			// Collect proxies
			Vector boxCenter = Vector::ZERO_3D_POINT;
			Float boxExtents = 0;
			HiResShadowsCollector& shadowsCollector = m_renderCollectorData->m_hiResShadowsCollector;
			const Uint32 numCurrHiResShadowProxies = CollectEntityrGroupHiResShadowsElements( *entityGroupIter, cameraFrustum, cameraClipper, shadowCameraRotation, *this, shadowsCollector, &boxCenter, &boxExtents );
			Float hiResShadowMaxExtents = ( info.m_hiResShadowMaxExtents > 0.0f ? info.m_hiResShadowMaxExtents : 5.0f );

			if ( numCurrHiResShadowProxies > 0 && boxExtents <= hiResShadowMaxExtents )
			{
				// Tick any proxies that may have requested it during the above collection
				m_scene->TickCollectedProxies();

				// Render to shadowmap
				{
					PC_SCOPE_RENDER_LVL0(RenderShadowmap);

					// Setup rendertargets
					if ( 0 == numPackedGroups )
					{
						// Setup shadow bias
						GpuApi::SetupShadowDepthBias( 0, biasSlopeScale * m_info->m_hiResShadowBiasOffsetSlopeMul * ceilf( 1.4142f * filterTexelRadius ) );

						// Disable reversed projection
						GpuApi::SetReversedProjectionState( false );

						// Set render target
						GpuApi::RenderTargetSetup rtSetup;
						rtSetup.SetNullColorTarget();
						rtSetup.SetDepthStencilTarget( shadowMap );
						rtSetup.SetViewportFromTarget( shadowMap );
						GpuApi::SetupRenderTargets( rtSetup );

						// Clear at first use
						GetRenderer()->ClearDepthTarget( 1.0f );
					}
					
					// Build and set shadow camera
					CRenderCamera shadowCamera( boxCenter, shadowCameraRotation, 0.0f/*ortho*/, 1.0f, -20.0f, 20.0f, boxExtents );
					GetRenderer()->GetStateManager().SetCamera( shadowCamera );

					// Setup viewport
					if ( GpuApi::GetViewport() != packedViewports[numPackedGroups] )
					{
						GpuApi::SetViewport( packedViewports[numPackedGroups] );
					}
					
					//
					ASSERT( !GpuApi::IsReversedProjectionState() );					
					CGpuApiScopedDrawContext drawContext( GpuApi::DRAWCONTEXT_ShadowMapGenCSM_DepthTex );

					// Render proxies that are solid
					{
						RenderingContext solidRenderContext( shadowCamera );
						solidRenderContext.m_pass = RP_ShadowDepthSolid;

						GetRenderer()->GetMeshBatcher()->RenderMeshes( *m_info, solidRenderContext, shadowsCollector.m_casterSolidElements, RMBF_Shadowmap, meshStats );
					}

					// Render proxies that are using discards
					{
						RenderingContext solidRenderContext( shadowCamera );
						solidRenderContext.m_pass = RP_ShadowDepthMasked;

						GetRenderer()->GetMeshBatcher()->RenderMeshes( *m_info, solidRenderContext, shadowsCollector.m_casterDiscardElements, RMBF_Shadowmap, meshStats );
					}
				}

				// Save groups packing
				RED_ASSERT( numPackedGroups < packingCapacity );
				packedEntityGroups[ numPackedGroups ] = entityGroupIter;
				packedBoxCenters[ numPackedGroups ] = boxCenter;
				packedBoxExtents[ numPackedGroups ] = boxExtents;
				++numPackedGroups;

				// Update stats
			#ifndef RED_FINAL_BUILD
				{
					extern SceneRenderingStats GRenderingStats;
					GRenderingStats.m_numHiResShadowsActors += 1;
					GRenderingStats.m_numHiResShadowsProxies += numCurrHiResShadowProxies;
				}
			#endif
			}
		}

		// Procees to next entity group
		entityGroupIter = entityGroupIter->m_next;
		
		// Render hires entity shadows to shadowmask if out of packing slots or no more groups left
		if ( numPackedGroups > 0 && (numPackedGroups == packingCapacity || !entityGroupIter) )
		{
			PC_SCOPE_RENDER_LVL1(RenderForward);

			// Set some renderstates
			GpuApi::SetupRenderTargets( rtMainSetup );
			GetRenderer()->GetStateManager().SetCamera( m_info->m_camera );
			GpuApi::BindTextures( 11, 1, &shadowMap, GpuApi::PixelShader );
			GpuApi::SetSamplerStatePreset( 9, GpuApi::SAMPSTATEPRESET_ClampLinearNoMipCompareLess, GpuApi::PixelShader );
			GpuApi::SetReversedProjectionState( origReversedProjection );
			GpuApi::SetupShadowDepthBias( 0, 0 );

			// Render groups
			for ( Uint32 packed_group_i=0; packed_group_i<numPackedGroups; ++packed_group_i )
			{
				// Collect elements
				HiResShadowsCollector& shadowsCollector = m_renderCollectorData->m_hiResShadowsCollector;
				CollectEntityrGroupHiResShadowsElements( *packedEntityGroups[packed_group_i], cameraFrustum, cameraClipper, shadowCameraRotation, *this, shadowsCollector, nullptr, nullptr );
				RED_ASSERT( shadowsCollector.HasAnyReceiver() );

				// Build shadow camera
				CRenderCamera shadowCamera( packedBoxCenters[packed_group_i], shadowCameraRotation, 0.0f/*ortho*/, 1.0f, -20.0f, 20.0f, packedBoxExtents[packed_group_i] );

				// Build world to screen
				Matrix shadowWorldToScreen = shadowCamera.GetWorldToScreen();
				{	
					const GpuApi::ViewportDesc &currViewport = packedViewports[packed_group_i];

					Matrix tform = Matrix::IDENTITY;
					Float scaleX = currViewport.width / (Float)shadowMapSize;
					Float scaleY = currViewport.height / (Float)shadowMapSize;
					tform.SetScale33( Vector ( scaleX, scaleY, 1.f ) );
					tform.SetTranslation( (currViewport.x + 0.5f * currViewport.width) / (Float)shadowMapSize * 2.f - 1.f, (1.f - (currViewport.y + 0.5f * currViewport.height) / (Float)shadowMapSize) * 2.f - 1.f, 0.f );
					shadowWorldToScreen = shadowWorldToScreen * tform;
				}

				//
				const Float invRangeZ = 1.f / (shadowCamera.GetFarPlane() - shadowCamera.GetNearPlane());
				GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_Matrix, shadowWorldToScreen );
				GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_HiResShadowsParams, Vector( filterTexelRadius / (Float)shadowMapSize, biasConstScale * m_info->m_hiResShadowBiasOffsetConst * invRangeZ, invRangeZ, 0 ) );

				// Draw solid elements first
				{
					CGpuApiScopedDrawContext drawContext( GpuApi::DRAWCONTEXT_HiResShadows );

					RenderingContext solidRenderContext( m_info->m_camera );
					solidRenderContext.m_pass = RP_HiResShadowMask;

					GetRenderer()->GetMeshBatcher()->RenderMeshes( *m_info, solidRenderContext, shadowsCollector.m_receiverSolidElements, 0, meshStats );
					GetRenderer()->GetMeshBatcher()->RenderMeshes( *m_info, solidRenderContext, shadowsCollector.m_receiverDiscardElements, 0, meshStats );
				}
			}

			// Reset renderstates
			GpuApi::BindTextures( 11, 1, nullptr, GpuApi::PixelShader );

			// Rewind groups packing
			numPackedGroups = 0;
		}
	}

	// Restore reversed projection
	GpuApi::SetReversedProjectionState( origReversedProjection );

	// Reset shadow depth bias
	GpuApi::SetupShadowDepthBias( 0, 0 );

#ifndef RED_FINAL_BUILD
	// Update stats
	{
		extern SceneRenderingStats GRenderingStats;
		GRenderingStats.m_numHiResShadowsChunks += meshStats.m_numChunks;
		GRenderingStats.m_numHiResShadowsTriangles += meshStats.m_numTriangles;
	}
#endif
}

Bool CRenderCollector::HasAnyDistantLights() const
{
	if( m_renderCollectorData )
	{
		return !m_renderCollectorData->m_distantLights.Empty();
	}
	return false;
}

void CRenderCollector::RenderDistantLights( CRenderDistantLightBatcher& batcher, Float intensityScale )
{
	ASSERT( m_renderCollectorData );
	ASSERT( intensityScale > 0 );

	const Uint32 numDistantLights = m_renderCollectorData->m_distantLights.Size();

	batcher.PredictedNumberOfLights( numDistantLights );

	for ( Uint32 i = 0; i < numDistantLights; ++i )
	{
		m_renderCollectorData->m_distantLights[i]->GatherDistantLight( *m_info, batcher );
	}

	batcher.RenderLights( intensityScale );
}

struct FlaresViewportModifier
{
	FlaresViewportModifier ()
		: m_isCurrOrig ( true )
		, m_origViewport ( GpuApi::GetViewport() )
	{}

	~FlaresViewportModifier ()
	{
		SetupViewport( true );
	}

	void SetupViewport( EFlareCategory flareCategory )
	{
		Bool needsCustomViewport = FLARECAT_Sun == flareCategory || FLARECAT_Moon == flareCategory;
		SetupViewport( !needsCustomViewport );
	}

	void SetupViewport( Bool enableOriginal )
	{
		if ( enableOriginal == m_isCurrOrig )
		{
			return;
		}

		if ( enableOriginal )
		{
			GpuApi::SetViewport( m_origViewport );
		}
		else
		{
			GpuApi::ViewportDesc farViewport = m_origViewport;
			if ( GpuApi::IsReversedProjectionState() )
			{
				farViewport.maxZ = farViewport.minZ;
			}
			else
			{
				farViewport.minZ = farViewport.maxZ;
			}
			GpuApi::SetViewport( farViewport );
		}
		
		m_isCurrOrig = enableOriginal;
	}

	Bool m_isCurrOrig;
	GpuApi::ViewportDesc m_origViewport;
};

void CRenderCollector::GrabFlaresTransparencyHelpers( const CRenderFrameInfo &info, const GpuApi::TextureRef &texGrabTarget, const GpuApi::TextureRef &texGrabSource )
{
	if ( !texGrabTarget || !texGrabSource )
	{
		return;
	}

	RED_ASSERT( texGrabTarget != texGrabSource );

	CGpuApiDeviceStateGrabber targetStateGrabber ( GpuApi::DEVSTATECAT_RenderTargetSetup );

	GpuApi::RenderTargetSetup rtSetup;
	rtSetup.SetColorTarget( 0, texGrabTarget );
	rtSetup.SetViewport( info.m_width, info.m_height );
	GpuApi::SetupRenderTargets( rtSetup );

	FlaresViewportModifier viewportMod;
	for ( Uint32 i=0; i<FLARECAT_MAX; ++i )
	{
		const TDynArray< CRenderProxy_Flare* > &flares = m_renderCollectorData->m_flaresByCategory[i];
		if ( !(FLARECAT_Sun == i || FLARECAT_Moon == i) || flares.Empty() )
		{
			continue;			
		}

		viewportMod.SetupViewport( (EFlareCategory)i );
		GetRenderer()->GetFlaresBatcher()->GrabTransparencyHelpers( *this, flares, *m_info, texGrabSource );
	}
}

void CRenderCollector::UpdateSkyFlaresOcclusion( const GpuApi::TextureRef &texTransparencyHelperOpaque, const GpuApi::TextureRef &texTransparencyHelperTransparent )
{
	FlaresViewportModifier viewportMod;

	for ( Uint32 i=0; i<FLARECAT_MAX; ++i )
	{
		const Bool isSkyFlareCat = (FLARECAT_Sun==i || FLARECAT_Moon==i);
		const TDynArray< CRenderProxy_Flare* > &flares = m_renderCollectorData->m_flaresByCategory[i];
		if ( !isSkyFlareCat || flares.Empty() )
		{
			continue;
		}

		const Bool useTransparencyHelpers = texTransparencyHelperOpaque && texTransparencyHelperTransparent;
		viewportMod.SetupViewport( (EFlareCategory)i );
		GetRenderer()->GetFlaresBatcher()->UpdateOcclusion( *this, flares, *m_info, useTransparencyHelpers ? texTransparencyHelperOpaque : GpuApi::TextureRef::Null(),  useTransparencyHelpers ? texTransparencyHelperTransparent : GpuApi::TextureRef::Null() );
	}
}

void CRenderCollector::UpdateNonSkyFlaresOcclusion()
{
	FlaresViewportModifier viewportMod;

	for ( Uint32 i=0; i<FLARECAT_MAX; ++i )
	{
		const Bool isSkyFlareCat = (FLARECAT_Sun==i || FLARECAT_Moon==i);
		const TDynArray< CRenderProxy_Flare* > &flares = m_renderCollectorData->m_flaresByCategory[i];
		if ( isSkyFlareCat || flares.Empty() )
		{
			continue;
		}

		viewportMod.SetupViewport( (EFlareCategory)i );
		GetRenderer()->GetFlaresBatcher()->UpdateOcclusion( *this, m_renderCollectorData->m_flaresByCategory[i], *m_info, GpuApi::TextureRef::Null(),  GpuApi::TextureRef::Null() );
	}
}

void CRenderCollector::DrawFlaresDebugOcclusionShapes()
{
	FlaresViewportModifier viewportMod;

	for ( Uint32 i=0; i<FLARECAT_MAX; ++i )
	{
		viewportMod.SetupViewport( (EFlareCategory)i );
		if ( !m_renderCollectorData->m_flaresByCategory[i].Empty() )
		{
			GetRenderer()->GetFlaresBatcher()->DrawDebugOcclusion( *this, m_renderCollectorData->m_flaresByCategory[i], *m_info );
		}
	}
}

void CRenderCollector::RenderFlares( const class RenderingContext& context, Bool allowCenterFlares, Bool allowLensFlares )
{
	FlaresViewportModifier viewportMod;

	if ( allowCenterFlares )
	{
		for ( Int32 i=(Int32)FLARECAT_MAX-1; i>=0; --i )
		{
			viewportMod.SetupViewport( (EFlareCategory)i );
			if ( !m_renderCollectorData->m_flaresByCategory[i].Empty() )
			{
				GetRenderer()->GetFlaresBatcher()->DrawFlares( *this, context, m_renderCollectorData->m_flaresByCategory[i] );
			}
		}
	}

	if ( allowLensFlares )
	{
		for ( Int32 i=(Int32)FLARECAT_MAX-1; i>=0; --i )
		{
			viewportMod.SetupViewport( (EFlareCategory)i );
			if ( !m_renderCollectorData->m_flaresByCategory[i].Empty() )
			{
				GetRenderer()->GetFlaresBatcher()->DrawLensFlares( *this, context, m_renderCollectorData->m_flaresByCategory[i] );
			}
		}
	}
}

void CRenderCollector::RenderDynamicFragments( enum ERenderingSortGroup sortGroup, const class RenderingContext& context )
{
	// Pass to rend
	ERenderingSortGroup sortGroups[] = { sortGroup };
	RenderDynamicFragments( 1, sortGroups, context );
}

void CRenderCollector::RenderDynamicFragments( Int32 sortGroupsCount, const enum ERenderingSortGroup *sortGroups, const class RenderingContext& context )
{
	// Draw dynamic fragments
	if ( m_frame )
	{
		// Initial by type group
		for ( Int32 i=0; i<sortGroupsCount; ++i )
		{
			const ERenderingSortGroup sortGroup = sortGroups[ i ];
			for ( IRenderFragment* cur = m_frame->GetFragments( sortGroup ); cur; cur=cur->GetNextBaseFragment() )
			{
				cur->Draw( context );
			}			
		}
	}
}

void CRenderCollector::CalculateCascadesData( const CCascadeShadowResources &cascadeShadowResources )
{
	PC_SCOPE_PIX( CalculateCascadesData );

	ASSERT( NULL != m_scene && "Assert because previously in cascades.init function there was a condition NULL!=scene - imo always satisfied" );
	m_cascades.Init( this, *m_info, cascadeShadowResources );
}

void CRenderCollector::BuildCascadesElements()
{
	PC_SCOPE_PIX( BuildCascadesElements );

	const CCascadeShadowResources &cascadeShadowResources = GetRenderer()->GetGlobalCascadesShadowResources();
	CalculateCascadesData( cascadeShadowResources );

	// Do we have anythint to draw ?
	if ( m_info->IsShowFlagOn( SHOW_Shadows ) && m_cascades.m_numCascades > 0 )
	{
#ifdef USE_UMBRA
		if ( m_occlusionData )
		{
			m_occlusionData->SetCascadesData( m_cascades.m_frustums, m_cascades.m_numCascades );
		}
#endif // USE_UMBRA
		m_scene->GetRenderElementMap()->CollectShadowProxies( m_cascades );
	}
}

void CRenderCollector::RunSceneQuery()
{
#ifdef USE_UMBRA
	if ( m_collectWithUmbra )
	{
		PC_SCOPE_PIX( OcclusionQuery );
		m_scene->PerformOcclusionQuery( m_frame );
	}
#endif // USE_UMBRA
}


void CRenderCollector::BuildSceneStatics()
{
	m_scene->CollectStaticDrawables( *this );
}


void CRenderCollector::BuildSceneElements()
{
	PC_SCOPE_PIX( BuildSceneElements );

	// Background stuff
	m_scene->CollectBackgroundDrawables( *this );

	// Normal drawables relative to the viewport
	m_scene->CollectDrawables( *this );

	// Collect active flares (must be called after all flares got 'collected', e.g. marked themselves as active if they are not already active)
#ifdef RED_ASSERTS_ENABLED
	for ( Uint32 i=0; i<FLARECAT_MAX; ++i )	
	{
		ASSERT( m_renderCollectorData->m_flaresByCategory[i].Empty() );
	}
#endif
	m_scene->CollectActiveFlares( *this );
}

void CRenderCollector::BuildDynamicShadowElements()
{
	PC_SCOPE_PIX( BuildDynamicShadowElements );

#ifndef RED_FINAL_BUILD
	extern SceneRenderingStats GRenderingStats;
#endif

	// reset shadow allocators
	GetRenderer()->GetShadowManager()->ResetStaticPacking();
	GetRenderer()->GetShadowManager()->ResetDynamicPacking();

	if ( !m_renderShadows )
	{
		// early exit
		return;
	}

	{
		PC_SCOPE_PIX( UpdateShadowParams );
		// Calculate the light parameters getting the final list of shadowed lights
		// This will calculate the requested shadow resolution (for dynamic lights) and shadow fading factor
		// If light does not need shadows for this frame function UpdateShadowParams will return false
		m_renderCollectorData->m_lightsTemp.ClearFast();
		for ( Uint32 i = 0; i < m_renderCollectorData->m_lights.Size(); ++i )
		{
			IRenderProxyLight* light = m_renderCollectorData->m_lights[ i ];
			if ( light->UpdateShadowParams( *this ) )
			{
				m_renderCollectorData->m_lightsTemp.PushBack( light );
			}
		}
	}

#ifndef RED_FINAL_BUILD
	// Update stats
	GRenderingStats.m_numLightWithShadows = m_renderCollectorData->m_lightsTemp.Size();
#endif

	{
		PC_SCOPE_PIX( SortByDistance );
		// sort the temp lights by distance before processing static shadows
		struct SortLightsByDistance
		{
			const Vector& m_cameraPosition;
			SortLightsByDistance( const Vector& cameraPosition )
				: m_cameraPosition( cameraPosition )
			{}

			RED_FORCE_INLINE Bool operator() ( IRenderProxyLight* lightA, IRenderProxyLight* lightB ) const
			{
				const Float distanceA = lightA->GetLocalToWorld().GetTranslation().DistanceSquaredTo( m_cameraPosition );
				const Float distanceB = lightB->GetLocalToWorld().GetTranslation().DistanceSquaredTo( m_cameraPosition );
				return distanceA > distanceB;
			}
		};
		Sort( m_renderCollectorData->m_lightsTemp.Begin(), m_renderCollectorData->m_lightsTemp.End(), SortLightsByDistance( m_camera->GetPosition() ) );
	}
	
	{
		PC_SCOPE_PIX( PrepareStaticShadowmaps );
		// allocate static shadow maps, do it only for world scenes
		const Bool processStaticShadows = m_scene && m_scene->IsWorldScene();
		if ( processStaticShadows )
		{
			TDynArray< IShadowmapQuery* > staticShadowmapQueries;
			{
				PC_SCOPE_PIX( CreateQueries );
				for ( IRenderProxyLight* light : m_renderCollectorData->m_lightsTemp )
				{
					if ( light->IsStaticShadowsCached() && light->ShouldPrepareStaticShadowmaps() )
					{
						staticShadowmapQueries.PushBack( light->CreateShadowmapQuery() );
					}
				}
			}
			{
				PC_SCOPE_PIX( Collect );
				if ( !staticShadowmapQueries.Empty() )
				{
					m_scene->GetRenderElementMap()->Collect( staticShadowmapQueries.TypedData(), staticShadowmapQueries.Size(), CRenderProxyTypeFlag( RPT_Mesh ), REMEF_ShadowCaster );
				}
			}
			{
				PC_SCOPE_PIX( PrepareStaticShadowmaps );
#ifndef RED_FINAL_BUILD
				Uint32 numStaticPointLights = 0;
#endif
				for ( IShadowmapQuery* query : staticShadowmapQueries )
				{
					if ( query->m_light )
					{
						Bool prepared = !query->m_light->PrepareStaticShadowmaps( *this, GetRenderer()->GetShadowManager(), query->m_proxies );
						RED_UNUSED(prepared);

#ifndef RED_FINAL_BUILD
						// allocate or update static cubemap for light
						if ( prepared )
						{
							// failed to allocate cubemap when needed
							// this will automatically revert the light to full dynamic shadows
							GRenderingStats.m_flagStaticShadowsLimit = true;
						}
						else
						{
							numStaticPointLights += 1;
						}
#endif
					}
				}
#ifndef RED_FINAL_BUILD
				// update stats
				GRenderingStats.m_numLightWithStaticShadows = numStaticPointLights;
#endif
			}
			staticShadowmapQueries.ClearPtr();
		}
	}

	{
		PC_SCOPE_PIX( SortByShadowSize );
		//sort the list of shadow casting lights by their resolution
		auto sortLightsByShadowSize = []( IRenderProxyLight* lightA, IRenderProxyLight* lightB )
		{
			return lightB->GetCurrentShadowResolution() > lightA->GetCurrentShadowResolution();
		};
		Sort( m_renderCollectorData->m_lightsTemp.Begin(), m_renderCollectorData->m_lightsTemp.End(), sortLightsByShadowSize );
	}

	{
		PC_SCOPE_PIX( PrepareDynamicShadowmaps );
		// try to pack the lights without reduced resolution first

		TDynArray< IShadowmapQuery* > dynamicShadowmapQueries;
		{
			PC_SCOPE_PIX( CreateQueries );
			for ( IRenderProxyLight* light : m_renderCollectorData->m_lightsTemp )
			{
				if ( light->ShouldPrepareDynamicShadowmaps() )
				{
					dynamicShadowmapQueries.PushBack( light->CreateShadowmapQuery() );
				}			
			}
		}
		{
			PC_SCOPE_PIX( Collect );
			if ( !dynamicShadowmapQueries.Empty() )
			{
				m_scene->GetRenderElementMap()->Collect( dynamicShadowmapQueries.TypedData(), dynamicShadowmapQueries.Size(), CRenderProxyTypeFlag( RPT_Mesh ) | RPT_Apex, REMEF_ShadowCaster );
			}
		}
		{
			PC_SCOPE_PIX( PackingPasses );
			const Uint32 maxPackingPasses = 2;
			Uint32 packingPass;
			for ( packingPass = 0; packingPass < maxPackingPasses; ++packingPass )
			{
				// reset packing at the start of each pass
				GetRenderer()->GetShadowManager()->ResetDynamicPacking();

				// try to allocate dynamic space in shadow manager
				Bool packingOK = true;
				for ( IShadowmapQuery* query : dynamicShadowmapQueries )
				{
					if ( !query->m_light->PrepareDynamicShadowmaps( *this, GetRenderer()->GetShadowManager(), query->m_proxies ) )
					{
						// do not break here, we still may be able to pack some smaller regions
						packingOK = false;
					}
				}

				// Packing has been completed in current pass
				if ( packingOK )
				{
					break;
				}

				// packing failed, reduce the shadowmap size
				for ( IShadowmapQuery* query : dynamicShadowmapQueries )
				{
					query->m_light->UpdateShadowParams( *this );
					query->m_light->ReduceDynamicShadowmapSize();
				}
			}

#ifndef RED_FINAL_BUILD
			// Update general flags
			if ( packingPass >= maxPackingPasses )
			{
				// we were not able to pack all dynamic shadows into texture space :*(
				GRenderingStats.m_flagDynamicShadowsLimit = true;
			}
			else if ( packingPass > 0 )
			{
				// we were able to pack all the lights but the resolution was reduced
				GRenderingStats.m_flagDynamicShadowsReduces = true;
			}
#endif
		}
		dynamicShadowmapQueries.ClearPtr();
	}
}

CRenderCollector::CRenderCollectorData::CRenderCollectorData()
{
}

void CRenderCollector::CRenderCollectorData::Reset()
{
	// Reset element lists
	for ( Uint32 j = 0; j < RPl_Max; ++j )
	{
		for ( Uint32 i=0; i<RSG_Max; i++ )
		{
			m_elements[j].m_elements[i].Reset();
		}
	}

	// Reset flares lists
	for ( Uint32 i=0; i < FLARECAT_MAX; ++i )
	{
		m_flaresByCategory[i].ClearFast();
	}

	// Reset accumulative refraction elements
	m_accumulativeRefractionElements.Reset();
	m_reflectiveMaskedElements.Reset();
	m_foregroundElements.Reset();

	m_dimmers.ClearFast();
	for( Uint32 i = 0 ; i < EDDI_MAX; ++i )
	{
		m_decals[i].ClearFast();
	}

	m_furProxies.ClearFast();
	m_furShadowProxies.ClearFast();

	m_dynamicDecalChunks.ClearFast();

	for( Uint32 i = 0; i < SCREENCAT_Max; ++i )
	{
		m_particles[i].ClearFast();
	}
	
	m_lights.ClearFast();
	m_distantLights.ClearFast();
	m_lightsTemp.ClearFast();
	
	m_hiResShadowsCollector.Reset();
}

CRenderCollector::CRenderCollectorData::~CRenderCollectorData()
{
}

void CRenderCollector::NotifyProxyOfCollectedElement( IRenderElement* element )
{
	if ( element->GetType() == RET_MeshChunk )
	{
		CRenderElement_MeshChunk* meshChunk = static_cast< CRenderElement_MeshChunk* >( element );
		static_cast< CRenderProxy_Mesh* >( element->GetProxy() )->OnCollectElement( this, false, meshChunk );
	}
}


void CRenderCollector::CollectElement( IRenderProxyBase* proxy  )
{
	ERenderProxyType proxyType = proxy->GetType();
	switch ( proxyType )
	{
	case RPT_PointLight:
	case RPT_SpotLight:
		{
			if( m_info->IsShowFlagOn( SHOW_Lights ) )
			{
				IRenderProxyLight* light = static_cast< IRenderProxyLight* >( proxy );
				light->CollectElements( *this );
			}
			break;
		}

	default:
		{
			RED_FATAL_ASSERT( proxy->IsDrawable(), "CollectElement doesn't handle this render proxy type, but it's not drawable: %i", proxyType );
			IRenderProxyDrawable* drawable = static_cast< IRenderProxyDrawable* >( proxy );
			drawable->CollectElements( *this );
		}
	}
}

void CRenderCollector::ProcessAutoHideProxy( IRenderProxyBase* proxy )
{
	if ( IRenderProxyDrawable* drawable = proxy->AsDrawable() )
	{
		drawable->OnNotVisibleFromAutoHide( *this );
	}
}


Bool CRenderCollector::PushElement( IRenderElement* element )
{
	ASSERT( element->GetMaterial() );
	ASSERT( element->GetMaterialParams() );

	NotifyProxyOfCollectedElement( element );

	ERenderingPlane renderingPlane = element->GetProxy()->GetRenderingPlane();
	ASSERT( renderingPlane < RPl_Max );

	// Push into sort group
	m_renderCollectorData->m_elements[renderingPlane].m_elements[ element->GetSortGroup() ].Push( element );

	m_usedLightChannels |= element->GetProxy()->GetLightChannels();

	// Add to refraction list
	if ( element->GetMaterial()->IsAccumulativelyRefracted() )
	{
		m_renderCollectorData->m_accumulativeRefractionElements.Push( element );
	}

	// Add to reflection list
	if ( element->GetMaterial()->IsReflectiveMasked() )
	{
		m_renderCollectorData->m_reflectiveMaskedElements.Push( element );
	}

	return true;
}

Bool CRenderCollector::PushDecal( CRenderProxy_Decal* decal )
{
	RED_ASSERT( decal->GetRenderIndex() < EDDI_MAX , TXT("Unknown decal render type") );
	m_renderCollectorData->m_decals[ decal->GetRenderIndex() ].PushBackUnique( decal );
	return true;
}

Bool CRenderCollector::PushDynamicDecalChunk( CRenderDynamicDecalChunk* decalChunk )
{
	m_renderCollectorData->m_dynamicDecalChunks.PushBack( decalChunk );
	return true;
}

void CRenderCollector::PushOnScreenParticle(CRenderProxy_Particles* proxy)
{
	m_renderCollectorData->m_particles[SCREENCAT_OnScreen].PushBack( proxy->GetEmitters() );
}

void CRenderCollector::PushOffScreenParticle(CRenderProxy_Particles* proxy)
{
	m_renderCollectorData->m_particles[SCREENCAT_OffScreen].PushBack( proxy->GetEmitters() );
}

Bool CRenderCollector::PushForegroundElement( IRenderElement* element )
{
	NotifyProxyOfCollectedElement( element );

	m_renderCollectorData->m_foregroundElements.Push( element );
	return true;
}

Bool CRenderCollector::HasAnyDecals() const
{
	static_assert( EDDI_MAX == 4, "Those arrays should be expanded to new size" );
	if( m_renderCollectorData )
	{
		return	!m_renderCollectorData->m_decals[0].Empty() || 
				!m_renderCollectorData->m_decals[1].Empty() ||
				!m_renderCollectorData->m_decals[2].Empty() ||
				!m_renderCollectorData->m_decals[3].Empty() ;
	}
	return false;
}

Bool CRenderCollector::HasAnyDecalsType( EDynamicDecalRenderIndex type ) const
{
	if( m_renderCollectorData )
	{
		return !m_renderCollectorData->m_decals[ (Uint32)type ].Empty();
	}
	return false;
}

void CRenderCollector::RenderDecals( EDynamicDecalRenderIndex type , const class RenderingContext& context )
{
	PC_SCOPE_PIX(RenderDecals);

	CRenderShaderPair *shader = nullptr;
	if ( GpuApi::DRAWCONTEXT_DecalsFocusMode == GpuApi::GetDrawContext() )
	{
		shader = GetRenderer()->m_shaderSimpleDecalFocusMode;
	}
	else
	{
		switch( type )
		{
			// Decide what shader to use
		case EDDI_REGULAR : 
			shader = GetRenderer()->m_shaderSimpleDecal;
			break;
		case EDDI_SPECULARED : 
			shader = GetRenderer()->m_shaderSimpleDecalSpeculared;
			break;
		case EDDI_NORMALMAPPED : 
			shader = GetRenderer()->m_shaderSimpleDecalNormalmapped;
			break;
		case EDDI_SPECULARED_NORMALMAPPED : 
			shader = GetRenderer()->m_shaderSimpleDecalSpecularedNormalmapped;
			break;
		default:
			RED_HALT( "Unknown bucket for dynamic decals" );
		}
	}

	if( !shader )
	{
		return;
	}

	shader->Bind();

	{
		RED_ASSERT( m_renderCollectorData );
		TDynArray< CRenderProxy_Decal* >& array = m_renderCollectorData->m_decals[ type ];

		for ( CRenderProxy_Decal* decal : array )
		{
			if ( decal )
			{
				decal->Render( context, *m_info );
			}
		}
	}
}

void CRenderCollector::RenderDynamicDecals( const RenderingContext& context )
{
	GetRenderer()->GetDynamicDecalBatcher()->RenderDecalChunks( *m_info, context, m_renderCollectorData->m_dynamicDecalChunks );
}

void CRenderCollector::RenderStripes( const class RenderingContext& context, Bool projected )
{
	PC_SCOPE_PIX(RenderStripes);
	
	if ( m_scene )
	{
		m_scene->RenderStripes( context, *m_info, this, projected );
	}
}

#ifdef USE_NVIDIA_FUR
void CRenderCollector::UpdateFurSkinning()
{
	if ( !m_info->IsShowFlagOn( SHOW_GeometrySkinned ) ) return;

	PC_SCOPE_PIX(UpdateFurSkinning);

	if ( m_scene )
	{
		m_scene->UpdateFurSkinning();
	}
}

void CRenderCollector::RenderFur( const class RenderingContext& context )
{
	if ( !m_info->IsShowFlagOn( SHOW_GeometrySkinned ) ) 
	{
		return;
	}

	PC_SCOPE_PIX(RenderFur);

#ifdef USE_UMBRA
	if ( !m_collectWithUmbra )
	{
		if ( m_scene )
		{
			m_scene->RenderFur( *this, context );
		}
	}
	else
	{
		RED_FATAL_ASSERT( m_renderCollectorData, "Collector data is null" );
		if ( context.m_pass == RP_ShadowDepthSolid )
		{
			for ( CRenderProxy_Fur* furProxy : m_renderCollectorData->m_furShadowProxies )
			{
				RED_FATAL_ASSERT( furProxy, "Fur proxy nulled out between collection and render." );
				furProxy->Render( *this, context );
			}
		}
		else
		{
			for ( CRenderProxy_Fur* furProxy : m_renderCollectorData->m_furProxies )
			{
				RED_FATAL_ASSERT( furProxy, "Fur proxy nulled out between collection and render." );
				furProxy->Render( *this, context );
			}
		}
	}
#else
	if ( m_scene )
	{
		m_scene->RenderFur( *this, context );
	}
#endif // USE_UMBRA
}
#endif // USE_NVIDIA_FUR

#ifdef USE_UMBRA
const CRenderOcclusionData& CRenderCollector::GetOcclusionData() const
{
	return *m_occlusionData;
}
#endif // USE_UMBRA

void CRenderCollector::FinishShadowCulling()
{
	PC_SCOPE_PIX( FinishShadowCulling );

	if ( m_shadowCullingTask )
	{
		while (!m_shadowCullingTask->IsFinished())
		{
			RED_BUSY_WAIT();
			// block
		}

		m_shadowCullingTask->Release();
		m_shadowCullingTask = nullptr;
	}

	if( m_scene )
	{
		m_scene->TickCollectedProxies();
	}
}


void CRenderCollector::FinishSceneOcclusionQuery()
{
	PC_SCOPE_PIX( FinishSceneOcclusionQuery );

	if ( !m_sceneCullingTask )
	{
		return;
	}

	m_sceneCullingTask->RunToOrWait( CSceneCullingTask::PHASE_OcclusionQuery );
}

void CRenderCollector::FinishSceneStaticsCulling()
{
	PC_SCOPE_PIX( FinishSceneStaticsCulling );

	if ( m_sceneCullingTask )
	{
		m_sceneCullingTask->RunToOrWait( CSceneCullingTask::PHASE_CollectStaticMeshes );
	}

	if ( m_scene )
	{
		m_scene->TickCollectedProxies();
	}
}

void CRenderCollector::FinishSceneCulling()
{
	PC_SCOPE_PIX( FinishSceneCulling );

	if ( m_sceneCullingTask )
	{
		m_sceneCullingTask->RunToOrWait( CSceneCullingTask::PHASE_CollectScene );
	}

	if( m_scene )
	{
		m_scene->TickCollectedProxies();
	}
}

void CRenderCollector::FinishDynamicShadowsCollection()
{
	PC_SCOPE_PIX( FinishDynamicShadowsCollection );

	if ( m_sceneCullingTask )
	{
		m_sceneCullingTask->RunToOrWait( CSceneCullingTask::PHASE_BuildDynamicShadowElements );
	}

	if( m_scene )
	{
		m_scene->TickCollectedProxies();
	}
}


void CRenderCollector::FinishSceneCullingTask()
{
	PC_SCOPE_PIX( FinishSceneCullingTask );

	if ( m_sceneCullingTask )
	{
		m_sceneCullingTask->RunToOrWait( CSceneCullingTask::PHASE_Finished );

		m_sceneCullingTask->Release();
		m_sceneCullingTask = nullptr;


		if( m_scene )
		{
			m_scene->TickCollectedProxies();
		}
	}
}



Bool CRenderCollector::IsWorldScene() const
{
	return m_scene && m_scene->IsWorldScene();
}

Bool CRenderCollector::WasLastFrameCameraInvalidated() const
{
	return !GetRenderCamera().GetLastFrameData().m_isValid;
}
