#pragma once
#include "../../common/engine/cameraComponent.h"

class CInteractionCameraComponent : public CCameraComponent
{
	DECLARE_ENGINE_CLASS( CInteractionCameraComponent, CCameraComponent, 0 )

	Float				m_distanceFromPlayerF;
	CName				m_focusBone;
	Bool				m_focusOnInteractionItem;

public:
	CInteractionCameraComponent();

	void LogicUpdate( Float _Dt )	override;
};

BEGIN_CLASS_RTTI( CInteractionCameraComponent );
PARENT_CLASS( CCameraComponent );
PROPERTY_EDIT( m_distanceFromPlayerF, TXT( "Distance From Player" ) );
PROPERTY_EDIT( m_focusBone, TXT( "Player Focus Bone" ) );
PROPERTY_EDIT( m_focusOnInteractionItem, TXT( "Focus Interaction Item" ) );
END_CLASS_RTTI();
