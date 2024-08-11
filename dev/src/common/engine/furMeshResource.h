/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "meshTypeResource.h"


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//
// IMPORTANT NOTE:
// If you will change some property please check also this function
// EMeshTypePreviewPropertyChangeAction CMeshTypePreviewFurComponent::OnResourcePropertyModified( CName propertyName )
// cause ATM we dont know if property modification should reload asset or refresh.
// So each property modification should match in this funtion. 
// F.ex. texture modification should reload cause it refreshes render proxy atm
// so you have to put property name in this function
// 
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// same as GFSDK_HAIR_STRAND_BLEND_MODE
enum EHairStrandBlendModeType
{
	HSBM_Overwrite,
	HSBM_Multiply,
	HSBM_ADD,
	HSBM_MODULATE,
};

BEGIN_ENUM_RTTI( EHairStrandBlendModeType );
ENUM_OPTION_DESC( TXT( "Overwrite" ),	HSBM_Overwrite );
ENUM_OPTION_DESC( TXT( "Multiply" ),	HSBM_Multiply );
ENUM_OPTION_DESC( TXT( "Add" ),			HSBM_ADD );
ENUM_OPTION_DESC( TXT( "Modulate" ),	HSBM_MODULATE );
END_ENUM_RTTI();

// same as GFSDK_HAIR_COLORIZE_MODE
enum EHairColorizeMode
{
	HCM_NONE,
	HCM_LOD,
	HCM_TANGENTS,
	HCM_MESH_NORMAL,
	HCM_ENV_NORMAL
};

BEGIN_ENUM_RTTI( EHairColorizeMode );
ENUM_OPTION_DESC( TXT( "None" ),	HCM_NONE );
ENUM_OPTION_DESC( TXT( "LOD" ),		HCM_LOD );
ENUM_OPTION_DESC( TXT( "Tangents" ),	HCM_TANGENTS );
ENUM_OPTION_DESC( TXT( "Mesh Normal" ),	HCM_MESH_NORMAL );
ENUM_OPTION_DESC( TXT( "Hair Normal" ),	HCM_ENV_NORMAL );
END_ENUM_RTTI();

// same as GFSDK_HAIR_TEXTURE_CHANNEL
enum EHairTextureChannel
{
	HTC_RED,
	HTC_GREEN,
	HTC_BLUE,
	HTC_ALPHA
};

BEGIN_ENUM_RTTI( EHairTextureChannel )
ENUM_OPTION_DESC( TXT( "RED" ), HTC_RED );
ENUM_OPTION_DESC( TXT( "GREEN" ), HTC_GREEN );
ENUM_OPTION_DESC( TXT( "BLUE" ), HTC_BLUE );
ENUM_OPTION_DESC( TXT( "ALPHA" ), HTC_ALPHA );
END_ENUM_RTTI();

struct SFurVisualizers
{
	DECLARE_RTTI_STRUCT( SFurVisualizers );

	SFurVisualizers();

	Bool		m_drawRenderHairs;			//!< draw render hair
	Bool		m_visualizeBones;			//!< visualize bones
	Bool		m_visualizeCapsules;		//!< visualize collision capsules
	Bool		m_visualizeGuideHairs;		//!< draw guide hairs
	Bool		m_visualizeControlVertices; //!< draw control vertices of guide hairs
	Bool		m_visualizeBoundingBox;		//!< draw bounding box of hairs
	EHairColorizeMode
				m_colorizeMode;				//!< colorize hair based on LOD, normal and tangents
	Bool		m_visualizeCullSphere;		//!< draw cull sphere
	Bool		m_visualizeDiffuseBone;		//!< visualize diffuse bone
	Bool		m_visualizeFrames;			//!< visualize coordinate frames
	Bool		m_visualizeGrowthMesh;		//!< draw growth mesh
	Bool		m_visualizeHairInteractions;//!< draw hair interaction lines
	Uint32		m_visualizeHairSkips;		//!< for per hair visualization, how many hairs to skip?
	Bool		m_visualizeLocalPos;		//!< visualize target pose for bending
	Bool		m_visualizePinConstraints;	//!< whether to visualize pin constraint spheres
	Bool		m_visualizeShadingNormals;	//!< visualize normals used for hair shading
	Bool		m_visualizeSkinnedGuideHairs; //!< draw skinned positions for guide hairs
	Bool		m_visualizeStiffnessBone;	//!< [true/false] visualize stiffness bone
};

BEGIN_CLASS_RTTI( SFurVisualizers );
	PROPERTY_EDIT(m_drawRenderHairs, TXT("draw render hair") );
	PROPERTY_EDIT(m_visualizeBones, TXT("visualize bones") );
	PROPERTY_EDIT(m_visualizeCapsules, TXT("visualize collision capsules") );
	PROPERTY_EDIT(m_visualizeGuideHairs, TXT("draw guide hairs") );
	PROPERTY_EDIT(m_visualizeControlVertices, TXT("draw control vertices of guide hairs") );
	PROPERTY_EDIT(m_visualizeBoundingBox, TXT("draw bounding box of hairs") );
	PROPERTY_EDIT(m_colorizeMode, TXT("colorize hair based on LOD, normal and tangents") );
	PROPERTY_EDIT(m_visualizeCullSphere, TXT("draw cull sphere") );
	PROPERTY_EDIT(m_visualizeDiffuseBone, TXT("visualize diffuse bone") );
	PROPERTY_EDIT(m_visualizeFrames, TXT("visualize coordinate frames") );
	PROPERTY_EDIT(m_visualizeGrowthMesh, TXT("draw growth mesh") );
	PROPERTY_EDIT(m_visualizeHairInteractions, TXT("draw hair interaction lines") );
	PROPERTY_EDIT(m_visualizeHairSkips, TXT("for per hair visualization, how many hairs to skip?") );
	PROPERTY_EDIT(m_visualizeLocalPos, TXT("visualize target pose for bending") );
	PROPERTY_EDIT(m_visualizePinConstraints, TXT("whether to visualize pin constraint spheres") );
	PROPERTY_EDIT(m_visualizeShadingNormals, TXT("visualize normals used for hair shading") );
	PROPERTY_EDIT(m_visualizeSkinnedGuideHairs, TXT("draw skinned positions for guide hairs") );
	PROPERTY_EDIT(m_visualizeStiffnessBone, TXT("[true/false] visualize stiffness bone") );
END_CLASS_RTTI();

struct SFurSimulation
{
	DECLARE_RTTI_STRUCT( SFurSimulation );

	SFurSimulation();

	Bool		m_simulate;					//!< whether to turn on/off simulation
	Float		m_massScale;				//!< [UNIT DEPENDENT] scale of gravity 
	Float		m_damping;					//!< damping to slow down hair motion
	Float		m_friction;					//!< [UNIT DEPENDENT] friction when capsule collision is used
	Float		m_backStopRadius;			//!< [UNIT DEPENDENT] radius of backstop collision
	Float		m_inertiaScale;				//!< Intertia control.  0: no inertia, 1: full inertia
	Float		m_inertiaLimit;				//!< [UINT DEPENDENT] speed limit where everything gets locked to skinned position (e.g. teleport)
	Bool		m_useCollision;				//!< whether to use the sphere/capsule collision or not for hair/body collision

	Vector		m_gravityDir;				//!< gravity force direction (unit vector)
	Vector		m_wind;						//!< [UNIT DEPENDENT] vector force for main wind direction
	Float		m_windNoise;				//!< strength of wind noise
	Float		m_windScaler;				//!< Wind scaler
};

BEGIN_CLASS_RTTI( SFurSimulation );
	PROPERTY_EDIT(m_simulate, TXT("whether to turn on/off simulation") );
	PROPERTY_EDIT(m_massScale, TXT("scale of gravity ") );
	PROPERTY_EDIT(m_damping, TXT("damping to slow down hair motion") );
	PROPERTY_EDIT(m_friction, TXT("friction applied during collision") );
	PROPERTY_EDIT(m_backStopRadius, TXT("radius of backstop collision") );
	PROPERTY_EDIT(m_inertiaScale, TXT("inertial control. 0: no inertia, 1: full inertia") );
	PROPERTY_EDIT(m_inertiaLimit, TXT("speed limit where everything gets locked to skinned position (for teleport, etc.)") );
	PROPERTY_EDIT(m_useCollision, TXT("whether to use the sphere/capsule collision or not for hair/body collision") );
	PROPERTY_EDIT(m_windScaler, TXT("wind scaler") );
	PROPERTY_EDIT(m_windNoise, TXT("wind noise") );
	PROPERTY_EDIT(m_gravityDir, TXT("gravity force direction (unit vector)") );
END_CLASS_RTTI();

struct SFurVolume
{
	DECLARE_RTTI_STRUCT( SFurVolume );

	SFurVolume();

	Float						m_density;				//!< ratio of number of interpolated hairs compared to maximum (64 per face)
	THandle< CBitmapTexture >	m_densityTex;			//!< hair density map [ shape control ]
	EHairTextureChannel			m_densityTexChannel;	//!< hair density map channel
	Bool						m_usePixelDensity;		//!< whether to use per-pixel sampling or per-vertex sampling for density map
	Float						m_lengthNoise;			//!< length variation noise
	Float						m_lengthScale;			//!< length control for growing hair effect
	THandle< CBitmapTexture >	m_lengthTex;			//!< length control [shape control] 
	EHairTextureChannel			m_lengthTexChannel;		//!< length control map channel
};

BEGIN_CLASS_RTTI( SFurVolume );
	PROPERTY_EDIT(m_density, TXT("ratio of number of interpolated hairs compared to maximum (64 per face)") );
	PROPERTY_EDIT(m_densityTex, TXT("hair density map [ shape control ]") );
	PROPERTY_EDIT(m_densityTexChannel, TXT("hair density map channel") );
	PROPERTY_EDIT(m_usePixelDensity, TXT("whether to use per-pixel sampling or per-vertex sampling for density map") );
	PROPERTY_EDIT(m_lengthNoise, TXT("length variation noise") );
	PROPERTY_EDIT(m_lengthScale, TXT("length control for growing hair effect") );
	PROPERTY_EDIT(m_lengthTex, TXT("length control [shape control] ") );
	PROPERTY_EDIT(m_lengthTexChannel, TXT("length control map channel ") );
END_CLASS_RTTI();

struct SFurStrandWidth
{
	DECLARE_RTTI_STRUCT( SFurStrandWidth );

	SFurStrandWidth();

	Float						m_width;				//!< [UNIT DEPENDENT] hair width 
	Float						m_widthRootScale;		//!< scale factor for top side of the strand
	THandle< CBitmapTexture >	m_rootWidthTex;			//!< width at the hair root [ shape control ]
	EHairTextureChannel			m_rootWidthTexChannel;	//!< root width texture channel
	Float						m_widthTipScale;		//!< scale factor for bottom side of the strand
	THandle< CBitmapTexture >	m_tipWidthTex;			//!< width at the hair tip [ shape control ]
	EHairTextureChannel			m_tipWidthTexChannel;	//!< tip width texture channel
	Float						m_widthNoise;			//!< noise factor for hair width noise
};

BEGIN_CLASS_RTTI( SFurStrandWidth );
	PROPERTY_EDIT_RANGE(m_width, TXT("hair width "), 0.0f, FLT_MAX );
	PROPERTY_EDIT(m_widthRootScale, TXT("scale factor for top side of the strand") );
	PROPERTY_EDIT(m_widthTipScale, TXT("scale factor for bottom side of the strand") );
	PROPERTY_EDIT(m_rootWidthTex, TXT("width at the hair root [ shape control ]") );
	PROPERTY_EDIT(m_rootWidthTexChannel, TXT("root width texture channel") );
	PROPERTY_EDIT(m_tipWidthTex, TXT("width at the hair tip [ shape control ]") );
	PROPERTY_EDIT(m_tipWidthTexChannel, TXT("tip width texture channel") );
	PROPERTY_EDIT_RANGE(m_widthNoise, TXT("noise factor for hair width noise"), 0.0f, 1.0f );
END_CLASS_RTTI();

struct SFurStiffness
{
	DECLARE_RTTI_STRUCT( SFurStiffness );

	SFurStiffness();

	Float						m_stiffness;			//!< stiffness to restore to skinned rest shape for hairs
	Float						m_stiffnessStrength;		//!< how strongly hairs move toward the stiffness target
	THandle< CBitmapTexture >	m_stiffnessTex;			//!< stiffness control [ simulation ]
	EHairTextureChannel			m_stiffnessTexChannel;	//!< stiffness texture channel
	Float						m_interactionStiffness;		//!< how strong the hair interaction force is
	Float						m_rootStiffness;		//!< attenuation of stiffness away from the root (stiffer at root, weaker toward tip)
	Float						m_pinStiffness;			//!< [0 - 1.0] stiffness for pin constraints
	THandle< CBitmapTexture >	m_rootStiffnessTex;		//!< stiffness control for root stiffness [simulation]
	EHairTextureChannel			m_rootStiffnessTexChannel; //!< root stiffness texture channel
	Float						m_stiffnessDamping;		//!< how fast hair stiffness gerneated motion decays over time
	Float						m_tipStiffness;			//!< attenuation of stiffness away from the tip (stiffer at tip, weaker toward root)
														//!< Note that when both root and tip stiffness are non-zero, they blend in additive manner.
	Float						m_bendStiffness;		//!< stiffness for bending, useful for long hair

	Bool						m_stiffnessBoneEnable;		//!< [true/false] If true, bone based scaling is used for stiffness
	Uint32						m_stiffnessBoneIndex;		//!< [0 - number of bones] index for the bone which we use as reference position for setting min/max distance
	Uint32						m_stiffnessBoneAxis;		//!< [0 - 2] which axis (X,Y,Z) to use for min/max. Default is 2 (Z-axis).
	Float						m_stiffnessStartDistance;	//!< [In Meters] signed distance from the bone center that specifies start of stiffness scaling
	Float						m_stiffnessEndDistance;		//!< [In Meters] signed distance from the bone center that specifies end of stiffness scaling
	Vector						m_stiffnessBoneCurve;		//! [0 - 1.0] curve values for bone based stiffness control

	Vector						m_stiffnessCurve;			//! [0 - 1.0] curve values for stiffness 
	Vector						m_stiffnessStrengthCurve;	//! [0 - 1.0] curve values for stiffness strength
	Vector						m_stiffnessDampingCurve;	//! [0 - 1.0] curve values for stiffness damping
	Vector						m_bendStiffnessCurve;		//! [0 - 1.0] curve values for bend stiffness
	Vector						m_interactionStiffnessCurve;//! [0 - 1.0] curve values for interaction stiffness
};

BEGIN_CLASS_RTTI( SFurStiffness );
	PROPERTY_EDIT_RANGE(m_stiffness, TXT("stiffness to restore to skinned rest shape for hairs"), 0.0f, 1.0f );
	PROPERTY_EDIT(m_stiffnessStrength, TXT("how strongly hairs move toward the stiffness target") );
	PROPERTY_EDIT(m_stiffnessTex, TXT("stiffness control [ simulation ]") );
	PROPERTY_EDIT(m_stiffnessTexChannel, TXT("stiffness texture channel") );
	PROPERTY_EDIT(m_interactionStiffness, TXT("how strong the hair interaction force is") );
	PROPERTY_EDIT_RANGE(m_pinStiffness, TXT("[0 - 1.0] stiffness for pin constraints"), 0.0f, 1.0f );
	PROPERTY_EDIT_RANGE(m_rootStiffness, TXT("attenuation of stiffness away from the root (stiffer at root, weaker toward tip)"), 0.0f, 1.0f );
	PROPERTY_EDIT(m_rootStiffnessTex, TXT("stiffness control for root stiffness [simulation]") );
	PROPERTY_EDIT(m_rootStiffnessTexChannel, TXT("root stiffness texture channel") );
	PROPERTY_EDIT(m_stiffnessDamping, TXT("how fast hair stiffness gerneated motion decays over time") );
	PROPERTY_EDIT_RANGE(m_tipStiffness, TXT("attenuation of stiffness away from the tip (stiffer at tip, weaker toward root). Note that when both root and tip stiffness are non-zero, they blend in additive manner."), 0.0f, 1.0f );
	PROPERTY_EDIT(m_bendStiffness, TXT("stiffness for bending, useful for long hair") );

	PROPERTY_EDIT(m_stiffnessBoneEnable, TXT("[true/false] If true, bone based scaling is used for stiffness") );
	PROPERTY_EDIT(m_stiffnessBoneIndex, TXT("[0 - number of bones] index for the bone which we use as reference position for setting min/max distance") );
	PROPERTY_EDIT_RANGE(m_stiffnessBoneAxis, TXT("[0 - 2] which axis (X,Y,Z) to use for min/max. Default is 2 (Z-axis)."), 0, 2 );
	PROPERTY_EDIT(m_stiffnessStartDistance, TXT("[In Meters] signed distance from the bone center that specifies start of stiffness scaling") );
	PROPERTY_EDIT(m_stiffnessEndDistance, TXT("[In Meters] signed distance from the bone center that specifies end of stiffness scaling") );
	PROPERTY_EDIT(m_stiffnessBoneCurve, TXT("[0 - 1.0] curve values for bone based stiffness control") );

	PROPERTY_EDIT(m_stiffnessCurve, TXT("[0 - 1.0] curve values for stiffness ") );
	PROPERTY_EDIT(m_stiffnessStrengthCurve, TXT("[0 - 1.0] curve values for stiffness strength") );
	PROPERTY_EDIT(m_stiffnessDampingCurve, TXT("[0 - 1.0] curve values for stiffness damping") );
	PROPERTY_EDIT(m_bendStiffnessCurve, TXT("[0 - 1.0] curve values for bend stiffness") );
	PROPERTY_EDIT(m_interactionStiffnessCurve, TXT("[0 - 1.0] curve values for interaction stiffness") );
END_CLASS_RTTI();

struct SFurClumping
{
	DECLARE_RTTI_STRUCT( SFurClumping );

	SFurClumping();

	Float						m_clumpRoundness;		//!< exponential factor to control roundness of clump shape 
														//!<	(0 = linear cone, clump scale *= power(t, roundness), where t is normalized distance from the root)
	THandle< CBitmapTexture >	m_clumpRoundnessTex;	//!< clump roundness control [ shape control]
	EHairTextureChannel			m_clumpRoundnessTexChannel; //!< clump roundness texture channel
	Float						m_clumpScale;			//!< how clumped each hair face is
	THandle< CBitmapTexture >	m_clumpScaleTex;		//!< clumpiness control [ shape control]
	EHairTextureChannel			m_clumpScaleTexChannel;	//!< clump scale texture channel
	Float						m_clumpNoise;			//!< probability of each hair gets clumped 
														//!<	(0 = all hairs get clumped, 1 = clump scale is randomly distributed from 0 to 1)
	THandle< CBitmapTexture >	m_clumpNoiseTex;		//!< clumping noise [ shape control]
	EHairTextureChannel			m_clumpNoiseTexChannel; //!< clump noise texture channel

	Uint32						m_clumpNumSubclumps;	//!< number of clumps per triangle
};

BEGIN_CLASS_RTTI( SFurClumping );
	PROPERTY_EDIT(m_clumpScale, TXT("how clumped each hair face is") );
	PROPERTY_EDIT(m_clumpScaleTex, TXT("clumpiness control [ shape control]") );
	PROPERTY_EDIT(m_clumpScaleTexChannel, TXT("clump scale texture channel") );
	PROPERTY_EDIT(m_clumpRoundness, TXT("exponential factor to control roundness of clump shape  (0 = linear cone, clump scale *= power(t, roundness), where t is normalized distance from the root)") );
	PROPERTY_EDIT(m_clumpRoundnessTex, TXT("clump roundness control [ shape control]") );
	PROPERTY_EDIT(m_clumpRoundnessTexChannel, TXT("clump roundness texture channel") );
	PROPERTY_EDIT(m_clumpNoise, TXT("probability of each hair gets clumped (0 = all hairs get clumped, 1 = clump scale is randomly distributed from 0 to 1)") );
	PROPERTY_EDIT(m_clumpNoiseTex, TXT("clumping noise [ shape control]") );
	PROPERTY_EDIT(m_clumpNoiseTexChannel, TXT("clump noise texture channel") );
	PROPERTY_EDIT_RANGE(m_clumpNumSubclumps, TXT("number of clumps per triangle"), 0, 8);
END_CLASS_RTTI();

struct SFurWaveness
{
	DECLARE_RTTI_STRUCT( SFurWaveness );

	SFurWaveness();

	// FurWaveness : Scale, ScaleTexture, ScaleNoise, Frequency, FrequencyNoise, RootStraighten, StrandWaviness, ClumpWaviness
	Float		m_waveScale;				//!< [UNIT DEPENDENT] size of waves for hair waviness 
	THandle< CBitmapTexture >	m_waveScaleTex;			//!< waviness scale [ shape control ]
	EHairTextureChannel			m_waveScaleTexChannel;	//!< waviness scale texture channel
	Float		m_waveScaleNoise;			//!< noise factor for the wave scale
	Float		m_waveFreq;					//!< wave frequency (1.0 = one sine wave along hair length)
	THandle< CBitmapTexture >	m_waveFreqTex;			//!< waviness frequency [ shape control ]
	EHairTextureChannel			m_waveFreqTexChannel;	//!< waviness frequency texture channel
	Float		m_waveFreqNoise;			//!< noise factor for the wave frequency
	Float		m_waveRootStraighten;		//!< cutoff from root to make root not move by waviness [0-1]
	Float		m_waveStrand;				//!< waviness at strand level
	Float		m_waveClump;				//!< waviness at clump level
};

BEGIN_CLASS_RTTI( SFurWaveness );
	PROPERTY_EDIT(m_waveScale, TXT("size of waves for hair waviness ") );
	PROPERTY_EDIT(m_waveScaleTex, TXT("waviness scale [ shape control ]") );
	PROPERTY_EDIT(m_waveScaleTexChannel, TXT("waviness scale texture channel") );
	PROPERTY_EDIT(m_waveScaleNoise, TXT("noise factor for the wave scale") );
	PROPERTY_EDIT(m_waveFreq, TXT("wave frequency (1.0 = one sine wave along hair length)") );
	PROPERTY_EDIT(m_waveFreqTex, TXT("waviness frequency [ shape control ]") );
	PROPERTY_EDIT(m_waveFreqTexChannel, TXT("waviness frequency texture channel") );
	PROPERTY_EDIT(m_waveFreqNoise, TXT("noise factor for the wave frequency") );
	PROPERTY_EDIT_RANGE(m_waveRootStraighten, TXT("cutoff from root to make root not move by waviness [0-1]"), 0.0f, 1.0f );
	PROPERTY_EDIT_RANGE(m_waveStrand, TXT("waviness at strand level"), 0.0f, 1.0f );
	PROPERTY_EDIT_RANGE(m_waveClump, TXT("waviness at clump level"), 0.0f, 1.0f );
END_CLASS_RTTI();

struct SFurPhysicalMaterials
{
	DECLARE_RTTI_STRUCT( SFurPhysicalMaterials );

	SFurSimulation		m_simulation;
	SFurVolume			m_volume;
	SFurStrandWidth		m_strandWidth;
	SFurStiffness		m_stiffness;
	SFurClumping		m_clumping;
	SFurWaveness		m_waveness;
};

BEGIN_CLASS_RTTI( SFurPhysicalMaterials );
	PROPERTY_EDIT(m_simulation, TXT("Simulation Controls") );
	PROPERTY_EDIT(m_volume, TXT("Volume Controls") );
	PROPERTY_EDIT(m_strandWidth, TXT("Strand Width Controls") );
	PROPERTY_EDIT(m_stiffness, TXT("Stiffness Controls") );
	PROPERTY_EDIT(m_clumping, TXT("Clumping Controls") );
	PROPERTY_EDIT(m_waveness, TXT("Waveness Controls") );
END_CLASS_RTTI();

struct SFurColor
{
	DECLARE_RTTI_STRUCT( SFurColor );

	SFurColor();

	Float		m_rootAlphaFalloff;				//!< falloff factor for alpha transition from root 
	Color		m_rootColor;					//!< hair color for root (when root color textures are not used)
	THandle< CBitmapTexture > m_rootColorTex;	//!< color at the hair root [ shading ]
	Color		m_tipColor;						//!< hair color for tip (when tip color textures are not used)
	THandle< CBitmapTexture > m_tipColorTex;	//!< color at the hair tip [ shading ]
	Float		m_rootTipColorWeight;			//!< blend factor between root and tip color in addition to hair length
	Float		m_rootTipColorFalloff;			//!< falloff factor for root/tip color interpolation
	THandle< CBitmapTexture > m_strandTex;		//!< texture along hair strand [ shading ]
	EHairStrandBlendModeType m_strandBlendMode;	//!< blend mode when strand texture is used. Supported mode are defined in GFSDK_HAIR_STRAND_BLEND_MODE.
	Float		m_strandBlendScale;				//!< scale strand texture before blend
	Float		m_textureBrightness;			//!< additional control for texture brightness
	Float		m_ambientEnvScale;				//!< ambient scale for environtment probe
};

BEGIN_CLASS_RTTI( SFurColor );
PROPERTY_EDIT(m_rootAlphaFalloff, TXT("falloff factor for alpha transition from root") );
PROPERTY_EDIT(m_rootColor, TXT("hair color for root (when root color textures are not used)") );
PROPERTY_EDIT(m_rootColorTex, TXT("color at the hair root [ shading ]") );
PROPERTY_EDIT(m_tipColor, TXT("hair color for tip (when tip color textures are not used)") );
PROPERTY_EDIT(m_tipColorTex, TXT("color at the hair tip [ shading ]") );
PROPERTY_EDIT_RANGE(m_rootTipColorWeight, TXT("blend factor between root and tip color in addition to hair length"), 0.0f, 1.0f );
PROPERTY_EDIT_RANGE(m_rootTipColorFalloff, TXT("falloff factor for root/tip color interpolation"), 0.0f, 1.0f );
PROPERTY_EDIT(m_strandTex, TXT("texture along hair strand [ shading ]") );
PROPERTY_EDIT(m_strandBlendMode, TXT("blend mode when strand texture is used") );
PROPERTY_EDIT(m_strandBlendScale, TXT("scale strand texture before blend") );
PROPERTY_EDIT(m_textureBrightness, TXT("additional texture brightness control") );
PROPERTY_EDIT(m_ambientEnvScale, TXT("scale factor for env probe ambient term") );
END_CLASS_RTTI();

struct SFurDiffuse
{
	DECLARE_RTTI_STRUCT( SFurDiffuse );

	SFurDiffuse();

	Float		m_diffuseBlend;				//!< blend factor between tangent based hair lighting vs normal based skin lighting (0 = all tangent, 1 = all normal)
	Float		m_diffuseScale;				//!< scale factor for diffuse term only
	Float		m_diffuseHairNormalWeight;	//!< blend factor between mesh normal and hair normal
	Uint32		m_diffuseBoneIndex;			//!< bone index for model center for diffuse shading
	Vector		m_diffuseBoneLocalPos;		//!< offset vector for model center from the diffuse bone 
	Float		m_diffuseNoiseScale;		//!< amount of noise scale
	Float		m_diffuseNoiseFreqU;		//!< luminance noise along u direction
	Float		m_diffuseNoiseFreqV;		//!< luminance noise along v direction
	Float		m_diffuseNoiseGain;			//!< amount of noise gain
};

BEGIN_CLASS_RTTI( SFurDiffuse );
	PROPERTY_EDIT(m_diffuseBlend, TXT("blend factor between tangent based hair lighting vs normal based skin lighting (0 = all tangent, 1 = all normal)") );
	PROPERTY_EDIT(m_diffuseScale, TXT("scale factor for diffuse term only") );
	PROPERTY_EDIT(m_diffuseHairNormalWeight, TXT("blend factor between mesh normal and hair normal") );
	PROPERTY_EDIT(m_diffuseBoneIndex, TXT("bone index to find model center for diffuse hair shading") );
	PROPERTY_EDIT(m_diffuseBoneLocalPos, TXT("offset vector for the bone based model center for diffuse shading") );

	PROPERTY_EDIT(m_diffuseNoiseScale, TXT("amount of noise scale") );
	PROPERTY_EDIT(m_diffuseNoiseFreqU, TXT("luminance noise along u direction") );
	PROPERTY_EDIT(m_diffuseNoiseFreqV, TXT("luminance noise along v direction") );
	PROPERTY_EDIT(m_diffuseNoiseGain, TXT("amount of noise gain") );
END_CLASS_RTTI();

struct SFurSpecular
{
	DECLARE_RTTI_STRUCT( SFurSpecular );

	SFurSpecular();

	Color		m_specularColor;				//!< specular lighing color (when specular textures are not used)		
	THandle< CBitmapTexture > m_specularTex;	//!< specularity control [shading ] 
	Float		m_specularPrimary;				//!< primary specular factor
	Float		m_specularPowerPrimary;			//!< primary specular power exponent
	Float		m_specularPrimaryBreakup;		//!< primary specular factor
	Float		m_specularSecondary;			//!< secondary specular factor
	Float		m_specularPowerSecondary;		//!< secondary specular power exponent		
	Float		m_specularSecondaryOffset;		//!< secondary highlight shift offset along tangents
	Float		m_specularNoiseScale;			//!< amount of specular noise
	Float		m_specularEnvScale;				//!< amount of specular scale from env probe
};

BEGIN_CLASS_RTTI( SFurSpecular );
	PROPERTY_EDIT(m_specularColor, TXT("specular lighing color (when specular textures are not used)") );
	PROPERTY_EDIT(m_specularTex, TXT("specularity control [shading ] ") );
	PROPERTY_EDIT(m_specularPrimary, TXT("primary specular factor") );
	PROPERTY_EDIT(m_specularPowerPrimary, TXT("primary specular power exponent") );
	PROPERTY_EDIT(m_specularPrimaryBreakup, TXT("primary specular breakup factor") );
	PROPERTY_EDIT(m_specularSecondary, TXT("secondary specular factor") );
	PROPERTY_EDIT(m_specularPowerSecondary, TXT("secondary specular power exponent") );
	PROPERTY_EDIT(m_specularSecondaryOffset, TXT("secondary highlight shift offset along tangents") );
	PROPERTY_EDIT(m_specularNoiseScale, TXT("amount of specular noise") );
	PROPERTY_EDIT(m_specularEnvScale, TXT("amount of specular scale from env probe") );
END_CLASS_RTTI();

struct SFurGlint
{
	DECLARE_RTTI_STRUCT( SFurGlint );

	SFurGlint();
	
	Float		m_glintStrength;			//!< strength of the glint noise
	Float		m_glintCount;				//!< number of glint sparklets along each hair
	Float		m_glintExponent;			//!< power component of glint strength
};

BEGIN_CLASS_RTTI( SFurGlint );
	PROPERTY_EDIT(m_glintStrength, TXT("strength of the glint noise") );
	PROPERTY_EDIT(m_glintCount, TXT("number of glint sparklets along each hair") );
	PROPERTY_EDIT(m_glintExponent, TXT("power component of glint strength") );
END_CLASS_RTTI();

struct SFurShadow
{
	DECLARE_RTTI_STRUCT( SFurShadow );

	SFurShadow();

	Float		m_shadowSigma;				//!< [UNIT DEPENDENT] shadow absorption factor
	Float		m_shadowDensityScale;		//!< density scale factor to reduce hair density for shadow map rendering
	Bool		m_castShadows;				//!< this hair casts shadows on the scene
	Bool		m_receiveShadows;			//!< this hair receives shadows from the scene
};

BEGIN_CLASS_RTTI( SFurShadow );
	PROPERTY_EDIT(m_shadowSigma, TXT("shadow absorption factor") );
	PROPERTY_EDIT(m_shadowDensityScale, TXT("density scale factor to reduce hair density for shadow map rendering") );
	PROPERTY_EDIT(m_castShadows, TXT("this hair casts shadows on the scene") );
	PROPERTY_EDIT(m_receiveShadows, TXT("this hair receives shadows from the scene") );
END_CLASS_RTTI();

struct SFurGraphicalMaterials
{
	DECLARE_RTTI_STRUCT( SFurGraphicalMaterials );

	SFurColor	m_color;
	SFurDiffuse m_diffuse;
	SFurSpecular m_specular;
	SFurGlint	m_glint;
	SFurShadow	m_shadow;
};

BEGIN_CLASS_RTTI( SFurGraphicalMaterials );
	PROPERTY_EDIT(m_color, TXT("Color Controls") );
	PROPERTY_EDIT(m_diffuse, TXT("Diffuse Controls") );
	PROPERTY_EDIT(m_specular, TXT("Specular Controls") );
	PROPERTY_EDIT(m_glint, TXT("Glint Controls") );
	PROPERTY_EDIT(m_shadow, TXT("Shadow Controls") );
END_CLASS_RTTI();

struct SFurCulling
{
	DECLARE_RTTI_STRUCT( SFurCulling );

	SFurCulling();

	Bool		m_useViewfrustrumCulling;	//!< when this is on, density for hairs outside view are set to 0. Use this option when fur is in a closeup.
	Bool		m_useBackfaceCulling;		//!< when this is on, density for hairs growing from backfacing faces will be set to 0
	Float		m_backfaceCullingThreshold;	//!< threshold to determine backface, note that this value should be slightly smaller 0 to avoid hairs at the silhouette from disappearing
};

BEGIN_CLASS_RTTI( SFurCulling );
	PROPERTY_EDIT(m_useViewfrustrumCulling, TXT("when this is on, density for hairs outside view are set to 0. Use this option when fur is in a closeup") );
	PROPERTY_EDIT(m_useBackfaceCulling, TXT("when this is on, density for hairs growing from backfacing faces will be set to 0") );
	PROPERTY_EDIT_RANGE(m_backfaceCullingThreshold, TXT("threshold to determine backface, note that this value should be slightly smaller 0 to avoid hairs at the silhouette from disappearing"), -1.0f, 1.0f );
END_CLASS_RTTI();

struct SFurDistanceLOD
{
	DECLARE_RTTI_STRUCT( SFurDistanceLOD );

	SFurDistanceLOD();

	Bool		m_enableDistanceLOD;		//!< whether to enable lod for far away object (distance LOD)
	Float		m_distanceLODStart;			//!< [UNIT DEPENDENT] distance (in scene unit) to camera where fur will start fading out (by reducing density)
	Float		m_distanceLODEnd;			//!< [UNIT DEPENDENT] distance (in scene unit) to camera where fur will completely disappear (and stop simulating)
	Float		m_distanceLODFadeStart;		//!< [UNIT DEPENDENT] distance (in scene unit) to camera where fur starts fading with alpha
	Float		m_distanceLODWidth;//!< width scale factor for distance LOD
	Float		m_distanceLODDensity;	//!< ratio of number of interpolated hairs compared to maximum in distance LOD.
};

BEGIN_CLASS_RTTI( SFurDistanceLOD );
	PROPERTY_EDIT(m_enableDistanceLOD, TXT("whether to enable lod for far away object (distance LOD)") );
	PROPERTY_EDIT_RANGE(m_distanceLODStart, TXT("distance (in scene unit) to camera where fur will start fading out (by reducing density)"), 0.0f, 10.0f );
	PROPERTY_EDIT_RANGE(m_distanceLODEnd, TXT("distance (in scene unit) to camera where fur will completely disappear (and stop simulating)"), 0.0f, 1000.0f );
	PROPERTY_EDIT_RANGE(m_distanceLODFadeStart, TXT("distance (in scene unit) to camera where fur starts fading with alpha"), 0.0f, 1000.0f );
	PROPERTY_EDIT_RANGE(m_distanceLODWidth, TXT("hair width that can change when distance LOD density is triggered by lod mechanism"), 0.0f, 1000.0f );
	PROPERTY_EDIT(m_distanceLODDensity, TXT("ratio of number of interpolated hairs compared to maximum in distance LOD") );
END_CLASS_RTTI();

struct SFurDetailLOD
{
	DECLARE_RTTI_STRUCT( SFurDetailLOD );

	SFurDetailLOD();

	Bool		m_enableDetailLOD;			//!< whether to enable lod for close object 
	Float		m_detailLODStart;			//!< [UNIT DEPENDENT] distance (in scene unit) to camera where fur will start getting denser toward closeup density
	Float		m_detailLODEnd;				//!< [UNIT DEPENDENT] distance (in scene unit) to camera where fur will get full closeup density value
	Float		m_detailLODWidth;	//!< [UNIT DEPENDENT] hair width that can change when close up density is triggered by closeup lod mechanism
	Float		m_detailLODDensity;	//!< ratio of number of interpolated hairs compared to maximum in detail LOD.
};

BEGIN_CLASS_RTTI( SFurDetailLOD );
	PROPERTY_EDIT(m_enableDetailLOD, TXT("whether to enable lod for close object (closeup LOD)") );
	PROPERTY_EDIT_RANGE(m_detailLODStart, TXT("distance (in scene unit) to camera where fur will start increasing detail in closeup"), 0.0f, 10.0f );
	PROPERTY_EDIT_RANGE(m_detailLODEnd, TXT("distance (in scene unit) to camera where fur will receive full density in closeup"), 0.0f, 10.0f );
	PROPERTY_EDIT(m_detailLODWidth, TXT("hair width that can change when detail LOD density is triggered by lod mechanism") );
	PROPERTY_EDIT_RANGE(m_detailLODDensity, TXT("ratio of number of interpolated hairs compared to maximum in detail LOD"), 0.0f, 10.0f );
END_CLASS_RTTI();

struct SFurLevelOfDetail
{
	DECLARE_RTTI_STRUCT( SFurLevelOfDetail );

	SFurLevelOfDetail();

	Bool				m_enableLOD;				//!< whether to enable/disable entire lod scheme
	SFurCulling			m_culling;
	SFurDistanceLOD		m_distanceLOD;
	SFurDetailLOD		m_detailLOD;
	Uint32				m_priority;					//!< priority level to determine whether to use fur or polygonal mesh
};

BEGIN_CLASS_RTTI( SFurLevelOfDetail );
	PROPERTY_EDIT(m_enableLOD, TXT("whether to enable/disable entire lod scheme") );
	PROPERTY_EDIT(m_culling, TXT("Culling") );
	PROPERTY_EDIT(m_distanceLOD, TXT("Distance LOD") );
	PROPERTY_EDIT(m_detailLOD, TXT("Detail LOD") );
	PROPERTY_EDIT_RANGE(m_priority, TXT("Lower priority value will prefer HairWorks even on lower settings"), 0, 1 );
END_CLASS_RTTI();

struct SFurMaterialSet
{
	DECLARE_RTTI_STRUCT( SFurMaterialSet );

	SFurPhysicalMaterials	m_physicalMaterials;
	SFurGraphicalMaterials	m_graphicalMaterials;
	Bool					m_useWetness;
};

BEGIN_CLASS_RTTI( SFurMaterialSet );
	PROPERTY_EDIT(m_useWetness, TXT("Should use wetness material") );
	PROPERTY_EDIT(m_physicalMaterials, TXT("Physical Materials") );
	PROPERTY_EDIT(m_graphicalMaterials, TXT("Graphical Materials") );
END_CLASS_RTTI();


class CFurMeshResource : public CMeshTypeResource
{
	friend class CHairWorksImporter;

	DECLARE_ENGINE_RESOURCE_CLASS( CFurMeshResource, CMeshTypeResource, "redfur", "Fur mesh resource" );

public:
	// This is equivalent to NVHairDesc.
	TDynArray< Vector >		m_positions;
	TDynArray< Vector2 >	m_uvs;
	TDynArray< Uint32 >		m_endIndices;		// The size of this == number of guide curves.
	TDynArray< Uint32 >		m_faceIndices;
	TDynArray< Vector >		m_boneIndices;
	TDynArray< Vector >		m_boneWeights;
	Uint32					m_boneCount;
	TDynArray< CName >		m_boneNames;		// names for each bone used to check if bone names match. buffer size should be at least GFSDK_HAIR_MAX_STRING * 'm_NumBones'.
	TDynArray< Matrix >		m_boneRigMatrices;
	TDynArray< Int32 >		m_boneParents;		// parent index for each bone.  if this is a root bone, the index will be -1. buffer size should be at least sizoef(gfsdk_S32) * m_NumBones.
	TDynArray< Matrix >		m_bindPoses;		// bind pose matrices for each bone. buffer size should be at least sizeof(gfsdk_float4x4) * m_NumBones.

	// bone sphere array
	TDynArray< Vector >		m_boneSphereLocalPosArray;	// array of offset value with regard to bind position of the bone
	TDynArray< Uint32 >		m_boneSphereIndexArray;		// array of index for the bone where the collision sphere is attached to
	TDynArray< Float >		m_boneSphereRadiusArray;	// array of radius for the collision sphere
	
	// pin constraints array
	TDynArray< Vector >		m_pinConstraintsLocalPosArray;		// array of offset value with regard to pin constraints
	TDynArray< Uint32 >		m_pinConstraintsIndexArray;			// array of index for the bone where the pin constraints sphere is attached to
	TDynArray< Float >		m_pinConstraintsRadiusArray;		// array of radius for the pin constraints sphere

	// bone capsule indices
	TDynArray< Uint32 >		m_boneCapsuleIndices;		// index to the bone spheres, size of this array must be 2 * m_NumBoneCapsules;

	// shading control
	Bool					m_useTextures;				//!< use textures 

	SFurVisualizers			m_visualizers;
	SFurPhysicalMaterials	m_physicalMaterials;
	SFurGraphicalMaterials	m_graphicalMaterials;
	SFurLevelOfDetail		m_levelOfDetail;

	SFurMaterialSet			m_materialSets;
	Float					m_materialWeight;

	// other options
	Uint32					m_splineMultiplier;			//!< number of spline points per cv 
	Float					m_importUnitsScale;			//!< import unit scale. editable parameters are not scaled when importing

private:
	typedef TDynArray< Float, MC_RenderData > TSkeletonBoneEpsilonArray;
	TSkeletonBoneEpsilonArray		m_boneVertexEpsilons;

public:
	CFurMeshResource();
	virtual ~CFurMeshResource();


	virtual void CreateRenderResource() override;


	Uint32 numGuideCurves() const	{ return m_endIndices.Size(); }

	inline Uint32 getStartAndEnd(Uint32 hairId, Uint32 &start, Uint32 &end) const
	{
		start = (hairId == 0) ? 0 : (m_endIndices[hairId-1] + 1);
		end = m_endIndices[hairId];

		return (end-start+1);
	}

	virtual Uint32			GetBoneCount() const { return m_boneCount; }
	virtual const CName*	GetBoneNames() const { return m_boneNames.TypedData(); }
	virtual const Matrix*	GetBoneRigMatrices() const { return m_boneRigMatrices.TypedData(); }
	
	Float GetWindScaler() const { return m_physicalMaterials.m_simulation.m_windScaler; }

	void CreateHairAssetDesc( struct GFSDK_HairAssetDescriptor* assetDesc ) const;
	void CreateDefaultHairInstanceDesc( struct GFSDK_HairInstanceDescriptor* instanceDesc ) const;
	void CreateTargetHairInstanceDesc( struct GFSDK_HairInstanceDescriptor* instanceDesc, int materialID ) const;
	
	Float GetMaterialWeight() const { return m_materialWeight; }
	void SetMaterialWeight( Float weight ) { m_materialWeight = weight; }

	virtual const Float* GetBoneVertexEpsilons() const { return m_boneVertexEpsilons.TypedData(); }

	virtual void OnPreSave();

	void ForceFullyLoad() override;

	//! Property values was changed in the editor
	virtual void OnPropertyPostChange( IProperty* property ) override;
	Bool IsUsingWetness() const { return m_materialSets.m_useWetness; }

#ifndef NO_EDITOR
	void EditorRefresh( IRenderProxy* renderProxy );
#endif

private:
	// recalculate vertx epsilon for fur
	void RecalculateVertexEpsilon();

	
};

BEGIN_CLASS_RTTI( CFurMeshResource );
PARENT_CLASS( CMeshTypeResource );
PROPERTY(m_positions);
PROPERTY(m_uvs);
PROPERTY(m_endIndices);
PROPERTY(m_faceIndices);
PROPERTY(m_boneIndices);
PROPERTY(m_boneWeights);
PROPERTY(m_boneCount);
PROPERTY(m_boneNames);
PROPERTY(m_boneRigMatrices);
PROPERTY(m_boneParents);
PROPERTY(m_bindPoses);
PROPERTY(m_boneSphereLocalPosArray);
PROPERTY(m_boneSphereIndexArray);
PROPERTY(m_boneSphereRadiusArray);
PROPERTY(m_boneCapsuleIndices);
PROPERTY(m_boneVertexEpsilons);
PROPERTY(m_pinConstraintsLocalPosArray);
PROPERTY(m_pinConstraintsIndexArray);
PROPERTY(m_pinConstraintsRadiusArray);

PROPERTY_EDIT_RANGE(m_splineMultiplier, TXT("Spline Multipliers"), 1, 4);

PROPERTY_EDIT(m_visualizers, TXT("Visualizers") );
PROPERTY_EDIT(m_physicalMaterials, TXT("Physical Materials") );
PROPERTY_EDIT(m_graphicalMaterials, TXT("Graphical Materials") );
PROPERTY_EDIT(m_levelOfDetail, TXT("Level Of Detail") );
PROPERTY_EDIT_NOSERIALIZE(m_materialWeight, TXT("Material Weight") ); //not save intentionally
PROPERTY_EDIT(m_materialSets, TXT("Additional Materials") );

// other options
PROPERTY( m_importUnitsScale );


END_CLASS_RTTI();
