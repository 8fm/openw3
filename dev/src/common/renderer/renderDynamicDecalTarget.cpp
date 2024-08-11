/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "renderDynamicDecalTarget.h"
#include "renderDynamicDecal.h"
#include "renderDynamicDecalChunk.h"
#include "renderCollector.h"

// limits the normal priority decal to 1 layer
const static Uint32 dynamicDecalNormalPriorityLayerLimit = 1;
// limits the high priority to 2 layers
// ...because this is being abused 
// by vfx artists pretty much everywhere
const static Uint32 dynamicDecalHighPriorityLayerLimit = 2;

IDynamicDecalTarget::~IDynamicDecalTarget()
{
	RED_FATAL_ASSERT( m_dyndecalChunks.Empty(), "IDynamicDecalTarget destroyed, but still has chunks! Make sure to call ClearDynamicDecalChunks()" );
}

void IDynamicDecalTarget::AddDynamicDecal( CRenderDynamicDecal* decal )
{
	// Assume we want to limit number of layers of decals at a time
	// Saves great amount of ms...
	// Need to expose better budgeting at some point
	if( m_dyndecalChunks.Size() >= dynamicDecalHighPriorityLayerLimit ) return;
	else if ( m_dyndecalChunks.Size() >= dynamicDecalNormalPriorityLayerLimit )
	{
		// new one is not dismemberment, so don't add
		if ( decal->GetSpawnPriority() < EDynamicDecalSpawnPriority::RDDS_Highest && decal->GetTimeToLive() < NumericLimits< Float >::Infinity() )
		{			
			return;
		}
		// it's dismemberment, so it take preference.
	}

	CGpuApiScopedDrawContext decalDrawContext( GpuApi::DRAWCONTEXT_NoColor_DepthStencilSet );	// Quick hack, create one and change to NoColor_NoDepth
	const GpuApi::RenderTargetSetup originalSetup = GpuApi::GetRenderTargetSetup();
	GpuApi::SetupBlankRenderTargets();

	Uint32 numChunksBefore = m_dyndecalChunks.Size();
	CreateDynamicDecalChunks( decal, m_dyndecalChunks );
	Int32 numChunksCreated = m_dyndecalChunks.Size() - numChunksBefore;

	RED_ASSERT( numChunksCreated >= 0, TXT("DynamicDecalTarget removed decal chunks?") );

	for ( Int32 i = 0; i < numChunksCreated; ++i )
	{
		CRenderDynamicDecalChunk* chunk = m_dyndecalChunks[numChunksBefore + i];
		RED_ASSERT( chunk != nullptr, TXT("Null decal chunk found") );
		if ( chunk != nullptr )
		{
			decal->AddChunk( chunk );
		}
	}

	GpuApi::SetupRenderTargets( originalSetup );
}

void IDynamicDecalTarget::OnDynamicDecalChunkDestroyed( CRenderDynamicDecalChunk* chunk )
{
	if ( m_dyndecalChunks.RemoveFast( chunk ) )
	{
		chunk->Release();
	}
}

void IDynamicDecalTarget::ClearDynamicDecalChunks()
{
	// Destroy our dynamic decal chunks. This will cause our OnDynamicDecalChunkDestroyed to be called,
	// which will release and remove from the array.
	while ( !m_dyndecalChunks.Empty() )
	{
		m_dyndecalChunks[0]->DestroyDecalChunk();
	}
}
