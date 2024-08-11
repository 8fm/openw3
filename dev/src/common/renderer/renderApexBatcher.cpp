/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "renderApexBatcher.h"
#include "renderSceneBatchers.h"
#include "renderElementApex.h"
#include "renderCollector.h"
#include "renderProxyApex.h"
#include "../engine/renderFragment.h"

using namespace physx;
using namespace physx::apex;

#ifdef USE_APEX

CRenderApexBatcher::CRenderApexBatcher()
{
}


CRenderApexBatcher::~CRenderApexBatcher()
{
}

void CRenderApexBatcher::RenderApex( const CRenderFrameInfo& info, const RenderingContext& context, const TDynArray< CRenderElement_Apex* >& batchList, Uint32 drawFlags, class MeshDrawingStats& outMeshStats )
{
	PC_SCOPE_PIX( RenderApex_ConvertToLinkedList );

	if ( batchList.Empty() ) return;
	if ( !info.IsShowFlagOn( SHOW_Apex ) ) return;

	// Construct list
	CRenderElement_Apex* list = NULL;

	for ( Uint32 i=0; i<batchList.Size(); i++ )
	{
		CRenderElement_Apex* elem = batchList[i];
		elem->SetBatchNext( list );
		list = elem;
	}

	// Render using list
	RenderApex( info, context, list, drawFlags, outMeshStats );
}

void CRenderApexBatcher::RenderApex( const CRenderFrameInfo& info, const RenderingContext& context, CRenderElement_Apex* batchList, Uint32 drawFlags, class MeshDrawingStats& outMeshStats )
{
	PC_SCOPE_PIX( RenderApex );

	if ( !batchList ) return;
	if ( !info.IsShowFlagOn( SHOW_Apex ) ) return;

	m_renderer.m_frameInfo = &info;
	m_renderer.m_context = &context;
#ifndef RED_FINAL_BUILD
	m_renderer.m_meshStats = &outMeshStats;
#endif

	// Save current context, so it can be put back to how it was once we're done.
	const CGpuApiScopedDrawContext drawContextOriginalRestore;
	const CGpuApiScopedTwoSidedRender scopedForcedTwoSided;
	
	// Rendering to GBuffer, need to set appropriate lighting channel stencil bits. The actual reference
	// values will be set per chunk, we just set the basic state here.
	const Bool deferredLighting = (context.m_pass == RP_GBuffer);
	const CGpuApiScopedDrawContext scopedDrawContext;
	if ( deferredLighting )
	{
		GpuApi::eDrawContext curContext = scopedDrawContext.GetOrigDrawContext();
		GpuApi::eDrawContext newContext = GpuApi::GetStencilLightChannelsContext( curContext );
		if ( newContext != curContext )
		{
			GpuApi::SetDrawContext( newContext, 0 );
		}
	}

	const Bool isShadowPass = IsShadowRenderPass( context.m_pass );


	Bool isParticleMaterial = false;
	CRenderMaterial* batchMaterial = nullptr;
	CRenderMaterialParameters* batchMaterialParams = nullptr;
	Bool isDissolved = false;

	// First group into particles and non-particles.
	CRenderBatchByParticleMaterial< CRenderElement_Apex > byParticleFlag( batchList );
	while ( byParticleFlag.Pop( isParticleMaterial, batchList ) )
	{
		if ( isParticleMaterial && ( drawFlags & RABF_Particles ) )
		{
			// Particles need to be sorted back to front. Can't group by material, since that could result in wrong ordering.
			SortBackToFront( context, batchList );
			RenderBatch( context, batchList );
		}
		else if ( !isParticleMaterial && ( drawFlags & ( RABF_Solid | RABF_SolidMasked ) ) )
		{
			// Group by material. The Apex Renderer doesn't account for repeated materials, but the GpuApi still does, so we should get
			// some benefit from that.
			CRenderBatchByMaterialParams< CRenderElement_Apex > byMaterial( batchList );
			while ( byMaterial.Pop( batchMaterialParams, batchMaterial, batchList ) )
			{
				const Bool materialIsMasked = batchMaterialParams->IsMasked();

				// If we're not drawing masked fragments, and the material is masked, we can skip this group. We can't go the other
				// way (not drawing solid, and non-masked material), because we may have dissolved fragments that should still be done.
				if ( !( drawFlags & RABF_SolidMasked ) && materialIsMasked )
				{
					continue;
				}

				CRenderBatchByUseDissolve< CRenderElement_Apex > byDissolve( batchList, isShadowPass );
				while ( byDissolve.Pop( isDissolved, batchList ) )
				{
					const Bool useDiscard = materialIsMasked || isDissolved;

					if ( ( ( drawFlags & RABF_SolidMasked ) && useDiscard ) || ( ( drawFlags & RABF_Solid ) && !useDiscard ) )
					{
						RenderBatch( context, batchList );
					}
				}
			}
		}
	}

	// Not strictly required, but these objects are likely about to be destroyed, so we'll be safe.
	m_renderer.m_context = NULL;
#ifndef RED_FINAL_BUILD
	m_renderer.m_meshStats = NULL;
#endif
}


void CRenderApexBatcher::RenderBatch( const RenderingContext& context, CRenderElement_Apex* batch )
{
	const Bool doLightChannels = GpuApi::IsStencilLightChannelsContext( GpuApi::GetDrawContext() );

	// Could be done better, I'm sure. Might be nice to set up shared render states and such before looping
	// over the batch. Might also be good in the future to run through the elements a single time, calling
	// dispatchRenderResources() a single time on each renderable, and just save out what needs to be drawn.
	// The Apex docs say that the results of the dispatchRenderResources call (render resources, buffers, etc)
	// are valid until the next time updateRenderResources() is called. Could maybe have the Proxy call dispatch
	// when collecting elements, and store out the render data into the element for later use...
	for ( CRenderElement_Apex* elem = batch; elem != NULL; elem = static_cast< CRenderElement_Apex* >( elem->GetBatchNext() ) )
	{
		NxApexRenderable* renderable = elem->GetApexRenderable();
		if ( !renderable ) continue;

		if ( !context.CheckLightChannels( elem->GetProxy()->GetLightChannels() ) )
		{
			continue;
		}

		m_renderer.m_currentMaterial = elem->GetRealMaterial();
		m_renderer.m_currentMaterialParameters = elem->GetRealParameters();
		m_renderer.m_currentProxy = static_cast< CRenderProxy_Apex* >( elem->GetProxy() );
		m_renderer.m_currentElement = elem;

#ifndef NO_COMPONENT_GRAPH
		if ( context.m_pass == RP_HitProxies )
		{
			const CHitProxyID& id = context.m_useConstantHitProxyID ? context.m_constantHitProxyID : elem->GetProxy()->GetHitProxyID();
			GetRenderer()->GetStateManager().SetPixelConst( PSC_HitProxyColor, id.GetColor().ToVector() );
		}
#endif


		// Set dissolve parameters.
		Bool isShadowPass = IsShadowRenderPass( context.m_pass );
		m_renderer.m_dissolve = elem->IsDissolved( isShadowPass );

		if ( m_renderer.m_dissolve && !context.m_forceNoDissolves )
		{
			Vector dissolveValues = elem->GetDissolveValues( isShadowPass );
			GetRenderer()->GetStateManager().SetVertexConst( VSC_DissolveParams, dissolveValues );

			GpuApi::TextureRef dissolvePattern = GpuApi::GetInternalTexture( GpuApi::INTERTEX_DissolvePattern);
			GpuApi::BindTextures( PSSMP_Dissolve, 1, &dissolvePattern, GpuApi::PixelShader );
		}


		if ( elem->GetProxy()->HasClippingEllipse() )
		{
			GetRenderer()->GetStateManager().SetVertexConst( VSC_Custom_Matrix, elem->GetProxy()->GetClippingEllipseParams()->m_localToEllipse );
		}


		// Color shift matrices
		if ( elem->GetMaterial()->IsUsingColorShift() )
		{
			const SRenderProxyDrawableMeshColoringParams* colorParams = elem->GetProxy()->GetColoringParams();
			if ( colorParams )
			{
				GetRenderer()->GetStateManager().SetPixelConst( PSC_ColorOne, colorParams->m_colorShiftMatrix0 );
				GetRenderer()->GetStateManager().SetPixelConst( PSC_ColorTwo, colorParams->m_colorShiftMatrix1 );
			}
			else
			{
				GetRenderer()->GetStateManager().SetPixelConst( PSC_ColorOne, Matrix::IDENTITY );
				GetRenderer()->GetStateManager().SetPixelConst( PSC_ColorTwo, Matrix::IDENTITY );
			}
		}

		// Update effect parameters
		const SRenderProxyDrawableEffectParams* effectParams = elem->GetProxy()->GetEffectParams();
		if ( effectParams != NULL )
		{
			// First parameter
			if ( elem->GetMaterial()->IsUsingEffectParam0() )
			{
				Bool useOverride = elem->HasMaterialReplacement();
				Vector customParam0 = useOverride ? effectParams->m_overrideParams : effectParams->m_customParam0;
				GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom0, customParam0 );
			}

			// Second parameter
			if ( elem->GetMaterial()->IsUsingEffectParam1() )
			{
				GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom1, effectParams->m_customParam1 );
			}
		}
		else // no effect parameters, set zeros if the material needs them
		{
			if ( elem->GetMaterial()->IsUsingEffectParam0() )
			{
				GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom0, Vector::ZEROS );
			}
			if ( elem->GetMaterial()->IsUsingEffectParam1() )
			{
				GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom1, Vector::ZEROS );
			}
		}

		// Set lighting group
		if ( doLightChannels )
		{
			GpuApi::SetDrawContext( GpuApi::GetDrawContext(), elem->GetProxy()->GetLightChannels() );
		}

		renderable->lockRenderResources();
		renderable->dispatchRenderResources( m_renderer );
		renderable->unlockRenderResources();
	}
}



struct RenderElementWithDistance
{
	CRenderElement_Apex* m_element;
	Float m_distanceToCameraSquared;

	RenderElementWithDistance( CRenderElement_Apex* element, const RenderingContext& context )
		: m_element( element )
	{
		m_distanceToCameraSquared = element->GetProxy()->GetBoundingBox().SquaredDistance( context.GetCamera().GetPosition() );
	}
};

static int CompareDistance( const void *a , const void *b ) 
{
	RenderElementWithDistance* elemA = ( RenderElementWithDistance* ) a;
	RenderElementWithDistance* elemB = ( RenderElementWithDistance* ) b;
	if ( elemA->m_distanceToCameraSquared < elemB->m_distanceToCameraSquared )
	{
		return 1;
	}
	else if ( elemA->m_distanceToCameraSquared > elemB->m_distanceToCameraSquared )
	{
		return -1;
	}
	else
	{
		return 0;
	}
}

void CRenderApexBatcher::SortBackToFront( const RenderingContext& context, CRenderElement_Apex*& batch )
{
	// Stick elements into an array so we can sort them. Also need to find out where they are relative to the camera.
	TDynArray< RenderElementWithDistance > elements;
	for ( CRenderElement_Apex* elem = batch; elem != NULL; elem = static_cast< CRenderElement_Apex* >( elem->GetBatchNext() ) )
	{
		elements.PushBack( RenderElementWithDistance( elem, context ) );
	}

	qsort( (void*)&elements[0], elements.Size(), sizeof( RenderElementWithDistance ), CompareDistance );

	// Rebuild list.
	for ( Uint32 i = 0; i < elements.Size() - 1; ++i )
	{
		elements[i].m_element->SetBatchNext( elements[i + 1].m_element );
	}
	elements.Back().m_element->SetBatchNext( NULL );
	batch = elements[0].m_element;
}


void CRenderApexBatcher::OnDeviceLost()
{
}

void CRenderApexBatcher::OnDeviceReset()
{
}

CName CRenderApexBatcher::GetCategory() const
{
	return CNAME( RenderApexBatcher );
}

#endif
