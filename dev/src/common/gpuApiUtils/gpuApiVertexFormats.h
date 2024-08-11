/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "gpuApiVertexPacking.h"

namespace GpuApi
{
	//////////////////////////////////////////////////////////////////////////////////

	// Vertex type
	enum eBufferChunkType
	{
		// Vertex buffer types
		BCT_VertexBillboard,
		BCT_VertexMeshStaticPositionOnly,
		BCT_VertexMeshStaticSimple,
		BCT_VertexMeshStaticExtended,
		BCT_VertexMeshSkinnedSimple,
		BCT_VertexMeshSkinnedExtended,

		// Water instanced patches
		BCT_VertexWater,

		// New terrain
		BCT_VertexTerrain,
		BCT_VertexTerrainSkirt,

		// Instanced types		
		BCT_VertexMeshStaticPositionOnlyInstanced,
		BCT_VertexMeshStaticSimpleInstanced,
		BCT_VertexMeshStaticExtendedInstanced,
		BCT_VertexMeshSkinnedSimpleInstanced,
		BCT_VertexMeshSkinnedExtendedInstanced,

		// Runtime formats ( not cooked or anything )
		BCT_VertexSystemPos,
		BCT_VertexSystemPosNormal,
		BCT_VertexSystemPosColor,
		BCT_VertexSystemPosColorUV,
		BCT_VertexSystemPosColorFUV,
		BCT_VertexSystemSkinnedPosColorUV,
		BCT_VertexSystemPos2,
		BCT_VertexSystemPos2Color,
		BCT_VertexSystemPos2UVColor,
		BCT_VertexSystemPos2ColorColor,
		BCT_VertexSystemParticleStandard,
		BCT_VertexSystemParticleMotionBlur,
		BCT_VertexSystemParticleTrail,
		BCT_VertexSystemParticleFacingTrail_Beam,

		// Index buffer types
		BCT_IndexUInt,
		BCT_IndexUShort,

		// Scaleform
		BCT_VertexScaleformAColorAFactorPos,
		BCT_VertexScaleformAColorATCPos,
		BCT_VertexScaleformAColorATCPosVBatch,
		BCT_VertexScaleformAColorPosVBatch,
		BCT_VertexScaleformAFactorPos,
		BCT_VertexScaleformATCPos,
		BCT_VertexScaleformATCPosVBatch,
		BCT_VertexScaleformPos,
		BCT_VertexScaleformPosVBatch,

		// For generating Morphed Mesh data
		BCT_VertexMeshStaticMorphGenIn,
		BCT_VertexMeshSkinnedMorphGenIn,
		BCT_VertexMeshStaticControlMorphGenIn,
		BCT_VertexMeshSkinnedControlMorphGenIn,
		BCT_VertexMeshStaticMorphGenOut,
		BCT_VertexMeshSkinnedMorphGenOut,

		// For generating dynamic decals (on moving objects)
		BCT_VertexDynamicDecalGenOut,

		// Stripes
		BCT_VertexSystemStripe,

		// Summary
		BCT_Max,
	};

	enum eBufferChunkCategory
	{
		BCC_Vertex,
		BCC_Index16Bit,
		BCC_Index32Bit,
		BCC_Constant,
		BCC_Structured,
		BCC_StructuredUAV,
		BCC_StructuredAppendUAV,
		BCC_IndirectUAV,
		BCC_VertexSRV,
		BCC_Index16BitUAV,
		BCC_Raw,

		BCC_Invalid
	};

	/////////////////////////////////////////////////////////////////////////////////////////////////////////

	struct SystemVertex_Pos
	{
		Float x, y, z;

		SystemVertex_Pos ()
		{}

		SystemVertex_Pos ( Float newX, Float newY, Float newZ )
			: x( newX ), y( newY ), z( newZ )
		{}

		static eBufferChunkType GetSystemVertexType()
		{
			return BCT_VertexSystemPos;
		}
	};

	struct SystemVertex_PosColor : public SystemVertex_Pos
	{
		Uint32	color;

		SystemVertex_PosColor ()
		{}

		SystemVertex_PosColor ( Float newX, Float newY, Float newZ, Uint32 newColor )
			: SystemVertex_Pos ( newX, newY, newZ )
			, color( newColor )
		{}		

		static eBufferChunkType GetSystemVertexType()
		{
			return BCT_VertexSystemPosColor;
		}
	};

	struct SystemVertex_PosColorFUV : public SystemVertex_Pos
	{
		Float	color[4];
		Float	u,v;

		SystemVertex_PosColorFUV()
		{}

		SystemVertex_PosColorFUV( Float newX, Float newY, Float newZ, const Float* newColor, Float newU, Float newV )
			: SystemVertex_Pos( newX, newY, newZ )
			, u( newU ), v( newV )
		{
			color[0] = newColor[0];
			color[1] = newColor[1];
			color[2] = newColor[2];
			color[3] = newColor[3];
		}

		static eBufferChunkType GetSystemVertexType()
		{
			return BCT_VertexSystemPosColorFUV;
		}

	};

	struct SystemVertex_PosColorUV : public SystemVertex_PosColor
	{
		Float u,v;

		SystemVertex_PosColorUV ()
		{}

		SystemVertex_PosColorUV ( Float newX, Float newY, Float newZ, Uint32 newColor, Float newU, Float newV )
			: SystemVertex_PosColor( newX, newY, newZ, newColor )
			, u( newU ), v( newV )
		{}

		static eBufferChunkType GetSystemVertexType()
		{
			return BCT_VertexSystemPosColorUV;
		}
	};

	struct SystemVertex_Terrain
	{
		Float m_patchCorner[2];
		/*Float m_patchSize;
		Float m_patchSizeNorm;
		Float m_patchUV[2];*/

		static eBufferChunkType GetSystemVertexType()
		{
			return BCT_VertexTerrain;
		}
	};

	struct SystemVertex_Water
	{
		Float m_position[3];

		static eBufferChunkType GetSystemVertexType()
		{
			return BCT_VertexWater;
		}
	};

	struct SystemVertex_SkinnedPosColorUV : public SystemVertex_PosColorUV
	{
		Uint8	indices[4];
		Uint8	weights[4];

		static eBufferChunkType GetSystemVertexType()
		{
			return BCT_VertexSystemSkinnedPosColorUV;
		}
	};
		
	struct SystemVertex_ParticleStandard
	{		
		Float	m_position[3];
		Uint32	m_color;
		Float	m_rotation[2];
		Float	m_frame;
		Float	m_size[2];
		Float	m_uv[2];

		static eBufferChunkType GetSystemVertexType()
		{
			return BCT_VertexSystemParticleStandard;
		}
	};

	struct SystemVertex_ParticleMotionBlur
	{		
		Float	m_position[3];
		Uint32	m_color;
		Float	m_rotation[2];
		Float	m_frame;
		Float	m_size[2];
		Float	m_uv[2];
		Float	m_direction[3];
		Float	m_stretch;
		Float	m_motionBlend;

		static eBufferChunkType GetSystemVertexType()
		{
			return BCT_VertexSystemParticleMotionBlur;
		}
	};

	struct SystemVertex_ParticleTrail
	{		
		Float	m_position[3];
		Uint32	m_color;
		Float	m_uv[2];
		Float	m_frame;

		static eBufferChunkType GetSystemVertexType()
		{
			return BCT_VertexSystemParticleTrail;
		}
	};

	struct SystemVertex_ParticleFacingTrail_Beam
	{		
		Float	m_position[3];
		Uint32	m_color;
		Float	m_prevPosition[3];
		Float	m_uv[2];
		Float	m_stretch;
		Float	m_frame;

		static eBufferChunkType GetSystemVertexType()
		{
			return BCT_VertexSystemParticleFacingTrail_Beam;
		}
	};

	struct SystemVertex_Pos2
	{		
		Uint16	m_position[2];

		static eBufferChunkType GetSystemVertexType()
		{
			return BCT_VertexSystemPos2;
		}
	};

	struct SystemVertex_Pos2Color
	{		
		Uint16	m_position[2];
		Uint8	m_color[4];

		static eBufferChunkType GetSystemVertexType()
		{
			return BCT_VertexSystemPos2Color;
		}
	};

	struct SystemVertex_Pos2UVColor
	{		
		Float	m_position[2];
		Float	m_uv[2];
		Uint8	m_color[4];

		static eBufferChunkType GetSystemVertexType()
		{
			return BCT_VertexSystemPos2UVColor;
		}
	};

	struct SystemVertex_Pos2ColorColor
	{		
		Uint16	m_position[2];
		Uint8	m_color1[4];
		Uint8	m_color2[4];

		static eBufferChunkType GetSystemVertexType()
		{
			return BCT_VertexSystemPos2ColorColor;
		}
	};

	struct SystemVertex_Stripe
	{
		Float	x, y, z;		//!< Position
		Uint32	color;			//!< Color
		Float	u, v;			//!< Diffuse texture coordinates
		Float	bu, bv;			//!< Blend texture coordinates
		Float	offset;			//!< Blend value offset
		Float	tx, ty, tz;		//!< Tangent
		Float	cx, cy, cz, sw;	//!< Stripe segment center and size

		SystemVertex_Stripe(){}
		static eBufferChunkType GetSystemVertexType()
		{
			return BCT_VertexSystemStripe;
		}
	};



	//////////////////////////////////////////////////////////////////////////////////

	// Returns vertexType if already instanced, or if no instanced version exists.
	eBufferChunkType GetInstancedChunkType( eBufferChunkType vertexType );

	// Get vertex packing format for current platform
	const VertexPacking::PackingElement* GetPackingForFormat( eBufferChunkType vertexFormat );

	//////////////////////////////////////////////////////////////////////////////////

};
