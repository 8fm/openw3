/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "areaBSP.h"
#include "areaConvex.h"
#include "boundedComponent.h"

class IRenderResource;

enum EAreaTerrainSide
{
	ATS_AboveAndBelowTerrain=0,
	ATS_OnlyAboveTerrain=1,
	ATS_OnlyBelowTerrain=2
};

BEGIN_ENUM_RTTI( EAreaTerrainSide );
	ENUM_OPTION( ATS_AboveAndBelowTerrain );
	ENUM_OPTION( ATS_OnlyAboveTerrain );
	ENUM_OPTION( ATS_OnlyBelowTerrain );
END_ENUM_RTTI();

/// The way the shape of the area is clipped with negative areas
enum EAreaClippingMode
{
	// Area is not clipped at all (fastest method)
	ACM_NoClipping = 0,

	// Area is clipped to negative areas with matching Tags
	ACM_ClipToNegativeAreas = 1,
};

BEGIN_ENUM_RTTI( EAreaClippingMode );
	ENUM_OPTION( ACM_NoClipping );
	ENUM_OPTION( ACM_ClipToNegativeAreas );
END_ENUM_RTTI();

/// Editable area component
class CAreaComponent : public CBoundedComponent
{
	DECLARE_ENGINE_ABSTRACT_CLASS( CAreaComponent, CBoundedComponent );

public:
	typedef TDynArray< Vector >		TAreaPoints;

	TAreaPoints						m_localPoints;			// Local space points
	TAreaPoints						m_worldPoints;			// World space points
	Matrix							m_worldToLocal;			// Cached world to local matrix
	Float							m_height;				// Height
	Color							m_color;				// Line color
	EAreaTerrainSide				m_terrainSide;			// Side of the terrain this area applies to
	Bool							m_saveShapeToLayer;		// Save the area shape to the layer
	Bool							m_editing;				// We are editing the area right now

public:
	// Get number of vertices
	RED_INLINE Uint32 GetNumVertices() const { return m_localPoints.Size(); }

	// Get local space vertices
	RED_INLINE const TAreaPoints& GetLocalPoints() const { return m_localPoints; }

	// Get world space vertices
	RED_INLINE const TAreaPoints& GetWorldPoints() const { return m_worldPoints; }

	// Get area world to local matrix
	RED_INLINE const Matrix& GetWorldToLocal() const { return m_worldToLocal; }

	// Get area world to local matrix
	RED_INLINE void GetWorldToLocal( Matrix& worldToLocal ) const { worldToLocal = m_worldToLocal; }

	// Get area height
	RED_INLINE const Float GetHeight() const { return m_height; }

	//! Get clipping tags
	RED_INLINE const TagList& GetClippingTags() const { return m_clippingAreaTags; }

	//! Get terrain side
	RED_INLINE EAreaTerrainSide GetTerrainSide() const { return m_terrainSide; }

public:
	CAreaComponent();
	virtual ~CAreaComponent();

	// Get contour rendering color
	virtual Color CalcLineColor() const;

	// Get contour line width
	virtual Float CalcLineWidth() const;

	// Calculate world space contour height
	virtual Float CalcHeightWS() const;

	// Get area's bottom Z coord
	Float CalcBottomZ() const;

	// Find edge closest to given point
	Int32 FindClosestEdge( const Vector& point );

	// Calculates a 2D distance to area edge
	Float CalcDistToClosestEdge2D( const Vector& point ) const;

	// Find closest point on the area edge to the given point
	Vector GetClosestPoint( const Vector& point );
	
	// Test if given point is inside the area
	virtual Bool TestPointOverlap( const Vector& worldPoint ) const;

	// Test if given box is inside the area
	virtual Bool TestBoxOverlap( const Box& box ) const;

	// test area-area intersection
	Bool TestIntersection( CAreaComponent* component );

	// Find closest point on the surface of this area, returns true if found, false if not found. Returns 0 distance if point is inside.
	Bool FindClosestPoint( const Vector& worldPoint, const Float maxSearchRadiusWorld, Vector& outClosestPointWorld, Float& outClosestWorldDistance ) const;

	// Calculate floor level (min Z) for given XY coordinates within the area. Returns false if the point is outside the area.
	Bool GetFloorLevel( const Vector& worldPoint, Float& outFloorLevel ) const;

	// Calculate ceiling level (max Z) for given XY coordinates within the area. Returns false if the point is outside the area.
	Bool GetCeilingLevel( const Vector& worldPoint, Float& outCeilingLevel ) const;

	// Get radius of a sphere that contains area
	Float GetBoudingAreaRadius() const;

	// Generate triangle mesh
	void GenerateTriMesh( TDynArray< Vector >& outVerts, TDynArray< Uint32 >& outIndices, Bool onlyWalls = false ) const;

	// Get trigger area shape, always returns some shape - in case of invalid areas it returns empty shape
	const CAreaShape& GetCompiledShape() const;

	// Get trigger area shape smart pointer for external usage
	CAreaShapePtr GetCompiledShapePtr() const;

	// Recompile trigger shape from local outline
	Bool RecompileAreaShape();

	// Set local points
	void SetLocalPoints( const CAreaComponent::TAreaPoints& points, Bool updateWorldPoints = false );

	// Invalidate compiled shape of the area (do after any major change in editor, most of the internal operations are updated automatically)
	void InvalidateAreaShape();

	// Rebuild area shapes if requested, required because rebuilding in OnUpdateTransform is not safe
	static void ProcessPendingAreaUpdates();

	//////////////////
	// Vertex editing interface
	//////////////////
#ifndef NO_EDITOR
	// Begin vertex edit mode
	virtual Bool OnEditorBeginVertexEdit( TDynArray< Vector >& vertices, Bool& isClosed, Float& height );

	// End vertex edit mode
	virtual void OnEditorEndVertexEdit();

	// Insert editor vertex
	virtual Bool OnEditorVertexInsert( Int32 edge, const Vector& wishedPosition, Vector& allowedPosition, Int32& outInsertPos );

	// Editor vertex deleted
	virtual Bool OnEditorVertexDestroy( Int32 vertexIndex );

	// Editor vertex was moved
	virtual void OnEditorNodeMoved( Int32 vertexIndex, const Vector& oldPosition, const Vector& wishedPosition, Vector& allowedPosition );
#endif
protected:
	// Spawning
	virtual void OnSpawned( const SComponentSpawnInfo& spawnInfo );

	// Generate editor fragments
	virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag );

	// Transformation
	virtual void OnUpdateTransformComponent( SUpdateTransformContext& context, const Matrix& prevLocalToWorld ) override;

	//! Update bounds
	virtual void OnUpdateBounds();

	// Serialization
	virtual void OnSerialize( IFile& file );

	//! Serialize additional component data (if entity is templated)
	virtual void OnSerializeAdditionalData( IFile& file );

	// Attach to world
	virtual void OnAttached( CWorld* world );

	// Detach from world
	virtual void OnDetached( CWorld* world );

#ifndef NO_DATA_VALIDATION
	// Check data, can be called either for node on the level or for node in the template
	virtual void OnCheckDataErrors( Bool isInTemplate ) const;
#endif // NO_DATA_VALIDATION

#ifndef NO_EDITOR
	// cooking
	virtual void OnNavigationCook( CWorld* world, CNavigationCookingContext* context ) override;
#endif

	// Property changed
	virtual void OnPropertyPostChange( CProperty* prop );

	// Draw area
	virtual void DrawArea( CRenderFrame* frame, const TAreaPoints & worldPoints );

	// Draw area convex shapes
	virtual void DrawAreaShapes( CRenderFrame* frame );

	// Compiled shape of this area has changed
	virtual void OnAreaShapeChanged();

	// Checks if the terrain condition is met for the given point in world space
	Bool TestTerrainCondition( const Vector& worldPoint ) const;

	// Does this component support additional data saving in layer (even for template entity) ?
	virtual Bool SupportsAdditionalInstanceData() const;

	// Recalculate world point coordinates form local coordinates
	void UpdateWorldPoints();

	//----------

	// Compiled shape of trigger volume (list of convex pieces)
	CAreaShape*								m_compiledAreaShape; // Reference counted

	// Drawable mesh with area shape
	IRenderResource*						m_compiledAreaDebugMesh;

	// Optional clipping mode
	EAreaClippingMode						m_clippingMode;

	// Optional negative area tag - matching areas will be subtracted from this shape
	TagList									m_clippingAreaTags;

	// Cached structure used for closest point searched
	mutable CAreaShapeClosestPointFinder*	m_compiledAreaClosestPointFinder;

	// Area components to rebuild
	typedef TDynArray< THandle< CAreaComponent >, MC_AreaShapes >		TDirtyAreaShapes;
	static TDirtyAreaShapes					st_dirtyAreaShapes;
	static Red::Threads::CMutex				st_dirtyAreaShapesLock;

	// Thread safe area flags - SET ONLY
	Red::Threads::AtomicOps::TAtomic32		m_runtimeDirtyFlag;

	///-------------
	// Scripts
	///-------------

	void funcTestPointOverlap( CScriptStackFrame& stack, void* result );
	void funcTestEntityOverlap( CScriptStackFrame& stack, void* result );
	void funcGetLocalPoints( CScriptStackFrame& stack, void* result );
	void funcGetWorldPoints( CScriptStackFrame& stack, void* result );
	void funcGetBoudingAreaRadius( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CAreaComponent );
	PARENT_CLASS( CBoundedComponent );
	PROPERTY_EDIT( m_height, TXT("Height of the area") );	 
	PROPERTY_EDIT( m_color, TXT("Editor line color") );
	PROPERTY_EDIT( m_terrainSide, TXT("Side of the terrain the area applies to") );
	PROPERTY_EDIT( m_clippingMode, TXT("(Advanced) Clipping mode for trigger shape") );
	PROPERTY_EDIT( m_clippingAreaTags, TXT("(Advanced) Matching negative areas will be subtracted from this area shape") );
	PROPERTY_EDIT( m_saveShapeToLayer, TXT("Save the area shape to the layer (for entity templates)") );
	PROPERTY( m_localPoints );
	PROPERTY( m_worldPoints );
	NATIVE_FUNCTION( "TestEntityOverlap", funcTestEntityOverlap );
	NATIVE_FUNCTION( "TestPointOverlap", funcTestPointOverlap );
	NATIVE_FUNCTION( "GetLocalPoints", funcGetLocalPoints );
	NATIVE_FUNCTION( "GetWorldPoints", funcGetWorldPoints );
	NATIVE_FUNCTION( "GetBoudingAreaRadius", funcGetBoudingAreaRadius );
END_CLASS_RTTI();
