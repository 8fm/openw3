float3 CompressNormalsToUnsignedGBuffer( float3 vNormal )
{ 
	vNormal.xyz = normalize( vNormal.xyz );

	float3 vNormalUns = abs( vNormal.xyz );
	float maxNAbs = max( vNormalUns.z, max(vNormalUns.x,vNormalUns.y) );
	float2 vTexCoord = vNormalUns.z < maxNAbs ? (vNormalUns.y < maxNAbs ? vNormalUns.yz : vNormalUns.xz):vNormalUns.xy;
	vTexCoord = vTexCoord.x < vTexCoord.y ? vTexCoord.yx : vTexCoord.xy;
	vTexCoord.y /= vTexCoord.x;
	vNormal.xyz /= maxNAbs;
	float fFittingScale = SYS_SAMPLE_LEVEL( PSSMP_NormalsFitting, vTexCoord, 0 ).x;
	vNormal.xyz *= fFittingScale;

	vNormal.xyz = (vNormal.rgb * .5f) + .5f;
	return vNormal;
}