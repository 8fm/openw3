/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../engine/performableAction.h"


class CAddFactPerformableAction : public IPerformableAction
{
	DECLARE_ENGINE_CLASS( CAddFactPerformableAction, IPerformableAction, 0 )
protected:
	String			m_factID;
	Int32			m_value;
	Int32			m_validForSeconds;
public:
	CAddFactPerformableAction();

	void Perform( CEntity* parent ) override;
};

BEGIN_CLASS_RTTI( CAddFactPerformableAction	)
	PARENT_CLASS( IPerformableAction )
	PROPERTY_EDIT( m_factID, TXT( "Fact name" ) )
	PROPERTY_EDIT( m_value, TXT( "Fact value" ) )
	PROPERTY_EDIT( m_validForSeconds, TXT("Time in seconds for fact to expire. -1 never, -2 after act") )
END_CLASS_RTTI()