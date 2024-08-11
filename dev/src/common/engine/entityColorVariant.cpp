/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "entityColorVariant.h"

IMPLEMENT_ENGINE_CLASS( CColorShift )

void CColorShift::CalculateColorShiftMatrix( Matrix& coloringMatrix ) const
{
	Float normalizedHue = m_hue * M_PI / 180.0f;
	Float normalizedSaturation = ( m_saturation + 100.0f ) / 100.0f;
	Float normalizedLuminance = ( m_luminance + 100.0f ) / 100.0f;

	Float saturationScale = ( 1.0f - normalizedSaturation ) / 3.0f;

	Matrix preHueMatrix( 
		Vector( 0.6210f, -0.2461f, -0.5774f, 0.0f ),
		Vector( -0.6038f, 0.4610f, -0.5774f, 0.0f ),
		Vector( 0.6038f, 0.9532f, 0.5774f, 0.0f ),
		Vector( 0.0f, 0.0f, 0.0f, 1.0f ) );

	Matrix hueRotationMatrix( 
		Vector( cos( normalizedHue ), -sin( normalizedHue ), 0.0f, 0.0f ),
		Vector( sin( normalizedHue ), cos( normalizedHue ), 0.0f, 0.0f ),
		Vector( 0.0f, 0.0f, 1.0f, 0.0f ),
		Vector( 0.0f, 0.0f, 0.0f, 1.0f ) );

	Matrix postHueMatrix( 
		Vector( 0.8165f, -0.4082f, 0.4082f, 0.0f ),
		Vector( 0.0f, 0.7071f, 0.7071f, 0.0f ),
		Vector( -0.8539f, -0.7405f, 0.1377f, 0.0f ),
		Vector( 0.0f, 0.0f, 0.0f, 1.0f ) );

	Matrix saturationMatrix(
		Vector( saturationScale + normalizedSaturation, saturationScale, saturationScale, 0.0f ),
		Vector( saturationScale, saturationScale + normalizedSaturation, saturationScale, 0.0f ),
		Vector( saturationScale, saturationScale, saturationScale + normalizedSaturation, 0.0f ),
		Vector( 0.0f, 0.0f, 0.0f, 1.0f ) );

	Matrix luminanceMatrix(
		Vector( normalizedLuminance, 0.0f, 0.0f, 0.0f ), 
		Vector( 0.0f, normalizedLuminance, 0.0f, 0.0f ),
		Vector( 0.0f, 0.0f, normalizedLuminance, 0.0f ),
		Vector( 0.0f, 0.0f, 0.0f, 1.0f ) );

	coloringMatrix = luminanceMatrix * postHueMatrix * hueRotationMatrix * preHueMatrix * saturationMatrix;
	//coloringMatrix = preHueMatrix * hueRotationMatrix * postHueMatrix * saturationMatrix * luminanceMatrix;

	//Float v = normalizedLuminance;
	//Float vsx = v * normalizedSaturation * cos( normalizedHue );
	//Float vsy = v * normalizedSaturation * sin( normalizedHue );

	//Matrix hsvMatrix(
	//	Vector( 0.299f * v + 0.701f * vsx + 0.168f * vsy,	0.587f * v - 0.587f * vsx + 0.330f * vsy,	0.114f * v - 0.114f * vsx - 0.497f * vsy,	0.0f ),
	//	Vector( 0.299f * v - 0.299f * vsx - 0.328f * vsy,	0.587f * v + 0.423f * vsx + 0.035f * vsy,	0.114f * v - 0.114f * vsx + 0.292f * vsy,	0.0f ),
	//	Vector( 0.299f * v - 0.300f * vsx + 0.125f * vsy,	0.587f * v - 0.588f * vsx - 1.050f * vsy,	0.114f * v + 0.886f * vsx - 0.203f * vsy,	0.0f ),
	//	Vector( 0.0f, 0.0f, 0.0f, 1.0f )
	//);

	//coloringMatrix = hsvMatrix;
}
