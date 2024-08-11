/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "../../common/engine/cameraComponent.h"

//------------------------------------------------------------------------------------------------------------------
// Camera component to be implemented in scripts
//------------------------------------------------------------------------------------------------------------------
class CScriptedCameraComponent : public CCameraComponent
{
	DECLARE_ENGINE_CLASS( CScriptedCameraComponent, CCameraComponent, 0 )

public:
	virtual	void LogicStartEntering		( Float timeToComplete );
	virtual	void LogicCompleteEntering	( );
	virtual	void LogicStartLeaving		( Float timeToComplete );
	virtual	void LogicCompleteLeaving	( );
	virtual	void LogicUpdate			( Float _Dt );
	virtual void LogicInitialize		( );
};

BEGIN_CLASS_RTTI( CScriptedCameraComponent );
PARENT_CLASS( CCameraComponent );
END_CLASS_RTTI();
