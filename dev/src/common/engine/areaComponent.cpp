/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "areaComponent.h"
#include "areaShapeBuilder.h"
#include "negativeAreaComponent.h"
#include "clipMap.h"
#include "../core/scriptStackFrame.h"
#include "renderFrameInfo.h"
#include "viewport.h"
#include "renderFragment.h"
#include "../core/mathUtils.h"
#include "game.h"
#include "world.h"
#include "layer.h"
#include "renderFrame.h"
#include "component.h"
#include "entity.h"


IMPLEMENT_RTTI_ENUM( EAreaTerrainSide );
IMPLEMENT_RTTI_ENUM( EAreaClippingMode );
IMPLEMENT_ENGINE_CLASS( CAreaComponent );

RED_DEFINE_STATIC_NAME( CCameraEffectTrigger );

/*//----/

class CAreaComponentCompilationMutex
{
public:
	static CAreaComponentCompilationMutex& GetInstance()
	{
		static CAreaComponentCompilationMutex theInstance;
		return theInstance;
	}

	void Acquire()
	{
		m_mutex.Acquire();
	}

	void Release()
	{
		m_mutex.Release();
	}

private:
	Red::Threads::CMutex		m_mutex;
};

//----*/

Red::Threads::CMutex CAreaComponent::st_dirtyAreaShapesLock;
CAreaComponent::TDirtyAreaShapes CAreaComponent::st_dirtyAreaShapes;

//----

CAreaComponent::CAreaComponent()
	: m_height( 2.0f )
	, m_color( Color::RED )
	, m_editing( false )
	, m_saveShapeToLayer( true )
	, m_terrainSide( ATS_AboveAndBelowTerrain )
	, m_runtimeDirtyFlag( 0 )
	, m_compiledAreaDebugMesh( NULL )
	, m_compiledAreaShape( NULL )
	, m_compiledAreaClosestPointFinder( NULL )
{
}

CAreaComponent::~CAreaComponent()
{
	SAFE_RELEASE( m_compiledAreaDebugMesh );
	SAFE_RELEASE( m_compiledAreaShape );

	if ( m_compiledAreaClosestPointFinder )
	{
		delete m_compiledAreaClosestPointFinder;
		m_compiledAreaClosestPointFinder = NULL;
	}
}

Bool CAreaComponent::SupportsAdditionalInstanceData() const
{
	return m_saveShapeToLayer;
}

void CAreaComponent::UpdateWorldPoints()
{
	Matrix matrix;
	GetLocalToWorld( matrix );
	m_worldPoints.Resize( m_localPoints.Size() );
	for ( Uint32 i=0; i<m_localPoints.Size(); i++ )
	{
		m_worldPoints[i] = matrix.TransformPoint( m_localPoints[i] );
	}
}

Color CAreaComponent::CalcLineColor() const
{
	return m_color;
}

Float CAreaComponent::CalcLineWidth() const
{
	return 1.0f;
}

Float CAreaComponent::CalcHeightWS() const
{
	return m_height * m_localToWorld.GetRow( 2 ).Mag3();
}

void CAreaComponent::OnSerializeAdditionalData( IFile& file )
{
	// Pass to base class
	TBaseClass::OnSerializeAdditionalData( file );

	// Serialize vertices
	file << m_localPoints;
	file << m_worldPoints;

	// Make sure we are writing valid data
	if ( file.IsWriter() )
		RecompileAreaShape();

	// Save/Load area shape
	CAreaShape::Serialize(file, m_compiledAreaShape);

	// Reset runtime area flags
	m_runtimeDirtyFlag = 0;
}

void CAreaComponent::OnSerialize( IFile& file )
{
	// Pass to base class
	TBaseClass::OnSerialize( file );

	// Only real save/load
	if (!file.IsGarbageCollector())
	{	
		// Serialize trigger shape		
		if (file.GetVersion() < VER_NEW_AREA_COMPONENT_SHAPE_DATA)
		{
			// Load old legacy data (no longer used in new case)
			CAreaBSP tempBSP;
			tempBSP.Serialize( file );

			// Recompile area shape from original vertices
			if ( file.IsReader() )
				RecompileAreaShape();
		}
		else
		{
			// Make sure we are writing valid data
			if (file.IsWriter() && !file.IsMapper() && m_runtimeDirtyFlag)
				RecompileAreaShape(); // make sure we save valid data

			// Save/Load area shape
			if ( !CAreaShape::Serialize(file, m_compiledAreaShape) )
				RecompileAreaShape(); // failed to read, recompile new one

			// shape appears to be valid
			m_runtimeDirtyFlag = 0;
		}
	}
}

void CAreaComponent::OnSpawned( const SComponentSpawnInfo& spawnInfo )
{
	// Pass to base class
	TBaseClass::OnSpawned( spawnInfo );

	if ( m_localPoints.Empty() )
	{
		// Add 4 quad corners
		m_localPoints.PushBack( Vector( -0.5f, -0.5f, 0.0f ) );
		m_localPoints.PushBack( Vector( +0.5f, -0.5f, 0.0f ) );
		m_localPoints.PushBack( Vector( +0.5f, +0.5f, 0.0f ) );
		m_localPoints.PushBack( Vector( -0.5f, +0.5f, 0.0f ) );
	}
}

void CAreaComponent::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );

	// Cache transform
	m_worldToLocal = GetLocalToWorld().FullInverted();

	// Register in editor fragment list
	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_Areas );
	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_AreaShapes );
	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_EffectAreas );
}

void CAreaComponent::OnDetached( CWorld* world )
{
	TBaseClass::OnDetached( world );

	// Unregister from editor fragment list
	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_Areas );
	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_AreaShapes );
	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_EffectAreas );
}

void CAreaComponent::OnPropertyPostChange( CProperty* prop )
{
	TBaseClass::OnPropertyPostChange( prop );

	if ( prop->GetName() == TXT("clippingMode") || prop->GetName() == TXT("clippingAreaTags") )
	{
		InvalidateAreaShape();
	}
}

#ifndef NO_DATA_VALIDATION
void CAreaComponent::OnCheckDataErrors( Bool isInTemplate ) const
{
	// Pass to base class
	TBaseClass::OnCheckDataErrors( isInTemplate );

	// Select error category
	const Char* category = TXT("World");

	// Check area shape
	if (NULL != m_compiledAreaShape)
	{
		// TODO: it's almost impossible to break the area shape, do we need a test here ?
	}
}
#endif // NO_DATA_VALIDATION

#ifndef NO_EDITOR
void CAreaComponent::OnNavigationCook( CWorld* world, CNavigationCookingContext* context )
{
	m_worldToLocal = GetLocalToWorld().FullInverted();

	TBaseClass::OnNavigationCook( world, context );
}

#endif

void CAreaComponent::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag )
{
	// Pass to base class
	TBaseClass::OnGenerateEditorFragments( frame, flag );

	// Hack because someone had the brilliant idea to use plain area trigger for area effects so we cannot
	// filter these areas using the area classes...
	if ( flag == SHOW_EffectAreas && GetEntity()->GetClass()->GetName() == CNAME( CCameraEffectTrigger ) )
	{
		// Draw the outline
		DrawArea( frame, m_worldPoints );
		return;
	}

	// Draw area box
	if ( (flag == SHOW_Areas || flag == SHOW_AreaShapes) && !frame->GetFrameInfo().IsClassRenderingDisabled( GetClass() ) 
		&& !frame->GetFrameInfo().IsTemplateRenderingDisabled( GetEntity()->GetEntityTemplate(), GetClass() ) )
	{
		// Do not draw the area twice for effect areas
		if ( flag == SHOW_EffectAreas && GetEntity()->GetClass()->GetName() == CNAME( CCameraEffectTrigger ) )
		{
			return;
		}

		// Draw the convex pieces
		if (flag == SHOW_AreaShapes)
		{
			DrawAreaShapes( frame );
		}

		// Draw the outline
		if (flag == SHOW_Areas)
		{
			if (!frame->GetFrameInfo().IsShowFlagOn(SHOW_AreaShapes) || IsSelected())
			{
				DrawArea( frame, m_worldPoints );
			}
		}
	}
}

void CAreaComponent::DrawArea( CRenderFrame* frame, const TAreaPoints& worldPoints )
{
	// Do not render lines when rendering hit proxies in vertex edit mode
	if ( m_editing && frame->GetFrameInfo().m_renderingMode == RM_HitProxies )
	{
		return;
	}

	if ( m_worldPoints.Empty() )
	{
		return;
	}

	// Height vector
	Vector height = GetLocalToWorld().TransformVector( Vector( 0.f, 0.f, m_height ) );
	const Bool drawHeight = m_height > 0.0f;

	// Hit proxy mode, render fat lines
	if ( frame->GetFrameInfo().m_renderingMode == RM_HitProxies )
	{
#ifndef NO_COMPONENT_GRAPH
		Color color = GetHitProxyID().GetColor();

		// Render fat contours
		for ( Uint32 i=0; i<worldPoints.Size(); i++ )
		{
			// Base
			const Vector a = worldPoints[ i ];
			const Vector b = worldPoints[ (i+1)%worldPoints.Size() ];
			frame->AddDebugFatLine( a, b, color, 0.1f );

			// Extruded lines
			if ( drawHeight )
			{
				frame->AddDebugFatLine( a, a + height, color, 0.1f );
				frame->AddDebugFatLine( a + height, b + height, color, 0.1f );
			}
		}
#endif
		// Done
		return;
	}

	// Get color
	Color color = CalcLineColor();
	if ( !IsSelected() )
	{
		color.R /= 2;
		color.G /= 2;
		color.B /= 2;
	}

	// Add faces
	if ( drawHeight )
	{
		TDynArray< DebugVertex > vertices;
		TDynArray< Uint16 > indices;
		vertices.Resize( worldPoints.Size() * 4 );
		indices.Resize( worldPoints.Size() * 12 );

		// Alpha
		color.A = 80;

		// Write
		Uint16* index = indices.TypedData();
		DebugVertex* vertex = vertices.TypedData();
		for ( Uint32 i=0; i<worldPoints.Size(); i++, vertex+=4, index+=12 )
		{
			Uint16 base = (Uint16)(i * 4);

			// Faces
			vertex[0].Set( worldPoints[ i ], color );
			vertex[1].Set( worldPoints[ i ] + height, color );
			vertex[2].Set( worldPoints[ (i+1) % worldPoints.Size() ] + height, color );
			vertex[3].Set( worldPoints[ (i+1) % worldPoints.Size() ], color );

			// Indices
			index[0] = base + 0;
			index[1] = base + 1;
			index[2] = base + 2;
			index[3] = base + 0;
			index[4] = base + 2;
			index[5] = base + 3;
			index[6] = base + 2;
			index[7] = base + 1;
			index[8] = base + 0;
			index[9] = base + 3;
			index[10] = base + 2;
			index[11] = base + 0;
		}

		// Draw faces
		new ( frame ) CRenderFragmentDebugPolyList( frame, Matrix::IDENTITY, vertices.TypedData(), vertices.Size(), indices.TypedData(), indices.Size(), RSG_DebugTransparent );
	}

	// Draw lines
	{
		if ( ! IsEnabled() )
		{
			color = Color::RED; // Draw lines RED, if area is disabled
		}
		else
		{
			color.A = 255;
		}

		TDynArray< DebugVertex > vertices;
		
		// Draw contour
		vertices.Resize( worldPoints.Size() * 6 );
		DebugVertex* write = vertices.TypedData();
		for ( Uint32 i=0; i<worldPoints.Size(); i++ )
		{
			// Line segment
			(write++)->Set( worldPoints[ i ], color );
			(write++)->Set( worldPoints[ (i+1)%worldPoints.Size() ], color );

			// Upper line
			if ( drawHeight )
			{
				// Up vector
				(write++)->Set( worldPoints[ i ], color );
				(write++)->Set( worldPoints[ i ] + height, color );

				// Upper segment
				(write++)->Set( worldPoints[ i ] + height, color );
				(write++)->Set( worldPoints[ (i+1)%worldPoints.Size() ] + height, color );
			}
		}

		// Draw lines
		const Uint32 numVertices = PtrDiffToUint32( (void*)(write - &vertices[0]) );
		ASSERT( numVertices <= vertices.Size() );
		
		Float userThickness = frame->GetFrameInfo().GetRenderingDebugOption( VDCommon_DebugLinesThickness );
		frame->AddDebugFatLines( vertices.TypedData(), numVertices, userThickness );
	}
}

void CAreaComponent::DrawAreaShapes( CRenderFrame* frame )
{
	// Do not render lines when rendering hit proxies in vertex edit mode
	if ( m_editing && frame->GetFrameInfo().m_renderingMode == RM_HitProxies )
	{
		return;
	}

	// Do not draw the convex pieces for negative areas
	if (IsA<CNegativeAreaComponent>())
	{
		return;
	}

	// Create the mesh
	if (NULL == m_compiledAreaDebugMesh && NULL != m_compiledAreaShape)
	{
		m_compiledAreaDebugMesh = m_compiledAreaShape->CompileDebugMesh( 0.0f, 0.0f, Matrix::IDENTITY );
	}

	// Draw the mesh
	if (NULL != m_compiledAreaDebugMesh)
	{
        // Hit proxy
#ifndef NO_COMPONENT_GRAPH
        if ( frame->GetFrameInfo().m_renderingMode == RM_HitProxies )
        {
            new ( frame ) CRenderFragmentDebugMesh( frame, GetLocalToWorld(), m_compiledAreaDebugMesh, GetHitProxyID() );
        }
		else
#endif
		{
			new ( frame ) CRenderFragmentDebugMesh( frame, GetLocalToWorld(), m_compiledAreaDebugMesh, true );  // draw as transparent
		}
	}
}

Bool CAreaComponent::TestTerrainCondition( const Vector& worldPoint ) const
{
	if ( m_terrainSide != ATS_AboveAndBelowTerrain )
	{
		// Find height at given point 
		CWorld* world = GGame->GetActiveWorld();
		CClipMap* clipmap = world ? world->GetTerrain() : NULL;

		// Make sure we have a world and a clipmap
		if ( clipmap )
		{
			// Get the height at whatever clipmap level is loaded
			Float height;
			clipmap->GetHeightForWorldPosition( worldPoint, height );

			// Make sure the condition is met
			Bool conditionMet;
			switch ( m_terrainSide )
			{
			case ATS_OnlyAboveTerrain:
				conditionMet = worldPoint.Z >= height;
				break;
			case ATS_OnlyBelowTerrain:
				conditionMet = worldPoint.Z < height;
				break;
			default:
				conditionMet = true;
			}

			return conditionMet;
		}
	}

	return true;
}

void CAreaComponent::OnUpdateTransformComponent( SUpdateTransformContext& context, const Matrix& prevLocalToWorld )
{
	PC_SCOPE( CAreaComponent );

	// Cache current transform
	Matrix oldMatrix; 
	GetLocalToWorld( oldMatrix );

	// Pass to base class, skip the CBoundedComponent implementation
	//TBaseClass::OnUpdateTransform();
	CComponent::OnUpdateTransformComponent( context, prevLocalToWorld );

	// Delete cached data used for point distance searched (this data depends on the area's transformation)
	if ( NULL != m_compiledAreaClosestPointFinder )
	{
		delete m_compiledAreaClosestPointFinder;
		m_compiledAreaClosestPointFinder = NULL;
	}

	// Transform points to world space
	UpdateWorldPoints();

	// Cache transform
	m_worldToLocal = GetLocalToWorld().FullInverted();

	// If we are using clipping to negative areas and we are moved we need to recalculate the shape
	if ( (!GGame->IsActive() && m_clippingMode != ACM_NoClipping) || !m_compiledAreaShape )
	{
		InvalidateAreaShape();
	}

	// Recalculate area bounds when moved
	OnUpdateBounds();
}
#ifndef NO_EDITOR
Bool CAreaComponent::OnEditorBeginVertexEdit( TDynArray< Vector >& vertices, Bool& isClosed, Float& height )
{
	ASSERT( ! m_editing );

	vertices  =  m_worldPoints;
	isClosed  = true;
	height    = m_height;
	m_editing = true;

	// Allow vertex edit
	return true;
}

void CAreaComponent::OnEditorEndVertexEdit()
{
	m_editing = false;
	OnUpdateBounds();
}

void CAreaComponent::OnEditorNodeMoved( Int32 vertexIndex, const Vector& oldPosition, const Vector& wishedPosition, Vector& allowedPosition )
{
	ASSERT( vertexIndex >= 0 );
    ASSERT( (Uint32) vertexIndex < m_localPoints.Size() );

	if ( ! MarkModified() )
	{
		allowedPosition = oldPosition;
		return;
	}

	// No position projection
	allowedPosition = wishedPosition;

	// Update vertex
	m_worldPoints[ vertexIndex ] = allowedPosition;
	m_localPoints[ vertexIndex ] = GetWorldToLocal().TransformPoint( allowedPosition );

	// Update transform
	ScheduleUpdateTransformNode();

	// Mark compiled trigger shape as dirty (not up to date)
	InvalidateAreaShape();
}

Bool CAreaComponent::OnEditorVertexDestroy( Int32 vertexIndex )
{
    // To few vertices already
	if ( m_localPoints.Size() <= 3 )
	{
		return false;
	}
	if ( vertexIndex < 0 || (Uint32)vertexIndex >= m_localPoints.Size() )
	{
		return false;
	}

	// Remove from vertices list
	m_localPoints.Erase( m_localPoints.Begin() + vertexIndex );
	m_worldPoints.Erase( m_worldPoints.Begin() + vertexIndex );

	// Mark compiled trigger shape as dirty (not up to date)
	InvalidateAreaShape();

	// Deleted
	return true;
}

Bool CAreaComponent::OnEditorVertexInsert( Int32 edge, const Vector& wishedPosition, Vector& allowedPosition, Int32& outInsertPos )
{
	Uint32 numEdges = m_worldPoints.Size();

	while ( edge < 0 ) { edge += numEdges; }

	// Insert between first and second edge vertex 
	Uint32 insertPos = ( edge + 1 ) % numEdges;

	// No position projection
	allowedPosition = wishedPosition;

	// Add point
	m_worldPoints.Insert( insertPos, allowedPosition );
	m_localPoints.Insert( insertPos, GetWorldToLocal().TransformPoint( allowedPosition ) );
	outInsertPos = insertPos;

	// Mark compiled trigger shape as dirty (not up to date)
	InvalidateAreaShape();

	return true;
}

#endif

Int32 CAreaComponent::FindClosestEdge( const Vector& point )
{
	Int32 closestEdge = -1;
	Float closestDistance = FLT_MAX;

	// Test distance to edges
	for ( Uint32 i=0; i<m_worldPoints.Size(); i++ )
	{
		// Get edge points
		Vector a = m_worldPoints[i];
		Vector b = m_worldPoints[(i+1)%m_worldPoints.Size()];

		// Calculate distance from edge to point
		Float distanceToEdge = point.DistanceToEdge( a, b );
		if ( distanceToEdge < closestDistance )
		{
			closestDistance = distanceToEdge;
			closestEdge = i;
		}
	}

	// Return first vertex of closest edge 
	return closestEdge;
}

Float CAreaComponent::CalcDistToClosestEdge2D( const Vector& point ) const
{
	Int32 closestEdge = -1;
	Float closestDistance = FLT_MAX;

	// Test distance to edges
	Uint32 j = 1;
	for ( Uint32 i=0; i<m_worldPoints.Size(); i++, j++ )
	{
		// Wrap around
		if ( j == m_worldPoints.Size() )
		{
			j = 0;
		}		

		// Get edge points
		const Vector a = m_worldPoints[i];
		const Vector b = m_worldPoints[j];

		// Calculate distance from edge to point
		Float distanceToEdge = point.DistanceToEdge2D( a, b );
		if ( distanceToEdge < closestDistance )
		{
			closestDistance = distanceToEdge;
			closestEdge = i;
		}
	}

	//return closestEdge == -1 ? -1.0f : closestDistance;
	return closestDistance;
}

Vector CAreaComponent::GetClosestPoint( const Vector& point )
{
	Vector closestPoint;
	MathUtils::GeometryUtils::ClosestPointPolygonPoint2D( m_worldPoints, point.AsVector2(), MSqrt( FLT_MAX ), closestPoint.AsVector2() );

	// Z-test
	closestPoint.Z = Clamp( point.Z, m_boundingBox.Min.Z, m_boundingBox.Max.Z );

	// Return closest point
	return closestPoint;
}

Bool CAreaComponent::TestPointOverlap( const Vector& worldPoint ) const
{
	if ( !m_localPoints.Size() )
	{
		return false;
	}

	// Check if we have a terrain side condition
	if ( !TestTerrainCondition( worldPoint ) )
	{
		return false;
	}

	const Vector localPoint = GetWorldToLocal().TransformPoint( worldPoint );
	return GetCompiledShape().PointOverlap(localPoint);
}

Bool CAreaComponent::TestBoxOverlap( const Box& box ) const
{
	if ( !m_localPoints.Size() )
	{
		return false;
	}

	const Matrix& worldToLocal = GetWorldToLocal();

	// Check if we have a terrain side condition
	// TODO: handle boxes as volumes instead of points from their center
	if ( !TestTerrainCondition( box.CalcCenter() ) )
	{
		return false;
	}

	// Move box center to local space
	Vector boxLocalCenter = worldToLocal.TransformPoint( box.CalcCenter() );

	// Calculate box axes in local space
	Vector boxAxisX = worldToLocal.TransformVector( Vector::EX );
	Vector boxAxisY = worldToLocal.TransformVector( Vector::EY );
	Vector boxAxisZ = worldToLocal.TransformVector( Vector::EZ );

	// Test overlap
	return GetCompiledShape().BoxOverlap( boxLocalCenter, box.CalcExtents(), boxAxisX, boxAxisY, boxAxisZ );
}

Bool CAreaComponent::TestIntersection( CAreaComponent* component )
{
	// quick bounding box exclusion
	if ( !m_boundingBox.Touches( component->m_boundingBox ) )
		return false;

	// full SAT based test
	 return MathUtils::GeometryUtils::IsPolygonsIntersecting2D< Vector >( m_worldPoints, component->m_worldPoints );
}

Bool CAreaComponent::FindClosestPoint( const Vector& worldPoint, const Float maxSearchRadiusWorld, Vector& outClosestPointWorld, Float& outClosestWorldDistance ) const
{
	PC_SCOPE( AreaFindClosestPoint );

	// Compile the search data if needed
	if ( NULL == m_compiledAreaClosestPointFinder )
	{
		m_compiledAreaClosestPointFinder = GetCompiledShape().CompileDistanceSearchData( GetLocalToWorld() );
		ASSERT( m_compiledAreaClosestPointFinder != NULL );
	}

	// Use the compiled data if we have it
	return m_compiledAreaClosestPointFinder->FindClosestPoint( worldPoint, maxSearchRadiusWorld, outClosestPointWorld, outClosestWorldDistance );
}

Bool CAreaComponent::GetFloorLevel( const Vector& worldPoint, Float& outFloorLevel ) const
{
	// Convert to local space
	const Vector localPoint = GetWorldToLocal().TransformPoint( worldPoint );
	const Vector localDir = GetWorldToLocal().TransformVector( Vector::EZ ).Normalized3(); // we assume that the world Z direction is the search dir
	Vector localFloorPoint;
	if ( GetCompiledShape().GetSurfacePoint( localPoint, localDir, localFloorPoint ) )
	{
		const Vector worldFloorPoint = GetLocalToWorld().TransformPoint( localFloorPoint );
		outFloorLevel = worldFloorPoint.Z;
		return true;
	}

	// point outside the range
	return false;
}

Bool CAreaComponent::GetCeilingLevel( const Vector& worldPoint, Float& outCeilingLevel ) const
{
	// Convert to local space
	const Vector localPoint = GetWorldToLocal().TransformPoint( worldPoint );
	const Vector localDir = GetWorldToLocal().TransformVector( -Vector::EZ ).Normalized3(); // we assume that the world Z direction is the search dir
	Vector localCeilingPoint;
	if ( GetCompiledShape().GetSurfacePoint( localPoint, localDir, localCeilingPoint ) )
	{
		const Vector worldCeilingPoint = GetLocalToWorld().TransformPoint( localCeilingPoint );
		outCeilingLevel = worldCeilingPoint.Z;
		return true;
	}

	// point outside the range
	return false;
}

Float CAreaComponent::GetBoudingAreaRadius() const
{
	return GetBoundingBox().CalcSize().Mag2() / 2.0f;
}

void CAreaComponent::GenerateTriMesh( TDynArray< Vector >& outVerts, TDynArray< Uint32 >& outIndices, Bool onlyWalls ) const
{
	if ( m_worldPoints.Empty() )
	{
		return;
	}

	const Box& bbox = GetBoundingBox();
	Float minZ = bbox.Min.Z;
	Float maxZ = bbox.Max.Z;
	// spawn wall
	for ( Uint32 i = 0, vertsCount = m_worldPoints.Size(); i < vertsCount; ++i )
	{
		const Vector2& v = m_worldPoints[ i ].AsVector2();

		outVerts.PushBack( Vector( v.X, v.Y, minZ ) );
		outVerts.PushBack( Vector( v.X, v.Y, maxZ ) );

		// add indices
		if ( i > 0 )
		{
			Uint32 baseInd = outVerts.Size();
			// wall tri1
			outIndices.PushBack( baseInd-1 );
			outIndices.PushBack( baseInd-2 );
			outIndices.PushBack( baseInd-3 );
			// wall tri2
			outIndices.PushBack( baseInd-4 );
			outIndices.PushBack( baseInd-3 );
			outIndices.PushBack( baseInd-2 );
		}
	}

	Uint32 indiceLimit = outVerts.Size();

	// final quad wrapping poly end with beginning
	outIndices.PushBack( 0 );
	outIndices.PushBack( indiceLimit-2 );
	outIndices.PushBack( indiceLimit-1 );

	outIndices.PushBack( indiceLimit-1 );
	outIndices.PushBack( 1 );
	outIndices.PushBack( 0 );

	if ( onlyWalls )
	{
		return;
	}

	// spawn roof & base
	for ( Uint32 i = 2, vertsCount = m_worldPoints.Size(); i < vertsCount; ++i )
	{
		// base
		outIndices.PushBack( 0 );
		outIndices.PushBack( (i-1)*2 );
		outIndices.PushBack( i*2 );

		// roof
		outIndices.PushBack( i*2 + 1);
		outIndices.PushBack( (i-1)*2 + 1 );
		outIndices.PushBack( 1 );
	}
}

void CAreaComponent::OnUpdateBounds()
{
	m_boundingBox.Clear();
	
	for( Uint32 i = 0; i < m_worldPoints.Size(); ++i )
	{
		m_boundingBox.AddPoint( m_worldPoints[i] );
	}
	m_boundingBox.Max.Z += CalcHeightWS();
}

void CAreaComponent::InvalidateAreaShape()
{
	// add to the global dirty list
	// this list is processed once per frame AFTER the update transform
	// this is a HACK that is here only to save PERFORMANCE - 
	// I don't want to add any system that may affect the performance of the whole area/trigger system just because we have 0.1% crash probability in editor
	if ( 0 == Red::Threads::AtomicOps::Or32( &m_runtimeDirtyFlag, 1 ) )
	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( st_dirtyAreaShapesLock );
		st_dirtyAreaShapes.PushBack( THandle< CAreaComponent >( this ) );
	}
}

void CAreaComponent::SetLocalPoints( const CAreaComponent::TAreaPoints& points, Bool updateWorldPoints /* = false */ )
{
	m_localPoints = (const TAreaPoints&) points;

	InvalidateAreaShape();

	ForceUpdateTransformNodeAndCommitChanges();

	// Transform points to world space if needed
	if ( updateWorldPoints )
	{
		UpdateWorldPoints();
	}

	ForceUpdateBoundsNode();
}

Float CAreaComponent::CalcBottomZ() const
{
	return !m_worldPoints.Empty() ? m_worldPoints[0].Z : GetWorldPositionRef().Z;
}

const CAreaShape& CAreaComponent::GetCompiledShape() const
{
	// return shape
	if ( m_compiledAreaShape )
		return *m_compiledAreaShape;

	// return empty shape as the default
	return CAreaShape::EMPTY();
}

CAreaShapePtr CAreaComponent::GetCompiledShapePtr() const
{
	return m_compiledAreaShape;
}

Bool CAreaComponent::RecompileAreaShape()
{
	// remove compiled data for point searches
	if ( NULL != m_compiledAreaClosestPointFinder )
	{
		delete m_compiledAreaClosestPointFinder;
		m_compiledAreaClosestPointFinder = NULL;
	}

	// Destroy preview mesh
	if ( NULL != m_compiledAreaDebugMesh )
	{
		m_compiledAreaDebugMesh->Release();
		m_compiledAreaDebugMesh = NULL;
	}

	// Release current mesh
	if ( NULL != m_compiledAreaShape )
	{
		m_compiledAreaShape->Release();
		m_compiledAreaShape = NULL;
	}

	// Add current outline to the builder
	CAreaShapeBuilder builder;
	builder.SetReferenceSpace(GetLocalToWorld());
	builder.AddOutline(*this, false);
	
	// Find negative areas touching this area
	if (m_clippingMode != ACM_NoClipping)
	{
		CLayer* parentLayer = GetLayer();
		if (NULL != parentLayer)
		{
			const LayerEntitiesArray& entities = parentLayer->GetEntities();
			for (Uint32 i=0; i<entities.Size(); ++i)
			{
				CEntity* entity = entities[i];
				if (NULL != entity)
				{
					const TDynArray<CComponent*>& components = entity->GetComponents();
					for (Uint32 j=0; j<components.Size(); ++j)
					{
						CNegativeAreaComponent* nac = Cast<CNegativeAreaComponent>(components[j]);
						if (NULL != nac && nac->GetBoundingBox().Touches(GetBoundingBox()))
						{
							if (m_clippingAreaTags.Empty() || TagList::MatchAny(nac->m_clippingAreaTags, m_clippingAreaTags))
							{
								builder.AddOutline(*nac, true);
							}
						}
					}
				}
			}
		}
	}

	// Build the shape
	m_compiledAreaShape = builder.Compile();
	if (NULL == m_compiledAreaShape)
	{
		WARN_ENGINE(TXT("Failed to compile trigger shape for '%ls'"), GetFriendlyName().AsChar());
		m_runtimeDirtyFlag = 0;
		return false;
	}

	// Inform any related geometry that the shape was rebuild
	OnAreaShapeChanged();

	// Reset dirty flag
	m_runtimeDirtyFlag = 0;
	return true;
}

void CAreaComponent::ProcessPendingAreaUpdates()
{
	PC_SCOPE_PIX( RebuildDirtyAreas );

	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( st_dirtyAreaShapesLock );

	TDirtyAreaShapes areasToUpdate;
	::Swap( areasToUpdate, st_dirtyAreaShapes );

	for ( THandle< CAreaComponent > ac : areasToUpdate )
	{
		if ( ac )
		{
			ac->RecompileAreaShape();
		}
	}
}

void CAreaComponent::OnAreaShapeChanged()
{
}

//---------------------------------------------------------------------------

void CAreaComponent::funcTestPointOverlap( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, point, Vector::ZEROS );
	FINISH_PARAMETERS;
	RETURN_BOOL( TestPointOverlap( point ) );
}

void CAreaComponent::funcTestEntityOverlap( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle<CEntity> ,HEntity , NULL );
	FINISH_PARAMETERS;
	CEntity* entity = HEntity.Get();
	if( entity )
	{
		RETURN_BOOL( TestBoxOverlap( entity->CalcBoundingBox() ) );
	}
	else
	{
		RETURN_BOOL( false );
	}
}

void CAreaComponent::funcGetLocalPoints( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( TDynArray< Vector >, points, TDynArray< Vector >() );
	FINISH_PARAMETERS;

	points.Reserve( m_localPoints.Size() );
	for ( const Vector& v : m_localPoints )
		points.PushBack( v );
}

void CAreaComponent::funcGetWorldPoints( CScriptStackFrame& stack, void* result )
{	
	GET_PARAMETER_REF( TDynArray< Vector >, points, TDynArray< Vector >() );
	FINISH_PARAMETERS;

	points.Reserve( m_worldPoints.Size() );
	for ( const Vector& v : m_worldPoints )
		points.PushBack( v );
}

void CAreaComponent::funcGetBoudingAreaRadius( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_FLOAT( GetBoudingAreaRadius() );
}
