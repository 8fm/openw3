#pragma once

#include "questCondition.h"

class CQuestLoadingScreenCondition : public IQuestCondition
{
	DECLARE_ENGINE_CLASS( CQuestLoadingScreenCondition, IQuestCondition, 0 )

private:
	Bool	m_isShown;

protected:
	CQuestLoadingScreenCondition();
	virtual ~CQuestLoadingScreenCondition();

	//! IQuestCondition implementation
	virtual Bool OnIsFulfilled() override;

public:
#ifndef NO_EDITOR_PROPERTY_SUPPORT
	virtual String GetDescription() const 
	{ 
		return String::Printf( TXT( "Shown: %d" ), m_isShown ); 
	}
#endif
};

BEGIN_CLASS_RTTI( CQuestLoadingScreenCondition )
	PARENT_CLASS( IQuestCondition )
	PROPERTY_EDIT( m_isShown, TXT("If loading screen is shown") )
END_CLASS_RTTI()
