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

#ifndef OPC_COLLIDER_H
#define OPC_COLLIDER_H

#include "OPC_ModelData.h"

namespace physx
{
namespace Gu
{
	enum CollisionFlag
	{
		OPC_FIRST_CONTACT			= (1<<0),		//!< Report all contacts (false) or only first one (true)
		OPC_CONTACT					= (1<<2),		//!< Final contact status after a collision query
		OPC_NO_PRIMITIVE_TESTS		= (1<<4),		//!< Keep or discard primitive-bv tests in leaf nodes (volume-mesh queries)
		OPC_LOOSE_PRIMITIVE_TESTS	= (1<<5),		//!< Perform approximate primitive tests, but may return some non colliding tris

		OPC_CONTACT_FOUND			= OPC_FIRST_CONTACT | OPC_CONTACT,

		OPC_FORCE_DWORD				= 0x7fffffff
	};

	class PX_PHYSX_COMMON_API Collider
	{
		public:
		PX_FORCE_INLINE						Collider() : mFlags(0),	mCurrentModel(NULL), mIMesh(NULL)	{}

		// Collision report

		// Settings

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/**
		 *	Enable/disable primitive tests.
		 *	\param		flag		[in] true to enable primitive tests, false to discard them
		 */
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		PX_FORCE_INLINE				void		SetPrimitiveTests(bool flag)
											{
												if(!flag)	mFlags |= OPC_NO_PRIMITIVE_TESTS;
												else		mFlags &= ~OPC_NO_PRIMITIVE_TESTS;
											}
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/**
		 *	Enable/disable loose primitive tests(ie Opcode may return non intersecting triangles in some cases).
		 *	\param		flag		[in] true to enable primitive tests, false to discard them
		 */
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		PX_FORCE_INLINE				void		SetLoosePrimitiveTests(bool flag)
											{
												if(flag)	mFlags |= OPC_LOOSE_PRIMITIVE_TESTS;
												else		mFlags &= ~OPC_LOOSE_PRIMITIVE_TESTS;
											}

							PxU32				mFlags;			//!< Bit flags
					const	RTreeMidphaseData*	mCurrentModel;	//!< Current model for collision query (owner of touched faces)
		// User mesh interface
					const	MeshInterface*		mIMesh;			//!< User-defined mesh interface

		// Internal methods
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/**
		 *	Setups current collision model
		 *	\param		model	[in] current collision model
		 *	\return		Ps::IntTrue if success
		 */
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		PX_FORCE_INLINE				Ps::IntBool	Setup(const RTreeMidphaseData* model)
											{
												// Keep track of current model
												mCurrentModel = model;
												if(!mCurrentModel)
													return Ps::IntFalse;

//												mIMesh = model->GetMeshInterface();
												mIMesh = model->mIMesh;
												return mIMesh!=NULL;
											}
	};
} // namespace Gu

}

#endif // OPC_COLLIDER_H
