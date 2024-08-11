/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "gameplayWindComponent.h"
#include "../engine/renderFrame.h"

IMPLEMENT_ENGINE_CLASS( CGameplayWindComponent );

const Float CGameplayWindComponent::WIND_RANGE_MIN( 1.f );
const Float CGameplayWindComponent::WIND_RANGE_MAX( 100.f );
const Float CGameplayWindComponent::WIND_ANGLE_MIN( 0.f );
const Float CGameplayWindComponent::WIND_ANGLE_MAX( 45.f );
const Float CGameplayWindComponent::WIND_POWER_MIN( 0.f );
const Float CGameplayWindComponent::WIND_POWER_MAX( 100.f );

CGameplayWindComponent::CGameplayWindComponent()
	: m_power( 1.f )
#ifndef NO_EDITOR_PROPERTY_SUPPORT
	, m_color( Color::LIGHT_BLUE )
#endif
{
}

void CGameplayWindComponent::OnAttached( CWorld* world )
{
	// Pass to base class
	TBaseClass::OnAttached( world );

	#ifndef NO_EDITOR_PROPERTY_SUPPORT
		// Register in editor fragment list
		world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_Wind );
	#endif

	// Flag gameplay entity as a wind emitter
	CGameplayEntity *gmplent = Cast< CGameplayEntity > ( GetEntity() );
	if ( gmplent )
	{
		gmplent->SetGameplayFlags( FLAG_HasWind );
	}
}

void CGameplayWindComponent::OnDetached( CWorld* world )
{
	#ifndef NO_EDITOR_PROPERTY_SUPPORT
		// Unregister from editor fragment list
		world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_Wind );
	#endif

	// Pass to base class
	TBaseClass::OnDetached( world );
}

#ifndef NO_EDITOR_PROPERTY_SUPPORT
void CGameplayWindComponent::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag )
{
	// Pass to base class
	TBaseClass::OnGenerateEditorFragments( frame, flag );

	if ( flag != SHOW_Wind )
	{
		return;
	}

	// draw
	if ( frame )
	{
		// gather the parameters
		const Float		radius		= GetWindRadius();
		const Float		range		= GetWindRange();
		const Float		halfAngle	= DEG2RAD( GetWindHalfAngle() );
		const Vector	dir			= GetWindDirection();

		// calculate number of circle points
		Uint16 numPoints = 10;
		if ( radius > 1.f )
		{
			const Float multiplier = 1.f + ( ( radius - 1.f ) / 20.f );
			numPoints = Uint16( Float( numPoints ) * multiplier );
		}

		// put the emitter position into the matrix
		Matrix m, matrix;
		m.SetIdentity();
		m.SetTranslation( GetWorldPosition() );

		// Set the color
		Color color = m_color, clr;

		// draw the arrow
		frame->AddDebugArrow( m, dir, range * m_power, color, true );

		// reuse the matrix to rotate the tube
		m.SetTranslation( Vector::ZEROS );
		m.SetRotX33( DEG2RAD( 90.f ) );
		matrix = m * GetWorldRotation().ToMatrix(); 

		// draw the tube
		color.A = 127;
		clr.R = color.R;
		clr.G = color.G;
		clr.B = color.B;
		clr.A = 31;
		frame->AddDebugTube( GetWorldPosition(), GetWorldPosition() + ( dir * range ), radius, radius + ( MTan( halfAngle ) * range ), matrix, color, clr, numPoints );

		// Set the color
		color = m_color;
		clr = m_color;
		color.A = clr.A = 255;

		// how many circles?
		const Uint32 numCircles = Uint32( ( MSqrt( range ) + 1.f ) * 2.f ) + 1; 

		// iterate
		for ( Uint32 i = 0; i < numCircles; ++i )
		{
			// calculate the advance
			const Float advance = Float( i ) / Float( numCircles - 1 );

			// calculate the distance
			const Float distance = range * advance; 

			// calculate the radius
			const Float R = radius + ( MTan( halfAngle ) * distance );

			// calculate the color
			color = clr;
			color.A = Uint8( Float( clr.A ) * ( 1.f - ( advance / 2.f ) ) );

			// draw the circle
			frame->AddDebugCircle( GetWorldPosition() + ( dir * distance ), R, matrix, color, numPoints );
		}

	}
}
#endif

//! Wind parameters: radius
Float CGameplayWindComponent::GetWindRadius() const
{
	return Clamp< Float > ( GetEntity()->GetScale().X * m_transform.GetScale().X, WIND_RANGE_MIN, WIND_RANGE_MAX );
}

//! Wind parameters: range
Float CGameplayWindComponent::GetWindRange() const
{
	return Clamp< Float > ( GetEntity()->GetScale().Y * m_transform.GetScale().Y, WIND_RANGE_MIN, WIND_RANGE_MAX );
}

//! Wind parameters: power
Float CGameplayWindComponent::GetWindPower() const
{
	return Clamp< Float > ( m_power, WIND_POWER_MIN, WIND_POWER_MAX );
}

//! Wind parameters: half-angle
Float CGameplayWindComponent::GetWindHalfAngle() const
{
	return Clamp< Float > ( GetEntity()->GetScale().Z * m_transform.GetScale().Z, WIND_ANGLE_MIN, WIND_ANGLE_MAX );
}

//! Wind parameters: direction
const Vector CGameplayWindComponent::GetWindDirection() const
{
	return GetLocalToWorld().ToEulerAnglesFull().TransformVector( Vector::EY );
}

Vector CGameplayWindComponent::GetWindAtPoint( const Vector& point ) const
{
	// wind power multiplier
	Float projectionAlpha = 0.f;

	// wind range
	const Float range = GetWindRange();

	// wind power
	const Float power = GetWindPower();

	// wind direction
	const Vector direction = GetWindDirection();

	// wind radius
	const Float radius = GetWindRadius();

	// distance from the direction vector
	const Float d = MSqrt( PointToLineDistanceSquared3( GetWorldPositionRef(), direction, point, projectionAlpha ) );

	// out of range
	if ( projectionAlpha <= -radius || projectionAlpha >= range )
	{
		return Vector::ZEROS;
	}

	// radius at the projected point 
	Float R;

	// we're behind the emitter
	if ( projectionAlpha < 0.f )
	{
		// use smallest radius
		R = radius/* - projectionAlpha*/;

		// pretend that we're closer to the end of range
		projectionAlpha = ( -projectionAlpha / radius ) * range;
	}
	else
	{
		// radius at the projected point 
		R = radius + ( MTan( DEG2RAD( GetWindHalfAngle() ) ) * projectionAlpha );
	}

	// out of radius?
	if ( d >= 2.f * R )
	{
		return Vector::ZEROS;
	}

	// multiplier for large radius
	Float radiusMultiplier = 1.f;

	if ( d > R )
	{
		radiusMultiplier = 1.f - ( ( d - R ) / R );
	}

	// inside the range and the radius, calculate the resulting wind vector
	return direction * ( range - projectionAlpha ) * radiusMultiplier * power; 
}