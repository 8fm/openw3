
#pragma once

#include "../core/mathUtils.h"

template < typename T >
class TDamper_ZeroInit
{
public:
	static T Create();
};

template <>
class TDamper_ZeroInit< Vector >
{
public:
	static RED_INLINE Vector Create() { return Vector::ZERO_3D_POINT; }
};

template<>
class TDamper_ZeroInit< EulerAngles >
{
public:
	static RED_INLINE EulerAngles Create() { return EulerAngles::ZEROS; }
};

template <>
class TDamper_ZeroInit< Float >
{
public:
	static RED_INLINE Float Create() { return 0.f; }
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

template < typename T >
class TDamper_DefSmooth
{
public:
	static T Create();
};

template <>
class TDamper_DefSmooth< Float >
{
public:
	static RED_INLINE Float Create() { return 0.1f; }
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

template < typename T >
class TDamper_DefaultDiffPolicy
{
public:
	static RED_INLINE T CalcDiff( const T& a, const T& b )
	{
		return a - b;
	}
};

template <>
class TDamper_DefaultDiffPolicy< EulerAngles >
{
public:
	static RED_INLINE EulerAngles CalcDiff( const EulerAngles& a, const EulerAngles& b )
	{
		return EulerAngles::AngleDistance( b, a );
	}
};

template < typename T >
class TDamper_AngularDiffPolicy
{
public:
	static RED_INLINE T CalcDiff( const T& a, const T& b )
	{
		return EulerAngles::AngleDistance( b, a );
	}
};

template <>
class TDamper_AngularDiffPolicy< Vector >
{
public:
	static RED_INLINE Vector CalcDiff( const Vector& a, const Vector& b )
	{
		return Vector( EulerAngles::AngleDistance( b.X, a.X ), EulerAngles::AngleDistance( b.Y, a.Y ), EulerAngles::AngleDistance( b.Z, a.Z ) );
	}
};

template < typename T >
class TDamper_CyclicDiffPolicy
{
public:
	static RED_INLINE T CalcDiff( const T& a, const T& b )
	{
		return MathUtils::CyclicDistance( b, a );
	}
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

template < typename T, typename SmoothType = Float, typename DiffPolicy = TDamper_DefaultDiffPolicy< T > >
class TDamper_CriticalDampPolicy
{
private:
	SmoothType	m_smoothTime;

	T			m_velocity;

public:
	TDamper_CriticalDampPolicy() : m_smoothTime( TDamper_DefSmooth<T>::Create() ), m_velocity( TDamper_ZeroInit<T>::Create() ) {};

	TDamper_CriticalDampPolicy( const SmoothType& smoothTime ) : m_smoothTime( smoothTime ), m_velocity( TDamper_ZeroInit<T>::Create() ) {};

	TDamper_CriticalDampPolicy( const TDamper_CriticalDampPolicy& copy ) : m_smoothTime( copy.m_smoothTime ), m_velocity( copy.m_velocity ) {};

	// We can specialize only the non-reference assign operator.
	TDamper_CriticalDampPolicy& operator=( const TDamper_CriticalDampPolicy& );
	
	void Update( T& curr, const T& dest, Float dt )
	{
		if ( m_smoothTime > 0.f )
		{
			const Float omega = 2.f / m_smoothTime;
			const Float x = omega * dt;
			const Float exp = 1.0f / ( 1.0f + x + 0.48f*x*x + 0.235f*x*x*x );
			const T diff = DiffPolicy::CalcDiff( curr, dest );
			const T temp = ( m_velocity + diff * omega ) * dt;
			m_velocity = ( m_velocity - temp * omega ) * exp;
			curr = dest + ( diff + temp ) * exp;
		}
		else if ( dt > 0.0f )
		{
			m_velocity = DiffPolicy::CalcDiff( dest, curr ) / dt;
			curr = dest;
		}
	}

	void SetSmoothingFactor( const SmoothType& smoothTime ) { m_smoothTime = smoothTime; }

	RED_INLINE const T&	GetVelocity() const					{ return m_velocity; }
	RED_INLINE void		SetVelocity( const T& velocity )	{ m_velocity = velocity; }
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

template < typename T, typename DampPolicy >
class TDamper
{
protected:
	T			m_curr;
	T			m_dest;
	DampPolicy	m_damp;

public:
	TDamper()
		: m_damp( TDamper_DefSmooth<T>::Create() )
	{
		Force( TDamper_ZeroInit<T>::Create() );
	}

	TDamper( const Float& dampFactor )
		: m_damp( dampFactor )
	{
		Force( TDamper_ZeroInit<T>::Create() );
	}

	TDamper( const DampPolicy& damp )
		: m_damp( damp )
	{
		Force( TDamper_ZeroInit<T>::Create() );
	}

	RED_INLINE void Update( Float dt )
	{
		m_damp.Update( m_curr, m_dest, dt );
	}

	RED_INLINE void Update( const T& dest, Float dt )
	{
		m_dest = dest;
		Update( dt );
	}

	void Update( T& curr, T& vel, const T& dest, Float dt )
	{
		m_curr = curr;
		m_damp.SetVelocity( vel );
		Update( dest, dt );
		curr = m_curr;
		vel = m_damp.GetVelocity();
	}

	RED_INLINE T GetValue() const
	{
		return m_curr;
	}

	RED_INLINE void SetDestValue( const T& dest )
	{
		m_dest = dest;
	}

	RED_INLINE T GetDestValue() const
	{
		return m_dest;
	}

	RED_INLINE T GetVelocity() const
	{
		return m_damp.GetVelocity();
	}

	RED_INLINE void SetDampFactor( const Float& dampFactor )
	{
		m_damp.SetSmoothingFactor( dampFactor );
	}

	void Reset()
	{
		m_curr = m_dest;
		m_damp.SetVelocity( TDamper_ZeroInit<T>::Create() );
	}

	void Force( const T& dest )
	{
		m_dest = dest;
		Reset();
	}

	RED_INLINE void ForceNoReset( const T& dest )
	{
		m_dest = dest;
		m_curr = dest;
	}
};

//////////////////////////////////////////////////////////////////////////

typedef TDamper< Float, TDamper_CriticalDampPolicy< Float, Float > >										TFloatCriticalDamp;
typedef TDamper< Float, TDamper_CriticalDampPolicy< Float, Float, TDamper_AngularDiffPolicy< Float > > >	TFloatAngleCriticalDamp;
typedef TDamper< Float, TDamper_CriticalDampPolicy< Float, Float, TDamper_CyclicDiffPolicy< Float > > >		TFloatCyclicCriticalDamp;

//////////////////////////////////////////////////////////////////////////

typedef TDamper< Float, TDamper_CriticalDampPolicy< Float, const Float& > >				TFloatCriticalDampRef;
typedef TDamper< Vector, TDamper_CriticalDampPolicy< Vector, const Float& > >			TVectorCriticalDampRef;
typedef TDamper< EulerAngles, TDamper_CriticalDampPolicy< EulerAngles, const Float& > >	TEulerAnglesCriticalDampRef;

//////////////////////////////////////////////////////////////////////////

typedef TDamper< Float, TDamper_CriticalDampPolicy< Float, const Float&, TDamper_AngularDiffPolicy< Float > > > TFloatAngleCriticalDampRef;

//////////////////////////////////////////////////////////////////////////

typedef TDamper< Float, TDamper_CriticalDampPolicy< Float, const Float&, TDamper_CyclicDiffPolicy< Float > > > TFloatCyclicCriticalDampRef;