#pragma once

#include "r6CameraComponent.h"





/**
 * @brief Test AV camera component (related to CAVComponentTest)
 * @author MSobiecki
 * @created 2014-02
 */
class CAVCameraTest : public CR6CameraComponent
{
	DECLARE_ENGINE_CLASS( CAVCameraTest, CR6CameraComponent, 0 )

	Vector				m_position;
	EulerAngles			m_rotation;

	Vector3				m_standardOffset;

	Float				m_maxPitchChangePerSecond;
	Float				m_maxYawChangePerSecond;

	Float				m_maxAVOrientationChangePerSecond;
	EulerAngles			m_prevAVOrientation;

	Float				m_currReverseAngle;
	Float				m_reverseAngleAdjustCoef;

	Float				m_currLookAtDistance;
	Float				m_minSpeedLookAtDistance;
	Float				m_maxSpeedLookAtDistance;
	Float				m_lookAtDistanceAdjustCoef;

	Float				m_currPitchSpeedOffset;
	Float				m_currYawSpeedOffset;
	Float				m_minPitchSpeedOffset;
	Float				m_minYawSpeedOffset;
	Float				m_maxPitchSpeedOffset;
	Float				m_maxYawSpeedOffset;
	Float				m_pitchOffsetAdjustCoef;
	Float				m_yawOffsetAdjustCoef;

	Float				m_currRoll;
	Float				m_rollAdjustCoef;
	Float				m_rollToAVRoll;

	Float				m_currHorizontalHoveringAngle;
	Float				m_currVerticalHoveringAngle;
	Float				m_minHorizontalHoveringAngle;
	Float				m_maxHorizontalHoveringAngle;
	Float				m_minVerticalHoveringAngle;
	Float				m_maxVerticalHoveringAngle;
	Float				m_hHoveringAngleAdjustCoef;
	Float				m_vHoveringAngleAdjustCoef;

	Float				m_minSpeedFOV;
	Float				m_maxSpeedFOV;






public:

	CAVCameraTest();

	virtual void OnActivate( const IScriptable* prevCameraObject, Bool resetCamera )	override;
	virtual Bool Update( Float dt )								override;


	virtual Bool GetData( Data& outData ) const					override;

private:

	/// Get an entity and "av mechanics" component of an AV to be tracked
	/// @return true, if both entity and component were found
	Bool GetAV( CEntity*& avEntity , CAVComponentTest*& avComponent );


	Float GetCurrentInverse() const;

//private:
//	void ResolveCollisions			( Vector& newPos, const Vector& camDir );

};










RED_INLINE Float CAVCameraTest::GetCurrentInverse() const
{
	if( m_currReverseAngle > 90.0f )
	{
		return -1.0f;
	}
	else
	{
		return 1.0f;
	}
}












BEGIN_CLASS_RTTI( CAVCameraTest );

	PARENT_CLASS( CR6CameraComponent );

	PROPERTY_EDIT( m_standardOffset						, TXT( "Describes the offset to the AV when nothing specific is happening (the AV is standing still)." ) );

	PROPERTY_EDIT( m_maxPitchChangePerSecond			, TXT( "Maximum camera pitch change during one second. [deg]" ) );
	PROPERTY_EDIT( m_maxYawChangePerSecond				, TXT( "Maximum camera yaw change during one second. [deg]" ) );

	PROPERTY_EDIT( m_maxAVOrientationChangePerSecond	, TXT( "Maximum av rotation change taken under consideration. [deg]" ) );

	PROPERTY_EDIT( m_reverseAngleAdjustCoef				, TXT( "How fast should camera be reversed when AV is going backwards." ) );

	PROPERTY_EDIT( m_minSpeedLookAtDistance				, TXT( "How far in front of the AV is the observation point while standing. [m]" ) );
	PROPERTY_EDIT( m_maxSpeedLookAtDistance				, TXT( "How far in front of the AV is the observation point while full speed. [m]" ) );
	PROPERTY_EDIT( m_lookAtDistanceAdjustCoef			, TXT( "How fast should 'look at distance' be adjusted." ) );

	PROPERTY_EDIT( m_minPitchSpeedOffset				, TXT( "How much should the camera be offsetted towards AV vertical rotation pivot when not moving. [m]" ) );
	PROPERTY_EDIT( m_minYawSpeedOffset					, TXT( "How much should the camera be offsetted towards AV horizontal rotation pivot when not moving. [m]" ) );
	PROPERTY_EDIT( m_maxPitchSpeedOffset				, TXT( "How much should the camera be offsetted towards AV vertical rotation pivot when speeding. [m]" ) );
	PROPERTY_EDIT( m_maxYawSpeedOffset					, TXT( "How much should the camera be offsetted towards AV horizontal rotation pivot when speeding. [m]" ) );
	PROPERTY_EDIT( m_pitchOffsetAdjustCoef				, TXT( "How fast should pitch offset be adjusted." ) );
	PROPERTY_EDIT( m_yawOffsetAdjustCoef				, TXT( "How fast should yaw offset be adjusted." ) );

	PROPERTY_EDIT( m_rollAdjustCoef						, TXT( "How fast should roll adjust to AV Roll." ) );
	PROPERTY_EDIT( m_rollToAVRoll						, TXT( "How much AV roll influences camera roll." ) );

	PROPERTY_EDIT( m_minHorizontalHoveringAngle			, TXT( "Angle offset from forward direction while hovering horizontally when standing. [deg]" ) );
	PROPERTY_EDIT( m_maxHorizontalHoveringAngle			, TXT( "Angle offset from forward direction while hovering horizontally when speeding. [deg]" ) );
	PROPERTY_EDIT( m_minVerticalHoveringAngle			, TXT( "Angle offset from forward direction while hovering vertically when standing. [deg]" ) );
	PROPERTY_EDIT( m_maxVerticalHoveringAngle			, TXT( "Angle offset from forward direction while hovering vertically when speeding. [deg]" ) );
	PROPERTY_EDIT( m_hHoveringAngleAdjustCoef			, TXT( "How fast should horizontal hovering angle be adjusted." ) );
	PROPERTY_EDIT( m_vHoveringAngleAdjustCoef			, TXT( "How fast should vertical hovering angle be adjusted." ) );

	PROPERTY_EDIT( m_minSpeedFOV						, TXT( "FOV while AV has minimum absolute speed (is standing)." ) );
	PROPERTY_EDIT( m_maxSpeedFOV						, TXT( "FOV while AV has maximum absolute speed." ) );

END_CLASS_RTTI();
