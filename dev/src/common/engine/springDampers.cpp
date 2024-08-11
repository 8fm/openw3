
#include "build.h"
#include "springDampers.h"


// Only classes, not functions, may be partially specialized.
// So if you need anything more, just add it by hand.
template<>
TDamper_CriticalDampPolicy< Float, Float, TDamper_DefaultDiffPolicy<Float> >& TDamper_CriticalDampPolicy< Float, Float, TDamper_DefaultDiffPolicy<Float> >::operator=( const TDamper_CriticalDampPolicy< Float, Float, TDamper_DefaultDiffPolicy<Float> >& other )
{
	m_smoothTime = other.m_smoothTime;
	m_velocity = other.m_velocity;

	return *this;
}