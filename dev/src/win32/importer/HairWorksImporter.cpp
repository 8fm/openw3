/**
* Copyright © 2008 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#include "../../common/renderer/build.h"
#include "../../common/core/importer.h"
#include "../../common/engine/bitmapTexture.h"
#include "../../common/core/depot.h"

#ifdef USE_NVIDIA_FUR

#include "../../../external/NvidiaHair/include/GFSDK_HairWorks.h"

#endif // USE_NVIDIA_FUR

#pragma warning( push )
#pragma warning( disable:4530 ) // C++ exception handler used, but unwind semantics are not enabled. Specify /EHsc
#include <vector>
#include "../../common/engine/furMeshResource.h"
#pragma warning( pop )

/// Importer for HairWorks .APX files
class CHairWorksImporter : public IImporter
{
	DECLARE_ENGINE_CLASS( CHairWorksImporter, IImporter, 0 );

#ifdef USE_NVIDIA_FUR
	static void LoadHairAssetDesc( CFurMeshResource* pMeshRead, const GFSDK_HairAssetDescriptor* hairAssetDesc );
	static void LoadDefaultHairInstanceDesc( CFurMeshResource* pMeshRead, GFSDK_HairInstanceDescriptor* hairInstanceDesc );
	static void LoadSecondHairInstanceDesc( CFurMeshResource* pMeshRead, GFSDK_HairInstanceDescriptor* hairInstanceDesc );
#endif

public:
	CHairWorksImporter();

	virtual CResource* DoImport( const ImportOptions& options );
};

BEGIN_CLASS_RTTI( CHairWorksImporter )
	PARENT_CLASS( IImporter )
END_CLASS_RTTI()

IMPLEMENT_ENGINE_CLASS( CHairWorksImporter );

CHairWorksImporter::CHairWorksImporter()
{
	// Importer
	m_resourceClass = ClassID< CFurMeshResource >();
	m_formats.PushBack( CFileFormat( TXT("apx"), TXT("NVIDIA HairWorks APX") ) );

	// Load config
	LoadObjectConfig( TXT("User") );
}

namespace
{
#ifdef USE_NVIDIA_FUR
	void LoadHairTexture( const char* textureNames, int index, THandle< CBitmapTexture >& texture )
	{
		if ( textureNames[ index * GFSDK_HAIR_MAX_STRING ] )
		{
			CBitmapTexture* tex = LoadResource< CBitmapTexture >(ANSI_TO_UNICODE( &textureNames[ index * GFSDK_HAIR_MAX_STRING ] ));
			if ( tex )
			{
				texture = tex;
			}
		}
	}
#endif
}

#ifdef USE_NVIDIA_FUR
// Load hair asset descriptor into fur mesh resource
void CHairWorksImporter::LoadHairAssetDesc( CFurMeshResource* pMeshRead, const GFSDK_HairAssetDescriptor* hairAssetDesc )
{
	pMeshRead->m_positions.ClearFast();
	gfsdk_float3* vertex = nullptr;
	// Calculate bounding box
	pMeshRead->m_boundingBox.Clear();
	for ( gfsdk_U32 idx = 0; idx < hairAssetDesc->m_NumVertices; idx++ )
	{
		vertex = &hairAssetDesc->m_pVertices[ idx ];
		pMeshRead->m_positions.PushBack( Vector( vertex->x, vertex->y, vertex->z ) );
		pMeshRead->m_boundingBox.AddPoint( pMeshRead->m_positions.Last() );
	}
	gfsdk_float2* uv = nullptr;
	pMeshRead->m_uvs.ClearFast();
	pMeshRead->m_faceIndices.ClearFast();
	for ( gfsdk_U32 idx = 0; idx < hairAssetDesc->m_NumFaces*3; ++idx )
	{
		uv = &hairAssetDesc->m_pFaceUVs[idx];
		pMeshRead->m_uvs.PushBack( Vector2( uv->x, uv->y ) );
		pMeshRead->m_faceIndices.PushBack( hairAssetDesc->m_pFaceIndices[ idx ] );
	}
	pMeshRead->m_endIndices.ClearFast();
	pMeshRead->m_boneIndices.ClearFast();
	pMeshRead->m_boneWeights.ClearFast();
	for ( gfsdk_U32 idx = 0; idx < hairAssetDesc->m_NumGuideHairs; ++idx )
	{
		pMeshRead->m_endIndices.PushBack( hairAssetDesc->m_pEndIndices[ idx ] );
		pMeshRead->m_boneIndices.PushBack( Vector( &hairAssetDesc->m_pBoneIndices[ idx ].x ) );
		pMeshRead->m_boneWeights.PushBack( Vector( &hairAssetDesc->m_pBoneWeights[ idx ].x ) );
	}
	pMeshRead->m_boneCount = hairAssetDesc->m_NumBones;
	pMeshRead->m_boneNames.ClearFast();
	pMeshRead->m_bindPoses.ClearFast();
	pMeshRead->m_boneRigMatrices.ClearFast();
	pMeshRead->m_boneParents.ClearFast();
	for ( gfsdk_U32 idx = 0; idx < hairAssetDesc->m_NumBones; ++idx )
	{
		if ( hairAssetDesc->m_pBoneNames )
		{
			pMeshRead->m_boneNames.PushBack( CName( ANSI_TO_UNICODE( &hairAssetDesc->m_pBoneNames[ idx * GFSDK_HAIR_MAX_STRING ] ) ) );
		}
		if ( hairAssetDesc->m_pBindPoses )
		{
			Matrix bindPose( &hairAssetDesc->m_pBindPoses[ idx ]._11 );
			pMeshRead->m_bindPoses.PushBack( bindPose );
			pMeshRead->m_boneRigMatrices.PushBack( bindPose.Inverted() );
		}
		if ( hairAssetDesc->m_pBoneParents )
		{
			pMeshRead->m_boneParents.PushBack( hairAssetDesc->m_pBoneParents[ idx ] );
		}
	}
	pMeshRead->m_boneSphereIndexArray.ClearFast();
	pMeshRead->m_boneSphereLocalPosArray.ClearFast();
	pMeshRead->m_boneSphereRadiusArray.ClearFast();
	for ( gfsdk_U32 idx = 0; idx < hairAssetDesc->m_NumBoneSpheres; ++idx )
	{
		if ( hairAssetDesc->m_pBoneSpheres )
		{
			pMeshRead->m_boneSphereIndexArray.PushBack( hairAssetDesc->m_pBoneSpheres[ idx ].m_BoneSphereIndex );
			pMeshRead->m_boneSphereLocalPosArray.PushBack(
				Vector( &hairAssetDesc->m_pBoneSpheres[ idx ].m_BoneSphereLocalPos.x ) );
			pMeshRead->m_boneSphereRadiusArray.PushBack(
				hairAssetDesc->m_pBoneSpheres[ idx ].m_BoneSphereRadius );
		}
	}
	pMeshRead->m_boneCapsuleIndices.ClearFast();
	for ( gfsdk_U32 idx  = 0; idx < hairAssetDesc->m_NumBoneCapsules*2; ++idx )
	{
		if ( hairAssetDesc->m_pBoneCapsuleIndices )
		{
			pMeshRead->m_boneCapsuleIndices.PushBack( hairAssetDesc->m_pBoneCapsuleIndices[ idx ] );
		}
	}
	pMeshRead->m_pinConstraintsIndexArray.ClearFast();
	pMeshRead->m_pinConstraintsLocalPosArray.ClearFast();
	pMeshRead->m_pinConstraintsRadiusArray.ClearFast();
	for ( gfsdk_U32 idx = 0; idx < hairAssetDesc->m_NumPinConstraints; ++idx )
	{
		if ( hairAssetDesc->m_pPinConstraints )
		{
			pMeshRead->m_pinConstraintsIndexArray.PushBack( hairAssetDesc->m_pPinConstraints[ idx ].m_BoneSphereIndex );
			pMeshRead->m_pinConstraintsLocalPosArray.PushBack(
				Vector( &hairAssetDesc->m_pPinConstraints[ idx ].m_BoneSphereLocalPos.x ) );
			pMeshRead->m_pinConstraintsRadiusArray.PushBack(
				hairAssetDesc->m_pPinConstraints[ idx ].m_BoneSphereRadius );
		}
	}

	LoadHairTexture( hairAssetDesc->m_pTextureNames, GFSDK_HAIR_TEXTURE_DENSITY, pMeshRead->m_physicalMaterials.m_volume.m_densityTex );
	LoadHairTexture( hairAssetDesc->m_pTextureNames, GFSDK_HAIR_TEXTURE_LENGTH, pMeshRead->m_physicalMaterials.m_volume.m_lengthTex );
	LoadHairTexture( hairAssetDesc->m_pTextureNames, GFSDK_HAIR_TEXTURE_ROOT_WIDTH, pMeshRead->m_physicalMaterials.m_strandWidth.m_rootWidthTex );
	LoadHairTexture( hairAssetDesc->m_pTextureNames, GFSDK_HAIR_TEXTURE_TIP_WIDTH, pMeshRead->m_physicalMaterials.m_strandWidth.m_tipWidthTex );
	LoadHairTexture( hairAssetDesc->m_pTextureNames, GFSDK_HAIR_TEXTURE_STIFFNESS, pMeshRead->m_physicalMaterials.m_stiffness.m_stiffnessTex );
	LoadHairTexture( hairAssetDesc->m_pTextureNames, GFSDK_HAIR_TEXTURE_ROOT_STIFFNESS, pMeshRead->m_physicalMaterials.m_stiffness.m_rootStiffnessTex );
	LoadHairTexture( hairAssetDesc->m_pTextureNames, GFSDK_HAIR_TEXTURE_CLUMP_SCALE, pMeshRead->m_physicalMaterials.m_clumping.m_clumpScaleTex );
	LoadHairTexture( hairAssetDesc->m_pTextureNames, GFSDK_HAIR_TEXTURE_CLUMP_ROUNDNESS, pMeshRead->m_physicalMaterials.m_clumping.m_clumpRoundnessTex );
	LoadHairTexture( hairAssetDesc->m_pTextureNames, GFSDK_HAIR_TEXTURE_CLUMP_NOISE, pMeshRead->m_physicalMaterials.m_clumping.m_clumpNoiseTex );
	LoadHairTexture( hairAssetDesc->m_pTextureNames, GFSDK_HAIR_TEXTURE_WAVE_SCALE, pMeshRead->m_physicalMaterials.m_waveness.m_waveScaleTex );
	LoadHairTexture( hairAssetDesc->m_pTextureNames, GFSDK_HAIR_TEXTURE_WAVE_FREQ, pMeshRead->m_physicalMaterials.m_waveness.m_waveFreqTex );
	LoadHairTexture( hairAssetDesc->m_pTextureNames, GFSDK_HAIR_TEXTURE_ROOT_COLOR, pMeshRead->m_graphicalMaterials.m_color.m_rootColorTex );
	LoadHairTexture( hairAssetDesc->m_pTextureNames, GFSDK_HAIR_TEXTURE_TIP_COLOR, pMeshRead->m_graphicalMaterials.m_color.m_tipColorTex );
	LoadHairTexture( hairAssetDesc->m_pTextureNames, GFSDK_HAIR_TEXTURE_STRAND, pMeshRead->m_graphicalMaterials.m_color.m_strandTex );
	LoadHairTexture( hairAssetDesc->m_pTextureNames, GFSDK_HAIR_TEXTURE_SPECULAR, pMeshRead->m_graphicalMaterials.m_specular.m_specularTex );
}

namespace Hairworks
{
void CopyVisualizers(SFurVisualizers& visualizers, GFSDK_HairInstanceDescriptor* hairInstanceDesc)
{
	visualizers.m_colorizeMode = (EHairColorizeMode)hairInstanceDesc->m_colorizeMode;
	visualizers.m_visualizeBones = hairInstanceDesc->m_visualizeBones;
	visualizers.m_visualizeCapsules = hairInstanceDesc->m_visualizeCapsules;
	visualizers.m_visualizeGuideHairs = hairInstanceDesc->m_visualizeGuideHairs;
	visualizers.m_visualizeControlVertices = hairInstanceDesc->m_visualizeControlVertices;
	visualizers.m_visualizeBoundingBox = hairInstanceDesc->m_visualizeBoundingBox;
	visualizers.m_drawRenderHairs = hairInstanceDesc->m_drawRenderHairs;
	visualizers.m_visualizeCullSphere			= hairInstanceDesc->m_visualizeCullSphere;
	visualizers.m_visualizeDiffuseBone			= hairInstanceDesc->m_visualizeDiffuseBone;
	visualizers.m_visualizeFrames				= hairInstanceDesc->m_visualizeFrames;
	visualizers.m_visualizeGrowthMesh			= hairInstanceDesc->m_visualizeGrowthMesh;
	visualizers.m_visualizeHairInteractions	= hairInstanceDesc->m_visualizeHairInteractions;
	visualizers.m_visualizeHairSkips			= hairInstanceDesc->m_visualizeHairSkips;
	visualizers.m_visualizeLocalPos			= hairInstanceDesc->m_visualizeLocalPos;
	visualizers.m_visualizePinConstraints		= hairInstanceDesc->m_visualizePinConstraints;
	visualizers.m_visualizeShadingNormals		= hairInstanceDesc->m_visualizeShadingNormals;
	visualizers.m_visualizeSkinnedGuideHairs	= hairInstanceDesc->m_visualizeSkinnedGuideHairs;
	visualizers.m_visualizeStiffnessBone		= hairInstanceDesc->m_visualizeStiffnessBone;
}

void CopyPhysicalMaterials(SFurPhysicalMaterials& physicalMaterials, GFSDK_HairInstanceDescriptor* hairInstanceDesc)
{
	// physical / strand width
	physicalMaterials.m_strandWidth.m_width = hairInstanceDesc->m_width;
	physicalMaterials.m_strandWidth.m_widthNoise = hairInstanceDesc->m_widthNoise;
	physicalMaterials.m_strandWidth.m_widthRootScale = hairInstanceDesc->m_widthRootScale;
	physicalMaterials.m_strandWidth.m_widthTipScale = hairInstanceDesc->m_widthTipScale;
	physicalMaterials.m_strandWidth.m_rootWidthTexChannel = ( EHairTextureChannel )hairInstanceDesc->m_textureChannels[ GFSDK_HAIR_TEXTURE_ROOT_WIDTH ];
	physicalMaterials.m_strandWidth.m_tipWidthTexChannel = ( EHairTextureChannel )hairInstanceDesc->m_textureChannels[ GFSDK_HAIR_TEXTURE_TIP_WIDTH ];


	// physical / clumping
	physicalMaterials.m_clumping.m_clumpNoise = hairInstanceDesc->m_clumpNoise;
	physicalMaterials.m_clumping.m_clumpRoundness = hairInstanceDesc->m_clumpRoundness;
	physicalMaterials.m_clumping.m_clumpScale = hairInstanceDesc->m_clumpScale;
	physicalMaterials.m_clumping.m_clumpNumSubclumps	= hairInstanceDesc->m_clumpNumSubclumps;
	physicalMaterials.m_clumping.m_clumpScaleTexChannel = ( EHairTextureChannel )hairInstanceDesc->m_textureChannels[ GFSDK_HAIR_TEXTURE_CLUMP_SCALE ];
	physicalMaterials.m_clumping.m_clumpRoundnessTexChannel = ( EHairTextureChannel )hairInstanceDesc->m_textureChannels[ GFSDK_HAIR_TEXTURE_CLUMP_ROUNDNESS ];
	physicalMaterials.m_clumping.m_clumpNoiseTexChannel = ( EHairTextureChannel )hairInstanceDesc->m_textureChannels[ GFSDK_HAIR_TEXTURE_CLUMP_NOISE ];

	// physical / volume
	physicalMaterials.m_volume.m_density = hairInstanceDesc->m_density;
	physicalMaterials.m_volume.m_lengthNoise = hairInstanceDesc->m_lengthNoise;
	physicalMaterials.m_volume.m_lengthScale = hairInstanceDesc->m_lengthScale;
	physicalMaterials.m_volume.m_usePixelDensity = hairInstanceDesc->m_usePixelDensity;
	physicalMaterials.m_volume.m_densityTexChannel = ( EHairTextureChannel )hairInstanceDesc->m_textureChannels[ GFSDK_HAIR_TEXTURE_DENSITY ];
	physicalMaterials.m_volume.m_lengthTexChannel = ( EHairTextureChannel )hairInstanceDesc->m_textureChannels[ GFSDK_HAIR_TEXTURE_LENGTH ];

	// physical / waveness
	physicalMaterials.m_waveness.m_waveScale = hairInstanceDesc->m_waveScale;
	physicalMaterials.m_waveness.m_waveScaleNoise = hairInstanceDesc->m_waveScaleNoise;
	physicalMaterials.m_waveness.m_waveRootStraighten = hairInstanceDesc->m_waveRootStraighten;
	physicalMaterials.m_waveness.m_waveFreq = hairInstanceDesc->m_waveFreq;
	physicalMaterials.m_waveness.m_waveFreqNoise = hairInstanceDesc->m_waveFreqNoise;
	physicalMaterials.m_waveness.m_waveStrand = hairInstanceDesc->m_waveStrand;
	physicalMaterials.m_waveness.m_waveClump = hairInstanceDesc->m_waveClump;
	physicalMaterials.m_waveness.m_waveScaleTexChannel = ( EHairTextureChannel )hairInstanceDesc->m_textureChannels[ GFSDK_HAIR_TEXTURE_WAVE_SCALE ];
	physicalMaterials.m_waveness.m_waveFreqTexChannel = ( EHairTextureChannel )hairInstanceDesc->m_textureChannels[ GFSDK_HAIR_TEXTURE_WAVE_FREQ ];

	// physical / simulation
	physicalMaterials.m_simulation.m_simulate = hairInstanceDesc->m_simulate;
	physicalMaterials.m_simulation.m_backStopRadius = hairInstanceDesc->m_backStopRadius;
	physicalMaterials.m_simulation.m_useCollision = hairInstanceDesc->m_useCollision;
	physicalMaterials.m_simulation.m_damping = hairInstanceDesc->m_damping;
	physicalMaterials.m_simulation.m_friction = hairInstanceDesc->m_friction;
	physicalMaterials.m_simulation.m_massScale = hairInstanceDesc->m_massScale;
	physicalMaterials.m_simulation.m_gravityDir = Vector( &hairInstanceDesc->m_gravityDir.x );
	physicalMaterials.m_simulation.m_inertiaScale = hairInstanceDesc->m_inertiaScale;
	physicalMaterials.m_simulation.m_inertiaLimit = hairInstanceDesc->m_inertiaLimit;
	physicalMaterials.m_simulation.m_wind = Vector( &hairInstanceDesc->m_wind.x );
	physicalMaterials.m_simulation.m_windNoise = hairInstanceDesc->m_windNoise;

	// physical / stiffness
	physicalMaterials.m_stiffness.m_stiffnessStrength = hairInstanceDesc->m_stiffnessStrength;
	physicalMaterials.m_stiffness.m_bendStiffness = hairInstanceDesc->m_bendStiffness;
	physicalMaterials.m_stiffness.m_stiffnessCurve = Vector( &hairInstanceDesc->m_stiffnessCurve.x );
	physicalMaterials.m_stiffness.m_stiffnessStrengthCurve = Vector( &hairInstanceDesc->m_stiffnessStrengthCurve.x );
	physicalMaterials.m_stiffness.m_stiffnessDampingCurve = Vector( &hairInstanceDesc->m_stiffnessDampingCurve.x );
	physicalMaterials.m_stiffness.m_bendStiffnessCurve = Vector( &hairInstanceDesc->m_bendStiffnessCurve.x );
	physicalMaterials.m_stiffness.m_interactionStiffnessCurve = Vector( &hairInstanceDesc->m_interactionStiffnessCurve.x );

	physicalMaterials.m_stiffness.m_interactionStiffness = hairInstanceDesc->m_interactionStiffness;
	physicalMaterials.m_stiffness.m_pinStiffness = hairInstanceDesc->m_pinStiffness;
	physicalMaterials.m_stiffness.m_rootStiffness = hairInstanceDesc->m_rootStiffness;
	physicalMaterials.m_stiffness.m_stiffnessDamping= hairInstanceDesc->m_stiffnessDamping;
	physicalMaterials.m_stiffness.m_tipStiffness = hairInstanceDesc->m_tipStiffness;
	physicalMaterials.m_stiffness.m_stiffness = hairInstanceDesc->m_stiffness;

	physicalMaterials.m_stiffness.m_stiffnessBoneEnable = hairInstanceDesc->m_stiffnessBoneEnable;
	physicalMaterials.m_stiffness.m_stiffnessBoneIndex = hairInstanceDesc->m_stiffnessBoneIndex;
	physicalMaterials.m_stiffness.m_stiffnessBoneAxis = hairInstanceDesc->m_stiffnessBoneAxis;
	physicalMaterials.m_stiffness.m_stiffnessStartDistance = hairInstanceDesc->m_stiffnessStartDistance;
	physicalMaterials.m_stiffness.m_stiffnessEndDistance = hairInstanceDesc->m_stiffnessEndDistance;
	physicalMaterials.m_stiffness.m_stiffnessBoneCurve = Vector( &hairInstanceDesc->m_stiffnessBoneCurve.x );

	physicalMaterials.m_stiffness.m_stiffnessTexChannel = ( EHairTextureChannel )hairInstanceDesc->m_textureChannels[ GFSDK_HAIR_TEXTURE_STIFFNESS ];
	physicalMaterials.m_stiffness.m_rootStiffnessTexChannel = ( EHairTextureChannel )hairInstanceDesc->m_textureChannels[ GFSDK_HAIR_TEXTURE_ROOT_STIFFNESS ];
}

void CopyLevelOfDetail(SFurLevelOfDetail& levelOfDetail, GFSDK_HairInstanceDescriptor* hairInstanceDesc)
{
	// LOD
	levelOfDetail.m_enableLOD = hairInstanceDesc->m_enableLOD;

	// LOD / distance lod
	levelOfDetail.m_distanceLOD.m_enableDistanceLOD = hairInstanceDesc->m_enableDistanceLOD;
	levelOfDetail.m_distanceLOD.m_distanceLODStart = hairInstanceDesc->m_distanceLODStart;
	levelOfDetail.m_distanceLOD.m_distanceLODEnd = hairInstanceDesc->m_distanceLODEnd;
	levelOfDetail.m_distanceLOD.m_distanceLODFadeStart = hairInstanceDesc->m_distanceLODFadeStart;
	levelOfDetail.m_distanceLOD.m_distanceLODDensity = hairInstanceDesc->m_distanceLODDensity;
	levelOfDetail.m_distanceLOD.m_distanceLODWidth = hairInstanceDesc->m_distanceLODWidth;

	// LOD / detailed lod
	levelOfDetail.m_detailLOD.m_enableDetailLOD = hairInstanceDesc->m_enableDetailLOD;
	levelOfDetail.m_detailLOD.m_detailLODStart = hairInstanceDesc->m_detailLODStart;
	levelOfDetail.m_detailLOD.m_detailLODEnd = hairInstanceDesc->m_detailLODEnd;
	levelOfDetail.m_detailLOD.m_detailLODDensity = hairInstanceDesc->m_detailLODDensity;
	levelOfDetail.m_detailLOD.m_detailLODWidth = hairInstanceDesc->m_detailLODWidth;

	// LOD / culling
	levelOfDetail.m_culling.m_useViewfrustrumCulling = hairInstanceDesc->m_useViewfrustrumCulling;
	levelOfDetail.m_culling.m_useBackfaceCulling = hairInstanceDesc->m_useBackfaceCulling;
	levelOfDetail.m_culling.m_backfaceCullingThreshold = hairInstanceDesc->m_backfaceCullingThreshold;
}

void CopyGraphicalMaterials(SFurGraphicalMaterials& graphicalMaterials, GFSDK_HairInstanceDescriptor* hairInstanceDesc)
{
	// graphical / shadow
	graphicalMaterials.m_shadow.m_shadowDensityScale = hairInstanceDesc->m_shadowDensityScale;
	graphicalMaterials.m_shadow.m_shadowSigma = hairInstanceDesc->m_shadowSigma;
	graphicalMaterials.m_shadow.m_castShadows = hairInstanceDesc->m_castShadows;
	graphicalMaterials.m_shadow.m_receiveShadows = hairInstanceDesc->m_receiveShadows;

	// graphical / diffuse
	graphicalMaterials.m_diffuse.m_diffuseBlend = hairInstanceDesc->m_diffuseBlend;
	graphicalMaterials.m_diffuse.m_diffuseScale = hairInstanceDesc->m_diffuseScale;
	graphicalMaterials.m_diffuse.m_diffuseHairNormalWeight = hairInstanceDesc->m_diffuseHairNormalWeight;
	graphicalMaterials.m_diffuse.m_diffuseBoneIndex = hairInstanceDesc->m_diffuseBoneIndex;
	graphicalMaterials.m_diffuse.m_diffuseBoneLocalPos = Vector(&hairInstanceDesc->m_diffuseBoneLocalPos.x);
	graphicalMaterials.m_diffuse.m_diffuseNoiseFreqU = hairInstanceDesc->m_diffuseNoiseFreqU;
	graphicalMaterials.m_diffuse.m_diffuseNoiseFreqV = hairInstanceDesc->m_diffuseNoiseFreqV;
	graphicalMaterials.m_diffuse.m_diffuseNoiseScale = hairInstanceDesc->m_diffuseNoiseScale;
	graphicalMaterials.m_diffuse.m_diffuseNoiseGain = hairInstanceDesc->m_diffuseNoiseGain;

	// graphical / color
	graphicalMaterials.m_color.m_textureBrightness = hairInstanceDesc->m_textureBrightness;
	graphicalMaterials.m_color.m_rootAlphaFalloff = hairInstanceDesc->m_rootAlphaFalloff;
	graphicalMaterials.m_color.m_rootColor = Vector( &hairInstanceDesc->m_rootColor.x );
	graphicalMaterials.m_color.m_tipColor = Vector( &hairInstanceDesc->m_tipColor.x );
	graphicalMaterials.m_color.m_rootTipColorWeight = hairInstanceDesc->m_rootTipColorWeight;
	graphicalMaterials.m_color.m_rootTipColorFalloff = hairInstanceDesc->m_rootTipColorFalloff;
	graphicalMaterials.m_color.m_strandBlendMode = (EHairStrandBlendModeType)hairInstanceDesc->m_strandBlendMode;
	graphicalMaterials.m_color.m_strandBlendScale = hairInstanceDesc->m_strandBlendScale;

	// graphical / specular
	graphicalMaterials.m_specular.m_specularColor = Vector( &hairInstanceDesc->m_specularColor.x );
	graphicalMaterials.m_specular.m_specularPrimary = hairInstanceDesc->m_specularPrimary;
	graphicalMaterials.m_specular.m_specularPrimaryBreakup = hairInstanceDesc->m_specularPrimaryBreakup;
	graphicalMaterials.m_specular.m_specularSecondary = hairInstanceDesc->m_specularSecondary;
	graphicalMaterials.m_specular.m_specularSecondaryOffset = hairInstanceDesc->m_specularSecondaryOffset;
	graphicalMaterials.m_specular.m_specularPowerPrimary = hairInstanceDesc->m_specularPowerPrimary;
	graphicalMaterials.m_specular.m_specularPowerSecondary = hairInstanceDesc->m_specularPowerSecondary;

	// graphical / glint
	graphicalMaterials.m_glint.m_glintStrength = hairInstanceDesc->m_glintStrength;
	graphicalMaterials.m_glint.m_glintCount = hairInstanceDesc->m_glintCount;
	graphicalMaterials.m_glint.m_glintExponent = hairInstanceDesc->m_glintExponent;
}

} // namespace Hairworks

// Load hair instance descriptor into fur mesh resource
void CHairWorksImporter::LoadDefaultHairInstanceDesc( CFurMeshResource* pMeshRead, GFSDK_HairInstanceDescriptor* hairInstanceDesc )
{
	pMeshRead->m_useTextures = hairInstanceDesc->m_useTextures;
	pMeshRead->m_splineMultiplier = hairInstanceDesc->m_splineMultiplier;

	// visualizer
	Hairworks::CopyVisualizers(pMeshRead->m_visualizers, hairInstanceDesc);

	// LOD
	Hairworks::CopyLevelOfDetail(pMeshRead->m_levelOfDetail, hairInstanceDesc);

	// physical materials
	Hairworks::CopyPhysicalMaterials(pMeshRead->m_physicalMaterials, hairInstanceDesc);

	// graphical materials
	Hairworks::CopyGraphicalMaterials(pMeshRead->m_graphicalMaterials, hairInstanceDesc);
}

// Load hair instance descriptor into fur mesh resource
void CHairWorksImporter::LoadSecondHairInstanceDesc( CFurMeshResource* pMeshRead, GFSDK_HairInstanceDescriptor* hairInstanceDesc )
{
	pMeshRead->m_useTextures = hairInstanceDesc->m_useTextures;
	pMeshRead->m_splineMultiplier = hairInstanceDesc->m_splineMultiplier;

	// visualizer
	Hairworks::CopyVisualizers(pMeshRead->m_visualizers, hairInstanceDesc);

	// LOD
	Hairworks::CopyLevelOfDetail(pMeshRead->m_levelOfDetail, hairInstanceDesc);

	// physical materials
	Hairworks::CopyPhysicalMaterials(pMeshRead->m_materialSets.m_physicalMaterials, hairInstanceDesc);

	// graphical materials
	Hairworks::CopyGraphicalMaterials(pMeshRead->m_materialSets.m_graphicalMaterials, hairInstanceDesc);
}

#endif // USE_NVIDIA_FUR

CResource* CHairWorksImporter::DoImport( const ImportOptions& options )
{
	// Save
	SaveObjectConfig( TXT("User") );

#ifdef USE_NVIDIA_FUR

	GFSDK_HairSDK* hairSDK = GetRenderer()->GetHairSDK();
	if ( !hairSDK )
	{
		RED_LOG_ERROR( CNAME( CHairWorksImporter ), TXT( "HairWorks libray not initialized" ) );
		return nullptr;
	}

	// checks for reimport
	CFurMeshResource* pMeshRead = nullptr;
	CFurMeshResource* existingResource = nullptr;
	if ( options.m_existingResource )
	{
		existingResource = static_cast< CFurMeshResource* >( options.m_existingResource );
		if ( !existingResource )
		{
			RED_HALT( "Existing resource is not a fur resource while reimporting" );
			return nullptr;
		}
		pMeshRead = existingResource;
	}
	else
	{
		pMeshRead = new CFurMeshResource();
	}

	GFSDK_HairAssetID hairAssetID;
	const char* apxFileName = UNICODE_TO_ANSI( options.m_sourceFilePath.AsChar() );
	GFSDK_HairConversionSettings settings;
	settings.m_targetSceneUnit = 100.0f;
	GFSDK_HAIR_RETURNCODES code = hairSDK->LoadHairAssetFromFile( apxFileName, &hairAssetID, nullptr, &settings );
	if ( code != GFSDK_RETURN_OK )
	{
		RED_LOG_ERROR( CNAME( CHairWorksImporter ), TXT( "Unable to load hair asset" ) );
		return nullptr;
	}

	GFSDK_HairInstanceDescriptor defaultHairInstanceDesc;
	if (  hairSDK->CopyInstanceDescriptorFromAsset( hairAssetID, defaultHairInstanceDesc, 0 ) != GFSDK_RETURN_OK )
	{
		RED_LOG_ERROR( CNAME( CHairWorksImporter ), TXT( "Unable to copy default instance desc" ) );
		return nullptr;
	}

	GFSDK_HairInstanceDescriptor secondHairInstanceDesc;
	if (  hairSDK->CopyInstanceDescriptorFromAsset( hairAssetID, secondHairInstanceDesc, 1 ) != GFSDK_RETURN_OK )
	{
		RED_LOG_ERROR( CNAME( CHairWorksImporter ), TXT( "Unable to copy second target instance desc" ) );
		return nullptr;
	}

	const GFSDK_HairAssetDescriptor* hairAssetDesc = hairSDK->GetHairAssetDescriptor( hairAssetID );

	// Copy HairAssetDesc and HairInstanceDesc into CFurMeshResource members
	LoadHairAssetDesc( pMeshRead, hairAssetDesc );
	
	if ( !existingResource )
	{
		LoadDefaultHairInstanceDesc( pMeshRead, &defaultHairInstanceDesc );
		LoadSecondHairInstanceDesc( pMeshRead, &secondHairInstanceDesc );
	}

	pMeshRead->CreateRenderResource();

	pMeshRead->RecalculateVertexEpsilon();
	return pMeshRead;
#else
	return nullptr;
#endif // USE_NVIDIA_FUR
}
