#pragma once

#include "build.h"
#include "safestPathRater.h"

class CNPCMovementControllerComponent : public CR6Component
{
	DECLARE_ENGINE_CLASS( CNPCMovementControllerComponent, CR6Component, 0 );
private:
	CSafestParhRater	m_safePathReater;
	
public:
	void OnAttached( CWorld* world ) override;

private:
	void funcSafeMoveToAsync( CScriptStackFrame& stack, void* result );
	void funcActionEnded( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CNPCMovementControllerComponent );
	PARENT_CLASS( CR6Component );
	NATIVE_FUNCTION( "I_SafeMoveToAsync"	, funcSafeMoveToAsync	);
	NATIVE_FUNCTION( "I_ActionEnded"		, funcActionEnded		);
END_CLASS_RTTI();