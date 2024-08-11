/// List of SYSTEM GLOBAL constants for vertex and pixel shader
/// It's esential to recompile all shaders after changing any of those lines

#include "globalConstants.fx"

/// Register
#ifdef __cplusplus

	#ifndef REG
		#define REG( _name, _reg, _type )				const Red::System::Uint32 _name = (_reg);
		#define REGI( _name, _reg, _type )				const Red::System::Uint32 _name = (_reg);
		#define REGB( _name, _reg, _type )				const Red::System::Uint32 _name = (_reg);
		#define REG_ARRAY( _name,_reg, _type, _size )	const Red::System::Uint32 _name = (_reg);
		#define SYS_SAMPLER( _name,_reg )				const Red::System::Uint32 _name = (_reg);
		#define SYS_TEXTURE_NO_SAMPLER( _name,_reg )	const Red::System::Uint32 _name = (_reg);
		#define VERTEXSHADER 1
		#define PIXELSHADER 1
	#endif

	#ifndef START_CB
		#define START_CB( _name, _reg )		// EMPTY
		#define END_CB						// EMPTY
	#endif
															
#endif

////////////////////////////////////////////////
/// Pixel shader samplers
////////////////////////////////////////////////

/// Common pixel shader sampler indices
SYS_SAMPLER( PSSMP_NormalsFitting,			13	)
SYS_SAMPLER( PSSMP_SceneColor,				6	)
SYS_SAMPLER( PSSMP_SceneDepth,				15	)	// TerrainClipMap bound to 15 too, but only for shadow mask. SceneDepth only bound for forward passes and some other stuff after deferred is done
SYS_SAMPLER( PSSMP_UVDissolve,				12	)
SYS_TEXTURE_NO_SAMPLER( PSSMP_GlobalShadowAndSSAO,	17	)
SYS_TEXTURE_NO_SAMPLER( PSSMP_Dissolve,				18	)

////////////////////////////////////////////////
/// Pixel shader constants
////////////////////////////////////////////////
	
#ifdef PIXELSHADER

#ifndef __cplusplus
#include "globalConstantBuffers.fx"
#endif

// TODO: This is a junk constant buffer. There shouldn't be anything like that at all. Whenever you can, ONLY remove stuff from it.
START_CB( FrequentPixelConsts, 2 )
REG( PSC_Custom0,							0,		float4 )	// frequent, TODO: get rid of it!
REG( PSC_Custom1,							1,		float4 )	// frequent, TODO: get rid of it!
REG( PSC_TransparencyParams,				2,		float4 )	// frequent, TODO: get rid of it!
REG( PSC_VSMData,							3,		float4 )	// frequent, TODO: get rid of it!
REG( PSC_SelectionEffect,					4,		float4 )	// frequent, TODO: get rid of it!
REG( PSC_ConstColor,						5,		float4 )	// frequent, TODO: get rid of it!
REG( PSC_HitProxyColor,						6,		float4 )	// frequent, TODO: get rid of it!
REG( PSC_ColorOne,							7,		float4x4 )	// frequent, TODO: get rid of it!				[MATRIX]
REG( PSC_ColorTwo,							11,		float4x4 )	// frequent, TODO: get rid of it!				[MATRIX]
REG( PSC_MorphRatio,						15,		float4 )	// frequent, TODO: get rid of it!
REG( PSC_CharactersLightingBoost,			16,		float4 )	// frequent, TODO: get rid of it!
REG( PSC_FXColor,							17,		float4 )	// frequent, TODO: get rid of it!

// This should be visible to both PS and VS. Then the vertex shader could, for example, skip processing the clipping ellipse based on it.
REG( PSC_DiscardFlags,						18,		float4 )	// TODO: Move this somewhere better
END_CB

#define PSC_Frequent_First		PSC_Custom0
#define PSC_Frequent_Last		PSC_DiscardFlags

START_CB( CustomPixelConsts, 3 )
REG( PSC_Custom_0,							20,		float4 )	// postprocess, TODO: remove usages from anything that isn't a postprocess
REG( PSC_Custom_1,							21,		float4 )	// postprocess, TODO: remove usages from anything that isn't a postprocess
REG( PSC_Custom_2,							22,		float4 )	// postprocess, TODO: remove usages from anything that isn't a postprocess
REG( PSC_Custom_3,							23,		float4 )	// postprocess, TODO: remove usages from anything that isn't a postprocess
REG( PSC_Custom_4,							24,		float4 )	// postprocess, TODO: remove usages from anything that isn't a postprocess
REG( PSC_Custom_5,							25,		float4 )	// postprocess, TODO: remove usages from anything that isn't a postprocess
REG( PSC_Custom_6,							26,		float4 )	// postprocess, TODO: remove usages from anything that isn't a postprocess
REG( PSC_Custom_7,							27,		float4 )	// postprocess, TODO: remove usages from anything that isn't a postprocess
REG( PSC_Custom_8,							28,		float4 )	// postprocess, TODO: remove usages from anything that isn't a postprocess
REG( PSC_Custom_9,							29,		float4 )	// postprocess, TODO: remove usages from anything that isn't a postprocess 
REG( PSC_Custom_10,							30,		float4 )	// postprocess, TODO: remove usages from anything that isn't a postprocess
REG( PSC_Custom_11,							31,		float4 )	// postprocess, TODO: remove usages from anything that isn't a postprocess
REG( PSC_Custom_12,							32,		float4 )	// postprocess, TODO: remove usages from anything that isn't a postprocess
REG( PSC_Custom_13,							33,		float4 )	// postprocess, TODO: remove usages from anything that isn't a postprocess
REG( PSC_Custom_14,							34,		float4 )	// postprocess, TODO: remove usages from anything that isn't a postprocess
REG( PSC_Custom_15,							35,		float4 )	// postprocess, TODO: remove usages from anything that isn't a postprocess
REG( PSC_Custom_16,							36,		float4 )	// postprocess, TODO: remove usages from anything that isn't a postprocess
REG( PSC_Custom_17,							37,		float4 )	// postprocess, TODO: remove usages from anything that isn't a postprocess
REG( PSC_Custom_18,							38,		float4 )	// postprocess, TODO: remove usages from anything that isn't a postprocess								  
REG( PSC_Custom_19,							39,		float4 )	// postprocess, TODO: remove usages from anything that isn't a postprocess								  
REG( PSC_Custom_20,							40,		float4 )	// postprocess, TODO: remove usages from anything that isn't a postprocess								  
REG( PSC_Custom_Matrix,						41,		float4x4 )	// postprocess, TODO: remove usages from anything that isn't a postprocess				[MATRIX]
END_CB

#define PSC_Custom_First	PSC_Custom_0
#define PSC_Custom_Last		PSC_Custom_Matrix + 3

#endif

// HiRes shadow params
#define PSC_Custom_HiResShadowsParams				PSC_Custom_20

// Eye related stuff never interferes with normalblend areas since it's never used on the same shader.
#define PSC_Custom_EyeOrientationLeft_AxisX			PSC_Custom_8
#define PSC_Custom_EyeOrientationLeft_AxisY			PSC_Custom_9
#define PSC_Custom_EyeOrientationLeft_AxisZ			PSC_Custom_10
#define PSC_Custom_EyeOrientationRight_AxisX		PSC_Custom_11
#define PSC_Custom_EyeOrientationRight_AxisY		PSC_Custom_12
#define PSC_Custom_EyeOrientationRight_AxisZ		PSC_Custom_13

// Make sure head position doesn't interfere with normalblend areas, since it may be used on the same shader (skin shader)
// This assumes that NUM_NORMALBLEND_AREAS==16 (0..15 registers) plus 4 registers of weights (16..19 registers).
#ifdef __cplusplus
#define PSC_Custom_HeadMatrix						PSC_Custom_Matrix
#else
#define PSC_Custom_HeadFrontDirection				(PSC_Custom_Matrix[0])
#define PSC_Custom_HeadCenterPosition				(PSC_Custom_Matrix[1])
#define PSC_Custom_HeadUpDirection					(PSC_Custom_Matrix[2])
#endif

#ifdef __cplusplus
	#undef REG
#endif
