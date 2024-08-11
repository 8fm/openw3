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
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  

#include "PxMemory.h"

#include "SwFactory.h"
#include "SwFabric.h"
#include "SwCloth.h"

#include "CuFactory.h"
#include "CuFabric.h"
#include "CuCloth.h"

#include "ClothImpl.h"
#include "ClothBase.h"

namespace physx
{

	namespace cloth
	{

		namespace 
		{
			template <typename T, typename A>
			Range<T> makeRange(shdfnd::Array<T, A>& vec)
			{
				T* ptr = vec.empty() ? 0 : &vec.front();
				return Range<T>(ptr, ptr + vec.size());
			}

			template <typename VectorType, typename FabricType>
			void findFabric(const VectorType& fabrics, uint32_t id, FabricType*& out)
			{
				out = NULL;
	
				// see if we have a fabric with the same id, if so use that
				VectorType::ConstIterator fIt=fabrics.begin(), fEnd=fabrics.end();
				for (; fIt != fEnd; ++fIt)
					if ((*fIt)->mId == id)
						out = *fIt;
			}

			// fabric conversion functions
			template <typename SrcClothType, typename DstFactoryType>
			typename DstFactoryType::FabricType* convertFabric(
				const SrcClothType& srcFabric, DstFactoryType& dstFactory)
			{
				typedef typename DstFactoryType::FabricType DstFabricType;

				// see if dstFactory already has a Fabric with this id
				
				DstFabricType* dstFabric;
				findFabric(dstFactory.mFabrics, srcFabric.mId, dstFabric);

				// fabric does not exist so create a new one
				if (!dstFabric)
				{
					// todo: allocate as raw arrays to avoid initialization cost
					Vector<uint32_t>::Type phases(srcFabric.getNumPhases());
					Vector<uint32_t>::Type sets(srcFabric.getNumSets());
					Vector<float>::Type restvalues(srcFabric.getNumRestvalues());
					Vector<uint32_t>::Type indices(srcFabric.getNumIndices());
					Vector<uint32_t>::Type anchors(srcFabric.getNumTethers());
					Vector<float>::Type tetherLengths(srcFabric.getNumTethers());

					Range<uint32_t> phaseRange = makeRange(phases);
					Range<float> restvalueRange = makeRange(restvalues);
					Range<uint32_t> setRange = makeRange(sets);
					Range<uint32_t> indexRange = makeRange(indices);
					Range<uint32_t> anchorRange = makeRange(anchors);
					Range<float> lengthRange = makeRange(tetherLengths);

					srcFabric.mFactory.extractFabricData(srcFabric, phaseRange, 
						setRange, restvalueRange, indexRange, anchorRange, lengthRange);

					dstFabric = static_cast<DstFabricType*>(dstFactory.createFabric(
						srcFabric.mNumParticles, phaseRange, setRange, 
						restvalueRange, indexRange, anchorRange, lengthRange));

					// give new fabric the same id as the source so it can be matched
					dstFabric->mId = srcFabric.mId;
				}

				return dstFabric;
			}

			void convertPhaseConfigs(const SwCloth& src, CuCloth& dst)
			{
				dst.setPhaseConfig( Range<const PhaseConfig>(
					src.mPhaseConfigs.begin(), src.mPhaseConfigs.end()) );
			}

			void convertPhaseConfigs(const CuCloth& src, SwCloth& dst)
			{
				dst.mPhaseConfigs = src.mHostPhaseConfigs;
			}

			void convertPhaseConfigs(const CuCloth& src, CuCloth& dst)
			{
				dst.setPhaseConfig( Range<const PhaseConfig>(
					src.mHostPhaseConfigs.begin(), src.mHostPhaseConfigs.end()) );
			}

			template <typename T> struct HostAccess {};

			template <> struct HostAccess<SwClothImpl> 
			{
				HostAccess(const Factory&, Range<const PxVec4> range) 
					: mRange(range) {}
				HostAccess(const Factory&, const Vec4fAlignedVector& vector) 
					: mRange(&vector.front(), &vector.front() + vector.size()) {}
				Range<const PxVec4> range() const { return mRange; }
				Range<const PxVec4> mRange;
			};

			template <> struct HostAccess<CuClothImpl> 
			{
				HostAccess(const Factory& factory, Range<const PxVec4> range) 
					: mHostCopy(range.size())
				{
					factory.copyToHost(range.begin(), range.end(), &mHostCopy.front());
				}
				HostAccess(const Factory& factory, const CuDeviceVector<PxVec4>& vector) 
					: mHostCopy(uint32_t(vector.size()),PxVec4(0.0f))
				{
					factory.copyToHost(vector.begin().get(), vector.end().get(), &mHostCopy.front());
				}
				Range<const PxVec4> range() const 
				{ 
					return Range<const PxVec4>(&mHostCopy.front(), 
						&mHostCopy.front() + mHostCopy.size()); 
				}
				Vec4fAlignedVector mHostCopy;
			};

			void convertSelfCollisionIndices(const SwCloth& src, CuCloth& dst)
			{
				dst.mSelfCollisionIndices = src.mSelfCollisionIndices;
			}

			void convertSelfCollisionIndices(const CuCloth& src, SwCloth& dst)
			{
				dst.mSelfCollisionIndices.resize(uint32_t(src.mSelfCollisionIndices.size()));
				src.mFactory.copyToHost(src.mSelfCollisionIndices.begin().get(), 
					src.mSelfCollisionIndices.end().get(), dst.mSelfCollisionIndices.begin());
			}

			void convertSelfCollisionIndices(const CuCloth& src, CuCloth& dst)
			{
				dst.mSelfCollisionIndices = src.mSelfCollisionIndices;
			}

			// cloth conversion
			template <typename DstFactoryType, typename SrcImplType>
			typename DstFactoryType::ImplType* convertCloth(
				DstFactoryType& dstFactory, const SrcImplType& srcImpl)
			{
				typedef typename DstFactoryType::FabricType DstFabricType;
				typedef typename DstFactoryType::ImplType DstImplType;
				typedef typename DstImplType::ClothType DstClothType;
				typedef typename SrcImplType::ClothType SrcClothType;
				typedef HostAccess<SrcImplType> SrcAccessType;

				const SrcClothType& srcCloth = srcImpl.mCloth;
				const Factory& srcFactory = srcCloth.mFactory;

				DstClothType::ContextLockType lock(dstFactory);

				// particles
				Range<const PxVec4> curParticles = srcImpl.getCurrentParticles(Cloth::HOST_MEMORY);

				// fabric
				DstFabricType& dstFabric = *convertFabric(srcCloth.mFabric, dstFactory);

				// create new cloth
				DstImplType* dstImpl = static_cast<DstImplType*>(
					dstFactory.createCloth(curParticles, dstFabric));
				DstClothType& dstCloth = dstImpl->mCloth;

				// copy across common parameters
				copy(dstCloth, srcCloth);

				// copy across previous particles (todo: avoid buffer)
				Range<const PxVec4> prevParticles = srcImpl.getPreviousParticles(Cloth::HOST_MEMORY);
				dstFactory.copyFromHost(prevParticles.begin(), prevParticles.end(), 
					dstImpl->getPreviousParticles().begin());

				// copy across phase configs
				convertPhaseConfigs(srcCloth, dstCloth);

				// collision data
				Vector<PxVec4>::Type spheres(srcImpl.getNumSpheres(), PxVec4(0.0f));
				PxVec4* spherePtr = spheres.empty() ? 0 : &spheres.front();
				Range<PxVec4> sphereRange(spherePtr, spherePtr+spheres.size());
				Vector<uint32_t>::Type capsules(srcImpl.getNumCapsules()*2);
				Range<uint32_t> capsuleRange = makeRange(capsules);
				Vector<PxVec4>::Type planes(srcImpl.getNumPlanes(), PxVec4(0.0f));
				PxVec4* planePtr = planes.empty() ? 0 : &planes.front();
				Range<PxVec4> planeRange(planePtr, planePtr+planes.size());
				Vector<uint32_t>::Type convexes(srcImpl.getNumConvexes());
				Range<uint32_t> convexRange = makeRange(convexes);
				Vector<PxVec3>::Type triangles(srcImpl.getNumTriangles()*3, PxVec3(0.0f));
				PxVec3* trianglePtr = triangles.empty() ? 0 : &triangles.front();
				Range<PxVec3> triangleRange(trianglePtr, trianglePtr+triangles.size());

				srcFactory.extractCollisionData(srcImpl, sphereRange, 
					capsuleRange, planeRange, convexRange, triangleRange);
				dstImpl->setSpheres(sphereRange, 0, 0);
				dstImpl->setCapsules(capsuleRange, 0, 0);
				dstImpl->setPlanes(planeRange, 0, 0);
				dstImpl->setConvexes(convexRange, 0, 0);
				dstImpl->setTriangles(triangleRange, 0, 0);

				// motion constraints, copy directly into new cloth buffer
				if (srcImpl.getNumMotionConstraints())
					srcFactory.extractMotionConstraints(srcImpl, dstImpl->getMotionConstraints());

				// separation constraints, copy directly into new cloth buffer
				if (srcImpl.getNumSeparationConstraints())
					srcFactory.extractSeparationConstraints(srcImpl, dstImpl->getSeparationConstraints());

				// particle accelerations
				if (srcImpl.getNumParticleAccelerations())
				{
					SrcAccessType accelerations(srcFactory, srcCloth.mParticleAccelerations);
					PxMemCopy(dstImpl->getParticleAccelerations().begin(),
						accelerations.range().begin(), accelerations.range().size() * sizeof(PxVec4));
				}

				// self-collision indices
				convertSelfCollisionIndices(srcCloth, dstCloth);

				// rest positions
				if (srcImpl.getNumRestPositions())
				{
					SrcAccessType restPositions(srcFactory, srcCloth.mRestPositions);
					dstFactory.copyFromHost(restPositions.range().begin(), restPositions.range().end(),
						dstImpl->getRestPositions().begin());
				}

				// virtual particles
				if (srcImpl.getNumVirtualParticles())
				{	
					// todo: allocate as raw arrays to avoid initialization cost
					Vector<Vec4u>::Type indices(srcImpl.getNumVirtualParticles());
					Vector<PxVec3>::Type weights(srcImpl.getNumVirtualParticleWeights(), PxVec3(0.0f));		

					uint32_t (*indicesPtr)[4] = indices.empty()?0:&array(indices.front());
					Range<uint32_t[4]> indicesRange(indicesPtr, indicesPtr+indices.size());

					PxVec3* weightsPtr = weights.empty()?0:&weights.front();
					Range<PxVec3> weightsRange(weightsPtr, weightsPtr+weights.size());

					srcFactory.extractVirtualParticles(srcImpl, indicesRange, weightsRange);

					dstImpl->setVirtualParticles(indicesRange, weightsRange);
				}

				return dstImpl;
			}

		} // anonymous namespace

		template <>
		Cloth* ClothImpl<CuCloth>::clone(Factory& factory) const
		{
			if (factory.getPlatform() == Factory::CPU)
				// create a SwCloth from a CuCloth
				return convertCloth(static_cast<SwFactory&>(factory), *this);

			// if cloth doesn't share this factory's cuda context
			// then we must go through host memory; we can reuse the 
			// conversion routine for this 
			if (&mCloth.mFactory != &factory)
				return convertCloth(static_cast<CuFactory&>(factory), *this);

			// otherwise copy construct directly
			return new CuClothImpl(factory, *this);

		}

		Cloth* CuFactory::clone(const Cloth& cloth)
		{
			if (cloth.getFactory().getPlatform() == Factory::CPU)
				// create a CuCloth from a SwCloth
				return convertCloth(*this, static_cast<const SwClothImpl&>(cloth));

			// forward to CuCloth
			return cloth.clone(*this);
		}

	} // namespace cloth

} // namespace physx


