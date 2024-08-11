#pragma once

class CIDInterlocutorComponent;
class CInteractiveDialogInstance;

//------------------------------------------------------------------------------------------------------------------
// Conditions adapted for dialog, having a ref to the scene they belong to
//------------------------------------------------------------------------------------------------------------------
class IIDContition : public CObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IIDContition, CObject )

	//------------------------------------------------------------------------------------------------------------------
	// Methods
	//------------------------------------------------------------------------------------------------------------------
public:
	virtual Bool				IsFulfilled			( Uint32 dialogId )					const	= 0;

	CIDInterlocutorComponent*	GetInterlocutor		( CName	name, Uint32 dialogId )		const;
	CInteractiveDialogInstance*	GetDialogInstance	( Uint32 dialogId )					const;
};

BEGIN_ABSTRACT_CLASS_RTTI( IIDContition )
	PARENT_CLASS( CObject )
END_CLASS_RTTI()

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
class IIDScriptedContition : public IIDContition
{
	DECLARE_ENGINE_CLASS( IIDScriptedContition, IIDContition, 0 )

	//------------------------------------------------------------------------------------------------------------------
	// Methods
	//------------------------------------------------------------------------------------------------------------------
protected:
	virtual Bool	IsFulfilled				( Uint32 dialogId ) const	override;

private:
	//void	funcGetDialogInstance	( CScriptStackFrame& stack, void* result );
	void			funcGetInterlocutor		( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( IIDScriptedContition )
	PARENT_CLASS( IIDContition )
	//NATIVE_FUNCTION( "I_GetDialogInstance"	, funcGetDialogInstance );
	NATIVE_FUNCTION( "I_GetInterlocutor"	, funcGetInterlocutor );
END_CLASS_RTTI()

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
class CIDScriptedInterlocutorContition : public IIDScriptedContition
{
	DECLARE_ENGINE_CLASS( CIDScriptedInterlocutorContition, IIDScriptedContition, 0 )

protected:
	CName m_conditionInterlocutor;

private:
	void funcGetConditionInterlocutor( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CIDScriptedInterlocutorContition )
	PARENT_CLASS( IIDScriptedContition )
	PROPERTY_CUSTOM_EDIT( m_conditionInterlocutor, TXT("Interlocutor for this condition"), TXT("InterlocutorIDList") )
	NATIVE_FUNCTION( "I_GetConditionInterlocutor", funcGetConditionInterlocutor )
END_CLASS_RTTI()

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------

struct SIDInterlocutorNameWrapper
{
	DECLARE_RTTI_STRUCT( SIDInterlocutorNameWrapper );
	CName m_interlocutor;
};

BEGIN_NODEFAULT_CLASS_RTTI( SIDInterlocutorNameWrapper );
	PROPERTY_CUSTOM_EDIT( m_interlocutor, TXT("InterlocutorID of an existing actor"), TXT("InterlocutorIDList") );
END_CLASS_RTTI();

class CIDScriptedManyInterlocutorsContition : public IIDScriptedContition
{
	DECLARE_ENGINE_CLASS( CIDScriptedManyInterlocutorsContition, IIDScriptedContition, 0 )

protected:
	TDynArray< SIDInterlocutorNameWrapper > m_conditionInterlocutors;

private:
	void funcGetConditionInterlocutor( CScriptStackFrame& stack, void* result );
	void funcGetNumConditionInterlocutors( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CIDScriptedManyInterlocutorsContition )
	PARENT_CLASS( IIDScriptedContition )
	PROPERTY_EDIT( m_conditionInterlocutors, TXT("Interlocutors for this condition") )
	NATIVE_FUNCTION( "I_GetConditionInterlocutor", funcGetConditionInterlocutor )
	NATIVE_FUNCTION( "I_GetNumConditionInterlocutors", funcGetNumConditionInterlocutors )
END_CLASS_RTTI()

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
enum EIDLogicOperation
{
	IDLO_And,
	IDLO_Or,
	IDLO_Xor
};

BEGIN_ENUM_RTTI( EIDLogicOperation );
	ENUM_OPTION( IDLO_And );
	ENUM_OPTION( IDLO_Or );
	ENUM_OPTION( IDLO_Xor );
END_ENUM_RTTI();

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
class IDConditionList : public IIDContition
{
	DECLARE_ENGINE_CLASS( IDConditionList, IIDContition, 0 )

	//------------------------------------------------------------------------------------------------------------------
	// Variables
	//------------------------------------------------------------------------------------------------------------------
private:
	EIDLogicOperation				m_operation;

protected:
	TDynArray< IIDContition* >		m_conditions;

	//------------------------------------------------------------------------------------------------------------------
	// Methods
	//------------------------------------------------------------------------------------------------------------------
public:
	virtual Bool	IsFulfilled			( Uint32 dialogId )		const;
};

BEGIN_CLASS_RTTI( IDConditionList )
	PARENT_CLASS( IIDContition )
	PROPERTY_EDIT( m_operation, TXT("Logic operation") )
	PROPERTY_INLINED( m_conditions, TXT("Conditions") )
END_CLASS_RTTI()