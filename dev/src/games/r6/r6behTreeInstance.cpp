/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "r6behTreeInstance.h"
#include "aiTreeComponent.h"
#include "aiActionPerformerAttachment.h"

IMPLEMENT_ENGINE_CLASS( CR6BehTreeInstance );

CR6BehTreeInstance::CR6BehTreeInstance()
{
}

CAIAction* CR6BehTreeInstance::SpawnAIActionInstance( const CAIAction* templ )
{
	// copy the object
	CClass* klass = templ->GetClass();
	void* mem = klass->CreateObject( klass->GetSize(), true );
	klass->Copy( mem, templ );
	CAIAction* inst = reinterpret_cast< CAIAction* > ( mem );

	// check the properties
#ifndef RED_FINAL
	TDynArray< CProperty* > props;
	klass->GetProperties( props );
	for ( Uint32 i = 0; i < props.Size(); ++i )
	{
		CProperty* prop = props[ i ];
		IRTTIType* type = prop->GetType();
		if ( type->GetType() == RT_Pointer )
		{
			R6_ASSERT( false, TXT("Direct pointers are not allowed inside any class derived from CAIAction! FIX NOW!") );

			// null the pointer
			CAIAction* newValue = nullptr;
			prop->GetSetter()->SetValue( inst, type, &newValue );
		}
	}
#endif

	m_actionInstances.PushBack( inst );
	return inst;
}

void CR6BehTreeInstance::RemoveAIActionInstance( CAIAction* inst )
{
	R6_ASSERT( m_actionInstances.Exist( inst ) );
	m_actionInstances.RemoveFast( inst );
	delete inst;
}

Bool CR6BehTreeInstance::CheckAIActionAvailability( CAIAction* action )
{
	EAIActionStatus status = action->GetStatus();
	if ( status == ACTION_InProgress )
	{
		// already started
		return true;
	}

	CAITreeComponent* component = FindParent< CAITreeComponent > ();
	R6_ASSERT( component );

	const TDynArray< CComponent* >& performers = component->GetPerformers();
	for ( Uint32 i = 0; i < performers.Size(); ++i )
	{
		if ( action->CanBeStartedOn( performers[ i ] ) )
		{
			return true;
		}
	}

	// no component capable of performing the action found
	return false;
}

EAIActionStatus CR6BehTreeInstance::PerformAIAction( CAIAction* actionToPerform )
{
	EAIActionStatus status = actionToPerform->GetStatus();
	if ( status == ACTION_InProgress )
	{
		// already started
		return ACTION_InProgress;
	}

	if ( status == ACTION_Successful || status == ACTION_Failed )
	{
		// reset runtime data
		status = actionToPerform->Reset();
	}

	CAITreeComponent* component = FindParent< CAITreeComponent > ();
	R6_ASSERT( component );

	if ( status == ACTION_NotStarted )
	{
		const TDynArray< CComponent* >& performers = component->GetPerformers();
		for ( Uint32 i = 0; i < performers.Size(); ++i )
		{
			if ( actionToPerform->CanBeStartedOn( performers[ i ] ) ) // TODO: think how to avoid calling it twice... behTreeNode::IsAvailable() ->	CR6BehTreeInstance::CheckAIActionAvailability() and then here for the second time... makes no sense, but we need to know the proper component here...
			{
				status = actionToPerform->StartOn( performers[ i ] );
				break;
			}
		}
	}

	return status;
}

EAIActionStatus CR6BehTreeInstance::StopAIAction( CAIAction* actionToStop )
{
	EAIActionStatus status = actionToStop->GetStatus();
	if ( status == ACTION_InProgress )
	{
		status = actionToStop->Cancel( TXT("CR6BehTreeInstance::StopAIAction() called.") );
	}

	return status;
}


