/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/
#pragma once

/// Idle state of NPC, NPC wanders around the world
class CNewNPCStateIdle : public CNewNPCStateReactingBase
{
	DECLARE_RTTI_SIMPLE_CLASS( CNewNPCStateIdle );
	NO_DEFAULT_CONSTRUCTOR( CNewNPCStateIdle );

protected:
};

BEGIN_CLASS_RTTI( CNewNPCStateIdle );
	PARENT_CLASS( CNewNPCStateReactingBase );
END_CLASS_RTTI();
