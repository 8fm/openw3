/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#if 0

/************************************************************************/
/* TerrainModPreviewShape implementation                                */
/************************************************************************/

TerrainModPreviewShape::TerrainModPreviewShape ()
{
	Reset();
}

void TerrainModPreviewShape::Reset()
{
	Set( Vector::ZEROS, 0, 1, 1, 0, 0, 0 );
}

TerrainModPreviewShape& TerrainModPreviewShape::Set( const Vector &center, Float bias, Float width, Float height, Float scale, Float modBlendSmoothness, Float rotationRightDeg )
{
	m_center				= center;
	m_bias					= bias;
	m_width					= width;
	m_height				= height;
	m_scale					= scale;
	m_modBlendSmoothness	= modBlendSmoothness;
	m_rotationRightDeg		= rotationRightDeg;
	return *this;
}

/************************************************************************/
/* TerrainModPreviewParams implementation                               */
/************************************************************************/

TerrainModPreviewParams::TerrainModPreviewParams ()
{
	Reset();
}

void TerrainModPreviewParams::Reset()
{
	SetModeNone();
}

TerrainModPreviewParams& TerrainModPreviewParams::SetModeNone()
{
	m_mode						= TMPM_None;
	m_mixedDestWeight			= 1;
	m_mixedScaledSourceWeight	= 0;
	m_mixedLerpWeight			= 0;
	m_mixedBiasWeight			= 0;
	return *this;
}

TerrainModPreviewParams& TerrainModPreviewParams::SetModeMixed(Float mixedDestWeight, Float mixedScaledSourceWeight, Float mixedLerpWeight, Float mixedBiasWeight)
{
	m_mode						= TMPM_Mixed;
	m_mixedDestWeight			= mixedDestWeight;
	m_mixedScaledSourceWeight	= mixedScaledSourceWeight;
	m_mixedLerpWeight			= mixedLerpWeight;
	m_mixedBiasWeight			= mixedBiasWeight;
	return *this;
}

/************************************************************************/
/* TerrainModPreviewShaderParams implementation                         */
/************************************************************************/

TerrainModPreviewShaderParams::TerrainModPreviewShaderParams ()
{
	Reset();
}

TerrainModPreviewShaderParams::TerrainModPreviewShaderParams ( ENoInit )
{
	// empty
}

void TerrainModPreviewShaderParams::Reset()
{
	Set( 1, 1, TerrainModPreviewShape (), TerrainModPreviewParams () );
}

TerrainModPreviewShaderParams& TerrainModPreviewShaderParams::Set( Int32 texturesWidth, Int32 texturesHeight, const TerrainModPreviewShape &previewShape, const TerrainModPreviewParams &previewParams )
{
	modXYBiasMode.Set4(
		previewShape.m_center.X,
		previewShape.m_center.Y,			
		previewShape.m_bias,			
		(Float) previewParams.m_mode );
	modWidthHeightScaleBlend.Set4(
		previewShape.m_width, 
		previewShape.m_height, 
		previewShape.m_scale,
		previewShape.m_modBlendSmoothness );
	modRotDirModTexSize.Set4(
		sinf( DEG2RAD( -previewShape.m_rotationRightDeg ) ),
		cosf( DEG2RAD( -previewShape.m_rotationRightDeg ) ),
		(Float) Max( 1, texturesWidth ),
		(Float) Max( 1, texturesHeight ) );
	modParams.Set4(
		previewParams.m_mixedDestWeight, 
		previewParams.m_mixedScaledSourceWeight, 
		previewParams.m_mixedLerpWeight,
		previewParams.m_mixedBiasWeight );
	return *this;
}

#endif