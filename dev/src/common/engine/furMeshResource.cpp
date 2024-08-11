#include "build.h"
#include "furMeshResource.h"
#include "bitmapTexture.h"
#include "renderProxy.h"
#include "renderCommands.h"

#ifdef RED_PLATFORM_WINPC
#ifdef USE_NVIDIA_FUR
#include "../../../external/NvidiaHair/include/GFSDK_HairWorks.h"
#endif // USE_NVIDIA_FUR
#endif

IMPLEMENT_RTTI_ENUM( EHairStrandBlendModeType );
IMPLEMENT_RTTI_ENUM( EHairColorizeMode );
IMPLEMENT_RTTI_ENUM( EHairTextureChannel );

IMPLEMENT_ENGINE_CLASS( SFurVisualizers );
IMPLEMENT_ENGINE_CLASS( SFurSimulation );
IMPLEMENT_ENGINE_CLASS( SFurVolume );
IMPLEMENT_ENGINE_CLASS( SFurStrandWidth );
IMPLEMENT_ENGINE_CLASS( SFurStiffness );
IMPLEMENT_ENGINE_CLASS( SFurClumping );
IMPLEMENT_ENGINE_CLASS( SFurWaveness );
IMPLEMENT_ENGINE_CLASS( SFurPhysicalMaterials );
IMPLEMENT_ENGINE_CLASS( SFurColor );
IMPLEMENT_ENGINE_CLASS( SFurDiffuse );
IMPLEMENT_ENGINE_CLASS( SFurSpecular );
IMPLEMENT_ENGINE_CLASS( SFurGlint );
IMPLEMENT_ENGINE_CLASS( SFurShadow );
IMPLEMENT_ENGINE_CLASS( SFurGraphicalMaterials );
IMPLEMENT_ENGINE_CLASS( SFurCulling );
IMPLEMENT_ENGINE_CLASS( SFurDistanceLOD );
IMPLEMENT_ENGINE_CLASS( SFurDetailLOD );
IMPLEMENT_ENGINE_CLASS( SFurLevelOfDetail );
IMPLEMENT_ENGINE_CLASS( SFurMaterialSet );
IMPLEMENT_ENGINE_CLASS( CFurMeshResource );

SFurVisualizers::SFurVisualizers()
	: m_drawRenderHairs( true )
	, m_visualizeBones( false )
	, m_visualizeCapsules( false )
	, m_visualizeGuideHairs( false )
	, m_visualizeControlVertices( false )
	, m_visualizeBoundingBox( false )
	, m_colorizeMode( HCM_NONE )
	, m_visualizePinConstraints( false )
	, m_visualizeGrowthMesh( false )
	, m_visualizeCullSphere( false )
	, m_visualizeDiffuseBone( false )
	, m_visualizeHairInteractions( false )
	, m_visualizeSkinnedGuideHairs( false )
	, m_visualizeStiffnessBone( false )
	, m_visualizeFrames( false )
	, m_visualizeLocalPos( false )
	, m_visualizeShadingNormals( false )
	, m_visualizeHairSkips( 0 )
{
}

SFurSimulation::SFurSimulation()
	: m_simulate( true )
	, m_massScale( 10.0f )
	, m_damping( 0.0f )
	, m_friction( 0.0f )
	, m_backStopRadius( 0.0f )
	, m_inertiaScale( 1.0f )
	, m_inertiaLimit( 1000.0f )
	, m_useCollision( false )
	, m_gravityDir( 0.0f, 0.0f, -1.f)
	, m_wind( Vector::ZERO_3D_POINT )
	, m_windNoise( 1.f )
	, m_windScaler( 1.f )	
{
}

SFurVolume::SFurVolume()
	: m_density( 0.5f )
	, m_densityTex( nullptr )
	, m_densityTexChannel( HTC_RED )
	, m_usePixelDensity( false )
	, m_lengthNoise( 1.0f )
	, m_lengthScale( 1.0f )
	, m_lengthTex( nullptr )
	, m_lengthTexChannel( HTC_RED )
{
}

SFurStrandWidth::SFurStrandWidth()
	: m_width( 3.0f )
	, m_widthRootScale( 1.0f )
	, m_rootWidthTex( nullptr )
	, m_rootWidthTexChannel( HTC_RED )
	, m_widthTipScale( 0.1f )
	, m_tipWidthTex( nullptr )
	, m_tipWidthTexChannel( HTC_RED )
	, m_widthNoise( 0.0f )
{
}

SFurStiffness::SFurStiffness()
	: m_stiffness( 0.5f )
	, m_stiffnessStrength( 1.0f )
	, m_stiffnessTex( nullptr )
	, m_stiffnessTexChannel( HTC_RED )
	, m_interactionStiffness( 0.0f )
	, m_rootStiffness( 0.5f )
	, m_pinStiffness( 1.0f )
	, m_rootStiffnessTex( nullptr )
	, m_rootStiffnessTexChannel( HTC_RED )
	, m_stiffnessDamping( 0.0f )
	, m_tipStiffness( 0.0f )
	, m_bendStiffness( 0.0f )
	, m_stiffnessBoneEnable( false )
	, m_stiffnessBoneIndex( 1000 )
	, m_stiffnessBoneAxis( 2 )
	, m_stiffnessStartDistance( 0.0f )
	, m_stiffnessEndDistance( 0.0f )
{
	m_stiffnessBoneCurve.Set4( 1.0f );
	m_stiffnessCurve.Set4( 1.0f );
	m_stiffnessStrengthCurve.Set4( 1.0f );
	m_stiffnessDampingCurve.Set4( 1.0f );
	m_bendStiffnessCurve.Set4( 1.0f );
	m_interactionStiffnessCurve.Set4( 1.0f );
}

SFurClumping::SFurClumping()
	: m_clumpRoundness( 1.0f )
	, m_clumpScale( 0.0f )
	, m_clumpScaleTex( nullptr )
	, m_clumpScaleTexChannel( HTC_RED )
	, m_clumpRoundnessTex( nullptr )
	, m_clumpRoundnessTexChannel( HTC_RED )
	, m_clumpNoise( 0.0f )
	, m_clumpNumSubclumps( 0 )
	, m_clumpNoiseTex( nullptr )
	, m_clumpNoiseTexChannel( HTC_RED )
{
}

SFurWaveness::SFurWaveness()
	: m_waveScale( 0.0f )
	, m_waveScaleTex( nullptr )
	, m_waveScaleTexChannel( HTC_RED )
	, m_waveScaleNoise(0.5f)
	, m_waveFreq( 3.0f )
	, m_waveFreqTex( nullptr )
	, m_waveFreqTexChannel( HTC_RED )
	, m_waveFreqNoise( 0.5f )
	, m_waveRootStraighten( 0.0f )
	, m_waveStrand( 0.0f )
	, m_waveClump( 1.0f )
{
}

SFurColor::SFurColor()
	: m_rootAlphaFalloff( 0.0f )
	, m_rootColor( Color::WHITE )
	, m_rootColorTex( nullptr )
	, m_tipColor( Color::WHITE )
	, m_tipColorTex( nullptr )
	, m_rootTipColorWeight( 0.5f )
	, m_rootTipColorFalloff( 1.0f )
	, m_strandTex( nullptr )
	, m_strandBlendMode( HSBM_Overwrite )
	, m_strandBlendScale( 1.0f )
	, m_textureBrightness( 1.0f )
	, m_ambientEnvScale( 1.0f )
{
}

SFurDiffuse::SFurDiffuse()
	: m_diffuseBlend( 1.0f )
	, m_diffuseScale( 1.0f )
	, m_diffuseHairNormalWeight( 0.0f )
	, m_diffuseBoneIndex( 1000 )
	, m_diffuseBoneLocalPos( 0.0f, 0.0f, 0.0f )
	, m_diffuseNoiseScale( 0.0f )
	, m_diffuseNoiseFreqU( 64.0f )
	, m_diffuseNoiseFreqV( 64.0f )
	, m_diffuseNoiseGain( 0.0f )
{
}

SFurSpecular::SFurSpecular()
	: m_specularColor( Color::WHITE )
	, m_specularTex( nullptr )
	, m_specularPrimary( 0.1f )
	, m_specularPowerPrimary( 100.0f )
	, m_specularPrimaryBreakup( 0.0f )
	, m_specularSecondary( 0.05f )
	, m_specularPowerSecondary( 20.0f )
	, m_specularSecondaryOffset( 0.1f )
	, m_specularNoiseScale( 0.0f )
	, m_specularEnvScale( 0.25f )
{
}

SFurGlint::SFurGlint()
	: m_glintStrength( 0.0f )
	, m_glintCount( 256.0f )
	, m_glintExponent( 2.0f )
{
}

SFurShadow::SFurShadow()
	: m_shadowSigma( 1.0f )
	, m_shadowDensityScale( 1.0f )
	, m_castShadows( true )
	, m_receiveShadows( true )
{
}

SFurCulling::SFurCulling()
	: m_useViewfrustrumCulling( true )
	, m_useBackfaceCulling( false )
	, m_backfaceCullingThreshold( -0.2f )
{
}

SFurLevelOfDetail::SFurLevelOfDetail()
	: m_enableLOD(false)
	, m_priority( 1 )
{
}

SFurDistanceLOD::SFurDistanceLOD()
	: m_enableDistanceLOD( true )
	, m_distanceLODStart( 5.0f ) 
	, m_distanceLODEnd( 10.0f ) 
	, m_distanceLODFadeStart( 20.0f ) 
	, m_distanceLODDensity( 0.0f )
	, m_distanceLODWidth( 1.0f )
{
}

SFurDetailLOD::SFurDetailLOD()
	: m_enableDetailLOD( true )
	, m_detailLODStart( 2.0f ) 
	, m_detailLODEnd( 1.0f ) 
	, m_detailLODDensity( 1.0f )
	, m_detailLODWidth( 1.0f )
{
}

CFurMeshResource::CFurMeshResource()
	: m_useTextures(true)
	, m_importUnitsScale( .01f )
	, m_materialWeight(0.0f)
	, m_splineMultiplier(4)
{
}

CFurMeshResource::~CFurMeshResource()
{
}


void CFurMeshResource::CreateRenderResource()
{
	SAFE_RELEASE( m_renderResource );

	if ( GRender && !GRender->IsDeviceLost() )
	{
		m_renderResource = GRender->UploadFurMesh( this );
	}
}


void CFurMeshResource::CreateHairAssetDesc( struct GFSDK_HairAssetDescriptor* hairAssetDesc ) const
{
#ifdef USE_NVIDIA_FUR

	hairAssetDesc->m_sceneUnit		= 100.0f; // 100 centimeter = 1 unit
	hairAssetDesc->m_NumVertices	= m_positions.Size();
	hairAssetDesc->m_NumGuideHairs	= numGuideCurves();
	hairAssetDesc->m_NumFaces		= m_uvs.Size() / 3;
	hairAssetDesc->m_NumBoneSpheres = m_boneSphereIndexArray.Size();
	hairAssetDesc->m_NumBoneCapsules = m_boneCapsuleIndices.Size() / 2;
	hairAssetDesc->m_NumPinConstraints = m_pinConstraintsIndexArray.Size();

	hairAssetDesc->m_pVertices		= new gfsdk_float3[hairAssetDesc->m_NumVertices];
	hairAssetDesc->m_pEndIndices	= new gfsdk_U32[hairAssetDesc->m_NumGuideHairs];
	hairAssetDesc->m_pFaceIndices	= new gfsdk_U32[m_faceIndices.Size()];
	hairAssetDesc->m_pFaceUVs		= new gfsdk_float2[m_uvs.Size()];

	hairAssetDesc->m_pBoneIndices = new gfsdk_float4[ m_boneIndices.Size() ];
	hairAssetDesc->m_pBoneWeights = new gfsdk_float4[ m_boneWeights.Size() ];

	Uint32 numBones = m_boneNames.Size();
	hairAssetDesc->m_pBoneNames = new gfsdk_char[ GFSDK_HAIR_MAX_STRING * numBones ];
	hairAssetDesc->m_pBindPoses = new gfsdk_float4x4[ numBones ];
	hairAssetDesc->m_pBoneParents = new gfsdk_S32[ numBones ];

	if ( !m_boneSphereIndexArray.Empty() )
	{
		hairAssetDesc->m_pBoneSpheres = new gfsdk_boneSphere[ m_boneSphereIndexArray.Size() ];
	}
	if ( !m_boneCapsuleIndices.Empty() )
	{
		hairAssetDesc->m_pBoneCapsuleIndices = new gfsdk_U32[ m_boneCapsuleIndices.Size() ];
	}
	if ( !m_pinConstraintsIndexArray.Empty() )
	{
		hairAssetDesc->m_pPinConstraints = new gfsdk_boneSphere[ m_pinConstraintsIndexArray.Size() ];
	}

	ASSERT(sizeof(gfsdk_float2) == sizeof(Vector2));
	ASSERT(sizeof(gfsdk_float3) == sizeof(Vector3));
	ASSERT(sizeof(gfsdk_float4) == sizeof(Vector));
	for (Uint32 i=0; i!=m_positions.Size(); ++i)
		memcpy(&hairAssetDesc->m_pVertices[i], &m_positions[i], sizeof(gfsdk_float3));

	for (Uint32 i=0; i!=numGuideCurves(); ++i)
		hairAssetDesc->m_pEndIndices[i] = m_endIndices[i];

	for (Uint32 i=0; i!=m_faceIndices.Size(); ++i)
		hairAssetDesc->m_pFaceIndices[i] = m_faceIndices[i];

	for (Uint32 i=0; i!=m_uvs.Size(); ++i)
		memcpy(&hairAssetDesc->m_pFaceUVs[i], &m_uvs[i], sizeof(gfsdk_float2));

	ASSERT( m_boneIndices.Size() == m_boneWeights.Size(), TXT("Different amount of boneindices than boneweights") );

	hairAssetDesc->m_NumBones = m_boneCount;

	for (Uint32 i=0; i!=m_boneIndices.Size(); ++i)
	{
		memcpy(&hairAssetDesc->m_pBoneIndices[i], &m_boneIndices[i], sizeof(gfsdk_float4));
		memcpy(&hairAssetDesc->m_pBoneWeights[i], &m_boneWeights[i], sizeof(gfsdk_float4));
	}

	ASSERT(m_boneNames.Size() == numBones);
	for (Uint32 i=0; i!=numBones; ++i)
	{
		// copy bone name
		gfsdk_char* targetName = &hairAssetDesc->m_pBoneNames[i*GFSDK_HAIR_MAX_STRING];
		gfsdk_char* srcName = UNICODE_TO_ANSI( m_boneNames[ i ].AsChar() );
		strcpy(targetName, srcName);
		// copy bindPoses
		if ( m_bindPoses.Size() == numBones )
		{
			memcpy(&hairAssetDesc->m_pBindPoses[ i ], m_bindPoses[ i ].AsFloat(), sizeof(gfsdk_float4x4) );
		}
		else
		{
			Matrix temp;
			temp.SetIdentity();
			memcpy(&hairAssetDesc->m_pBindPoses[ i ], temp.AsFloat(), sizeof(gfsdk_float4x4));
		}
		// copy boneParents
		if ( m_boneParents.Size() == numBones )
		{
			hairAssetDesc->m_pBoneParents[ i ] = m_boneParents[ i ];
		}
		else
		{
			hairAssetDesc->m_pBoneParents[ i ] = -1;
		}
	}

	ASSERT( m_boneSphereIndexArray.Size() == m_boneSphereLocalPosArray.Size(), TXT("Different amount of bone sphere local pos than bone sphere index") );
	ASSERT( m_boneSphereIndexArray.Size() == m_boneSphereRadiusArray.Size(), TXT("Different amount of bone sphere radius than bone sphere index") );

	for (Uint32 i=0; i!=m_boneSphereIndexArray.Size(); ++i)
	{
		hairAssetDesc->m_pBoneSpheres[i].m_BoneSphereIndex = m_boneSphereIndexArray[i];
		memcpy(&hairAssetDesc->m_pBoneSpheres[i].m_BoneSphereLocalPos, &m_boneSphereLocalPosArray[i], sizeof(gfsdk_float3));
		hairAssetDesc->m_pBoneSpheres[i].m_BoneSphereRadius = m_boneSphereRadiusArray[i];
	}
	for (Uint32 i=0; i!=m_boneCapsuleIndices.Size(); ++i)
	{
		hairAssetDesc->m_pBoneCapsuleIndices[i] = m_boneCapsuleIndices[i];
	}

	ASSERT( m_pinConstraintsIndexArray.Size() == m_pinConstraintsLocalPosArray.Size(), TXT("Different amount of pin constraints local pos than pin constraints index") );
	ASSERT( m_pinConstraintsIndexArray.Size() == m_pinConstraintsRadiusArray.Size(), TXT("Different amount of pin constraints radius than pin constraints index") );

	for (Uint32 i=0; i!=m_pinConstraintsIndexArray.Size(); ++i)
	{
		hairAssetDesc->m_pPinConstraints[i].m_BoneSphereIndex = m_pinConstraintsIndexArray[i];
		memcpy(&hairAssetDesc->m_pPinConstraints[i].m_BoneSphereLocalPos, &m_pinConstraintsLocalPosArray[i], sizeof(gfsdk_float3));
		hairAssetDesc->m_pPinConstraints[i].m_BoneSphereRadius = m_pinConstraintsRadiusArray[i];
	}

#endif // USE_NVIDIA_FUR
}

namespace
{
#ifdef USE_NVIDIA_FUR
	gfsdk_float4 ConvertVector4(const Vector& v)
	{
		gfsdk_float4 temp;
		temp.x = v.X; temp.y = v.Y; temp.z = v.Z; temp.w = v.W;
		return temp;
	}

	gfsdk_float3 ConvertVector3(const Vector& v)
	{
		gfsdk_float3 temp;
		temp.x = v.X; temp.y = v.Y; temp.z = v.Z;
		return temp;
	}
#endif

#ifdef USE_NVIDIA_FUR
void UpdatePhysicalMaterial(
	const SFurPhysicalMaterials& physicalMaterials,
	struct GFSDK_HairInstanceDescriptor* instanceDesc)
{
	// physical / strand width
	instanceDesc->m_width						= physicalMaterials.m_strandWidth.m_width;
	instanceDesc->m_widthNoise					= physicalMaterials.m_strandWidth.m_widthNoise;
	instanceDesc->m_widthRootScale				= physicalMaterials.m_strandWidth.m_widthRootScale;
	instanceDesc->m_widthTipScale				= physicalMaterials.m_strandWidth.m_widthTipScale;
	instanceDesc->m_textureChannels[ GFSDK_HAIR_TEXTURE_ROOT_WIDTH ] = ( GFSDK_HAIR_TEXTURE_CHANNEL )physicalMaterials.m_strandWidth.m_rootWidthTexChannel;
	instanceDesc->m_textureChannels[ GFSDK_HAIR_TEXTURE_TIP_WIDTH ] = ( GFSDK_HAIR_TEXTURE_CHANNEL )physicalMaterials.m_strandWidth.m_tipWidthTexChannel;

	// physical / clumping
	instanceDesc->m_clumpNoise					= physicalMaterials.m_clumping.m_clumpNoise;
	instanceDesc->m_clumpRoundness				= physicalMaterials.m_clumping.m_clumpRoundness;
	instanceDesc->m_clumpScale					= physicalMaterials.m_clumping.m_clumpScale;
	instanceDesc->m_clumpNumSubclumps			= physicalMaterials.m_clumping.m_clumpNumSubclumps;
	instanceDesc->m_textureChannels[ GFSDK_HAIR_TEXTURE_CLUMP_SCALE ] = ( GFSDK_HAIR_TEXTURE_CHANNEL )physicalMaterials.m_clumping.m_clumpScaleTexChannel;
	instanceDesc->m_textureChannels[ GFSDK_HAIR_TEXTURE_CLUMP_NOISE ] = ( GFSDK_HAIR_TEXTURE_CHANNEL )physicalMaterials.m_clumping.m_clumpNoiseTexChannel;
	instanceDesc->m_textureChannels[ GFSDK_HAIR_TEXTURE_CLUMP_ROUNDNESS ] = ( GFSDK_HAIR_TEXTURE_CHANNEL )physicalMaterials.m_clumping.m_clumpRoundnessTexChannel;

	// physical / volume
	instanceDesc->m_density						= physicalMaterials.m_volume.m_density;
	instanceDesc->m_lengthNoise					= physicalMaterials.m_volume.m_lengthNoise;
	instanceDesc->m_lengthScale					= physicalMaterials.m_volume.m_lengthScale;
	instanceDesc->m_usePixelDensity				= physicalMaterials.m_volume.m_usePixelDensity;
	instanceDesc->m_textureChannels[ GFSDK_HAIR_TEXTURE_DENSITY ] = ( GFSDK_HAIR_TEXTURE_CHANNEL )physicalMaterials.m_volume.m_densityTexChannel;
	instanceDesc->m_textureChannels[ GFSDK_HAIR_TEXTURE_LENGTH ] = ( GFSDK_HAIR_TEXTURE_CHANNEL )physicalMaterials.m_volume.m_lengthTexChannel;
	
	// physical / waveness
	instanceDesc->m_waveScale					= physicalMaterials.m_waveness.m_waveScale;
	instanceDesc->m_waveScaleNoise				= physicalMaterials.m_waveness.m_waveScaleNoise;
	instanceDesc->m_waveFreq					= physicalMaterials.m_waveness.m_waveFreq;
	instanceDesc->m_waveFreqNoise				= physicalMaterials.m_waveness.m_waveFreqNoise;
	instanceDesc->m_waveRootStraighten			= physicalMaterials.m_waveness.m_waveRootStraighten;
	instanceDesc->m_waveStrand					= physicalMaterials.m_waveness.m_waveStrand;
	instanceDesc->m_waveClump					= physicalMaterials.m_waveness.m_waveClump;
	instanceDesc->m_textureChannels[ GFSDK_HAIR_TEXTURE_WAVE_SCALE ] = ( GFSDK_HAIR_TEXTURE_CHANNEL )physicalMaterials.m_waveness.m_waveScaleTexChannel;
	instanceDesc->m_textureChannels[ GFSDK_HAIR_TEXTURE_WAVE_FREQ ] = ( GFSDK_HAIR_TEXTURE_CHANNEL )physicalMaterials.m_waveness.m_waveFreqTexChannel;

	// physical / simulation
	instanceDesc->m_backStopRadius				= physicalMaterials.m_simulation.m_backStopRadius;
	instanceDesc->m_useCollision				= physicalMaterials.m_simulation.m_useCollision;
	instanceDesc->m_damping						= physicalMaterials.m_simulation.m_damping;
	instanceDesc->m_friction					= physicalMaterials.m_simulation.m_friction;
	instanceDesc->m_massScale					= physicalMaterials.m_simulation.m_massScale;
	instanceDesc->m_gravityDir					= ConvertVector3( physicalMaterials.m_simulation.m_gravityDir );
	instanceDesc->m_inertiaScale				= physicalMaterials.m_simulation.m_inertiaScale;
	instanceDesc->m_inertiaLimit				= physicalMaterials.m_simulation.m_inertiaLimit;
	instanceDesc->m_simulate					= physicalMaterials.m_simulation.m_simulate;
	instanceDesc->m_wind						= ConvertVector3( physicalMaterials.m_simulation.m_wind );
	instanceDesc->m_windNoise					= physicalMaterials.m_simulation.m_windNoise;

	// physical / stiffness
	instanceDesc->m_bendStiffness				= physicalMaterials.m_stiffness.m_bendStiffness;
	instanceDesc->m_stiffness					= physicalMaterials.m_stiffness.m_stiffness;
	instanceDesc->m_stiffnessStrength			= physicalMaterials.m_stiffness.m_stiffnessStrength;
	instanceDesc->m_interactionStiffness		= physicalMaterials.m_stiffness.m_interactionStiffness;
	instanceDesc->m_pinStiffness				= physicalMaterials.m_stiffness.m_pinStiffness;
	instanceDesc->m_rootStiffness				= physicalMaterials.m_stiffness.m_rootStiffness;
	instanceDesc->m_stiffnessDamping			= physicalMaterials.m_stiffness.m_stiffnessDamping;
	instanceDesc->m_tipStiffness				= physicalMaterials.m_stiffness.m_tipStiffness;
	instanceDesc->m_stiffnessCurve				= ConvertVector4( physicalMaterials.m_stiffness.m_stiffnessCurve );
	instanceDesc->m_stiffnessStrengthCurve		= ConvertVector4( physicalMaterials.m_stiffness.m_stiffnessStrengthCurve );
	instanceDesc->m_stiffnessDampingCurve		= ConvertVector4( physicalMaterials.m_stiffness.m_stiffnessDampingCurve );
	instanceDesc->m_bendStiffnessCurve			= ConvertVector4( physicalMaterials.m_stiffness.m_bendStiffnessCurve );

	instanceDesc->m_stiffnessBoneEnable			= physicalMaterials.m_stiffness.m_stiffnessBoneEnable;
	instanceDesc->m_stiffnessBoneIndex			= physicalMaterials.m_stiffness.m_stiffnessBoneIndex;
	instanceDesc->m_stiffnessBoneAxis			= physicalMaterials.m_stiffness.m_stiffnessBoneAxis;
	instanceDesc->m_stiffnessStartDistance		= physicalMaterials.m_stiffness.m_stiffnessStartDistance;
	instanceDesc->m_stiffnessEndDistance		= physicalMaterials.m_stiffness.m_stiffnessEndDistance;
	instanceDesc->m_stiffnessBoneCurve			= ConvertVector4( physicalMaterials.m_stiffness.m_stiffnessBoneCurve );

	instanceDesc->m_textureChannels[ GFSDK_HAIR_TEXTURE_STIFFNESS ] = ( GFSDK_HAIR_TEXTURE_CHANNEL )physicalMaterials.m_stiffness.m_stiffnessTexChannel;
	instanceDesc->m_textureChannels[ GFSDK_HAIR_TEXTURE_ROOT_STIFFNESS ] = ( GFSDK_HAIR_TEXTURE_CHANNEL )physicalMaterials.m_stiffness.m_rootStiffnessTexChannel;
}
#endif

#ifdef USE_NVIDIA_FUR
void UpdateGraphicalMaterial(
	const SFurGraphicalMaterials& graphicalMaterials,
	struct GFSDK_HairInstanceDescriptor* instanceDesc)
{
	// graphical / shadow
	instanceDesc->m_shadowDensityScale			= graphicalMaterials.m_shadow.m_shadowDensityScale;
	instanceDesc->m_shadowSigma					= graphicalMaterials.m_shadow.m_shadowSigma;
	instanceDesc->m_castShadows					= graphicalMaterials.m_shadow.m_castShadows;
	instanceDesc->m_receiveShadows				= graphicalMaterials.m_shadow.m_receiveShadows;

	// graphical / diffuse
	instanceDesc->m_diffuseBlend				= graphicalMaterials.m_diffuse.m_diffuseBlend;
	instanceDesc->m_diffuseScale				= graphicalMaterials.m_diffuse.m_diffuseScale;
	instanceDesc->m_diffuseHairNormalWeight		= graphicalMaterials.m_diffuse.m_diffuseHairNormalWeight;
	instanceDesc->m_diffuseBoneIndex			= graphicalMaterials.m_diffuse.m_diffuseBoneIndex;
	instanceDesc->m_diffuseBoneLocalPos			= ConvertVector3(graphicalMaterials.m_diffuse.m_diffuseBoneLocalPos);
	instanceDesc->m_diffuseNoiseFreqU			= graphicalMaterials.m_diffuse.m_diffuseNoiseFreqU;
	instanceDesc->m_diffuseNoiseFreqV			= graphicalMaterials.m_diffuse.m_diffuseNoiseFreqV;
	instanceDesc->m_diffuseNoiseScale			= graphicalMaterials.m_diffuse.m_diffuseNoiseScale;
	instanceDesc->m_diffuseNoiseGain			= graphicalMaterials.m_diffuse.m_diffuseNoiseGain;

	// graphical / color
	instanceDesc->m_ambientEnvScale				= graphicalMaterials.m_color.m_ambientEnvScale;
	instanceDesc->m_textureBrightness			= graphicalMaterials.m_color.m_textureBrightness;
	instanceDesc->m_rootAlphaFalloff			= graphicalMaterials.m_color.m_rootAlphaFalloff;
	instanceDesc->m_rootColor					= ConvertVector4( graphicalMaterials.m_color.m_rootColor.ToVector() );
	instanceDesc->m_tipColor					= ConvertVector4( graphicalMaterials.m_color.m_tipColor.ToVector() );
	instanceDesc->m_rootTipColorWeight			= graphicalMaterials.m_color.m_rootTipColorWeight;
	instanceDesc->m_rootTipColorFalloff			= graphicalMaterials.m_color.m_rootTipColorFalloff;
	instanceDesc->m_strandBlendMode				= graphicalMaterials.m_color.m_strandBlendMode;
	instanceDesc->m_strandBlendScale			= graphicalMaterials.m_color.m_strandBlendScale;

	// graphical / specular
	instanceDesc->m_specularColor				= ConvertVector4( graphicalMaterials.m_specular.m_specularColor.ToVector() );
	instanceDesc->m_specularNoiseScale			= graphicalMaterials.m_specular.m_specularNoiseScale;
	instanceDesc->m_specularEnvScale			= graphicalMaterials.m_specular.m_specularEnvScale;
	instanceDesc->m_specularPrimary				= graphicalMaterials.m_specular.m_specularPrimary;
	instanceDesc->m_specularPrimaryBreakup		= graphicalMaterials.m_specular.m_specularPrimaryBreakup;
	instanceDesc->m_specularSecondary			= graphicalMaterials.m_specular.m_specularSecondary;
	instanceDesc->m_specularSecondaryOffset		= graphicalMaterials.m_specular.m_specularSecondaryOffset;
	instanceDesc->m_specularPowerPrimary		= graphicalMaterials.m_specular.m_specularPowerPrimary;
	instanceDesc->m_specularPowerSecondary		= graphicalMaterials.m_specular.m_specularPowerSecondary;

	// graphical / glint
	instanceDesc->m_glintStrength				= graphicalMaterials.m_glint.m_glintStrength;
	instanceDesc->m_glintCount					= graphicalMaterials.m_glint.m_glintCount;
	instanceDesc->m_glintExponent				= graphicalMaterials.m_glint.m_glintExponent;
}
#endif

#ifdef USE_NVIDIA_FUR
void UpdateVisualizers(
	const SFurVisualizers& visualizers,
	struct GFSDK_HairInstanceDescriptor* instanceDesc)
{
	// visualizer
	instanceDesc->m_visualizeBones				= visualizers.m_visualizeBones;
	instanceDesc->m_visualizeCapsules			= visualizers.m_visualizeCapsules;
	instanceDesc->m_visualizeGuideHairs			= visualizers.m_visualizeGuideHairs;
	instanceDesc->m_visualizeControlVertices	= visualizers.m_visualizeControlVertices;
	instanceDesc->m_visualizeBoundingBox		= visualizers.m_visualizeBoundingBox;
	instanceDesc->m_drawRenderHairs				= visualizers.m_drawRenderHairs;
	instanceDesc->m_colorizeMode				= visualizers.m_colorizeMode;
	instanceDesc->m_visualizeCullSphere			= visualizers.m_visualizeCullSphere;
	instanceDesc->m_visualizeDiffuseBone		= visualizers.m_visualizeDiffuseBone;
	instanceDesc->m_visualizeFrames				= visualizers.m_visualizeFrames;
	instanceDesc->m_visualizeGrowthMesh			= visualizers.m_visualizeGrowthMesh;
	instanceDesc->m_visualizeHairInteractions	= visualizers.m_visualizeHairInteractions;
	instanceDesc->m_visualizeHairSkips			= visualizers.m_visualizeHairSkips;
	instanceDesc->m_visualizeLocalPos			= visualizers.m_visualizeLocalPos;
	instanceDesc->m_visualizePinConstraints		= visualizers.m_visualizePinConstraints;
	instanceDesc->m_visualizeShadingNormals		= visualizers.m_visualizeShadingNormals;
	instanceDesc->m_visualizeSkinnedGuideHairs	= visualizers.m_visualizeSkinnedGuideHairs;
	instanceDesc->m_visualizeStiffnessBone		= visualizers.m_visualizeStiffnessBone;
}
#endif

#ifdef USE_NVIDIA_FUR
void UpdateLevelOfDetail(
	const SFurLevelOfDetail& levelOfDetail,
	struct GFSDK_HairInstanceDescriptor* instanceDesc)
{
	// lod / distance lod
	instanceDesc->m_enableLOD					= levelOfDetail.m_enableLOD;			
	instanceDesc->m_enableDistanceLOD			= levelOfDetail.m_distanceLOD.m_enableDistanceLOD;
	instanceDesc->m_distanceLODStart			= levelOfDetail.m_distanceLOD.m_distanceLODStart;
	instanceDesc->m_distanceLODEnd				= levelOfDetail.m_distanceLOD.m_distanceLODEnd;
	instanceDesc->m_distanceLODFadeStart		= levelOfDetail.m_distanceLOD.m_distanceLODFadeStart;
	instanceDesc->m_distanceLODDensity			= levelOfDetail.m_distanceLOD.m_distanceLODDensity;
	instanceDesc->m_distanceLODWidth			= levelOfDetail.m_distanceLOD.m_distanceLODWidth;

	// lod / detailed lod
	instanceDesc->m_enableDetailLOD				= levelOfDetail.m_detailLOD.m_enableDetailLOD;
	instanceDesc->m_detailLODStart				= levelOfDetail.m_detailLOD.m_detailLODStart;
	instanceDesc->m_detailLODEnd				= levelOfDetail.m_detailLOD.m_detailLODEnd;
	instanceDesc->m_detailLODDensity			= levelOfDetail.m_detailLOD.m_detailLODDensity;
	instanceDesc->m_detailLODWidth				= levelOfDetail.m_detailLOD.m_detailLODWidth;

	// lod / culling
	instanceDesc->m_useViewfrustrumCulling		= levelOfDetail.m_culling.m_useViewfrustrumCulling;
	instanceDesc->m_useBackfaceCulling			= levelOfDetail.m_culling.m_useBackfaceCulling;
	instanceDesc->m_backfaceCullingThreshold	= levelOfDetail.m_culling.m_backfaceCullingThreshold;
}
#endif

} // namespace

void CFurMeshResource::CreateDefaultHairInstanceDesc( struct GFSDK_HairInstanceDescriptor* instanceDesc ) const
{
#ifdef USE_NVIDIA_FUR

	instanceDesc->m_useTextures					= m_useTextures;

	// graphical material
	UpdateGraphicalMaterial(m_graphicalMaterials, instanceDesc);

	// physical material
	UpdatePhysicalMaterial(m_physicalMaterials, instanceDesc);

	// visualizer
	UpdateVisualizers(m_visualizers, instanceDesc);

	// lod / distance lod
	UpdateLevelOfDetail(m_levelOfDetail, instanceDesc);

	Matrix identity = Matrix::IDENTITY;

	// init model to world matrix
	Red::System::MemoryCopy( &instanceDesc->m_modelToWorld._11, identity.AsFloat(), sizeof(Matrix) );

	// update spline multiplier
	instanceDesc->m_splineMultiplier = m_splineMultiplier;

#endif // USE_NVIDIA_FUR
}

void CFurMeshResource::CreateTargetHairInstanceDesc( struct GFSDK_HairInstanceDescriptor* instanceDesc, int ) const
{
#ifdef USE_NVIDIA_FUR

	instanceDesc->m_useTextures					= m_useTextures;

	// graphical material
	UpdateGraphicalMaterial(m_materialSets.m_graphicalMaterials, instanceDesc);

	// physical material
	UpdatePhysicalMaterial(m_materialSets.m_physicalMaterials, instanceDesc);

	// visualizer
	UpdateVisualizers(m_visualizers, instanceDesc);

	// lod / distance lod
	UpdateLevelOfDetail(m_levelOfDetail, instanceDesc);

	Matrix identity = Matrix::IDENTITY;

	// init model to world matrix
	Red::System::MemoryCopy( &instanceDesc->m_modelToWorld._11, identity.AsFloat(), sizeof(Matrix) );

	// update spline multiplier
	instanceDesc->m_splineMultiplier = m_splineMultiplier;

#endif // USE_NVIDIA_FUR
}

void CFurMeshResource::RecalculateVertexEpsilon()
{
	const Uint32 numBones = m_boneNames.Size();
	if ( numBones > 0 )
	{
		m_boneVertexEpsilons.Resize( numBones );
	
		// Calculate distance from BBox
		Vector halfDiagonal = (m_boundingBox.Max-m_boundingBox.Min)/2;
		Float distToBone = halfDiagonal.Mag3();
		// Grow by 20% like CMesh does
		distToBone *= 1.2f;

		// write it to array
		for ( Uint32 i=0; i<numBones; ++i )
		{
			// Get max
			m_boneVertexEpsilons[ i ] = Max< Float >( m_boneVertexEpsilons[ i ], distToBone );
		}
	}
}

void CFurMeshResource::OnPreSave()
{
	TBaseClass::OnPreSave();
	RecalculateVertexEpsilon();
}


void CFurMeshResource::ForceFullyLoad()
{
	if ( m_renderResource == nullptr )
	{
		CreateRenderResource();
	}
}


void CFurMeshResource::OnPropertyPostChange( IProperty* property )
{
	// Pass to base class
	TBaseClass::OnPropertyPostChange( property );
	{
		if ( property->GetName() == TXT("autoHideDistance") )
		{
			const Float blendingOffset = 5.f;
			m_levelOfDetail.m_distanceLOD.m_distanceLODEnd = m_autoHideDistance - blendingOffset;
		}
		// TODO: remove this check after PROPETY_EDIT_RANGE start work
		if ( property->GetName() == TXT("splineMultiplier") )
		{
			m_splineMultiplier = Clamp< Uint32 >( m_splineMultiplier, 1, 4 );
		}
	}
}

#ifndef NO_EDITOR
void CFurMeshResource::EditorRefresh( IRenderProxy* renderProxy )
{
	RED_ASSERT( renderProxy->GetType() == RPT_Fur, TXT("Updating different render proxy.") );
	if( renderProxy->GetType() == RPT_Fur )
	{

#ifdef USE_NVIDIA_FUR
	// update visuals for hairworks api
	GFSDK_HairInstanceDescriptor* newParams = new GFSDK_HairInstanceDescriptor;
	CreateDefaultHairInstanceDesc( newParams );
	( new CRenderCommand_EditorSetFurParams( renderProxy, newParams, 0 ) )->Commit();

	// create target material like wetness/underwater etc
	newParams = new GFSDK_HairInstanceDescriptor;
	CreateTargetHairInstanceDesc( newParams, 1 );
	( new CRenderCommand_EditorSetFurParams( renderProxy, newParams, 1 ) )->Commit();
#endif //USE_NVIDIA_FUR

	}
}
#endif //NO_EDITOR
