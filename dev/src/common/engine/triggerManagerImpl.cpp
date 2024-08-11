#include "build.h"
#include "triggerManager.h"
#include "triggerEventBuffer.h"
#include "triggerManagerImpl.h"
#include "renderFrame.h"

// minimum (square) distance activator should move on
#define TRIGGER_ACTIVATOR_EPSILON_SQR 0.17f

//---------------------------------------------------------------------------

enum ETriggerTerrainFlags
{
	eTriggerTerrainFlags_None=0,
	eTriggerTerrainFlags_Above=1,
	eTriggerTerrainFlags_Below=2,
	eTriggerTerrainFlags_Both=3,
};

//---------------------------------------------------------------------------

CTriggerConvex::CTriggerConvex( class CTriggerObject* object, const class CAreaShape* shape, const Uint32 convexIndex )
	: m_object( object )
	, m_shape( shape )
	, m_convexIndex( convexIndex )
	, m_referencePlanes( NULL )
	, m_token( NULL )
{
	// Get the source convex
	const CAreaConvex* srcConvex = shape->GetConvex( convexIndex );

	// Create the planes array
	const Uint32 numPlanes = srcConvex->GetNumPlanes();
	const Uint32 numBevelPlanes = 4;
	m_numReferencePlanes = numPlanes + numBevelPlanes;
	m_referencePlanes = (Vector*) RED_MEMORY_ALLOCATE_ALIGNED( MemoryPool_Default, MC_TriggerSystem, sizeof(Vector) * m_numReferencePlanes, 16 );
}

CTriggerConvex::~CTriggerConvex()
{
	ASSERT( m_token == NULL );

	if ( NULL != m_referencePlanes )
	{
		RED_MEMORY_FREE( MemoryPool_Default, MC_TriggerSystem, m_referencePlanes );
		m_referencePlanes = NULL;
	}
}

const Uint32 CTriggerConvex::CalcMemoryUsage() const
{
	return (sizeof(Vector) * m_numReferencePlanes) + sizeof(CTriggerConvex);
}

void CTriggerConvex::RemovePlacement( TTriggerCovnexTree& tree )
{
	if ( NULL != m_token )
	{
		tree.Remove( m_token );
		m_token = NULL;
	}
}

void CTriggerConvex::UpdatePlacement( TTriggerCovnexTree& tree )
{
	// Extract reference space (trigger rotation and scaling)
	// NOTE: the reference matrix does not contain the offset in the world space
	const Matrix& localToReference = m_object->GetLocalToReference();
	const Matrix& localToReferenceInvTrans = m_object->GetLocalToReferenceInvTrans();

	// Extract convex offset
	const CAreaConvex* srcConvex = m_shape->GetConvex(m_convexIndex);
	const Vector offset = srcConvex->GetOffset();

	// Beveling radius
	const Float bevelRadius = m_object->GetBevelRadius();
	const Float bevelRadiusVertical = m_object->GetBevelRadiusVertical();

	// Calculate reference poistion (actual offset in the world space)
	// The reference poistion consists of the offset of local convex piece 
	// (transformed by trigger rotation & scale) and the offset of the whole trigger
	const IntegerVector4& basePosition = m_object->GetPosition();
	m_referencePosition = basePosition + IntegerVector4( localToReference.TransformVector( offset ) );

	// Calculate plane equations in the reference space
	const Uint32 numPlanes = srcConvex->GetNumPlanes();
	for ( Uint32 i=0; i<numPlanes; ++i )
	{
		const Vector& srcPlane = srcConvex->GetPlane(i);
		const Vector& srcPoint = srcPlane * -srcPlane.W;

		const Vector& newPoint = localToReference.TransformPoint( srcPoint );
		const Vector& newNormal = localToReferenceInvTrans.TransformVector( srcPlane ).Normalized3();

		// reconstruct plane
		m_referencePlanes[i] = newNormal;
		m_referencePlanes[i].W = -Vector::Dot3( newNormal, newPoint );

		// in beveling shift only the XY planes
		if ( Abs(newNormal.Z) < 0.707f )
		{
			m_referencePlanes[i].W -= bevelRadius;
		}
		else
		{
			m_referencePlanes[i].W -= bevelRadiusVertical;
		}			
	}

	// Add beveling planes in the world space (works well with AABB testing)
	Vector bevelDirections[4];
	bevelDirections[0].Set4(1,0,0,0);
	bevelDirections[1].Set4(-1,0,0,0);
	bevelDirections[2].Set4(0,1,0,0);
	bevelDirections[3].Set4(0,-1,0,0);
	const Uint32 numVertices = srcConvex->GetNumVertices();
	for ( Uint32 i=0; i<ARRAY_COUNT(bevelDirections); ++i )
	{
		const Vector& dir = bevelDirections[i];

		Float dotMax = -FLT_MAX;
		for ( Uint32 j=0; j<numVertices; ++j )
		{
			const Vector& srcVertex = srcConvex->GetVertex(j);
			const Vector newVertex = localToReference.TransformVector( srcVertex );

			const Float dot = Vector::Dot3( dir, newVertex );
			dotMax = Max< Float >( dot, dotMax );
		}

		// add beveling plane
		m_referencePlanes[ numPlanes + i ].Set4( dir.X, dir.Y, dir.Z, -(dotMax + bevelRadius) );
	}

	// Calculate the trigger space bounding box
	Box localBox;
	localBox.Clear();
	for (Uint32 i=0; i<numVertices; ++i)
	{
		const Vector& srcVertex = srcConvex->GetVertex(i);

		const Vector newVertex = localToReference.TransformVector( srcVertex );
		localBox.AddPoint( newVertex );
	}

	// Make the box bigger by the bevel radiuis
	localBox.Extrude( Max< Float >( bevelRadius, bevelRadiusVertical ) );

	// Calculate actual world space bounding box
	IntegerBox worldBox;
	worldBox.Min = m_referencePosition + IntegerVector4( localBox.Min );
	worldBox.Max = m_referencePosition + IntegerVector4( localBox.Max );

	// Unlink from the tree
	if ( NULL != m_token )
	{
		tree.Remove( m_token );
		m_token = NULL;
	}

	// Insert with new bounds
	m_token = tree.Insert( worldBox, this );
}

Bool CTriggerConvex::TestOverlap( const IntegerBox& worldBox ) const
{
	// Convert to local coordinates (so we can use floats)
	Box testBox;
	testBox.Min = ( worldBox.Min - m_referencePosition ).ToVector();
	testBox.Max = ( worldBox.Max - m_referencePosition ).ToVector();

	// Get the center point and shift values for planes (extents)
	Vector center = testBox.CalcCenter();
	center.SetW(1.0f);
	Vector extents = testBox.CalcExtents();
	extents.SetW(0.0f);

	// Test the plane overlap
	for ( Uint32 i=0; i<m_numReferencePlanes; ++i )
	{
		const Vector& plane = m_referencePlanes[i];

		const Vector offsetV = plane.Abs() * extents;
		const Float offset = Vector::Dot3( offsetV, Vector::ONES );

		const Vector movedPlane( plane.X, plane.Y, plane.Z, plane.W - offset );

		// test against beveled plane
		if ( Vector::Dot4( movedPlane, center ) > 0.0f )
		{
			return false;
		}
	}

	// Inside
	return true;
}

Bool CTriggerConvex::TestOverlap( const Vector& worldPos ) const
{
	// Get the center point and shift values for planes (extents)
	Vector testPos = worldPos - m_referencePosition.ToVector();
	testPos.SetW(1.0f);

	// Test the plane overlap
	for ( Uint32 i=0; i<m_numReferencePlanes; ++i )
	{
		const Vector& plane = m_referencePlanes[i];

		// test against beveled plane
		if ( Vector::Dot4( plane, testPos ) > 0.0f )
		{
			return false;
		}
	}

	// Inside
	return true;
}

Bool CTriggerConvex::IntersectSegment( const IntegerVector4& start, const IntegerVector4& end, const Vector& extents, Float& outEntryTime, Float& outExitTime ) const
{
	// convert to local space
	Vector baseStart = ( start - m_referencePosition ).ToVector();
	baseStart.SetW( 1.0f );
	Vector baseEnd = ( end - m_referencePosition ).ToVector();
	baseEnd.SetW( 1.0f );

	// current segment
	Vector startPos = baseStart;
	Vector endPos = baseEnd;	
	Float startTime = 0.0f;
	Float endTime = 1.0f;

	// test all planes
	for ( Uint32 i=0; i<m_numReferencePlanes; ++i )
	{
		// extend the plane
		const Vector& plane = m_referencePlanes[i];
		const Vector offsetV = plane.Abs() * extents;
		const Float offset = Vector::Dot3( offsetV, Vector::ONES );
		const Vector movedPlane( plane.X, plane.Y, plane.Z, plane.W - offset );

		// calculate distance to plane
		const Float startDist = Vector::Dot4( movedPlane, startPos );
		const Float endDist = Vector::Dot4( movedPlane, endPos );

		// sign
		const Bool startNeg = ( startDist < 0.0f );
		const Bool endNeg = ( endDist < 0.0f );

		// no intersection at all
		if ( !startNeg && !endNeg )
		{
			return false;
		}

		// totally inside, continue
		if ( startNeg && endNeg )
		{
			continue;
		}

		// calculate intersection ratio
		const Float frac = startDist / ( startDist - endDist );
		ASSERT( frac >= 0.0f && frac <= 1.0f );

		// calculate intersection time
		const Float time = Lerp( frac, startTime, endTime );
		const Vector pos = Lerp( time, baseStart, baseEnd );

		// clip the segment
		if ( startDist > 0.0f )
		{
			// clip the front segment
			startTime = time;
			startPos = pos;			
		}
		else 
		{
			// clip from the back
			endTime = time;
			endPos = pos;
		}
	}

	// the [startPos-endPos] segment is inside, update the segment clip info
	outEntryTime = Min( outEntryTime, startTime );
	outExitTime = Max( outExitTime, endTime );

	// we had intersection
	return true;
}

void CTriggerConvex::RenderOutline( CRenderFrame* frame, const Color& color, Bool overlay ) const
{
	// drawing matrix
	Matrix drawMatrix = m_object->GetLocalToReference();
	drawMatrix.SetTranslation( m_referencePosition.ToVector() );

	// use the source convex data
	const CAreaConvex* srcConvex = m_shape->GetConvex( m_convexIndex );
	srcConvex->RenderOutline( frame, drawMatrix, color, overlay );
}

//---------------------------------------------------------------------------

class CTriggerNullCallback : public ITriggerCallback
{
public:
	virtual void OnActivatorEntered( const class ITriggerObject* object, const class ITriggerActivator* activator ) {};
	virtual void OnActivatorExited( const class ITriggerObject* object, const class ITriggerActivator* activator ) {};

public:
	static CTriggerNullCallback& GetInstance()
	{
		static CTriggerNullCallback theInstance;
		return theInstance;
	}
};

//---------------------------------------------------------------------------

CTriggerObject::CTriggerObject( class CTriggerManager* manager, const CTriggerObjectInfo& info, const Uint32 id )
	: m_manager( manager )
	, m_shape( info.m_shape )
	, m_callback( info.m_callback )
	, m_position( info.m_localToWorld.GetTranslationRef() )
	, m_localToWorld( info.m_localToWorld )
#ifndef RED_TRIGGER_SYSTEM_NO_DEBUG_NAMES
	, m_debugName( info.m_debugName )
#endif
	, m_includedChannels( info.m_includeChannels )
	, m_excludedChannels( info.m_excludeChannels ) 
	, m_component( info.m_component )
	, m_bevelRadius( info.m_bevelRadius )
	, m_bevelRadiusVertical( info.m_bevelRadiusVertical )
	, m_objectID( id )
	, m_flagHasMoved( 0 )
	, m_flagUseCCD( info.m_useCCD )
	, m_flagTerrainMask( 0 )
	, m_refCount( 1 )
{
	// keep the extra reference to the shape data
	m_shape->AddRef();

	// update reference matrix (rotation and scaling of the trigger space)
	m_localToReference = info.m_localToWorld;
	m_localToReference.SetTranslation(0,0,0);

	// calculate the inverse transposed matrix ( for plane normals )
	m_localToReferenceInvTrans = m_localToReference;
	m_localToReferenceInvTrans.FullInvert();
	m_localToReferenceInvTrans.Transpose();

	// default to special null callback when the callback is not specified
	if ( NULL == m_callback )
	{
		m_callback = &CTriggerNullCallback::GetInstance();
	}

	// extract trigger shapes
	if ( NULL != info.m_shape )
	{
		const Uint32 numShapes = info.m_shape->GetNumShapes();
		m_convexList.Resize( numShapes );
		for ( Uint32 i=0; i<numShapes; ++i )
		{
			CTriggerConvex* convex = new CTriggerConvex( this, info.m_shape, i );
			m_convexList[i] = convex;
		}
	}

	// compute terrain mask
	if ( info.m_allowAboveTerrain ) m_flagTerrainMask |= eTriggerTerrainFlags_Above;
	if ( info.m_allowBelowTerrain ) m_flagTerrainMask |= eTriggerTerrainFlags_Below;
}

CTriggerObject::~CTriggerObject()
{
	ASSERT( m_manager == NULL );
	ASSERT( m_refCount.GetValue() == 0 );

	// release the shape reference
	m_shape->Release();

	// release convex shapes
	m_convexList.ClearPtr();
}

#ifndef RED_TRIGGER_SYSTEM_NO_DEBUG_NAMES
const String& CTriggerObject::GetDebugName() const
{
	return m_debugName;
}
#endif

CAreaComponent* CTriggerObject::GetComponent() const
{
	return m_component;
}

const Bool CTriggerObject::IsCCDEnabled() const
{
	return m_flagUseCCD;
}

void CTriggerObject::EnableCCD(const Bool isCCDEnabled)
{
	m_flagUseCCD = isCCDEnabled;
}

Bool CTriggerObject::TestPoint( const Vector& worldPoint ) const
{
	// test all shapes
	const Uint32 numShapes = m_convexList.Size();
	for ( Uint32 i=0; i<numShapes; ++i )
	{
		if ( m_convexList[i]->TestOverlap( worldPoint ) )
		{
			return true;
		}
	}

	// not overlapping
	return false;
}

Bool CTriggerObject::TestBox( const Vector& center, const Vector& extents ) const
{
	IntegerBox worldBox;
	worldBox.Min = IntegerVector4( center - extents );
	worldBox.Max = IntegerVector4( center + extents );

	// test all shapes
	const Uint32 numShapes = m_convexList.Size();
	for ( Uint32 i=0; i<numShapes; ++i )
	{
		if ( m_convexList[i]->TestOverlap( worldBox ) )
		{
			return true;
		}
	}

	// outside
	return false;
}

Bool CTriggerObject::IntersectSegment( const IntegerVector4& start, const IntegerVector4& end, const Vector& extents, Float& outEntryTime, Float& outExitTime ) const
{
	const IntegerVector4 worldExtents( extents );

	// calculate global box
	IntegerBox worldBox;
	worldBox.Min = start - worldExtents;
	worldBox.Max = start + worldExtents;
	worldBox.Min.SetMin( end - worldExtents );
	worldBox.Max.SetMax( end + worldExtents );

	// overlap segment
	Float startTime = FLT_MAX;
	Float endTime = -FLT_MAX;
	Bool hadIntersection = false;

	// test all shapes
	const Uint32 numShapes = m_convexList.Size();
	for ( Uint32 i=0; i<numShapes; ++i )
	{
		if ( m_convexList[i]->m_token->m_worldBounds.Touches( worldBox ) )
		{
			if ( m_convexList[i]->IntersectSegment( start, end, extents, startTime, endTime ) )
			{
				hadIntersection = true;
			}
		}
	}

	// We had reported intersection
	if ( hadIntersection )
	{
		// update the in time
		outEntryTime = startTime;
		outExitTime = endTime;
		return true;
	}

	// No intersection reported
	return false;
}

Bool CTriggerObject::TraceBox( const Vector& start, const Vector& end, const Vector& extents, Float& inTime, Float& outTime ) const
{
	// world space start
	const IntegerVector4 worldStart( start );
	const IntegerVector4 worldEnd( end );
	const IntegerVector4 worldExtents( extents );

	// calculate global box
	IntegerBox worldBox;
	worldBox.Min = worldStart - worldExtents;
	worldBox.Max = worldStart + worldExtents;
	worldBox.Min.SetMin( worldEnd - worldExtents );
	worldBox.Max.SetMax( worldEnd + worldExtents );

	// overlap segment
	Float startTime = FLT_MAX;
	Float endTime = -FLT_MAX;
	Bool hadIntersection = false;

	// test all shapes
	const Uint32 numShapes = m_convexList.Size();
	for ( Uint32 i=0; i<numShapes; ++i )
	{
		if ( m_convexList[i]->m_token->m_worldBounds.Touches( worldBox ) )
		{
			if ( m_convexList[i]->IntersectSegment( worldStart, worldEnd, extents, startTime, endTime ) )
			{
				hadIntersection = true;
			}
		}
	}

	// We had reported intersection
	if ( hadIntersection )
	{
		// update the in time
		if ( startTime < inTime )
		{
			inTime = startTime;
		}
		if ( endTime > outTime )
		{
			outTime = endTime;
		}

		return true;
	}

	// No intersection reported
	return false;
}

void CTriggerObject::InvalidateNearbyActivators()
{
	if ( NULL != m_manager && !m_convexList.Empty() )
	{
		// calculate merged box of all convex pieces
		IntegerBox mergedBox;
		for ( Uint32 i=0; i<m_convexList.Size(); ++i )
		{
			CTriggerConvex::TTreeToken* token = m_convexList[i]->m_token;
			if ( NULL != token )
			{
				mergedBox.AddBox( token->m_worldBounds );
			}
		}

		// invalidate activators in the area
		if ( !mergedBox.IsEmpty() )
		{
			m_manager->InvalidateActivatorsFromArea( mergedBox );
		}
	}
}

void CTriggerObject::SetMask( const Uint32 inclusionMask, const Uint32 exclusionMask )
{
	if ( (inclusionMask != m_includedChannels) || (m_excludedChannels != exclusionMask) )
	{
		// update masks
		m_includedChannels = inclusionMask;
		m_excludedChannels = exclusionMask;

		// force nearby activators to be reevaluated
		InvalidateNearbyActivators();
	}
}

void CTriggerObject::SetTerrainMask(const Bool allowAboveTerrain, const Bool allowBelowTerrain)
{
	Uint8 mask = 0;
	if ( allowAboveTerrain ) mask |= eTriggerTerrainFlags_Above;
	if ( allowBelowTerrain ) mask |= eTriggerTerrainFlags_Below;

	if ( mask != m_flagTerrainMask )
	{
		// update masks
		m_flagTerrainMask = mask;

		// force nearby activators to be reevaluated
		InvalidateNearbyActivators();
	}
}

void CTriggerObject::SetPosition( const Matrix& matrix )
{
	// update only if the matrix is different (filters unneded updates)
	if ( 0 != Red::System::MemoryCompare( &m_localToWorld, &matrix, sizeof(Matrix) ) )
	{
		// store new position
		m_localToWorld = matrix;
		m_position = IntegerVector4( matrix.GetTranslationRef() );

		// set new reference matrix
		m_localToReference = matrix;
		m_localToReference.SetTranslation(0,0,0);

		// calculate the inverse transposed matrix ( for plane normals )
		m_localToReferenceInvTrans = m_localToReference;
		m_localToReferenceInvTrans.FullInvert();
		m_localToReferenceInvTrans.Transpose();

		// report to the trigger system that this trigger has invalid position data and needs an update
		if ( NULL != m_manager )
		{
			m_manager->RegisterObjectMovement(this);
			ASSERT( m_flagHasMoved == 1 );
		}
	}
}

void CTriggerObject::Render( class CRenderFrame* frame )
{
	// Draw the world space bounding boxes of the registered trigger shapes
	if ( frame->GetFrameInfo().IsShowFlagOn( SHOW_TriggerBounds ) )
	{
		for (TConvexList::const_iterator it = m_convexList.Begin();
			 it != m_convexList.End(); ++it)
		{			
			CTriggerConvex* convex = (*it);
			if ( NULL != convex->m_token )
			{
				Box drawBox;
				drawBox.Min = convex->m_token->m_worldBounds.Min.ToVector();
				drawBox.Max = convex->m_token->m_worldBounds.Max.ToVector();

				frame->AddDebugBox( drawBox, Matrix::IDENTITY, Color::YELLOW, false );
			}
		}
	}
}

void CTriggerObject::Remove()
{
	// this function can be called for dead object - do nothing then
	if ( NULL != m_manager )
	{
		// this function can be called from a thread (why, oh why...), we need to make sure it will not crash
		{
			m_manager->LockUpdates();

			// remove the shapes from the tree
			for ( Uint32 i=0; i<m_convexList.Size(); ++i )
			{
				CTriggerConvex* convex = m_convexList[i];
				convex->RemovePlacement( m_manager->GetConvexTree() );
			}		

			m_manager->UnlockUpdates();
		}

		// remove from the manager
		m_manager->RemoveObject( this );
	}
	else
	{
#ifndef RED_TRIGGER_SYSTEM_NO_DEBUG_NAMES
		// this is not a normal condition!
		WARN_ENGINE( TXT("Trigger object '%ls' removed after manager was destroyed"), m_debugName.AsChar() );
#else
		// this is not a normal condition!
		WARN_ENGINE( TXT("Trigger object removed after manager was destroyed") );
#endif
	}
}

void CTriggerObject::UpdatePlacement( TTriggerCovnexTree& tree )
{
	ASSERT( m_manager != NULL );

	// Recalculate planes and bounding boxes for convex sub shapes
	// When updating make sure to re register the shapes in the tree
	for ( Uint32 i=0; i<m_convexList.Size(); ++i )
	{
		CTriggerConvex* convex = m_convexList[i];
		convex->UpdatePlacement( tree );
	}

	// reset dirty flag
	m_flagHasMoved = 0;
}

Bool CTriggerObject::CanInteractWith( const Uint32 channel ) const
{
	if ( 0 != (m_excludedChannels & channel) )
	{
		return false;
	}

	if ( 0 == (m_includedChannels & channel) )
	{
		return false;
	}

	return true;
}

Bool CTriggerObject::CanInteractWith( const class CTriggerActivator& activator ) const
{
	return CanInteractWith( activator.GetChannels() );
}

void CTriggerObject::OnEnter( const CTriggerActivator* activator ) const
{
	// callback is never NULL
	m_callback->OnActivatorEntered( this, activator );
}

void CTriggerObject::OnExit( const CTriggerActivator* activator ) const
{
	// callback is never NULL
	m_callback->OnActivatorExited( this, activator );
}

void CTriggerObject::AddRef()
{
	m_refCount.Increment();
}

void CTriggerObject::Release()
{
	if ( 0 == m_refCount.Decrement() )
	{
		delete this;
	}
}

void CTriggerObject::UnlinkManager()
{
	ASSERT( m_manager != NULL );
	m_manager = NULL;
}

const Uint32 CTriggerObject::CalcMemoryUsage() const
{
	Uint32 ret = sizeof(CTriggerObject);
	for ( Uint32 i=0; i<m_convexList.Size(); ++i )
	{
		ret += m_convexList[i]->CalcMemoryUsage();
	}
	return ret;
}

//---------------------------------------------------------------------------

CTriggerActivator::CTriggerActivator( class CTriggerManager* manager, const CTriggerActivatorInfo& initInfo, const Uint32 uniqueID )
	: m_manager( manager )
	, m_position( initInfo.m_localToWorld.GetTranslationRef() )
	, m_extents( initInfo.m_extents )
#ifndef RED_TRIGGER_SYSTEM_NO_DEBUG_NAMES
	, m_debugName( initInfo.m_debugName )
#endif
	, m_channelMask( initInfo.m_channels )
	, m_component( initInfo.m_component )
	, m_maxCCDDistance( initInfo.m_maxContinousDistance )
	, m_flagUseCCD( 0 )
	, m_flagHasMoved( 0 )
	, m_token( NULL )
	, m_uniqueID( uniqueID )
	, m_refCount( 1 )
{
	// Setup initial movement
	m_newPosition = m_position;
	m_flagTeleported = 1;
}

CTriggerActivator::~CTriggerActivator()
{
	ASSERT( m_refCount.GetValue() == 0 );
	ASSERT( m_token == NULL );
}

void CTriggerActivator::RemovePlacement( TTriggerActivatorTree& tree )
{
	if ( NULL != m_token )
	{
		tree.Remove( m_token );
		m_token = NULL;
	}
}

void CTriggerActivator::UpdatePlacement( TTriggerActivatorTree& tree )
{
	// remove current placement
	if ( NULL != m_token )
	{
		tree.Remove( m_token );
		m_token = NULL;
	}

	// calculate new placement
	IntegerBox placementBox;
	placementBox.Min = m_position - m_extents;
	placementBox.Max = m_position + m_extents;
	m_token = tree.Insert( placementBox, this );
}

void CTriggerActivator::InvalidatePlacement()
{
	if ( NULL != m_manager )
	{
		m_manager->RegisterActivatorMovement( this );
		ASSERT( m_flagHasMoved == 1 );
	}
}

void CTriggerActivator::UnregisterTriggerObject( const CTriggerObject* object )
{
	const Uint32 objectIndex = object->GetID();
	m_interactions.Remove( objectIndex );
}

#ifndef RED_TRIGGER_SYSTEM_NO_DEBUG_NAMES
const String& CTriggerActivator::GetDebugName() const
{
	return m_debugName;
}
#endif

class CComponent* CTriggerActivator::GetComponent() const
{
	return m_component;
}

const Uint32 CTriggerActivator::GetMask() const
{
	return m_channelMask;
}

const Bool CTriggerActivator::IsCCDEnabled() const
{
	return m_flagUseCCD;
}

void CTriggerActivator::EnableCCD(const Bool isCCDEnabled)
{
	m_flagUseCCD = isCCDEnabled;
}

void CTriggerActivator::SetExtents( const Vector& extents )
{
	m_extents = IntegerVector4( extents );
	InvalidatePlacement();
}

void CTriggerActivator::SetMask( const Uint32 mask )
{
	m_channelMask = mask;
	InvalidatePlacement();
}

void CTriggerActivator::Move( const IntegerVector4& position, const Bool teleportation/*= false*/ )
{
	if ( NULL == m_manager )
	{
#ifndef RED_TRIGGER_SYSTEM_NO_DEBUG_NAMES
		ERR_ENGINE( TXT("Moved trigger activator '%ls' that has no manager. Please debug."), m_debugName.AsChar() );
#else
		ERR_ENGINE( TXT("Moved trigger activator that has no manager. Please debug.") );
#endif
		return;
	}			
		
	if ( ( m_position - position ).SqrLength() > TRIGGER_ACTIVATOR_EPSILON_SQR  )
	{
		// save the target position for this frame
		m_newPosition = position;

		// if any of the movements in this frame was a teleportation assume the whole movement was
		if ( teleportation )
		{
			m_flagTeleported = 1;
		}

		// request trigger manager to update this activator
		InvalidatePlacement();
		ASSERT( m_flagHasMoved == 1 );
	}
}

void CTriggerActivator::Render( class CRenderFrame* frame )
{
	// Render as a bounding box
	if ( frame->GetFrameInfo().IsShowFlagOn( SHOW_TriggerActivators ) )
	{
		// Draw the box only if activator size is not zero
		const Vector size = m_extents.ToVector();
		if ( size.SquareMag3() > 0.0001f )
		{
			// AABB bounding box
			Box drawBox;
			drawBox.Min = (m_position - m_extents).ToVector();
			drawBox.Max = (m_position + m_extents).ToVector();
			frame->AddDebugBox( drawBox, Matrix::IDENTITY, Color::YELLOW, false );

			// Cylinder
			const Vector top = ( m_position + IntegerVector4( IntegerUnit(0), IntegerUnit(0), m_extents.Z ) ).ToVector();
			const Vector bottom = ( m_position - IntegerVector4( IntegerUnit(0), IntegerUnit(0), m_extents.Z ) ).ToVector();
			const Float radius = m_extents.X.ToFloat();
			frame->AddDebugWireframeTube( top, bottom, radius, radius, Matrix::IDENTITY, Color::YELLOW, Color::YELLOW, false );
		}
		else
		{
			// For zero sized activators render the marker
			const Vector pos = m_position.ToVector();
			frame->AddDebugLine( pos - Vector(0,0,0.1f), pos + Vector(0,0,0.1f), Color::YELLOW, false );
			frame->AddDebugLine( pos - Vector(0,0.1f,0), pos + Vector(0,0.1f,0), Color::YELLOW, false );
			frame->AddDebugLine( pos - Vector(0.1f,0,0), pos + Vector(0.1f,0,0), Color::YELLOW, false );
		}
	}
}

void CTriggerActivator::Remove()
{
	// this function can be called for dead object
	if ( NULL != m_manager )
	{
		// remove from internal manager structures	
		m_manager->RemoveActivator(this);
	}
	else
	{
#ifndef RED_TRIGGER_SYSTEM_NO_DEBUG_NAMES
		// this is not a normal condition!
		WARN_ENGINE( TXT("Trigger activator '%ls' removed after manager was destroyed"), m_debugName.AsChar() );
#else
		// this is not a normal condition!
		WARN_ENGINE( TXT("Trigger activator removed after manager was destroyed") );
#endif
	}
}

void CTriggerActivator::AddRef()
{
	m_refCount.Increment();
}

void CTriggerActivator::Release()
{
	if ( 0 == m_refCount.Decrement() )
	{
		delete this;
	}
}

void CTriggerActivator::UnlinkManager()
{
	ASSERT( m_manager != NULL );
	m_manager = NULL;
	m_token = NULL;
}

const ITriggerObject* CTriggerActivator::GetOccupiedTrigger(const Uint32 index) const
{
	if( m_interactions.Size() <= index ) return 0;
	Uint32 objectIndex = m_interactions[ index ];
	if( m_manager->GetNumObjects() <= objectIndex ) return 0;
	return m_manager->GetObject( objectIndex );
}

const Uint32 CTriggerActivator::GetNumOccupiedTrigger() const
{
	return m_interactions.Size();
}

const Bool CTriggerActivator::IsInsideTrigger( const ITriggerObject* trigger ) const
{
	if ( trigger == nullptr )
	{
		return false;
	}
	const Uint32 interactionsCount = m_interactions.Size();
	const Uint32 objectsCount = m_manager->GetNumObjects();
	for ( Uint32 i = 0; i < interactionsCount; i++ )
	{
		const Uint32 objectIndex = m_interactions[ i ];
		if ( objectIndex < objectsCount && m_manager->GetObject( objectIndex ) == trigger )
		{
			return true;
		}
	}
	return false;
}

//---------------------------------------------------------------------------

CTriggerManager::CTriggerManager()
	: m_maskBuffer( 1024 )
	, m_debugListener( NULL )
	, m_terrainQuery( NULL )
	, m_isUpdating( false )
	, m_nextFreeActivatorID( 1 )
{
	m_globalEventsReporter = new TTriggerGlobalEventsReporter( GEC_Trigger );
}

CTriggerManager::~CTriggerManager()
{
	for ( Uint32 i=0; i<m_objects.Size(); ++i )
	{
		CTriggerObject* object = m_objects[i];
		if ( NULL != object )
		{
			object->UnlinkManager();
			object->Release();
		}
	}

	for ( Uint32 i=0; i<m_activators.Size(); ++i )
	{
		CTriggerActivator* activator = m_activators[i];
		if ( NULL != activator )
		{
			activator->UnlinkManager();
			activator->Release();
		}
	}
	m_objectsToRemove.ClearFast();
	m_activatorsToRemove.ClearFast();
		
	delete m_globalEventsReporter;
}

void CTriggerManager::SetDebugEventListener( ITriggerManagerDebugEventListener* debugListener )
{
	m_debugListener = debugListener;
}

void CTriggerManager::SetTerrainIntegration( ITriggerSystemTerrainQuery* queryInterface )
{
	m_terrainQuery = queryInterface;
}

CTriggerObject* CTriggerManager::CreateTrigger( const CTriggerObjectInfo& initInfo )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_lock );

	// no shape specified
	if ( NULL == initInfo.m_shape )
	{
		return NULL;
	}

	// Allocate object ID
	Uint32 objectID;
	if ( !m_freeObjectIDs.Empty() )
	{
		objectID = m_freeObjectIDs.Back();
		m_freeObjectIDs.PopBack();
	}
	else
	{
		objectID = m_objects.Size();
		m_objects.PushBack( NULL );
	}

	// Update the temporary mask buffer (so it will be able to contain all the required data)
	if ( objectID >= m_maskBuffer.GetCapacity() )
	{
		m_maskBuffer.Resize( m_maskBuffer.GetCapacity() * 2 );
	}

	// create wrapper object and add to local list
	CTriggerObject* object = new CTriggerObject( this, initInfo, objectID );
	m_objects[ objectID ] = object;

	// keep internal reference
	object->AddRef();

	// update bounding boxes and insert the convex pieces into the tree
	object->UpdatePlacement(m_convexTree);

	// invalidate all activators in the vicinity of trigger shape
	object->InvalidateNearbyActivators();

	// report
	if ( NULL != m_debugListener )
	{
		m_debugListener->OnTriggerCreated( object );
	}

	m_globalEventsReporter->AddEvent( GET_TriggerCreated, object->GetComponent() );

	return object;
}

CTriggerActivator* CTriggerManager::CreateActivator( const CTriggerActivatorInfo& initInfo )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_lock );

	// allocate activator ID
	const Uint32 id = m_nextFreeActivatorID++;

	// create wrapper object and add to local list
	CTriggerActivator* activator = new CTriggerActivator( this, initInfo, id );
	m_activators.PushBack( activator );

	// keep internal reference
	activator->AddRef();

	// add to tree
	activator->UpdatePlacement( m_activatorTree );
	m_activatorTree.Update();

	// report
	if ( NULL != m_debugListener )
	{
		m_debugListener->OnActivatorCreated( activator );
	}

	m_globalEventsReporter->AddEvent( GET_TriggerActivatorCreated, activator->GetComponent() );

	// make sure that the new activator will activate triggers
	RegisterActivatorMovement( activator );

	return activator;
}

void CTriggerManager::Update()
{
	PC_SCOPE_PIX(TriggerManagerUpdate)
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_lock );

	m_isUpdating = true;

	// Process all movements of the trigger shapes
	// This will also update and dirty state of the global spatial tree
	ProcessObjectsMovement();

	// Process movement of the trigger activators
	ProcessActivatorsMovement();

	m_isUpdating = false;

	// Remove objects
	{
		PC_SCOPE( RemoveObjects );
		TObjects::iterator itEnd = m_objectsToRemove.End();
		for ( TObjects::iterator it = m_objectsToRemove.Begin(); it != itEnd; ++it )
		{
			RemoveObjectInternal( *it );
		}
		m_objectsToRemove.ClearFast();
	}

	// Remove activators
	{
		PC_SCOPE( RemoveActivators );
		TActivators::iterator itEnd = m_activatorsToRemove.End();
		for ( TActivators::iterator it = m_activatorsToRemove.Begin(); it != itEnd; ++it )
		{
			RemoveActivatorInternal( *it );
		}
		m_activatorsToRemove.ClearFast();
	}

	// Report global events
	m_globalEventsReporter->ReportEvents();
}

void CTriggerManager::Render( class CRenderFrame* frame )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_lock );

	// Render triggers
	for ( TObjects::const_iterator it = m_objects.Begin();
		it != m_objects.End(); ++it )
	{
		CTriggerObject* object = (*it);
		if ( NULL != object )
		{
			object->Render( frame );
		}
	}

	// Render all activators
	for ( TActivators::const_iterator it = m_activators.Begin();
		it != m_activators.End(); ++it )
	{
		(*it)->Render( frame );
	}

	// Draw the spatial tree structure
	if ( frame->GetFrameInfo().IsShowFlagOn( SHOW_TriggerTree ) )
	{
		m_convexTree.Render( frame );
	}
}

void CTriggerManager::LockUpdates()
{
	m_lock.Acquire();
}

void CTriggerManager::UnlockUpdates()
{
	m_lock.Release();	
}

void CTriggerManager::RemoveObject( CTriggerObject* obj )
{
	if ( m_isUpdating )
	{
		m_objectsToRemove.PushBackUnique( obj );
	}
	else
	{
		RemoveObjectInternal( obj );
	}
}

void CTriggerManager::RemoveObjectInternal( CTriggerObject* obj )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_lock );

	// remove from the movement list
	if ( obj->m_flagHasMoved )
	{
		obj->m_flagHasMoved = 0;
		m_movedObjects.Remove( obj );
	}	

	// remove from the object list
	const Uint32 objectID = obj->GetID();
	if ( m_objects[objectID] == obj )
	{
		m_objects[objectID] = NULL;

		// remove the trigger from all activators active lists
		// TODO: if this will be slow we need to createa some kind of linked list
		{
			PC_SCOPE( UnregisterTriggerObjects );

			for ( Uint32 i=0; i<m_activators.Size(); ++i )
			{
				CTriggerActivator* activator = m_activators[i];
				activator->UnregisterTriggerObject( obj );
			}
		}

		// add the ID back to the free list
		m_freeObjectIDs.PushBack( objectID );

		// report
		if ( NULL != m_debugListener )
		{
			m_debugListener->OnTriggerRemoved( obj );
		}

		m_globalEventsReporter->AddEvent( GET_TriggerRemoved, obj->GetComponent(), true );
		
		// remove the object
		obj->UnlinkManager();
		obj->Release();
	}
}

void CTriggerManager::RemoveActivator( CTriggerActivator* activator )
{
	if ( m_isUpdating )
	{
		m_activatorsToRemove.PushBackUnique( activator );
	}
	else
	{
		RemoveActivatorInternal( activator );
	}
}

void CTriggerManager::RemoveActivatorInternal( CTriggerActivator* activator )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_lock );

	// remove from movement update list
	if ( activator->m_flagHasMoved )
	{
		activator->m_flagHasMoved = 0;
		m_movedActivators.Remove( activator );
	}

	// remove from tree
	activator->RemovePlacement( m_activatorTree );

	// report
	if ( NULL != m_debugListener )
	{
		m_debugListener->OnActivatorRemoved( activator );
	}

	m_globalEventsReporter->AddEvent( GET_TriggerActivatorRemoved, activator->GetComponent(), true );

	// remove from the activator list
	if ( m_activators.Remove( activator ) )
	{
		activator->UnlinkManager();
		activator->Release();
	}
}

void CTriggerManager::RegisterObjectMovement( CTriggerObject* obj )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_lock );

	if ( 0 == obj->m_flagHasMoved )
	{
		obj->m_flagHasMoved = 1;
		m_movedObjects.PushBack( obj );
	}
}

void CTriggerManager::RegisterActivatorMovement( CTriggerActivator* obj )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_lock );

	if ( 0 == obj->m_flagHasMoved )
	{
		obj->m_flagHasMoved = 1;
		m_movedActivators.PushBack( obj );
	}
}

void CTriggerManager::ProcessObjectsMovement()
{
	PC_SCOPE(ProcessObjectsMovement);

	// Move the objects (and thus the convex pieces)
	TObjects movedObjects;
	Swap( movedObjects, m_movedObjects );
	for ( Uint32 i=0; i<movedObjects.Size(); ++i )
	{
		CTriggerObject* object = movedObjects[i];
		ASSERT( object != NULL );

		if ( NULL != object )
		{
			// invalidate all activators in the vicinity of trigger shape
			object->InvalidateNearbyActivators();

			// recalculate planes and bounding boxes
			object->UpdatePlacement( m_convexTree );

			// after the object was moved invalidate all trigger shapes in the new place
			object->InvalidateNearbyActivators();
		}
	}

	// Recalculate the bounding boxes in the spatial tree
	{
		PC_SCOPE( UpdateTreeBounds );
		m_convexTree.Update();
	}
}

/// Generic event solver for non CCD movement
struct CGenericInteractionResolver
{
public:
	CTriggerMaskBuffer* m_mask;
	ITriggerSystemTerrainQuery* m_terrainQuery;
	Bool m_terrainFlagCalculated;
	Uint8 m_terrainFlags;

public:
	CGenericInteractionResolver( CTriggerMaskBuffer& mask, ITriggerSystemTerrainQuery* terrainQuery )
		: m_mask( &mask )
		, m_terrainFlagCalculated( false )
		, m_terrainFlags( eTriggerTerrainFlags_Both ) 
		, m_terrainQuery( terrainQuery )
	{
		// if we don't have terrain integration then assume we always interact
		if ( NULL == m_terrainQuery )
		{
			m_terrainFlagCalculated = true;
		}
	}

	bool FilterTerrainFlags( const CTriggerObject* object, const Vector& referencePos )
	{
		if ( object->UsesTerrainFiltering() )
		{
			// recalculate terrain flags of activator
			if ( !m_terrainFlagCalculated )
			{
				const Bool isAbove = m_terrainQuery->IsPositionAboveTerrain( referencePos );
				m_terrainFlags = (Uint8)( isAbove ? eTriggerTerrainFlags_Above : eTriggerTerrainFlags_Below );
				m_terrainFlagCalculated = true;
			}

			// filter
			if ( (object->GetTerrainMask() & m_terrainFlags) == 0 )
			{
				// we cannot interact with this trigger
				return false;
			}
		}

		// allow
		return true;
	}

	void GenerateEvents( CTriggerEventBuffer& outEvents, CTriggerObject** objects, CTriggerActivator* activator )
	{
		const Uint32 numObjects = m_mask->GetNumTestedObjects();
		for ( Uint32 i=0; i<numObjects; ++i )
		{
			const Uint32 objectIndex = m_mask->GetTestedObject(i);

			const Bool wasInside = m_mask->IsInside(objectIndex);
			const Bool entered = m_mask->WasEntered(objectIndex);
			const Bool exited = m_mask->WasExited(objectIndex);

			Bool reportEnter = false;
			Bool reportExit = false;
			Bool isInside = wasInside;

			if ( !wasInside )
			{
				if ( entered && exited )
				{
					// entered and exited on the same frame
					reportEnter = true;
					reportExit = true;
					isInside = false; // still outside
				}
				else if ( entered )
				{
					reportEnter = true;
					reportExit = false;
					isInside = true;
				}
			}
			else
			{
				// did we exit ?
				if ( exited )
				{
					reportExit = true;
					isInside = false;
				}
			}

			if ( reportExit )
			{
				ASSERT(objects[objectIndex] != NULL);
				ASSERT(objects[objectIndex]->GetID() == objectIndex);
				outEvents.ReportExitEvent( objects[objectIndex], activator );
			}

			if ( reportEnter )
			{
				ASSERT(objects[objectIndex] != NULL);
				ASSERT(objects[objectIndex]->GetID() == objectIndex);
				outEvents.ReportEntryEvent( objects[objectIndex], activator );
			}

			// set the inside flag for the new inside list
			if ( isInside )
			{
				m_mask->SetInsideFlag( objectIndex );
			}
			else
			{
				m_mask->ClearInsideFlag( objectIndex );
			}
		}
	}

	void CollectNewInsideList( TDynArray<Uint32>& outInsideList )
	{
		outInsideList.ClearFast();

		const Uint32 numObjects = m_mask->GetNumTestedObjects();
		for ( Uint32 i=0; i<numObjects; ++i )
		{
			const Uint32 objectIndex = m_mask->GetTestedObject(i);
			if ( m_mask->IsInside( objectIndex ))
			{
				outInsideList.PushBack( objectIndex );
			}
		}
	}
};

/// Event solver for non CCD movement
struct CNonCCDInteractionResolver : public CGenericInteractionResolver
{
public:
	Uint32 m_channels;
	IntegerBox m_box;
	Vector m_refPosition;

	CNonCCDInteractionResolver( CTriggerMaskBuffer& mask, const CTriggerActivator* activator, ITriggerSystemTerrainQuery* terrainQuery )
		: CGenericInteractionResolver( mask, terrainQuery )
	{
		// cache activator channels
		m_channels = activator->GetChannels();

		// activator reference position (for terrain tests)
		m_refPosition = activator->GetNewPosition().ToVector();

		// final position box
		m_box.Min = activator->GetNewPosition() - activator->GetExtents();
		m_box.Max = activator->GetNewPosition() + activator->GetExtents();
	}

	void Collect( CTriggerConvex* convex )
	{
		const CTriggerObject* object = convex->m_object;
		const Uint32 objectID = object->GetID();

		// This shape is on the ignore list
		if ( m_mask->IsIgnored( objectID ) )
		{
			return;
		}

		// Can we interact with this shape ?
		if ( !object->CanInteractWith( m_channels ) )
		{
			// never test it again
			m_mask->SetIgnoredFlag( objectID );
			return;
		}

		// Sepcial terrain filtering
		if ( !FilterTerrainFlags( object, m_refPosition ) )
		{
			// never test it again
			m_mask->SetIgnoredFlag( objectID );
			return;
		}

		// does the new activator position intersect with convex shape ?
		if ( convex->TestOverlap( m_box ) )
		{
			// report that we have an intersection with the object
			m_mask->ReportEntry( objectID );

			// since this is a one point test and we are considered to be inside the object
			// we need to clear the "exit" flag to indicate that we won't be exiting it this frame
			m_mask->ClearExitFlag( objectID );

			// report object as being on our list
			m_mask->ReportAsTested( objectID );

			// do not test it again
			m_mask->SetIgnoredFlag( objectID );
		}
	}
};

/// Event solver for generic CCD movement
struct CGenericCCDInteractionResolver : public CGenericInteractionResolver
{
public:
	Uint32 m_channels;
	IntegerVector4 m_start;
	IntegerVector4 m_extents;
	IntegerVector4 m_end;
	Vector m_extentsV;
	Vector m_refPosition;
	IntegerBox m_box;

	CGenericCCDInteractionResolver( CTriggerMaskBuffer& mask, const CTriggerActivator* activator, ITriggerSystemTerrainQuery* terrainQuery )
		: CGenericInteractionResolver( mask, terrainQuery )
	{
		// cache activator channels
		m_channels = activator->GetChannels();

		// box positions
		m_start = activator->GetPosition();
		m_end = activator->GetNewPosition();
		m_extents = activator->GetExtents();
		m_extentsV = m_extents.ToVector();

		// activator reference position (for terrain tests)
		m_refPosition = activator->GetNewPosition().ToVector();

		// final position box
		m_box.Min = m_end - m_extents;
		m_box.Max = m_end + m_extents;
	}

	void Collect( CTriggerConvex* convex )
	{
		const CTriggerObject* object = convex->m_object;
		const Uint32 objectID = object->GetID();

		// This shape is on the ignore list
		if ( m_mask->IsIgnored( objectID ) )
		{
			return;
		}

		// Can we interact with this shape ?
		if ( !object->CanInteractWith( m_channels ) )
		{
			// never test it again
			m_mask->SetIgnoredFlag( objectID );
			return;
		}

		// Sepcial terrain filtering
		if ( !FilterTerrainFlags( object, m_refPosition ) )
		{
			// never test it again
			m_mask->SetIgnoredFlag( objectID );
			return;
		}

		// Handle both CCD and non CCD triggers here
		const Bool useCCD = object->IsCCDEnabled();
		if ( !useCCD )
		{
			// does the new activator position intersect with convex shape ?
			if ( convex->TestOverlap( m_box ) )
			{
				// report that we have an intersection with the object
				m_mask->ReportEntry( objectID );

				// since this is a one point test and we are considered to be inside the object
				// we need to clear the "exit" flag to indicate that we won't be exiting it this frame
				m_mask->ClearExitFlag( objectID );

				// report object as being on our list
				m_mask->ReportAsTested( objectID );

				// do not test it again
				m_mask->SetIgnoredFlag( objectID );
			}
		}
		else
		{
			// In CCD mode once we see at least one convex piece of the trigger we need to test the whole shape.
			// This is unfortunate but necessary to get the propper entry/exit time intervals.

			// Test only once (ingore from now on)
			m_mask->SetIgnoredFlag( objectID );

			// Report this object as tested (so we know for which objects to generate the entry/exit) events
			m_mask->ReportAsTested( objectID );

			// Calcualte the entry/exit intervals for the whole trigger object (all covnex pieces)
			Float entryTime, exitTime;
			if ( object->IntersectSegment( m_start, m_end, m_extentsV, entryTime, exitTime ) )
			{
				// entered the trigger
				if ( entryTime > 0.0f )
				{
					m_mask->ReportEntry( objectID );
				}

				// special case: if we were not inside the trigger initially signal the fake entry event
				else if ( !m_mask->IsInside( objectID ) )
				{
					// we are now inside the trigger, report the entry event
					m_mask->ReportEntry( objectID );
				}

				// exited the trigger ( if we don't report the exit event we assume we are left inside the trigger )
				if ( exitTime < 1.0f )
				{
					// we have exited the trigger as some time during the movement interval
					m_mask->ReportExit( objectID );
				}
				else
				{
					// we are still in the object (end point was inside the trigger)
					m_mask->ClearExitFlag( objectID );
				}
			}
		}
	}
};

/// Event solver for non CCD movement
struct CActivatorInvalidator
{
public:
	CActivatorInvalidator()
	{
	}

	void Collect( CTriggerActivator* activator )
	{
		activator->InvalidatePlacement();
	}
};

void CTriggerManager::InvalidateActivatorsFromArea( const IntegerBox& worldBox )
{
	// collect from tree
	CActivatorInvalidator invalidator;
	m_activatorTree.CollectObjects( worldBox, invalidator );
}

void CTriggerManager::ProcessActivatorsMovement()
{
	PC_SCOPE(ProcessActivatorsMovement);

	// Process each moved activator generating the enter/exit events
	CTriggerEventBuffer events;
	for ( Uint32 i=0; i<m_movedActivators.Size(); ++i )
	{
		CTriggerActivator* activator = m_movedActivators[i];

		// Determine if we are allowed to use CCD on this activator
		Bool useCCD = activator->IsCCDEnabled();
		if ( activator->m_flagTeleported )
		{
			// When the activator was teleported do not use CCD
			activator->m_flagTeleported = 0;
			useCCD = false;
		}

		// Get the movement distance length and disable CCD if it's to small (within size of the activator) or to large (teleport)
		const IntegerVector4 movement = activator->GetNewPosition() - activator->GetPosition();
		const Float movementLength = movement.ToVector().Mag();
		const Float ccdLowerLimit = activator->GetExtents().ToVector().Mag();
		const Float ccdUpperLimit = activator->GetCCDMaxDistance();
		if ( movementLength <= ccdLowerLimit || movementLength >= ccdUpperLimit )
		{
			// in case of to large distance traveled disable the CCD
			if ( movementLength >= ccdUpperLimit )
			{
//				WARN_ENGINE( TXT("TriggerSystem: CCD disable for activator '%ls' in this frame: movement distance is to large (%f meters, limit = %f)"),
//					activator->GetDebugName().AsChar(), movementLength, ccdUpperLimit );
			}

			useCCD = false;
		}

		// Compute the movement box
		IntegerBox movementBox;
		movementBox.Min = activator->GetNewPosition() - activator->GetExtents();
		movementBox.Max = activator->GetNewPosition() + activator->GetExtents();

		// When using CCD extend the search range over the whole movement segment
		if ( useCCD )
		{
			// Previous placement box
			IntegerBox oldBox;
			oldBox.Min = activator->GetPosition() - activator->GetExtents();
			oldBox.Max = activator->GetPosition() + activator->GetExtents();

			movementBox.AddBox(oldBox);
		}

		// Reset helper data mask/filter structure
		m_maskBuffer.Reset();

		// Set the "inside" flags for objects that we were in in the preovious frame
		for ( Uint32 i=0; i<activator->m_interactions.Size(); ++i )
		{
			const Uint32 objectID = activator->m_interactions[i];
			m_maskBuffer.ReportExit( objectID ); // initail exit - if we don't get any info from this object this means we exited the trigger
			m_maskBuffer.SetInsideFlag( objectID );
			m_maskBuffer.ReportAsTested( objectID );
		}

		// Generate the enter/exit events
		if (useCCD)
		{
			PC_SCOPE(ResolveCCD);

			// CCD interaction resolving, slower and more compilcated
			//
			// Determine if the movement from previous to new position generates the "enter" and "exit" events
			// This is calculated by calculating the overlap of movement segment with each trigger object as a whole (all convex pieces).
			// If the overlap contains entry time > 0.0f than an "enter" condition occured (previous position was outside the trigger and we have crossed the boundary)
			// If the overlap contains exit time < 1.0f than an "exit" condition occured (new position is outside trigger and we have crossed the boundary)
			// Special case 1: Entry time = 0.0f - we have started inside the trigger, this is ignored, instead the "inside flag" from previous frame is used
			// Special case 2: Exit time = 1.0f - we have ended inside the trigger, this sets the "inside" flag for the next frame to true.
			// 
			// Finaly compare the inside/enter flags and generate the final enter/exit events as well as the final "inside" flag set
			// 

			CGenericCCDInteractionResolver resolver( m_maskBuffer, activator, m_terrainQuery );
			m_convexTree.CollectObjects( movementBox, resolver );

			// Generate the enter/exit events
			resolver.GenerateEvents( events, m_objects.TypedData(), activator );

			// Collect the
			resolver.CollectNewInsideList( activator->m_interactions );
		}
		else
		{
			PC_SCOPE(ResolveNonCCD);

			// Non CCD interaction resolving, very simple and fast
			//
			// If a given convex is from an object that was not yet determined to be yet entered this frame
			// test if we are intersecting the convex at the new position. If so mark the object as "entered"
			// and never test it again this frame (saves potential intersections with other shapes from the same trigger).
			// 
			// Finaly compare the inside/enter flags and generate the final enter/exit events as well as the final "inside" flag set
			// 
			// INSIDE | ENTERED 
			//    1   |    0         -> OnExit, Inside = 0
			//    0   |    1         -> OnEnter, Inside = 1
			//    1   |    1         -> nothing, Inside = 1
			//    0   |    0         -> nothing, Inside = 0 (does not happen)

			CNonCCDInteractionResolver resolver( m_maskBuffer, activator, m_terrainQuery );
			m_convexTree.CollectObjects( movementBox, resolver );

			// Generate the enter/exit events
			resolver.GenerateEvents( events, m_objects.TypedData(), activator );

			// Collect the
			resolver.CollectNewInsideList( activator->m_interactions );
		}

		// Update position
		activator->m_position = activator->m_newPosition;
		activator->m_flagHasMoved = false;

		// Refresh activator position in the activator tree
		activator->UpdatePlacement( m_activatorTree );
	}

	// Reset list of moved activators
	m_movedActivators.ClearFast();

	// Update the structure of the activator tree
	m_activatorTree.Update();

	// Fire the generated events
	{
		PC_SCOPE(FireEvents);

		// All entry events
		for ( Uint32 i=0; i<events.m_numEntryEvents; ++i )
		{
			const CTriggerEventBuffer::Event& e = events.m_entryEvents[i];
			e.m_object->OnEnter( e.m_activator );
		}

		// All exit events
		for ( Uint32 i=0; i<events.m_numExitEvents; ++i )
		{
			const CTriggerEventBuffer::Event& e = events.m_exitEvents[i];
			e.m_object->OnExit( e.m_activator );
		}

		// Debug report
		if ( NULL != m_debugListener )
		{
			for ( Uint32 i=0; i<events.m_numEntryEvents; ++i )
			{
				const CTriggerEventBuffer::Event& e = events.m_entryEvents[i];
				m_debugListener->OnTriggerEntered( e.m_object, e.m_activator );
			}

			// All exit events
			for ( Uint32 i=0; i<events.m_numExitEvents; ++i )
			{
				const CTriggerEventBuffer::Event& e = events.m_exitEvents[i];
				m_debugListener->OnTriggerExited( e.m_object, e.m_activator );
			}
		}
	}
}

/// Generic static query
struct CStaticInteractionResolver
{
public:
	Uint32 m_channels;
	Vector m_pos;
	CTriggerMaskBuffer* m_mask;

	ITriggerManager::TResultTriggers* m_out;

	CStaticInteractionResolver( CTriggerMaskBuffer& mask, const Uint32 channels, const Vector& pos, ITriggerManager::TResultTriggers& outResult )
		: m_mask( &mask )
		, m_pos( pos )
		, m_channels( channels )
		, m_out( &outResult )
	{
	}

	void Collect( CTriggerConvex* convex )
	{
		const CTriggerObject* object = convex->m_object;
		const Uint32 objectID = object->GetID();

		// This shape is on the ignore list
		if ( m_mask->IsIgnored( objectID ) )
		{
			return;
		}

		// Can we interact with this shape ?
		if ( !object->CanInteractWith( m_channels ) )
		{
			// never test it again
			m_mask->SetIgnoredFlag( objectID );
			return;
		}

		// Handle both CCD and non CCD triggers here
		if ( convex->TestOverlap( m_pos ) )
		{
			// do not test it again
			m_mask->SetIgnoredFlag( objectID );

			// add to output list
			if ( !m_out->Full() )
			{
				m_out->PushBack( object );
			}
		}
	}
};

void CTriggerManager::GetTriggersAtPoint( const Vector& worldPosition, const Uint32 triggerChannelMask, TResultTriggers& outTriggers )
{
	PC_SCOPE(GetTriggersAtPoint);

	// we need exclusive access to the tree :(
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_lock );

	// cleanup helper mask
	m_maskBuffer.Reset();

	// compute query box, can be a single point
	IntegerBox queryBox;
	queryBox.Min = IntegerVector4( worldPosition );
	queryBox.Max = queryBox.Min;

	// collect data
	CStaticInteractionResolver resolver( m_maskBuffer, triggerChannelMask, worldPosition, outTriggers );
	m_convexTree.CollectObjects( queryBox, resolver );
}

//---------------------------------------------------------------------------

ITriggerManager* ITriggerManager::CreateInstance()
{
	return new CTriggerManager();
}

//---------------------------------------------------------------------------
