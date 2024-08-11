/**
* Copyright © 2008 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "curve.h"
#include "evaluator.h"

/// A single floating point value evaluator
class IEvaluatorFloat : public IEvaluator
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IEvaluatorFloat, IEvaluator );

public:
	//! Calculate distribution value
	virtual void Evaluate( Float x, Float& result ) const=0;

	virtual Bool GetApproximationSamples( TDynArray< Float >& samples ) const { samples.Clear(); samples.PushBack( 0.1f ); samples.PushBack( 0.1f ); return true; };
};

BEGIN_ABSTRACT_CLASS_RTTI( IEvaluatorFloat );
PARENT_CLASS( IEvaluator );
END_CLASS_RTTI();

/// A constant floating point evaluator
class CEvaluatorFloatConst : public IEvaluatorFloat
{
	DECLARE_ENGINE_CLASS( CEvaluatorFloatConst, IEvaluatorFloat, 0 );

public:
	Float	m_value;		//!< Output value

public:
	RED_INLINE CEvaluatorFloatConst() {};

	RED_INLINE CEvaluatorFloatConst( CObject* parent, Float value )
		: m_value( value )
	{
		SetParent( parent );
	}

public:
	virtual void Evaluate( Float x, Float& result ) const;

	virtual Bool GetApproximationSamples( TDynArray< Float >& samples ) const;

	Float GetValue() const { return m_value; }
};

BEGIN_CLASS_RTTI( CEvaluatorFloatConst );
PARENT_CLASS( IEvaluatorFloat );
PROPERTY_EDIT( m_value, TXT("Constant float value") ); 
END_CLASS_RTTI();

/// A random floating point value from given range
class CEvaluatorFloatRandomUniform : public IEvaluatorFloat
{
	DECLARE_ENGINE_CLASS( CEvaluatorFloatRandomUniform, IEvaluatorFloat, 0 );

public:
	Float	m_min;		//!< Range start
	Float	m_max;		//!< Range end

public:
	virtual void Evaluate( Float x, Float& result ) const;

	virtual Bool GetApproximationSamples( TDynArray< Float >& samples ) const;

	Float GetMinValue() const { return m_min; }
	Float GetMaxValue() const { return m_max; }

private:
	virtual void OnPropertyPostChange( IProperty* property );
	virtual void OnPostLoad();
	const CResource*	 GetResource() const;
};

BEGIN_CLASS_RTTI( CEvaluatorFloatRandomUniform );
PARENT_CLASS( IEvaluatorFloat );
PROPERTY_EDIT( m_min, TXT("Lower bound of the random range") ); 
PROPERTY_EDIT( m_max, TXT("Upper bound of the random range") ); 
END_CLASS_RTTI();

/// A floating point value interpolated from start to end
class CEvaluatorFloatStartEnd : public IEvaluatorFloat
{
	DECLARE_ENGINE_CLASS( CEvaluatorFloatStartEnd, IEvaluatorFloat, 0 );

public:
	Float	m_start;	//!< Starting value
	Float	m_end;		//!< Final value

public:
	virtual Bool IsFunction() const { return true; }
	virtual void Evaluate( Float x, Float& result ) const;

	virtual Bool GetApproximationSamples( TDynArray< Float >& samples ) const;

	Float GetStartValue() const { return m_start; }
	Float GetEndValue() const { return m_end; }
};

BEGIN_CLASS_RTTI( CEvaluatorFloatStartEnd );
PARENT_CLASS( IEvaluatorFloat );
PROPERTY_EDIT( m_start, TXT("Initial value") ); 
PROPERTY_EDIT( m_end, TXT("Final value") ); 
END_CLASS_RTTI();

/// A floating point value driven from a curve
class CEvaluatorFloatCurve : public IEvaluatorFloat
{
	DECLARE_ENGINE_CLASS( CEvaluatorFloatCurve, IEvaluatorFloat, 0 );

public:

	CurveParameter m_curves;		//!< The curve

public:
	CEvaluatorFloatCurve();
	virtual CurveParameter* GetCurves() { return &m_curves; }
	virtual Bool IsFunction() const { return true; }
	virtual void Evaluate( Float x, Float& result ) const;
	virtual void OnSerialize( IFile& file );

	virtual Bool GetApproximationSamples( TDynArray< Float >& samples ) const;
};

BEGIN_CLASS_RTTI( CEvaluatorFloatCurve );
PARENT_CLASS( IEvaluatorFloat );
END_CLASS_RTTI();


/// A constant floating point evaluator
class CEvaluatorFloatRainStrength : public IEvaluatorFloat
{
	DECLARE_ENGINE_CLASS( CEvaluatorFloatRainStrength, IEvaluatorFloat, 0 );

protected:
	Float	m_valueMultiplier;		//!< Output value

public:
	RED_INLINE CEvaluatorFloatRainStrength() {};

	RED_INLINE CEvaluatorFloatRainStrength( CObject* parent, Float value )
		: m_valueMultiplier( value )
	{
		SetParent( parent );
	}

public:
	virtual void Evaluate( Float x, Float& result ) const;
};

BEGIN_CLASS_RTTI( CEvaluatorFloatRainStrength );
PARENT_CLASS( IEvaluatorFloat );
PROPERTY_EDIT( m_valueMultiplier, TXT("Rain strength value multiplier") ); 
END_CLASS_RTTI();


/// Evaluate float evaluator, with default value
RED_INLINE Float EvaluateFloat( IEvaluatorFloat* evaluator, Float x = 0.0f, Float defaultValue = 0.0f )
{
	Float result = defaultValue;
	if ( evaluator )
	{
		evaluator->Evaluate( x, result );
	}
	return result;
}
