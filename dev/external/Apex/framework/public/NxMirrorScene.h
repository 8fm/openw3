/*
 * Copyright 2009-2010 NVIDIA Corporation.  All rights reserved.
 *
 * NOTICE TO USER:
 *
 * This source code is subject to NVIDIA ownership rights under U.S. and
 * international Copyright laws.  Users and possessors of this source code
 * are hereby granted a nonexclusive, royalty-free license to use this code
 * in individual and commercial software.
 *
 * NVIDIA MAKES NO REPRESENTATION ABOUT THE SUITABILITY OF THIS SOURCE
 * CODE FOR ANY PURPOSE.  IT IS PROVIDED "AS IS" WITHOUT EXPRESS OR
 * IMPLIED WARRANTY OF ANY KIND.  NVIDIA DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOURCE CODE, INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE.
 * IN NO EVENT SHALL NVIDIA BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL,
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS,  WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION,  ARISING OUT OF OR IN CONNECTION WITH THE USE
 * OR PERFORMANCE OF THIS SOURCE CODE.
 *
 * U.S. Government End Users.   This source code is a "commercial item" as
 * that term is defined at  48 C.F.R. 2.101 (OCT 1995), consisting  of
 * "commercial computer  software"  and "commercial computer software
 * documentation" as such terms are  used in 48 C.F.R. 12.212 (SEPT 1995)
 * and is provided to the U.S. Government only as a commercial end item.
 * Consistent with 48 C.F.R.12.212 and 48 C.F.R. 227.7202-1 through
 * 227.7202-4 (JUNE 1995), all U.S. Government End Users acquire the
 * source code with only those rights set forth herein.
 *
 * Any use of this source code in individual and commercial software must
 * include, in the user documentation and internal comments to the code,
 * the above Disclaimer and U.S. Government End Users Notice.
 */
#ifndef NX_MIRROR_SCENE_H

#define NX_MIRROR_SCENE_H

/*!
\file
\brief classes NxMirrorScene, NxMirrorScene::MirrorFilter
*/


#include "foundation/PxPreprocessor.h"
#include "NxApexDefs.h"

#if NX_SDK_VERSION_MAJOR == 3
namespace physx
{
	class PxActor;
	class PxShape;
	class PxVec3;

	namespace apex
	{

		/**
		\brief NxMirrorScene is used to create a selected mirrored copy of a primary scene.  Works only with PhysX 3.x
		*/
		class NxMirrorScene
		{
		public:
			/**
			\brief MirrorFilter is a callback interface implemented by the application to confirm which actors and shapes are, or are not, replicated into the mirrored scene
			*/
			class MirrorFilter
			{
			public:
				/**
				\brief The application returns true if this actor should be mirrored into the secondary mirrored scene.

				\param[in] actor A const reference to the actor in the primary scene to be considered mirrored into the secondary scene.
				*/
				virtual bool shouldMirror(const physx::PxActor &actor) = 0;

				/**
				\brief The application returns true if this shape should be mirrored into the secondary mirrored scene.

				\param[in] shape A const reference to the shape in the primary scene to be considered mirrored into the secondary scene.
				*/
				virtual bool shouldMirror(const physx::PxShape &shape) = 0;

				/**
				\brief Affords the application with an opportunity to modify the contents/state of the shape before is placed into the mirrored scene.

				\param[in] shape A reference to the shape that is about to be placed into the mirrored scene.
				*/
				virtual void reviseMirrorShape(physx::PxShape &shape) = 0;

				/**
				\brief Affords the application with an opportunity to modify the contents/state of the actor before is placed into the mirrored scene.

				\param[in] actor A reference to the actor that is about to be placed into the mirrored scene
				*/
				virtual void reviseMirrorActor(physx::PxActor &actor) = 0;
			};

			/**
			\brief SynchronizePrimaryScene updates the positions of the objects around the camera relative to the static and dynamic distances specified
			These objects are then put in a thread safe queue to be processed when the mirror scene synchronize is called

			\param[in] cameraPos The current position of the camera relative to where objects are being mirrored
			*/
			virtual void synchronizePrimaryScene(const physx::PxVec3 &cameraPos) = 0;

			/**
			\brief Processes the updates to get this mirrored scene to reflect the subset of the
			primary scene that is being mirrored.  Completely thread safe, assumes that
			the primary scene and mirrored scene are most likely being run be completely
			separate threads.
			*/
			virtual void synchronizeMirrorScene(void) = 0;

			/**
			\brief Releases the NxMirrorScene class and all associated mirrored objects; it is important to not that this does *not* release
			the actual APEX scnee; simply the NxMirrorScene helper class.
						*/
			virtual void release(void) = 0;

		};

	}; // end of apex namespace
}; // end of physx namespace

#endif // NX_SDK_VERSION_MAJOR

#endif
