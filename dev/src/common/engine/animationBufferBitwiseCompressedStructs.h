/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#pragma once

///////////////////////////////////////////////////////////////////////////////

enum SAnimationBufferBitwiseCompression
{
	ABBC_None,
	ABBC_24b,
	ABBC_16b
};

BEGIN_ENUM_RTTI( SAnimationBufferBitwiseCompression );
	ENUM_OPTION( ABBC_None );
	ENUM_OPTION( ABBC_24b );
	ENUM_OPTION( ABBC_16b );
END_ENUM_RTTI();

///////////////////////////////////////////////////////////////////////////////

enum SAnimationBufferDataCompressionMethod
{
	ABDCM_Invalid,
	ABDCM_Plain,
	ABDCM_Quaternion, // useful for quaternions
	ABDCM_QuaternionXYZSignedW, // useful for quaternions, to store last value as sign in last bit
	ABDCM_QuaternionXYZSignedWLastBit, // useful for quaternions, to store last value as sign in last bit
	ABDCM_Quaternion48b, // X:16b Y:16b Z:15b W:1b
	ABDCM_Quaternion40b, // X:13b Y:13b Z:13b W:1b
	ABDCM_Quaternion32b, // X:10b Y:10b Z:11b W:1b
	ABDCM_Quaternion64bW, // X:16b Y:16b Z:16b W:16b
	ABDCM_Quaternion48bW, // X:12b Y:12b Z:12b W:12b
	ABDCM_Quaternion40bW, // X:10b Y:10b Z:10b W:10b
};

BEGIN_ENUM_RTTI( SAnimationBufferDataCompressionMethod );
	ENUM_OPTION( ABDCM_Invalid );
	ENUM_OPTION( ABDCM_Plain );
	ENUM_OPTION( ABDCM_Quaternion );
	ENUM_OPTION( ABDCM_QuaternionXYZSignedW );
	ENUM_OPTION( ABDCM_QuaternionXYZSignedWLastBit );
	ENUM_OPTION( ABDCM_Quaternion48b );
	ENUM_OPTION( ABDCM_Quaternion40b );
	ENUM_OPTION( ABDCM_Quaternion32b );
	ENUM_OPTION( ABDCM_Quaternion64bW );
	ENUM_OPTION( ABDCM_Quaternion48bW );
	ENUM_OPTION( ABDCM_Quaternion40bW );
END_ENUM_RTTI();

///////////////////////////////////////////////////////////////////////////////

enum SAnimationBufferOrientationCompressionMethod
{
	ABOCM_PackIn64bitsW,
	ABOCM_PackIn48bitsW,
	ABOCM_PackIn40bitsW,
	ABOCM_AsFloat_XYZW,
	ABOCM_AsFloat_XYZSignedW,
	ABOCM_AsFloat_XYZSignedWInLastBit,
	ABOCM_PackIn48bits,
	ABOCM_PackIn40bits,
	ABOCM_PackIn32bits,
};

BEGIN_ENUM_RTTI( SAnimationBufferOrientationCompressionMethod );
	ENUM_OPTION( ABOCM_PackIn64bitsW );
	ENUM_OPTION( ABOCM_PackIn48bitsW );
	ENUM_OPTION( ABOCM_PackIn40bitsW );
	ENUM_OPTION( ABOCM_AsFloat_XYZW );
	ENUM_OPTION( ABOCM_AsFloat_XYZSignedW );
	ENUM_OPTION( ABOCM_AsFloat_XYZSignedWInLastBit );
	ENUM_OPTION( ABOCM_PackIn48bits );
	ENUM_OPTION( ABOCM_PackIn40bits );
	ENUM_OPTION( ABOCM_PackIn32bits );
END_ENUM_RTTI();

///////////////////////////////////////////////////////////////////////////////

enum SAnimationBufferBitwiseCompressionPreset
{
	ABBCP_Custom,
	ABBCP_VeryHighQuality,
	ABBCP_HighQuality,
	ABBCP_NormalQuality,
	ABBCP_LowQuality,
	ABBCP_VeryLowQuality,
	ABBCP_Raw,
};

BEGIN_ENUM_RTTI( SAnimationBufferBitwiseCompressionPreset );
	ENUM_OPTION( ABBCP_Custom );
	ENUM_OPTION( ABBCP_VeryHighQuality );
	ENUM_OPTION( ABBCP_HighQuality );
	ENUM_OPTION( ABBCP_NormalQuality ); // default
	ENUM_OPTION( ABBCP_LowQuality );
	ENUM_OPTION( ABBCP_VeryLowQuality );
	ENUM_OPTION( ABBCP_Raw );
END_ENUM_RTTI();

///////////////////////////////////////////////////////////////////////////////

struct SAnimationBufferBitwiseCompressionSettings
{
	DECLARE_RTTI_STRUCT( SAnimationBufferBitwiseCompressionSettings );

	Float m_translationTolerance;
	Float m_translationSkipFrameTolerance;
	Float m_orientationTolerance;
	SAnimationBufferOrientationCompressionMethod m_orientationCompressionMethod;
	Float m_orientationSkipFrameTolerance;
	Float m_scaleTolerance;
	Float m_scaleSkipFrameTolerance;
	Float m_trackTolerance;
	Float m_trackSkipFrameTolerance;

	SAnimationBufferBitwiseCompressionSettings();

	void UsePreset( SAnimationBufferBitwiseCompressionPreset preset );
};

BEGIN_CLASS_RTTI( SAnimationBufferBitwiseCompressionSettings );
	PROPERTY_EDIT( m_translationTolerance, TXT("") )
	PROPERTY_EDIT( m_translationSkipFrameTolerance, TXT("") )
	PROPERTY_EDIT( m_orientationTolerance, TXT("") )
	PROPERTY_EDIT( m_orientationCompressionMethod, TXT("") )
	PROPERTY_EDIT( m_orientationSkipFrameTolerance, TXT("") )
	PROPERTY_EDIT( m_scaleTolerance, TXT("") )
	PROPERTY_EDIT( m_scaleSkipFrameTolerance, TXT("") )
	PROPERTY_EDIT( m_trackTolerance, TXT("") )
	PROPERTY_EDIT( m_trackSkipFrameTolerance, TXT("") )
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////

