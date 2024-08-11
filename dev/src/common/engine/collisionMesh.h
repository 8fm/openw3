/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../core/events.h"
#include "collisionContent.h"
#include "../core/resource.h"
#include "hitProxyId.h"

class ICollisionShape;
class CCompiledCollision;
class IRenderResource;
class CRenderFrame;

/// Mesh used for physics collision
/// Collision mesh is composed of various primitives:
/// Boxes, spheres, cylinders, capsules, convexes or triangle meshes
class CCollisionMesh : public CResource, public ICollisionContent
#ifndef NO_EDITOR_EVENT_SYSTEM
	, public IEdEventListener
#endif
{
	DECLARE_ENGINE_RESOURCE_CLASS( CCollisionMesh, CResource, "w2col", "Collision mesh" );

public:
	// Render context
	struct RenderContext
	{
		Matrix						m_localToWorld;			//!< Local transformation
		CHitProxyID					m_hitProxyID;			//!< Hit proxy ID of drawed object
		Bool						m_selected;				//!< Draw as selected
		Bool						m_solid;				//!< Draw as solid mesh

		RED_INLINE RenderContext()
			: m_localToWorld( Matrix::IDENTITY )
			, m_selected( false )
			, m_solid( true )
		{};
	};


protected:
	TDynArray< ICollisionShape* >		m_shapes;		//!< List of shapes in this collision mesh
	Float								m_occlusionAttenuation;
	Float								m_occlusionDiagonalLimit;
	Int32									m_swimmingRotationAxis;

	// Mutable, since it is created on-the-fly in GenerateFragments().
	mutable IRenderResource*			m_debugMesh;		//!< Debug mesh for drawing collision

public:
	//! Get the list of collision shapes
	RED_INLINE const TDynArray< ICollisionShape* >& GetShapes() const { return m_shapes; }

	RED_INLINE void SetOcclusionAttenuation( Float occlusionAttenuation ) { m_occlusionAttenuation = occlusionAttenuation; }
	RED_INLINE void SetOcclusionDiagonalLimit( Float occlusionDiagonalLimit ) { m_occlusionDiagonalLimit = occlusionDiagonalLimit; }
	RED_INLINE void SetSwimmingRotationAxis( Int32 rotAx ) { 	m_swimmingRotationAxis = rotAx; }

	RED_INLINE Float GetOcclusionAttenuation() const { return m_occlusionAttenuation; }
	RED_INLINE Float GetOcclusionDiagonalLimit() const { return m_occlusionDiagonalLimit; }
	RED_INLINE Int32	 GetSwimmingRotationAxis() const { return m_swimmingRotationAxis; }
public:
	CCollisionMesh();
	virtual ~CCollisionMesh();

	//! Get additional resource info, displayed in editor
	virtual void GetAdditionalInfo( TDynArray< String >& info ) const;

	//! Generate rendering fragments
	void GenerateFragments( CRenderFrame* frame, const RenderContext& context ) const;

protected:
	ICollisionShape* AddShape( ICollisionShape* shape );
public:
	ICollisionShape* AddBox( const Matrix& localToMesh, const Vector& halfExtends, const CName& physicalMaterialName = CNAME( default ) );
	ICollisionShape* AddSphere( const Matrix& localToMesh, Float radius, const CName& physicalMaterialName = CNAME( default ) );
	ICollisionShape* AddCapsule( const Matrix& localToMesh, Float radius, Float height, const CName& physicalMaterialName = CNAME( default ) );
	ICollisionShape* AddConvex( const Matrix& localToMesh, const Vector* vertices, Uint32 count, const CName& physicalMaterialName = CNAME( default ) );
	ICollisionShape* AddConvex( const Matrix& localToMesh, const CMesh* mesh, const CName& physicalMaterialName = CNAME( default ) );
	ICollisionShape* AddTriMesh( const Matrix& localToMesh, const Vector* vertices, Uint32 vertexCount, const Uint32* triangleIndices, Uint32 triangleCount, const TDynArray< unsigned short >& triangleMaterials = TDynArray< unsigned short >(), const TDynArray< CName >& physicalMaterialName = TDynArray< CName >(), Bool flipWinding = false );
	ICollisionShape* AddTriMesh( const Matrix& localToMesh, const CMesh* mesh, const TDynArray< CName >& physicalMaterialNames = TDynArray< CName >() );

	void RemoveAll();
	void RemoveShape( Int32 index );

	void Append( const CCollisionMesh& other, const Matrix& m );

#ifndef NO_EDITOR
	void InvalidateCollision();

	TDynArray< Float > GetDensityScalers() const;
	void FillDensityScalers( const TDynArray< Float >& scalers );
#endif

#ifndef NO_EDITOR_EVENT_SYSTEM
	virtual void DispatchEditorEvent( const CName& name, IEdEventData* data );
#endif

public:
	//! Build compiled mesh for given settings ( scale, etc ) from this collision mesh
	CompiledCollisionPtr CompileCollision( CObject* collsion ) const override final;
};

BEGIN_CLASS_RTTI( CCollisionMesh )
	PARENT_CLASS( CResource )
	PROPERTY( m_shapes );
	PROPERTY( m_occlusionAttenuation );
	PROPERTY( m_occlusionDiagonalLimit );
	PROPERTY( m_swimmingRotationAxis );
END_CLASS_RTTI();

