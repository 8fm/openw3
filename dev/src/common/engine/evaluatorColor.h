/**
* Copyright © 2008 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "curve.h"
#include "evaluator.h"

/// A single color evaluator
class IEvaluatorColor : public IEvaluator
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IEvaluatorColor, IEvaluator );

public:
	//! Calculate distribution value
	virtual void Evaluate( Float x, Vector& result ) const=0;

	virtual Bool GetApproximationSamples( TDynArray< Vector >& samples ) const { return true; };
};

BEGIN_ABSTRACT_CLASS_RTTI( IEvaluatorColor );
PARENT_CLASS( IEvaluator );
END_CLASS_RTTI();

/// A constant color
class CEvaluatorColorConst : public IEvaluatorColor
{
	DECLARE_ENGINE_CLASS( CEvaluatorColorConst, IEvaluatorColor, 0 );

protected:
	Color	m_value;		//!< Output value

public:
	RED_INLINE CEvaluatorColorConst() {};

	RED_INLINE CEvaluatorColorConst( CObject* parent, const Color& value )
		: m_value( value )
	{
		SetParent( parent );
	}

public:
	virtual void Evaluate( Float x, Vector& result ) const;

	virtual Bool GetApproximationSamples( TDynArray< Vector >& samples ) const;
};

BEGIN_CLASS_RTTI( CEvaluatorColorConst );
PARENT_CLASS( IEvaluatorColor );
PROPERTY_EDIT( m_value, TXT("Value") );
END_CLASS_RTTI();

/// A color value interpolated from start to end
class CEvaluatorColorStartEnd : public IEvaluatorColor
{
	DECLARE_ENGINE_CLASS( CEvaluatorColorStartEnd, IEvaluatorColor, 0 );

protected:
	Color	m_start;	//!< Starting color
	Color	m_end;		//!< Final color

public:
	virtual Bool IsFunction() const { return true; }
	virtual void Evaluate( Float x, Vector& result ) const;

	virtual Bool GetApproximationSamples( TDynArray< Vector >& samples ) const;
};

BEGIN_CLASS_RTTI( CEvaluatorColorStartEnd );
PARENT_CLASS( IEvaluatorColor );
PROPERTY_EDIT( m_start, TXT("Initial value") ); 
PROPERTY_EDIT( m_end, TXT("Final value") ); 
END_CLASS_RTTI();

/// A color value driven from a curve
class CEvaluatorColorCurve : public IEvaluatorColor
{
	DECLARE_ENGINE_CLASS( CEvaluatorColorCurve, IEvaluatorColor, 0 );

protected:

	CurveParameter	m_curves;		//!< The curves

public:

	CEvaluatorColorCurve();
	virtual CurveParameter* GetCurves() { return &m_curves; }
	virtual Bool IsFunction() const { return true; }
	virtual void Evaluate( Float x, Vector& result ) const;
	virtual void OnSerialize( IFile& file );

	virtual Bool GetApproximationSamples( TDynArray< Vector >& samples ) const;
};

BEGIN_CLASS_RTTI( CEvaluatorColorCurve );
PARENT_CLASS( IEvaluatorColor );
END_CLASS_RTTI();

/// A random color
class CEvaluatorColorRandom : public IEvaluatorColor
{
	DECLARE_ENGINE_CLASS( CEvaluatorColorRandom, IEvaluatorColor, 0 );

protected:
	Color	m_value;		//!< Output value

public:
	CEvaluatorColorRandom();
	CEvaluatorColorRandom( CObject* parent, const Color& value );

public:
	virtual void Evaluate( Float x, Vector& result ) const;
	virtual Bool GetApproximationSamples( TDynArray< Vector >& samples ) const;
};

BEGIN_CLASS_RTTI( CEvaluatorColorRandom );
PARENT_CLASS( IEvaluatorColor );
PROPERTY_EDIT( m_value, TXT("Value") );
END_CLASS_RTTI();

/// Evaluate color evaluator, with default value
RED_INLINE Vector EvaluateColor( IEvaluatorColor* evaluator, Float x = 0.0f, const Vector& defaultValue = Vector::ZEROS )
{
	Vector result = defaultValue;
	if ( evaluator )
	{
		evaluator->Evaluate( x, result );
	}
	return result;
}