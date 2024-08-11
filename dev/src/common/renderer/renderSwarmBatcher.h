/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CRenderElement_MeshChunk;
class CRenderProxy_Swarm;
class CRenderMesh;

/// Rendering for swarm meshes
class CRenderSwarmBatcher : public IDynamicRenderResource
{
protected:

public:
	CRenderSwarmBatcher();
	~CRenderSwarmBatcher();

	void RenderMeshes( const CRenderFrameInfo& info, const RenderingContext& context, const TDynArray< CRenderElement_MeshChunk* >& batchList, Uint32 renderFlags, class MeshDrawingStats& outMeshStats );

protected:
	bool SelectBatchRenderStates( const Batch& newBatch, const RenderingContext& context );
	void DrawBatchOfMeshes( const CRenderFrameInfo& info, const RenderingContext& context, const Batch& batch, CRenderMesh* mesh, Uint32 chunkIndex, Uint32 renderFlags, CRenderElement_MeshChunk* batchList, class MeshDrawingStats& outMeshStats );

protected:
	// Device Reset/Lost handling
	virtual void OnDeviceLost();
	virtual void OnDeviceReset();
	virtual CName GetCategory() const;
	virtual Uint32 GetUsedVideoMemory() const;

	GpuApi::BufferRef						m_instancesBufferRef;		//!< Instancing buffer
	Int32									m_instancesDataElemOffset;	//!< Current instances write position
};
