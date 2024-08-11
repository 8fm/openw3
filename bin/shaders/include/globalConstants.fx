/// Common header for all dynamically compiled shaders
/// It's esential to recompile all shaders after changing any of those lines
#ifndef GLOBAL_CONSTANTS_FX_H_INCLUDED
#define GLOBAL_CONSTANTS_FX_H_INCLUDED

#ifdef __cplusplus
# define BUILD_RGB_LUMINANCE_WEIGHTS( x, y, z )				Vector( x, y, z, 0.f )
#else
# define BUILD_RGB_LUMINANCE_WEIGHTS( x, y, z )				float3( x, y, z )
#endif

////////////////////////////////////////////////
/// Gamma related parameters
////////////////////////////////////////////////

#define GAMMA_TO_LINEAR_EXPONENT		2.2
#define GAMMA_TO_LINEAR_EXPONENT_INV	(1.0 / GAMMA_TO_LINEAR_EXPONENT)


////////////////////////////////////////////////
/// Luminance related parameters
////////////////////////////////////////////////

#define RGB_LUMINANCE_WEIGHTS_LINEAR						BUILD_RGB_LUMINANCE_WEIGHTS( 0.2126f, 0.7152f, 0.0722f )
#define RGB_LUMINANCE_WEIGHTS_GAMMA							BUILD_RGB_LUMINANCE_WEIGHTS( 0.299f, 0.587f, 0.114f )

#define RGB_LUMINANCE_WEIGHTS_LINEAR_Histogram				RGB_LUMINANCE_WEIGHTS_LINEAR
#define RGB_LUMINANCE_WEIGHTS_LINEAR_ApplyFogDataFull		BUILD_RGB_LUMINANCE_WEIGHTS( 0.333, 0.555, 0.222 )
#define RGB_LUMINANCE_WEIGHTS_LINEAR_Sepia					BUILD_RGB_LUMINANCE_WEIGHTS( 0.299f, 0.587f, 0.114f )
#define RGB_LUMINANCE_WEIGHTS_LINEAR_ParametricBalance		BUILD_RGB_LUMINANCE_WEIGHTS( 0.3333/1.1, 0.5555/1.1, 0.2222/1.1 )
#define RGB_LUMINANCE_WEIGHTS_LINEAR_FlareGrab				BUILD_RGB_LUMINANCE_WEIGHTS( 0.3f, 0.59f, 0.11f )
#define RGB_LUMINANCE_WEIGHTS_LINEAR_FlareApply				BUILD_RGB_LUMINANCE_WEIGHTS( .3f, .59f, .11f )
#define RGB_LUMINANCE_WEIGHTS_LINEAR_DofScatter				BUILD_RGB_LUMINANCE_WEIGHTS( 0.3333, 0.3333, 0.3333 )
#define RGB_LUMINANCE_WEIGHTS_LINEAR_DownsampleLum4			BUILD_RGB_LUMINANCE_WEIGHTS( 0.2126f, 0.7152f, 0.0722f )

#ifdef __cplusplus
# define RGB_LUMINANCE_WEIGHTS_MatBlockDesaturate			BUILD_RGB_LUMINANCE_WEIGHTS( 0.299f, 0.587f, 0.114f )
# define RGB_LUMINANCE_WEIGHTS_EnvTransparencyColorFilter	BUILD_RGB_LUMINANCE_WEIGHTS( 0.33f, 0.34f, 0.33f )
# define RGB_LUMINANCE_WEIGHTS_GAMMA_SelectionDesaturate	BUILD_RGB_LUMINANCE_WEIGHTS( 0.3333f, 0.3334f, 0.3333f )
# define RGB_LUMINANCE_WEIGHTS_COLOR						Color ( 54, 182, 19 )
# define RGB_LUMINANCE_WEIGHTS_COLOR_GAMMA					Color ( 76, 149, 30 )
#endif


////////////////////////////////////////////////
/// Various parameters
////////////////////////////////////////////////


#define DISSOLVE_TEXTURE_SIZE				16
#define POISSON_ROTATION_TEXTURE_SIZE		32
#define VOLUME_TEXTURE_SLOT					24

// Poisson samples count may need binaries recompilation...
#define POISSON_SHADOW_SAMPLES_COUNT		16
#define POISSON_SHADOW_SAMPLES_COUNT_2		8

// Maximum accumulative refraction offset. Lower values give better offseting precision, 
// but limits offset range and may cause some inaccuracies when multiple refractive surfaces 
// overlap. Feel free to change it as you want - there shouldn't be any dependencies except 
// of materials compiled in code.
#define ACCUMULATIVE_REFRACTION_MAX_OFFSET	0.15

#define WEATHER_VOLUMES_SIZE_DIV			2
#define WEATHER_VOLUMES_SIZE_MUL			(1.f / WEATHER_VOLUMES_SIZE_DIV)

#define DYNAMIC_WATER_WORLD_SIZE			30.0f

#define GLOBAL_SHADOW_BUFFER_CHANNEL_SSAO				0
#define GLOBAL_SHADOW_BUFFER_CHANNEL_SHADOW				1
#define GLOBAL_SHADOW_BUFFER_CHANNEL_INTERIOR_FACTOR	2
#define GLOBAL_SHADOW_BUFFER_CHANNEL_DIMMERS			3

#define SHARED_CONSTS_COLOR_GROUPS_CAPACITY		48

#define SHADER_LIGHT_USAGE_MASK_INTERIOR		((1 << 10) << 1)
#define SHADER_LIGHT_USAGE_MASK_EXTERIOR		((1 << 11) << 1)
#define SHADER_LIGHT_USAGE_MASK_CAMERA_LIGHT	((1 << 31))

#define ENVPROBE_FLAG_INTERIOR					(1 << 0)
#define ENVPROBE_FLAG_EXTERIOR					(1 << 1)

////////////////////////////////////////////////
/// Realtime reflections
////////////////////////////////////////////////

#ifdef __cplusplus
#ifdef RED_PLATFORM_DURANGO
# define REALTIME_REFLECTIONS_FIXED_SIZE		1
#else
# define REALTIME_REFLECTIONS_FIXED_SIZE		0
#endif
#endif

#define REALTIME_REFLECTIONS_SIZE_DIV			4
#define REALTIME_REFLECTIONS_MASK_GAMMA			2.0
#define REALTIME_REFLECTIONS_MASK_GAMMA_INV		(1.0 / REALTIME_REFLECTIONS_MASK_GAMMA)

#ifdef __cplusplus
#ifdef REALTIME_REFLECTIONS_FIXED_SIZE
# if REALTIME_REFLECTIONS_FIXED_SIZE
#  define REALTIME_REFLECTIONS_SIZE_WIDTH		(1600/4)
#  define REALTIME_REFLECTIONS_SIZE_HEIGHT		(900/4)
# endif
#else
# error Invalid definition
#endif
#endif

#define REALTIME_REFLECTIONS_MAX_OFFSET_VALUE		0.2
#define REALTIME_REFLECTIONS_MAX_OFFSET_VALUE_INV	(1.0 / REALTIME_REFLECTIONS_MAX_OFFSET_VALUE)

#ifdef __cplusplus
// Clear value with encoded neutral offset so that mipmapping would be more accurate on the borders of the reflection
#define REALTIME_REFLECTIONS_CLEAR_VALUE	(Vector(0.5,0.5,1,0))
#else
// Info about the input offset should be in rlr_apply.fx shader.
float4 EncodeRealtimeReflectionsMask( float2 offset, float amount, bool isGlobalReflection = false )
{
	float2 encodedOffset = offset * (REALTIME_REFLECTIONS_MAX_OFFSET_VALUE_INV * 0.5) + 0.5;
	float encodedAmount = pow( max( 0, amount ), REALTIME_REFLECTIONS_MASK_GAMMA_INV );
	return float4( encodedOffset, isGlobalReflection ? 1 : 0, encodedAmount );
}
#endif

////////////////////////////////////////////////
/// Light channels
////////////////////////////////////////////////


#ifndef __cplusplus
# define LC_Characters	(1 << 1)
#endif


////////////////////////////////////////////////

// Base capacity is 64
#define HISTOGRAM_CAPACITY_MULTIPLIER		4


////////////////////////////////////////////////

#endif
