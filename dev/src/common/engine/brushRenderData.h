/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class BrushFaceMapping;
class BrushRenderData;
class IRenderProxy;
class CHitProxyID;
class IRenderScene;

/// Brush render vertex
class BrushRenderVertex
{
public:
	Float	m_position[3];		//!< Position ( world space )
	Float	m_normal[3];		//!< Normal
	Float	m_uv[2];			//!< UV mapping
	Float	m_baseUV[2];		//!< Base face mapping
	Float	m_tangent[3];		//!< Tangent
	Float	m_binormal[3];		//!< Bitangent

public:
	//! Manual serialization
	friend IFile& operator<<( IFile& file, BrushRenderVertex& vertex )
	{
		file << vertex.m_position[0] << vertex.m_position[1] << vertex.m_position[2];
		file << vertex.m_normal[0] << vertex.m_normal[1] << vertex.m_normal[2];
		file << vertex.m_uv[0] << vertex.m_uv[1];
		file << vertex.m_baseUV[0] << vertex.m_baseUV[1];
		file << vertex.m_tangent[0] << vertex.m_tangent[1] << vertex.m_tangent[2];
		file << vertex.m_binormal[0] << vertex.m_binormal[1] << vertex.m_binormal[2];
		return file;
	}
};

/// Brush render face
class BrushRenderFace
{
public:
	IMaterial*			m_material;			//!< Rendering material
	Uint16				m_firstVertex;		//!< First render vertex of this face
	Uint16				m_firstIndex;		//!< First render index of this face
	Uint16				m_numVertices;		//!< Number of rendering vertices for this face
	Uint16				m_numIndices;		//!< Number of rendering indices for this face

public:
	//! Get material assigned to this face
	RED_INLINE IMaterial* GetMaterial() const { return m_material; }

	//! Get index of first vertex of this face in the render data vertex buffer
	RED_INLINE Uint16 GetFirstVertex() const { return m_firstVertex; }

	//! Get index of first index of this face in the render data index buffer
	RED_INLINE Uint16 GetFirstIndex() const { return m_firstIndex; }

	//! Get number of vertices
	RED_INLINE Uint16 GetNumVertices() const { return m_numVertices; }

	//! Get number of indices
	RED_INLINE Uint16 GetNumIndices() const { return m_numIndices; }

public:
	BrushRenderFace();
	BrushRenderFace( CBrushFace* sourceFace, Uint16 firstVertex, Uint16 firstIndex );

	//! Update face material
	void UpdateMaterial( IMaterial* material );

	//! Update face mapping
	void UpdateMapping( BrushRenderData& renderData, const Matrix& brushLocalToWorld, const BrushFaceMapping& mapping );

	//! Serialize
	void Serialize( IFile& file );

public:
	//! Serialization operator
	friend IFile& operator<<( IFile& ar, BrushRenderFace& face )
	{
		face.Serialize( ar );
		return ar;
	}
};

/// Brush render data
class BrushRenderData
{
	friend class BrushRenderFace;
	friend class CCSGCompiler;

protected:
	TDynArray< BrushRenderVertex >		m_vertices;				//!< Rendering vertices
	TDynArray< Uint16 >					m_indices;				//!< Rendering indices ( global indices )
	TDynArray< BrushRenderFace >		m_faces;				//!< Faces to render
	TDynArray< IRenderProxy* >			m_renderingProxies;		//!< Faces rendering proxy

public:
	//! Get vertex buffer
	RED_INLINE const TDynArray< BrushRenderVertex >& GetVertices() const { return m_vertices; }

	//! Get index buffer
	RED_INLINE const TDynArray< Uint16 >& GetIndices() const { return m_indices; }

public:
	//! Reset geometry data
	void ResetGeometry();

	//! Update texture mapping
	void UpdateMapping( Int32 faceID, const Matrix& brushLocalToWorld, const BrushFaceMapping& mapping );

	//! Update selection for face
	void SetSelection( Int32 faceID, Bool flag );

	//! Update hit proxy for face
	void SetHitProxy( Int32 faceID, const CHitProxyID& id );

	//! Update face material
	void SetMaterial( Int32 faceID, IMaterial* material );

	//! Serialize
	void Serialize( IFile& file );

	//! Create rendering proxies
	void CreateRenderingProxies( IRenderScene* scene );

	//! Destroy rendering proxies
	void DestroyRenderingProxies( IRenderScene* scene );

public:
	//! Create render face for given brush face, returns FaceID
	Int32 BeginFace( CBrushFace* sourceFace );

	//! Add vertex to face
	Uint16 AddVertex( Int32 faceID, const Vector& position, const Vector& normal, Float faceU, Float faceV );

	//! Add index to face
	void AddIndex( Int32 faceID, Uint16 vertexIndex );

public:
	//! Serialization operator
	friend IFile& operator<<( IFile& ar, BrushRenderData& renderData )
	{
		renderData.Serialize( ar );
		return ar;
	}
};