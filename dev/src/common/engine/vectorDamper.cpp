#include "build.h"
#include "vectorDamper.h"
#include "..\core\gameSave.h"

/////////////////////////////////////////////////////////////////////

CVectorDamper::CVectorDamper()
    : m_damping( 1.0f )
    , m_startValue( Vector::ZEROS )
    , m_destStartDiff( Vector::ZEROS )
    , m_time( 0.0f )
    , m_easingFunctionPtr( nullptr )
{
    SetTransitionParms( EET_Linear, EET_InOut );
}

//////////////////////////////////////////////////////////////////////////

CVectorDamper::CVectorDamper( const Vector& start, const Vector& dest, Float damping, ETransitionType transition, EEasingType type )
    : m_startValue( start )
{
    SetDestValue( dest );
    SetDampingTime( damping );
    SetTransitionParms( transition, type );
}

//////////////////////////////////////////////////////////////////////////

void CVectorDamper::Setup( const Vector& start, const Vector& dest, Float damping, ETransitionType transition, EEasingType type )
{
    m_startValue = start;
    SetDestValue( dest );
    SetDampingTime( damping );
    SetTransitionParms( transition, type );
}

/////////////////////////////////////////////////////////////////////

void CVectorDamper::SetTransitionParms(ETransitionType transition, EEasingType type)
{
    m_easingFunctionPtr = EasingFunctions::GetFunctionPtr( transition, type );
}

/////////////////////////////////////////////////////////////////////

void CVectorDamper::SetDestValue( const Vector& destValue )
{
    const Vector diff = destValue - m_startValue;
    m_destStartDiff = diff;
}

/////////////////////////////////////////////////////////////////////

void CVectorDamper::SetStartValue( const Vector& startValue )
{
    const Vector dest = m_startValue + m_destStartDiff;
    m_startValue = startValue;
    m_destStartDiff = dest - m_startValue;
}

/////////////////////////////////////////////////////////////////////

void CVectorDamper::SetInterpolationTime( Float currTime )
{
    m_time = Clamp<Float>( currTime, 0.0f, 1.0f );
}

//////////////////////////////////////////////////////////////////////////

Vector CVectorDamper::Update( Float dt )
{
    if( m_easingFunctionPtr == nullptr || HasReachedDestValue() )
    {
        return GetDestValue();
    }

    m_time += dt * m_damping;
    m_time = Clamp<Float>( m_time, 0.0f, 1.0f );

    return GetInterpolatedValue();
}

/////////////////////////////////////////////////////////////////////

void CVectorDamper::SetDampingTime( Float dampingTime )
{
    Float inverseDamping = 1.0f;

    if( dampingTime != 0.0f )
    {
        inverseDamping = 1.0f / dampingTime;
    }

    if( inverseDamping < DAMPER_EPSILON )
    {
        m_damping = DAMPER_EPSILON;
    }
    else
    {
        m_damping = inverseDamping;
    }
}

/////////////////////////////////////////////////////////////////////

void CVectorDamper::ResetInterpolationTime( void )
{
    m_time = 0.0f;
}

/////////////////////////////////////////////////////////////////////

Vector CVectorDamper::GetInterpolatedValue() const
{
    if( m_time == 0.0f )
    {
        return m_startValue;
    }
    else if( m_time >= 1.0f )
    {
        return GetDestValue();
    }
    else
    {
        const Float X = m_easingFunctionPtr( m_time, m_startValue.X, m_destStartDiff.X, 1.0f );
        const Float Y = m_easingFunctionPtr( m_time, m_startValue.Y, m_destStartDiff.Y, 1.0f );
        const Float Z = m_easingFunctionPtr( m_time, m_startValue.Z, m_destStartDiff.Z, 1.0f );

        return Vector( X, Y, Z );
    }
}

/////////////////////////////////////////////////////////////////////

Vector CVectorDamper::GetStartValue() const
{
    return m_startValue;
}

/////////////////////////////////////////////////////////////////////

Vector CVectorDamper::GetDestValue() const
{
    return m_startValue + m_destStartDiff;
}

/////////////////////////////////////////////////////////////////////

Float CVectorDamper::GetDampingTime() const
{
    return 1.0f / m_damping;
}

//////////////////////////////////////////////////////////////////////////

Float CVectorDamper::GetInterpolationTime() const
{
    return m_time;
}

/////////////////////////////////////////////////////////////////////

Bool CVectorDamper::HasReachedDestValue() const
{
    return m_time >= 1.0f;
}

/////////////////////////////////////////////////////////////////////

RED_DEFINE_STATIC_NAME(vectorDamper);
RED_DEFINE_STATIC_NAME(vectorDamperDamping);
RED_DEFINE_STATIC_NAME(vectorDamperStartValue);
RED_DEFINE_STATIC_NAME(vectorDamperDestStartDiff);
RED_DEFINE_STATIC_NAME(vectorDamperTime);

//////////////////////////////////////////////////////////////////////////

void CVectorDamper::OnSaveGameplayState( IGameSaver* saver )
{
    if( m_easingFunctionPtr == nullptr )
        return;

    saver->BeginBlock( CNAME(vectorDamper) );
    {
        saver->WriteValue( CNAME(vectorDamperDamping),          m_damping );
        saver->WriteValue( CNAME(vectorDamperStartValue),       m_startValue );
        saver->WriteValue( CNAME(vectorDamperDestStartDiff),    m_destStartDiff );
        saver->WriteValue( CNAME(vectorDamperTime),             m_time );
    }
    saver->EndBlock( CNAME(vectorDamper) );
}

//////////////////////////////////////////////////////////////////////////

void CVectorDamper::OnLoadGameplayState( IGameLoader* loader )
{
    if( m_easingFunctionPtr == nullptr )
        return;

    loader->BeginBlock( CNAME(vectorDamper) );
    {
        loader->ReadValue( CNAME(vectorDamperDamping),          m_damping );
        loader->ReadValue( CNAME(vectorDamperStartValue),       m_startValue );
        loader->ReadValue( CNAME(vectorDamperDestStartDiff),    m_destStartDiff );
        loader->ReadValue( CNAME(vectorDamperTime),             m_time );
    }
    loader->EndBlock( CNAME(vectorDamper) );
}

//////////////////////////////////////////////////////////////////////////