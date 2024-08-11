
#pragma once

#include "../../common/engine/visualDebug.h"

class CVisualDebug_MovementTrajectory : public CObject, IVisualDebugInterface
{

	DECLARE_ENGINE_CLASS( CVisualDebug_MovementTrajectory, CObject, 0 );

	Bool					m_isAdded;
	CActor*					m_entity;
	CMovingAgentComponent*	m_movingAgentComponent;
	Matrix					m_posHistory[2];

public:
	CVisualDebug_MovementTrajectory();

	void Init( CActor* entity );
	void Reset();

public:
	virtual void Render( CRenderFrame* frame, const Matrix& matrix );

private:
	void funcReset( CScriptStackFrame& stack, void* result );
	void funcInit( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CVisualDebug_MovementTrajectory );
	PARENT_CLASS( CObject );
	NATIVE_FUNCTION( "Reset", funcReset );
	NATIVE_FUNCTION( "Init", funcInit );
END_CLASS_RTTI();
