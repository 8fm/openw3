/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

class IMotionExtractionCompression : public CObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IMotionExtractionCompression, CObject );

public:
	virtual IMotionExtraction* Compress( const CUncompressedMotionExtraction *motionToCompress ) const { return NULL; }
};

BEGIN_ABSTRACT_CLASS_RTTI( IMotionExtractionCompression );
	PARENT_CLASS( CObject );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CMotionExtractionLineCompression : public IMotionExtractionCompression
{
	DECLARE_ENGINE_CLASS( CMotionExtractionLineCompression, IMotionExtractionCompression, 0 );

protected:
	Float	m_eps;
	Uint32	m_minKnots;

public:
	CMotionExtractionLineCompression();

	void SetMinKnots( Uint32 knots );
	void SetEps( Float eps );

public:
	virtual IMotionExtraction* Compress( const CUncompressedMotionExtraction *motionToCompress ) const;
};

BEGIN_CLASS_RTTI( CMotionExtractionLineCompression );
	PARENT_CLASS( IMotionExtractionCompression );
	PROPERTY_EDIT( m_eps, TXT("") );
	PROPERTY_EDIT( m_minKnots, TXT("") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CMotionExtractionLineCompression2 : public IMotionExtractionCompression
{
	DECLARE_ENGINE_CLASS( CMotionExtractionLineCompression2, IMotionExtractionCompression, 0 );

protected:
	Float	m_eps;
	Uint32	m_minKnots;
	Int32		m_maxKnotsDistance;

public:
	CMotionExtractionLineCompression2();

	void SetMinKnots( Uint32 knots );
	void SetEps( Float eps );

public:
	virtual IMotionExtraction* Compress( const CUncompressedMotionExtraction *motionToCompress ) const;

private:
	void CalcFlags( const TDynArray< Vector >& frames, Uint8& flags, Uint32& size ) const;
};

BEGIN_CLASS_RTTI( CMotionExtractionLineCompression2 );
	PARENT_CLASS( IMotionExtractionCompression );
	PROPERTY_EDIT( m_eps, TXT("") );
	PROPERTY_EDIT( m_minKnots, TXT("") );
	PROPERTY_EDIT( m_maxKnotsDistance, TXT("") );
END_CLASS_RTTI();
