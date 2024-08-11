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

#ifndef __MODIFIER_DATA_H__
#define __MODIFIER_DATA_H__

#include "PsShare.h"
#include "foundation/PxVec3.h"
#include <PxMat33Legacy.h>
#include "InplaceTypes.h"
#include "RandState.h"

#include "NxUserRenderInstanceBufferDesc.h"
#include "NxUserRenderSpriteBufferDesc.h"

namespace physx
{
namespace apex
{
namespace iofx
{

#ifndef __CUDACC__
PX_INLINE float saturate(float x)
{
	return (x < 0.0f) ? 0.0f : (1.0f < x) ? 1.0f : x;
}
#endif

// output color is NxRenderDataFormat::B8G8R8A8
#define FLT_TO_BYTE(x) ( (unsigned int)(saturate(abs(x)) * 255) )
#define MAKE_COLOR_UBYTE4(r, g, b, a) ( ((r) << 16) | ((g) << 8) | ((b) << 0) | ((a) << 24) )


class IosObjectBaseData;

struct ModifierCommonParams
{
	int				inputHasCollision;
	int				inputHasDensity;
	physx::PxVec3	upVector;

	physx::PxVec3	eyePosition;
	physx::PxVec3	eyeDirection;
	physx::PxVec3	eyeAxisX;
	physx::PxVec3	eyeAxisY;
	physx::PxF32	zNear;
	float			deltaTime;

#ifndef __CUDACC__
	template <typename R>
	void reflect(R& r)
	{
		r.reflect(inputHasCollision);
		r.reflect(inputHasDensity);
		r.reflect(upVector);
		r.reflect(eyePosition);
		r.reflect(eyeDirection);
		r.reflect(eyeAxisX);
		r.reflect(eyeAxisY);
		r.reflect(zNear);
		r.reflect(deltaTime);
	}
#endif
};

// Mesh structs
struct MeshInput
{
	physx::PxVec3	position;
	physx::PxF32	mass;
	physx::PxVec3	velocity;
	physx::PxF32	liferemain;
	physx::PxF32	density;
	physx::PxVec3	collisionNormal;
	unsigned int	collisionFlags;
	physx::PxU32	userData;

	PX_INLINE void load(const IosObjectBaseData& objData, physx::PxU32 pos);
};

struct MeshPublicState
{
	physx::PxMat33Legacy	rotation;
	physx::PxVec3			scale;

	float	color[4];

	static PX_CUDA_CALLABLE PX_INLINE void initDefault(MeshPublicState& state)
	{
		state.rotation.setIdentity();
		state.scale = physx::PxVec3(1.0f);

		state.color[0] = -1.0f;
		state.color[1] = -1.0f;
		state.color[2] = -1.0f;
		state.color[3] = -1.0f;
	}
};

/* TODO: Private state size should be declared by each IOFX asset, so the IOS can allocate
 * the private buffer dynamically based on the IOFX assets used with the IOS.  Each asset would
 * in turn be given an offset for their private data in this buffer.
 */
struct MeshPrivateState
{
	physx::PxMat33Legacy			rotation;

	static PX_CUDA_CALLABLE PX_INLINE void initDefault(MeshPrivateState& state)
	{
		state.rotation.setIdentity();
	}
};

struct MeshOutputLayout
{
	physx::PxU32 stride;
	physx::PxU32 offsets[NxRenderInstanceLayoutElement::NUM_SEMANTICS];

#ifdef __CUDACC__
#define WRITE_TO_FLOAT(data, offset) { *((volatile float*)(sdata + ((offset) >> 2) * pitch) + idx) = data; }
#define WRITE_TO_UINT(data, offset) { *((volatile unsigned int*)(sdata + ((offset) >> 2) * pitch + idx)) = data; }

	__device__ PX_INLINE void write(volatile unsigned int* sdata, unsigned int idx, unsigned int pitch, const MeshInput& input, const MeshPublicState& state, unsigned int outputID) const
#else
#define WRITE_TO_FLOAT(data, offset) { *(float*)(outputPtr + outputID * stride + offset) = data; }
#define WRITE_TO_UINT(data, offset) { *(unsigned int*)(outputPtr + outputID * stride + offset) = data; }

	PX_INLINE void write(physx::PxU32 outputID, const MeshInput& input, const MeshPublicState& state, const physx::PxU8* outputPtr) const
#endif
	{
		if (offsets[NxRenderInstanceLayoutElement::POSITION_FLOAT3] != static_cast<physx::PxU32>(-1)) //POSITION: 3 dwords
		{
			WRITE_TO_FLOAT( input.position.x, offsets[NxRenderInstanceLayoutElement::POSITION_FLOAT3] + 0 * sizeof(float) )
			WRITE_TO_FLOAT( input.position.y, offsets[NxRenderInstanceLayoutElement::POSITION_FLOAT3] + 1 * sizeof(float) )
			WRITE_TO_FLOAT( input.position.z, offsets[NxRenderInstanceLayoutElement::POSITION_FLOAT3] + 2 * sizeof(float) )
		}
		if (offsets[NxRenderInstanceLayoutElement::ROTATION_SCALE_FLOAT3x3] != static_cast<physx::PxU32>(-1)) //ROTATION_SCALE: 9 dwords
		{
			physx::PxVec3 axis0 = state.rotation.getColumn(0) * state.scale.x;
			physx::PxVec3 axis1 = state.rotation.getColumn(1) * state.scale.y;
			physx::PxVec3 axis2 = state.rotation.getColumn(2) * state.scale.z;

			WRITE_TO_FLOAT( axis0.x, offsets[NxRenderInstanceLayoutElement::ROTATION_SCALE_FLOAT3x3] + 0 * sizeof(float) )
			WRITE_TO_FLOAT( axis0.y, offsets[NxRenderInstanceLayoutElement::ROTATION_SCALE_FLOAT3x3] + 1 * sizeof(float) )
			WRITE_TO_FLOAT( axis0.z, offsets[NxRenderInstanceLayoutElement::ROTATION_SCALE_FLOAT3x3] + 2 * sizeof(float) )

			WRITE_TO_FLOAT( axis1.x, offsets[NxRenderInstanceLayoutElement::ROTATION_SCALE_FLOAT3x3] + 3 * sizeof(float) )
			WRITE_TO_FLOAT( axis1.y, offsets[NxRenderInstanceLayoutElement::ROTATION_SCALE_FLOAT3x3] + 4 * sizeof(float) )
			WRITE_TO_FLOAT( axis1.z, offsets[NxRenderInstanceLayoutElement::ROTATION_SCALE_FLOAT3x3] + 5 * sizeof(float) )

			WRITE_TO_FLOAT( axis2.x, offsets[NxRenderInstanceLayoutElement::ROTATION_SCALE_FLOAT3x3] + 6 * sizeof(float) )
			WRITE_TO_FLOAT( axis2.y, offsets[NxRenderInstanceLayoutElement::ROTATION_SCALE_FLOAT3x3] + 7 * sizeof(float) )
			WRITE_TO_FLOAT( axis2.z, offsets[NxRenderInstanceLayoutElement::ROTATION_SCALE_FLOAT3x3] + 8 * sizeof(float) )
		}
		if (offsets[NxRenderInstanceLayoutElement::VELOCITY_LIFE_FLOAT4] != static_cast<physx::PxU32>(-1)) //VELOCITY: 3 dwords
		{
			WRITE_TO_FLOAT( input.velocity.x, offsets[NxRenderInstanceLayoutElement::VELOCITY_LIFE_FLOAT4] + 0 * sizeof(float) )
			WRITE_TO_FLOAT( input.velocity.y, offsets[NxRenderInstanceLayoutElement::VELOCITY_LIFE_FLOAT4] + 1 * sizeof(float) )
			WRITE_TO_FLOAT( input.velocity.z, offsets[NxRenderInstanceLayoutElement::VELOCITY_LIFE_FLOAT4] + 2 * sizeof(float) )
			WRITE_TO_FLOAT( input.liferemain, offsets[NxRenderInstanceLayoutElement::VELOCITY_LIFE_FLOAT4] + 3 * sizeof(float) )
		}
		if (offsets[NxRenderInstanceLayoutElement::DENSITY_FLOAT1] != static_cast<physx::PxU32>(-1)) //DENSITY: 1 dword
		{
			WRITE_TO_FLOAT( input.density, offsets[NxRenderInstanceLayoutElement::DENSITY_FLOAT1] )
		}
		if (offsets[NxRenderInstanceLayoutElement::COLOR_BGRA8] != static_cast<physx::PxU32>(-1)) //COLOR: 1 dword
		{
			WRITE_TO_UINT( MAKE_COLOR_UBYTE4( FLT_TO_BYTE(state.color[0]), 
											FLT_TO_BYTE(state.color[1]), 
											FLT_TO_BYTE(state.color[2]), 
											FLT_TO_BYTE(state.color[3]) ),
											offsets[NxRenderInstanceLayoutElement::COLOR_BGRA8])
		}
		if (offsets[NxRenderInstanceLayoutElement::COLOR_RGBA8] != static_cast<physx::PxU32>(-1)) //COLOR: 1 dword
		{
			WRITE_TO_UINT( MAKE_COLOR_UBYTE4( FLT_TO_BYTE(state.color[2]), 
											FLT_TO_BYTE(state.color[1]), 
											FLT_TO_BYTE(state.color[0]), 
											FLT_TO_BYTE(state.color[3]) ),
											offsets[NxRenderInstanceLayoutElement::COLOR_RGBA8])
		}
		if (offsets[NxRenderInstanceLayoutElement::COLOR_FLOAT4] != static_cast<physx::PxU32>(-1)) //COLOR_FLOAT4: 4 dword
		{
			WRITE_TO_FLOAT( state.color[0], offsets[NxRenderInstanceLayoutElement::COLOR_FLOAT4] + 0 * sizeof(float) )
			WRITE_TO_FLOAT( state.color[1], offsets[NxRenderInstanceLayoutElement::COLOR_FLOAT4] + 1 * sizeof(float) )
			WRITE_TO_FLOAT( state.color[2], offsets[NxRenderInstanceLayoutElement::COLOR_FLOAT4] + 2 * sizeof(float) )
			WRITE_TO_FLOAT( state.color[3], offsets[NxRenderInstanceLayoutElement::COLOR_FLOAT4] + 3 * sizeof(float) )
		}
		if (offsets[NxRenderInstanceLayoutElement::USER_DATA_UINT1] != static_cast<physx::PxU32>(-1)) //USER_DATA: 1 dword
		{
			WRITE_TO_UINT( input.userData, offsets[NxRenderInstanceLayoutElement::USER_DATA_UINT1] )
		}
	}
#undef WRITE_TO_UINT
#undef WRITE_TO_FLOAT

#ifndef __CUDACC__
	template <typename R>
	void reflect(R& )
	{
	}
#endif
};


// Sprite structs
struct SpriteInput
{
	physx::PxVec3	position;
	physx::PxF32	mass;
	physx::PxVec3	velocity;
	physx::PxF32	liferemain;
	physx::PxF32	density;
	physx::PxU32	userData;

	PX_INLINE void load(const IosObjectBaseData& objData, physx::PxU32 pos);
};


struct SpritePublicState
{
	physx::PxVec3	scale;
	float			subTextureId;
	float			rotation;

	float			color[4];

	static PX_CUDA_CALLABLE PX_INLINE void initDefault(SpritePublicState& state)
	{
		state.scale = physx::PxVec3(1.0f);

		state.subTextureId = 0;
		state.rotation = 0;

		state.color[0] = -1.0f;
		state.color[1] = -1.0f;
		state.color[2] = -1.0f;
		state.color[3] = -1.0f;
	}
};

/* TODO: Private state size should be declared by each IOFX asset, so the IOS can allocate
 * the private buffer dynamically based on the IOFX assets used with the IOS.  Each asset would
 * in turn be given an offset for their private data in this buffer.
 */
struct SpritePrivateState
{
	float	rotation;

	static PX_CUDA_CALLABLE PX_INLINE void initDefault(SpritePrivateState& state)
	{
		state.rotation = 0;
	}
};

struct SpriteOutputLayout
{
	physx::PxU32 stride;
	physx::PxU32 offsets[NxRenderSpriteLayoutElement::NUM_SEMANTICS];

#ifdef __CUDACC__
#define WRITE_TO_FLOAT(data, offset) { *((volatile float*)(sdata + ((offset) >> 2) * pitch + idx)) = data; }
#define WRITE_TO_UINT(data, offset) { *((volatile unsigned int*)(sdata + ((offset) >> 2) * pitch + idx)) = data; }

	__device__ PX_INLINE void write(volatile unsigned int* sdata, unsigned int idx, unsigned int pitch, const SpriteInput& input, const SpritePublicState& state, unsigned int outputID) const
#else
#define WRITE_TO_FLOAT(data, offset) { *(float*)(outputPtr + outputID * stride + offset) = data; }
#define WRITE_TO_UINT(data, offset) { *(unsigned int*)(outputPtr + outputID * stride + offset) = data; }

	PX_INLINE void write(physx::PxU32 outputID, const SpriteInput& input, const SpritePublicState& state, const physx::PxU8* outputPtr) const
#endif
	{
		if(offsets[NxRenderSpriteLayoutElement::POSITION_FLOAT3] != static_cast<physx::PxU32>(-1))
		{
			WRITE_TO_FLOAT( input.position.x, offsets[NxRenderSpriteLayoutElement::POSITION_FLOAT3] + 0 * sizeof(float))
			WRITE_TO_FLOAT( input.position.y, offsets[NxRenderSpriteLayoutElement::POSITION_FLOAT3] + 1 * sizeof(float))
			WRITE_TO_FLOAT( input.position.z, offsets[NxRenderSpriteLayoutElement::POSITION_FLOAT3] + 2 * sizeof(float))
		}
		if(offsets[NxRenderSpriteLayoutElement::COLOR_BGRA8] != static_cast<physx::PxU32>(-1))
		{
			WRITE_TO_UINT( MAKE_COLOR_UBYTE4(FLT_TO_BYTE(state.color[0]), 
											FLT_TO_BYTE(state.color[1]), 
											FLT_TO_BYTE(state.color[2]), 
											FLT_TO_BYTE(state.color[3])),
					   offsets[NxRenderSpriteLayoutElement::COLOR_BGRA8])
		}
		if(offsets[NxRenderSpriteLayoutElement::COLOR_RGBA8] != static_cast<physx::PxU32>(-1))
		{
			WRITE_TO_UINT( MAKE_COLOR_UBYTE4(FLT_TO_BYTE(state.color[2]), 
											FLT_TO_BYTE(state.color[1]), 
											FLT_TO_BYTE(state.color[0]), 
											FLT_TO_BYTE(state.color[3])),
					   offsets[NxRenderSpriteLayoutElement::COLOR_RGBA8])
		}
		if(offsets[NxRenderSpriteLayoutElement::COLOR_FLOAT4] != static_cast<physx::PxU32>(-1))
		{
			WRITE_TO_FLOAT( state.color[0], offsets[NxRenderSpriteLayoutElement::COLOR_FLOAT4] + 0 * sizeof(float) )
			WRITE_TO_FLOAT( state.color[1], offsets[NxRenderSpriteLayoutElement::COLOR_FLOAT4] + 1 * sizeof(float) )
			WRITE_TO_FLOAT( state.color[2], offsets[NxRenderSpriteLayoutElement::COLOR_FLOAT4] + 2 * sizeof(float) )
			WRITE_TO_FLOAT( state.color[3], offsets[NxRenderSpriteLayoutElement::COLOR_FLOAT4] + 3 * sizeof(float) )
		}
		if(offsets[NxRenderSpriteLayoutElement::VELOCITY_FLOAT3] != static_cast<physx::PxU32>(-1))
		{
			WRITE_TO_FLOAT( input.velocity.x, offsets[NxRenderSpriteLayoutElement::VELOCITY_FLOAT3] + 0 * sizeof(float))
			WRITE_TO_FLOAT( input.velocity.y, offsets[NxRenderSpriteLayoutElement::VELOCITY_FLOAT3] + 1 * sizeof(float))
			WRITE_TO_FLOAT( input.velocity.z, offsets[NxRenderSpriteLayoutElement::VELOCITY_FLOAT3] + 2 * sizeof(float))
		}
		if(offsets[NxRenderSpriteLayoutElement::SCALE_FLOAT2] != static_cast<physx::PxU32>(-1))
		{
			WRITE_TO_FLOAT( state.scale.x, offsets[NxRenderSpriteLayoutElement::SCALE_FLOAT2] + 0 * sizeof(float) )
			WRITE_TO_FLOAT( state.scale.y, offsets[NxRenderSpriteLayoutElement::SCALE_FLOAT2] + 1 * sizeof(float) )
		}
		if(offsets[NxRenderSpriteLayoutElement::LIFE_REMAIN_FLOAT1] != static_cast<physx::PxU32>(-1))
		{
			WRITE_TO_FLOAT( input.liferemain, offsets[NxRenderSpriteLayoutElement::LIFE_REMAIN_FLOAT1] )
		}
		if(offsets[NxRenderSpriteLayoutElement::DENSITY_FLOAT1] != static_cast<physx::PxU32>(-1))
		{
			WRITE_TO_FLOAT( input.density, offsets[NxRenderSpriteLayoutElement::DENSITY_FLOAT1] )
		}
		if(offsets[NxRenderSpriteLayoutElement::SUBTEXTURE_FLOAT1] != static_cast<physx::PxU32>(-1))
		{
			WRITE_TO_FLOAT( state.subTextureId, offsets[NxRenderSpriteLayoutElement::SUBTEXTURE_FLOAT1] )
		}
		if(offsets[NxRenderSpriteLayoutElement::ORIENTATION_FLOAT1] != static_cast<physx::PxU32>(-1))
		{
			WRITE_TO_FLOAT( state.rotation, offsets[NxRenderSpriteLayoutElement::ORIENTATION_FLOAT1] )
		}
		if(offsets[NxRenderSpriteLayoutElement::USER_DATA_UINT1] != static_cast<physx::PxU32>(-1))
		{
			WRITE_TO_UINT( input.userData, offsets[NxRenderSpriteLayoutElement::USER_DATA_UINT1] )
		}
	}
#undef WRITE_TO_UINT
#undef WRITE_TO_FLOAT

#ifndef __CUDACC__
	template <typename R>
	void reflect(R& )
	{
	}
#endif
};

struct TextureOutputData
{
	physx::PxU16    layout;
	physx::PxU8     widthShift;
	physx::PxU8     pitchShift;
};

struct SpriteTextureOutputLayout
{
	physx::PxU32      textureCount;
	TextureOutputData textureData[4];
	physx::PxU8*      texturePtr[4];

#ifdef __CUDACC__
#define WRITE_TO_FLOAT4(e0, e1, e2, e3) { *(float4*)(ptr + (y << pitchShift) + (x << 4)) = make_float4(e0, e1, e2, e3); }
#define WRITE_TO_UINT(data) { *(unsigned int*)(ptr + (y << pitchShift) + (x << 2)) = data; }

	__device__ PX_INLINE void write(volatile unsigned int* sdata, unsigned int idx, unsigned int pitch, const SpriteInput& input, const SpritePublicState& state, unsigned int outputID) const
#else
#define WRITE_TO_FLOAT4(e0, e1, e2, e3) { *(physx::PxVec4*)(ptr + (y << pitchShift) + (x << 4)) = physx::PxVec4(e0, e1, e2, e3); }
#define WRITE_TO_UINT(data) { *(unsigned int*)(ptr + (y << pitchShift) + (x << 2)) = data; }

	PX_INLINE void write(unsigned int outputID, const SpriteInput& input, const SpritePublicState& state, const physx::PxU8*) const
#endif
	{
#define WRITE_TO_TEXTURE(N) \
		if (N < textureCount) \
		{ \
			physx::PxU32 y = (outputID >> textureData[N].widthShift); \
			physx::PxU32 x = outputID - (y << textureData[N].widthShift); \
			physx::PxU8  pitchShift = textureData[N].pitchShift; \
			physx::PxU8* ptr = texturePtr[N]; \
			switch (textureData[N].layout) \
			{ \
			case NxRenderSpriteTextureLayout::POSITION_FLOAT4: \
				WRITE_TO_FLOAT4( input.position.x, input.position.y, input.position.z, 1.0f ) \
				break; \
			case NxRenderSpriteTextureLayout::SCALE_ORIENT_SUBTEX_FLOAT4: \
				WRITE_TO_FLOAT4( state.scale.x, state.scale.y, state.rotation, state.subTextureId ) \
				break; \
			case NxRenderSpriteTextureLayout::COLOR_BGRA8: \
				WRITE_TO_UINT( MAKE_COLOR_UBYTE4( FLT_TO_BYTE(state.color[0]), FLT_TO_BYTE(state.color[1]), FLT_TO_BYTE(state.color[2]), FLT_TO_BYTE(state.color[3]) ) ) \
				break; \
			case NxRenderSpriteTextureLayout::COLOR_FLOAT4: \
				WRITE_TO_FLOAT4( state.color[0], state.color[1], state.color[2], state.color[3] ) \
				break; \
			} \
		}

		WRITE_TO_TEXTURE(0)
		WRITE_TO_TEXTURE(1)
		WRITE_TO_TEXTURE(2)
		WRITE_TO_TEXTURE(3)
	}
#undef WRITE_TO_TEXTURE
#undef WRITE_TO_UINT
#undef WRITE_TO_FLOAT4

#ifndef __CUDACC__
	template <typename R>
	void reflect(R& r)
	{
	}
#endif
};


struct CurvePoint
{
	physx::PxF32 x, y;

	PX_INLINE CurvePoint() : x(0.0f), y(0.0f) {}
	PX_INLINE CurvePoint(physx::PxF32 _x, physx::PxF32 _y) : x(_x), y(_y) {}
};

}

#ifndef __CUDACC__
template <> struct InplaceTypeTraits<physx::apex::iofx::CurvePoint>
{
	enum { hasReflect = false };
};
#endif

namespace iofx
{

PX_CUDA_CALLABLE PX_INLINE physx::PxF32 lerpPoints(physx::PxF32 x, const CurvePoint& p0, const CurvePoint& p1)
{
	return ((x - p0.x) / (p1.x - p0.x)) * (p1.y - p0.y) + p0.y;
}

class Curve
{
	InplaceArray<CurvePoint> _pointArray;

public:
#ifndef __CUDACC__
	template <typename S>
	PX_INLINE void create(S& storage, physx::PxU32 numPoints)
	{
		_pointArray.resize(storage, numPoints);
	}

	template <typename S>
	PX_INLINE CurvePoint* getPoints(S& storage)
	{
		return _pointArray.getElems(storage);
	}
#endif

	template <typename S>
	PX_CUDA_CALLABLE PX_INLINE physx::PxF32 evaluate(S& storage, physx::PxF32 x) const
	{
		physx::PxU32 count = _pointArray.getSize();
		const CurvePoint* points = _pointArray.getElems(storage);
		if (count == 0)
		{
			return 0.0f;
		}
		else if (x <= points[0].x)
		{
			return points[0].y;
		}
		else if (x >= points[count - 1].x)
		{
			return points[count - 1].y;
		}

		//do binary search
		unsigned int beg = 0;
		unsigned int end = count;
		while (beg < end)
		{
			unsigned int mid = beg + ((end - beg) >> 1);
			if (x < points[mid].x)
			{
				end = mid;
			}
			else
			{
				beg = mid + 1;
			}
		}
		beg = physx::PxMin<physx::PxU32>(beg, count - 1);
		return lerpPoints(x, points[beg - 1], points[beg]);
	}

#ifndef __CUDACC__
	template <typename R>
	void reflect(R& r)
	{
		r.reflect(_pointArray);
	}
#endif
};

}
}
} // namespace apex

#endif /* __MODIFIER_DATA_H__ */
