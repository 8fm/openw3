#ifndef GLOBAL_FOG_INCLUDED
#define GLOBAL_FOG_INCLUDED

float CalcFogDensityScale( float3 pos, float4 densityParams, float densityFactor )
{	
	float ref_pos_offset 	= fogBaseParams.y;
	float density_factor	= densityFactor;
	
	float h = pos.z + ref_pos_offset;
	float _scale = 1.0;
	
	/*
	{
		float cs = CalculateCloudsShadow( pos/10 );
		float cc = saturate( (pos.z - 400) / 340 );		
		cs = lerp( cs, 0.55, cc );
		_scale *= cs;
	}
	//*/
	
	float result = _scale * saturate( 1 / (1 + max( 0.0, density_factor * h ) ) );	
	return result;
}

struct SFogData
{
	float4 paramsFog;
	float4 paramsAerial;
};

SFogData CalculateFogFullCustomCamera( in bool isSky, in bool isClouds, float3 fragPosWorldSpace, float3 customCameraPos, float fogStartDist = 0 )
{	
	float4 density_params 	= isSky ? fogDensityParamsSky : ( isClouds ? fogDensityParamsClouds : fogDensityParamsScene );

	float density    		= density_params.x;	
	float dist_clamp 		= fogBaseParams.z;
	float final_exp_fog		= fogBaseParams.w;
	float final_exp_aerial	= aerialParams.x;
	
	// 

	float3 frag_vec		= fragPosWorldSpace.xyz - customCameraPos.xyz;
	float  frag_dist    = length( frag_vec );
	float3 frag_dir		= frag_vec / frag_dist;

	customCameraPos += frag_dir * fogStartDist;
	frag_dist = max( 0, frag_dist - fogStartDist );
	
	frag_dist = isSky || isClouds ? dist_clamp : min( dist_clamp, frag_dist );
	frag_vec = frag_dir * frag_dist;
	
	const float inv_num_samples = 1.0 / 16;
	float3 frag_step = frag_vec * inv_num_samples;
	float density_sample_scale = frag_dist * density * inv_num_samples;
	float dot_fragDirSunDir = dot( fogSunDir.xyz, frag_dir );

	float density_factor = 1.0;
	{
		float2 densities		= density_params.zy;
		float  density_shift 	= fogBaseParams.x;
	
		float fc_t = dot_fragDirSunDir;
		fc_t = (fc_t + density_shift) / (1.0 + density_shift);		
		fc_t = saturate( fc_t );		
		density_factor = lerp( densities.x, densities.y, fc_t );
	}
	
	float3 curr_col_fog;
	float3 curr_col_aerial;
	{
		float _dot = dot_fragDirSunDir;

		float _dd = _dot;
		{
			const float _distOffset = -150;
			const float _distRange = 500;
			const float _mul = 1.0 / _distRange;
			const float _bias = _distOffset * _mul;

			_dd = abs(_dd);
			_dd *= _dd;
			_dd *= saturate( frag_dist * _mul + _bias );
		}

		curr_col_fog = lerp( fogColorMiddle.xyz, (_dot>0 ? fogColorFront.xyz : fogColorBack.xyz), _dd );
		curr_col_aerial = lerp( aerialColorMiddle.xyz, (_dot>0 ? aerialColorFront.xyz : aerialColorBack.xyz), _dd );
	}
	
	float fog_amount = 1;
	float fog_amount_scale = 0;
	[branch]
	if ( frag_dist >= aerialParams.y )
	{
		float curr_pos_z_base = (customCameraPos.z + fogBaseParams.y) * density_factor;
		float curr_pos_z_step = frag_step.z * density_factor;
	
		[unroll]
		for ( int i=16; i>0; --i )
		{
			fog_amount *= 1 - saturate( density_sample_scale / (1 + max( 0.0, curr_pos_z_base + (i) * curr_pos_z_step ) ) );
		}

		fog_amount = 1 - fog_amount;
		fog_amount_scale = saturate( (frag_dist - aerialParams.y) * aerialParams.z );
	}

	SFogData fog_data;
	fog_data.paramsFog = float4 ( curr_col_fog, fog_amount_scale * pow( abs(fog_amount), final_exp_fog ) );
	fog_data.paramsAerial = float4 ( curr_col_aerial, fog_amount_scale * pow( abs(fog_amount), final_exp_aerial ) );

	// custom fog
	{
		float4 paramsFog;
		{
			const float customFogFactor = saturate( fog_data.paramsFog.a * fogCustomRangesEnv0.x + fogCustomRangesEnv0.y );
			const float customAmountFactor = saturate( fog_data.paramsFog.a * fogCustomRangesEnv0.z + fogCustomRangesEnv0.w );
			paramsFog.xyz = lerp( fog_data.paramsFog.xyz, fogCustomValuesEnv0.xyz, customFogFactor );
			paramsFog.w = saturate( fog_data.paramsFog.w * lerp( 1.0, fogCustomValuesEnv0.w, customAmountFactor ) );		
		}

		[branch]
		if ( mostImportantEnvsBlendParams.x > 0 )
		{
			float4 paramsFog2;
			{
				const float customFogFactor = saturate( fog_data.paramsFog.a * fogCustomRangesEnv1.x + fogCustomRangesEnv1.y );
				const float customAmountFactor = saturate( fog_data.paramsFog.a * fogCustomRangesEnv1.z + fogCustomRangesEnv1.w );
				paramsFog2.xyz = lerp( fog_data.paramsFog.xyz, fogCustomValuesEnv1.xyz, customFogFactor );
				paramsFog2.w = saturate( fog_data.paramsFog.w * lerp( 1.0, fogCustomValuesEnv1.w, customAmountFactor ) );
			}

			paramsFog = lerp( paramsFog, paramsFog2, mostImportantEnvsBlendParams.x );
		}

		fog_data.paramsFog = paramsFog;

		if( isClouds ) 
		{
			fog_data.paramsFog.w *= 1.0f - saturate( dot( float3(0,0,1), frag_dir.xyz ) );
		}

	}

	return fog_data;
}

SFogData CalculateFogFull( in bool isSky, in bool isClouds, float3 fragPosWorldSpace )
{	
	const float nearPlane = cameraDepthRange.z;
	return CalculateFogFullCustomCamera( isSky, isClouds, fragPosWorldSpace, cameraPosition.xyz, nearPlane );
}

float4 CalculateFog( in bool isSky, in bool isClouds, float3 fragPosWorldSpace )
{
	return CalculateFogFull( isSky, isClouds, fragPosWorldSpace ).paramsFog;
}

float3 ApplyFogDataFull( in float3 color, in SFogData fog )
{
	float3 aerialColor = dot( RGB_LUMINANCE_WEIGHTS_LINEAR_ApplyFogDataFull, color ) * fog.paramsAerial.xyz;
	//float3 aerialColor = min( 1, dot( RGB_LUMINANCE_WEIGHTS_LINEAR_ApplyFogDataFull, color ) / max( 0.0001, 1.0 - fog.paramsAerial.xyz ) )
	color = lerp( color, aerialColor, fog.paramsAerial.w );
	color = lerp( color, fog.paramsFog.xyz, fog.paramsFog.w );
	return color;
}

float3 ApplyFogData( in float3 color, in float4 fog )
{
	color = lerp( color, fog.xyz, fog.w );	
	return color;
}

float3 ApplyFog( in float3 color, in bool isSky, in bool isClouds, in float3 fragPosWorldSpace )
{
	return ApplyFogDataFull( color.xyz, CalculateFogFull( isSky, isClouds, fragPosWorldSpace ) );
}

// ---

float4 ApplyFog_Opaque( in float3 color, in SFogData fogData )
{
	return float4 ( ApplyFogDataFull( color.xyz, fogData ), 1 );
}

float4 ApplyFog_Opaque( in float3 color, in bool isSky, in bool isClouds, in float3 fragPosWorldSpace )
{
	return ApplyFog_Opaque( color.xyz, CalculateFogFull( isSky, isClouds, fragPosWorldSpace ) );
}

float4 ApplyFog_Additive( in float3 color, in float alpha, in SFogData fogData )
{
	// ace_todo: make somehow use of fog/aerial color here

	alpha *= saturate( 1 - fogData.paramsFog.w );
	return float4 ( color, alpha );
}

float4 ApplyFog_Additive( in float3 color, in float alpha, in bool isSky, in bool isClouds, in float3 fragPosWorldSpace )
{
	return ApplyFog_Additive( color, alpha, CalculateFogFull( isSky, isClouds, fragPosWorldSpace ) );
}

float4 ApplyFog_AlphaBlend( in float3 color, in float alpha, in SFogData fogData )
{
	return float4 ( ApplyFogDataFull( color.xyz, fogData ), alpha );
}

float4 ApplyFog_AlphaBlend( in float3 color, in float alpha, in bool isSky, in bool isClouds, in float3 fragPosWorldSpace )
{
	return ApplyFog_AlphaBlend( color, alpha, CalculateFogFull( isSky, isClouds, fragPosWorldSpace ) );
}

// ---

#endif
/* make sure leave an empty line at the end of ifdef'd files because of SpeedTree compiler error when including two ifdef'ed files one by one : it produces something like "#endif#ifdef XXXX" which results with a bug */
