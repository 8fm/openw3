/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
class CR6NPC : public CNewNPC
{
	DECLARE_ENGINE_CLASS( CR6NPC, CNewNPC, 0 );

public:
	CR6NPC();

public:
	void funcEnableCharacterControllerPhysics		( CScriptStackFrame& stack, void* result );
	void funcGetClosestWalkableSpot					( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CR6NPC );
	PARENT_CLASS( CNewNPC );
	NATIVE_FUNCTION( "I_EnableCharacterControllerPhysics"		, funcEnableCharacterControllerPhysics );		
	NATIVE_FUNCTION( "I_GetClosestWalkableSpot"					, funcGetClosestWalkableSpot );		
END_CLASS_RTTI();
