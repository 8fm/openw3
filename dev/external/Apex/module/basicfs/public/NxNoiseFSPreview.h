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
#ifndef NX_NOISE_FSPREVIEW_H
#define NX_NOISE_FSPREVIEW_H

#include "NxApex.h"
#include "NxApexAssetPreview.h"

namespace physx
{
namespace apex
{

PX_PUSH_PACK_DEFAULT

class NxApexRenderDebug;

/**
	\brief The APEX_NOISE namespace contains the defines for setting the preview detail levels.

	NOISE_DRAW_NOTHING - draw nothing<BR>
	NOISE_DRAW_SHAPE - draw the shape<BR>
	NOISE_DRAW_ASSET_INFO - draw the asset info in the screen space<BR>
	NOISE_DRAW_FULL_DETAIL - draw all components of the preview<BR>
*/
namespace APEX_NOISE
{
/**
	\def NOISE_DRAW_NOTHING
	Draw no preview items.
*/
static const physx::PxU32 NOISE_DRAW_NOTHING = 0x00;
/**
	\def NOISE_DRAW_SHAPE
	Draw the box.  The top of the box should be above ground and the bottom should be below ground.
*/
static const physx::PxU32 NOISE_DRAW_SHAPE = 0x01;
/**
	\def NOISE_DRAW_ASSET_INFO
	Draw the Asset Info in Screen Space.  This displays the various asset members that are relevant to the current asset.
	parameters that are not relevant because they are disabled are not displayed.
*/
static const physx::PxU32 NOISE_DRAW_ASSET_INFO = 0x02;
/**
	\def NOISE_DRAW_FULL_DETAIL
	Draw all of the preview rendering items.
*/
static const physx::PxU32 NOISE_DRAW_FULL_DETAIL = (NOISE_DRAW_SHAPE + NOISE_DRAW_ASSET_INFO);
}

/**
\brief This class provides the asset preview for APEX NoiseFS Assets.  The class provides multiple levels of prevew
detail that can be selected individually.
*/
class NxNoiseFSPreview : public NxApexAssetPreview
{
public:
	/**
	Set the detail level of the preview rendering by selecting which features to enable.<BR>
	Any, all, or none of the following masks may be added together to select what should be drawn.<BR>
	The defines for the individual items are:<br>
		NOISE_DRAW_NOTHING - draw nothing<BR>
		NOISE_DRAW_SHAPE - draw the shape<BR>
		NOISE_DRAW_ASSET_INFO - draw the asset info in the screen space<BR>
		NOISE_DRAW_FULL_DETAIL - draw all components of the asset preview<BR>
	All items may be drawn using the macro DRAW_FULL_DETAIL.
	*/
	virtual void	setDetailLevel(physx::PxU32 detail) = 0;
};


PX_POP_PACK

}
} // namespace physx::apex

#endif // NX_NOISE_FSPREVIEW_H
