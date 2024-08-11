#if defined(IS_HELPER_COLOR_GRAB)
////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////

#include "common.fx"
#include "globalConstantsVS.fx"
#include "globalConstantsPS.fx"

Texture2D		t_SourceColor		: register( t0 );

struct VS_INPUT
{
	float4 pos : POSITION0;
};

struct VS_OUTPUT
{
	float4 pos : SYS_POSITION;
};

#ifdef VERTEXSHADER
VS_OUTPUT vs_main( VS_INPUT i )
{
	VS_OUTPUT o;

	o.pos = mul( mul( i.pos, VSC_LocalToWorld ), VSC_WorldToScreen );
	
	return o;
}
#endif

#ifdef PIXELSHADER
float4 ps_main( VS_OUTPUT i ) : SYS_TARGET_OUTPUT0
{
	return t_SourceColor[ i.pos.xy ];
}
#endif

////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
#else
////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////

#include "common.fx"
#include "globalConstantsVS.fx"
#include "globalConstantsPS.fx"


#if ENABLE_ALPHA_FETCH
Texture2D		t_TextureOpaque			: register( t0 );
Texture2D		t_TextureTransp			: register( t1 );
Texture2D		t_TextureDissolve		: register( t2 );
#define			vDissolveTextureSize	((uint2)PSC_Custom_0.xy)
#define			vTranspFilterParams		(PSC_Custom_1.xy)
#endif


struct VS_INPUT
{
	float4 pos : POSITION0;
	float4 color : COLOR0;
};

struct VS_OUTPUT
{
	float4 pos : SYS_POSITION;
	float4 color : COLOR0;
};

#ifdef VERTEXSHADER
VS_OUTPUT vs_main( VS_INPUT i )
{
	VS_OUTPUT o;

	o.pos = mul( mul( i.pos, VSC_LocalToWorld ), VSC_WorldToScreen );
	o.color = i.color.zyxw;

	return o;
}
#endif

#ifdef PIXELSHADER
void ps_main( VS_OUTPUT i )
{
#if ENABLE_ALPHA_FETCH
	{
		const uint2 vpos = (uint2)i.pos.xy;

		float alpha = 0;
		{
			const float3 colOpaque = t_TextureOpaque[ vpos ];
			const float3 colTransp = t_TextureTransp[ vpos ];
			
			const float3 change = max( 0, 1.f - colTransp / max( 0.0001, colOpaque ) ); //< brightening doesn't remove flare
			const float totalChange = dot( float3( 0.3, 0.5, 0.2 ), change );

			alpha = saturate( (totalChange - vTranspFilterParams.x) / vTranspFilterParams.y );
		}
		
		const float mask = t_TextureDissolve[vpos % vDissolveTextureSize].x;
		if ( alpha > 0.999 * mask )
		{
			discard;
		}
	}
#endif
}
#endif

#endif
