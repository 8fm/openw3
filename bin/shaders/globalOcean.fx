
#define DEBUG_WATER_SOLID_COLOR 	0
#define DEBUG_DYNAMIC_SOLID_COLOR	0
#define DEBUG_WATER_DECAL			0
#define DEBUG_DELTA					0


// sync with renderProxyWater.cpp
#define WATER_GRID_ROW_SIZE 8
#define WATER_GRID_SIZE 100.0f
#define WATER_GRID_SIZE_HALF 50.0f
#define WATER_GRID_EXPANSION 1500.0f;
#define GRID_TESSLATION 16

//////////////////////////////////////////////////
// Constants 
#include "globalOceanConstants.fx"

//////////////////////////////////////////////////

// array with all base textures
TEXTURE2D_ARRAY			t_array : register (t0);
SamplerState			s_array : register (s0);

// fourier
Texture2D				t_furier : register (t5);
SamplerState			s_furier : register (s5);

// dynamic waves
Texture2D				t_dynamic : register (t1);
SamplerState			s_dynamic : register (s1);

// decal waves
Texture2D				t_decal : register (t3);
SamplerState			s_decal : register (s3);

// The only structure visible for all shaders including pixel shader
struct DS_OUTPUT
{
	float4 	pos 			: SYS_POSITION;
	float2 	uv 				: TEXCOORD0;	
	float 	viewPosZ		: TEXCOORD2;
	float4	globalPos		: TEXCOORD3;    

	float4	paramsFog		: TEXCOORD4;
	float4	paramsAerial	: TEXCOORD5;
};

#ifdef VERTEXSHADER
//--------------------------------------------------------------------------------------
// Vertex shader
//--------------------------------------------------------------------------------------

struct VS_INPUT
{
	float3 pos 				: POSITION0;
	float2 offset			: PATCH_OFFSET;
};

struct VS_OUTPUT
{
	float4	pos 				: TEXCOORD0;
	int		clipmapLevel		: TEXCOORD1;

	float4 	paramsFog			: TEXCOORD2;
	float4 	paramsAerial		: TEXCOORD3;	
};


VS_OUTPUT vs_main( VS_INPUT input )
{
	VS_OUTPUT o = (VS_OUTPUT)0;
	o.pos = float4( input.pos.xyz, 1.0f );
	
	float2 instanceOffset = input.offset;

	o.pos.xy += instanceOffset;
	float3 globalPos = input.pos.xyz;
	globalPos.xy += instanceOffset;
	
	const float siz = WATER_GRID_SIZE/float(GRID_TESSLATION);
	float2 cam = floor(VSC_CameraPosition.xy/siz)*siz;
	float2 diir = globalPos.xy-cam;
	float2 delt = float2(0.0f,0.0f);
	
	float m_0 = step( 350.0f, diir.x );
	float m_1 = step( diir.x, -390.0f );
	
	float n_0 = step( 350.0f, diir.y );
	float n_1 = step( diir.y, -390.0f );	

	delt.x += ( m_0 ) * WATER_GRID_EXPANSION;
	delt.x += ( m_1 ) * -WATER_GRID_EXPANSION;
	
	delt.y += ( n_0 ) * WATER_GRID_EXPANSION;
	delt.y += ( n_1 ) * -WATER_GRID_EXPANSION;	

	o.pos.xy += delt;
	globalPos.xy += delt;

	// since we are pushing the lakes up, cannot discard before checkin waterOffset
	o.clipmapLevel = DetermineClipmapLevel( o.pos.xy );
	float hmapHeight = GetTerrainHeight( globalPos.xy, o.clipmapLevel );
	float waterOffset = getWaterAreaOffset( (int)Lakes.numLakes, globalPos.xyz );		

	// assume visibility
	// do not cut off if water area was used (underground caves)
		
	if( abs(waterOffset) > 0.01f )
	{
		o.pos.z = waterOffset;		
	}
	else
	{			
		// cut off only when grid density is enough (near camera, rest is too low tess)
		if( globalPos.z < (hmapHeight - 1.5f) && length(globalPos.xy - VSC_CameraPosition.xy) < 500.0f )
		{		
			o.pos.w = 0.0f;
		}		
	}	

	SFogData fogData = CalculateFogFull( false, false, o.pos.xyz );
	
	o.paramsFog = fogData.paramsFog;
	o.paramsAerial = fogData.paramsAerial;	
	
	return o;
}

//--------------------------------------------------------------------------------------
// Hull shader
//--------------------------------------------------------------------------------------

struct HS_CONSTANT_DATA_OUTPUT
{
    float		Edges[4]			: SYS_EDGE_TESS_FACTOR;
    float		Inside[2]			: SYS_INSIDE_TESS_FACTOR;
	int			clipmapLevel		: CMLEVEL;	
};

struct HS_CONTROL_POINT_OUTPUT
{
	float4		pos 				: CPPOS;
	float4		paramsFog			: PMFOG;
	float4		paramsAerial		: PMAERIAL;
};

HS_CONSTANT_DATA_OUTPUT ConstantsHS( InputPatch<VS_OUTPUT, 4> p, uint PatchID : SYS_PRIMITIVE_ID )
{
    HS_CONSTANT_DATA_OUTPUT output  = (HS_CONSTANT_DATA_OUTPUT)0;	

	float tessFactor = GlobalWater.tessFactor;
	float tessFactorInside0 = GlobalWater.tessFactor;
	float tessFactorInside1 = GlobalWater.tessFactor;

	float ssEdge0 = tessFactor;
	float ssEdge1 = tessFactor;
	float ssEdge2 = tessFactor;
	float ssEdge3 = tessFactor;
	
    if( (p[0].pos.w + p[1].pos.w + p[2].pos.w + p[3].pos.w) < 0.01f )
    {
 		tessFactor =  0.0f;
		tessFactorInside0 = 0.0f;
		tessFactorInside1 = 0.0f;
	
		ssEdge0 = 0.0f;
		ssEdge1 = 0.0f;
		ssEdge2 = 0.0f;
		ssEdge3 = 0.0f;
    }
	else				
	{	
		float4 ssPos0 = float4( p[0].pos.xyz, 1.0f );
		ssPos0 = mul( ssPos0, VSC_WorldToScreen );
		ssPos0.xy /= ssPos0.w;

		float4 ssPos1 = float4( p[1].pos.xyz, 1.0f );
		ssPos1 = mul( ssPos1, VSC_WorldToScreen );
		ssPos1.xy /= ssPos1.w;

		float4 ssPos2 = float4( p[2].pos.xyz, 1.0f );
		ssPos2 = mul( ssPos2, VSC_WorldToScreen );
		ssPos2.xy /= ssPos2.w;

		float4 ssPos3 = float4( p[3].pos.xyz, 1.0f );
		ssPos3 = mul( ssPos3, VSC_WorldToScreen );
		ssPos3.xy /= ssPos3.w;	
		
		// TODO
		//tessFactor *= distance( VSC_ViewportSize.xy, float2(1920,1080) );

		const float tessExp = 0.9f;

		ssEdge0 = tessFactor*saturate( tessExp*distance(ssPos0.xy, ssPos2.xy) );
		ssEdge1 = tessFactor*saturate( tessExp*distance(ssPos0.xy, ssPos1.xy) );		
		ssEdge2 = tessFactor*saturate( tessExp*distance(ssPos1.xy, ssPos3.xy) );
		ssEdge3 = tessFactor*saturate( tessExp*distance(ssPos2.xy, ssPos3.xy) );

		ssEdge0 = clamp(ssEdge0, 1.0f, GlobalWater.tessFactor);
		ssEdge1 = clamp(ssEdge1, 1.0f, GlobalWater.tessFactor);
		ssEdge2 = clamp(ssEdge2, 1.0f, GlobalWater.tessFactor);
		ssEdge3 = clamp(ssEdge3, 1.0f, GlobalWater.tessFactor);

		tessFactorInside0 = 0.5f*(ssEdge1 + ssEdge3);
		tessFactorInside1 = 0.5f*(ssEdge0 + ssEdge2);
				
		if( distance( 0.25f*(p[0].pos.xy + p[1].pos.xy + p[2].pos.xy + p[3].pos.xy), VSC_CameraPosition.xy ) > 400.0f ) 
		{
			ssEdge0 = ssEdge1 = ssEdge2 = ssEdge3 = 1.0f;
			tessFactorInside0 = tessFactorInside1 = 1.0f;
		}		
	}		
	
	// Assign tessellation levels
	output.Edges[0] = ssEdge0;
	output.Edges[1] = ssEdge1;
	output.Edges[2] = ssEdge2;
	output.Edges[3] = ssEdge3;
	
	output.Inside[0] = tessFactorInside0;
	output.Inside[1] = tessFactorInside1;

	output.clipmapLevel = max( max( p[0].clipmapLevel, p[1].clipmapLevel ), max( p[2].clipmapLevel, p[3].clipmapLevel ) );			

    return output;
}

[DOMAIN_PATCH_TYPE("quad")]
[HS_PARTITIONING("fractional_even")]
[HS_OUTPUT_TOPOLOGY("triangle_ccw")]
[HS_OUTPUT_CONTROL_POINTS(4)]
[HS_PATCH_CONSTANT_FUNC("ConstantsHS")]
[HS_MAX_TESS_FACTOR(64.0)]
HS_CONTROL_POINT_OUTPUT hs_main( InputPatch<VS_OUTPUT, 4> inputPatch, uint uCPID : SYS_OUTPUT_CONTROL_POINT_ID )
{
    HS_CONTROL_POINT_OUTPUT    output = (HS_CONTROL_POINT_OUTPUT)0;
    output.pos = inputPatch[uCPID].pos;

	output.paramsFog = inputPatch[uCPID].paramsFog;	
	output.paramsAerial = inputPatch[uCPID].paramsAerial;
	
    return output;
}

//--------------------------------------------------------------------------------------
// Domain Shader
//--------------------------------------------------------------------------------------

[DOMAIN_PATCH_TYPE("quad")]
DS_OUTPUT ds_main( HS_CONSTANT_DATA_OUTPUT input, float2 quadUV : SYS_DOMAIN_LOCATION, const OutputPatch<HS_CONTROL_POINT_OUTPUT, 4> quadPatch )
{
    DS_OUTPUT output = (DS_OUTPUT)0;
	

    // Interpolate world space position with quad uv coordinates
	float3 globalPos = lerp( lerp( quadPatch[0].pos.xyz, quadPatch[1].pos.xyz, quadUV.x), lerp( quadPatch[2].pos.xyz, quadPatch[3].pos.xyz, quadUV.x ), quadUV.y );			

	float hmapHeight = GetTerrainHeight( globalPos.xy, input.clipmapLevel );	
	
	// lakes			
	float referenceWaterLevel = globalPos.z;
	float3 shoreProximity = getWaterShoreProximityDampValue(hmapHeight, referenceWaterLevel);
	float caveDampValue = lerp(0.1f, 1.0f, getWaterBelowGroundDampValue( hmapHeight, referenceWaterLevel ));
	
	// dampen noisy waves with distance 	
	float distanceDamp = getDS_DistanceDamp( globalPos.xy, VSC_CameraPosition.xy, 250.0f ); 
	
	output.uv = globalPos.xy;
		
	float2 waves_UV_biggest = globalPos.xy / (GlobalWater.uvScale.x);	
	float2 waves_UV_medium = globalPos.xy / (GlobalWater.uvScale.y);
	float2 waves_UV_small = globalPos.xy / (GlobalWater.uvScale.z);	

	float4 furier_large = SAMPLE_LEVEL( t_furier, s_furier, waves_UV_biggest, 0 );
	float4 furier_mediumN =  SAMPLE_LEVEL( t_furier, s_furier, waves_UV_medium, 0 );	

	// DS choppy			
	float3 choppyFactor = caveDampValue*float3(GlobalWater.choppyFactors.x, GlobalWater.choppyFactors.y, 0.0f );	

	furier_large = SAMPLE_LEVEL( t_furier, s_furier, waves_UV_biggest + choppyFactor.x*furier_large.xy, 0 );
	furier_large.z = F_LARGE_Nz;	
	furier_large.xyz = normalize(furier_large.xyz);

	furier_mediumN = SAMPLE_LEVEL( t_furier, s_furier, waves_UV_medium + choppyFactor.y*furier_mediumN.xy, 0 );
	furier_mediumN.z = F_MEDIUM_Nz;	
	furier_mediumN.xyz = normalize(furier_mediumN.xyz);
	
	// damp the largest waves near coastline and in caves		
	float3 h = GlobalWater.amplitudeScale.xyz * shoreProximity.xyz;	
																	
	float resultingH = h.x * furier_large.w + h.y * furier_mediumN.w;// + h.z * furier_smallN.w;
	globalPos.z = caveDampValue * distanceDamp*resultingH + referenceWaterLevel;	
	
	globalPos.xy -= choppyFactor.x * furier_large.xy;	
	globalPos.xy -= choppyFactor.y * furier_mediumN.xy;	

	float boatProximity = 0.0f;
	
	for(int i=0; i<Displacements.numDispl; ++i)
	{	
		float4 dispRow1 = Displacements.displacementData[ (4*i)+0 ];
		float4 dispRow2 = Displacements.displacementData[ (4*i)+1 ];
		float4 dispRow3 = Displacements.displacementData[ (4*i)+2 ];
		float4 dispPos  = Displacements.displacementData[ (4*i)+3 ];
		dispPos.z  = clamp( dispPos.z, globalPos.z-0.3f, globalPos.z+0.3f );
		
		float3 offsetPos = globalPos - dispPos.xyz;
		
		{
			float locX = dot( offsetPos, dispRow1.xyz )/( dot( dispRow1.xyz, dispRow1.xyz ) );
			float locY = dot( offsetPos, dispRow2.xyz )/( dot( dispRow2.xyz, dispRow2.xyz ) );
			float locZ = dot( offsetPos, dispRow3.xyz )/( dot( dispRow3.xyz, dispRow3.xyz ) );

			float3 localPos = float3( locX, locY, locZ );

			float dirX = dot( float3(0.f,0.f,-1.f), dispRow1.xyz )/( dot( dispRow1.xyz, dispRow1.xyz ) );
			float dirY = dot( float3(0.f,0.f,-1.f), dispRow2.xyz )/( dot( dispRow2.xyz, dispRow2.xyz ) );
			float dirZ = dot( float3(0.f,0.f,-1.f), dispRow3.xyz )/( dot( dispRow3.xyz, dispRow3.xyz ) );

			float3 localDir = normalize( float3( dirX, dirY, dirZ ) );

			float vdir = dot( localPos, localDir );
			float sqrtVal = (vdir*vdir) - (( dot(localPos, localPos) - 1.0f ));

			if (sqrtVal >= 0)
			{
				float toNear = -vdir+sqrt( sqrtVal );
				float toFar  = -vdir-sqrt( sqrtVal );
				float t = max( toNear, toFar );
				if ( t>0.f )
				{
					localDir *= t;
					float3 globDir = localDir.x*dispRow1.xyz + localDir.y*dispRow2.xyz + localDir.z*dispRow3.xyz;
					globalPos += 0.4f*globDir;
				}
			}	

			//boatProximity += saturate(2.6f*locZ) * 1.3f*(1.0f-saturate( (length(float3(locX,locY,0.4f*locZ)) - 0.3f)  ));
			//boatProximity += 1.0f-pow( saturate( length(float3(locX,locY,locZ)) - 0.0f ), 16.0f );
		}
	}
		
	output.pos 				= mul( float4(globalPos, 1.0f), VSC_WorldToScreen );		

	float3 vSpace = mul( float4(globalPos, 1.0f), VSC_WorldToView ).xyz;
	output.viewPosZ	= vSpace.z;		
	output.globalPos.xyz	= globalPos.xyz;	
	output.globalPos.w		= referenceWaterLevel;	
		
	//SFogData fogData = CalculateFogFull( false, false, globalPos.xyz );	
	//output.paramsFog = fogData.paramsFog;
	//output.paramsAerial = fogData.paramsAerial;	

	output.paramsFog		= lerp( lerp( quadPatch[0].paramsFog, quadPatch[1].paramsFog, quadUV.x), lerp( quadPatch[2].paramsFog, quadPatch[3].paramsFog, quadUV.x ), quadUV.y );
	output.paramsAerial		= lerp( lerp( quadPatch[0].paramsAerial, quadPatch[1].paramsAerial, quadUV.x), lerp( quadPatch[2].paramsAerial, quadPatch[3].paramsAerial, quadUV.x ), quadUV.y );	
	
    return output;
}
#endif

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------

#ifdef PIXELSHADER

struct PS_OUTPUT
{
	float4 color 					: SYS_TARGET_OUTPUT0;	
	float4 rlrMask					: SYS_TARGET_OUTPUT1;
};

struct PS_INPUT
{
	float4 	pos 			: SYS_POSITION;
	float2 	uv 				: TEXCOORD0;		
	float	viewPosZ		: TEXCOORD2;
	float4	globalPos		: TEXCOORD3;   

	float4	paramsFog		: TEXCOORD4;
	float4	paramsAerial	: TEXCOORD5;
};

PS_OUTPUT ps_main( PS_INPUT i )
{
	PS_OUTPUT o = (PS_OUTPUT)0;	
			
	// calculate uv in screen space
	float2 uv = float2( i.pos.x/PSC_ViewportSize.x, i.pos.y/PSC_ViewportSize.y );
		
								// TOOD, subsize is not working
	const float2 refractionClampMax = PSC_ViewportSubSize.xy - 0.5f * PSC_ViewportSize.zw;

	// sample the decals (river streams)
	float2 decal_uv = uv*0.5f;
	float2 soofs = float2( 2.5f / PSC_ViewportSubSize.x, 2.5f / PSC_ViewportSubSize.y );
	float decal0 = (SAMPLE( t_decal, s_decal, saturate(decal_uv+float2(-soofs.x,0.0f)) ).z);	
		
	float u = decal0 - SAMPLE( t_decal, s_decal, saturate(decal_uv+float2( soofs.x,0.0f)) ).z;
	float v = SAMPLE( t_decal, s_decal, saturate(decal_uv+float2(0.0f, soofs.y)) ).z - SAMPLE( t_decal, s_decal, saturate(decal_uv+float2(0.0f,-soofs.y)) ).z;

	const float3 deacal_norm = normalize( float3( u, v, 1.0f ) );
		
	// separate distance reflection to avoid shimmering
	float distanceDamp = getDS_DistanceDamp( i.globalPos.xy, PSC_CameraPosition.xy, 350.0f );	
		
	// de-noise normalmap with distance
	float smoothNormal = saturate( pow( distanceDamp, 8.0f ) );

	float2 waves_UV_biggest = i.uv.xy / ( GlobalWater.uvScale.x );
	float2 waves_UV_medium = i.uv.xy / ( GlobalWater.uvScale.y );
	float2 waves_UV_small = i.uv.xy / ( GlobalWater.uvScale.z );	
		
	float3 furier_largeN = SAMPLE_LEVEL( t_furier, s_furier, waves_UV_biggest, 0 ).xyz;	
	float3 furier_mediumN =  SAMPLE_LEVEL( t_furier, s_furier, waves_UV_medium, 0 ).xyz;
	float3 furier_smallN =  SAMPLE_LEVEL( t_furier, s_furier, waves_UV_small, 0 ).xyz;

	// unfortunately we need to sample hmap in PS because we dont have enough vertices	
	float hmapHeight = GetTerrainHeight( i.globalPos.xy, DetermineClipmapLevel( i.globalPos.xy ) );	
	
	// damping normals and foam in caves
	float caveFactor = getWaterBelowGroundDampValue( hmapHeight, i.globalPos.z );

	float wdepth = abs(i.globalPos.z - hmapHeight);

				// dynaimc normal (xy), delta, windScale
	float4 foamControls = float4(0.0f, 0.0f, 0.0f, 0.0f);		
	{
		// dampen the proximity with distance, so PS high waves normals can be seen clearer than small / medium
		float3 shoreProximity = float3(1,1,1);		
		shoreProximity.xyz = lerp( float3(1,0,0), shoreProximity.xyz, distanceDamp );
		shoreProximity.xyz = lerp( float3(0,0,1), shoreProximity.xyz, caveFactor );
 				
		// PS choppy
		float3 choppyFactor = caveFactor*float3(GlobalWater.choppyFactors.z, GlobalWater.choppyFactors.w, 0.0f );

		furier_largeN = SAMPLE_LEVEL( t_furier, s_furier, waves_UV_biggest + choppyFactor.x * furier_largeN.xy, 0 ).xyz;
		furier_mediumN = SAMPLE_LEVEL( t_furier, s_furier, waves_UV_medium + choppyFactor.y * furier_mediumN.xy, 0 ).xyz;		
		//furier_smallN = SAMPLE_LEVEL( t_furier, s_furier, waves_UV_small + choppyFactor.z * furier_smallN.xy, 0 ).xyz;
		
		// scale delta based on wind
		foamControls.w = saturate( max(0.05f, GlobalWater.amplitudeScale.w) );
				

		foamControls.z = clamp(		lerp(0.92f, 0.4f*saturate(0.2f*wdepth), foamControls.w)*furier_largeN.z 
									+ lerp(0.2f, 0.4f*saturate(0.2f*wdepth), foamControls.w)*furier_mediumN.z, 
									0.0f, 100.0f );		


		foamControls.z *= distanceDamp;		
		
		furier_largeN.z = F_LARGE_Nz;		
		furier_largeN.xyz = normalize( lerp(float3(0,0,1), normalize(furier_largeN.xyz), getDS_DistanceDamp( i.globalPos.xy, PSC_CameraPosition.xy, 2000.0f ) * saturate(shoreProximity.x) ) );				

		furier_mediumN.z = F_MEDIUM_Nz;		
		furier_mediumN.xyz = normalize( lerp(float3(0,0,1), normalize(furier_mediumN.xyz), getDS_DistanceDamp( i.globalPos.xy, PSC_CameraPosition.xy, 40.0f ) * saturate(shoreProximity.y) ) );		

		furier_smallN.z = F_SMALL_Nz;		
		furier_smallN = normalize( lerp(float3(0,0,1), normalize(furier_smallN.xyz), getDS_DistanceDamp( i.globalPos.xy, PSC_CameraPosition.xy, 20.0f ) * saturate(shoreProximity.z) ) );
	}

	furier_mediumN.xyz = addNormals( furier_mediumN.xyz, furier_smallN.xyz );
	
	float3 N = addNormals( furier_largeN.xyz, furier_mediumN.xyz );		
	
	// river decals will override fourier normal
	N.xy *= saturate(1.0f-decal0);
	N.xyz = normalize(N.xyz);
		
	float3 dynamicCollidersN = float3( 0.0f, 0.0f, 0.005f );
	{
    	float2 www = ((i.globalPos.xy - SimulationCamera.position.xy)/DYNAMIC_WATER_WORLD_SIZE)+float2(0.5f,0.5f);
    	
		float4 dynam = SAMPLE_LEVEL( t_dynamic, s_dynamic, www , 0 );
		foamControls.xy = dynam.xy;
		dynamicCollidersN.xy = dynam.xy;

		const float volumeCut = CalculateVolumeCutByPixelCoord( i.pos.xy, i.globalPos.xyz, true );
		
		float3 dynamicCollidersNrain = float3( dynam.zw * saturate(1.0f-length(i.globalPos.xy- PSC_CameraPosition.xy)*0.05f) * volumeCut, 0.05f );
		dynamicCollidersN.xyz = normalize( dynamicCollidersN.xyz );
		dynamicCollidersNrain.xyz = normalize( dynamicCollidersNrain.xyz );
			
		dynamicCollidersN.xyz = normalize( dynamicCollidersNrain.xyz*(dynamicCollidersN.z) + dynamicCollidersN.xyz );

		// dampen regular normal if collider present
		N.z *= lerp( 40.0f, 1.0f, saturate( dynamicCollidersN.z * deacal_norm.z ) );
		N.xy *= lerp( 0.00001f, 1.0f, saturate( dynamicCollidersN.z * deacal_norm.z ) );

		// add dynamic colliders
		N.xyz = addNormals( dynamicCollidersN, normalize(N.xyz) );		
	}

	// add decals
	N.xyz = addNormals( deacal_norm.xyz, N.xyz );	

	float3 Nspec = N;

	// calculate depth		
	float rawDepth = SYS_SAMPLE( PSSMP_SceneDepth, uv.xy ).x;
	float sceneDepth = DeprojectDepthRevProjAware( rawDepth );
	
	float wdepth_viewSpace = abs(sceneDepth - i.viewPosZ);		
	float underwaterEdgeFactor = pow( saturate( 4.5f*abs( dot(i.globalPos.xyz - PSC_CameraPosition.xyz, PSC_CameraVectorForward.xyz) ) ), 6.4f );

	// calculate general refractive uvs'	
	float2 uvRef = lerp(0.03f, 0.015f, foamControls.w)*( N.xy );
	uvRef *= saturate( 2.0f*wdepth_viewSpace );
	uvRef *= underwaterEdgeFactor;
	uvRef *= smoothNormal;

	float boatCutoff = 0.0f;	
	
	// lama LMSH
	for(int j=0; j<(int)Displacements.numDispl; ++j)
	{	
		const float4 dispRow1 = Displacements.displacementData[ (4*j)+0 ];
		const float4 dispRow2 = Displacements.displacementData[ (4*j)+1 ];
		const float4 dispRow3 = Displacements.displacementData[ (4*j)+2 ];
		const float4 dispPos  = Displacements.displacementData[ (4*j)+3 ];			
		
		const float3 offsetPos = i.globalPos.xyz - dispPos.xyz;		
		{
			const float locX = dot( offsetPos, dispRow1.xyz )/( dot( dispRow1.xyz, dispRow1.xyz ) );
			const float locY = dot( offsetPos, dispRow2.xyz )/( dot( dispRow2.xyz, dispRow2.xyz ) );
			const float locZ = dot( offsetPos, dispRow3.xyz )/( dot( dispRow3.xyz, dispRow3.xyz ) );

			const float3 localPos = float3( locX, locY, locZ );					
			//if( length(localPos) < 1.0f ) boatCutoff += 1.0f;
			boatCutoff += 1.0f-pow( saturate( length( localPos ) ), 16.0f );
		}
	}	
		
	boatCutoff = saturate(boatCutoff);	

	// manipulate under the boat	
	foamControls.z *= (1.0f - boatCutoff);
	uvRef *= (1.0f - boatCutoff);
	
	// recalculate depth - since its uv being manipulated by uvRef
	{
		const float depth2 = SYS_SAMPLE( PSSMP_SceneDepth, uv.xy + uvRef.xy ).x;
		const float sceneDepth2 = DeprojectDepthRevProjAware( depth2 );
		
		if ( dot( worldToView[2].xyz, (i.globalPos.xyz - PSC_CameraPosition.xyz) ) > sceneDepth2 )
		{
			uvRef = 0;
		}
		else
		{
			wdepth_viewSpace = abs(sceneDepth2 - i.viewPosZ);
		}		
	}	
	
	float4 baseFoamColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
	baseFoamColor.w = wdepth;
	{
		//foam part
		float3 baseFoamUV = getFoamUV( i.uv, uvRef.xy*(2.0f - foamControls.w) + dynamicCollidersN.xy*0.1f + deacal_norm.xy*0.25f, PSC_TimeVector.x );		
		float2 shallow_foamUV = 0.05f*PSC_TimeVector.x + ( 2.2f*baseFoamUV.xy + ( dynamicCollidersN.xy*0.3f*cos(PSC_TimeVector.x) ) );		
	
		shallow_foamUV += 0.4f*N.xy;
		baseFoamColor.y = t_array.Sample( s_array, float3( shallow_foamUV, 0.0f) ).x;		
	
		const float3 baseDeltaUV = getFoamUV( 0.15f*(0.2f*i.uv.xy + 0.8f*i.globalPos.xy), 4.0f*uvRef, 0.5f*PSC_TimeVector.x );	
		baseFoamColor.x = t_array.Sample( s_array, baseDeltaUV ).y;
	}	

	baseFoamColor.y *= baseFoamColor.x;
	
	const float3 V = normalize( PSC_CameraPosition.xyz - i.globalPos.xyz + float3(0.005f, 0.005f, 0.05f)*N.xyz );	
	const float3 L = normalize( PSC_GlobalLightDir.xyz );
	const float3 H = normalize( V + L );	
		
	float3 refractiveColor = float3(0.0f, 0.0f, 0.0f);		
	
	float3 baseRefractiveColor = SYS_SAMPLE( PSSMP_SceneColor, uv.xy + uvRef.xy).xyz;
	{
		const float depth_cut = saturate(3.0f*wdepth_viewSpace);	
		const float depth_add = 1.0f - saturate(0.5f*wdepth_viewSpace);

		int samplesNum = 0;
		const float underwaterBlur = 0.0015f * depth_cut * (1.0f - boatCutoff);			
	
		const float fuzziness = 2.0f*( furier_mediumN.z + furier_mediumN.x + furier_mediumN.y );

		float2 blurOffsets[6];
		blurOffsets[0] = float2(0.0f,1.0f);
		blurOffsets[1] = float2(1.0f,0.7f);
		blurOffsets[2] = float2(1.0f,-0.7f);
		blurOffsets[3] = float2(0.0f,-1.0f);
		blurOffsets[4] = float2(-1.0f,-0.7f);
		blurOffsets[5] = float2(-1.0f,0.7f);		

		// blur
		[unroll]
		for(int _i=0; _i<6; _i++)
		{	
			float2 offset = blurOffsets[_i];
			offset *= underwaterBlur * fuzziness * (1.0f + clamp(1.0f - dot( refractiveColor.xyz, float3(1.0f,1.0f,1.0f) ), -0.5f, 0.5f ));
			offset *= depth_add;		
			
			refractiveColor.xyz += clamp( SYS_SAMPLE( PSSMP_SceneColor, clamp( uv.xy + ( offset + uvRef ), float2(0.0, 0.0), refractionClampMax ) ).xyz, 0.01f*baseRefractiveColor.xyz, 4.0f*baseRefractiveColor.xyz );
			samplesNum += 1;
		}
		refractiveColor.xyz /= samplesNum;	
	}	
	
	// fake caustics
	{		
		const float2 cs_uv = getCausticsUV( 0.12f, PSC_CameraPosition.xyz, PSC_CameraVectorForward.xyz, 0.005f*normalize(N.xy), i.globalPos.xyz, sceneDepth, lightDir.xyz );	
		const float cs_speed = 0.005f*PSC_TimeVector.x;						
		baseFoamColor.z = t_array.Sample( s_array, float3( cs_uv.xy + cs_speed, 0.0f ) ).w;	
		baseFoamColor.z *= lerp( 0.0f, saturate(1.7f - wdepth), saturate(15.0f*wdepth) );				
	}

	// shadow	
	float shadowFactor = 1.0f;
	{
		float3 shadowPos = i.globalPos.xyz - float3( 3.0f*(1.02f-smoothNormal) * N.xy, 0.0f );
		
		shadowFactor = CalcTerrainShadows( shadowPos.xyz );
		[branch]
		if( shadowFactor > 0.0f )
		{
			shadowFactor = min(shadowFactor, CalcShadowFactor( shadowPos, false, 1000, false ) );
		}
		shadowFactor *= CalculateCloudsShadow( shadowPos.xyz );
	}	
		 
	float3 worldPos = PositionFromDepthRevProjAware( sceneDepth, uv );
	// reference level
	worldPos.z -= i.globalPos.w;

	float reflectionAmount = 0.0f;		
	
	o.color.xyz = getWaterSurfacePS
	( 	
		wdepth,	
		caveFactor,
		underwaterEdgeFactor,
		wdepth_viewSpace,		
		N, Nspec,				
		baseFoamColor, 		
		H, L, V,
		refractiveColor, 					
		shadowFactor, 		
		PSC_CameraVectorForward.xyz,		
		(PSC_CameraPosition.xyz - i.globalPos.xyz),		
        foamControls,			
		reflectionAmount,		
		decal0, 		
		i.paramsFog.xyzw,
		i.paramsAerial.xyzw,
		worldPos.xyz
	);

	/////////
	o.color.xyz = lerp( o.color.xyz, baseRefractiveColor, boatCutoff );
	
	float reflNormalCoef = lerp(0.015f, 0.1f, smoothNormal);
	bool useGlobalReflection = true;
	if( caveFactor < 0.999f ) 
	{
		// bump up the reflection normals in caves
		reflNormalCoef *= 6.0f*(1.0f-caveFactor);
		useGlobalReflection = false;
	}

	o.rlrMask = (1.0f-boatCutoff)*EncodeRealtimeReflectionsMask( reflNormalCoef*N.xy, reflectionAmount, useGlobalReflection );

#if DEBUG_WATER_SOLID_COLOR
	o.color.xyz = float3(0.5f,0.5f,0.5f);
	o.rlrMask = 0.0f;
#endif 
#if DEBUG_DYNAMIC_SOLID_COLOR
	float d = (dot( dynamicCollidersN, normalize(float3(1.0f,1.0f,-0.6f)) )*0.5f)+0.5f;
	float dr = (dot( dynamicCollidersNrain, normalize(float3(1.0f,1.0f,-0.6f)) )*0.5f)+0.5f;
	d=(d+dr)*0.5f;
	o.color.xyz = float3(d,d,d);
	o.rlrMask = 0.0f;
#endif
#if DEBUG_WATER_DECAL
	float de = SAMPLE( t_decal, s_decal, saturate(decal_uv) ).z;
	o.color.xyz = float3(de,de,de);
	o.rlrMask = 0.0f;
#endif
#if DEBUG_DELTA	
	
	foamControls.z = pow( clamp(foamControls.z, 0.0f, 10.0f), 2.2f );
	o.color.xyz = ( float3( foamControls.z, foamControls.z, foamControls.z ) );
	o.rlrMask = 0.0f;
#endif

	return o;
}

#endif
