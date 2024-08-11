
#include "common.fx"
#include "globalConstants.fx"
#include "globalConstantsPS.fx"
#include "globalConstantsVS.fx"
#include "include_constants.fx"

#ifndef GLOBAL_OCEAN_CONSTANTS_EXCLUDE_SHAPES
//	local water areas
TEXTURE2D_ARRAY			t_shapesarray : register (t2);
SamplerState			s_shapesarray : register (s2);
#endif

//	terrain heightmap
TEXTURE2D_ARRAY<float>	tHeightmap 	: register( t4 );
SamplerState			sHeightmap 	: register( s4 );

// heightmapIndex
TEXTURE2D<float4>		tClipWindows : register( t17 );

// Use the kosher unified terrain constants block
#include "terrainConstants.fx"
DEFINE_TERRAIN_PARAMS_CB( 5 );

#define F_LARGE_Nz 0.00015f
#define F_MEDIUM_Nz 0.00035f
#define F_SMALL_Nz 0.0008f

#define EXTINCTION float3(0.5f, 0.32f, 0.34f)
#define EXTINCTION_NIGHT float3(0.33f, 0.32f, 0.31f)
#define SCATTERING float3(0.5f, 0.74f, 1.0f)

#ifndef GLOBAL_OCEAN_CONSTANTS_EXCLUDE_SHAPES
float getWaterAreaOffset( int numOfAreas, float3 globalPos )
{
	float wateroffset = 0.0f;

	for(int j=0;j<numOfAreas;j++)
	{
		const float4 off = Lakes.offsets[j];		
		const float2 lakeuv = (globalPos.xy - off.xy) / off.zw;

		wateroffset = SAMPLE_LEVEL( t_shapesarray, s_shapesarray, float3( lakeuv, j ), 0 ).x;

		if( abs(wateroffset) > 0.001f ) break;
	}
	return wateroffset;
}
#endif

#if defined (VERTEXSHADER) || defined (PIXELSHADER)
bool RectContains( float2 pos, float4 rect )
{	
	bool ret = false;
	//if ( pos.xy > rect.xy && pos.xy < rect.zw )
	if ( pos.x > rect.x && pos.x < rect.z
		&& pos.y > rect.y && pos.y < rect.w )
	{
		ret = true;
	}
	return ret;
}

int DetermineClipmapLevel( float2 worldPos )
{
	float numClipmapWindows = 0.0f;
	float numArrays = 2.0f;
	tClipWindows.GetDimensions( numClipmapWindows, numArrays );

	for ( int windowIdx=0; windowIdx<(int)numClipmapWindows; ++windowIdx )
	{
		float4 windowRect = tClipWindows.Load( int3( windowIdx, 0, 0 ) );
		if ( RectContains( worldPos.xy, windowRect ) )
		{ 
			return windowIdx;
		}
	}
	// cannot contain at all
	return -1;
}

float GetTerrainHeight( float2 worldPos, int clipmapLevel )
{		
	// default, outside the terrain unsderwater deph
	if( clipmapLevel < 0 ) return -10.0f;

	// Choose clipmap level and compute UV for it
	float2 regionUV;

	float4 windowRect = tClipWindows.Load( int3( clipmapLevel, 0, 0 ) );
	float4 validTex = tClipWindows.Load( int3( clipmapLevel, 1, 0 ) );
	
	float3 clipmapDim;
	tHeightmap.GetDimensions( clipmapDim.x, clipmapDim.y, clipmapDim.z );
	
	regionUV = abs( ( worldPos.xy - windowRect.xy ) / ( windowRect.zw - windowRect.xy ) );

	// Offset UV's by half-texel. This puts the texels at the vertex positions, instead of between vertices.
	regionUV.xy += 0.5f / clipmapDim.xy;

	regionUV = ( regionUV * ( validTex.zw - validTex.xy ) + validTex.xy );

	// Sample heightmap
	float4 height = SAMPLE_LEVEL( tHeightmap, sHeightmap, float3(regionUV, (float)clipmapLevel), 0 );	
	return lerp(TerrainParams.lowestElevation.x, TerrainParams.highestElevation.x, saturate( height.x ) );	
}

float3 getWaterShoreProximityDampValue( float hmapHeight, float referenceWaterLevel )
{	
	const float2 highWavesRange = float2(0.1f, 5.0f);
	const float2 mediumWavesRange = float2(0.1f, 2.0f);		

	const float waterDepth = abs(referenceWaterLevel - hmapHeight);
	const float highWaterRange = 		saturate( (waterDepth - highWavesRange.x)/(highWavesRange.y - highWavesRange.x) );	
	const float mediumWaterRange = 	saturate( (waterDepth - mediumWavesRange.x)/(mediumWavesRange.y - mediumWavesRange.x) );
	
	return float3( lerp( 0.3f, 1.0f, highWaterRange ), lerp( 0.6f, 1.0f, mediumWaterRange ), 1.0f );	
}

float getWaterBelowGroundDampValue( float hmapHeight, float referenceWaterLevel )
{	
	return (1.0f - saturate(- 0.1f*( referenceWaterLevel - hmapHeight)) );
}


float getDS_DistanceDamp( float2 globalPosXY, float2 viewPosition, float cutDistance )
{
	const float distToCamera = length( globalPosXY.xy - viewPosition.xy );
	
	return saturate( ( cutDistance - distToCamera )/cutDistance );
}

float2 getCausticsUV( float cs_size, float3 cameraPosition, float3 cameraVectorForward, float2 N, float3 globalPos, float sceneDepth, float3 lightDir )
{
	float3 dir = normalize( globalPos.xyz - cameraPosition.xyz );    
	float  dd = dot(cameraVectorForward.xyz, dir);
    dir*=(1.0f/dd);
    
    float3 hitPos = cameraPosition.xyz + (dir)*sceneDepth;

	hitPos.xyz = cameraPosition.xyz + dir*sceneDepth;

	return float2( hitPos.xy*cs_size + N.xy - 0.1f*lightDir.xy * hitPos.z );
}

float3 addNormals( float3 n1, float3 n2 )
{
    const float3 t = normalize(n1.xyz) + float3(0,0,1);
    const float3 u = normalize( float3( -1.0f*n2.xy, n2.z ) );

    const float3 r = t*dot(t, u) - u*t.z;
    return normalize(r);
}

#endif

#ifdef PIXELSHADER

// disbaled due to bugs, will decide if that should be exposed later
#define globalWaterTransparency				1.7f
#define globalWaterColor 					float3(PSC_WaterShadingParams.x, PSC_WaterShadingParams.y, PSC_WaterShadingParams.z)
#define globalWaterFresnelGain				PSC_WaterShadingParamsExtra.x
#define globalWaterCaustiscGain				PSC_WaterShadingParamsExtra.y
#define globalWaterAmbientScale				PSC_WaterShadingParamsExtra.z
#define globalWaterDiffuseScale				PSC_WaterShadingParamsExtra.w
#define globalWaterFoamIntensity			clamp(PSC_WaterShadingParamsExtra2.x + 0.5f, 0.0f, 20.0f)
#define globalUnderwaterBrightness			clamp( PSC_UnderWaterShadingParams.x, 0.0f, 10.0f )
#define globalUnderwaterFogIntensity		PSC_UnderWaterShadingParams.y
#define globalUnderwaterFogColor			PSC_WaterShadingParamsExtra2.yzw

float waterFresnel( float roughness, float specularity, float3 H, float3 V )
{	
	//F term			
	const float pow_base  = saturate(1 - saturate(dot(H,V)));

	const float gloss = 1 - roughness;			
	return specularity + max( 0.0f, max( gloss, specularity ) - specularity )*pow( pow_base, 4.0f );	
}

float3 waterSpecular( float roughness, float specularity, float3 N, float3 V, float3 L, float3 H )		
{
	float3 specular = float3(0,0,0);
		
	[branch]
	if ( dot(L,N) > 0.0f )
	{			
		const float sNH = saturate( dot( N, H ) );
		const float sNL = saturate( dot( N, L ) );	
		
		float D = 0.0f;
			
			// D term
		float a = pow(roughness, 2.0f);
		float aSq = pow(a, 2.0f);
				
		D = aSq/( 3.14152f*( pow( ( pow( sNH, 2.0f )*( aSq-1.0f ) + 1.0f ), 2.0f ) ) );
		float manipulatedNV = abs( dot( N, V ) ); 

		//G term
		float k = pow( (roughness + 1.0f), 2.0f )/8;
		float Gl = sNL/(sNL*(1-k) + k);
		float Gv = manipulatedNV/(manipulatedNV*(1-k) + k);
		float G = Gl * Gv;
		
		specular = D * G * waterFresnel( roughness, specularity, H, V )/(4.0f*manipulatedNV*sNL);			

		specular = saturate( sNL*specular );			
	}	
		
	return ( pow(lightColor.xyz, 1.0f/2.2f) )*specular.xyz;
}

float3 waterSimpleSpecular( float3 L, float3 H, float gloss, float3 N ) 
{
	const float nh = max(0.0f, dot(N, H));
	const float specular = pow(nh, gloss);

	return ( pow(lightColor.xyz, 1.0f/2.2f) )*specular.xxx;
}


float3 getFoamUV( float2 uv, float2 uvRef, float timeVec )
{
	float3 baseFoamUV = float3(1.2f*uv, 0.0f);
	baseFoamUV.xy += 0.16f*uvRef.xy;
	
	return baseFoamUV;
}

float3 getWaterSurfacePS
(
	float wdepth,
	float caveFactor,
	float underwaterEdgeFactor,
	float wdepth_viewSpace,	
	float3 N, float3 Nspec,
	float4 baseFoamColor, 	
	float3 H, float3 L, float3 V, 
	float3 refractiveColor,	
	float shadowFactor, 	
	float3 cameraVectorForward,
	float3 toCamera,	
    float4 foamControls,
	out float outReflectionAmount,	
	float river_decal,	
	float4 paramsFog,
	float4 paramsAerial,
	float3 worldPos
)
{		
	// dampen reflection when underwater	
	// 2d
	const float toCameraLength = length( toCamera.xy );

	//// make a smooth line on underwater intersection		
	underwaterEdgeFactor *= lerp( 0.65f, 1.0f, saturate(1.0f + toCamera.z) );
	
	float dynamicWaterTransparencyGain = abs( foamControls.x ) + abs( foamControls.y );	
	
	// depth transparency		
	// caveFactor < 1 means that we are underground, so we will manipulate the depth even more	
	float manipulated_wdepth = pow( lerp( wdepth_viewSpace, wdepth, saturate( 0.005f*toCameraLength - (1.0f-caveFactor) ) ), 0.6f );		
	const float _wdepthExt = manipulated_wdepth;
																		// use skyAmount to determine extinction
	float3 depthExtinction = float3(_wdepthExt, _wdepthExt, _wdepthExt) * lerp( EXTINCTION_NIGHT.xyz, EXTINCTION.xyz, saturate( PSC_GlobalLightDir.w ) );
	
	// shallow foam   
	float finalFoam = baseFoamColor.y * saturate( 1.7f - wdepth ) * saturate(foamControls.z);
	finalFoam = lerp(0.3f, 1.0f, foamControls.w) * saturate( (finalFoam - 10.0f*dynamicWaterTransparencyGain) );
	
	finalFoam += saturate( (0.9f*baseFoamColor.y + 0.15f*baseFoamColor.x) * 20.0f*dynamicWaterTransparencyGain*min(0.3f,foamControls.z) );	

	const float roughnessFactor = lerp( 1.0f, 0.0f, saturate( 2.2f*(foamControls.w - 0.6f) ) );
	float3 specular = float3(0.0f, 0.0f, 0.0f);
	// hack specular		
	//{
		//const float roughness = lerp(0.2f, 0.33f, foamControls.w);
				
		Nspec.z += 0.016f*toCameraLength;	
		Nspec.xy *= max(0.05f, saturate( 1 - toCameraLength / 180.0f ) );
		Nspec = normalize(Nspec);	
			
		// dampen specular for stormy waves (should be managed by artists using light power, but its not)		
		//specular = 2.5f*waterSpecular( roughness, 0.2f* pow(saturate( 1.15f - foamControls.w ), 2.0f), Nspec.xyz, V, L, H );	
		
		// consoles
		//const float gloss = clamp( 0.02f*toCameraLength, 160.0f, 1800.0f );
		//specular = lerp( 20.0f, 2.5f, pow(saturate(L.z+0.4f), 2.0f) )*waterSimpleSpecular( L, H, gloss, Nspec );
		
		// covers cheap specular missing G term
		//specular *= clamp( 0.05f* toCameraLength, 1.0f, 10.0f );
	//}	
	
	// manipulate specular for horizon fade (not physically corrct, art req)
	//specular *= saturate( 1.0f - (toCameraLength - 25.0f) / 480.0f );	
	//specular *= saturate( 0.2f*toCameraLength );
	//specular *= max( 0.2f, roughnessFactor );


	specular = waterSpecular( 0.1f, 0.7f* pow(saturate( 1.15f - foamControls.w ), 2.0f), Nspec.xyz, V, L, H );	
	specular *= caveFactor;	
	
	// subsurface scattering backlight	
	float colorScale = 0.1f*pow( foamControls.z, clamp(7.0f - 5.0f*foamControls.w, 0.01f, 7.0f ) );
	colorScale *= lerp( 0.2f, 1.0f, 1.0f-saturate( dot( N.xyz, PSC_GlobalLightDir.xyz ) ) );	
	colorScale *= caveFactor*saturate(2.0*globalWaterFoamIntensity);

	float3 waterColor = globalWaterColor;
			
	// darken background with the water depth
	refractiveColor.xyz *= lerp( 1.0f, 0.75f, saturate( underwaterEdgeFactor*0.5f*manipulated_wdepth ) );		
				
	{
		// add foam at peaks			
		float foamPeaksColor = lerp( pow(0.01f*baseFoamColor.x, 8.0f), baseFoamColor.x , saturate( pow( 0.5f*foamControls.z, 12.0f ) ) );		

		foamPeaksColor = lerp( foamPeaksColor, 0.5f*pow( foamPeaksColor, 0.5f ), foamControls.w );

		foamPeaksColor *= 1.0f - saturate( 1.0f - wdepth );
		foamPeaksColor *= saturate(foamControls.w);
		// add no peaks when river foam on top
		foamPeaksColor = lerp( foamPeaksColor, 0.0f, saturate(river_decal) );

		// adding all the foam stuff TODO	
		finalFoam = max(foamPeaksColor + 0.5f*dynamicWaterTransparencyGain*baseFoamColor.y, finalFoam);
	}	
			// expose foam color intensity here
	finalFoam *= lerp( 20.0f*globalWaterAmbientScale, 20.0f*globalWaterDiffuseScale, shadowFactor );
	
	// dampen foam under ground caves
	finalFoam *= lerp( 0.1f, 1.0f, caveFactor );

	// adding fake caustics		
	{
		float fakeCaustics = saturate( globalWaterCaustiscGain * baseFoamColor.z );	
		const float manipulatedShadowFactor = lerp(0.3f, 1.0f, shadowFactor );
		fakeCaustics *= saturate(3.0f*colorScale);
		waterColor.xyz += manipulatedShadowFactor * fakeCaustics;	
		refractiveColor.xyz += manipulatedShadowFactor * fakeCaustics;
	}

	// add specular lighting (cutt of specular for shadows)	
	waterColor.xyz += specular*(shadowFactor);			

	// scattering
	// hack subsurf			
	waterColor.xyz += max( saturate(3.0f - foamControls.w)*SCATTERING * colorScale, globalWaterFoamIntensity*finalFoam );			

	// adding rivers foam on top (currently overriding envs settings, no lighting - looks much better)
	river_decal *= globalWaterFoamIntensity;
	waterColor += lerp( 1.5f*saturate( 0.85f*baseFoamColor.x + 0.15f ) * 3.0f*pow( river_decal, 1.6f ), 0.0f, saturate(20.0f*dynamicWaterTransparencyGain) );
	waterColor *= caveFactor;	

	float3 finalColor = float3(0.0f, 0.0f, 0.0f);	

	SFogData fogData;
	fogData.paramsFog = paramsFog;
	fogData.paramsAerial = paramsAerial;

	// add fresnel term
	//< this is to reduce double fogging which happens at distant, fogged areas (fog is in reflections and on the actual surface). so let's just remove reflection where the fog is high.			
	outReflectionAmount = pow( waterFresnel( 0.7f, 0.05f, N, V ), clamp(0.5f + globalWaterFresnelGain, 0.01f, 6.0f) );
	outReflectionAmount *= pow( (1 - fogData.paramsFog.w), 2.5f );
	outReflectionAmount *= underwaterEdgeFactor;
	
	outReflectionAmount *= lerp( globalWaterAmbientScale, globalWaterDiffuseScale, shadowFactor );		//< far from perfect conversion, but the only way to go with separated reflection pass
			
	float underwaterFactor = clamp(1.0-saturate( -10.0f*worldPos.z), 0.01f, 1.0f);	
	//finalColor = lerp(refractiveColor.xyz, waterColor.xyz, saturate( underwaterFactor*underwaterEdgeFactor * depthExtinction.xyz ) );	
	finalColor = lerp(refractiveColor.xyz, waterColor.xyz, saturate( underwaterFactor*underwaterEdgeFactor*depthExtinction.xyz ) );	
	finalColor = lerp( finalColor.xyz, globalUnderwaterBrightness*globalUnderwaterFogColor.xyz, saturate( 0.011f*toCameraLength*(1.0f-underwaterFactor) ) );	

	// add a foam layer on shallow water	
	finalColor.xyz += lerp( 0.0f, globalWaterFoamIntensity * finalFoam * saturate( 1.0f-wdepth ), saturate( 2.0f*wdepth ) );

	// add fog
	finalColor.xyz = ApplyFogDataFull( finalColor.xyz, fogData ).xyz;					
	
	// art req, bumps contrast
	outReflectionAmount *= lerp( 0.4f, 1.0f, saturate( 4.0f*pow(colorScale, 0.5f) ) );

	// bump up the reflection in caves	
	outReflectionAmount += 0.5f*(1.0f-caveFactor);
	outReflectionAmount *= saturate( depthExtinction.x );
	outReflectionAmount = saturate( outReflectionAmount );		

	// dump it when underwater and looking up
	outReflectionAmount *= clamp(underwaterFactor, 0.5f, 1.0f);

	return finalColor.xyz;			
}
#endif
