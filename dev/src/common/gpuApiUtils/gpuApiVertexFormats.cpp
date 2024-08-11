/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#include "gpuApiUtils.h"
#include "gpuApiVertexDeclarations.h"

// Get vertex packing format for current platform
namespace GpuApi
{

	eBufferChunkType GetInstancedChunkType( eBufferChunkType vertexType )
	{
		switch ( vertexType )
		{
			// Chunk Types with instanced versions.
		case BCT_VertexMeshStaticSimple:			return BCT_VertexMeshStaticSimpleInstanced;
		case BCT_VertexMeshStaticExtended:			return BCT_VertexMeshStaticExtendedInstanced;
			// (these ones don't have an actual layout defined, but the BCT is there)
		case BCT_VertexMeshSkinnedSimple:			return BCT_VertexMeshSkinnedSimpleInstanced;
		case BCT_VertexMeshSkinnedExtended:			return BCT_VertexMeshSkinnedExtendedInstanced;
			// simple vertex version (shadows only)
		case BCT_VertexMeshStaticPositionOnly:		return BCT_VertexMeshStaticPositionOnlyInstanced;

			// Already instanced.
		case BCT_VertexMeshStaticPositionOnlyInstanced:
		case BCT_VertexMeshStaticSimpleInstanced:
		case BCT_VertexMeshStaticExtendedInstanced:
			// (these ones don't have an actual layout defined, but the BCT is there)
		case BCT_VertexMeshSkinnedSimpleInstanced:
		case BCT_VertexMeshSkinnedExtendedInstanced:
		case BCT_VertexWater:		
		case BCT_VertexTerrain:
		case BCT_VertexTerrainSkirt:
			return vertexType;

		default:
			GPUAPI_HALT( "No matching instanced vertex type" );
			return vertexType;
		}
	}


	const VertexPacking::PackingElement* GetPackingForFormat( eBufferChunkType vertexFormat )
	{
		switch ( vertexFormat )
		{
			// Runtime formats
		case BCT_VertexSystemPos:						return RuntimeVertices::DebugBase;
		case BCT_VertexSystemPosNormal:					return RuntimeVertices::DebugNormal;
		case BCT_VertexSystemPosColor:					return RuntimeVertices::DebugColor;
		case BCT_VertexSystemPosColorUV:				return RuntimeVertices::DebugColorUV;
		case BCT_VertexSystemPosColorFUV:				return RuntimeVertices::DebugColorFUV;
		case BCT_VertexSystemSkinnedPosColorUV:			return RuntimeVertices::SkinnedDebugColorUV;
		case BCT_VertexSystemPos2:						return RuntimeVertices::SystemPos2;
		case BCT_VertexSystemPos2Color:					return RuntimeVertices::SystemPos2Color;
		case BCT_VertexSystemPos2UVColor:				return RuntimeVertices::SystemPos2UVColor;
		case BCT_VertexSystemPos2ColorColor:			return RuntimeVertices::SystemPos2ColorColor;
		case BCT_VertexBillboard:						return RuntimeVertices::Billboard;
		case BCT_VertexWater:							return RuntimeVertices::Water;		
		case BCT_VertexTerrain:							return RuntimeVertices::Terrain;
		case BCT_VertexTerrainSkirt:					return RuntimeVertices::TerrainSkirt;
		case BCT_VertexSystemStripe:					return RuntimeVertices::Stripe;

			// Particle formats
		case BCT_VertexSystemParticleStandard:			return RuntimeVertices::ParticleStandard;
		case BCT_VertexSystemParticleMotionBlur:		return RuntimeVertices::ParticleMotionBlur;
		case BCT_VertexSystemParticleTrail:				return RuntimeVertices::ParticleTrail;
		case BCT_VertexSystemParticleFacingTrail_Beam:	return RuntimeVertices::ParticleFacingTrail_Beam;

			// Scaleform
		case BCT_VertexScaleformAColorAFactorPos:		return RuntimeVertices::Scaleform_AColorAFactorPos;
		case BCT_VertexScaleformAColorATCPos:			return RuntimeVertices::Scaleform_AColorATCPos;
		case BCT_VertexScaleformAColorATCPosVBatch:		return RuntimeVertices::Scaleform_AColorATCPosVBatch;
		case BCT_VertexScaleformAColorPosVBatch:		return RuntimeVertices::Scaleform_AColorPosVBatch;
		case BCT_VertexScaleformAFactorPos:				return RuntimeVertices::Scaleform_AFactorPos;
		case BCT_VertexScaleformATCPos:					return RuntimeVertices::Scaleform_ATCPos;
		case BCT_VertexScaleformATCPosVBatch:			return RuntimeVertices::Scaleform_ATCPosVBatch;
		case BCT_VertexScaleformPos:					return RuntimeVertices::Scaleform_Pos;
		case BCT_VertexScaleformPosVBatch:				return RuntimeVertices::Scaleform_PosVBatch;


			// Cooked formats
		case BCT_VertexMeshStaticSimple:				return CookedVertices::StaticMeshSimple;
		case BCT_VertexMeshStaticExtended:				return CookedVertices::StaticMeshExtended;
		case BCT_VertexMeshStaticSimpleInstanced:		return CookedVertices::StaticMeshSimpleInstanced;
		case BCT_VertexMeshStaticExtendedInstanced:		return CookedVertices::StaticMeshExtendedInstanced;
		case BCT_VertexMeshStaticPositionOnly:			return CookedVertices::StaticMeshPositionOnly;
		case BCT_VertexMeshStaticPositionOnlyInstanced:	return CookedVertices::StaticMeshPositionOnlyInstanced;
		case BCT_VertexMeshSkinnedSimple:				return CookedVertices::SkinnedMeshSimple;
		case BCT_VertexMeshSkinnedExtended:				return CookedVertices::SkinnedMeshExtended;
		case BCT_VertexMeshSkinnedSimpleInstanced:		return CookedVertices::SkinnedMeshSimpleInstanced;
		case BCT_VertexMeshSkinnedExtendedInstanced:	return CookedVertices::SkinnedMeshExtendedInstanced;

		case BCT_VertexMeshStaticMorphGenIn:			return MorphGenVertices::InStaticMesh;
		case BCT_VertexMeshSkinnedMorphGenIn:			return MorphGenVertices::InSkinnedMesh;
		case BCT_VertexMeshStaticControlMorphGenIn:		return MorphGenVertices::InStaticMeshControl;
		case BCT_VertexMeshSkinnedControlMorphGenIn:	return MorphGenVertices::InSkinnedMeshControl;
		case BCT_VertexMeshStaticMorphGenOut:			return MorphGenVertices::OutStaticMesh;
		case BCT_VertexMeshSkinnedMorphGenOut:			return MorphGenVertices::OutSkinnedMesh;

		case BCT_VertexDynamicDecalGenOut:				return DecalGenVertices::OutDynamicDecal;

			// Not handled
		case BCT_IndexUInt:
		case BCT_IndexUShort:
			GPUAPI_ASSERT( TXT("index type buffers don't have vertex packing") );
			break;

		default:
			GPUAPI_ASSERT( TXT("unsupported buffer chunk type") );
		}

		// No packing
		return NULL;
	}
}
