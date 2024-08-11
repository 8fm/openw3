#pragma once

#include "questCondition.h"


///////////////////////////////////////////////////////////////////////////////

enum EInputCompareFunc
{
	ICF_DontCare,
	ICF_Less,
	ICF_Greater,
};

BEGIN_ENUM_RTTI( EInputCompareFunc );
	ENUM_OPTION( ICF_DontCare );
	ENUM_OPTION( ICF_Less );
	ENUM_OPTION( ICF_Greater );
END_ENUM_RTTI();


class CQuestInputCondition : public IQuestCondition
{
	DECLARE_ENGINE_CLASS( CQuestInputCondition, IQuestCondition, 0 )

private:
	CName					m_gameInput;
	EInputCompareFunc		m_valueCompareFunc;
	Float					m_value;

	Bool					m_isFulfilled;

public:
	CQuestInputCondition();
	virtual ~CQuestInputCondition() {}

#ifndef NO_EDITOR_PROPERTY_SUPPORT
	virtual String GetDescription() const 
	{ 
		return String::Printf( TXT( "Game input: %s, m_value: %f" ), m_gameInput.AsString().AsChar(), m_value ); 
	}
#endif

	// Called by the quest system when the 
	void OnEvent( const CName& event, Float value );

	const CName& GetGameInputName() const { return m_gameInput; }

protected:
	//! IQuestCondition implementation
	virtual void OnActivate();
	virtual void OnDeactivate();
	virtual Bool OnIsFulfilled();
};

BEGIN_CLASS_RTTI( CQuestInputCondition )
	PARENT_CLASS( IQuestCondition )
	PROPERTY_CUSTOM_EDIT( m_gameInput, TXT( "Game input we expect to get" ), TXT("GameInputSelection") )
	PROPERTY_EDIT( m_valueCompareFunc, TXT( "How to interpret the associated input value" ) )
	PROPERTY_EDIT( m_value, TXT( "Input value threshold." ) )
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////
