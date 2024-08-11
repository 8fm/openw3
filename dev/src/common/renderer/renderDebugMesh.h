/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

/// Debug mesh 
class CRenderDebugMesh : public IDynamicRenderResource
{
protected:	
	//! Packed mesh vertex
	struct PackedVertex
	{
		Float	m_position[3];
		Uint32	m_color;
	};

protected:
	GpuApi::BufferRef	m_vertices;		//!< Mesh vertices
	GpuApi::BufferRef	m_indices;		//!< Mesh indices
	Uint32				m_numVertices;	//!< Num vertices
	Uint32				m_numIndices;	//!< Num indices

private:
	CRenderDebugMesh( const GpuApi::BufferRef &vertices, const GpuApi::BufferRef &indices, Uint32 numVertices, Uint32 numIndices );

public:
	~CRenderDebugMesh();

	//! Draw mesh
	void Draw( Bool drawAsWireframe, Bool drawSingleColor );

	// Describe resource
	virtual CName GetCategory() const;

	// Calculate video memory used by resource
	virtual Uint32 GetUsedVideoMemory() const;

	// Device lost/reset
	virtual void OnDeviceLost();
	virtual void OnDeviceReset();

public:
	// Compile from static mesh
	static CRenderDebugMesh* Create( const TDynArray< DebugVertex >& vertices, const TDynArray< Uint32 >& indices );
};