/**
* Copyright © 2008 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "evaluator.h"
#include "curve.h"
#include "../../common/redMath/random/random.h"

/// Free axes
enum EFreeVectorAxes
{
	FVA_One=1,		//!< One free axis
	FVA_Two=2,		//!< Two free axes
	FVA_Three=3,	//!< Three free axes
	FVA_Four=4,		//!< All four axes are free
};

BEGIN_ENUM_RTTI( EFreeVectorAxes );
ENUM_OPTION( FVA_One );
ENUM_OPTION( FVA_Two );
ENUM_OPTION( FVA_Three );
ENUM_OPTION( FVA_Four );
END_ENUM_RTTI();

/// A Vector value evaluator
class IEvaluatorVector : public IEvaluator
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IEvaluatorVector, IEvaluator );

protected:
	EFreeVectorAxes			m_freeAxes;		//!< Number of vector free axes
	Bool					m_spill;		//!< Spill last free axis value to the limited axes

public:
	IEvaluatorVector();

	//! Evaluate value
	virtual void Evaluate( Float x, Vector& result ) const;

	virtual Bool GetApproximationSamples( TDynArray< Vector >& samples ) const 
	{ 
		Vector vec( 0.1f, 0.1f, 0.1f );

		samples.Clear(); 
		samples.PushBack( vec ); 
		samples.PushBack( vec ); 
		return true; 
	};

	void SpillVector( Vector& v ) const;

	EFreeVectorAxes GetFreeAxes() { return m_freeAxes; }
	Bool IsSpilled() { return m_spill; }

protected:
	//! Calculate distribution value
	virtual void EvaluateInternal( Float x, Vector & result ) const=0;
};

BEGIN_ABSTRACT_CLASS_RTTI( IEvaluatorVector );
PARENT_CLASS( IEvaluator );
PROPERTY_EDIT( m_freeAxes, TXT("How many of the verctor axes are really free") ); 
PROPERTY_EDIT( m_spill, TXT("Spill value of last free axis to the limited ones") ); 
END_CLASS_RTTI();

/// A constant vector evaluator
class CEvaluatorVectorConst : public IEvaluatorVector
{
	DECLARE_ENGINE_CLASS( CEvaluatorVectorConst, IEvaluatorVector, 0 );

protected:
	Vector	m_value;		//!< Output value

public:
	RED_INLINE CEvaluatorVectorConst() {};

	RED_INLINE CEvaluatorVectorConst( CObject* parent, const Vector& value )
		: m_value( value )
	{
		SetParent( parent );
	}

public:
	virtual void EvaluateInternal( Float x, Vector & result ) const;

	virtual Bool GetApproximationSamples( TDynArray< Vector >& samples ) const;

	void SetValue( const Vector& value ) { m_value = value; }

	Vector& GetValue() { return m_value; }
};

BEGIN_CLASS_RTTI( CEvaluatorVectorConst );
PARENT_CLASS( IEvaluatorVector );
PROPERTY_EDIT( m_value, TXT("Value") ); 
END_CLASS_RTTI();


/// A random vector value from given range
class CEvaluatorVectorRandomUniform : public IEvaluatorVector
{
	DECLARE_ENGINE_CLASS( CEvaluatorVectorRandomUniform, IEvaluatorVector, 0 );

protected:
	Vector	m_min;		//!< Range start
	Vector	m_max;		//!< Range end

public:
	virtual void EvaluateInternal( Float x, Vector& result ) const;

	virtual Bool GetApproximationSamples( TDynArray< Vector >& samples ) const;

	void SetMinValue( const Vector& min ) { m_min = min; }
	void SetMaxValue( const Vector& max ) { m_max = max; }

	Vector& GetMinValue() { return m_min; }
	Vector& GetMaxValue() { return m_max; }
};

BEGIN_CLASS_RTTI( CEvaluatorVectorRandomUniform );
PARENT_CLASS( IEvaluatorVector );
PROPERTY_EDIT( m_min, TXT("Lower bound of the random range") ); 
PROPERTY_EDIT( m_max, TXT("Upper bound of the random range") ); 
END_CLASS_RTTI();

/// A vector value interpolated from start to end
class CEvaluatorVectorStartEnd : public IEvaluatorVector
{
	DECLARE_ENGINE_CLASS( CEvaluatorVectorStartEnd, IEvaluatorVector, 0 );

protected:
	Vector	m_start;	//!< Starting value
	Vector	m_end;		//!< Final value

public:
	virtual Bool IsFunction() const { return true; }
	virtual void EvaluateInternal( Float x, Vector& result ) const;

	virtual Bool GetApproximationSamples( TDynArray< Vector >& samples ) const;

	void SetStartValue( const Vector& startValue ) { m_start = startValue; }
	void SetEndValue( const Vector& endValue ) { m_end = endValue; }

	Vector& GetStartValue()  { return m_start;}
	Vector& GetEndValue()  { return m_end;}
};

BEGIN_CLASS_RTTI( CEvaluatorVectorStartEnd );
PARENT_CLASS( IEvaluatorVector );
PROPERTY_EDIT( m_start, TXT("Initial value") ); 
PROPERTY_EDIT( m_end, TXT("Final value") ); 
END_CLASS_RTTI();

/// A vector value driven from a curve
class CEvaluatorVectorCurve : public IEvaluatorVector
{
	DECLARE_ENGINE_CLASS( CEvaluatorVectorCurve, IEvaluatorVector, 0 );

protected:

	CurveParameter	m_curves;		//!< The curves

public:
	CEvaluatorVectorCurve();
	virtual CurveParameter* GetCurves() { return &m_curves; }
	virtual Bool IsFunction() const { return true; }
	virtual void EvaluateInternal( Float x, Vector & result ) const;
	virtual void OnPropertyPostChange( IProperty* property );
	virtual void OnSerialize( IFile& file );

	virtual Bool GetApproximationSamples( TDynArray< Vector >& samples ) const;

};

BEGIN_CLASS_RTTI( CEvaluatorVectorCurve );
PARENT_CLASS( IEvaluatorVector );
END_CLASS_RTTI();

/// Evaluate vector evaluator, with default value
RED_INLINE Vector EvaluateVector( IEvaluatorVector* evaluator, Float x = 0.0f, const Vector& defaultValue = Vector::ZEROS )
{
	Vector result = defaultValue;
	if ( evaluator )
	{
		evaluator->Evaluate( x, result );
	}
	return result;
}

// Random vector (0-1)
Vector GRandomVector();

Vector3 GRandomVector3();

// Random vector
RED_INLINE Vector GRandomVector( const Vector& min, const Vector& max )
{	
	const Vector delta = max - min;
	return min + ( delta * GRandomVector() );
}

// Random vector
RED_INLINE Vector3 GRandomVector( const Vector3& min, const Vector3& max )
{	
	const Vector3 delta = max - min;
	return min + ( delta * GRandomVector3() );
}

// Random vector
template< typename TEngine > 
RED_INLINE Vector GRandomVector( const Vector& min, const Vector& max, Red::Math::Random::Generator< TEngine >& generator )
{	
	const Vector delta = max - min;
	const Vector multiplier( generator.template Get< Float >(), generator.template Get< Float >(), generator.template Get< Float >(), generator.template Get< Float >() );
	return min + ( delta * multiplier );
}

// Random vector
template< typename TEngine > 
RED_INLINE Vector3 GRandomVector3( const Vector& min, const Vector& max, Red::Math::Random::Generator< TEngine >& generator )
{	
	const Vector3 delta = max - min;
	const Vector3 multiplier( generator.template Get< Float >(), generator.template Get< Float >(), generator.template Get< Float >() );
	return min + ( delta * multiplier );
}