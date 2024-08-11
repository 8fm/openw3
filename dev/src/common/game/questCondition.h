#pragma once


class CXMLWriter;

// An interface for conditions that can be used from the quest conditional 
// blocks (regular and pausing)
class IQuestCondition : public CObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IQuestCondition, CObject )

private:
	CName	m_name;
	Bool	m_active;

public:
	IQuestCondition();
	virtual ~IQuestCondition() {}

	const CName& GetName() const { return m_name; }

	void Activate();
	void Deactivate();
	Bool IsFulfilled();

	virtual void SaveGame( IGameSaver* saver );
	virtual void LoadGame( IGameLoader* loader );

#ifndef NO_EDITOR_PROPERTY_SUPPORT
	virtual String	GetDescription() const { return String::EMPTY; }
#endif
	

protected:
	virtual void OnActivate() {}

	virtual void OnDeactivate() {}

	virtual Bool OnIsFulfilled() { return true; }
};

BEGIN_ABSTRACT_CLASS_RTTI( IQuestCondition )
	PARENT_CLASS( CObject )
	PROPERTY_EDIT( m_name, TXT( "Condition name" ) )
	PROPERTY_SAVED( m_active )
END_CLASS_RTTI()

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class CQuestScriptedCondition : public IQuestCondition
{
	DECLARE_ENGINE_CLASS( CQuestScriptedCondition, IQuestCondition, 0 )

	const CFunction*	m_evaluateFunction;

public:
#ifndef NO_EDITOR_PROPERTY_SUPPORT
	virtual String	GetDescription() const { return TXT( "Quest scripted condition" ); }
#endif

protected:

	CQuestScriptedCondition();

	virtual void OnActivate();
	virtual void OnDeactivate();
	virtual Bool OnIsFulfilled();
	virtual void OnScriptReloaded() override;
	void CacheScriptedFunctions();
};

BEGIN_CLASS_RTTI( CQuestScriptedCondition )
	PARENT_CLASS( IQuestCondition )
END_CLASS_RTTI()

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum ELogicOperation
{
	LO_And,
	LO_Or,
	LO_Xor,
	LO_Nand,
	LO_Nor,
	LO_Nxor,
};

BEGIN_ENUM_RTTI( ELogicOperation );
	ENUM_OPTION( LO_And );
	ENUM_OPTION( LO_Or );
	ENUM_OPTION( LO_Xor );
	ENUM_OPTION( LO_Nand );
	ENUM_OPTION( LO_Nor );
	ENUM_OPTION( LO_Nxor );
END_ENUM_RTTI();

class CQuestLogicOperationCondition : public IQuestCondition
{
	DECLARE_ENGINE_CLASS( CQuestLogicOperationCondition, IQuestCondition, 0 )

private:
	TDynArray< IQuestCondition* >	m_conditions;
	ELogicOperation					m_logicOperation;

public:
	CQuestLogicOperationCondition();

public:
#ifndef NO_EDITOR_PROPERTY_SUPPORT
	virtual String	GetDescription() const { return TXT( "Quest logic operation condition" ); }
#endif

protected:
	virtual void OnActivate() override;
	virtual void OnDeactivate() override;
	virtual Bool OnIsFulfilled() override;
};

BEGIN_CLASS_RTTI( CQuestLogicOperationCondition )
	PARENT_CLASS( IQuestCondition )
	PROPERTY_EDIT( m_logicOperation, TXT( "Logic operation used to evaluate conditions" ) )
	PROPERTY_INLINED( m_conditions, TXT( "Conditions to evaluate" ) )
END_CLASS_RTTI()