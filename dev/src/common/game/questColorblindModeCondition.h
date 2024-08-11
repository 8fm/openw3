/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "questCondition.h"

class CQuestColorblindModeCondition : public IQuestCondition
{
	DECLARE_ENGINE_CLASS( CQuestColorblindModeCondition, IQuestCondition, 0 )

public:

#ifndef NO_EDITOR_PROPERTY_SUPPORT
	virtual String GetDescription() const 
	{ 
		return String::Printf( TXT( "Is colorblind mode" ) ); 
	}
#endif

protected:
	//! IQuestCondition implementation
	virtual Bool OnIsFulfilled() override;
};

BEGIN_CLASS_RTTI( CQuestColorblindModeCondition )
	PARENT_CLASS( IQuestCondition )
END_CLASS_RTTI()