#pragma once

#include "questCondition.h"
#include "../../common/engine/globalEventsManager.h"

class CQuestFactsDBConditionBase : public IQuestCondition, public IGlobalEventsListener
{
	DECLARE_ENGINE_ABSTRACT_CLASS( CQuestFactsDBConditionBase, IQuestCondition )

protected:
	String				m_factId;			//!< The id/name of desired fact
	Int32				m_value;			//!< Value to compare with
	ECompareFunc		m_compareFunc;		//!< How to compare values
	EQueryFact			m_queryFact;		//!< Which query method to use	

	Bool				m_isFulfilled;
	Bool				m_wasRegistered;

	CQuestFactsDBConditionBase();
	virtual ~CQuestFactsDBConditionBase();

	//! IQuestCondition implementation
	virtual void OnActivate() override;
	virtual void OnDeactivate() override;
	virtual Bool OnIsFulfilled() override;

	Bool RegisterCallback( Bool reg );
	void QueryFact();
	virtual Bool IsFactCondFulfilled() = 0;

	//! IGlobalEventsManager
	virtual void OnGlobalEvent( EGlobalEventCategory eventCategory, EGlobalEventType eventType, const SGlobalEventParam& param ) override;

public:
#ifndef NO_EDITOR_PROPERTY_SUPPORT
	virtual String	GetDescription() const 
	{ 
		return String::Printf( 
			TXT( "FactID: %s, Value: %d, CompareFunc: %s, QueryFact %s" ),
			m_factId.AsChar(), m_value, 
			CEnum::ToString( m_compareFunc ).AsChar(), CEnum::ToString( m_queryFact ).AsChar() ); 
	}
#endif
};

BEGIN_ABSTRACT_CLASS_RTTI( CQuestFactsDBConditionBase )
	PARENT_CLASS( IQuestCondition )
	PROPERTY_EDIT( m_factId, TXT("The id/name of desired fact.") )
	PROPERTY_EDIT( m_queryFact, TXT("Select query method to use.") )
	PROPERTY_EDIT( m_value, TXT("Value to compare with.") )
	PROPERTY_EDIT( m_compareFunc, TXT("How to compare values.") )
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////

class CQuestFactsDBCondition : public CQuestFactsDBConditionBase
{
	DECLARE_ENGINE_CLASS( CQuestFactsDBCondition, CQuestFactsDBConditionBase, 0 )

public:

	virtual Bool IsFactCondFulfilled() override;
};

BEGIN_CLASS_RTTI( CQuestFactsDBCondition )
	PARENT_CLASS( CQuestFactsDBConditionBase )
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////

class CQuestFactsDBForbiddenCondition : public CQuestFactsDBConditionBase
{
	DECLARE_ENGINE_CLASS( CQuestFactsDBForbiddenCondition, CQuestFactsDBConditionBase, 0 )

public:
#ifndef NO_EDITOR_PROPERTY_SUPPORT
	virtual String GetDescription() const 
	{ 
		return String::Printf( TXT( "Forbidden %s" ), TBaseClass::GetDescription().AsChar() );
	}
#endif

	virtual Bool IsFactCondFulfilled() override;
};

BEGIN_CLASS_RTTI( CQuestFactsDBForbiddenCondition )
	PARENT_CLASS( CQuestFactsDBConditionBase )
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////

class CQuestFactsDBExCondition : public IQuestCondition, public IGlobalEventsListener
{
	DECLARE_ENGINE_CLASS( CQuestFactsDBExCondition, IQuestCondition, 0 )

private:
	String				m_factId1;			//!< The id/name of desired fact
	String				m_factId2;			//!< The id/name of desired fact
	ECompareFunc		m_compareFunc;		//!< How to compare values
	EQueryFact			m_queryFact;		//!< Which query method to use	

	Bool				m_isFulfilled;
	Bool				m_wasRegistered;

protected:
	CQuestFactsDBExCondition();
	virtual ~CQuestFactsDBExCondition();

	//! IQuestCondition implementation
	virtual void OnActivate() override;
	virtual void OnDeactivate() override;
	virtual Bool OnIsFulfilled() override;

	Bool RegisterCallback( Bool reg );
	void QueryFact();

	//! IGlobalEventsManager
	virtual void OnGlobalEvent( EGlobalEventCategory eventCategory, EGlobalEventType eventType, const SGlobalEventParam& param ) override;

public:
#ifndef NO_EDITOR_PROPERTY_SUPPORT
	virtual String	GetDescription() const 
	{ 
		return String::Printf( 
			TXT( "Fact1ID: %s, Fact2ID: %s, Value: %d, CompareFunc: %s, QueryFact %s" ),
			m_factId1.AsChar(), m_factId2.AsChar(),
			CEnum::ToString( m_compareFunc ).AsChar(), CEnum::ToString( m_queryFact ).AsChar() ); 
	}
#endif
};

BEGIN_CLASS_RTTI( CQuestFactsDBExCondition )
	PARENT_CLASS( IQuestCondition )
	PROPERTY_EDIT( m_factId1, TXT("The id/name of desired fact.") )
	PROPERTY_EDIT( m_factId2, TXT("The id/name of desired fact.") )
	PROPERTY_EDIT( m_queryFact, TXT("Select query method to use.") )
	PROPERTY_EDIT( m_compareFunc, TXT("How to compare values.") )
END_CLASS_RTTI()
