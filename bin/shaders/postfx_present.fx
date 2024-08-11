
#include "postfx_common.fx"


#define vTexCoordTransformColor VSC_Custom_0

#define vInvSurfaceSize 		 PSC_Custom_12
#define vInvSurfaceSizeOptConst  PSC_Custom_13
#define vInvSurfaceSizeOptConst2 PSC_Custom_14
#define vInvSurfaceSizeOptConst3 PSC_Custom_15


#define FXAA_PC 1
#ifdef __PSSL__
#	define FXAA_PSSL 1
#else
	#define FXAA_HLSL_5 1
#endif
#define FXAA_QUALITY__PRESET 12
#define FXAA_GREEN_AS_LUMA 1

#include "inc_nvidiaAA.fx"

Texture2D t_TextureColor	: register( t0 );
SamplerState s_TextureColor	: register( s0 );

//FxaaTex textureColor = FxaaTex(s_TextureColor, t_TextureColor);

struct VS_INPUT
{
	float4 pos			: POSITION0;
};


struct VS_OUTPUT
{
	float4 pos		   : SYS_POSITION;
	float2 coordColor  : TEXCOORD0;
};

#ifdef VERTEXSHADER

VS_OUTPUT vs_main( VS_INPUT i )
{
	VS_OUTPUT o;

	o.pos          = i.pos;
	o.coordColor   = ApplyTexCoordTransform( i.pos.xy, vTexCoordTransformColor );

	return o;
}
#endif

#ifdef PIXELSHADER

float4 ps_main( VS_OUTPUT i ) : SYS_TARGET_OUTPUT0
{
	FxaaTex textureColor;// = FxaaTex(s_TextureColor, t_TextureColor);
	textureColor.smpl = s_TextureColor;
	textureColor.tex = t_TextureColor;


    const float2 final_color_texcoord = i.coordColor;

	const float4 fxaaConsole360ConstDir = float4(1.0, -1.0, 0.25, -0.25);
	
	return FxaaPixelShader(				final_color_texcoord, 
										float4(-1,-1,-1,-1), 
										textureColor,
										textureColor, // bias -1, not used
										textureColor, // bias -2, not used									
										vInvSurfaceSize.xy,
										vInvSurfaceSizeOptConst,
										vInvSurfaceSizeOptConst2,
										vInvSurfaceSizeOptConst3,
										0.25f,
										0.125f,
										0.f,
										12.0f,
										0.4f,
										0.f,
										fxaaConsole360ConstDir
										);
}

#endif
