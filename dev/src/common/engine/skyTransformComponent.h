#pragma once
#include "component.h"


/// Sky transform type
enum ESkyTransformType : CEnum::TValueType
{
	STT_SunPosition,
	STT_MoonPosition,
	STT_GlobalLightPosition,
};

BEGIN_ENUM_RTTI( ESkyTransformType );
	ENUM_OPTION( STT_SunPosition )
	ENUM_OPTION( STT_MoonPosition )
	ENUM_OPTION( STT_GlobalLightPosition )
END_ENUM_RTTI();


/// Sky transform component
class CSkyTransformComponent : public CComponent
{
	DECLARE_ENGINE_CLASS( CSkyTransformComponent, CComponent, 0 );

protected:
	Vector				m_lastCameraPos;
	ESkyTransformType	m_transformType;
	Float				m_cameraDistance;
	Float				m_uniformScaleDistance;

public:
	CSkyTransformComponent();

public:
	// Called when component is attached to world ( layer gets visible, etc )
	virtual void OnAttached( CWorld* world );

	// Called when component is detached from world ( layer gets hidden, etc )
	virtual void OnDetached( CWorld* world );

public:
	// On tick
	virtual void OnTick( Float timeDelta );

	// Calculates global world space transform matrix
	Bool CalcWorldGlobalTransformMatrix( Matrix& matrix ) const;

	// Do we need global, world space matrix for node? ( aka. rigid body, etc )
	Bool UsesGlobalTransformMatrix() const { return true; }

};

BEGIN_CLASS_RTTI( CSkyTransformComponent )
	PARENT_CLASS(CComponent)
	PROPERTY_EDIT(m_transformType, TXT("Transformation type"));
	PROPERTY_EDIT(m_cameraDistance, TXT("Distance from camera"));
	PROPERTY_EDIT(m_uniformScaleDistance, TXT("Uniform scale distance"));
END_CLASS_RTTI()
