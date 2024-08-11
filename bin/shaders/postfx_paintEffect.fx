#include "postfx_common.fx"

Texture2D			t_TextureColor	: register( t0 );
Texture2D			t_TextureDepth	: register( t1 );
Texture2D<uint2>	t_Stencil		: register( t2 );
SamplerState		s_Linear		: register( s0 );
SamplerState		s_Point			: register( s1 );

#define fPaintAmount		PSC_Custom_0.x

struct VS_INPUT
{
	float4 pos			: POSITION0;
};

struct VS_OUTPUT
{
	float4 pos		   : SYS_POSITION;
};

#ifdef VERTEXSHADER
VS_OUTPUT vs_main( VS_INPUT i )
{
	VS_OUTPUT o;
	o.pos   = i.pos;
	return o;
}
#endif

#ifdef PIXELSHADER
float4 SampleDepth( float2 coords )
{
	return SAMPLE_LEVEL( t_TextureDepth, s_Point, coords, 0.0 ).x;
}

float4 SampleColor( float2 coords )
{
	return float4 ( SAMPLE_LEVEL( t_TextureColor, s_Point, coords, 0.0 ).xyz, 1 );
}

float DistToPaintAmount( float dist )
{
	return saturate( (dist - 10) / 15 );		// param dystans postaci
}

float AmountForCoords( float2 coord )
{
	const float distAmount = DistToPaintAmount( DeprojectDepthRevProjAware( SampleDepth( coord ) ) );
	const float charAmount = 0 != ( GetStencilValue( t_Stencil[ coord * surfaceDimensions.xy ] ) & LC_Characters ) ? 0 : 1;
	float amount = lerp( charAmount, 1, distAmount );
	return amount;
}

float3 CalculatePaintEffect( float2 vpos, float2 _off )
{
	const float2 coords = vpos.xy * surfaceDimensions.zw;
	
	float3 smp_result_min = 0;
	float3 smp_result_max = 0;
	float4 smp_result_avg = 0;
	{
		#ifdef SAMPLES_NUM_SELECTION
			#if 0 == SAMPLES_NUM_SELECTION
				const int smp_n = 11;
			#elif 1 == SAMPLES_NUM_SELECTION
				const int smp_n = 13;
			#elif 2 == SAMPLES_NUM_SELECTION
				const int smp_n = 15;
			#endif
		#else
		Number of samples undefined
		#endif

		const int smp_center = (smp_n - 1) / 2;
		const float2 smp_offset = _off * surfaceDimensions.zw;	
		const float2 clampRangeMin = 0.5 * surfaceDimensions.zw;
		const float2 clampRangeMax = (screenDimensions.xy - 0.5) * surfaceDimensions.zw;
		
		float2 thick_off = 0;
		int ti = 0;
// 		const int thick = 1;
// 		for ( ti=-thick; ti<=thick; ++ti )
 		{
//			thick_off = ti * normalize( smp_offset.yx * surfaceDimensions.xy ) * float2(-1, 1) * surfaceDimensions.zw;

			[unroll] for ( int i=0; i<smp_n; ++i )
			{
				const float2 currCrd = clamp( coords.xy + (i - 0.5 * (smp_n-1)) * smp_offset + thick_off, clampRangeMin, clampRangeMax );
				const float3 color = SampleColor( currCrd ).xyz;			
				const float amount = i == smp_center && 0 == ti ? 1 : AmountForCoords( currCrd );
				smp_result_max = max( smp_result_max, color * amount );
				smp_result_min = max( smp_result_min, (1 - color) * amount );
				smp_result_avg += float4( color, 1 ) * amount;
			}
		}

		smp_result_min = 1 - smp_result_min;
		smp_result_avg.xyz /= max( 0.0001, smp_result_avg.w );
	}

	float3 www = float3( 0.3, 0.5, 0.2 );
	float dd_min = abs(dot(www, smp_result_min) - dot(www, smp_result_avg.xyz));
	float dd_max = abs(dot(www, smp_result_max) - dot(www, smp_result_avg.xyz));
	float3 smp_result = lerp( smp_result_avg.xyz, smp_result_max, saturate( 3 * dd_min / (dd_min + dd_max + 0.001) ) );
	//float3 smp_result = abs(dot(www, smp_result_min) - dot(www, smp_result_avg.xyz)) > abs(dot(www, smp_result_max) - dot(www, smp_result_avg.xyz)) ? smp_result_max : smp_result_min;
	//float3 smp_result = smp_result_max;
	//float3 smp_result = smp_result_avg.xyz;

	return smp_result;
}

// Noise based on
// http://stackoverflow.com/questions/14062107/how-to-apply-the-perlin-noise-on-a-sphere

float Noise(float2 xy)
{
	float2 noise = (frac(sin(dot(xy ,float2(12.9898,78.233)*2.0)) * 43758.5453));
	return abs(noise.x + noise.y) * 0.5;
}

float SmoothNoise( float integer_x, float integer_y ) 
{
	float corners = ( Noise( float2(integer_x - 1, integer_y - 1) ) + Noise( float2(integer_x + 1, integer_y + 1 )) + Noise( float2(integer_x + 1, integer_y - 1 )) + Noise( float2(integer_x - 1, integer_y + 1 )) ) / 16.0f;
	float sides = ( Noise( float2(integer_x, integer_y - 1 )) + Noise( float2(integer_x, integer_y + 1 )) + Noise( float2(integer_x + 1, integer_y )) + Noise( float2(integer_x - 1, integer_y )) ) / 8.0f;
	float center = Noise( float2(integer_x, integer_y )) / 4.0f;

	return corners + sides + center;
}

float InterpolatedNoise( float x, float y ) 
{
	//float integer_x = x - frac(x), fractional_x = frac(x);
	float integer_x = floor( x );
	float fractional_x = x - integer_x;

	//float integer_y = y - frac(y), fractional_y = frac(y);
	float integer_y = floor( y );
	float fractional_y = y - integer_y;

	float p1 = SmoothNoise( integer_x, integer_y );
	float p2 = SmoothNoise( integer_x + 1, integer_y );
	float p3 = SmoothNoise( integer_x, integer_y + 1 );
	float p4 = SmoothNoise( integer_x + 1, integer_y + 1 );

	p1 = lerp( p1, p2, fractional_x );
	p2 = lerp( p3, p4, fractional_x );

	return saturate( lerp( p1, p2, fractional_y ) );
}

float4 ps_main( VS_OUTPUT i ) : SYS_TARGET_OUTPUT0
{
	float ss = 50; // param tiling zaburzen kierunku na ekranie
	float n = InterpolatedNoise( i.pos.x * screenDimensions.z * ss, i.pos.y * screenDimensions.w * screenDimensions.y / screenDimensions.x * ss );
	//n += PSC_TimeVector.x * 0.15; // param rotacja po czasie
	//return float4 ( n, n, n, 1 );

	float2 off;
	sincos( n * (2 * PI), off.x, off.y );
	off *= lerp( 0, 1, AmountForCoords( i.pos.xy * surfaceDimensions.zw ) );	
	off *= 1.0;

	//off *= sin( PSC_TimeVector.x ) * 0.5 + 0.5;

	float3 filtered = CalculatePaintEffect( i.pos.xy, off );

	filtered = lerp( SampleColor( i.pos.xy * surfaceDimensions.zw ).xyz, filtered, fPaintAmount );
	
	return float4( filtered, 1 );
}
#endif
