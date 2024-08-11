// This code contains NVIDIA Confidential Information and is disclosed to you
// under a form of NVIDIA software license agreement provided separately to you.
//
// Notice
// NVIDIA Corporation and its licensors retain all intellectual property and
// proprietary rights in and to this software and related documentation and
// any modifications thereto. Any use, reproduction, disclosure, or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA Corporation is strictly prohibited.
//
// ALL NVIDIA DESIGN SPECIFICATIONS, CODE ARE PROVIDED "AS IS.". NVIDIA MAKES
// NO WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ALL IMPLIED WARRANTIES OF NONINFRINGEMENT,
// MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Information and code furnished is believed to be accurate and reliable.
// However, NVIDIA Corporation assumes no responsibility for the consequences of use of such
// information or for any infringement of patents or other rights of third parties that may
// result from its use. No license is granted by implication or otherwise under any patent
// or patent rights of NVIDIA Corporation. Details are subject to change without notice.
// This code supersedes and replaces all information previously supplied.
// NVIDIA Corporation products are not authorized for use as critical
// components in life support devices or systems without express written approval of
// NVIDIA Corporation.
//
// Copyright (c) 2008-2013 NVIDIA Corporation. All rights reserved.

#ifndef __BASIC_IOS_COMMON_SRC_H__
#define __BASIC_IOS_COMMON_SRC_H__

namespace physx
{
	namespace apex
	{
		namespace basicios
		{

			PX_CUDA_CALLABLE PX_INLINE void updateCollisionVelocity(const CollisionData& data, const physx::PxVec3& normal, const physx::PxVec3& position, physx::PxVec3& velocity)
			{
				physx::PxVec3 bodyVelocity = data.bodyLinearVelocity + data.bodyAngluarVelocity.cross(position - data.bodyCMassPosition);

				velocity -= bodyVelocity;
				physx::PxF32 normalVelocity = normal.dot(velocity);
				if (normalVelocity < 0.0f)
				{
					velocity -= normal * ((1.0f + data.materialRestitution) * normalVelocity);
				}
				velocity += bodyVelocity;
			}


			PX_CUDA_CALLABLE PX_INLINE void calculatePointSegmentSquaredDist(const PxVec3& a, const PxVec3& b, const PxVec3& point, PxF32& distanceSquared, PxVec3& nearestPoint) // a, b - segment points
			{
				PxVec3 v, w, temp; //vectors
				PxF32 c1, c2, ratio; //constants
				v = a - b;
				w = point - b;
				PxF32 distSquared = 0;

				c1 = w.dot(v);
				if(c1 <= 0) 
				{
					distSquared = (point.x - b.x) *  (point.x - b.x) + (point.y - b.y) * (point.y - b.y) + (point.z - b.z) * (point.z - b.z);
					nearestPoint = b;
				}
				else
				{
					c2 = v.dot(v);
					if(c2 <= c1)
					{
						distSquared = (point.x - a.x) *  (point.x - a.x) + (point.y - a.y) * (point.y - a.y) + (point.z - a.z) * (point.z - a.z);
						nearestPoint = a;
					}
					else
					{
						ratio = c1 / c2;
						temp = b + ratio * v;
						distSquared = (point.x - temp.x) *  (point.x - temp.x) + (point.y - temp.y) * (point.y - temp.y) + (point.z - temp.z) * (point.z - temp.z);
						nearestPoint = temp;
					}
				}

				distanceSquared = distSquared;
			}

#ifdef __CUDACC__
			__device__
#endif
			PX_INLINE PxF32 checkTriangleCollision(const PxVec4* memTrimeshVerts, const PxU32* memTrimeshIndices, const CollisionTriMeshData& data, PxF32 radius, PxVec3 localPosition, PxVec3 &normal)
			{
				PxF32 minDistSquared = PX_MAX_F32;
				PxVec3 localNormal(0);
					
				for(PxU32 j = 0 ; j < data.numTriangles; j++)
				{
					PxVec3 p0, p1, p2;
					physx::PxU32 i0, i1, i2;
					i0 = SIM_FETCH(TrimeshIndices, data.firstIndex + 3 * j);
					i1 = SIM_FETCH(TrimeshIndices, data.firstIndex + 3 * j + 1);
					i2 = SIM_FETCH(TrimeshIndices, data.firstIndex + 3 * j + 2);
					splitFloat4(p0, SIM_FETCH(TrimeshVerts, data.firstVertex + i0));
					splitFloat4(p1, SIM_FETCH(TrimeshVerts, data.firstVertex + i1));
					splitFloat4(p2, SIM_FETCH(TrimeshVerts, data.firstVertex + i2));

					//localNormal = (p1 - p0).cross(p2 - p0);
					//if(radius > 0) localNormal += localPosition;
						
					PxBounds3 aabb( (p0.minimum(p1.minimum(p2))), (p0.maximum(p1.maximum(p2))) );
					aabb.fattenFast( radius );
					if( !aabb.contains(localPosition) ) continue;

					p0 = p0 - localPosition;
					p1 = p1 - localPosition;
					p2 = p2 - localPosition;

					PxVec3 a(p1 - p0);
					PxVec3 b(p2 - p0);
					PxVec3 n = a.cross(b);	
					n.normalize();

					//check if point far away from the triangle's plane, then give up
					if(n.x * p0.x + n.y * p0.y + n.z * p0.z > radius) continue;

					//check if the nearest point is one of the triangle's vertices
					PxVec3 closestPoint; // closest point

					PxReal det1p0p1, det2p0p2, det2p1p2, det0p0p1, det0p0p2, det1p1p2;
					//i = 0
					det1p0p1 = p0.dot(-(p1 - p0)); 
					det2p0p2 = p0.dot(-(p2 - p0));
					//i = 1
					det0p0p1 = p1.dot(p1 - p0);
					det2p1p2 = p1.dot(-(p2 - p1));
					//i = 2
					det0p0p2 = p2.dot(p2 - p0);
					det1p1p2 = p2.dot(p2 - p1);

					if(det1p0p1 <= 0 && det2p0p2 <= 0) closestPoint = p0;
					else if(det0p0p1 <= 0 && det2p1p2 <= 0) closestPoint = p1;
					else if(det0p0p2 <= 0 && det1p1p2 <= 0) closestPoint = p2;
					else 
					{
						//check if the nearest point is internal point of one of the triangle's edges
						PxReal det0p0p1p2, det1p0p1p2, det2p0p1p2;
						det0p0p1p2 = det0p0p1 * det1p1p2 + det2p1p2 * p2.dot(p1 - p0);
						det1p0p1p2 = det1p0p1 * det0p0p2 - det2p0p2 * p2.dot(p1 - p0);
						det2p0p1p2 = det2p0p2 * det0p0p1 - det1p0p1 * p1.dot(p2 - p0);

						if(det0p0p1p2 <= 0) closestPoint = (p1 * det1p1p2 + p2 * det2p1p2) / (det1p1p2 + det2p1p2);
						else if(det1p0p1p2 <= 0) closestPoint = (p0 * det0p0p2 + p2 * det2p0p2) / (det0p0p2 + det2p0p2);
						else if(det2p0p1p2 <= 0) closestPoint = (p0 * det0p0p1 + p1 * det1p0p1) / (det0p0p1 + det1p0p1);
						//point is inside the triangle
						else closestPoint = (p0 * det0p0p1p2 + p1 * det1p0p1p2 + p2 * det2p0p1p2) / (det0p0p1p2 + det1p0p1p2 + det2p0p1p2);
					}

					PxF32 distSquared = closestPoint.x * closestPoint.x + closestPoint.y * closestPoint.y + closestPoint.z * closestPoint.z;
					if(distSquared > radius * radius)
					{
						continue;
					}

					if(distSquared < minDistSquared)
					{
						minDistSquared = distSquared;
						localNormal = n;
					}
				}
				normal = localNormal;

				return minDistSquared;
			}

#ifdef __CUDACC__
			__device__
#endif
			PX_INLINE physx::PxU32 handleCollisions(const SimulationParams* params, SIM_MEM_TYPE simMemory, physx::PxVec3& position, physx::PxVec3& velocity, physx::PxVec3& normal)
			{
				const PxPlane* memConvexPlanes = params->convexPlanes;
				const PxVec4*  memConvexVerts = params->convexVerts;
				const PxU32*   memConvexPolygonsData = params->convexPolygonsData;

				// Algorithms are similar to CPU version
				const PxVec4*  memTrimeshVerts = params->trimeshVerts;
				const PxU32*   memTrimeshIndices = params->trimeshIndices;

				PxF32 collisionRadius = params->collisionDistance + params->collisionThreshold;

				int numTriMeshes = params->trimeshes.getSize();
				for (int i = 0; i < numTriMeshes; ++i)
				{
					const CollisionTriMeshData& data = params->trimeshes.getElems(simMemory)[i];

					if (!data.aabb.contains(position))		//check coarse bounds
					{
						continue;
					}

					physx::PxVec3 localPosition = data.inversePose * position;
									
					PxVec3 localNormal;
					PxF32 minDistSquared = checkTriangleCollision(memTrimeshVerts, memTrimeshIndices, data, collisionRadius, localPosition, localNormal);
					if (minDistSquared == PX_MAX_F32)
					{
						continue;
					}

					float penDepth = params->collisionDistance - physx::PxSqrt(minDistSquared);

					if( penDepth > 0 )
					{
						localPosition += localNormal * penDepth;
						normal = data.pose.M * localNormal;
						position = data.pose * localPosition;
						updateCollisionVelocity(data, normal, position, velocity);
					}
					return 1;
				}

				int numConvexMeshes = params->convexMeshes.getSize();
				for (int i = 0; i < numConvexMeshes; ++i)
				{
					const CollisionConvexMeshData& data = params->convexMeshes.getElems(simMemory)[i];

					if (!data.aabb.contains(position))		//check coarse bounds
					{
						continue;
					}

					physx::PxVec3 localPosition = data.inversePose * position;

					physx::PxF32 penDepth = PX_MAX_F32;
					PxVec3 localNormal(0);

					bool insideConvex = true;
					bool insidePolygon = true;
					PxF32 distSquaredMin = PX_MAX_F32;
					PxVec3 nearestPointMin;

					PxU32 polygonsDataOffset = data.polygonsDataOffset;
					for (PxU32 polyId = 0; polyId < data.numPolygons; polyId++) // for each polygon
					{		
						PxPlane plane;
						SIM_FETCH_PLANE(plane, ConvexPlanes, data.firstPlane + polyId);

						PxU32 vertCount = SIM_FETCH(ConvexPolygonsData, polygonsDataOffset);

						physx::PxF32 dist = (localPosition.dot(plane.n) + plane.d);
						if (dist > 0) //outside convex
						{
							insideConvex = false;

							if (dist > collisionRadius)
							{
								insidePolygon = false;
								distSquaredMin = dist * dist;
								break;
							}

							insidePolygon = true;
							PxVec3 polygonNormal = plane.n;

							PxU32 begVertId = SIM_FETCH(ConvexPolygonsData, polygonsDataOffset + vertCount);
							PxVec3 begVert; splitFloat4(begVert, SIM_FETCH(ConvexVerts, data.firstVertex + begVertId));
							for (PxU32 vertId = 1; vertId <= vertCount; ++vertId) //for each vertex
							{
								PxU32 endVertId = SIM_FETCH(ConvexPolygonsData, polygonsDataOffset + vertId);
								PxVec3 endVert; splitFloat4(endVert, SIM_FETCH(ConvexVerts, data.firstVertex + endVertId));

								PxVec3 segment = endVert - begVert;
								PxVec3 segmentNormal = polygonNormal.cross(segment);
								PxF32  sign = segmentNormal.dot(localPosition - begVert);
								if (sign < 0)
								{
									insidePolygon = false;

									PxF32 distSquared;
									PxVec3 nearestPoint;
									calculatePointSegmentSquaredDist(begVert, endVert, localPosition, distSquared, nearestPoint);
									if (distSquared < distSquaredMin)
									{
										distSquaredMin = distSquared;
										nearestPointMin = nearestPoint;
									}
								}

								begVert = endVert;
							}
							if (insidePolygon)
							{
								penDepth = params->collisionDistance - dist;
								localNormal = polygonNormal;
								break;
							}
						}

						if (insideConvex)
						{
							physx::PxF32 penDepthPlane = params->collisionDistance - dist; //dist is negative inside
							if (penDepthPlane < penDepth) //inside convex 
							{
								penDepth = penDepthPlane; 
								localNormal = plane.n;
							}
						}
						polygonsDataOffset += (vertCount + 1);
					}

					if (!insideConvex && !insidePolygon)
					{
						if (distSquaredMin > collisionRadius * collisionRadius)
						{
							continue; //no intersection, too far away
						}
						physx::PxF32 dist = physx::PxSqrt(distSquaredMin);

						localNormal = localPosition - nearestPointMin;
						localNormal *= (1 / dist); //normalize
						
						penDepth = params->collisionDistance - dist;
					}

					if (penDepth > 0)
					{
						localPosition += localNormal * penDepth;
						normal = data.pose.M * localNormal;
						position = data.pose * localPosition;
						updateCollisionVelocity(data, normal, position, velocity);
					}
					return 1;
				}

				int numBoxes = params->boxes.getSize();
				for (int i = 0; i < numBoxes; ++i)
				{
					const CollisionBoxData& data = params->boxes.getElems(simMemory)[i];
					 

					if (!data.aabb.contains(position))
					{
						continue;
					}

					physx::PxVec3 localPosition = data.inversePose * position;


					physx::PxVec3 closestPoint = PxVec3(physx::PxClamp(localPosition.x, -data.halfSize.x, data.halfSize.x), physx::PxClamp(localPosition.y, -data.halfSize.y, data.halfSize.y), physx::PxClamp(localPosition.z, -data.halfSize.z, data.halfSize.z));
					physx::PxVec3 v = localPosition - closestPoint;
					physx::PxF32 vMagnitudeSquared = v.magnitudeSquared();
					if(vMagnitudeSquared > collisionRadius * collisionRadius) continue; //no intersection

					physx::PxBounds3 bounds = PxBounds3(-data.halfSize, data.halfSize);
					physx::PxF32 penDepth;

					physx::PxVec3 localNormal(0);
					if(vMagnitudeSquared > 0)
					{
						physx::PxF32 vMagnitude = physx::PxSqrt(vMagnitudeSquared);
						localNormal = v * (1 / vMagnitude);

						penDepth = params->collisionDistance - vMagnitude;
					}
					else
					{
						physx::PxVec3 penDepth3D = physx::PxVec3(
							data.halfSize.x - physx::PxAbs(localPosition.x),
							data.halfSize.y - physx::PxAbs(localPosition.y),
							data.halfSize.z - physx::PxAbs(localPosition.z)
							);
						physx::PxF32 penDepth3Dmin = penDepth3D.minElement();

						if (penDepth3Dmin == penDepth3D.x)
						{
							localNormal.x = localPosition.x < 0 ? -1.0f : 1.0f;
						}
						else if (penDepth3Dmin == penDepth3D.y)
						{
							localNormal.y = localPosition.y < 0 ? -1.0f : 1.0f;
						}
						else if (penDepth3Dmin == penDepth3D.z)
						{
							localNormal.z = localPosition.z < 0 ? -1.0f : 1.0f;
						}

						penDepth = params->collisionDistance + penDepth3Dmin;
					}

					normal = data.pose.M * localNormal;

					if (penDepth > 0)
					{
						localPosition += localNormal * penDepth;
						position = data.pose * localPosition;

						updateCollisionVelocity(data, normal, position, velocity);
					}
					return 1;
				}

				int numCapsules = params->capsules.getSize();
				for (int i = 0; i < numCapsules; ++i)
				{
					const CollisionCapsuleData& data = params->capsules.getElems(simMemory)[i];

					if (!data.aabb.contains(position))
					{
						continue;
					}

					physx::PxVec3 localPosition = data.inversePose * position;

					// Capsule is Minkowski sum of sphere with segment
					physx::PxF32 closestY = physx::PxClamp(localPosition.y, -data.halfHeight, data.halfHeight);
					physx::PxVec3 localNormal(localPosition.x, localPosition.y - closestY, localPosition.z);

					physx::PxF32 distance = localNormal.magnitude();
					physx::PxF32 penDepth = (data.radius - distance);
					// No intersection?
					if (-penDepth > params->collisionThreshold)
					{
						continue;
					}
					localNormal /= distance;
					normal = data.pose.M * localNormal;

					if (penDepth > 0)
					{
						localPosition = data.radius * localNormal;
						localPosition.y += closestY;

						position = data.pose * localPosition;

						updateCollisionVelocity(data, normal, position, velocity);
					}
					return 1;
				}

				int numSpheres = params->spheres.getSize();
				for (int i = 0; i < numSpheres; ++i)
				{
					const CollisionSphereData& data = params->spheres.getElems(simMemory)[i];

					if (!data.aabb.contains(position))
					{
						continue;
					}

					physx::PxVec3 localNormal = data.inversePose * position;
					physx::PxF32 distance = localNormal.magnitude();

					physx::PxF32 penDepth = (data.radius - distance);
					// No intersection?
					if (-penDepth > params->collisionThreshold)
					{
						continue;
					}
					localNormal /= distance;
					normal = data.pose.M * localNormal;

					if (penDepth > 0)
					{
						position = data.pose * (data.radius * localNormal);

						updateCollisionVelocity(data, normal, position, velocity);
					}
					return 1;
				}

				int numHalfSpaces = params->halfSpaces.getSize(); 
				for (int i = 0; i < numHalfSpaces; ++i)
				{
					const CollisionHalfSpaceData& data = params->halfSpaces.getElems(simMemory)[i];

					physx::PxF32 penDepth = (data.origin - position).dot(data.normal);

					// No intersection?
					if (-penDepth > params->collisionThreshold)
					{
						continue;
					}

					normal = data.normal;
					if (penDepth > 0)
					{
						position += penDepth * data.normal;

						updateCollisionVelocity(data, normal, position, velocity);
					}
					return 1;
				}

				return 0;
			}



#ifdef __CUDACC__
			__device__
#endif
			PX_INLINE physx::PxF32 calcParticleBenefit(
				const InjectorParams& inj, const physx::PxVec3& eyePos,
				const physx::PxVec3& pos, const physx::PxVec3& vel, physx::PxF32 life)
			{
				physx::PxF32 benefit = inj.mLODBias;
				//distance term
				physx::PxF32 distance = (eyePos - pos).magnitude();
				if (physx::PxIsFinite(distance))
				{
					benefit += inj.mLODDistanceWeight * (1.0f - physx::PxMin(1.0f, distance / inj.mLODMaxDistance));
				}
				//velocity term, TODO: clamp velocity
				physx::PxF32 velMag = vel.magnitude();
				if (physx::PxIsFinite(velMag))
				{
					benefit += inj.mLODSpeedWeight * velMag;
				}
				//life term
				benefit += inj.mLODLifeWeight * life;

				return physx::PxClamp(benefit, 0.0f, 1.0f);
			}

			template <typename FieldAccessor>
#ifdef __CUDACC__
			__device__
#endif
			PX_INLINE float simulateParticle(
				const SimulationParams* params, SIM_MEM_TYPE simMemory, const InjectorParams* injectorParamsList,
				float deltaTime, physx::PxVec3 gravity, physx::PxVec3 eyePos,
				bool isNewParticle, unsigned int srcIdx, unsigned int dstIdx,
				SIM_FLOAT4* memPositionMass, SIM_FLOAT4* memVelocityLife, NiIofxActorID* memIofxActorIDs,
				float* memLifeSpan, float* memLifeTime, unsigned int* memInjector, SIM_FLOAT4* memCollisionNormalFlags, physx::PxU32* memUserData,
				FieldAccessor& fieldAccessor, unsigned int &injIndex
				)
			{
				//read
				physx::PxVec3 position;
				physx::PxF32 mass = splitFloat4(position, SIM_FETCH(PositionMass, srcIdx));

				physx::PxVec3 velocity;

				splitFloat4(velocity, SIM_FETCH(VelocityLife, srcIdx));

				physx::PxF32 lifeSpan = SIM_FETCH(LifeSpan, srcIdx);

				unsigned int injector = SIM_FETCH(Injector, srcIdx);
				const InjectorParams& injParams = injectorParamsList[injector];

				injIndex = injParams.mLocalIndex;

				NiIofxActorID iofxActorID = NiIofxActorID(SIM_FETCH(IofxActorIDs, srcIdx));
				// injParams.mLODBias == FLT_MAX if injector was released!
				// and IOFX returns NiIofxActorID::NO_VOLUME for homeless/dead particles
				bool validActorID = (injParams.mLODBias < FLT_MAX) && (isNewParticle || (iofxActorID.getVolumeID() != NiIofxActorID::NO_VOLUME));
				if (!validActorID)
				{
					iofxActorID.setActorClassID(NiIofxActorID::INV_ACTOR);
					injIndex = PX_MAX_U32;
				}

				physx::PxVec3 collisionNormal(0.0f);
				physx::PxU32 collisionFlags = 0;

				physx::PxF32 lifeTime = lifeSpan;
				if (!isNewParticle)
				{
					lifeTime = SIM_FETCH(LifeTime, srcIdx);

					//collide using the old state
					collisionFlags = handleCollisions(params, simMemory, position, velocity, collisionNormal);

					//advance to a new state
					physx::PxVec3 velocityDelta = deltaTime * gravity;
					fieldAccessor(srcIdx, velocityDelta);

					velocity += velocityDelta;
					position += deltaTime * velocity;

					lifeTime = physx::PxMax(lifeTime - deltaTime, 0.0f);
				}
				//write
				memLifeTime[dstIdx] = lifeTime;

				if (!isNewParticle || dstIdx != srcIdx)
				{
					memPositionMass[dstIdx] = combineFloat4(position, mass);
					memVelocityLife[dstIdx] = combineFloat4(velocity, lifeTime / lifeSpan);
				}
				if (!validActorID || dstIdx != srcIdx)
				{
					memIofxActorIDs[dstIdx] = iofxActorID;
				}
				if (dstIdx != srcIdx)
				{
					memLifeSpan[dstIdx] = lifeSpan;
					memInjector[dstIdx] = injector;

					memUserData[dstIdx] = SIM_FETCH(UserData, srcIdx);
				}
				memCollisionNormalFlags[dstIdx] = combineFloat4(collisionNormal, SIM_INT_AS_FLOAT(collisionFlags));

				float benefit = -FLT_MAX;
				if (validActorID && lifeTime > 0.0f)
				{
					benefit = calcParticleBenefit(injParams, eyePos, position, velocity, lifeTime / lifeSpan);
				}
				return benefit;
			}
		}
	}

} // namespace physx::apex

#endif
