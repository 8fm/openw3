#pragma once
#include "easingFunctions.h"

////////////////////////////////////////////////////////////////////////////////
// For more info see FloatDamper.h, VectorDamper works the same

class CVectorDamper
{
public:
    CVectorDamper();
    CVectorDamper( const Vector& start, const Vector& dest, Float damping = 1.0f, ETransitionType transition = EET_Linear, EEasingType type = EET_InOut );

    void Setup( const Vector& start, const Vector& dest, Float damping = 1.0f, ETransitionType transition = EET_Linear, EEasingType type = EET_InOut );

    // Returns interpolated value computed from current damper settings
    Vector  GetInterpolatedValue() const;
    // Returns start value
    Vector  GetStartValue() const;
    // Returns destination value
    Vector  GetDestValue() const;
    // Returns interpolation damping factor. If damping < 1 interpolation slows down, if damping > 0 interpolation accelerates
    Float   GetDampingTime() const;
    // Returns total accumulated interpolation time
    Float   GetInterpolationTime() const;
    // Returns true if interpolation was finished
    Bool    HasReachedDestValue() const;

    // Set value to interpolate from
    void    SetStartValue( const Vector& startValue );
    // Set value to interpolate to
    void    SetDestValue( const Vector& destValue );
    // Set damping factor of interpolation, if you feed update with seconds, damping is the amount of seconds that will pass before value would be interpolated
    void    SetDampingTime( Float dampingTime );
    // Set total interpolation time
    void    SetInterpolationTime( Float currTime );
    // Reset total interpolation time
    void    ResetInterpolationTime( void );
    // Adds 'deltaTime' to total interpolation time and returns result of interpolation
    Vector Update( Float dt );

    // Set interpolation function and function type ( covered in easingFunctions.h )
    void    SetTransitionParms( ETransitionType transition, EEasingType type );

    // Save state
    void OnSaveGameplayState( IGameSaver* saver );
    // Load state
    void OnLoadGameplayState( IGameLoader* loader );

private:
    EasingFunctionPtr   m_easingFunctionPtr;    // Pointer to easing function
    Float               m_damping;
    Vector              m_startValue;
    Vector              m_destStartDiff;
    Float               m_time;
};

//////////////////////////////////////////////////////////////////////////
