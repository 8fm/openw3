#include "build.h"
#include "idCondition.h"

#include "idSystem.h"
#include "idInstance.h"
#include "idInterlocutor.h"

IMPLEMENT_ENGINE_CLASS( IIDContition )
IMPLEMENT_ENGINE_CLASS( IIDScriptedContition )
IMPLEMENT_ENGINE_CLASS( SIDInterlocutorNameWrapper )
IMPLEMENT_ENGINE_CLASS( CIDScriptedInterlocutorContition )
IMPLEMENT_ENGINE_CLASS( CIDScriptedManyInterlocutorsContition )
IMPLEMENT_ENGINE_CLASS( IDConditionList )
IMPLEMENT_RTTI_ENUM( EIDLogicOperation )

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
CInteractiveDialogInstance*	IIDContition::GetDialogInstance	(Uint32 dialogId)	const
{
	CInteractiveDialogSystem*	dialogSystem	= GCommonGame->GetSystem< CInteractiveDialogSystem >();
	return dialogSystem->GetDialogInstance( dialogId );
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
CIDInterlocutorComponent*	IIDContition::GetInterlocutor( CName name, Uint32 dialogId ) const
{
	CInteractiveDialogSystem*	dialogSystem	= GCommonGame->GetSystem< CInteractiveDialogSystem >();
	return dialogSystem->GetInterlocutorOnDialog( dialogId, name );
}
																										
//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
Bool	IIDScriptedContition::IsFulfilled( Uint32 dialogId ) const
{
	Bool result = true;
	IIDScriptedContition* nonConsThis = const_cast< IIDScriptedContition* >( this );
	CallFunctionRet< Bool >( nonConsThis, CNAME( Evaluate ), dialogId, result );

	// TODO: Need a way of enabling the log only for conditions evaluated once (like in a condition block)
	// For now, just leaving it here to uncomment when needed.
	// RED_LOG( Dialog, TXT("Checking condition %s, result is %s"), GetClass()->GetName().AsString().AsChar(), result ); 

	return result;
}
/*
//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void	IIDScriptedContition::funcGetDialogInstance	( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	CInteractiveDialogInstance*	dialogInstance	= GetDialogInstance();

	// ED TODO: Fix it
	//RETURN_HANDLE( CInteractiveDialogInstance,  dialogInstance );
}
*/
//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void	IIDScriptedContition::funcGetInterlocutor	( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, interlocutorId, CName::NONE );
	GET_PARAMETER( Uint32, dialogId, 0 );
	FINISH_PARAMETERS;
	
	CIDInterlocutorComponent*	interlocutor	= GetInterlocutor( interlocutorId, dialogId );

	RETURN_HANDLE( CIDInterlocutorComponent,  interlocutor );
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
Bool IDConditionList::IsFulfilled( Uint32 dialogId ) const
{
	// By default it is considered fulfilled, in case the array is empty
	Bool result = true;
	for( TDynArray< IIDContition* >::const_iterator it =	m_conditions.Begin(); it != m_conditions.End(); ++it )
	{
		IIDContition*	condition	= *it;
		if ( it == m_conditions.Begin() )
		{
			if ( condition )
			{
				result = condition->IsFulfilled( dialogId );
			}
		}
		else
		{
			// If there is no condition, we don't take it into account, so it stays true by default
			if ( condition )
			{
				switch ( m_operation )
				{
				case IDLO_And:	result &= condition->IsFulfilled( dialogId ); break;
				case IDLO_Or:	result |= condition->IsFulfilled( dialogId ); break;
				case IDLO_Xor:	result ^= condition->IsFulfilled( dialogId ); break;
				}
			}
		}
	}
	return result;
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CIDScriptedInterlocutorContition::funcGetConditionInterlocutor( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Uint32, dialogId, 0 );
	FINISH_PARAMETERS;

	CIDInterlocutorComponent* component = NULL;
	CInteractiveDialogInstance* instance = GetDialogInstance( dialogId );
	if ( instance )
	{
		component = instance->GetInterlocutor( m_conditionInterlocutor );
	}

	RETURN_HANDLE( CIDInterlocutorComponent, component ); 
}

void CIDScriptedManyInterlocutorsContition::funcGetConditionInterlocutor( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Uint32, dialogId, 0 );
	GET_PARAMETER( Uint32, interlocutorIndex, 0 );
	FINISH_PARAMETERS;

	CIDInterlocutorComponent* component = NULL;
	if ( interlocutorIndex < m_conditionInterlocutors.Size() )
	{
		CInteractiveDialogInstance* instance = GetDialogInstance( dialogId );
		if ( instance )
		{
			component = instance->GetInterlocutor( m_conditionInterlocutors[ interlocutorIndex ].m_interlocutor );
		}
	}

	RETURN_HANDLE( CIDInterlocutorComponent, component ); 
}

void CIDScriptedManyInterlocutorsContition::funcGetNumConditionInterlocutors( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_INT( m_conditionInterlocutors.SizeInt() );
}
