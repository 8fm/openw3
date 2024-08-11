/**
   * Copyright © 2012 CD Projekt Red. All Rights Reserved.
   */
#pragma once

/// Gameplay wind emitter
class CGameplayWindComponent : public CComponent
{
	DECLARE_ENGINE_CLASS( CGameplayWindComponent, CComponent, 0 );

protected:
	Float				m_power;
#ifndef NO_EDITOR_PROPERTY_SUPPORT
	Color				m_color;
#endif

public:
	static const Float	WIND_RANGE_MIN;
	static const Float	WIND_RANGE_MAX;
	static const Float	WIND_ANGLE_MIN;
	static const Float	WIND_ANGLE_MAX;
	static const Float	WIND_POWER_MIN;
	static const Float	WIND_POWER_MAX;

	CGameplayWindComponent();

	// Called when component is attached to world ( layer gets visible, etc )
	virtual void OnAttached( CWorld* world );

	// Called when component is detached from world ( layer gets hidden, etc )
	virtual void OnDetached( CWorld* world );

#ifndef NO_EDITOR_PROPERTY_SUPPORT
	//! Generate editor fragments
	virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag );
#endif

	//! Wind parameters: radius
	Float GetWindRadius() const;

	//! Wind parameters: range
	Float GetWindRange() const;

	//! Wind parameters: power
	Float GetWindPower() const;

	//! Wind parameters: half-angle
	Float GetWindHalfAngle() const;

	//! Wind parameters: direction (normalized)
	const Vector GetWindDirection() const;

	//! The Wind
	Vector GetWindAtPoint( const Vector& point ) const;
};

BEGIN_CLASS_RTTI( CGameplayWindComponent );
	PARENT_CLASS( CComponent );
	PROPERTY_EDIT_RANGE( m_power, TXT("Scalar wind power (multiplier)"), CGameplayWindComponent::WIND_POWER_MIN, CGameplayWindComponent::WIND_POWER_MAX );
#ifndef NO_EDITOR_PROPERTY_SUPPORT
	PROPERTY_EDIT( m_color, TXT("Debug color in editor") );
#endif
END_CLASS_RTTI();