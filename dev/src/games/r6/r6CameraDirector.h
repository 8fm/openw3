#pragma once

#include "..\..\common\engine\cameraDirector.h"
#include "..\..\common\core\math.h"



class CR6CameraDirector : public CCameraDirector
{
	DECLARE_ENGINE_CLASS( CR6CameraDirector, CCameraDirector, 0 )



	Float m_cameraShake;
	Float m_cameraShakeTime;
	Float m_cameraShakeAmplitude;

	Bool m_autoCameraManagementOn;

public:

	CR6CameraDirector();



	virtual void Update( Float timeDelta ) override;
	virtual void CacheCameraData() override;


	/// @brief This method will contain all camera management code on the CameraDirector side
	void UpdateAutoCameraManager( Float timeDelta );


	/// @brief this is a global shake for all cameras
	/// @todo add shakes for specific cameras
	void SetAllCamerasShake( Float time, Float amplitude );


	Vector GetPointInCachedLocal( const Vector& worldPoint ) const;


	RED_INLINE void TurnAutoCameraManagementOn()	{ m_autoCameraManagementOn = true; }
	RED_INLINE void TurnAutoCameraManagementOff() { m_autoCameraManagementOn = false; }



private:

	void funcSetAllCamerasShake( CScriptStackFrame& stack, void* result );
	void funcGetPointInCachedLocal( CScriptStackFrame& stack, void* result );

	void funcTurnAutoCameraManagementOn( CScriptStackFrame& stack, void* result );
	void funcTurnAutoCameraManagementOff( CScriptStackFrame& stack, void* result );

};







BEGIN_CLASS_RTTI( CR6CameraDirector )
	
	PARENT_CLASS( CCameraDirector )
	
	NATIVE_FUNCTION( "SetAllCamerasShake", funcSetAllCamerasShake );
	NATIVE_FUNCTION( "GetPointInCachedLocal", funcGetPointInCachedLocal );
	
	NATIVE_FUNCTION( "TurnAutoCameraManagementOn", funcTurnAutoCameraManagementOn );
	NATIVE_FUNCTION( "TurnAutoCameraManagementOff", funcTurnAutoCameraManagementOff );

END_CLASS_RTTI();






RED_INLINE Vector CR6CameraDirector::GetPointInCachedLocal( const Vector& worldPoint ) const
{
	Matrix worldToLocal = m_cachedTransform.FullInverted();
	return worldToLocal.TransformPoint(worldPoint);
}


