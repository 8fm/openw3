#pragma once

#include "../../common/game/questCondition.h"

///////////////////////////////////////////////////////////////////////////////

struct SUsedFastTravelEvent;

enum EFastTravelConditionType
{
	FTCT_StartedFastTravel,
	FTCT_FinishedFastTravel,
};

BEGIN_ENUM_RTTI( EFastTravelConditionType );
	ENUM_OPTION( FTCT_StartedFastTravel );
	ENUM_OPTION( FTCT_FinishedFastTravel );
END_ENUM_RTTI();

class CQuestUsedFastTravelCondition : public IQuestCondition
{
	DECLARE_ENGINE_CLASS( CQuestUsedFastTravelCondition, IQuestCondition, 0 )

private:
	Bool						m_isFulfilled;

	CName						m_pinTag;
	EFastTravelConditionType	m_conditionType;

public:
	CQuestUsedFastTravelCondition();
	virtual ~CQuestUsedFastTravelCondition() {}

#ifndef NO_EDITOR_PROPERTY_SUPPORT
	virtual String GetDescription() const 
	{ 
		return String::Printf( TXT( "Used fast travel condition" ) ); 
	}
#endif

	// Called by the quest system when the world map fires an event
	void OnEvent( const SUsedFastTravelEvent& event );

protected:
	//! IQuestCondition implementation
	virtual void OnActivate();
	virtual void OnDeactivate();
	virtual Bool OnIsFulfilled();
};

BEGIN_CLASS_RTTI( CQuestUsedFastTravelCondition )
	PARENT_CLASS( IQuestCondition )
	PROPERTY_EDIT( m_pinTag, TXT("Pin tag") )
	PROPERTY_EDIT( m_conditionType, TXT("Condition type") )
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////
