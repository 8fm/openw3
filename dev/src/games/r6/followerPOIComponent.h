/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once

class CFollowerPOIComponent : public CSelfUpdatingComponent
{
	DECLARE_ENGINE_CLASS( CFollowerPOIComponent, CSelfUpdatingComponent, 0 );

private:
	Int32		m_priority;

public:
	RED_INLINE Int32 GetPriority( ) const { return m_priority; }

private:
	void funcGetPriority( CScriptStackFrame& stack, void* result );

};

BEGIN_CLASS_RTTI( CFollowerPOIComponent );
	PARENT_CLASS( CSelfUpdatingComponent );
	NATIVE_FUNCTION( "I_GetPriority", funcGetPriority );	
	PROPERTY_EDIT( m_priority, TXT( "NPC will go to the POI with highest priority number first" ) );
END_CLASS_RTTI();