#pragma once


#include "r6CameraComponent.h"



class CExplorationCameraComponent : public CR6CameraComponent
{
	DECLARE_ENGINE_CLASS( CExplorationCameraComponent, CR6CameraComponent, 0 )
	
	Float				m_distance;
	Float				m_angleFromAim;
	Float				m_cameraRadius;

	/// @todo this should be removed, this is player height?
	Float				m_heightOffset;

	Float				m_maxRoll;
	Float				m_maxRollChangePerSecond;
	Float				m_yawSpeedForMaxRoll;
	Float				m_currRoll;

	Vector				m_position;
	EulerAngles			m_rotation;


	/// @brief Used for collision detection
	Float				m_cameraBlobRadius;

	CCurve*				m_pitchToFovCurve;
	CCurve*				m_pitchToDistanceCurve;

	Float				m_maxFOVChangePerSecond;
	Float				m_maxDistanceChangePerSecond;

public:
	CExplorationCameraComponent();

	virtual void OnActivate( const IScriptable* prevCameraObject, Bool resetCamera ) override;
	virtual Bool Update( Float timeDelta ) override;

	virtual void OnPropertyPostChange( IProperty* property ) override;

	virtual Bool GetData( Data& outData ) const override;

private:
	void AdjustByPitch( Float timeDelta );
	void AdjustValue( Float& val, Float target, Float maxAbsChange );


	Float GetTargetFovFromPitch( Float pitch ) const;
	Float GetTargetDistanceFromPitch( Float pitch ) const;


	/// @Brief this method is used to call 'create' for ccurve pointers.
	///			It must be here, due to a lack of funtionality in CCurve editor.
	/// @todo MS: this is marked as an issue and should be fixed in the future.
	void CreateEmptyCurves();


	Vector ResolveCollisions( const Vector& playerPos, const Vector& targetPos );
};






BEGIN_CLASS_RTTI( CExplorationCameraComponent );

	PARENT_CLASS( CR6CameraComponent );

	PROPERTY_EDIT( m_angleFromAim							, TXT( "Angle From Aim" ) );
	PROPERTY_EDIT( m_cameraRadius							, TXT( "Camera Radius" ) );
	PROPERTY_EDIT( m_heightOffset							, TXT( "Height Offset" ) );

	PROPERTY_EDIT_RANGE( m_maxRoll							, TXT( "Max roll added by turning. [deg]" ), 0.0f, 90.0f );
	PROPERTY_EDIT( m_maxRollChangePerSecond					, TXT( "Max roll change during one second. [deg/s]"))
	PROPERTY_EDIT( m_yawSpeedForMaxRoll						, TXT( "Yaw speed required to reach max roll. [deg/s]" ) );

	PROPERTY_EDIT( m_cameraBlobRadius						, TXT( "Size of physical blob used for collision detection. [m]" ) );

	PROPERTY_CUSTOM_EDIT( m_pitchToFovCurve					, TXT( "Pitch to FOV mapping - relation between current Pitch and FOV. Y axis is equal FOV divided by 30 (30 degrees == 1 on Y axis). X axis maps to Pitch ( [0] -> -90, [1] -> 0, [2] -> 90 )" ), TXT( "CurveSelection" ) );
	PROPERTY_CUSTOM_EDIT( m_pitchToDistanceCurve			, TXT( "Pitch to distance mapping - relation between current Pitch and Camera-to-player distance. Y axis corresponds to distance in meters. X axis maps to Pitch ( [0] -> -90, [1] -> 0, [2] -> 90 )" ), TXT( "CurveSelection" ) );


	PROPERTY_EDIT_RANGE( m_maxFOVChangePerSecond			, TXT( "How fast is FOV adjusted by pitch. [deg/s]" ), 0.0f, 3600.0f );
	PROPERTY_EDIT_RANGE( m_maxDistanceChangePerSecond		, TXT( "How fast is distance adjusted by pitch. [m/s]" ), 0.0f, 1000.0f );



END_CLASS_RTTI();
