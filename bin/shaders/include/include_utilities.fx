#ifndef UTILITIES_H_INCLUDED
#define UTILITIES_H_INCLUDED

#if defined( __GLSL__ )
bool AssignIf( bool condition, bool val1, bool val2 )
{
	if ( condition )	return val1;
	else				return val2;
}
int AssignIf( bool condition, int val1, int val2 )
{
	if ( condition )	return val1;
	else				return val2;
}
uint AssignIf( bool condition, uint val1, uint val2 )
{
	if ( condition )	return val1;
	else				return val2;
}
float AssignIf( bool condition, float val1, float val2 )
{
	if ( condition )	return val1;
	else				return val2;
}
float2 AssignIf( bool condition, float2 val1, float2 val2 )
{
	if ( condition )	return val1;
	else				return val2;
}
float3 AssignIf( bool condition, float3 val1, float3 val2 )
{
	if ( condition )	return val1;
	else				return val2;
}
float4 AssignIf( bool condition, float4 val1, float4 val2 )
{
	if ( condition )	return val1;
	else				return val2;
}
float3x3 AssignIf( bool condition, float3x3 val1, float3x3 val2 )
{
	if ( condition )	return val1;
	else				return val2;
}
float4x4 AssignIf( bool condition, float4x4 val1, float4x4 val2 )
{
	if ( condition )	return val1;
	else				return val2;
}
#else
#define AssignIf( condition, val1, val2 )  condition ? val1 : val2
#endif


#define YCBCR_INTERLEAVE_HORIZONTAL 0

float3 PackYCbCr( float3 col )
{
   float3 _packed = float3(	0.299f * col.r + 0.587f * col.g + 0.114f * col.b,   
							0.5f + (-0.168 * col.r - 0.331f * col.g + 0.5f * col.b),
							0.5f + (0.5 * col.r - 0.418f * col.g - 0.081f * col.b) );
   //_packed = saturate( _packed );
   return _packed;
}

float3 UnpackYCbCr( float3 YCbCr )
{
   float y  = YCbCr.x;
   float cb = YCbCr.y;
   float cr = YCbCr.z;
   float3 unpacked = float3 ( 
      y + 1.402 * (cr - 0.5),
      y - 0.344 * (cb - 0.5) - 0.714 * (cr - 0.5),
      y + 1.772 * (cb - 0.5) );
   unpacked = saturate( unpacked );
   return unpacked;
}

float2 EncodeTwoChannelsYCbCr( float3 col, uint2 vpos )
{
	float3 yCbCr = PackYCbCr( saturate( col ) );
	
	// funny note: (0 == (vpos.y % 2)) won't work for speedtree shaders compilation.
	//             maybe because the "dx9 compatibility" shaders compilation setting is disabled?
#if YCBCR_INTERLEAVE_HORIZONTAL
	return float2( yCbCr.x, (0 == (vpos.y & 1)) ? yCbCr.y :  yCbCr.z );
#else
	return float2( yCbCr.x, (0 == (vpos.y & 1)) ? yCbCr.y :  yCbCr.z );
#endif
}

float3 SampleTwoChannelYCbCr( Texture2D<float4> _tex, uint2 pixelCoord, out float4 outEncodedValue )
{
	float4 v = _tex[pixelCoord];
	outEncodedValue = v;

#if YCBCR_INTERLEAVE_HORIZONTAL
	if ( 0 == pixelCoord.x % 2 )	v.z  = _tex[pixelCoord + uint2(1,0)].y;
	else							v.yz = float2 ( _tex[pixelCoord - uint2(1,0)].y, v.y );
#else
 	if ( 0 == pixelCoord.y % 2 )	v.z  = _tex[pixelCoord + uint2(0,1)].y;
 	else							v.yz = float2 ( _tex[pixelCoord - uint2(0,1)].y, v.y );
#endif
	
	return v.xyz;
}

float3 SampleTwoChannelYCbCrArray( TEXTURE2D_ARRAY<float4> _tex, uint sliceIndex, uint2 pixelCoord, out float4 outEncodedValue )
{
	uint3 samplePixelCoord = uint3(pixelCoord, sliceIndex);
	float4 v = _tex[samplePixelCoord];
	outEncodedValue = v;

#if YCBCR_INTERLEAVE_HORIZONTAL
	if ( 0 == pixelCoord.x % 2 )	v.z  = _tex[samplePixelCoord + uint3(1,0,0)].y;
	else							v.yz = float2 ( _tex[samplePixelCoord - uint3(1,0,0)].y, v.y );
#else
 	if ( 0 == pixelCoord.y % 2 )	v.z  = _tex[samplePixelCoord + uint3(0,1,0)].y;
 	else							v.yz = float2 ( _tex[samplePixelCoord - uint3(0,1,0)].y, v.y );
#endif
	
	return v.xyz;
}

float3 DecodeTwoChannelColor( Texture2D<float4> _tex, uint2 pixelCoord, out float4 outEncodedValue )
{
	return UnpackYCbCr( SampleTwoChannelYCbCr( _tex, pixelCoord, outEncodedValue ) );
}

float3 DecodeTwoChannelColorArray( TEXTURE2D_ARRAY<float4> _tex, uint sliceIndex, uint2 pixelCoord, out float4 outEncodedValue )
{
	return UnpackYCbCr( SampleTwoChannelYCbCrArray( _tex, sliceIndex, pixelCoord, outEncodedValue ) );
}

float3 DecodeTwoChannelColor( TEXTURE2D_MS<float4> _tex, uint2 pixelCoord, int sampleIndex, out float4 outEncodedValue )
{
	float4 v = _tex.Load( (int2)pixelCoord, sampleIndex );
	outEncodedValue = v;

#if YCBCR_INTERLEAVE_HORIZONTAL
	if ( 0 == pixelCoord.x % 2 )	v.z  = _tex.Load( pixelCoord + uint2(1,0), sampleIndex ).y;
	else							v.yz = float2 ( _tex.Load(pixelCoord - int2(1,0), sampleIndex ).y, v.y );
#else
 	if ( 0 == pixelCoord.y % 2 )	v.z  = _tex.Load( (int2)pixelCoord + int2(0,1), sampleIndex ).y;
 	else							v.yz = float2 ( _tex.Load((int2)pixelCoord - int2(0,1), sampleIndex ).y, v.y );
#endif
	
	return UnpackYCbCr( v.xyz );
}

float3 DecodeTwoChannelColor( Texture2D<float4> _tex, uint2 pixelCoord )
{
	float4 _decoy;
	return DecodeTwoChannelColor( _tex, pixelCoord, _decoy );
}

float3 DecodeTwoChannelColorArray( TEXTURE2D_ARRAY<float4> _tex, uint sliceIndex, uint2 pixelCoord )
{
	float4 _decoy;
	return DecodeTwoChannelColorArray( _tex, sliceIndex, pixelCoord, _decoy );
}

float CalcCubeMipLevel( int resolution, float3 N, float3 N2 )
{
	float a = acos( dot( N, N2 ) );
	float pd = a / (0.5 * 3.14159285) * resolution;
	return max( 0, log2( pd ) );
}

float CalcCubeMipLevelExplicitNormals( int resolution, float3 N, float3 N2, float3 N3 )
{
	float a = acos( min( dot( N, N2 ), dot( N, N3 ) ) );
	float pd = a / (0.5 * 3.14159285) * resolution;
	return log2( pd );
}

float CalcCubeMipLevel( int resolution, float3 N, int2 pixelCoord )
{
	N = normalize( N );

#ifdef VERTEXSHADER
	// VS does not support ddx/ddy...
	return CalcCubeMipLevel(resolution, N, N);
#else
	float3 dx = ddx( N );
	float3 dy = ddy( N );

	if ( (pixelCoord.x & 1) )
		dx *= -1;
	if ( (pixelCoord.y & 1) )
		dy *= -1;

	// ace_ibl_optimize: optimize this (we can merge stuff from inside of CalcCubeMipLevel)
	
	return max( CalcCubeMipLevel( resolution, N, normalize(N+dx) ), CalcCubeMipLevel( resolution, N, normalize(N+dy) ) );
#endif
}

float PosBasedCalcRandomFactor( float3 pos )
{
   float period = 9.51462632126623;
   pos.xy = fmod( pos.xy, period );
   
   float2 p = pos.xy + float2(pos.z, 0);
   const float2 r = float2( 23.1406926327792690, 2.6651441426902251 );
   return frac( cos( fmod( 123456789.0, 1e-7 + 256.0 * dot(p,r) ) ) );  
}

float4 PosBasedRandomColorData( float3 pos )
{
	float3 colorWeights = float3 (
		PosBasedCalcRandomFactor( pos ),
		PosBasedCalcRandomFactor( pos + 3.6979714896946124 ),
		PosBasedCalcRandomFactor( pos + 2.5654710701512515 ) );
	colorWeights /= max( 0.00001, dot( colorWeights, 1 ) );

	return float4 ( colorWeights, 1.0 );
}

float4 SpeedTreeCalcRandomColorData( float3 pos, float3 instanceRightVector )
{
	// non uniform distribution but at least cheap
	float f = instanceRightVector.x * 0.5 + 0.5;
	float3 v;
	v.x = 1 - saturate( f * 3 ) * saturate( (1 - f) * 3 );
	v.y = saturate( 1 - abs( f - 0.33333 ) * 3 );
	v.z = saturate( 1 - abs( f - 0.66666 ) * 3 );
	return float4( v, 1 );
}

#define DEFINE_IS_TEXTURE_BOUND_FUNCTION( _TexType )	\
	bool IsTextureBound( _TexType tex )					\
	{													\
		uint w, h;										\
		tex.GetDimensions( w, h );						\
		return w > 0;									\
	}

DEFINE_IS_TEXTURE_BOUND_FUNCTION( TextureCube )
DEFINE_IS_TEXTURE_BOUND_FUNCTION( Texture2D )

bool IsTextureBound( TEXTURECUBE_ARRAY tex )
{										
	uint w, h, e;							
	tex.GetDimensions( w, h, e );
	return w > 0;						
}

float4 ParaboloidToCube( float2 uv, int idx )
{
	uv = uv * 2 - 1;

	float3 dir;
	dir.x = 2.0f * uv.x;
	dir.y = -2.0f * uv.y;
	dir.z = -1.f + dot(uv,uv);
	dir *= (idx ? 1.f : -1.f);
	dir /= (dot(uv,uv) + 1.f);
	float alpha = (dot(uv,uv) <= 1) ? 1 : 0;	

	return float4 ( dir, alpha );
}

int GetCubeDirParaboloidIndexUp()
{
	return 0;
}

int GetCubeDirParaboloidIndexDown()
{
	return 1;
}

int GetCubeDirParaboloidIndex( float3 dir )
{
	return dir.z < 0 ? GetCubeDirParaboloidIndexDown() : GetCubeDirParaboloidIndexUp();
}
/// 'dir' MUST BE NORMALIZED !!!
float2 CubeToParaboloid( float3 dir, int idx )
{
	float2 uv0 = float2( 0.5, -0.5 ) * dir.xy / max(0.001, 1.0 - dir.z) + 0.5;
 	float2 uv1 = float2( -0.5, 0.5 ) * dir.xy / max(0.001, 1.0 + dir.z) + 0.5;
	return idx ? uv0 : uv1;
}

/// 'dir' MUST BE NORMALIZED !!!
float2 CubeToParaboloid( float3 dir )
{
	return CubeToParaboloid( dir, GetCubeDirParaboloidIndex( dir ) );
}

int DissolvePatternHelper( uint2 crd )
{
	return (crd.x & 1) ? ((crd.y & 1) ? 1 : 2) : ((crd.y & 1) ? 3 : 0);
	
	/*
	crd = crd % 2;
	if ( all(int2( 0, 0 ) == crd) )	return 0;
	if ( all(int2( 1, 1 ) == crd) )	return 1;
	if ( all(int2( 1, 0 ) == crd) )	return 2;
	return 3;
	*/
}

float CalcDissolvePattern( uint2 pixelCoord, uint numSteps )
{
	int v = 0;

	[unroll]
	for ( uint i=0; i<numSteps; ++i )
	{
		v = v << 2;
		v += DissolvePatternHelper( pixelCoord >> i );
	}

	float r = (1 << numSteps);
	return (v + 0.5) / (r * r);
}

float CalcDissolvePattern2( uint2 pixelCoord )
{
	return CalcDissolvePattern( pixelCoord, 2 );

	// TODO: test whether code below won't be faster (it's less instructions, but cycleSim is also lower)

/*
	// Values calculated from numbers below, with operation (x + 0.5) / 16
	// 
	//   0   8   2  11
	//  12   4  14   6
	//   3  10   1   9
	//  15   7  13   5

	const float t[16] = {
		0.03125,		0.53125,		0.15625,		0.71875,
		0.78125,		0.28125,		0.90625,		0.40625,
		0.21875,		0.65625,		0.09375,		0.59375,
		0.96875,		0.46875,		0.84375,		0.34375 };

	uint idx = (pixelCoord.x & 3) + ((pixelCoord.y & 3) << 2);
	return t[idx];
*/
}

float3 FilterSkyBanding( uint2 pixelCoord, float3 color )
{
	// disabling for now because it produces really heavy banding on IPS matrices (because it performs some kind of dither if it's own).
	// also it's not perfect with uberscreenshots (dither is visible on those, and this needs to be addressed in a special way).
	//	const float range = 0.075; //< minimum value that works well for us with rgb_16f and r11g11b10_float formats
	//	return color * (1.0 + range * (CalcDissolvePattern2( pixelCoord ) - 0.5));
	return color;
}

#ifdef STENCIL_TEX_FETCH_CHANNEL
// Function not defined if macro is not defined for purpose, so that we would get compilation errors instead of broken silent fetches.
uint GetStencilValue( uint2 fetchedValue )
{
	#if 1 == STENCIL_TEX_FETCH_CHANNEL
		return fetchedValue.y;
	#elif 0 == STENCIL_TEX_FETCH_CHANNEL
		return fetchedValue.x;
	#else
		Invalid Stencil Tex Fetch Channel
	#endif
}
#endif

#endif
