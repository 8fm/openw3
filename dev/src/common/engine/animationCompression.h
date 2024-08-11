/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "behaviorIncludes.h"

// animation compression
class IAnimationCompression : public CObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IAnimationCompression, CObject );

public:
	IAnimationCompression() {}

public:
	virtual void SetCompressionFactor( Float factor ) {}
#ifdef USE_HAVOK_ANIMATION
	virtual hkaAnimation* Compress( const hkaAnimation *animToCompress ) const = 0;
#endif
};

BEGIN_ABSTRACT_CLASS_RTTI( IAnimationCompression );
	PARENT_CLASS( CObject );
END_CLASS_RTTI();


class CNoAnimationCompression : public IAnimationCompression
{
	DECLARE_ENGINE_CLASS( CNoAnimationCompression, IAnimationCompression, 0 );

public:
#ifdef USE_HAVOK_ANIMATION
	virtual hkaAnimation* Compress( const hkaAnimation *animToCompress ) const;
#endif
};

BEGIN_CLASS_RTTI( CNoAnimationCompression );
	PARENT_CLASS( IAnimationCompression );
END_CLASS_RTTI();


class CWaveletAnimationCompression : public IAnimationCompression
{
	DECLARE_ENGINE_CLASS( CWaveletAnimationCompression, IAnimationCompression, 0 );

	Uint8						m_quantizationBits;
	Float						m_positionTolerance;
	Float						m_rotationTolerance;
	Float						m_scaleTolerance;

public:
	CWaveletAnimationCompression();
#ifdef USE_HAVOK_ANIMATION
	virtual hkaAnimation* Compress( const hkaAnimation *animToCompress ) const;
#endif
};

BEGIN_CLASS_RTTI( CWaveletAnimationCompression );
	PARENT_CLASS( IAnimationCompression );
	PROPERTY_EDIT( m_quantizationBits, TXT("Bits used for quantization") );
	PROPERTY_EDIT( m_positionTolerance, TXT("Position tolerance") );
	PROPERTY_EDIT( m_rotationTolerance, TXT("Rotation tolerance") );
	PROPERTY_EDIT( m_scaleTolerance, TXT("Scale tolerance") );
END_CLASS_RTTI();


class CDeltaAnimationCompression : public IAnimationCompression
{
	DECLARE_ENGINE_CLASS( CDeltaAnimationCompression, IAnimationCompression, 0 );

	Uint8						m_quantizationBits;
	Float						m_positionTolerance;
	Float						m_rotationTolerance;
	Float						m_scaleTolerance;

public:
	CDeltaAnimationCompression();
#ifdef USE_HAVOK_ANIMATION
	virtual hkaAnimation* Compress( const hkaAnimation *animToCompress ) const;
#endif
};

BEGIN_CLASS_RTTI( CDeltaAnimationCompression );
	PARENT_CLASS( IAnimationCompression );
	PROPERTY_EDIT( m_quantizationBits, TXT("Bits used for quantization") );
	PROPERTY_EDIT( m_positionTolerance, TXT("Position tolerance") );
	PROPERTY_EDIT( m_rotationTolerance, TXT("Rotation tolerance") );
	PROPERTY_EDIT( m_scaleTolerance, TXT("Scale tolerance") );
END_CLASS_RTTI();


class CSplineAnimationCompression : public IAnimationCompression
{
	DECLARE_ENGINE_CLASS( CSplineAnimationCompression, IAnimationCompression, 0 );

	Float						m_positionTolerance;
	Uint16						m_positionPolynomialDegree;
	Float						m_rotationTolerance;
	Uint16						m_rotationPolynomialDegree;
	Float						m_scaleTolerance;
	Uint16						m_scalePolynomialDegree;
	Float						m_floatTolerance;
	Uint16						m_floatPolynomialDegree;

public:
	CSplineAnimationCompression();

	virtual void SetCompressionFactor( Float factor );
#ifdef USE_HAVOK_ANIMATION
	virtual hkaAnimation* Compress( const hkaAnimation *animToCompress ) const;
#endif
};

BEGIN_CLASS_RTTI( CSplineAnimationCompression );
	PARENT_CLASS( IAnimationCompression );
	PROPERTY_EDIT( m_positionTolerance, TXT("Position tolerance") );
	PROPERTY_EDIT( m_positionPolynomialDegree, TXT("Degree of the polynomial used for position compression") );
	PROPERTY_EDIT( m_rotationTolerance, TXT("Rotation tolerance") );
	PROPERTY_EDIT( m_rotationPolynomialDegree, TXT("Degree of the polynomial used for rotation compression") );
	PROPERTY_EDIT( m_scaleTolerance, TXT("Scale tolerance") );
	PROPERTY_EDIT( m_scalePolynomialDegree, TXT("Degree of the polynomial used for scale compression") );
	PROPERTY_EDIT( m_floatTolerance, TXT("Float tolerance") );
	PROPERTY_EDIT( m_floatPolynomialDegree, TXT("Degree of the polynomial used for float compression") );
END_CLASS_RTTI();

class CAnimationFpsCompression : public IAnimationCompression
{
	DECLARE_ENGINE_CLASS( CAnimationFpsCompression, IAnimationCompression, 0 );

	EAnimationFps	m_fps;

public:
	CAnimationFpsCompression();

	void SetFps( EAnimationFps fps );

	virtual void SetCompressionFactor( Float factor );
#ifdef USE_HAVOK_ANIMATION
	virtual hkaAnimation* Compress( const hkaAnimation *animToCompress ) const;
#endif
};

BEGIN_CLASS_RTTI( CAnimationFpsCompression );
	PARENT_CLASS( IAnimationCompression );
	PROPERTY_EDIT( m_fps, TXT("FPS") );
END_CLASS_RTTI();
