/*
 * Copyright 2009-2011 NVIDIA Corporation.  All rights reserved.
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

#ifndef PX_PROCESS_RENDER_DEBUG_H
#define PX_PROCESS_RENDER_DEBUG_H

#include "PxRenderDebug.h"

namespace physx
{
namespace general_renderdebug4
{

#define MAX_LINE_VERTEX 2048
#define MAX_SOLID_VERTEX 2048

PX_PUSH_PACK_DEFAULT


class PxProcessRenderDebug
{
public:
	enum DisplayType
	{
		SCREEN_SPACE,
		WORLD_SPACE,
		WORLD_SPACE_NOZ,
		DT_LAST
	};

	virtual void processRenderDebug(const DebugPrimitive **dplist,
									PxU32 pcount,
									RenderDebugInterface *iface,
									DisplayType type) = 0;

	virtual void flush(RenderDebugInterface *iface,DisplayType type) = 0;

	virtual void flushFrame(RenderDebugInterface *iface) = 0;

	virtual void release(void) = 0;

	virtual void setViewMatrix(const physx::PxMat44 &view) 
	{
		PX_UNUSED(view);
	}

protected:
	virtual ~PxProcessRenderDebug(void) { };

};


PxProcessRenderDebug * createProcessRenderDebug(void);


PX_POP_PACK

}; // end of namespace
using namespace general_renderdebug4;
}; // end of namespace

#endif
