/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "boundedComponent.h"

/// Convex planar shape
class CPlanarShapeComponent : public CBoundedComponent
{
	DECLARE_ENGINE_CLASS( CPlanarShapeComponent, CBoundedComponent, 0 );

public:
	TDynArray< Vector >				m_localPoints;		// Local space points
	TDynArray< Vector >				m_worldPoints;		// World space points
	TDynArray< CVertexComponent* >	m_vertices;			// Vertex editing entities

public:
	// Get number of vertices
	RED_INLINE Uint32 GetNumVertices() const { return m_localPoints.Size(); }

	// Get local space vertices
	RED_INLINE const TDynArray< Vector >& GetLocalPoints() const { return m_localPoints; }

	// Get world space vertices
	RED_INLINE const TDynArray< Vector >& GetWorldPoints() const { return m_worldPoints; }

public:
	CPlanarShapeComponent();

	// Get contour and face rendering color
	virtual Color CalcLineColor() const;

	// Get contour and face rendering color
	virtual Color CalcFaceColor() const;

	// Get contour line width
	virtual Float CalcLineWidth() const;

	// Returns true if shape needs to be planar
	virtual Bool HasPlanarConstraint() const;

	// Returns true if shape needs to be convex
	virtual Bool HasConvexConstraint() const;

public:
	// Find edge closest to given point
	Int32 FindClosestEdge( const Vector& point );

#ifndef NO_EDITOR
public:
	// Begin vertex edit mode
	virtual Bool OnEditorBeginVertexEdit( TDynArray< Vector >& vertices, Bool& isClosed, Float& height );

	// End vertex edit mode
	virtual void OnEditorEndVertexEdit() {}

	// Insert editor vertex
	virtual Bool OnEditorVertexInsert( Int32 edge, const Vector& wishedPosition, Vector& allowedPosition, Int32& outInsertPos );

	// Editor vertex deleted
	virtual Bool OnEditorVertexDestroy( Int32 vertexIndex );

	// Editor vertex was moved
	virtual void OnEditorNodeMoved( Int32 vertexIndex, const Vector& oldPosition, const Vector& wishedPosition, Vector& allowedPosition );
#endif

public:
	// Spawning
	virtual void OnSpawned( const SComponentSpawnInfo& spawnInfo );

	// Update transformation
	virtual void OnUpdateTransformComponent( SUpdateTransformContext& context, const Matrix& prevLocalToWorld ) override;

	// Update bounds
	virtual void OnUpdateBounds();

protected:
	// Render planar shape
	void RenderPolygon( CRenderFrame* frame );

protected:
	static Bool IsConvex( const TDynArray< Vector >& points, const Vector& normal );
};

BEGIN_CLASS_RTTI( CPlanarShapeComponent );
	PARENT_CLASS( CBoundedComponent );
	PROPERTY( m_localPoints );
	PROPERTY( m_worldPoints );
END_CLASS_RTTI();