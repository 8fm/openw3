#pragma once
#include "../../common/engine/cameraComponent.h"

class CSlotCameraComponent : public CCameraComponent
{
	DECLARE_ENGINE_CLASS( CSlotCameraComponent, CCameraComponent, 0 )

	CName				m_cameraSlot;
	CName				m_lookAtSlot;

public:
	CSlotCameraComponent();

	void LogicUpdate( Float _Dt )	override;

private:
	Bool GetSlotData( CEntity* _entity, CName _slotName, Vector& _slotPosition, Vector& _slotForward );
};

BEGIN_CLASS_RTTI( CSlotCameraComponent );
PARENT_CLASS( CCameraComponent );
PROPERTY_EDIT( m_cameraSlot, TXT( "Camera Slot" ) );
PROPERTY_EDIT( m_lookAtSlot, TXT( "Optional - Look At Slot" ) );
END_CLASS_RTTI();
