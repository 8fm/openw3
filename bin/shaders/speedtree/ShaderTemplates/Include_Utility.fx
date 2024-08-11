///////////////////////////////////////////////////////////////////////
//
//	*** INTERACTIVE DATA VISUALIZATION (IDV) CONFIDENTIAL AND PROPRIETARY INFORMATION ***
//
//	This software is supplied under the terms of a license agreement or
//	nondisclosure agreement with Interactive Data Visualization, Inc. and
//  may not be copied, disclosed, or exploited except in accordance with
//  the terms of that agreement.
//
//      Copyright (c) 2003-2014 IDV, Inc.
//      All rights reserved in all media.
//
//      IDV, Inc.
//      Web: http://www.idvinc.com

#ifndef ST_INCLUDE_UTILITY
#define ST_INCLUDE_UTILITY


///////////////////////////////////////////////////////////////////////
//  Constants

#define ST_PI		3.14159265358979323846
#define ST_TWO_PI	6.283185307179586476925


///////////////////////////////////////////////////////////////////////
//  ConvertToStdCoordSys
//
//	By default, SpeedTree operates in a right-handed, Z-up coordinate system. This
//	function will convert from an alternate coordinate system to SpeedTree's right-
//	handed Z-up system if an alternate has been selected. The supported alternate
//	coordinate systems are right-handed Y-up, left-handed Y-up, and left-handed Z-up.

float3 ConvertToStdCoordSys(float3 vCoord)
{
	if (ST_COORDSYS_Z_UP)
		return ST_COORDSYS_LEFT_HANDED ? float3(vCoord.x, -vCoord.y, vCoord.z) : vCoord;
	else
		return ST_COORDSYS_LEFT_HANDED ? float3(vCoord.x, vCoord.z, vCoord.y) : float3(vCoord.x, -vCoord.z, vCoord.y);
}


///////////////////////////////////////////////////////////////////////
//  ConvertFromStdCoordSys
//
//	By default, SpeedTree operates in a right-handed, Z-up coordinate system. This
//	function will convert from SpeedTree's standard system to an alternate coord system
//	if one has been selected. The supported alternate coordinate systems are right-
//	handed Y-up, left-handed Y-up, and left-handed Z-up.

float3 ConvertFromStdCoordSys(float3 vCoord)
{
	if (ST_COORDSYS_Z_UP)
		return ST_COORDSYS_LEFT_HANDED ? float3(vCoord.x, -vCoord.y, vCoord.z) : vCoord;
	else
		return ST_COORDSYS_LEFT_HANDED ? float3(vCoord.x, vCoord.z, vCoord.y) : float3(vCoord.x, vCoord.z, -vCoord.y);
}



///////////////////////////////////////////////////////////////////////
//  Helper Function: BuildOrientationMatrix
//
//	Given three vectors to define an orientation, a matrix is built based on
//	the rendering platform and chosen coordinate system.

float3x3 BuildOrientationMatrix(float3 vRight, float3 vOut, float3 vUp)
{
	if (ST_COORDSYS_Z_UP)
		return ST_OPENGL ? float3x3(vRight, vOut, vUp) : float3x3(vRight.x, vOut.x, vUp.x,
																  vRight.y, vOut.y, vUp.y,
																  vRight.z, vOut.z, vUp.z);
	else
		return ST_OPENGL ? float3x3(vRight, vUp, vOut) : float3x3(vRight.x, vUp.x, vOut.x,
																  vRight.y, vUp.y, vOut.y,
																  vRight.z, vUp.z, vOut.z);
}


///////////////////////////////////////////////////////////////////////
//  Helper Function: BuildBillboardOrientationMatrix
//
//	Given three vectors to define an orientation, a matrix is built based on
//	the rendering platform and chosen coordinate system to turn the facing
//	leaf card geometry; this function is pretty basic, the algorithm for
//	determining the vector values is in the calling code.

float3x3 BuildBillboardOrientationMatrix(float3 vRight, float3 vOut, float3 vUp)
{
	if (ST_OPENGL)
		return float3x3(vRight, vOut, vUp);
	else
	{
		if (ST_COORDSYS_LEFT_HANDED)
			return float3x3(-vRight.x, vOut.x, vUp.x,
							-vRight.y, vOut.y, vUp.y,
							-vRight.z, vOut.z, vUp.z);
		else
			return float3x3(vRight.x, vOut.x, vUp.x,
							vRight.y, vOut.y, vUp.y,
							vRight.z, vOut.z, vUp.z);
	}
}


///////////////////////////////////////////////////////////////////////
//  Helper Function: DotProductLighting
//
//	Simple lighting equation SpeedTree uses in a few shaders; SpeedTree lerps
//	from the ambient color to diffuse by the dot product of the normal and
//	light direction vectors.

float3 DotProductLighting(float3 vAmbientColor, float3 vDiffuseColor, float fDot)
{
	return lerp(vAmbientColor, vDiffuseColor, saturate(fDot));
}


///////////////////////////////////////////////////////////////////////
//  Helper Function: AmbientContrast
//
//	This is one technique SpeedTree employs to help keep the dark-side lighting
//	of vegetation interesting (non-flat). This is simpler than our image-based
//	ambient lighting but not mutually exclusive.

float AmbientContrast(float fContrastFactor, float fDot)
{
	return lerp(fContrastFactor, 1.0, abs(fDot));
}


///////////////////////////////////////////////////////////////////////
//  Helper Function: Fade3dTree
//
//	The SDK's Core library will compute a 3D instance's LOD value such that:
//		- [1.0 -> 0.0] means the 3D tree is moving from highest 3D LOD to
//                     lowest 3D LOD
//		- [0.0 -> -1.0] means the 3D tree is fading gradually from lowest
//                      3D LOD to full billboard
//
//	Return value for Fade3dTree: 1.0 is no fade, 0.0 is full fade

float Fade3dTree(float fLodValue)
{
	// C code:
	//if (fLodValue < 0.0f)
	//{
	//	// fade is active
	//	return 1.0f - -fLodValue;
	//}
	//else
	//	// fade is inactive
	//	return 1.0f;

	return 1.0 - saturate(-fLodValue);
}


///////////////////////////////////////////////////////////////////////
//  Helper Function: FadeBillboard
//
//	Given how far a billboard is from the camera (or LOD ref point) in world
//	units, this function will generate a [0.0, 1.0] scalar value that begins
//	at distance m_fBillboardStartDist and spans a distance of m_fBillboardRange.
//
//	Since billboards fade in as the camera moves further away, the scale value
//	will be 0.0 when (dist <= m_fBillboardStartDist), making the billboard invisible.
//	As the distance moves toward (m_fBillboardStartDist + m_fBillboardRange), it will
//	move linearly to 1.0, making the billboard fully visible.

float FadeBillboard(float fDistanceFromCamera)
{
	return saturate((fDistanceFromCamera - m_fBillboardStartDist) / m_fBillboardRange);
}


///////////////////////////////////////////////////////////////////////
//  Helper Function: FadeGrass
//
//	Given how far a grass instance is from the camera (or LOD ref point) in world
//	units, this function will generate a [0.0, 1.0] scalar value that begins
//	at distance m_f3dGrassStartDist and spans a distance of m_fBillboardRange.
//
//	Since grass instances fade out as the camera moves further away, the scale value
//	will be 1.0 when (dist <= m_f3dGrassStartDist), making the grass fully visible.
//	As the distance moves toward (m_f3dGrassRange + m_fBillboardRange), it will
//	move linearly to 0.0, making the grass invisible and able to be culled.

float FadeGrass(float fDistanceFromCamera)
{
	return saturate(1.0 - (fDistanceFromCamera - m_f3dGrassStartDist) / m_f3dGrassRange);
}


///////////////////////////////////////////////////////////////////////
//  Helper Function: ComputeTransmissionFactor
//
//	Transmission (SpeedTree's approximation of subsurface scattering), is
//	computed using the surface normal together with a "view dependency" value
//	set by the artist in the Modeler. View dependency controls how much the
//	viewing angle limits the effect of translucency. A value of 1.0 (the
//	default) fades light transmission in as the camera looks towards the
//	light source. A value of 0.0 allows light transmission on the front-lit
//	side.

float ComputeTransmissionFactor(float3 vNormal, float fViewDependency, bool bBackfaceCulling)
{
	// adjust normal to compensate for backfacing geometry
	if (bBackfaceCulling)
		vNormal = -vNormal;

	// how much is the camera looking into the light source?
	float fBackContribution = ST_COORDSYS_LEFT_HANDED ? (dot(-m_vCameraDirection, m_vDirLight.m_vDir.xyz) + 1.0) * 0.5 :
														(dot(m_vCameraDirection, m_vDirLight.m_vDir.xyz) + 1.0) * 0.5;

	// compute the result to get the right falloff
	fBackContribution *= fBackContribution;
	fBackContribution *= fBackContribution;

	// use the reverse normal to compute scatter
	float fScatterDueToNormal = (dot(-m_vDirLight.m_vDir.xyz, -vNormal) + 1.0) * 0.5;

	// choose between scatter and back contribution based on artist-controlled parameter
	float fTransmissionFactor = lerp(fScatterDueToNormal, fBackContribution, fViewDependency);

	// back it off based on how much leaf is in the way
	float fReductionDueToNormal = ST_COORDSYS_LEFT_HANDED ? (dot(-m_vCameraDirection, vNormal) + 1.0) * 0.5 :
															(dot(m_vCameraDirection, vNormal) + 1.0) * 0.5;
	fTransmissionFactor *= fReductionDueToNormal;

	return fTransmissionFactor;
}


///////////////////////////////////////////////////////////////////////
//  Helper Function: ProjectToScreen
//
//	Simple screen projection using a composite modelview / projection matrix
//	uploaded to m_mModelViewProj3d. SpeedTree subtracts the camera position
//	from the incoming position to help curb floating-point resolution issues
//	with large position values.

float4 ProjectToScreen(float4 vPos)
{
	// the camera stay at the origin, so everything else is translated to it
	vPos.xyz -= m_vCameraPosition;

	return mul_float4x4_float4(m_mModelViewProj3d, vPos);
}


///////////////////////////////////////////////////////////////////////
//	Helper Function: DecodeVectorFromColor
//
//	Converts a color of range [0, 1] to a vector of range [-1, 1].
//
//	float4 version.

float4 DecodeVectorFromColor(float4 vColor)
{
	return vColor * 2.0 - 1.0;
}


///////////////////////////////////////////////////////////////////////
//	Helper Function: DecodeVectorFromColor
//
//	Converts a color of range [0, 1] to a vector of range [-1, 1].
//
//	float3 version.

float3 DecodeVectorFromColor(float3 vColor)
{
	return vColor * 2.0 - 1.0;
}

// LAVA+
///////////////////////////////////////////////////////////////////////  
//	Helper Function: DecodeVectorFromDXTn

float3 DecodeNormalFromDXTn(float4 vDXTn)
{
	float3 vRawNormal = float3 ( 2.0 * vDXTn.wy - 1.0, 0 );
	vRawNormal.z = sqrt( max( 0, 1 - vRawNormal.x * vRawNormal.x - vRawNormal.y * vRawNormal.y ) );			
	return vRawNormal;
}
// LAVA-

///////////////////////////////////////////////////////////////////////
//	Helper Function: EncodeVectorToColor
//
//	Converts a vector of range [-1, 1] to a color of range [0, 1].

float3 EncodeVectorToColor(float3 vVector)
{
	return vVector * 0.5 + 0.5;
}


///////////////////////////////////////////////////////////////////////
//	Helper Function: DecodeFloat4FromUBytes
//
//	This function helps to align SpeedTree's supported platforms behaviors when
//	storing normal values as unsigned bytes.

// todo: really?
float4 DecodeFloat4FromUBytes(float4 vCompressedBytes)
{
	if (ST_DIRECTX11 || ST_PS4)
		return vCompressedBytes * 2.0 - 1.0;
	else if (ST_XBOX_360)
		// 360 automatically normalizes signed byte values when uploaded as D3DDECLTYPE_BYTE4N, but
		// free swizzle is still used to counter big endian
		return vCompressedBytes.wzyx / 127.5 - 1.0;
	else
		return vCompressedBytes / 127.5 - 1.0;
}


///////////////////////////////////////////////////////////////////////
//	Helper Function: CheckForEarlyExit
//
//	Basically this is a function to encapsulate alpha testing. It depends
//	on whether the app is using alpha testing or alpha-to-coverage to
//	handle transparency. Also note OpenGL's use of discard versus HLSL's
//	use of clip. In OpenGL, discard is not a valid call from a vertex shader
//	so it is #ifdef'd out.

void CheckForEarlyExit(float fAlphaValue, bool bTransparencyActive)
{
	if (bTransparencyActive)
	{
		#if (ST_OPENGL)
			// at least one GLSL compiler will not permit discard to be present in a vertex
			// shader's source, even if it isn't used, so we put this #ifdef around it
			#ifdef ST_PIXEL_SHADER
				if (fAlphaValue < ST_ALPHA_KILL_THRESHOLD)
					discard;
			#endif
		#else
			clip(fAlphaValue - ST_ALPHA_KILL_THRESHOLD);
		#endif
	}
}


///////////////////////////////////////////////////////////////////////
//  CubicSmooth
//
//	Used in conjunction with TrigApproximate()

float4 CubicSmooth(float4 vData)
{
	return vData * vData * (3.0 - 2.0 * vData);
}


///////////////////////////////////////////////////////////////////////
//  TriangleWave
//
//	Used in conjunction with TrigApproximate()

float4 TriangleWave(float4 vData)
{
	return abs((frac(vData + 0.5) * 2.0) - 1.0);
}


///////////////////////////////////////////////////////////////////////
//  TrigApproximate
//
//	Faster/approximate sin wave.

float4 TrigApproximate(float4 vData)
{
	return (CubicSmooth(TriangleWave(vData)) - 0.5) * 2.0;
}


///////////////////////////////////////////////////////////////////////
//  UnpackNormalFromFloat
//
//	Designed to work with 16-bit float values, this function pulls a float3
//	of range [-1, 1] from a single 16-bit float value. Also works with 32-bit
//	floats, but spreads the error better for 16.

float3 UnpackNormalFromFloat(float fValue)
{
	#define ST_DECODE_KEY float3(16.0, 1.0, 0.0625)

	// decode into [0,1] range
	float3 vDecodedValue = frac(fValue / ST_DECODE_KEY);

	// move back into [-1,1] range & normalize
	return (vDecodedValue * 2.0 - 1.0);

	#undef ST_DECODE_KEY
}


////////////////////////////////////////////////////////////
//	PackNormalIntoFloat_Stereographic
//
//	http://aras-p.info/texts/CompactNormalStorage.html

float2 PackNormalIntoFloat2_Stereographic(float3 n)
{
	float scale = 1.7777;

	float2 enc = n.xy / (n.z + 1.0);
	enc /= scale;
	enc = enc * 0.5 + 0.5;

	return enc;
}


////////////////////////////////////////////////////////////
//	UnpackNormalFromFloat_Stereographic
//
//	http://aras-p.info/texts/CompactNormalStorage.html

float3 UnpackNormalFromFloat2_Stereographic(float2 enc2)
{
	float4 enc = float4(enc2, 0, 0);

	float scale = 1.7777;
	float3 nn = enc.xyz * float3(2.0 * scale, 2.0 * scale,0) + float3(-scale, -scale, 1.0);
	float g = 2.0 / dot(nn.xyz, nn.xyz);

	float3 n;
	n.xy = g * nn.xy;
	n.z = g - 1.0;

	return n;
}


////////////////////////////////////////////////////////////
//	PackNormalIntoFloat_Spheremap
//
//	http://aras-p.info/texts/CompactNormalStorage.html

float2 PackNormalIntoFloat2_Spheremap(float3 n)
{
	float f = sqrt(8.0 * n.z + 8.0);

	return n.xy / f + 0.5;
}


////////////////////////////////////////////////////////////
//	UnpackNormalFromFloat_Spheremap

float3 UnpackNormalFromFloat2_Spheremap(float2 enc2)
{
	float2 fenc = enc2 * 4.0 - 2.0;
	float f = dot(fenc, fenc);
	float g = sqrt(1.0 - f / 4.0);
	float3 n;
	n.xy = fenc * g;
	n.z = 1.0 - f / 2.0;

	return normalize(n);
}


///////////////////////////////////////////////////////////////////////
//  ArbitraryAxisRotationMatrix
//
//  Constructs an arbitrary axis rotation matrix. Resulting matrix will
//	rotate a point fAngle radians around vAxis.

float3x3 ArbitraryAxisRotationMatrix(float3 vAxis, float fAngle)
{
	// compute sin/cos of fAngle
	float2 vSinCos;
	#if (ST_OPENGL)
		vSinCos.x = sin(fAngle);
		vSinCos.y = cos(fAngle);
	#else
		sincos(fAngle, vSinCos.x, vSinCos.y);
	#endif

	#define c_var vSinCos.y
	#define s_var vSinCos.x
	#define x_var vAxis.x
	#define y_var vAxis.y
	#define z_var vAxis.z
	float t_var = 1.0 - c_var;

	return float3x3(t_var * x_var * x_var + c_var,			t_var * x_var * y_var - s_var * z_var,	t_var * x_var * z_var + s_var * y_var,
					t_var * x_var * y_var + s_var * z_var,	t_var * y_var * y_var + c_var,			t_var * y_var * z_var - s_var * x_var,
					t_var * x_var * z_var - s_var * y_var,	t_var * y_var * z_var + s_var * x_var,	t_var * z_var * z_var + c_var);

	#undef c_var
	#undef s_var
	#undef x_var
	#undef y_var
	#undef z_var
}


///////////////////////////////////////////////////////////////////////
//  LimitToCameraPlane
//
//	Used in conjunction with billboard wind -- it confines the wind motion
//	to the plane defined by the screen

float3 LimitToCameraPlane(float3 vDir)
{
	const float3 c_vCameraRight = ST_OPENGL ? m_mCameraFacingMatrix[1].xyz : float3(m_mCameraFacingMatrix[0].y, m_mCameraFacingMatrix[1].y, m_mCameraFacingMatrix[2].y);
	const float3 c_vCameraUp = ST_OPENGL ? m_mCameraFacingMatrix[2].xyz : float3(m_mCameraFacingMatrix[0].z, m_mCameraFacingMatrix[1].z, m_mCameraFacingMatrix[2].z);

	return dot(vDir, c_vCameraRight) * c_vCameraRight + dot(vDir, c_vCameraUp) * c_vCameraUp;
}


///////////////////////////////////////////////////////////////////////
//  ProjectToLightSpace
//
//	Used to project SpeedTree geometry into a shadow map.

float4 ProjectToLightSpace(float3 vPos, float4x4 mLightViewProj)
{
	float4 vProjection = mul_float4x4_float4(mLightViewProj, float4(vPos, 1.0));

	if (ST_DIRECTX9 || ST_DIRECTX11)
	{
		vProjection.xy = 0.5 * vProjection.xy / vProjection.w + float2(0.5, 0.5) + m_sShadows.m_vShadowMapTexelOffset;
		vProjection.y = 1.0 - vProjection.y;
	}
	else
	{
		vProjection.xyz += vProjection.www;
		vProjection.xy += m_sShadows.m_vShadowMapTexelOffset;
		vProjection.xyz *= 0.5;

		if (ST_PS4)
			vProjection.y = 1.0 - vProjection.y;
	}

	return vProjection;
}


///////////////////////////////////////////////////////////////////////
//  GenerateHueVariationByPos
//
//	Determines a unique [0, 1] float3 based on attributes unique to an instance;
//	in this case we're using a combination of the instance's 3D position
//	and orientation vector.

float3 GenerateHueVariationByPos(float fVariationScalar, float3 vInstanceRightVector)
{
	return fVariationScalar * fmod(vInstanceRightVector, float3(1.0, 1.0, 1.0)) * m_vHueVariationColor;
}


///////////////////////////////////////////////////////////////////////
//  GenerateHueVariationByVertex
//
//	Determine a unique [0, 1] float3 value based on attributes unique to a vertex
//	that *do not* change with LOD. We're using the normal in combination
//	with the instance's orientation vector.
//
//	This function can be modified as needed, but care should be take not to
//	use vertex attributes that might change as LOD changes, else the hue
//	variation will change along with it.

float3 GenerateHueVariationByVertex(float fVariationScalar, float3 vNormal)
{
	return fVariationScalar * sin(vNormal * 20.0f) * m_vHueVariationColor.rgb;
}


///////////////////////////////////////////////////////////////////////
//  FadeShadowByDepth
//
//	In the SpeedTree example shadow system, the shadow fades out after a
//	certain distance away from the camera. Given a shadow map look up value,
//	this function will push it to 1.0 (white / no shadow) based on a couple
//	of parameters. Function paramters:
//
//		fShadowMapValue: the raw value from the shadowmap look up (0.0 is
//						 fully shadowed, 1.0 is fully lit)
//
//		fDepthAtShadowedPoint: distance from the camera to the pixel
//
//		fStartingFadeDepth: [0.0, 1.0] value designating where the shadow
//							should begin to fade. 0.0 is at the camera pos,
//							1.0 is m_fFarClip.x
//
//		fFadeFactor: value computed on upload to save shader instructions; it's
//					 1.0f / (end_of_last_cascade - fStartingFadeDepth), where
//					 end_of_last_cascade is in same units as fStartingFadeDepth

float FadeShadowByDepth(float fShadowMapValue, float fDepthAtShadowedPoint, float fStartingFadeDepth, float fFadeFactor)
{
	float fFade = saturate((fDepthAtShadowedPoint - fStartingFadeDepth) * fFadeFactor);

	return lerp(fShadowMapValue, 1.0, fFade);
}


///////////////////////////////////////////////////////////////////////
//  Helper function: BillboardSelectMapFromAtlas
//
//	Billboard instances are rendered in batches of the same base tree (e.g. all
//	palm trees are rendered in one call), but they may all have arbitrary
//	orientations. Each base tree also probably has several billboard renderings
//	of the tree from different angles (360-degree billboards), all in the same
//	billboard atlas that's current bound. This function will figure out which
//	atlas entry is the correct one.
//
//	Given a camera, an instance orientation (right, out, up), and the number of
//	360-degree billboard images uploaded for the current base tree, this function
//	will determine the correct texcoords per vertex from the billboard atlas.
//
//	The coordinate system figures heavily into the computation, so you'll see use
//	of ST_COORDSYS_RIGHT_HANDED, ST_COORDSYS_Z_UP, etc.

float2 BillboardSelectMapFromAtlas(float2 vUnitTexCoords,		// texcoords of cutout billboard, defined in Compiler, in [0.0,1.0] range;
																// will be converted to correct smaller texcoords to access sub-image in atlas
								   float3 vRightVector,			// right orientation of current instance
								   float3 vOutVector,			// out orientation of current instance
								   float3 vUpVector,			// up orientation of current instance
								   float3 vCameraDirection)		// which way the main camera is facing
{
	// algorithmic depends on opposite direction
	float3 vDir = -vCameraDirection.xyz;

	// given a particular coordinate system some adjustments are necessary for algorithm that expects right-handed/Z-up
	if (ST_COORDSYS_LEFT_HANDED)
	{
		if (ST_COORDSYS_Y_UP)
			vDir = float3(-vDir.x, vDir.y, -vDir.z);
		else
			vRightVector = float3(-vRightVector.x, -vRightVector.y, vRightVector.z);
	}

	// project camera direction onto plane that holds vRightVector and vOutVector vectors
	float fUpDot = dot(vDir, vUpVector);
	float3 vProjectedCamera = normalize(vDir - fUpDot * vUpVector);

	// determine which angle the projection lands on; corresponds to the correct bb sub-image
	float fRightDot = dot(vRightVector, vProjectedCamera);
	if (ST_PS3)
		fRightDot = clamp(fRightDot, -1.0, 1.0);
	float fRightAngle = acos(fRightDot);

	// need a value in [0,2pi] range, not [-pi,pi]
	float fOutDot = dot(vOutVector, vProjectedCamera);
	if (fOutDot < 0.0)
		fRightAngle = ST_TWO_PI - fRightAngle;

	// bump angle to be in the middle of the pie slice
	fRightAngle += ST_PI / m_fNumBillboards;
	fRightAngle = fmod(fRightAngle, ST_TWO_PI);

	// convert angle to billboard image index
	int nImageIndex	= int(fRightAngle / m_fRadiansPerImage);

	// m_avBillboardTexCoords holds the texcoords for each 360-degree image for a given base tree; each single
	// entry xyzw defines a sub-rectangle in the billboard atlas and does it using four values:
	//
	//		(left u texcoord, bottom v texcoord, width of u span, height of v span)
	//
	// for example, if there are four evenly-spaced billboard images in a given atlas and no other textures
	// reside there, the four entries in m_avBillboardTexCoords might be:
	//
	//		m_avBillboardTexCoords[0] = (0.0, 0.5, 0.5, 0.5) // top left bb image
	//		m_avBillboardTexCoords[1] = (0.5, 0.5, 0.5, 0.5) // top right bb image
	//		m_avBillboardTexCoords[1] = (0.0, 1.0, 0.5, 0.5) // bottom left bb image
	//		m_avBillboardTexCoords[1] = (0.5, 1.0, 0.5, 0.5) // bottom right bb image
	//
	// note that because billboard images may be oriented vertically or horizontally, the .x coordinate in
	// each entry may be negative, indicating a horizontal orientation; the example above assumes all
	// vertical orientations

	// nImage index holds the correct offset into this table
	#ifndef g_avBillboardTexCoords
		float4 vTableEntry = m_avBillboardTexCoords[nImageIndex];
	#else
		// in opengl, g_avBillboardTexCoords is defined a bit differently
		float4 vTableEntry = A_avSingleUniformBlock[BILLBOARD_TEXCOORDS_LOCATION + nImageIndex];
	#endif

	// some further adjustment is still needed for alternate coordinate systems
	if (ST_COORDSYS_RIGHT_HANDED && ST_COORDSYS_Y_UP)
		vUnitTexCoords.x = 1.0 - vUnitTexCoords.x;
	else if (ST_COORDSYS_LEFT_HANDED && ST_COORDSYS_Z_UP)
		vUnitTexCoords.x = 1.0 - vUnitTexCoords.x;

	// if .x coordinate is negative, the billboard is oriented horizontally; this code also applies to the unit
	// texcoords to the sub-image of the billboard atlas
	float2 vAtlasTexCoords = (vTableEntry.x < 0.0) ? float2(-vTableEntry.x + vTableEntry.z * vUnitTexCoords.y,
															vTableEntry.y + vTableEntry.w * vUnitTexCoords.x) :
													 float2(vTableEntry.x + vTableEntry.z * vUnitTexCoords.x,
															vTableEntry.y + vTableEntry.w * vUnitTexCoords.y);

	return vAtlasTexCoords;
}


///////////////////////////////////////////////////////////////////////
//  RgbToLuminance
//
//	Centralized function so that users can choose between an accurate conversion
//	or quick and dirty.

float RgbToLuminance(float3 vRGB)
{
	// decent conversion
	//return dot(vRGB, float3(0.299, 0.587, 0.114)); // CCIR 601 luminance coeffs

	// very quick and dirty
	return vRGB.g;
}

#endif // ST_INCLUDE_UTILITY
