/**
 * Copyright © 2008 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "physicsWorldPhysXImpl.h"
#include "physicsWorldPhysxImplBatchTrace.h"

#ifdef USE_PHYSX

#include "PxQueryReport.h"
#include "../physics/physicsWorldUtils.h"

using namespace physx;

#define SCENE_QUERY_FLAGS_FOR_RAYCAST		PxHitFlag::eIMPACT | PxHitFlag::eNORMAL | PxHitFlag::eDISTANCE
#define SCENE_QUERY_FLAGS_FOR_SWEEP_TEST	PxHitFlag::eIMPACT | PxHitFlag::eNORMAL | PxHitFlag::eDISTANCE | PxHitFlag::eMESH_MULTIPLE
#define SCENE_QUERY_FILTER_FLAGS			PxSceneQueryFilterFlags( PxSceneQueryFilterFlag::eDYNAMIC | PxSceneQueryFilterFlag::eSTATIC | PxSceneQueryFilterFlag::ePREFILTER )

namespace TraceUtils
{

	inline void CalcUnitNormal( const Vector& vA, const Vector& vB, const Vector& vC, Vector& normal )
	{
		normal = Vector::Cross( vB - vA, vC - vA );
		normal.Normalize3();
	}

	inline void CalcUnitPlaneEquation( const Vector& vA, const Vector& vB, const Vector& vC, Vector& planeOut )
	{
		// Compute triangle normal
		Vector vN;
		CalcUnitNormal( vA, vB, vC, vN );

		// Compute and store plane offset in w
		planeOut.X = vN.X;
		planeOut.Y = vN.Y;
		planeOut.Z = vN.Z;
		planeOut.W = -vN.Dot3( vA );
	}

	inline Bool IntersectPointTriangle( const Vector& vP, const Vector& vA, const Vector& vB, const Vector& vC,	const Vector& vN, Float tolerance, Vector* __restrict baryOut )
	{
		// Determine whether the point is inside the triangle
		Vector vDots;
		{
			Vector vPA = vA - vP;
			Vector vPB = vB - vP;
			Vector vPC = vC - vP;

			Vector nPAB = Vector::Cross( vPB, vN ); 
			Vector nPBC = Vector::Cross( vPC, vN ); 
			Vector nPCA = Vector::Cross( vPA, vN ); 

			vDots.X = vPB.Dot3( nPBC );
			vDots.Y = vPC.Dot3( nPCA );
			vDots.Z = vPA.Dot3( nPAB );
		}

		// Output barycentric coords
		if ( baryOut )
		{
			// Barycentric coordinates need normalization.
			const Float baryInvSum = 1.f / ( vDots.X + vDots.Y + vDots.Z );
			*baryOut = vDots * baryInvSum;
		}

		// vDots contains the un-normalized barycentric coordinates
		return vDots.X > tolerance && vDots.Y > tolerance && vDots.Z > tolerance;
	}

	//	Computes and returns the squared distance from vP to the triangle (vA, vB, vC). The normal and barycentric coordinates of the closest point on the triangle
	//	are also provided on output.

	inline Float PointTriangle3DDistanceSquared( const Vector& point, const Vector& vA, const Vector& vB, const Vector& vC, Vector* __restrict normalOut, Vector* __restrict baryOut )
	{
		// Set point w component to 1 to compute the plane equation easier
		Vector vP;	
		{
			vP.X = point.X;
			vP.Y = point.Y;
			vP.Z = point.Z;
			vP.W = 1.f;
		}

		// Compute plane equation
		Vector planeEq;
		Vector normal;
		CalcUnitPlaneEquation( vA, vB, vC, planeEq );

		// Closest point in triangle computation
		{
			// Compute projection of P (i.e. Q) on the triangle plane
			Float distanceToPlane = planeEq.Dot4( vP );
			Vector vQ = vP - ( planeEq * distanceToPlane );

			// Set normal to triangle normal. Will be overwritten later if case		
			normal.X = ( distanceToPlane < 0 ) ? -planeEq.X : planeEq.X;
			normal.Y = ( distanceToPlane < 0 ) ? -planeEq.Y : planeEq.Y;
			normal.Z = ( distanceToPlane < 0 ) ? -planeEq.Z : planeEq.Z;
			normal.W = ( distanceToPlane < 0 ) ? -planeEq.W : planeEq.W;

			// Determine whether the point is inside the triangle
			const Float epsilon = FLT_EPSILON;
			if ( IntersectPointTriangle( vQ, vA, vB, vC, planeEq, epsilon, baryOut ) )
			{
				*normalOut = normal;
				return distanceToPlane * distanceToPlane;
			}
		}

		// Closest point is on one of the edges
		{
			Vector vAB = vB - vA;
			Vector vBC = vC - vB;
			Vector vCA = vA - vC;
			Vector vAP = vP - vA;
			Vector vBP = vP - vB;
			Vector vCP = vP - vC;

			// Compute edge lengths
			Vector vEdgeLen, vSegLen;
			vSegLen.X = vAP.Dot3( vAB );
			vSegLen.Y = vBP.Dot3( vBC );
			vSegLen.Z = vCP.Dot3( vCA );
			vEdgeLen.X = vAB.Dot3( vAB );
			vEdgeLen.Y = vBC.Dot3( vBC );
			vEdgeLen.Z = vCA.Dot3( vCA );

			// Prevent division by zero
			vEdgeLen.X = Max( vEdgeLen.X, FLT_EPSILON );
			vEdgeLen.Y = Max( vEdgeLen.Y, FLT_EPSILON );
			vEdgeLen.Z = Max( vEdgeLen.Z, FLT_EPSILON );
			vEdgeLen.W = Max( vEdgeLen.W, FLT_EPSILON );

			// Compute times of intersection
			Vector vTois;
			{
				vTois.X = vSegLen.X / vEdgeLen.X;
				vTois.Y = vSegLen.Y / vEdgeLen.Y;
				vTois.Z = vSegLen.Z / vEdgeLen.Z;
				vTois.W = vSegLen.W / vEdgeLen.W;

				vTois.X = Max( vTois.X, 0.f );
				vTois.Y = Max( vTois.Y, 0.f );
				vTois.Z = Max( vTois.Z, 0.f );
				vTois.W = Max( vTois.W, 0.f );

				vTois.X = Min( vTois.X, 1.f );
				vTois.Y = Min( vTois.Y, 1.f );
				vTois.Z = Min( vTois.Z, 1.f );
				vTois.W = Min( vTois.W, 1.f );
			}

			// Compute normals & distances
			Vector vDistances;
			Vector normals[ 3 ];
			{
				normals[ 0 ] = vAP - ( vAB * vTois.X );
				normals[ 1 ] = vBP - ( vBC * vTois.Y );
				normals[ 2 ] = vCP - ( vCA * vTois.Z );

				vDistances.X = normals[ 0 ].Dot3( normals[ 0 ] );
				vDistances.Y = normals[ 1 ].Dot3( normals[ 1 ] );
				vDistances.Z = normals[ 2 ].Dot3( normals[ 2 ] );
			}

			// Compute the index of the minimum length normal
			Float minLen = vDistances.X;
			Int32 bestIndex = 0;

			if ( vDistances.Y < minLen )
			{
				minLen = vDistances.Y;
				bestIndex = 1;
			}

			if ( vDistances.Z < minLen )
			{
				minLen = vDistances.Z;
				bestIndex = 2;
			}

			normal = normals[ bestIndex ];

			// Compute barycentric coords if required
			if ( baryOut )
			{
				const Float u0 = 1.f - vTois.X;
				const Float u1 = 1.f - vTois.Y;
				const Float u2 = 1.f - vTois.Z;
				const Float v0 = vTois.X;
				const Float v1 = vTois.Y;
				const Float v2 = vTois.Z;

				if ( bestIndex == 0 )
				{
					*baryOut = Vector( u0, v0, 0.f, 0.f );
				}
				else if ( bestIndex == 1 )
				{
					*baryOut = Vector( 0.f, u1, v1, 0.f );
				}
				else if ( bestIndex == 2 )
				{
					*baryOut = Vector( v2, 0.f, u2, 0.f );
				}
			}

			*normalOut = normal;
			return minLen;		
		}
	}

	Float SpherePenetration( const PxSphereGeometry& geometry, const PxTransform& transform, const physx::PxShape* shape, const PxTransform& pose, Vector& outHitPosition )
	{
		const Uint32 BUF_SIZE = 256;
		PxGeometryType::Enum geomType = shape->getGeometryType();
		PxVec3 hitPosition = transform.p;

		switch ( geomType )
		{
		case PxGeometryType::eSPHERE:
		case PxGeometryType::ePLANE:
		case PxGeometryType::eCAPSULE:
		case PxGeometryType::eBOX:
		case PxGeometryType::eCONVEXMESH:
			{
				const Float dist = PxGeometryQuery::pointDistance( transform.p, shape->getGeometry().any(), pose, &hitPosition );
				if ( dist > 0.f )
				{
					outHitPosition = TO_VECTOR( hitPosition );
					return MSqrt( dist );
				}

				outHitPosition = TO_VECTOR( transform.p );
				return dist;
			}

		case PxGeometryType::eTRIANGLEMESH:
			{
				PxU32 triangleIndexBuffer[ BUF_SIZE ];
				PxU32 startIndex = 0;
				Bool bufferOverflowOccured = false;

				PxTriangleMeshGeometry trimesh; 
				if ( shape->getTriangleMeshGeometry( trimesh ) )
				{
					PxU32 numTriangles = PxMeshQuery::findOverlapTriangleMesh( geometry, transform, trimesh, pose, triangleIndexBuffer, BUF_SIZE, startIndex, bufferOverflowOccured );
					ASSERT( false == bufferOverflowOccured );

					if ( numTriangles )
					{
						// overlap happened
						const Vector point = TO_VECTOR( transform.p );
						Float bestDistanceSq = FLT_MAX;
						Vector normal, bary;

						for ( PxU32 i = 0; i < numTriangles; ++i )
						{
							PxTriangle tri;
							PxU32 vertexIndices[ 3 ];
							PxMeshQuery::getTriangle( trimesh, pose, triangleIndexBuffer[ i ], tri, vertexIndices );
							const Float distSq = PointTriangle3DDistanceSquared( point, TO_VECTOR( tri.verts[ 0 ] ), TO_VECTOR( tri.verts[ 1 ] ), TO_VECTOR( tri.verts[ 2 ] ), &normal, &bary );
							if ( distSq < bestDistanceSq )
							{
								bestDistanceSq = distSq;

								// Compute 3D (WS) position from barycentric coordinates (triangle space)
								outHitPosition = ( TO_VECTOR( tri.verts[ 0 ] ) * bary.X ) + ( TO_VECTOR( tri.verts[ 1 ] ) * bary.Y ) + ( TO_VECTOR( tri.verts[ 2 ] ) * bary.Z ); 
							}
						}

						return MSqrt( bestDistanceSq );
					}
				}

				return -1.f;
			}

		case PxGeometryType::eHEIGHTFIELD:
			{
				PxU32 triangleIndexBuffer[ BUF_SIZE ];
				PxU32 startIndex = 0;
				Bool bufferOverflowOccured = false;

				PxHeightFieldGeometry heightfield;
				if ( shape->getHeightFieldGeometry( heightfield ) )
				{
					PxU32 numTriangles = PxMeshQuery::findOverlapHeightField( geometry, transform, heightfield, pose, triangleIndexBuffer, BUF_SIZE, startIndex, bufferOverflowOccured );
					ASSERT( false == bufferOverflowOccured );

					if ( numTriangles )
					{
						// overlap happened
						const Vector point = TO_VECTOR( transform.p );
						Float bestDistanceSq = FLT_MAX;
						Vector normal, bary;

						for ( PxU32 i = 0; i < numTriangles; ++i )
						{
							PxTriangle tri;
							PxU32 vertexIndices[ 3 ];
							PxMeshQuery::getTriangle( heightfield, pose, triangleIndexBuffer[ i ], tri, vertexIndices );
							const Float distSq = PointTriangle3DDistanceSquared( point, TO_VECTOR( tri.verts[ 0 ] ), TO_VECTOR( tri.verts[ 1 ] ), TO_VECTOR( tri.verts[ 2 ] ), &normal, &bary );
							if ( distSq < bestDistanceSq )
							{
								bestDistanceSq = distSq;

								// Compute 3D (WS) position from barycentric coordinates (triangle space)
								outHitPosition = ( TO_VECTOR( tri.verts[ 0 ] ) * bary.X ) + ( TO_VECTOR( tri.verts[ 1 ] ) * bary.Y ) + ( TO_VECTOR( tri.verts[ 2 ] ) * bary.Z ); 
							}
						}

						return MSqrt( bestDistanceSq );
					}
				}

				return -1.f;
			}
		}

		return -1.f;
	}

}

Bool CPhysicsWorldPhysXImpl::IsSceneWhileProcessing() const
{ 
	Uint64 id = m_whileSceneProcess.GetValue();
	return id != 0;
}

CPhysicsWorldBatchQuery* CPhysicsWorldPhysXImpl::CreateBatchTrace( Uint32 raycastPrealocate, Uint32 maxRaycastHits, Uint32 sweepPrealocate, Uint32 maxSweepHits )
{
	CPhysicsWorldBatchQuery* batch = new CPhysicsWorldBatchQuery();
	batch->m_scene = m_scene;
	batch->Init( raycastPrealocate, maxRaycastHits, sweepPrealocate, maxSweepHits );
	return batch;
}

// Traces
ETraceReturnValue CPhysicsWorldPhysXImpl::RayCastWithSingleResult( const Vector& from, const Vector& to, CPhysicsEngine::CollisionMask include, CPhysicsEngine::CollisionMask exclude, SPhysicsContactInfo& contactInfo )
{
	PC_SCOPE_PHYSICS( Physics_RayCastWithSingleResult );

	const PxVec3 origin = TO_PX_VECTOR( from );
	PxVec3 unitVec = Vector::Equal3( from, to ) ? PxVec3( 0.f, 0.f, 0.001f ) : TO_PX_VECTOR( to ) - origin;
	const Float len = Clamp< Float > ( unitVec.normalizeSafe(), 0.001f, PX_MAX_SWEEP_DISTANCE );

	SPhysicalFilterData filter( include , exclude );
	PxQueryFilterData sceneQueryFilterData( filter.m_data, SCENE_QUERY_FILTER_FLAGS );
	PxRaycastHit hit;

	Bool wasHit = false;
	ETraceReturnValue retVal = TRV_NoHit;
	SceneUsageAddRef();
	if( !IsSceneWhileProcessing() )
	{
		wasHit = m_scene->raycastSingle( origin, unitVec, len, SCENE_QUERY_FLAGS_FOR_RAYCAST, hit, sceneQueryFilterData, &m_singleTraceCallback );
	}
	else
	{
		RED_LOG( PhysicalTrace, TXT( "failed RayCastWithSingleResult processed while scene fetch" ) );
		retVal = TRV_ProcessedWhileFetch;
	}

	SceneUsageRemoveRef();

	if ( wasHit )
	{
		contactInfo.m_position = TO_VECTOR( hit.position );
		contactInfo.m_normal = TO_VECTOR( hit.normal );
		contactInfo.m_distance = hit.distance / len;

		CPhysicsWrapperInterface* userData = reinterpret_cast< CPhysicsWrapperInterface* >( hit.shape->getActor()->userData );
		contactInfo.m_userDataA = userData;
		contactInfo.m_rigidBodyIndexA = ( SActorShapeIndex& ) hit.shape->userData;
		if( contactInfo.m_actorVolume )
		{
			PxBounds3 bounds = hit.actor->getWorldBounds();
			contactInfo.m_actorVolume->Min = TO_VECTOR( bounds.minimum );
			contactInfo.m_actorVolume->Max = TO_VECTOR( bounds.maximum );
		}
		if( contactInfo.m_shapeVolume )
		{
			PxBounds3 bounds = physx::PxShapeExt::getWorldBounds( *hit.shape, *hit.shape->getActor() );
			contactInfo.m_shapeVolume->Min = TO_VECTOR( bounds.minimum );
			contactInfo.m_shapeVolume->Max = TO_VECTOR( bounds.maximum );
		}
		retVal = TRV_Hit;
	}

	return retVal;
}

ETraceReturnValue CPhysicsWorldPhysXImpl::RayCastWithMultipleResults( const Vector& from, const Vector& to, CPhysicsEngine::CollisionMask include, CPhysicsEngine::CollisionMask exclude, SPhysicsContactInfo* outContactInfos, Uint32 & outContactsCount, Uint32 maxContactInfos )
{
	PC_SCOPE_PHYSICS( Physics_RayCastWithMultipleResults );

	const PxVec3 origin = TO_PX_VECTOR( from );
	PxVec3 unitVec = Vector::Equal3( from, to ) ? PxVec3( 0.f, 0.f, 0.001f ) : TO_PX_VECTOR( to ) - origin;
	const Float len = Clamp< Float > ( unitVec.normalizeSafe(), 0.001f, PX_MAX_SWEEP_DISTANCE );

	SPhysicalFilterData filter( include, exclude );
	PxSceneQueryFilterData sceneQueryFilterData( filter.m_data, SCENE_QUERY_FILTER_FLAGS );
	
	Bool blockingHit = false;
	PxRaycastHit *hits = new PxRaycastHit[ maxContactInfos ];

	Int32 hitCount = -1;
	SceneUsageAddRef();
	ETraceReturnValue retVal = TRV_NoHit;
	outContactsCount = 0;
	if( !IsSceneWhileProcessing() )
	{
		hitCount = m_scene->raycastMultiple( origin, unitVec, len, SCENE_QUERY_FLAGS_FOR_RAYCAST, hits, maxContactInfos, blockingHit, sceneQueryFilterData, &m_multiTraceCallback );

		// -1 here means we had buffer overflow, but we still return the results
		if( hitCount < 0 )
		{
			outContactsCount = maxContactInfos;
			retVal = TRV_BufferOverflow;
		}
		else if( hitCount > 0 )
		{
			outContactsCount = (Uint32)hitCount;
			retVal = TRV_Hit;
		}
	}
	else
	{
		RED_LOG( PhysicalTrace, TXT( "failed RayCastWithMultipleResults processed while scene fetch" ) );
		retVal = TRV_ProcessedWhileFetch;
	}
	SceneUsageRemoveRef();

	for ( Uint32 i = 0; i < outContactsCount; ++i )
	{
		SPhysicsContactInfo &contactInfo = outContactInfos[ i ];
		const PxRaycastHit& hit = hits[ i ];

		contactInfo.m_position = TO_VECTOR( hit.position );
		contactInfo.m_normal = TO_VECTOR( hit.normal );
		contactInfo.m_distance = hit.distance / len;

		CPhysicsWrapperInterface* userData = reinterpret_cast< CPhysicsWrapperInterface* >( hit.shape->getActor()->userData );
		contactInfo.m_userDataA = userData;
		contactInfo.m_rigidBodyIndexA = ( SActorShapeIndex& ) hit.shape->userData;
		if( contactInfo.m_actorVolume )
		{
			PxBounds3 bounds = hit.actor->getWorldBounds();
			contactInfo.m_actorVolume->Min = TO_VECTOR( bounds.minimum );
			contactInfo.m_actorVolume->Max = TO_VECTOR( bounds.maximum );
		}
		if( contactInfo.m_shapeVolume )
		{
			PxBounds3 bounds = physx::PxShapeExt::getWorldBounds( *hit.shape, *hit.shape->getActor() );
			contactInfo.m_shapeVolume->Min = TO_VECTOR( bounds.minimum );
			contactInfo.m_shapeVolume->Max = TO_VECTOR( bounds.maximum );
		}

	}

	delete [] hits;
	return retVal;
}

ETraceReturnValue CPhysicsWorldPhysXImpl::SweepTestAnyResult( const Vector& from, const Vector& to, Float radius, CPhysicsEngine::CollisionMask include, CPhysicsEngine::CollisionMask exclude )
{
	PC_SCOPE_PHYSICS( Physics_SweepTestAnyResult );

	PxSphereGeometry geometry( radius );
	PxTransform transform( TO_PX_VECTOR( from ), PxQuat::createIdentity() );

	const PxVec3 origin = TO_PX_VECTOR( from );
	PxVec3 unitVec = Vector::Equal3( from, to ) ? PxVec3( 0.f, 0.f, 0.001f ) : TO_PX_VECTOR( to ) - origin;
	const Float len = Clamp< Float > ( unitVec.normalizeSafe(), 0.001f, PX_MAX_SWEEP_DISTANCE );

	PxSceneQueryHit hit;

	SPhysicalFilterData filter( include, exclude );
	PxSceneQueryFilterData sceneQueryFilterData( filter.m_data, SCENE_QUERY_FILTER_FLAGS );

	Bool wasHit = false;
	ETraceReturnValue retVal = TRV_NoHit;
	SceneUsageAddRef();
	if( !IsSceneWhileProcessing() )
	{
		wasHit = m_scene->sweepAny( geometry, transform, unitVec, len, SCENE_QUERY_FLAGS_FOR_SWEEP_TEST, hit, sceneQueryFilterData, &m_singleTraceCallback );
		retVal = wasHit ? TRV_Hit : TRV_NoHit;
	}
	else
	{
		RED_LOG( PhysicalTrace, TXT( "failed SweepTestAnyResult processed while scene fetch" ) );
		retVal = TRV_ProcessedWhileFetch;
	}
	SceneUsageRemoveRef();

	return retVal;
}

ETraceReturnValue CPhysicsWorldPhysXImpl::SweepTestWithSingleResult( const Vector& from, const Vector& to, Float radius, CPhysicsEngine::CollisionMask include, CPhysicsEngine::CollisionMask exclude, SPhysicsContactInfo& contactInfo, Bool precise )
{
	PC_SCOPE_PHYSICS( Physics_SweepTestWithSingleResult );

	PxSphereGeometry geometry( radius );
	PxTransform transform( TO_PX_VECTOR( from ), PxQuat::createIdentity() );

	const PxVec3 origin = TO_PX_VECTOR( from );
	PxVec3 unitVec = Vector::Equal3( from, to ) ? PxVec3( 0.f, 0.f, 0.001f ) : TO_PX_VECTOR( to ) - origin;
	const Float len = Clamp< Float > ( unitVec.normalizeSafe(), 0.001f, PX_MAX_SWEEP_DISTANCE );

	PxSweepHit hit;
	if ( precise )
	{
		hit.flags |= PxHitFlag::ePRECISE_SWEEP;
	}
	Bool blockingHit = false;

	SPhysicalFilterData filter( include, exclude );
	PxSceneQueryFilterData sceneQueryFilterData( filter.m_data, SCENE_QUERY_FILTER_FLAGS );

	Bool wasHit = false;
	ETraceReturnValue retVal = TRV_NoHit;
	SceneUsageAddRef();
	if( !IsSceneWhileProcessing() )
	{
		wasHit = m_scene->sweepSingle( geometry, transform, unitVec, len, SCENE_QUERY_FLAGS_FOR_SWEEP_TEST, hit, sceneQueryFilterData, &m_singleTraceCallback );
	}
	else
	{
		RED_LOG( PhysicalTrace, TXT( "failed SweepTestWithSingleResult processed while scene fetch" ) );
		retVal = TRV_ProcessedWhileFetch;
	}
	SceneUsageRemoveRef();

	if ( wasHit )
	{
		contactInfo.m_position = TO_VECTOR( hit.position );
		contactInfo.m_normal = TO_VECTOR( hit.normal );
		contactInfo.m_distance = hit.distance / len;
		contactInfo.m_userDataA = static_cast<CPhysicsWrapperInterface*>( hit.shape->getActor()->userData );
		contactInfo.m_rigidBodyIndexA = ( SActorShapeIndex& ) hit.shape->userData;

		// Work around strange behavior of PhysX:
		// When sphere is initially colliding (before it starts to sweep), it returns impact point always in the center of the sphere,
		// even if it's actually somewhere else. We need to fix that by doing our own maths (which we're doing anyway in overlap tests),
		// to calculate the real impact point.
		if ( Vector::Equal3( contactInfo.m_position, from ) )
		{
			Vector ourOwnResult;
			PxSphereGeometry geometryWithExtraTolerance( radius + 0.05f );
			if ( 0 <= TraceUtils::SpherePenetration( geometryWithExtraTolerance, transform, hit.shape, hit.shape->getLocalPose() * hit.shape->getActor()->getGlobalPose(), ourOwnResult ) )
			{
				contactInfo.m_position = ourOwnResult;
			}
		}

		if( contactInfo.m_actorVolume )
		{
			PxBounds3 bounds = hit.actor->getWorldBounds();
			contactInfo.m_actorVolume->Min = TO_VECTOR( bounds.minimum );
			contactInfo.m_actorVolume->Max = TO_VECTOR( bounds.maximum );
		}
		if( contactInfo.m_shapeVolume )
		{
			PxBounds3 bounds = physx::PxShapeExt::getWorldBounds( *hit.shape, *hit.shape->getActor() );
			contactInfo.m_shapeVolume->Min = TO_VECTOR( bounds.minimum );
			contactInfo.m_shapeVolume->Max = TO_VECTOR( bounds.maximum );
		}
		retVal = TRV_Hit;
	}

	return retVal;
}

ETraceReturnValue CPhysicsWorldPhysXImpl::CapsuleSweepTestWithSingleResult( const Vector& from, const Vector& to, const Float radius, const Float height, CPhysicsEngine::CollisionMask include, CPhysicsEngine::CollisionMask exclude, SPhysicsContactInfo& contactInfo, Bool usePreciseSweep )
{
	ASSERT( height > 2.0f*radius );

	PC_SCOPE_PHYSICS( Physics_CapsuleSweepTestWithSingleResult );

	PxCapsuleGeometry geometry( radius, height*0.5f-radius );
	PxTransform transform( TO_PX_VECTOR( from ), PxQuat( M_PI*0.5f, PxVec3( 0.0f, 1.0f, 0.0f ) ) );
	if( !transform.isValid() ) return TRV_InvalidTransform;

	const PxVec3 origin = TO_PX_VECTOR( from );
	PxVec3 unitVec = Vector::Equal3( from, to ) ? PxVec3( 0.f, 0.f, 0.001f ) : TO_PX_VECTOR( to ) - origin;
	const Float len = Clamp< Float > ( unitVec.normalizeSafe(), 0.001f, PX_MAX_SWEEP_DISTANCE );

	PxSweepHit hit;
	Bool blockingHit = false;

	SPhysicalFilterData filter( include, exclude );
	PxSceneQueryFilterData sceneQueryFilterData( filter.m_data, SCENE_QUERY_FILTER_FLAGS );

	PxHitFlags sweepFlags = SCENE_QUERY_FLAGS_FOR_SWEEP_TEST;
	if ( usePreciseSweep )
		sweepFlags |= PxHitFlag::ePRECISE_SWEEP;

	Bool wasHit = false;
	ETraceReturnValue retVal = TRV_NoHit;
	SceneUsageAddRef();
	if( !IsSceneWhileProcessing() )
	{
		wasHit = m_scene->sweepSingle( geometry, transform, unitVec, len, sweepFlags, hit, sceneQueryFilterData, &m_singleTraceCallback );
	}
	else
	{
		RED_LOG( PhysicalTrace, TXT( "failed CapsuleSweepTestWithSingleResult processed while scene fetch" ) );
		retVal = TRV_ProcessedWhileFetch;
	}
	SceneUsageRemoveRef();

	if ( wasHit )
	{
		contactInfo.m_position = TO_VECTOR( hit.position );
		contactInfo.m_normal = TO_VECTOR( hit.normal );
		contactInfo.m_distance = hit.distance / len;
		contactInfo.m_userDataA = static_cast<CPhysicsWrapperInterface*>( hit.shape->getActor()->userData );
		contactInfo.m_rigidBodyIndexA = ( SActorShapeIndex& ) hit.shape->userData;

		// Work around strange behavior of PhysX:
		// When sphere is initially colliding (before it starts to sweep), it returns impact point always in the center of the sphere,
		// even if it's actually somewhere else. We need to fix that by doing our own maths (which we're doing anyway in overlap tests),
		// to calculate the real impact point.
		if ( Vector::Equal3( contactInfo.m_position, from ) )
		{
			Vector ourOwnResult;
			PxSphereGeometry geometryWithExtraTolerance( radius + 0.005f );
			if ( 0 <= TraceUtils::SpherePenetration( geometryWithExtraTolerance, transform, hit.shape, hit.shape->getLocalPose() * hit.shape->getActor()->getGlobalPose(), ourOwnResult ) )
			{
				//LOG_ENGINE( TXT( "penetration %0.3f > %0.3f > %0.3f" ), len, (contactInfo.m_position-ourOwnResult).Mag3(), ((contactInfo.m_position-ourOwnResult).Mag3()/len) );
				contactInfo.m_distance = (contactInfo.m_position-ourOwnResult).Mag3()-radius;
				contactInfo.m_normal = (contactInfo.m_position-ourOwnResult).Normalized3();
				contactInfo.m_position = ourOwnResult;
			}
		}

		if( contactInfo.m_actorVolume )
		{
			PxBounds3 bounds = hit.actor->getWorldBounds();
			contactInfo.m_actorVolume->Min = TO_VECTOR( bounds.minimum );
			contactInfo.m_actorVolume->Max = TO_VECTOR( bounds.maximum );
		}
		if( contactInfo.m_shapeVolume )
		{
			PxBounds3 bounds = physx::PxShapeExt::getWorldBounds( *hit.shape, *hit.shape->getActor() );
			contactInfo.m_shapeVolume->Min = TO_VECTOR( bounds.minimum );
			contactInfo.m_shapeVolume->Max = TO_VECTOR( bounds.maximum );
		}
		retVal = TRV_Hit;
	}

	return retVal;
}

ETraceReturnValue CPhysicsWorldPhysXImpl::BoxSweepTestWithSingleResult( const Vector& from, const Vector& to, const Vector3& halfExtents, const Matrix& rotation, CPhysicsEngine::CollisionMask include, CPhysicsEngine::CollisionMask exclude, SPhysicsContactInfo& contactInfo, Bool usePreciseSweep )
{
	PC_SCOPE_PHYSICS( Physics_BoxSweepTestWithSingleResult );

	const Vector qRot = rotation.ToQuat();

	PxBoxGeometry geometry( halfExtents.X, halfExtents.Y, halfExtents.Z );
	PxTransform transform( TO_PX_VECTOR( from ), TO_PX_QUAT( qRot ) );
	if( !transform.isValid() ) return TRV_InvalidTransform;

	const PxVec3 origin = TO_PX_VECTOR( from );
	PxVec3 unitVec = Vector::Equal3( from, to ) ? PxVec3( 0.f, 0.f, 0.001f ) : TO_PX_VECTOR( to ) - origin;
	const Float len = Clamp< Float > ( unitVec.normalizeSafe(), 0.001f, PX_MAX_SWEEP_DISTANCE );

	PxSweepHit hit;
	Bool blockingHit = false;

	SPhysicalFilterData filter( include, exclude );
	PxSceneQueryFilterData sceneQueryFilterData( filter.m_data, SCENE_QUERY_FILTER_FLAGS );
    
    // Use precise sweep test?
    PxHitFlags sweepFlags = SCENE_QUERY_FLAGS_FOR_SWEEP_TEST;
    if( usePreciseSweep )
        sweepFlags |= PxHitFlag::ePRECISE_SWEEP;

	Bool wasHit = false;
	ETraceReturnValue retVal = TRV_NoHit;
	SceneUsageAddRef();
	if( !IsSceneWhileProcessing() )
	{
		wasHit = m_scene->sweepSingle( geometry, transform, unitVec, len, sweepFlags, hit, sceneQueryFilterData, &m_singleTraceCallback );
	}
	else
	{
		RED_LOG( PhysicalTrace, TXT( "failed BoxSweepTestWithSingleResult processed while scene fetch" ) );
		retVal = TRV_ProcessedWhileFetch;
	}
	SceneUsageRemoveRef();

	if ( wasHit )
	{
		contactInfo.m_position = TO_VECTOR( hit.position );
		contactInfo.m_normal = TO_VECTOR( hit.normal );
		contactInfo.m_distance = hit.distance / len;
		contactInfo.m_userDataA = static_cast<CPhysicsWrapperInterface*>( hit.shape->getActor()->userData );
		contactInfo.m_rigidBodyIndexA = ( SActorShapeIndex& ) hit.shape->userData;

		if( contactInfo.m_actorVolume )
		{
			PxBounds3 bounds = hit.actor->getWorldBounds();
			contactInfo.m_actorVolume->Min = TO_VECTOR( bounds.minimum );
			contactInfo.m_actorVolume->Max = TO_VECTOR( bounds.maximum );
		}
		if( contactInfo.m_shapeVolume )
		{
			PxBounds3 bounds = physx::PxShapeExt::getWorldBounds( *hit.shape, *hit.shape->getActor() );
			contactInfo.m_shapeVolume->Min = TO_VECTOR( bounds.minimum );
			contactInfo.m_shapeVolume->Max = TO_VECTOR( bounds.maximum );
		}
		retVal = TRV_Hit;
	}

	return retVal;
}

ETraceReturnValue CPhysicsWorldPhysXImpl::SphereSweepTestWithMultipleResults( const Vector& from, const Vector& to, Float radius, CPhysicsEngine::CollisionMask include, CPhysicsEngine::CollisionMask exclude, SPhysicsContactInfo* outContactInfos, Uint32 & outContactsCount, Uint32 maxContactInfos )
{
	PC_SCOPE_PHYSICS( Physics_SphereSweepTestWithMultipleResults );

	PxSphereGeometry geometry( radius );
	PxTransform transform( TO_PX_VECTOR( from ), PxQuat::createIdentity() );

	const PxVec3 origin = TO_PX_VECTOR( from );
	PxVec3 unitVec = Vector::Equal3( from, to ) ? PxVec3( 0.f, 0.f, 0.001f ) : TO_PX_VECTOR( to ) - origin;
	const Float len = Clamp< Float > ( unitVec.normalizeSafe(), 0.001f, PX_MAX_SWEEP_DISTANCE );

	PxSweepHit* hits = new PxSweepHit[ maxContactInfos ];
	Bool blockingHit = false;

	SPhysicalFilterData filter( include, exclude );
	PxSceneQueryFilterData sceneQueryFilterData( filter.m_data, SCENE_QUERY_FILTER_FLAGS );

	Int32 hitCount = -1;
	SceneUsageAddRef();
	ETraceReturnValue retVal = TRV_NoHit;
	outContactsCount = 0;
	if( !IsSceneWhileProcessing() )
	{
		hitCount = m_scene->sweepMultiple( geometry, transform, unitVec, len, SCENE_QUERY_FLAGS_FOR_SWEEP_TEST, hits, maxContactInfos, blockingHit, sceneQueryFilterData, &m_multiTraceCallback );

		// -1 here means we had buffer overflow, but we still return the results
		if( hitCount < 0 )
		{
			outContactsCount = maxContactInfos;
			retVal = TRV_BufferOverflow;
		}
		else if( hitCount > 0 )
		{
			outContactsCount = (Uint32)hitCount;
			retVal = TRV_Hit;
		}
	}
	else
	{
		RED_LOG( PhysicalTrace, TXT( "failed SphereSweepTestWithMultipleResults processed while scene fetch" ) );
		retVal = TRV_ProcessedWhileFetch;
	}
	SceneUsageRemoveRef();

	for ( Uint32 i = 0; i < outContactsCount; ++i )
	{
		SPhysicsContactInfo& contactInfo = outContactInfos[ i ];
		const PxSweepHit& hit = hits[ i ];

		contactInfo.m_position = TO_VECTOR( hit.position );
		contactInfo.m_normal = TO_VECTOR( hit.normal );
		contactInfo.m_distance = hit.distance / len;
		contactInfo.m_userDataA = static_cast< CPhysicsWrapperInterface* > ( hit.shape->getActor()->userData );
		contactInfo.m_rigidBodyIndexA = ( SActorShapeIndex& ) hit.shape->userData;

		// Work around strange behavior of PhysX:
		// When sphere is initially colliding (before it starts to sweep), it returns impact point always in the center of the sphere,
		// even if it's actually somewhere else. We need to fix that by doing our own maths (which we're doing anyway in overlap tests),
		// to calculate the real impact point.
		if ( Vector::Equal3( contactInfo.m_position, from ) )
		{
			Vector ourOwnResult;
			PxSphereGeometry geometryWithExtraTolerance( radius + 0.05f );
			if ( 0 <= TraceUtils::SpherePenetration( geometryWithExtraTolerance, transform, hit.shape, hit.shape->getLocalPose() * hit.shape->getActor()->getGlobalPose(), ourOwnResult ) )
			{
				contactInfo.m_position = ourOwnResult;
			}
		}

		if( contactInfo.m_actorVolume )
		{
			PxBounds3 bounds = hit.actor->getWorldBounds();
			contactInfo.m_actorVolume->Min = TO_VECTOR( bounds.minimum );
			contactInfo.m_actorVolume->Max = TO_VECTOR( bounds.maximum );
		}
		if( contactInfo.m_shapeVolume )
		{
			PxBounds3 bounds = physx::PxShapeExt::getWorldBounds( *hit.shape, *hit.shape->getActor() );
			contactInfo.m_shapeVolume->Min = TO_VECTOR( bounds.minimum );
			contactInfo.m_shapeVolume->Max = TO_VECTOR( bounds.maximum );
		}

	}

	delete [] hits;
	return retVal;
}

ETraceReturnValue CPhysicsWorldPhysXImpl::SphereSweepTestWithCustomCallbackNoResults( const Vector& from, const Vector& to, Float radius, CPhysicsEngine::CollisionMask include, CPhysicsEngine::CollisionMask exclude, ICustomTraceCallback * customCallback )
{
	PC_SCOPE_PHYSICS( Physics_SphereSweepTestWithCustomCallbackNoResults );

	PxSphereGeometry geometry( radius );
	PxTransform transform( TO_PX_VECTOR( from ), PxQuat::createIdentity() );

	const PxVec3 origin = TO_PX_VECTOR( from );
	PxVec3 unitVec = Vector::Equal3( from, to ) ? PxVec3( 0.f, 0.f, 0.001f ) : TO_PX_VECTOR( to ) - origin;
	const Float len = Clamp< Float > ( unitVec.normalizeSafe(), 0.001f, PX_MAX_SWEEP_DISTANCE );

	PxSweepHit hit;
	Bool blockingHit = false;

	SPhysicalFilterData filter( include, exclude );
	PxSceneQueryFilterData sceneQueryFilterData( filter.m_data, SCENE_QUERY_FILTER_FLAGS );

	SceneUsageAddRef();
	ETraceReturnValue returnValue = TRV_Hit;
	if( !IsSceneWhileProcessing() )
	{
		m_scene->sweepSingle( geometry, transform, unitVec, len, SCENE_QUERY_FLAGS_FOR_SWEEP_TEST, hit, sceneQueryFilterData, customCallback);
	}
	else
	{
		RED_LOG( PhysicalTrace, TXT( "failed SphereSweepTestWithCustomCallbackNoResults processed while scene fetch" ) );
		returnValue = TRV_ProcessedWhileFetch;
	}
	SceneUsageRemoveRef();

	return returnValue;
}

ETraceReturnValue CPhysicsWorldPhysXImpl::CapsuleSweepTestWithMultipleResults( const Vector& from, const Vector& to, const Float radius, const Float height, CPhysicsEngine::CollisionMask include, CPhysicsEngine::CollisionMask exclude, SPhysicsContactInfo* contactInfos, Uint32 & outContactsCount, Uint32 maxContactInfos, CPhysicsTraceCallback* traceCallback, Bool usePreciseSweep )
{
	ASSERT( height > 2.0f*radius );

	PC_SCOPE_PHYSICS( Physics_CapsuleSweepTestWithMultipleResults );

	PxCapsuleGeometry geometry( radius, height*0.5f-radius );
	PxTransform transform( TO_PX_VECTOR( from ), PxQuat( M_PI*0.5f, PxVec3( 0.0f, 1.0f, 0.0f ) ) );
	if( !transform.isValid() ) return TRV_InvalidTransform;

	const PxVec3 origin = TO_PX_VECTOR( from );
	PxVec3 unitVec = Vector::Equal3( from, to ) ? PxVec3( 0.f, 0.f, 0.001f ) : TO_PX_VECTOR( to ) - origin;
	const Float len = Clamp< Float > ( unitVec.normalizeSafe(), 0.001f, PX_MAX_SWEEP_DISTANCE );

	SPhysicalFilterData filter( include, exclude );
	PxSceneQueryFilterData sceneQueryFilterData( filter.m_data, SCENE_QUERY_FILTER_FLAGS );

	PxSweepHit* hits = new PxSweepHit[ maxContactInfos ];
	Bool blockingHit = false;

	PxHitFlags sweepFlags = SCENE_QUERY_FLAGS_FOR_SWEEP_TEST;
	if ( usePreciseSweep )
		sweepFlags |= PxHitFlag::ePRECISE_SWEEP;

	Int32 hitCount = -1;
	SceneUsageAddRef();
	ETraceReturnValue retVal = TRV_NoHit;
	outContactsCount = 0;
	if( !IsSceneWhileProcessing() )
	{
		if( traceCallback != nullptr )
			hitCount = m_scene->sweepMultiple( geometry, transform, unitVec, len, sweepFlags, hits, maxContactInfos, blockingHit, sceneQueryFilterData, traceCallback );
		else
			hitCount = m_scene->sweepMultiple( geometry, transform, unitVec, len, sweepFlags, hits, maxContactInfos, blockingHit, sceneQueryFilterData, &m_multiTraceCallback );

		// -1 here means we had buffer overflow, but we still return the results
		if( hitCount < 0 )
		{
			outContactsCount = maxContactInfos;
			retVal = TRV_BufferOverflow;
		}
		else if( hitCount > 0 )
		{
			outContactsCount = (Uint32)hitCount;
			retVal = TRV_Hit;
		}	
	}
	else
	{
		RED_LOG( PhysicalTrace, TXT( "failed CapsuleSweepTestWithMultipleResults processed while scene fetch" ) );
		retVal = TRV_ProcessedWhileFetch;
	}
	SceneUsageRemoveRef();

	for ( Uint32 i = 0; i < outContactsCount; ++i )
	{
		SPhysicsContactInfo& contactInfo = contactInfos[ i ];
		const PxSweepHit& hit = hits[ i ];

		contactInfo.m_position = TO_VECTOR( hit.position );
		contactInfo.m_normal = TO_VECTOR( hit.normal );
		contactInfo.m_distance = hit.distance / len;
		contactInfo.m_userDataA = static_cast<CPhysicsWrapperInterface*>( hit.shape->getActor()->userData );
		contactInfo.m_rigidBodyIndexA = ( SActorShapeIndex& ) hit.shape->userData;

		// Work around strange behavior of PhysX:
		// When sphere is initially colliding (before it starts to sweep), it returns impact point always in the center of the sphere,
		// even if it's actually somewhere else. We need to fix that by doing our own maths (which we're doing anyway in overlap tests),
		// to calculate the real impact point.
		if ( Vector::Equal3( contactInfo.m_position, from ) )
		{
			Vector ourOwnResult;
			PxSphereGeometry geometryWithExtraTolerance( radius + 0.005f );
			if ( 0 <= TraceUtils::SpherePenetration( geometryWithExtraTolerance, transform, hit.shape, hit.shape->getLocalPose() * hit.shape->getActor()->getGlobalPose(), ourOwnResult ) )
			{
				//LOG_ENGINE( TXT( "penetration %0.3f > %0.3f > %0.3f" ), len, (contactInfo.m_position-ourOwnResult).Mag3(), ((contactInfo.m_position-ourOwnResult).Mag3()/len) );
				contactInfo.m_distance = (contactInfo.m_position-ourOwnResult).Mag3()-radius;
				contactInfo.m_normal = (contactInfo.m_position-ourOwnResult).Normalized3();
				contactInfo.m_position = ourOwnResult;
			}
		}

		if( contactInfo.m_actorVolume )
		{
			PxBounds3 bounds = hit.actor->getWorldBounds();
			contactInfo.m_actorVolume->Min = TO_VECTOR( bounds.minimum );
			contactInfo.m_actorVolume->Max = TO_VECTOR( bounds.maximum );
		}
		if( contactInfo.m_shapeVolume )
		{
			PxBounds3 bounds = physx::PxShapeExt::getWorldBounds( *hit.shape, *hit.shape->getActor() );
			contactInfo.m_shapeVolume->Min = TO_VECTOR( bounds.minimum );
			contactInfo.m_shapeVolume->Max = TO_VECTOR( bounds.maximum );
		}

	}

	delete [] hits;
	return retVal;
}

ETraceReturnValue CPhysicsWorldPhysXImpl::BoxSweepTestWithMultipleResults( const Vector& from, const Vector& to, const Vector3& halfExtents, const Matrix& rotation, CPhysicsEngine::CollisionMask include, CPhysicsEngine::CollisionMask exclude, SPhysicsContactInfo* outContactInfos, Uint32 & outContactsCount, Uint32 maxContactInfos, CPhysicsTraceCallback* traceCallback, Bool usePreciseSweep )
{
	PC_SCOPE_PHYSICS( Physics_BoxSweepTestWithMultipleResults );

	const Vector qRot = rotation.ToQuat();

	PxBoxGeometry geometry( halfExtents.X, halfExtents.Y, halfExtents.Z );
	PxTransform transform( TO_PX_VECTOR( from ), TO_PX_QUAT( qRot ) );
	if( !transform.isValid() ) return TRV_InvalidTransform;

	const PxVec3 origin = TO_PX_VECTOR( from );
	PxVec3 unitVec = Vector::Equal3( from, to ) ? PxVec3( 0.f, 0.f, 0.001f ) : TO_PX_VECTOR( to ) - origin;
	const Float len = Clamp< Float > ( unitVec.normalizeSafe(), 0.001f, PX_MAX_SWEEP_DISTANCE );

	SPhysicalFilterData filter( include, exclude );
	PxSceneQueryFilterData sceneQueryFilterData( filter.m_data, SCENE_QUERY_FILTER_FLAGS );

	PxSweepHit* hits = new PxSweepHit[ maxContactInfos ];
	Bool blockingHit = false;

	PxHitFlags sweepFlags = SCENE_QUERY_FLAGS_FOR_SWEEP_TEST;
	if ( usePreciseSweep )
		sweepFlags |= PxHitFlag::ePRECISE_SWEEP;

	Int32 hitCount = -1;
	SceneUsageAddRef();
	ETraceReturnValue retVal = TRV_NoHit;
	outContactsCount = 0;
	if( !IsSceneWhileProcessing() )
	{
		if( traceCallback != nullptr )
			hitCount = m_scene->sweepMultiple( geometry, transform, unitVec, len, sweepFlags, hits, maxContactInfos, blockingHit, sceneQueryFilterData, traceCallback );
		else
			hitCount = m_scene->sweepMultiple( geometry, transform, unitVec, len, sweepFlags, hits, maxContactInfos, blockingHit, sceneQueryFilterData, &m_multiTraceCallback );

		// -1 here means we had buffer overflow, but we still return the results
		if( hitCount < 0 )
		{
			outContactsCount = maxContactInfos;
			retVal = TRV_BufferOverflow;
		}
		else if( hitCount > 0 )
		{
			outContactsCount = (Uint32)hitCount;
			retVal = TRV_Hit;
		}	
	}
	else
	{
		RED_LOG( PhysicalTrace, TXT( "failed CapsuleSweepTestWithMultipleResults processed while scene fetch" ) );
	}
	SceneUsageRemoveRef();

	for ( Uint32 i = 0; i < outContactsCount; ++i )
	{
		SPhysicsContactInfo& contactInfo = outContactInfos[ i ];
		const PxSweepHit& hit = hits[ i ];

		contactInfo.m_position = TO_VECTOR( hit.position );
		contactInfo.m_normal = TO_VECTOR( hit.normal );
		contactInfo.m_distance = hit.distance / len;
		contactInfo.m_userDataA = static_cast<CPhysicsWrapperInterface*>( hit.shape->getActor()->userData );
		contactInfo.m_rigidBodyIndexA = ( SActorShapeIndex& ) hit.shape->userData;


		if( contactInfo.m_actorVolume )
		{
			PxBounds3 bounds = hit.actor->getWorldBounds();
			contactInfo.m_actorVolume->Min = TO_VECTOR( bounds.minimum );
			contactInfo.m_actorVolume->Max = TO_VECTOR( bounds.maximum );
		}
		if( contactInfo.m_shapeVolume )
		{
			PxBounds3 bounds = physx::PxShapeExt::getWorldBounds( *hit.shape, *hit.shape->getActor() );
			contactInfo.m_shapeVolume->Min = TO_VECTOR( bounds.minimum );
			contactInfo.m_shapeVolume->Max = TO_VECTOR( bounds.maximum );
		}
	}

	delete [] hits;
	return retVal;
}

ETraceReturnValue CPhysicsWorldPhysXImpl::SphereOverlapWithAnyResult( const Vector& position, Float radius, CPhysicsEngine::CollisionMask include, CPhysicsEngine::CollisionMask exclude, SPhysicsOverlapInfo& contactInfo ) 
{
	PC_SCOPE_PHYSICS( Physics_SphereOverlapWithAnyResult );

	PxSphereGeometry geometry( radius );
	PxTransform transform( TO_PX_VECTOR( position ), PxQuat::createIdentity() );

	PxOverlapHit hit;
	Bool blockingHit = false;

	SPhysicalFilterData filter( include, exclude );
	PxSceneQueryFilterData sceneQueryFilterData( filter.m_data, SCENE_QUERY_FILTER_FLAGS );

	Bool wasHit = false;
	ETraceReturnValue retVal = TRV_NoHit;
	SceneUsageAddRef();
	if( !IsSceneWhileProcessing() )
	{
		wasHit = m_scene->overlapAny( geometry, transform, hit, sceneQueryFilterData, &m_singleTraceCallback );
	}
	else
	{
		RED_LOG( PhysicalTrace, TXT( "failed SphereOverlapWithAnyResult processed while scene fetch" ) );
		retVal = TRV_ProcessedWhileFetch;
	}
	SceneUsageRemoveRef();

	if ( wasHit )
	{
		contactInfo.m_userData = static_cast< CPhysicsWrapperInterface* >( hit.actor->userData );
		contactInfo.m_actorNshapeIndex = ( SActorShapeIndex& ) hit.shape->userData;
		contactInfo.m_penetration = TraceUtils::SpherePenetration( geometry, transform, hit.shape, hit.shape->getLocalPose() * hit.actor->getGlobalPose(), contactInfo.m_position ) - radius;
		if ( contactInfo.m_penetration < -radius )
		{
			contactInfo.m_penetration = 0.f;
		}
		if( contactInfo.m_actorVolume )
		{
			PxBounds3 bounds = hit.actor->getWorldBounds();
			contactInfo.m_actorVolume->Min = TO_VECTOR( bounds.minimum );
			contactInfo.m_actorVolume->Max = TO_VECTOR( bounds.maximum );
		}
		if( contactInfo.m_shapeVolume )
		{
			PxBounds3 bounds = physx::PxShapeExt::getWorldBounds( *hit.shape, *hit.shape->getActor() );
			contactInfo.m_shapeVolume->Min = TO_VECTOR( bounds.minimum );
			contactInfo.m_shapeVolume->Max = TO_VECTOR( bounds.maximum );
		}
		retVal = TRV_Hit;
	}

	return retVal;
}

ETraceReturnValue CPhysicsWorldPhysXImpl::BoxOverlapWithAnyResult( const Vector& position, const Vector3& halfExtents, const Matrix& orientation, CPhysicsEngine::CollisionMask include, CPhysicsEngine::CollisionMask exclude, SPhysicsOverlapInfo& contactInfo ) 
{
	PC_SCOPE_PHYSICS( Physics_BoxOverlapWithAnyResult );

	PxBoxGeometry geometry( halfExtents.X, halfExtents.Y, halfExtents.Z );

	PxQuat quat = TO_PX_QUAT( orientation.ToQuat() );
	PxTransform transform( TO_PX_VECTOR( position ), quat );
	if( !transform.isValid() ) return TRV_InvalidTransform;

	PxOverlapHit hit;
	Bool blockingHit = false;

	SPhysicalFilterData filter( include, exclude );
	PxSceneQueryFilterData sceneQueryFilterData( filter.m_data, SCENE_QUERY_FILTER_FLAGS );

	Bool wasHit = false;
	ETraceReturnValue retVal = TRV_NoHit;
	SceneUsageAddRef();
	if( !IsSceneWhileProcessing() )
	{
		wasHit = m_scene->overlapAny( geometry, transform, hit, sceneQueryFilterData, &m_singleTraceCallback );
	}
	else
	{
		RED_LOG( PhysicalTrace, TXT( "failed BoxOverlapWithAnyResult processed while scene fetch" ) );
		retVal = TRV_ProcessedWhileFetch;
	}
	SceneUsageRemoveRef();

	if ( wasHit )
	{
		contactInfo.m_userData = static_cast< CPhysicsWrapperInterface* >( hit.actor->userData );
		contactInfo.m_actorNshapeIndex = ( SActorShapeIndex& ) hit.shape->userData;
		contactInfo.m_penetration = 0.f;
		if( contactInfo.m_actorVolume )
		{
			PxBounds3 bounds = hit.actor->getWorldBounds();
			contactInfo.m_actorVolume->Min = TO_VECTOR( bounds.minimum );
			contactInfo.m_actorVolume->Max = TO_VECTOR( bounds.maximum );
		}
		if( contactInfo.m_shapeVolume )
		{
			PxBounds3 bounds = physx::PxShapeExt::getWorldBounds( *hit.shape, *hit.shape->getActor() );
			contactInfo.m_shapeVolume->Min = TO_VECTOR( bounds.minimum );
			contactInfo.m_shapeVolume->Max = TO_VECTOR( bounds.maximum );
		}
		retVal = TRV_Hit;
	}

	return retVal;
}

ETraceReturnValue CPhysicsWorldPhysXImpl::BoxOverlapWithMultipleResults( const Vector& position, const Vector3& halfExtents, const Matrix& orientation, CPhysicsEngine::CollisionMask include, CPhysicsEngine::CollisionMask exclude, SPhysicsOverlapInfo* outContactInfos, Uint32 & outContactsCount, Uint32 maxContactInfos )
{
	PC_SCOPE_PHYSICS( Physics_BoxOverlapWithMultipleResults );

	PxBoxGeometry geometry( halfExtents.X, halfExtents.Y, halfExtents.Z );

	PxQuat quat = TO_PX_QUAT( orientation.ToQuat() );
    PxTransform transform( TO_PX_VECTOR( position ), quat );
	if( !transform.isValid() ) return TRV_InvalidTransform;

    PxOverlapHit* hits = new PxOverlapHit [ maxContactInfos ];

    SPhysicalFilterData filter( include, exclude );
    PxSceneQueryFilterData sceneQueryFilterData( filter.m_data, SCENE_QUERY_FILTER_FLAGS );

	Int32 hitCount = -1;
	SceneUsageAddRef();
	ETraceReturnValue retVal = TRV_NoHit;
	outContactsCount = 0;
	if( !IsSceneWhileProcessing() )
	{
		hitCount = m_scene->overlapMultiple( geometry, transform, hits, maxContactInfos, sceneQueryFilterData, &m_multiTraceCallback );

		// -1 here means we had buffer overflow, but we still return the results
		if( hitCount < 0 )
		{
			outContactsCount = maxContactInfos;
			retVal = TRV_BufferOverflow;
		}
		else if( hitCount > 0 )
		{
			outContactsCount = (Uint32)hitCount;
			retVal = TRV_Hit;
		}
	}
	else
	{
		RED_LOG( PhysicalTrace, TXT( "failed BoxOverlapWithMultipleResults processed while scene fetch" ) );
		retVal = TRV_ProcessedWhileFetch;
	}
	SceneUsageRemoveRef();

	for ( Uint32 i = 0; i < outContactsCount; ++i )
    {
        SPhysicsOverlapInfo& contactInfo = outContactInfos[ i ];
        PxOverlapHit& hit = hits[ i ];

        contactInfo.m_userData = static_cast< CPhysicsWrapperInterface* > ( hit.actor->userData );
        contactInfo.m_actorNshapeIndex = ( SActorShapeIndex& ) hit.shape->userData;
        contactInfo.m_penetration = 0.0f;
		if( contactInfo.m_actorVolume )
		{
			PxBounds3 bounds = hit.actor->getWorldBounds();
			contactInfo.m_actorVolume->Min = TO_VECTOR( bounds.minimum );
			contactInfo.m_actorVolume->Max = TO_VECTOR( bounds.maximum );
		}
		if( contactInfo.m_shapeVolume )
		{
			PxBounds3 bounds = physx::PxShapeExt::getWorldBounds( *hit.shape, *hit.shape->getActor() );
			contactInfo.m_shapeVolume->Min = TO_VECTOR( bounds.minimum );
			contactInfo.m_shapeVolume->Max = TO_VECTOR( bounds.maximum );
		}
    }

    delete [] hits;
    return retVal;
}

ETraceReturnValue CPhysicsWorldPhysXImpl::BoxOverlapWithWithCustomCallbackNoResults( const Vector& position, const Vector3& halfExtents, const Matrix& orientation, CPhysicsEngine::CollisionMask include, CPhysicsEngine::CollisionMask exclude, ICustomTraceCallback * customCallback )
{
	PC_SCOPE_PHYSICS( Physics_SphereSweepTestWithCustomCallbackNoResults );

	PxBoxGeometry geometry( halfExtents.X, halfExtents.Y, halfExtents.Z );

	PxQuat quat = TO_PX_QUAT( orientation.ToQuat() );
	PxTransform transform( TO_PX_VECTOR( position ), quat );
	if( !transform.isValid() ) return TRV_InvalidTransform;

	PxOverlapHit hit;

	Bool blockingHit = false;

	SPhysicalFilterData filter( include, exclude );
	PxSceneQueryFilterData sceneQueryFilterData( filter.m_data, SCENE_QUERY_FILTER_FLAGS );

	SceneUsageAddRef();
	ETraceReturnValue returnValue = TRV_Hit;
	if( !IsSceneWhileProcessing() )
	{
		m_scene->overlapAny( geometry, transform, hit, sceneQueryFilterData, customCallback);
	}
	else
	{
		RED_LOG( PhysicalTrace, TXT( "failed SphereSweepTestWithCustomCallbackNoResults processed while scene fetch" ) );
		returnValue = TRV_ProcessedWhileFetch;
	}
	SceneUsageRemoveRef();

	return returnValue;
}


ETraceReturnValue CPhysicsWorldPhysXImpl::CapsuleOverlapWithAnyResult( const Vector& position, const Float radius, const Float height, CPhysicsEngine::CollisionMask include, CPhysicsEngine::CollisionMask exclude, SPhysicsOverlapInfo& contactInfo )
{
	PC_SCOPE_PHYSICS( Physics_CapsuleOverlapWithAnyResult );

	PxCapsuleGeometry geometry( radius, height*0.5f-radius );
	PxTransform transform( TO_PX_VECTOR( position ), PxQuat( M_PI*0.5f, PxVec3( 0.0f, 1.0f, 0.0f ) ) );
	if( !transform.isValid() ) return TRV_InvalidTransform;

	PxOverlapHit hit;
	Bool blockingHit = false;

	SPhysicalFilterData filter( include, exclude );
	PxSceneQueryFilterData sceneQueryFilterData( filter.m_data, SCENE_QUERY_FILTER_FLAGS );

	Bool wasHit = false;
	ETraceReturnValue retVal = TRV_NoHit;
	SceneUsageAddRef();
	if( !IsSceneWhileProcessing() )
	{
		wasHit = m_scene->overlapAny( geometry, transform, hit, sceneQueryFilterData, &m_singleTraceCallback );
	}
	else
	{
		RED_LOG( PhysicalTrace, TXT( "failed CapsuleOverlapWithAnyResult processed while scene fetch" ) );
		retVal = TRV_ProcessedWhileFetch;
	}
	SceneUsageRemoveRef();

	if ( wasHit )
	{
		contactInfo.m_userData = static_cast< CPhysicsWrapperInterface* >( hit.actor->userData );
		contactInfo.m_actorNshapeIndex = ( SActorShapeIndex& ) hit.shape->userData;
		contactInfo.m_penetration = 0.f;
		if( contactInfo.m_actorVolume )
		{
			PxBounds3 bounds = hit.actor->getWorldBounds();
			contactInfo.m_actorVolume->Min = TO_VECTOR( bounds.minimum );
			contactInfo.m_actorVolume->Max = TO_VECTOR( bounds.maximum );
		}
		if( contactInfo.m_shapeVolume )
		{
			PxBounds3 bounds = physx::PxShapeExt::getWorldBounds( *hit.shape, *hit.shape->getActor() );
			contactInfo.m_shapeVolume->Min = TO_VECTOR( bounds.minimum );
			contactInfo.m_shapeVolume->Max = TO_VECTOR( bounds.maximum );
		}
		retVal = TRV_Hit;
	}

	return retVal;
}

ETraceReturnValue CPhysicsWorldPhysXImpl::SphereOverlapWithMultipleResults( const Vector& position, Float radius, CPhysicsEngine::CollisionMask include, CPhysicsEngine::CollisionMask exclude, SPhysicsOverlapInfo* outContactInfos, Uint32 & outContactsCount, Uint32 maxContactInfos )
{
	PC_SCOPE_PHYSICS( Physics_SphereOverlapWithMultipleResults );

	PxSphereGeometry geometry( radius );
	PxTransform transform( TO_PX_VECTOR( position ), PxQuat::createIdentity() );

	PxOverlapHit* hits = new PxOverlapHit[ maxContactInfos ];

	SPhysicalFilterData filter( include, exclude );
	PxSceneQueryFilterData sceneQueryFilterData( filter.m_data, SCENE_QUERY_FILTER_FLAGS );

	Int32 hitCount = -1;
	SceneUsageAddRef();
	ETraceReturnValue retVal = TRV_NoHit;
	outContactsCount = 0;
	if( !IsSceneWhileProcessing() )
	{
		hitCount = m_scene->overlapMultiple( geometry, transform, hits, maxContactInfos, sceneQueryFilterData, &m_multiTraceCallback );

		// -1 here means we had buffer overflow, but we still return the results
		if( hitCount < 0 )
		{
			outContactsCount = maxContactInfos;
			retVal = TRV_BufferOverflow;
		}
		else if( hitCount > 0 )
		{
			outContactsCount = (Uint32)hitCount;
			retVal = TRV_Hit;
		}
	}
	else
	{
		RED_LOG( PhysicalTrace, TXT( "failed SphereOverlapWithMultipleResults processed while scene fetch" ) );
		retVal = TRV_ProcessedWhileFetch;
	}
	SceneUsageRemoveRef();

	for ( Uint32 i = 0; i < outContactsCount; ++i )
	{
		SPhysicsOverlapInfo& contactInfo = outContactInfos[ i ];
		const PxOverlapHit& hit = hits[ i ];

		contactInfo.m_userData = static_cast< CPhysicsWrapperInterface* > ( hit.actor->userData );
		contactInfo.m_actorNshapeIndex = ( SActorShapeIndex& ) hit.shape->userData;
		contactInfo.m_penetration = TraceUtils::SpherePenetration( geometry, transform, hit.shape, hit.shape->getLocalPose() * hit.actor->getGlobalPose(), contactInfo.m_position ) - radius;
		if ( contactInfo.m_penetration < -radius )
		{
			contactInfo.m_penetration = 0.f;
		}
		if( contactInfo.m_actorVolume )
		{
			PxBounds3 bounds = hit.actor->getWorldBounds();
			contactInfo.m_actorVolume->Min = TO_VECTOR( bounds.minimum );
			contactInfo.m_actorVolume->Max = TO_VECTOR( bounds.maximum );
		}
		if( contactInfo.m_shapeVolume )
		{
			PxBounds3 bounds = physx::PxShapeExt::getWorldBounds( *hit.shape, *hit.shape->getActor() );
			contactInfo.m_shapeVolume->Min = TO_VECTOR( bounds.minimum );
			contactInfo.m_shapeVolume->Max = TO_VECTOR( bounds.maximum );
		}
	}

	delete [] hits;
	return retVal;
}

#endif
