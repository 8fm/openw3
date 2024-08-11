/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "renderProxyApex.h"
#include "renderElementMap.h"
#include "renderElementApex.h"
#include "renderHelpers.h"
#include "renderCollector.h"
#include "renderScene.h"
#include "renderMaterial.h"
#include "renderVisibilityQueryManager.h"
#include "renderDynamicDecalChunk_Apex.h"

#ifdef USE_APEX

#include "..\physics\physXEngine.h"

#include "NxApexNameSpace.h"
#include "NxResourceProvider.h"
#include "NxApexRenderable.h"
#include "..\engine\apexWrapperBase.h"
#include "..\engine\destructionSystemComponent.h"
#include "..\engine\clothComponent.h"

using namespace physx;
using namespace physx::apex;

CRenderProxy_Apex::CRenderProxy_Apex( const RenderProxyInitInfo& initInfo )
	: IRenderProxyDissolvable( RPT_Apex, initInfo )
	, IRenderEntityGroupProxy( initInfo.m_entityGroup )
	, m_apexType( AT_Unknown )
	, m_renderable( NULL )
	, m_hasMaterialReplacement( false )
	, m_drawOriginalMaterials( false )
	, m_originalElementCount( 0 )
	, m_isRenderableDirty( false )
	, m_wetness( 0.0f )
	, m_useShadowDistances( false )
	, m_skipOcclusionTest( false )
{
	ASSERT( initInfo.m_component );
	ASSERT( initInfo.m_component->IsA< CDestructionSystemComponent >() || initInfo.m_component->IsA< CClothComponent >() );

	CMeshTypeResource::TMaterials materials;
	TDynArray< String > materialNames;

	CApexResource* apexResource = nullptr;

	// TODO: If CDestructionSystemComponent and CClothComponent were derived from a common base (CApexComponent), this could be
	// much simpler. Both branches have the same code!
	if (initInfo.m_component->IsA< CDestructionSystemComponent >())
	{
		m_apexType = AT_Destructible;

		const CDestructionSystemComponent* dsc = static_cast< const CDestructionSystemComponent* >( initInfo.m_component );
		m_wrapper = const_cast< CApexWrapper* >( static_cast< const CApexWrapper* >(dsc->GetPhysicsRigidBodyWrapper()) );
		if (dsc->HasResource())
		{
			materialNames = dsc->GetApexMaterialNames();
			materials = dsc->GetMaterials();
		}

		if ( dsc->IsCastingShadows() )
		{
			m_drawableFlags |= RPF_CastingShadows;
		}
		else if (dsc->IsCastingShadowsFromLocalLightsOnly() )
		{
			m_drawableFlags |= RPF_CastShadowsFromLocalLightsOnly;
		}

		apexResource = dsc->GetApexResource();

		m_shadowDistanceSquared = dsc->GetShadowDistance( m_renderMask );
	}
	else if (initInfo.m_component->IsA< CClothComponent >())
	{
		m_apexType = AT_Cloth;

		const CClothComponent* cc = static_cast< const CClothComponent* >( initInfo.m_component );
		m_wrapper = const_cast< CApexWrapper* >( static_cast< const CApexWrapper* >(cc->GetClothWrapper()) );
		if ( cc->HasResource() )
		{
			materialNames = cc->GetApexMaterialNames();
			materials = cc->GetMaterials();
		}

		if ( cc->IsCastingShadows() )
		{
			m_drawableFlags |= RPF_CastingShadows;
		}
		else if (cc->IsCastingShadowsFromLocalLightsOnly() )
		{
			m_drawableFlags |= RPF_CastShadowsFromLocalLightsOnly;
		}

		apexResource = static_cast< CApexResource* >( cc->GetMeshTypeResource() );

		//////////////////////////////////////////////////////////////////////////
		// // HACK! HACK! HACK! to fix blinking cloth in Ciri cutscene
		if ( apexResource && apexResource->GetDepotPath() == TXT("environment\\decorations\\trade\\market_stand_poor\\stall_tent_high_oneside_px.redcloth") )
		{
			if ( initInfo.m_component->GetWorld() && initInfo.m_component->GetWorld()->GetDepotPath() == TXT("levels\\kaer_morhen\\kaer_morhen.w2w") )
			{
				m_skipOcclusionTest = true;
			}
		}
		// HACK END
		//////////////////////////////////////////////////////////////////////////

		m_shadowDistanceSquared = cc->GetShadowDistance( m_renderMask );
	}

	m_shadowDistanceSquared *= m_shadowDistanceSquared;

	if ( apexResource != nullptr )
	{
		if ( apexResource->IsTwoSided() )
		{
			m_drawableFlags |= RPF_IsTwoSided;
		}
	}


	if( m_wrapper )
	{
		m_wrapper->AddRef();

		m_renderable = m_wrapper->AcquireRenderable();
		m_isRenderableDirty = true;
	}

	// APEX proxies are always dynamic
	m_drawableFlags |= RPF_Dynamic;


	// If the material names are not in-sync with the materials, we won't create anything. This might happen after doing a re-import
	// while the apex resource is in use. Since the apex material names are re-generated during OnSave, and proxies are refreshed
	// both before and after saving, the first refresh will result in no apex material names. In the second refresh we'll have names,
	// so it'll be fine.
	if ( materialNames.Size() == materials.Size() )
	{
		// Create a render element for each render material. Create them here instead of when we are attached, so that we can discard
		// the material arrays, rather than keeping them around so we can create elements later.
		m_renderElements.Reserve( materials.Size() );
		for ( Uint32 i = 0; i < materials.Size(); ++i )
		{
			IMaterial* material = materials[ i ].Get();

			// If we don't have a material, we can't create a render element.
			if ( !material ) continue;

			const IMaterialDefinition* definition = material->GetMaterialDefinition();

			CRenderMaterial* renderMaterial = NULL;
			CRenderMaterialParameters* renderParameters = NULL;

			// Extract material settings
			ExtractRenderResource( definition, renderMaterial ); //calls addref
			ExtractRenderResource( material, renderParameters );

			// If we don't have a material or parameters, then the material probably hasn't been fully created yet or something.
			// In any case, we won't be able to render anything.
			if ( !renderMaterial || !renderParameters ) continue;

			m_renderElements.PushBack( new CRenderElement_Apex( this, renderMaterial, renderParameters, materialNames[i], m_renderable ) );

			// Release the references on the render resources, since the render element got its own.
			renderMaterial->Release();
			renderParameters->Release();
		}
	}

	// initialize LOD selector with single LOD with auto hide dinstance
	GetLodSelector().SetupSingle( m_autoHideDistance );
}

CRenderProxy_Apex::~CRenderProxy_Apex()
{
	DisableMaterialReplacement();

	if( m_wrapper && m_renderable )
	{
		m_wrapper->ReleaseRenderable( m_renderable );
		m_renderable = NULL;
	}

	for ( Uint32 i = 0; i < m_renderElements.Size(); ++i )
	{
		m_renderElements[i]->Release();
	}
	m_renderElements.ClearFast();

	if( m_wrapper )
	{
		((IRenderObject*)m_wrapper)->Release();
	}
	m_renderable = NULL;
}


void CRenderProxy_Apex::Prefetch( CRenderFramePrefetch* prefetch ) const
{
	// Same as when rendering, just use cached distance directly here, rather than CalcCameraDistanceSqForTextures. Apex is generally
	// small, so this should be close enough.
	const Float distanceSq = CalcCameraDistanceSq( prefetch->GetCameraPosition(), prefetch->GetCameraFovMultiplierUnclamped() );
	for ( CRenderElement_Apex* element : m_renderElements )
	{
		prefetch->AddMaterialBind( element->GetMaterial(), element->GetMaterialParams(), distanceSq );
	}
}


void CRenderProxy_Apex::AttachToScene( CRenderSceneEx* scene )
{
	IRenderProxyDrawable::AttachToScene( scene );
	IRenderEntityGroupProxy::AttachToScene();

	// TEMP HACK : To get apex shadows dissolving reasonably, can't use last allocated frame for initial frame index.
	// The reason for setting to last allocated frame is that at one point it helped make dissolve-on-attach work (or maybe
	// auto-hide dissolve). But there seem to be problems with that already (flashes visible when attached, even if beyond
	// the auto-hide distance), so I can at least make the shadows work, and then solve the other stuff later...
	m_frameTracker.SetFrameIndex( scene->GetLastAllocatedFrame() - 2 );


	// Push materials to Apex. This will likely cause the same material name to get updated many times, but since the names uniquely identify the material,
	// it will just mean setting the same material name to the same material object multiple times.
	// The registration is done when we attach rather than when we create the elements, to ensure we don't have a frame or two where the render thread is
	// using old proxies, but new materials have been registered -- this results in flickering of apex objects.
#ifdef USE_APEX
	for ( Uint32 i = 0; i < m_renderElements.Size(); ++i )
	{
		CRenderElement_Apex* element = m_renderElements[i];
		RegisterApexMaterial( element->GetMaterialName(), element->GetMaterial(), element->GetMaterialParams() );
	}
#endif
}

void CRenderProxy_Apex::DetachFromScene( CRenderSceneEx* scene )
{
	IRenderProxyDrawable::DetachFromScene( scene );
	IRenderEntityGroupProxy::DetachFromScene();

	ClearDynamicDecalChunks();
}

void CRenderProxy_Apex::SetApexRenderable( physx::apex::NxApexRenderable* renderable )
{
	ASSERT( m_wrapper, TXT("SetApexRenderable(), but no wrapper") );
	if ( !m_wrapper ) return;

	ASSERT( renderable != m_renderable, TXT("Got same NxApexRenderable as before. This shouldn't happen for cloths, and destructibles shouldn't be getting a new renderable after creation.") );
	ASSERT( renderable, TXT("Got a NULL NxApexRenderable...") );
	if ( renderable == m_renderable || !renderable ) return;

	if ( m_renderable )
	{
		m_wrapper->ReleaseRenderable( m_renderable );
	}
	m_renderable = renderable;

	for ( Uint32 i = 0; i < m_renderElements.Size(); ++i )
	{
		m_renderElements[i]->SetRenderable( m_renderable );
	}

	m_isRenderableDirty = true;
}

void CRenderProxy_Apex::UpdateRenderResources()
{
	if ( m_renderable != nullptr && m_isRenderableDirty )
	{
		m_renderable->lockRenderResources();
		// Provide a non-null user data, so that apexRenderInterface will create a normal render resource.
		// TODO : Investigate if it would be useful to do something with the pointer? I think the last time I checked,
		// it wasn't reliable when using the apex renderable pools...
		m_renderable->updateRenderResources( false, this );
		m_renderable->unlockRenderResources();
		m_isRenderableDirty = false;
	}
}

void CRenderProxy_Apex::SetUseShadowDistances( Bool enable )
{
	m_useShadowDistances = enable;
}

void CRenderProxy_Apex::CollectedTick( CRenderSceneEx* scene )
{
	if ( !m_wrapper ) return;

	// HACK : This sucks, we should be getting rid of dependencies on the wrapper here. But, don't have access to the
	// cloth actor here, and we don't currently have another way to handle the scene's forced LOD.
	if ( m_apexType == AT_Cloth )
	{
		Int32 sceneForcedLod = scene->GetForcedLOD();
		if ( sceneForcedLod >= 0 )
		{
			( ( CApexClothWrapper* )m_wrapper )->ForceLODLevel( sceneForcedLod );
		}
	}

	if ( m_renderable )
	{
		UpdateRenderResources();
#ifndef RED_FINAL_BUILD
		extern SceneRenderingStats GRenderingStats;
		if ( m_apexType == AT_Destructible )
		{
			++GRenderingStats.m_numApexDestructiblesUpdated;
		}
		else if ( m_apexType == AT_Cloth )
		{
			++GRenderingStats.m_numApexClothsUpdated;
		}
#endif
	}
}


void CRenderProxy_Apex::OnNotVisibleFromAutoHide( CRenderCollector& collector )
{
	if ( !IsVisible() || m_renderElements.Empty() ) return;

	if ( !collector.m_frame->GetFrameInfo().IsShowFlagOn( SHOW_Apex ) ) return;
	if ( !collector.m_frame->GetFrameInfo().IsShowFlagOn( SHOW_GeometrySkinned ) && m_apexType == AT_Cloth ) return;
	if ( !collector.m_frame->GetFrameInfo().IsShowFlagOn( SHOW_GeometryStatic ) && m_apexType == AT_Destructible ) return;

	UpdateOncePerFrame( collector );
}


void CRenderProxy_Apex::CollectElements( CRenderCollector& collector )
{
	if ( !IsVisible() || m_renderElements.Empty() ) return;

	if ( !collector.m_frame->GetFrameInfo().IsShowFlagOn( SHOW_Apex ) ) return;
	if ( !collector.m_frame->GetFrameInfo().IsShowFlagOn( SHOW_GeometrySkinned ) && m_apexType == AT_Cloth ) return;
	if ( !collector.m_frame->GetFrameInfo().IsShowFlagOn( SHOW_GeometryStatic ) && m_apexType == AT_Destructible ) return;

	UpdateOncePerFrame( collector );

	if ( !IsFadeVisible() )
	{
		return;
	}

	for ( Uint32 i = 0; i < m_renderElements.Size(); ++i )
	{
		collector.PushElement( m_renderElements[i] );
	}

	// query update
	UpdateVisibilityQueryState( CRenderVisibilityQueryManager::VisibleScene );

#ifndef RED_FINAL_BUILD
	extern SceneRenderingStats GRenderingStats;
	if ( m_apexType == AT_Destructible )
	{
		++GRenderingStats.m_numApexDestructiblesRendered;
	}
	else if ( m_apexType == AT_Cloth )
	{
		++GRenderingStats.m_numApexClothsRendered;
	}
#endif
}

void CRenderProxy_Apex::CollectCascadeShadowElements( SMergedShadowCascades& cascades, Uint32 perCascadeTestResults )
{
	if ( !cascades.m_collector )
	{
		return;
	}

	CRenderCollector &collector = *cascades.m_collector;

	if ( m_renderElements.Empty() ) return;
	if ( !collector.m_frame->GetFrameInfo().IsShowFlagOn( SHOW_Apex ) ) return;

	UpdateOncePerFrame( collector );

	if ( GetShadowFadeAlpha().IsZero() )
	{
		return;
	}

	// Process all cascades
	Bool isVisibleInCascades = false;
	const Uint32 numCascades = cascades.m_numCascades;
	for ( Uint32 cascadeIndex=0; cascadeIndex<numCascades; ++cascadeIndex, perCascadeTestResults >>= 2 )
	{
		SShadowCascade* cascade = &cascades.m_cascades[ cascadeIndex ];

		// Test the visibility of the fragment in this cascade
		const Uint32 visResult = perCascadeTestResults & 3;

		// We are not in the cascade
		if ( visResult == 0 )
		{
			continue;
		}

		for ( Uint32 i = 0; i < m_renderElements.Size(); ++i )
		{
			if ( IsSortGroupCastingShadow( m_renderElements[i]->GetSortGroup() ) )
			{
				cascade->PushShadowElement( m_renderElements[i] );
				isVisibleInCascades = true;
			}
		}

#ifndef RED_FINAL_BUILD	
		extern SceneRenderingStats GRenderingStats;
		if ( m_apexType == AT_Destructible )
		{
			++GRenderingStats.m_numApexDestructiblesRenderedSM;
		}
		else if ( m_apexType == AT_Cloth )
		{
			++GRenderingStats.m_numApexClothsRenderedSM;
		}
#endif

		// do not add to bigger collect if we are fully inside the smaller cascade
		// THIS WORKS ONLY IF BLENDING BETWEEN CASCADES IS DISABLED
		if ( visResult == 2 )
		{
			break;
		}
	}

	// query update
	if ( isVisibleInCascades )
	{
		UpdateVisibilityQueryState( CRenderVisibilityQueryManager::VisibleMainShadows );
	}
}

void CRenderProxy_Apex::CollectLocalShadowElements( const CRenderCollector& collector, const CRenderCamera& shadowCamera, SShadowCascade& elementCollector )
{
	UpdateOncePerFrame( collector );

	if ( GetShadowFadeAlpha().IsZero() )
	{
		return;
	}

	UpdateVisibilityQueryState( CRenderVisibilityQueryManager::VisibleAdditionalShadows );

	for ( Uint32 i = 0; i < m_renderElements.Size(); ++i )
	{
		elementCollector.PushShadowElement( m_renderElements[i] );
	}
}

const EFrameUpdateState CRenderProxy_Apex::UpdateOncePerFrame( const CRenderCollector& collector )
{
	const auto ret = IRenderProxyDrawable::UpdateOncePerFrame( collector );
	if ( ret == FUS_AlreadyUpdated )
	{
		return ret;
	}

	const Bool wasVisibleLastFrame = ( ret == FUS_UpdatedLastFrame );
	
	IRenderProxyFadeable::UpdateDistanceFade( GetCachedDistanceSquared(), m_autoHideDistanceSquared, wasVisibleLastFrame && !collector.WasLastFrameCameraInvalidated() );

	UpdateShadowFadingValues( collector, wasVisibleLastFrame );

	// For destruction, we don't get a new renderable each frame, so we just manually mark it dirty here.
	if ( m_apexType == AT_Destructible )
	{
		m_isRenderableDirty = true;
	}

	if( collector.m_scene && ( IsFadeVisible() || !GetShadowFadeAlpha().IsZero() ) )
	{
		collector.m_scene->RequestCollectedTickAsync( this );
	}

	return ret;
}

void CRenderProxy_Apex::UpdateShadowFadingValues( const CRenderCollector& collector, const Bool wasUpdatedPreviousFrame )
{
	if ( m_useShadowDistances && GetEntityGroup() )
	{
		// Update shadow fade
		GetEntityGroup()->UpdateOncePerFrame( collector );

		// Get shadow alpha
		SetShadowfadeAlpha( GetEntityGroup()->GetShadowFadeAlpha( collector.GetDissolveSynchronizer(), false ) );
	}
	else
	{
		const Bool isVisible = GetCachedDistanceSquared() <= m_shadowDistanceSquared;

		const auto& dissolveSync = collector.GetDissolveSynchronizer();
		// Process fading and visibility
		if ( wasUpdatedPreviousFrame )
		{
			if ( isVisible )
			{
				m_shadowDissolve.FadeIn( dissolveSync );
			}
			else
			{
				m_shadowDissolve.FadeOut( dissolveSync );
			}
		}
		else
		{
			// Proxy was not visible last frame - we can switch the visibility instantly
			if ( isVisible )
			{
				m_shadowDissolve.SetAlphaOne();
			}
			else
			{
				m_shadowDissolve.SetAlphaZero();
			}
		}

		SetShadowfadeAlpha( m_shadowDissolve.ComputeAlpha( dissolveSync ) );
	}
}

Bool CRenderProxy_Apex::IsDissolved() const
{
	// Destructibles are always counted as dissolved, to allow chunks to fade out.
	// TODO : Detect whether we actually have fading chunks
	if ( m_apexType == AT_Destructible )
		return true;

	return IRenderProxyDissolvable::IsDissolved();
}

Bool CRenderProxy_Apex::IsShadowDissolved() const
{
	// Destructibles are always counted as dissolved, to allow chunks to fade out.
	// TODO : Detect whether we actually have fading chunks
	if ( m_apexType == AT_Destructible )
		return true;

	return IRenderProxyDissolvable::IsShadowDissolved();
}

void CRenderProxy_Apex::CreateDynamicDecalChunks( CRenderDynamicDecal* decal, DynamicDecalChunkList& outChunks )
{
	if ( m_renderable == nullptr )
	{
		return;
	}

	// We don't update the render resource here, but rather just use whatever previous state is has. This should be good enough,
	// and avoids a case where we might create chunks on a destructible with dirty flag set (maybe it was recently created and
	// hasn't been rendered yet), so we update it, and then later that frame collect it for some sort of rendering and update it
	// again... most of the time, this should be fine (destructibles seem to behave properly and don't update again if they have
	// the same state), but on the off chance of apex updating something somewhere in there, we'd end up updating twice, which
	// will crash on PS4 (multiple discard locks in one frame).

	CRenderDynamicDecalChunk_Apex* newChunk = CRenderDynamicDecalChunk_Apex::Create( decal, this );
	if ( newChunk != nullptr )
	{
		outChunks.PushBack( newChunk );
	}
}

void CRenderProxy_Apex::SetReplacedMaterial( IRenderResource* material, IRenderResource* parameters, Bool drawOriginal )
{
	// Remove previous replacement (if any)
	DisableMaterialReplacement();

	// Get material and parameters
	CRenderMaterial* renderMaterial = static_cast< CRenderMaterial* >( material );
	CRenderMaterialParameters* renderParameters = static_cast< CRenderMaterialParameters* >( parameters );

	// Keep draw original flag
	m_drawOriginalMaterials = drawOriginal;

	// If we want to draw the original too, create new elements with the replacement
	if ( drawOriginal )
	{
		// Store original number of elements
		m_originalElementCount = m_renderElements.Size();

		// Create new elements and set a material replacement
		m_renderElements.Reserve( m_renderElements.Size()*2 );
		for ( Uint32 i=0; i < m_originalElementCount; ++i )
		{
			CRenderElement_Apex* originalElement = m_renderElements[i];
			CRenderElement_Apex* newElement = new CRenderElement_Apex( this, originalElement->GetMaterial(), originalElement->GetMaterialParams(), originalElement->GetMaterialName(), m_renderable );
			newElement->SetMaterialReplacement( renderMaterial, renderParameters );
			m_renderElements.PushBack( newElement );
		}
	}
	else // otherwise set a material replacement in the existing elements
	{
		// Set the material in all elements
		for ( auto it=m_renderElements.Begin(); it != m_renderElements.End(); ++it )
		{
			CRenderElement_Apex* element = *it;
			element->SetMaterialReplacement( renderMaterial, renderParameters );
		}
	}

	// Set the flag
	m_hasMaterialReplacement = true;
}

void CRenderProxy_Apex::DisableMaterialReplacement()
{
	if ( m_hasMaterialReplacement )
	{
		// If we created new elements, destroy the extras
		if ( m_drawOriginalMaterials )
		{
			for ( Uint32 i=m_originalElementCount; i < m_renderElements.Size(); ++i )
			{
				SAFE_RELEASE(m_renderElements[i]);
			}
			m_renderElements.Resize( m_originalElementCount );
		}

		// Reset draw original state
		m_originalElementCount = 0;
		m_drawOriginalMaterials = false;

		// Clear the material replacement from all (remaining) elements
		for ( auto it=m_renderElements.Begin(); it != m_renderElements.End(); ++it )
		{
			CRenderElement_Apex* element = *it;
			element->ClearMaterialReplacement();
		}
	}
}

#endif
