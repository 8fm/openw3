////////////////////////////////////////////////////////////////////////////////////
// This file contains vertex declarations for all system vertex formats           //
////////////////////////////////////////////////////////////////////////////////////

// Do not include this file. Should only be needed by gpuApiVertexFormats.cpp.
// Including it elsewhere results in compiler errors for these variables already
// being defined.

#pragma once

namespace GpuApi
{
	namespace RuntimeVertices
	{
		using namespace VertexPacking;

		PackingElement Terrain[] =
		{
			//	Type			Usage			UsageIndex	Slot	SlotType
			{	PT_Float2,		PS_Position,	0,			0,	ST_PerVertex	},
			{	PT_Float1,		PS_PatchSize,	0,			1,	ST_PerInstance	},
			{	PT_Float3,		PS_PatchBias,	0,			1,	ST_PerInstance	},
			PackingElement::END_OF_ELEMENTS
		};

		PackingElement Water[] =
		{
			//	Type			Usage			UsageIndex	Slot	SlotType
			{	PT_Float3,		PS_Position,	0,			0,	ST_PerVertex	},
			{	PT_Float2,		PS_PatchOffset,	0,			1,	ST_PerInstance	},
			PackingElement::END_OF_ELEMENTS
		};

		PackingElement TerrainSkirt[] =
		{
			{	PT_Float2,		PS_Position,	0,			0,	ST_PerVertex	},
			{	PT_Float4,		PS_PatchBias,	0,			1,	ST_PerInstance	},
			PackingElement::END_OF_ELEMENTS
		};

		// D3DVertexFormatDebugBase
		PackingElement DebugBase[] =
		{
			{	PT_Float3,		PS_Position,	0,			0,	ST_PerVertex	},
			PackingElement::END_OF_ELEMENTS
		};

		// D3DVertexFormatDebugNormal
		PackingElement DebugNormal[] =
		{
			{   PT_Float3,      PS_Position,    0,			0,  ST_PerVertex    },
			{   PT_Float3,      PS_Normal,      0,			0,  ST_PerVertex    },
			PackingElement::END_OF_ELEMENTS
		};

		// D3DVertexFormatDebugColor
		PackingElement DebugColor[] =
		{
			{	PT_Float3,		PS_Position,	0,			0,	ST_PerVertex	},
			{	PT_Color,		PS_Color,		0,			0,	ST_PerVertex	},
			PackingElement::END_OF_ELEMENTS
		};

		// D3DVertexFormatDebugColorUV
		PackingElement DebugColorUV[] =
		{
			{	PT_Float3,		PS_Position,	0,			0,	ST_PerVertex	},
			{	PT_Color,		PS_Color,		0,			0,	ST_PerVertex	},
			{	PT_Float2,		PS_TexCoord,	0,			0,	ST_PerVertex	},
			PackingElement::END_OF_ELEMENTS
		};

		PackingElement DebugColorFUV[] = 
		{
			{	PT_Float3,		PS_Position,	0,			0,	ST_PerVertex	},
			{	PT_Float4,		PS_Color,		0,			0,	ST_PerVertex	},
			{	PT_Float2,		PS_TexCoord,	0,			0,	ST_PerVertex	},
			PackingElement::END_OF_ELEMENTS
		};

		// D3DVertexFormatSkinnedDebugColorUV
		PackingElement SkinnedDebugColorUV[] =
		{
			{	PT_Float3,		PS_Position,	0,			0,	ST_PerVertex	},
			{	PT_Color,		PS_Color,		0,			0,	ST_PerVertex	},
			{	PT_Float2,		PS_TexCoord,	0,			0,	ST_PerVertex	},
			{	PT_Color,		PS_SkinIndices,	0,			0,	ST_PerVertex	},
			{	PT_Color,		PS_SkinWeights,	0,			0,	ST_PerVertex	},
			PackingElement::END_OF_ELEMENTS
		};

		// D3DVertexFormatDebugColorUV
		PackingElement SystemPos2[] =
		{
			{	PT_Short2,		PS_Position,	0,			0,	ST_PerVertex	},
			PackingElement::END_OF_ELEMENTS
		};

		PackingElement SystemPos2UVColor[] =
		{
			{	PT_Float2,		PS_Position,	0,			0,	ST_PerVertex	},
			{	PT_Float2,		PS_TexCoord,	0,			0,	ST_PerVertex	},
			{	PT_Color,		PS_Color,		0,			0,	ST_PerVertex	},
			PackingElement::END_OF_ELEMENTS
		};

		PackingElement SystemPos2Color[] =
		{
			{	PT_Short2,		PS_Position,	0,			0,	ST_PerVertex	},
			{	PT_Color,		PS_Color,		0,			0,	ST_PerVertex	},
			PackingElement::END_OF_ELEMENTS
		};

		PackingElement SystemPos2ColorColor[] =
		{
			{	PT_Short2,		PS_Position,	0,			0,	ST_PerVertex	},
			{	PT_Color,		PS_Color,		0,			0,	ST_PerVertex	},
			{	PT_Color,		PS_Color,		1,			0,	ST_PerVertex	},
			PackingElement::END_OF_ELEMENTS
		};

		// Bilboard particle vertex format, 44 bytes
		PackingElement ParticleStandard[] =
		{
			{	PT_Float3,		PS_Position,	0,			0,	ST_PerVertex	},
			{	PT_Color,		PS_Color,		0,			0,	ST_PerVertex	},
			{	PT_Float3,		PS_TexCoord,	0,			0,	ST_PerVertex	},
			{	PT_Float2,		PS_TexCoord,	1,			0,	ST_PerVertex	},
			{	PT_Float2,		PS_TexCoord,	2,			0,	ST_PerVertex	},
			PackingElement::END_OF_ELEMENTS
		};

		// Motion blur particle vertex format, 64
		PackingElement ParticleMotionBlur[] =
		{
			{	PT_Float3,		PS_Position,	0,			0,	ST_PerVertex	},
			{	PT_Color,		PS_Color,		0,			0,	ST_PerVertex	},
			{	PT_Float3,		PS_TexCoord,	0,			0,	ST_PerVertex	},
			{	PT_Float2,		PS_TexCoord,	1,			0,	ST_PerVertex	},
			{	PT_Float2,		PS_TexCoord,	2,			0,	ST_PerVertex	},
			{	PT_Float3,		PS_TexCoord,	3,			0,	ST_PerVertex	},
			{	PT_Float1,		PS_TexCoord,	4,			0,	ST_PerVertex	},
			{	PT_Float1,		PS_TexCoord,	5,			0,	ST_PerVertex	},
			PackingElement::END_OF_ELEMENTS
		};

		// Sphere aligned particle vertex format, 48 bytes
		PackingElement ParticleSphereAlign[] =
		{
			{	PT_Float3,		PS_Position,	0,			0,	ST_PerVertex	},
			{	PT_Color,		PS_Color,		0,			0,	ST_PerVertex	},
			{	PT_Float3,		PS_TexCoord,	0,			0,	ST_PerVertex	},
			{	PT_Float2,		PS_TexCoord,	1,			0,	ST_PerVertex	},
			{	PT_Float2,		PS_TexCoord,	2,			0,	ST_PerVertex	},
			{	PT_Float1,		PS_TexCoord,	3,			0,	ST_PerVertex	},
			PackingElement::END_OF_ELEMENTS
		};

		// Trail particle vertex format, 36 bytes
		PackingElement ParticleTrail[] =
		{
			{	PT_Float3,		PS_Position,	0,			0,	ST_PerVertex	},
			{	PT_Color,		PS_Color,		0,			0,	ST_PerVertex	},
			{	PT_Float2,		PS_TexCoord,	0,			0,	ST_PerVertex	},
			{	PT_Float1,		PS_TexCoord,	1,			0,	ST_PerVertex	},
			PackingElement::END_OF_ELEMENTS
		};

		// Camera facing trail particle vertex format, 44 bytes
		PackingElement ParticleFacingTrail_Beam[] =
		{
			{	PT_Float3,		PS_Position,	0,			0,	ST_PerVertex	},
			{	PT_Color,		PS_Color,		0,			0,	ST_PerVertex	},
			{	PT_Float3,		PS_TexCoord,	0,			0,	ST_PerVertex	},
			{	PT_Float2,		PS_TexCoord,	1,			0,	ST_PerVertex	},
			{	PT_Float1,		PS_TexCoord,	2,			0,	ST_PerVertex	},
			{	PT_Float1,		PS_TexCoord,	3,			0,	ST_PerVertex	},
			PackingElement::END_OF_ELEMENTS
		};

		// Particle billboard
		PackingElement Billboard[] =
		{
			{	PT_Float3,		PS_Position,	0,			0,	ST_PerVertex	},
			{	PT_Color,		PS_Color,		0,			0,	ST_PerVertex	},
			{	PT_Float2,		PS_TexCoord,	0,			0,	ST_PerVertex	},
			{	PT_Float2,		PS_TexCoord,	1,			0,	ST_PerVertex	},
			{	PT_Float2,		PS_TexCoord,	2,			0,	ST_PerVertex	},
			{	PT_Float2,		PS_TexCoord,	3,			0,	ST_PerVertex	},
			{	PT_Float2,		PS_TexCoord,	4,			0,	ST_PerVertex	},
			PackingElement::END_OF_ELEMENTS
		};

		PackingElement ApexPosNorCol[] =
		{
			{	PT_Float3,		PS_Position,	0,			0,	ST_PerVertex	},
			{	PT_Float3,		PS_Normal,		0,			0,	ST_PerVertex	},
			{	PT_Color,		PS_Color,		0,			0,	ST_PerVertex	},
			PackingElement::END_OF_ELEMENTS
		};

#ifdef RED_PLATFORM_ORBIS
		ePackingUsage PS_ScaleformPos = PS_Position;
#else
		ePackingUsage PS_ScaleformPos = PS_SysPosition;
#endif

		PackingElement Scaleform_Pos[] =
		{
			{	PT_Float2,		PS_ScaleformPos,0,			0,	ST_PerVertex	},
			PackingElement::END_OF_ELEMENTS
		};
		PackingElement Scaleform_PosVBatch[] =
		{
			{	PT_Float2,		PS_ScaleformPos,0,			0,	ST_PerVertex	},
			{	PT_UByte1,		PS_Color,		1,			0,	ST_PerVertex	},
			PackingElement::END_OF_ELEMENTS
		};

		PackingElement Scaleform_AFactorPos[] =
		{
			{	PT_Color,		PS_Color,		0,			0,	ST_PerVertex	},
			{	PT_Float2,		PS_ScaleformPos,0,			0,	ST_PerVertex	},
			PackingElement::END_OF_ELEMENTS
		};

		PackingElement Scaleform_AColorAFactorPos[] =
		{
			{	PT_Color,		PS_Color,		0,			0,	ST_PerVertex	},
			{	PT_Color,		PS_Color,		1,			0,	ST_PerVertex	},
			{	PT_Float2,		PS_ScaleformPos,0,			0,	ST_PerVertex	},
			PackingElement::END_OF_ELEMENTS
		};

		PackingElement Scaleform_AColorPosVBatch[] =
		{
			{	PT_Color,		PS_Color,		0,			0,	ST_PerVertex	},
			{	PT_Float2,		PS_ScaleformPos,0,			0,	ST_PerVertex	},
			{	PT_UByte1,		PS_Color,		2,			0,	ST_PerVertex	},
			PackingElement::END_OF_ELEMENTS
		};

		PackingElement Scaleform_AColorATCPos[] =
		{
			{	PT_Color,		PS_Color,		0,			0,	ST_PerVertex	},
			{	PT_Float2,		PS_TexCoord,	0,			0,	ST_PerVertex	},
			{	PT_Float2,		PS_ScaleformPos,0,			0,	ST_PerVertex	},
			PackingElement::END_OF_ELEMENTS
		};
		PackingElement Scaleform_AColorATCPosVBatch[] =
		{
			{	PT_Color,		PS_Color,		0,			0,	ST_PerVertex	},
			{	PT_Float2,		PS_TexCoord,	0,			0,	ST_PerVertex	},
			{	PT_Float2,		PS_ScaleformPos,0,			0,	ST_PerVertex	},
			{	PT_UByte1,		PS_Color,		2,			0,	ST_PerVertex	},
			PackingElement::END_OF_ELEMENTS
		};

		PackingElement Scaleform_ATCPos[] =
		{
			{	PT_Float2,		PS_TexCoord,	0,			0,	ST_PerVertex	},
			{	PT_Float2,		PS_ScaleformPos,0,			0,	ST_PerVertex	},
			PackingElement::END_OF_ELEMENTS
		};
		PackingElement Scaleform_ATCPosVBatch[] =
		{
			{	PT_Float2,		PS_TexCoord,	0,			0,	ST_PerVertex	},
			{	PT_Float2,		PS_ScaleformPos,0,			0,	ST_PerVertex	},
			{	PT_UByte1,		PS_Color,		1,			0,	ST_PerVertex	},
			PackingElement::END_OF_ELEMENTS
		};

		// Stripe vertex
		PackingElement Stripe[] =
		{
			{	PT_Float3,		PS_SysPosition,	0,			0,	ST_PerVertex	},
			{	PT_Color,		PS_Color,		0,			0,	ST_PerVertex	},
			{	PT_Float2,		PS_TexCoord,	0,			0,	ST_PerVertex	},
			{	PT_Float3,		PS_TexCoord,	1,			0,	ST_PerVertex	},
			{	PT_Float3,		PS_Tangent,		0,			0,	ST_PerVertex	},
			{	PT_Float4,		PS_TexCoord,	2,			0,	ST_PerVertex	},
			PackingElement::END_OF_ELEMENTS
		};
	}


	namespace CookedVertices
	{
		// The slot numbers in these layouts should match the streams provided by CRenderMesh.
		// Except for the instance slot, which should match what CRenderMeshBatcher sets.

		using namespace VertexPacking;

		PackingElement StaticMeshPositionOnly[] =
		{
			{	PT_UShort4N,	PS_Position,			0,	0,	ST_PerVertex	},
			PackingElement::END_OF_ELEMENTS
		};

		PackingElement StaticMeshPositionOnlyInstanced[] =
		{
			{	PT_UShort4N,	PS_Position,			0,	0,	ST_PerVertex	},
			{	PT_Float4,		PS_InstanceTransform,	0,	7,	ST_PerInstance	},
			{	PT_Float4,		PS_InstanceTransform,	1,	7,	ST_PerInstance	},
			{	PT_Float4,		PS_InstanceTransform,	2,	7,	ST_PerInstance	},
			{	PT_Float4,		PS_InstanceLODParams,	0,	7,	ST_PerInstance	},
			PackingElement::END_OF_ELEMENTS
		};

		PackingElement StaticMeshSimple[] =
		{
			{	PT_UShort4N,	PS_Position,			0,	0,	ST_PerVertex	},
			{	PT_Float16_2,	PS_TexCoord,			0,	1,	ST_PerVertex	},
			{	PT_Dec4,		PS_Normal,				0,	2,	ST_PerVertex	},
			{	PT_Dec4,		PS_Tangent,				0,	2,	ST_PerVertex	},
			PackingElement::END_OF_ELEMENTS
		};

		PackingElement StaticMeshSimpleInstanced[] =
		{
			{	PT_UShort4N,	PS_Position,			0,	0,	ST_PerVertex	},
			{	PT_Float16_2,	PS_TexCoord,			0,	1,	ST_PerVertex	},
			{	PT_Dec4,		PS_Normal,				0,	2,	ST_PerVertex	},
			{	PT_Dec4,		PS_Tangent,				0,	2,	ST_PerVertex	},
			{	PT_Float4,		PS_InstanceTransform,	0,	7,	ST_PerInstance	},
			{	PT_Float4,		PS_InstanceTransform,	1,	7,	ST_PerInstance	},
			{	PT_Float4,		PS_InstanceTransform,	2,	7,	ST_PerInstance	},
			{	PT_Float4,		PS_InstanceLODParams,	0,	7,	ST_PerInstance	},
			PackingElement::END_OF_ELEMENTS
		};

		PackingElement StaticMeshExtended[] =
		{
			{	PT_UShort4N,	PS_Position,			0,	0,	ST_PerVertex	},
			{	PT_Float16_2,	PS_TexCoord,			0,	1,	ST_PerVertex	},
			{	PT_Dec4,		PS_Normal,				0,	2,	ST_PerVertex	},
			{	PT_Dec4,		PS_Tangent,				0,	2,	ST_PerVertex	},
			{	PT_Color,		PS_Color,				0,	3,	ST_PerVertex	},
			{	PT_Float16_2,	PS_TexCoord,			1,	3,	ST_PerVertex	},
			PackingElement::END_OF_ELEMENTS
		};

		PackingElement StaticMeshExtendedInstanced[] =
		{
			{	PT_UShort4N,	PS_Position,			0,	0,	ST_PerVertex	},
			{	PT_Float16_2,	PS_TexCoord,			0,	1,	ST_PerVertex	},
			{	PT_Dec4,		PS_Normal,				0,	2,	ST_PerVertex	},
			{	PT_Dec4,		PS_Tangent,				0,	2,	ST_PerVertex	},
			{	PT_Color,		PS_Color,				0,	3,	ST_PerVertex	},
			{	PT_Float16_2,	PS_TexCoord,			1,	3,	ST_PerVertex	},
			{	PT_Float4,		PS_InstanceTransform,	0,	7,	ST_PerInstance	},
			{	PT_Float4,		PS_InstanceTransform,	1,	7,	ST_PerInstance	},
			{	PT_Float4,		PS_InstanceTransform,	2,	7,	ST_PerInstance	},
			{	PT_Float4,		PS_InstanceLODParams,	0,	7,	ST_PerInstance	},
			PackingElement::END_OF_ELEMENTS
		};

		PackingElement SkinnedMeshSimple[] =
		{
			{	PT_UShort4N,	PS_Position,			0,	0,	ST_PerVertex	},
			{	PT_UByte4,		PS_SkinIndices,			0,	0,	ST_PerVertex	},
			{	PT_UByte4N,		PS_SkinWeights,			0,	0,	ST_PerVertex	},
			{	PT_Float16_2,	PS_TexCoord,			0,	1,	ST_PerVertex	},
			{	PT_Dec4,		PS_Normal,				0,	2,	ST_PerVertex	},
			{	PT_Dec4,		PS_Tangent,				0,	2,	ST_PerVertex	},
			PackingElement::END_OF_ELEMENTS
		};

		PackingElement SkinnedMeshSimpleInstanced[] =
		{
			{	PT_UShort4N,	PS_Position,			0,	0,	ST_PerVertex	},
			{	PT_UByte4,		PS_SkinIndices,			0,	0,	ST_PerVertex	},
			{	PT_UByte4N,		PS_SkinWeights,			0,	0,	ST_PerVertex	},
			{	PT_Float16_2,	PS_TexCoord,			0,	1,	ST_PerVertex	},
			{	PT_Dec4,		PS_Normal,				0,	2,	ST_PerVertex	},
			{	PT_Dec4,		PS_Tangent,				0,	2,	ST_PerVertex	},
			{	PT_Float4,		PS_InstanceTransform,	0,	7,	ST_PerInstance	},
			{	PT_Float4,		PS_InstanceTransform,	1,	7,	ST_PerInstance	},
			{	PT_Float4,		PS_InstanceTransform,	2,	7,	ST_PerInstance	},
			{	PT_Float4,		PS_InstanceLODParams,	0,	7,	ST_PerInstance	},
			{	PT_Float4,		PS_InstanceSkinningData,0,	7,	ST_PerInstance	},
			PackingElement::END_OF_ELEMENTS
		};

		PackingElement SkinnedMeshExtended[] =
		{
			{	PT_UShort4N,	PS_Position,			0,	0,	ST_PerVertex	},
			{	PT_UByte4,		PS_SkinIndices,			0,	0,	ST_PerVertex	},
			{	PT_UByte4N,		PS_SkinWeights,			0,	0,	ST_PerVertex	},
			{	PT_Float16_2,	PS_TexCoord,			0,	1,	ST_PerVertex	},
			{	PT_Dec4,		PS_Normal,				0,	2,	ST_PerVertex	},
			{	PT_Dec4,		PS_Tangent,				0,	2,	ST_PerVertex	},
			{	PT_Color,		PS_Color,				0,	3,	ST_PerVertex	},
			{	PT_Float16_2,	PS_TexCoord,			1,	3,	ST_PerVertex	},
			PackingElement::END_OF_ELEMENTS
		};

		PackingElement SkinnedMeshExtendedInstanced[] =
		{
			{	PT_UShort4N,	PS_Position,			0,	0,	ST_PerVertex	},
			{	PT_UByte4,		PS_SkinIndices,			0,	0,	ST_PerVertex	},
			{	PT_UByte4N,		PS_SkinWeights,			0,	0,	ST_PerVertex	},
			{	PT_Float16_2,	PS_TexCoord,			0,	1,	ST_PerVertex	},
			{	PT_Dec4,		PS_Normal,				0,	2,	ST_PerVertex	},
			{	PT_Dec4,		PS_Tangent,				0,	2,	ST_PerVertex	},
			{	PT_Color,		PS_Color,				0,	3,	ST_PerVertex	},
			{	PT_Float16_2,	PS_TexCoord,			1,	3,	ST_PerVertex	},
			{	PT_Float4,		PS_InstanceTransform,	0,	7,	ST_PerInstance	},
			{	PT_Float4,		PS_InstanceTransform,	1,	7,	ST_PerInstance	},
			{	PT_Float4,		PS_InstanceTransform,	2,	7,	ST_PerInstance	},
			{	PT_Float4,		PS_InstanceLODParams,	0,	7,	ST_PerInstance	},
			{	PT_Float4,		PS_InstanceSkinningData,0,	7,	ST_PerInstance	},
			PackingElement::END_OF_ELEMENTS
		};
	}

	namespace MorphGenVertices
	{
		using namespace VertexPacking;

		// Input layouts must match regular mesh layouts above.
		// Output layouts must be compatible with the mesh layouts as well (same size, data written from SO shader must be readable
		// by a mesh layout).
		
		PackingElement InStaticMesh[] =
		{
			{	PT_UShort4N,	PS_Position,	0,			0,	ST_PerVertex	},
			{	PT_Dec4,		PS_Normal,		0,			1,	ST_PerVertex	},
			{	PT_Dec4,		PS_Tangent,		0,			1,	ST_PerVertex	},

			{	PT_UShort4N,	PS_Position,	1,			2,	ST_PerVertex	},
			{	PT_Dec4,		PS_Normal,		1,			3,	ST_PerVertex	},
			{	PT_Dec4,		PS_Tangent,		1,			3,	ST_PerVertex	},
			PackingElement::END_OF_ELEMENTS
		};
		PackingElement InStaticMeshControl[] =
		{
			{	PT_UShort4N,	PS_Position,	0,			0,	ST_PerVertex	},
			{	PT_Dec4,		PS_Normal,		0,			1,	ST_PerVertex	},
			{	PT_Dec4,		PS_Tangent,		0,			1,	ST_PerVertex	},

			{	PT_UShort4N,	PS_Position,	1,			2,	ST_PerVertex	},
			{	PT_Dec4,		PS_Normal,		1,			3,	ST_PerVertex	},
			{	PT_Dec4,		PS_Tangent,		1,			3,	ST_PerVertex	},

			{	PT_Float16_2,	PS_TexCoord,	0,			4,	ST_PerVertex	},
			PackingElement::END_OF_ELEMENTS
		};
		PackingElement OutStaticMesh[] =
		{
			// HACK : HLSL Doesn't seem to like outputting other than floats for Position, so use TexCoord instead...
			{	PT_UInt2,		PS_TexCoord,	0,			0,	ST_PerVertex	},
			// Normal and tangent packed together into one element. In theory we should be able to output one element to each output
			// buffer, so this would let us do that... doesn't seem to work on xbox...
			{	PT_UInt2,		PS_Normal,		0,			0,	ST_PerVertex	},
			PackingElement::END_OF_ELEMENTS
		};

		PackingElement InSkinnedMesh[] =
		{
			{	PT_UShort4N,	PS_Position,	0,			0,	ST_PerVertex	},
			{	PT_UInt1,		PS_SkinIndices,	0,			0,	ST_PerVertex	},
			{	PT_UInt1,		PS_SkinWeights,	0,			0,	ST_PerVertex	},
			{	PT_Dec4,		PS_Normal,		0,			1,	ST_PerVertex	},
			{	PT_Dec4,		PS_Tangent,		0,			1,	ST_PerVertex	},

			{	PT_UShort4N,	PS_Position,	1,			2,	ST_PerVertex	},
			{	PT_UInt1,		PS_SkinIndices,	1,			2,	ST_PerVertex	},
			{	PT_UInt1,		PS_SkinWeights,	1,			2,	ST_PerVertex	},
			{	PT_Dec4,		PS_Normal,		1,			3,	ST_PerVertex	},
			{	PT_Dec4,		PS_Tangent,		1,			3,	ST_PerVertex	},
			PackingElement::END_OF_ELEMENTS
		};
		PackingElement InSkinnedMeshControl[] =
		{
			{	PT_UShort4N,	PS_Position,	0,			0,	ST_PerVertex	},
			{	PT_UInt1,		PS_SkinIndices,	0,			0,	ST_PerVertex	},
			{	PT_UInt1,		PS_SkinWeights,	0,			0,	ST_PerVertex	},
			{	PT_Dec4,		PS_Normal,		0,			1,	ST_PerVertex	},
			{	PT_Dec4,		PS_Tangent,		0,			1,	ST_PerVertex	},

			{	PT_UShort4N,	PS_Position,	1,			2,	ST_PerVertex	},
			{	PT_UInt1,		PS_SkinIndices,	1,			2,	ST_PerVertex	},
			{	PT_UInt1,		PS_SkinWeights,	1,			2,	ST_PerVertex	},
			{	PT_Dec4,		PS_Normal,		1,			3,	ST_PerVertex	},
			{	PT_Dec4,		PS_Tangent,		1,			3,	ST_PerVertex	},
			
			{	PT_Float16_2,	PS_TexCoord,	0,			4,	ST_PerVertex	},
			PackingElement::END_OF_ELEMENTS
		};
		PackingElement OutSkinnedMesh[] =
		{
			// HACK : HLSL Doesn't seem to like outputting other than floats for Position, so use TexCoord instead...
			// Position packed together with skinning data, rather than splitting it apart.
			{	PT_UInt4,		PS_TexCoord,	0,			0,	ST_PerVertex	},
			// Again, normal and tangent together.
			{	PT_UInt2,		PS_Normal,		0,			0,	ST_PerVertex	},
			PackingElement::END_OF_ELEMENTS
		};
	}

	namespace DecalGenVertices
	{
		using namespace VertexPacking;

		PackingElement OutDynamicDecal[] =
		{
			{	PT_UInt2,		PS_TexCoord,	0,			0,	ST_PerVertex	},			// Float16_4 packed into UInt2 for Stream-out
			PackingElement::END_OF_ELEMENTS
		};
	}
};
