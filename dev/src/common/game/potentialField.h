/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once


///////////////////////////////////////////////////////////////////////////////

class IPotentialFieldGenerator
{
public:
	virtual ~IPotentialFieldGenerator() {}

	virtual Float GetPotentialValue( const Vector& pos, Float strengthMultiplier ) const = 0;
};

///////////////////////////////////////////////////////////////////////////////

class IPotentialField : public CObject, public IPotentialFieldGenerator
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IPotentialField, CObject );

public:
	virtual ~IPotentialField() {}

	//! Range test (pass local position)
	virtual Bool RangeTest( const Vector& pos, Float strengthMultiplier ) const { return false; }

	virtual Float GetPotentialValue( const Vector& pos, Float strengthMultiplier ) const { return 0.0f; }
};
BEGIN_ABSTRACT_CLASS_RTTI( IPotentialField );
	PARENT_CLASS( CObject );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////

class CCircularPotentialField : public IPotentialField
{
	DECLARE_ENGINE_CLASS( CCircularPotentialField, IPotentialField, 0 );

private:
	Float			m_radius;
	Float			m_rangeTop;
	Float			m_rangeBottom;
	Vector			m_origin;
	Bool			m_solid;

public:
	CCircularPotentialField();

	void SetRadius( Float radius ) { m_radius = radius; }

	void SetVerticalRanges( Float rangeTop, Float rangeBottom ) { m_rangeTop = rangeTop; m_rangeBottom = rangeBottom; }

	RED_INLINE void SetOrigin( const Vector& origin ) { m_origin = origin; }

	//! Range test (pass local position)
	virtual Bool RangeTest( const Vector& pos, Float strengthMultiplier ) const;

	virtual Float GetPotentialValue( const Vector& pos, Float strengthMultiplier ) const;

	virtual void OnGenerateDebugFragments( CRenderFrame* frame, const Matrix& localToWorld );
};
BEGIN_CLASS_RTTI( CCircularPotentialField );
	PARENT_CLASS( IPotentialField );
	PROPERTY_EDIT( m_radius, TXT( "radius" ) );
	PROPERTY_EDIT( m_rangeTop, TXT( "top range" ) );
	PROPERTY_EDIT( m_rangeBottom, TXT( "bottom range" ) );
	PROPERTY_EDIT( m_origin, TXT( "origin" ) );
	PROPERTY_EDIT( m_solid, TXT( "solid/gradial" ) );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////

class CSoundPotentialField : public IPotentialField
{
	DECLARE_ENGINE_CLASS( CSoundPotentialField, IPotentialField, 0 );

private:
	Float			m_radius;


public:
	CSoundPotentialField();

	//! Range test (pass local position)
	virtual Bool RangeTest( const Vector& pos, Float parameter ) const;

	virtual Float GetPotentialValue( const Vector& pos, Float parameter ) const;

	Float CalcFieldStrength( Float strength, Float distance ) const;
};
BEGIN_CLASS_RTTI( CSoundPotentialField );
PARENT_CLASS( IPotentialField );
PROPERTY_EDIT( m_radius, TXT( "radius" ) );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////
