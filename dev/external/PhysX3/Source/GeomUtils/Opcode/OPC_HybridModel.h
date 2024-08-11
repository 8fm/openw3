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

#ifndef OPC_HYBRIDMODEL_H
#define OPC_HYBRIDMODEL_H

#include "Opcode.h"

#include "PsUserAllocated.h"
#include "OPC_TreeBuilders.h"	// PT: just for build settings! Split header!
#include "OPC_ModelData.h"
#include "GuRTree.h"
#include "PxMetaData.h"

namespace physx
{
class PxInputStream;
namespace Gu
{

	class PX_PHYSX_COMMON_API RTreeMidphase : public Ps::UserAllocated
	{
		public:
// PX_SERIALIZATION
												RTreeMidphase(const PxEMPTY&) : mRTree(PxEmpty)
												{
												}
		static			void					getBinaryMetaData(PxOutputStream& stream);
//~PX_SERIALIZATION
												RTreeMidphase();
												~RTreeMidphase();

		PX_FORCE_INLINE	void					getRTreeMidphaseData(RTreeMidphaseData& data)	const
												{
													data.mIMesh			= mIMesh;
													data.mRTree			= &mRTree;
												}

		PX_FORCE_INLINE	const MeshInterface*	GetMeshInterface()						const	{ return mIMesh;		}
		PX_FORCE_INLINE	void					SetMeshInterface(const MeshInterface* imesh)	{ mIMesh = imesh;		}

// PX_SERIALIZATION
						void					exportExtraData(PxSerializationContext&);
						void					importExtraData(PxDeserializationContext& context);
//~PX_SERIALIZATION

		protected:
						const MeshInterface*	mIMesh;			//!< User-defined mesh interface
		public:
						PxReal					mGeomEpsilon;	//!< see comments in cooking code referencing this variable
						Gu::RTree				mRTree;
	};

} // namespace Gu

}

#endif // OPC_HYBRIDMODEL_H
