
#pragma once

#include "r6AdvancedVehicleComponent.h"







struct SAVIdleData
{
	DECLARE_RTTI_STRUCT( SAVIdleData );

	Bool	m_previewInEditor;
	Float	m_time;
	Vector3 m_center;
	Float	m_waveFrequency;
	Float	m_waveAmplitude;
	Float	m_currStrength;
	/// time required from idle start to moment, it gets full strength
	Float	m_maxStrengthAfter;

	SAVIdleData();
};








struct SAVSpeedData
{
	DECLARE_RTTI_STRUCT( SAVSpeedData );

	Bool	m_driving;
	Float	m_accForward;
	Float	m_dccForward;
	Float	m_accReverse;
	Float	m_dccReverse;
	Float	m_maxSpeedForward;
	Float	m_maxSpeedReverse;
	Float	m_currSpeed;


	SAVSpeedData();
};








struct SAVTurningData
{
	DECLARE_RTTI_STRUCT( SAVTurningData );

	Bool	m_turning;
	Float	m_yawAcc;
	Float	m_pitchAcc;
	Float	m_maxYawSpeed;
	Float	m_maxPitchSpeed;
	Float	m_currYawSpeed;
	Float	m_currPitchSpeed;

	Float	m_currRoll;
	Float	m_maxRoll;

	Float	m_pitchSnappingWhileStanding;
	Float	m_pitchSnappingWhileFullSpeed;

	SAVTurningData();
};








struct SAVHoveringData
{
	DECLARE_RTTI_STRUCT( SAVHoveringData );

	Bool	m_hovering;

	Float	m_minAccVertical;
	Float	m_maxAccVertical;
	Float	m_minTopSpeedVertical;
	Float	m_maxTopSpeedVertical;
	Float	m_currSpeedVertical;


	/// @todo remove?
	Float	m_maxPitch;

	Float	m_minAccHorizontal;
	Float	m_maxAccHorizontal;
	Float	m_minTopSpeedHorizontal;
	Float	m_maxTopSpeedHorizontal;
	Float	m_currSpeedHorizontal;

	Float	m_currRoll;
	Float	m_maxRoll;


	Float	m_minDropDistance;
	Float	m_maxDropDistance;
	Float	m_dropMultiplier;

	Float	m_minSideDropDistance;
	Float	m_maxSideDropDistance;
	Float	m_sideDropMultiplier;


	SAVHoveringData();
};









struct SAVSafetyBrakesData
{
	DECLARE_RTTI_STRUCT( SAVSafetyBrakesData );

	Float	m_minDistanceToGround;
	Float	m_startBrakingDistance;
	Float	m_currBrakingForce;
	Float	m_maxBrakingForce;
	Float	m_currBrakingACC;
	Float	m_maxBrakingACC;
	Float	m_currBrakingSpeed;
	Float	m_maxBrakingSpeed;

	SAVSafetyBrakesData();
};











/**
 * @brief Test AV component (AV movement mechanics)
 * @author MSobiecki
 * @created 2014-02
 */
class CAVComponentTest : public CR6AdvancedVehicleComponent
{
	DECLARE_ENGINE_CLASS( CAVComponentTest, CR6AdvancedVehicleComponent, 0 );

public:




private:



// ----------------------- general -----------------------
	Float					m_mass;
	Vector3					m_velocity;
	Float					m_maxAltitude;
	Float					m_altitudeCorrectionCoef;





// ----------------------- av mechanics data -----------------------
	SAVIdleData				m_idleData;
	SAVSpeedData			m_speedData;
	SAVTurningData			m_turningData;
	SAVHoveringData			m_hoveringData;
	SAVSafetyBrakesData		m_sbData;





// ----------------------- input -----------------------
	Float					m_accelerateInput;
	Float					m_steeringAxisInputX;
	Float					m_steeringAxisInputY;
	Float					m_hoveringAxisInputX;
	Float					m_hoveringAxisInputY;



	
// ----------------------- debug -----------------------
	/// @todo MS:	Remove all this development stuff!

	/// If something is in this section, it should be removed with some small changes (development decisions)
	Bool					m_DEV_STEERING_RIGHT_AXIS;
	Bool					m_DEV_STEERING_INVERSE;
	Bool					m_DEV_HOVERING_RIGHT_AXIS;
	Bool					m_DEV_HOVERING_INVERSE;

	///	If something is in this section, it may be safely removed (with all occurences) without any changes:
	Float					m_debugCurrACC;
	Float					m_debugCurrDropCoef;
	Float					m_debugCurrSideACCMult;
	Float					m_debugCurrSideSpeedMult;





protected:
	CAVComponentTest();

	virtual void OnAttached( CWorld* world )	override;
	
	virtual void OnTickIdle( Float dt )			override;

	virtual void UpdatePlayerInput( Float dt )	override;
	virtual void UpdateLogic( Float dt )		override;


	void UpdateLinearSpeed( Float dt );
	void UpdateTurningPitch( Float dt );
	void UpdateTurningYaw( Float dt );
	void UpdateHovering( Float dt );



	void UpdateIdle( Float dt );

	void TryMove( Float dt );
	
	void CombineRoll();



	Float GetCurrentAccFromInput( Float diff );
	void UpdateHoveringSpeed( Float dt, Float axisInput, Float axisAcc, Float axisMaxSpeed, Float& axisCurrSpeed );
	
	void ClampPitch( Float& pitch );

public:

	virtual void OnPilotMounted( CPilotComponent* pilot ) override;
	virtual void OnPilotDisMounted( ) override;

	

	/// @return current Abs( speed ) / max speed (should be between 0 and 1)
	Float GetVehicleSpeedToMaxSpeedCoefAbs();
	/// @return current pitch speed / max pitch speed ( [-1; 1] )
	Float GetVehiclePitchSpeedToMaxSpeedCoef();
	/// @return current yaw speed / max yaw speed ( [-1; 1] )
	Float GetVehicleYawSpeedToMaxSpeedCoef();

	Float GetHorizontalHoveringCoef();
	Float GetVerticalHoveringCoef();


	Vector3 GetHorizontalHoveringVelocity();


	/// @return true, if reverse gear is on and velocity is non zero
	Bool IsMovingBackward();
	

	virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag ) override;

private:

	/// @todo MS: Move this to math library maybe?
	Vector3 ProjectToUnitOnXYPlane( const Vector3& v ) const;



	Float GetDistanceInDirection( const Vector3& dir ) const;

	Float GetGroundDistance()	const;
	Float GetTopDistance()		const;
	Float GetRightDistance()	const;
	Float GetLeftDistance()		const;


	void UpdateCurrentSideDrops( Float& outSpeed, Float& outAcc);

	void DrawDebugGauge( CRenderFrame* frame, Float val, Float X, Float Y, const Color& color, const Char* caption = NULL );
};









RED_INLINE Float CAVComponentTest::GetVehicleSpeedToMaxSpeedCoefAbs()
{
	Float maxSpeed = Max( m_speedData.m_maxSpeedForward, m_speedData.m_maxSpeedReverse );
	if( maxSpeed < 1e-5f )
	{
		return 0.0f;
	}

	return Clamp( ( Abs( m_speedData.m_currSpeed ) / maxSpeed ), 0.0f, 1.0f );
}









RED_INLINE Float CAVComponentTest::GetVehiclePitchSpeedToMaxSpeedCoef()
{
	if( m_turningData.m_maxPitchSpeed < 1e-5f )
	{
		return 0.0f;
	}

	return ( m_turningData.m_currPitchSpeed / m_turningData.m_maxPitchSpeed );
}









RED_INLINE Float CAVComponentTest::GetVehicleYawSpeedToMaxSpeedCoef()
{
	if( m_turningData.m_maxYawSpeed < 1e-5f )
	{
		return 0.0f;
	}

	return ( m_turningData.m_currYawSpeed / m_turningData.m_maxYawSpeed );
}









RED_INLINE Float CAVComponentTest::GetHorizontalHoveringCoef()
{
	Float speedCoef = GetVehicleSpeedToMaxSpeedCoefAbs();
	Float currHHTopSpeed = Lerp( speedCoef, m_hoveringData.m_minTopSpeedHorizontal, m_hoveringData.m_maxTopSpeedHorizontal );
	if( currHHTopSpeed < 1e-5f )
	{
		return 0.0f;
	}

	return Clamp( ( m_hoveringData.m_currSpeedHorizontal / currHHTopSpeed ), -1.0f, 1.0f );
}








RED_INLINE Float CAVComponentTest::GetVerticalHoveringCoef()
{
	Float speedCoef = GetVehicleSpeedToMaxSpeedCoefAbs();
	Float currHVTopSpeed = Lerp( speedCoef, m_hoveringData.m_minTopSpeedVertical, m_hoveringData.m_maxTopSpeedVertical );

	if( currHVTopSpeed < 1e-5f )
	{
		return 0.0f;
	}

	return Clamp( ( m_hoveringData.m_currSpeedVertical / currHVTopSpeed ), -1.0f, 1.0f );
}









RED_INLINE Vector3 CAVComponentTest::GetHorizontalHoveringVelocity()
{
	return ProjectToUnitOnXYPlane( GetEntity()->GetWorldRight() ) * m_hoveringData.m_currSpeedHorizontal;
}








RED_INLINE Bool CAVComponentTest::IsMovingBackward()
{
	return ( m_speedData.m_currSpeed < 0.0f );
}










RED_INLINE Float CAVComponentTest::GetGroundDistance() const
{
	return GetDistanceInDirection( Vector3( 0.0f, 0.0f, -1.0f ) );
}









RED_INLINE Float CAVComponentTest::GetTopDistance()	const
{
	return GetDistanceInDirection( Vector3( 0.0f, 0.0f, 1.0f ) );
}







RED_INLINE Float CAVComponentTest::GetRightDistance() const
{
	return GetDistanceInDirection( ProjectToUnitOnXYPlane( GetEntity()->GetWorldRight() ) );
}








RED_INLINE Float CAVComponentTest::GetLeftDistance() const
{
	return GetDistanceInDirection( ProjectToUnitOnXYPlane( -( GetEntity()->GetWorldRight() ) ) );
}














// ----------------------- idle editable values -----------------------
BEGIN_NODEFAULT_CLASS_RTTI( SAVIdleData );
	PROPERTY_EDIT( m_previewInEditor					, TXT( "Should idle movement be visible in the editor? " ) );
	PROPERTY_EDIT_RANGE( m_waveFrequency				, TXT( "Frequency of waving while standing still [Hz]." ), 0.0f,  60.0f );
	PROPERTY_EDIT_RANGE( m_waveAmplitude				, TXT( "Amplitude of waving while standing still [m]." ), 0.0f, 10.0f );
	PROPERTY_EDIT_RANGE( m_maxStrengthAfter				, TXT( "Time that should pass before the idle occurs in its max strength. [s]" ), 0.0f, 1000.0f );
END_CLASS_RTTI();







// ----------------------- accelerating and breaking editable values -----------------------
BEGIN_NODEFAULT_CLASS_RTTI( SAVSpeedData );
	PROPERTY_EDIT_RANGE( m_accForward					, TXT( "Acceleration on 'drive' gear. [m/s2]" ), 0.0f, 1000.0f );
	PROPERTY_EDIT_RANGE( m_dccForward					, TXT( "Deceleration on 'drive' gear. [m/s2]" ), 0.0f, 1000.0f );
	PROPERTY_EDIT_RANGE( m_accReverse					, TXT( "Acceleration on 'reverse' gear. [m/s2]" ), 0.0f, 1000.0f );
	PROPERTY_EDIT_RANGE( m_dccReverse					, TXT( "Deceleration on 'reverse' gear. [m/s2]" ), 0.0f, 1000.0f );
	PROPERTY_EDIT_RANGE( m_maxSpeedForward				, TXT( "Maximum speed on 'drive' gear. [m/s]" ), 0.0f, 1000.0f );
	PROPERTY_EDIT_RANGE( m_maxSpeedReverse				, TXT( "Maximum speed on 'reverse' gear. [m/s]" ), 0.0f, 1000.0f );
END_CLASS_RTTI();






// ----------------------- turning editable values -----------------------
BEGIN_NODEFAULT_CLASS_RTTI( SAVTurningData );
	PROPERTY_EDIT_RANGE( m_yawAcc						, TXT( "Acceleration of horizontal turning angle (how fast the vehicle starts turning). [deg/s2]" ), 0.0f, 360.0f );
	PROPERTY_EDIT_RANGE( m_pitchAcc						, TXT( "Acceleration of vertical turning angle (how fast the vehicle starts turning). [deg/s2]" ), 0.0f, 360.0f );
	PROPERTY_EDIT_RANGE( m_maxYawSpeed					, TXT( "Maximum horizontal turning angle speed (how strong is vehicles turning). [deg/s]" ), 0.0f, 360.0f );
	PROPERTY_EDIT_RANGE( m_maxPitchSpeed				, TXT( "Maximum vertical turning angle speed (how strong is vehicles turning). [deg/s]" ), 0.0f, 360.0f );
	PROPERTY_EDIT_RANGE( m_maxRoll						, TXT( "Maximum roll of the vehicle while in full turn. [deg]" ), 0.0f, 90.0f );

	PROPERTY_EDIT_RANGE( m_pitchSnappingWhileStanding	, TXT( "How much should pitch be snapped to 0 degrees while AV is standing (zero speed)" ), 0.0f, 10000.0f );
	PROPERTY_EDIT_RANGE( m_pitchSnappingWhileFullSpeed	, TXT( "How much should pitch be snapped to 0 degrees while AV is speeding (max speed)" ), 0.0f, 10000.0f );
END_CLASS_RTTI();






// ----------------------- hovering editable values -----------------------
BEGIN_NODEFAULT_CLASS_RTTI( SAVHoveringData );
	PROPERTY_EDIT_RANGE( m_minAccVertical				, TXT( "Acceleration in vertical direction while fully hovering but with no linear speed (standing). [m/s2]" ), 0.0f, 100.0f );
	PROPERTY_EDIT_RANGE( m_maxAccVertical				, TXT( "Acceleration in vertical direction while fully hovering with full linear speed (speeding). [m/s2]" ), 0.0f, 100.0f );
	PROPERTY_EDIT_RANGE( m_minTopSpeedVertical			, TXT( "Maximum speed in vertical direction while fully hovering but with no linear speed (standing). [m/s]" ), 0.0f, 100.0f );
	PROPERTY_EDIT_RANGE( m_maxTopSpeedVertical			, TXT( "Maximum speed in vertical direction while fully hovering with full linear speed (speeding). [m/s]" ), 0.0f, 100.0f );

	PROPERTY_EDIT_RANGE( m_maxPitch						, TXT( "Pitch of the AV while hovering vertically. [deg]" ), 0.0f, 90.0f );

	PROPERTY_EDIT_RANGE( m_minAccHorizontal				, TXT( "Acceleration in horizontal direction while fully hovering but with no linear speed (standing). [m/s2]" ), 0.0f, 100.0f );
	PROPERTY_EDIT_RANGE( m_maxAccHorizontal				, TXT( "Acceleration in horizontal direction while fully hovering with full linear speed (speeding). [m/s2]" ), 0.0f, 100.0f );
	PROPERTY_EDIT_RANGE( m_minTopSpeedHorizontal		, TXT( "Maximum speed in horizontal direction while fully hovering but with no linear speed (standing). [m/s]" ), 0.0f, 100.0f );
	PROPERTY_EDIT_RANGE( m_maxTopSpeedHorizontal		, TXT( "Maximum speed in horizontal direction while fully hovering with full linear speed (speeding). [m/s]" ), 0.0f, 100.0f );

	PROPERTY_EDIT_RANGE( m_maxRoll						, TXT( "Roll of the AV while hovering horizontally. [deg]" ), 0.0f, 90.0f );


	PROPERTY_EDIT( m_minDropDistance					, TXT( "Distance for whitch vertical dropp is allready off. [m]" ) );
	PROPERTY_EDIT( m_maxDropDistance					, TXT( "Distance for whitch vertical dropp is fully active. [m]" ) );
	PROPERTY_EDIT( m_dropMultiplier						, TXT( "How many times stronger is the vertical drop when in the maximum distance." ) );

	PROPERTY_EDIT( m_minSideDropDistance				, TXT( "Distance for whitch side dropp is allready off. [m]" ) );
	PROPERTY_EDIT( m_maxSideDropDistance				, TXT( "Distance for whitch side dropp is fully active. [m]" ) );
	PROPERTY_EDIT( m_sideDropMultiplier					, TXT( "How many times stronger is the side drop when in the maximum distance." ) );
END_CLASS_RTTI();






// ----------------------- safety brakes editable values -----------------------
BEGIN_NODEFAULT_CLASS_RTTI( SAVSafetyBrakesData );
	PROPERTY_EDIT( m_minDistanceToGround				, TXT( "What is the minimum distance to ground you can approach without fully braking. [m]" ) );
	PROPERTY_EDIT( m_startBrakingDistance				, TXT( "What is the distance in whitch the AV starts braking. [m]" ) );
	PROPERTY_EDIT( m_maxBrakingForce					, TXT( "What is the maximum braking force. [N]" ) );
	PROPERTY_EDIT( m_maxBrakingACC						, TXT( "What is the maximum braking acceleration. [m/s2]" ) );
	PROPERTY_EDIT( m_maxBrakingSpeed					, TXT( "What is the maximum braking speed. [m/s]" ) );
END_CLASS_RTTI();
















BEGIN_CLASS_RTTI( CAVComponentTest );

	PARENT_CLASS( CR6AdvancedVehicleComponent );


// ----------------------- general editable values -----------------------
	PROPERTY_EDIT( m_maxAltitude						, TXT( "What is the maximum altitude to fly freely (above this altitude, correction begins). [m]" ) );
	PROPERTY_EDIT( m_altitudeCorrectionCoef				, TXT( "How fast should the correction be performed when max altitude is exceeded." ) );


// ----------------------- av mechanics editable values -----------------------
	PROPERTY_EDIT( m_idleData							, TXT( "Data related to idle state of the av." ) );
	PROPERTY_EDIT( m_speedData							, TXT( "Data related to controlling speed - accelerating and braking." ) );
	PROPERTY_EDIT( m_turningData						, TXT( "Data related to turning of the AV." ) );
	PROPERTY_EDIT( m_hoveringData						, TXT( "Data related to hovering of the AV." ) );
	PROPERTY_EDIT( m_sbData								, TXT( "Data related to automatic safety brakes of the AV." ) );


// ----------------------- debug editable values -----------------------
	PROPERTY_EDIT( m_DEV_STEERING_RIGHT_AXIS			, TXT( "[DEVELOPMENT] Is steering related do right or left axis (right == true)" ) );
	PROPERTY_EDIT( m_DEV_STEERING_INVERSE				, TXT( "[DEVELOPMENT] Invert Y-axis controls while steering" ) );
	PROPERTY_EDIT( m_DEV_HOVERING_RIGHT_AXIS			, TXT( "[DEVELOPMENT] Is hovering related to right or left axis (right == true)" ) );
	PROPERTY_EDIT( m_DEV_HOVERING_INVERSE				, TXT( "[DEVELOPMENT] Invert Y-axis controls while hovering" ) );

END_CLASS_RTTI();
