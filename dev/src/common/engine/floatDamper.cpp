#include "build.h"
#include "FloatDamper.h"
#include "..\core\gameSave.h"

/////////////////////////////////////////////////////////////////////

CFloatDamper::CFloatDamper()
    : m_damping( 1.0f )
    , m_startValue( 0.0f)
    , m_destStartDiff( 0.0f )
    , m_time( 0.0f )
    , m_easingFunctionPtr( nullptr )
{
    SetTransitionParms( EET_Linear, EET_InOut );
}

//////////////////////////////////////////////////////////////////////////

CFloatDamper::CFloatDamper( Float start, Float dest, Float damping, ETransitionType transition, EEasingType type )
    : m_startValue( start )
{
    SetDestValue( dest );
    SetDampingTime( damping );
    SetTransitionParms( transition, type );
}

//////////////////////////////////////////////////////////////////////////

void CFloatDamper::Setup( Float start, Float dest, Float damping, ETransitionType transition, EEasingType type )
{
    m_startValue = start;
    SetDestValue( dest );
    SetDampingTime( damping );
    SetTransitionParms( transition, type );
}

/////////////////////////////////////////////////////////////////////

void CFloatDamper::SetTransitionParms(ETransitionType transition, EEasingType type)
{
    m_easingFunctionPtr = EasingFunctions::GetFunctionPtr( transition, type );
}

//////////////////////////////////////////////////////////////////////////

RED_DEFINE_STATIC_NAME(damperFloat);
RED_DEFINE_STATIC_NAME(damperDamping);
RED_DEFINE_STATIC_NAME(damperStartValue);
RED_DEFINE_STATIC_NAME(damperDestStartDiff);
RED_DEFINE_STATIC_NAME(damperTime);

void CFloatDamper::OnSaveGameplayState( IGameSaver* saver )
{
    if( m_easingFunctionPtr == nullptr )
        return;

    saver->BeginBlock( CNAME(damperFloat) );
    {
        saver->WriteValue( CNAME(damperDamping), m_damping );
        saver->WriteValue( CNAME(damperStartValue), m_startValue );
        saver->WriteValue( CNAME(damperDestStartDiff), m_destStartDiff );
        saver->WriteValue( CNAME(damperTime), m_time );
    }
    saver->EndBlock( CNAME(damperFloat) );
}

//////////////////////////////////////////////////////////////////////////

void CFloatDamper::OnLoadGameplayState( IGameLoader* loader )
{
    if( m_easingFunctionPtr == nullptr )
        return;

    loader->BeginBlock( CNAME(damperFloat) );
    {
        loader->ReadValue( CNAME(damperDamping), m_damping );
        loader->ReadValue( CNAME(damperStartValue), m_startValue );
        loader->ReadValue( CNAME(damperDestStartDiff), m_destStartDiff );
        loader->ReadValue( CNAME(damperTime), m_time );
    }
    loader->EndBlock( CNAME(damperFloat) );
}

/////////////////////////////////////////////////////////////////////

void CFloatDamper::SetDestValue( Float destValue )
{
    const Float diff = destValue - m_startValue;
    m_destStartDiff = diff;
}

/////////////////////////////////////////////////////////////////////

void CFloatDamper::SetStartValue( Float startValue )
{
    const Float dest = m_startValue + m_destStartDiff;
    m_startValue = startValue;
    m_destStartDiff = dest - m_startValue;
}

/////////////////////////////////////////////////////////////////////

void CFloatDamper::SetInterpolationTime( Float currTime )
{
    m_time = Clamp<Float>( currTime, 0.0f, 1.0f );
}

//////////////////////////////////////////////////////////////////////////

Float CFloatDamper::Update( Float dt )
{
    if( m_easingFunctionPtr == nullptr || HasReachedDestValue())
    {
        return GetDestValue();
    }

    m_time += dt * m_damping;
    m_time = Clamp<Float>( m_time, 0.0f, 1.0f );

    return GetInterpolatedValue();
}

/////////////////////////////////////////////////////////////////////

void CFloatDamper::SetDampingTime( Float dampingTime )
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

void CFloatDamper::ResetInterpolationTime( void )
{
    m_time = 0.0f;
}

/////////////////////////////////////////////////////////////////////

Float CFloatDamper::GetInterpolatedValue() const
{
    if( m_time <= 0.0f )
    {
        return m_startValue;
    }
    else if( m_time >= 1.0f )
    {
        return GetDestValue();
    }
    else
    {
        return m_easingFunctionPtr( m_time, m_startValue, m_destStartDiff, 1.0f );
    }
}

/////////////////////////////////////////////////////////////////////

Float CFloatDamper::GetStartValue() const
{
    return m_startValue;
}

/////////////////////////////////////////////////////////////////////

Float CFloatDamper::GetDestValue() const
{
    return m_startValue + m_destStartDiff;
}

/////////////////////////////////////////////////////////////////////

Float CFloatDamper::GetDampingTime() const
{
    return 1.0f / m_damping;
}

//////////////////////////////////////////////////////////////////////////

Float CFloatDamper::GetInterpolationTime() const
{
    return m_time;
}

/////////////////////////////////////////////////////////////////////

Bool CFloatDamper::HasReachedDestValue() const
{
    return m_time >= 1.0f;
}

/////////////////////////////////////////////////////////////////////