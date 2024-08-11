/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "r4GlobalEventsScriptsDispatcher.h"

IMPLEMENT_ENGINE_CLASS( CR4GlobalEventsScriptsDispatcher );

RED_DEFINE_NAME( OnGlobalEventName );
RED_DEFINE_NAME( OnGlobalEventString );

CR4GlobalEventsScriptsDispatcher::CR4GlobalEventsScriptsDispatcher()
{}

CR4GlobalEventsScriptsDispatcher::~CR4GlobalEventsScriptsDispatcher()
{
	if ( m_registeredCategories.Size() > 0 && GGame != nullptr && GGame->GetGlobalEventsManager() != nullptr )
	{
		CGlobalEventsManager* globalEventsManager = GGame->GetGlobalEventsManager();
		TCategories::iterator itEnd = m_registeredCategories.End();
		for ( TCategories::iterator it = m_registeredCategories.Begin(); it != itEnd; ++it )
		{
			globalEventsManager->RemoveListener( *it, this );
		}
	}
}

void CR4GlobalEventsScriptsDispatcher::OnGlobalEvent( EGlobalEventCategory eventCategory, EGlobalEventType eventType, const SGlobalEventParam& param )
{
	if ( eventCategory == GEC_Tag )
	{
		CName name = param.Get< CName >();
		CallEvent( CNAME( OnGlobalEventName ), eventCategory, eventType, name );
	}
	else if ( eventCategory == GEC_Fact )
	{
		String s = param.Get< String >();
		CallEvent( CNAME( OnGlobalEventString ), eventCategory, eventType, s );
	}
	// do not send other categories of events
}

void CR4GlobalEventsScriptsDispatcher::funcRegisterForCategoryFilterName( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( EGlobalEventCategory, eventCategory, GEC_Empty );
	GET_PARAMETER( CName, filter, CName::NONE );
	FINISH_PARAMETERS;

	Bool res = false;
	if ( eventCategory != GEC_Empty && filter != CName::NONE )
	{
		res = RegisterForCategory( eventCategory, filter );
	}

	RETURN_BOOL( res );
}

void CR4GlobalEventsScriptsDispatcher::funcRegisterForCategoryFilterNameArray( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( EGlobalEventCategory, eventCategory, GEC_Empty );
	GET_PARAMETER( TDynArray< CName >, filter, TDynArray< CName >() );
	FINISH_PARAMETERS;

	Bool res = false;
	if ( eventCategory != GEC_Empty && filter.Size() > 0 )
	{
		res = RegisterForCategory( eventCategory, filter );
	}

	RETURN_BOOL( res );
}

void CR4GlobalEventsScriptsDispatcher::funcRegisterForCategoryFilterString( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( EGlobalEventCategory, eventCategory, GEC_Empty );
	GET_PARAMETER( String, filter, String::EMPTY );
	FINISH_PARAMETERS;

	Bool res = false;
	if ( eventCategory != GEC_Empty && filter != String::EMPTY )
	{
		res = RegisterForCategory( eventCategory, filter );
	}

	RETURN_BOOL( res );
}

void CR4GlobalEventsScriptsDispatcher::funcRegisterForCategoryFilterStringArray( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( EGlobalEventCategory, eventCategory, GEC_Empty );
	GET_PARAMETER( TDynArray< String >, filter, TDynArray< String >() );
	FINISH_PARAMETERS;

	Bool res = false;
	if ( eventCategory != GEC_Empty && filter.Size() > 0 )
	{
		res = RegisterForCategory( eventCategory, filter );
	}

	RETURN_BOOL( res );
}

void CR4GlobalEventsScriptsDispatcher::funcUnregisterFromCategoryFilterName( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( EGlobalEventCategory, eventCategory, GEC_Empty );
	GET_PARAMETER( CName, filter, CName::NONE );
	FINISH_PARAMETERS;

	Bool res = false;
	if ( eventCategory != GEC_Empty && filter != CName::NONE )
	{
		res = UnregisterFromCategory( eventCategory, filter );
	}

	RETURN_BOOL( res );
}

void CR4GlobalEventsScriptsDispatcher::funcUnregisterFromCategoryFilterNameArray( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( EGlobalEventCategory, eventCategory, GEC_Empty );
	GET_PARAMETER( TDynArray< CName >, filter, TDynArray< CName >() );
	FINISH_PARAMETERS;

	Bool res = false;
	if ( eventCategory != GEC_Empty && filter.Size() > 0 )
	{
		res = UnregisterFromCategory( eventCategory, filter );
	}

	RETURN_BOOL( res );
}

void CR4GlobalEventsScriptsDispatcher::funcUnregisterFromCategoryFilterString( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( EGlobalEventCategory, eventCategory, GEC_Empty );
	GET_PARAMETER( String, filter, String::EMPTY );
	FINISH_PARAMETERS;

	Bool res = false;
	if ( eventCategory != GEC_Empty && filter != String::EMPTY )
	{
		res = UnregisterFromCategory( eventCategory, filter );
	}

	RETURN_BOOL( res );
}

void CR4GlobalEventsScriptsDispatcher::funcUnregisterFromCategoryFilterStringArray( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( EGlobalEventCategory, eventCategory, GEC_Empty );
	GET_PARAMETER( TDynArray< String >, filter, TDynArray< String >() );
	FINISH_PARAMETERS;

	Bool res = false;
	if ( eventCategory != GEC_Empty && filter.Size() > 0 )
	{
		res = UnregisterFromCategory( eventCategory, filter );
	}

	RETURN_BOOL( res );
}

void CR4GlobalEventsScriptsDispatcher::funcAddFilterNameForCategory( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( EGlobalEventCategory, eventCategory, GEC_Empty );
	GET_PARAMETER( CName, filter, CName::NONE );
	FINISH_PARAMETERS;

	Bool res = false;
	if ( eventCategory != GEC_Empty && filter != CName::NONE )
	{
		res = AddFilterForCategory( eventCategory, filter );
	}

	RETURN_BOOL( res );
}

void CR4GlobalEventsScriptsDispatcher::funcAddFilterNameArrayForCategory( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( EGlobalEventCategory, eventCategory, GEC_Empty );
	GET_PARAMETER( TDynArray< CName >, filter, TDynArray< CName >() );
	FINISH_PARAMETERS;

	Bool res = false;
	if ( eventCategory != GEC_Empty && filter.Size() > 0 )
	{
		res = AddFilterForCategory( eventCategory, filter );
	}

	RETURN_BOOL( res );
}

void CR4GlobalEventsScriptsDispatcher::funcAddFilterStringForCategory( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( EGlobalEventCategory, eventCategory, GEC_Empty );
	GET_PARAMETER( String, filter, String::EMPTY );
	FINISH_PARAMETERS;

	Bool res = false;
	if ( eventCategory != GEC_Empty && filter != String::EMPTY )
	{
		res = AddFilterForCategory( eventCategory, filter );
	}

	RETURN_BOOL( res );
}

void CR4GlobalEventsScriptsDispatcher::funcAddFilterStringArrayForCategory( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( EGlobalEventCategory, eventCategory, GEC_Empty );
	GET_PARAMETER( TDynArray< String >, filter, TDynArray< String >() );
	FINISH_PARAMETERS;

	Bool res = false;
	if ( eventCategory != GEC_Empty && filter.Size() > 0 )
	{
		res = AddFilterForCategory( eventCategory, filter );
	}

	RETURN_BOOL( res );
}

void CR4GlobalEventsScriptsDispatcher::funcRemoveFilterNameFromCategory( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( EGlobalEventCategory, eventCategory, GEC_Empty );
	GET_PARAMETER( CName, filter, CName::NONE );
	FINISH_PARAMETERS;

	Bool res = false;
	if ( eventCategory != GEC_Empty && filter != CName::NONE )
	{
		res = RemoveFilterFromCategory( eventCategory, filter );
	}

	RETURN_BOOL( res );
}

void CR4GlobalEventsScriptsDispatcher::funcRemoveFilterNameArrayFromCategory( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( EGlobalEventCategory, eventCategory, GEC_Empty );
	GET_PARAMETER( TDynArray< CName >, filter, TDynArray< CName >() );
	FINISH_PARAMETERS;

	Bool res = false;
	if ( eventCategory != GEC_Empty && filter.Size() > 0 )
	{
		res = RemoveFilterFromCategory( eventCategory, filter );
	}

	RETURN_BOOL( res );
}

void CR4GlobalEventsScriptsDispatcher::funcRemoveFilterStringFromCategory( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( EGlobalEventCategory, eventCategory, GEC_Empty );
	GET_PARAMETER( String, filter, String::EMPTY );
	FINISH_PARAMETERS;

	Bool res = false;
	if ( eventCategory != GEC_Empty && filter != String::EMPTY )
	{
		res = RemoveFilterFromCategory( eventCategory, filter );
	}

	RETURN_BOOL( res );
}

void CR4GlobalEventsScriptsDispatcher::funcRemoveFilterStringArrayFromCategory( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( EGlobalEventCategory, eventCategory, GEC_Empty );
	GET_PARAMETER( TDynArray< String >, filter, TDynArray< String >() );
	FINISH_PARAMETERS;

	Bool res = false;
	if ( eventCategory != GEC_Empty && filter.Size() > 0 )
	{
		res = RemoveFilterFromCategory( eventCategory, filter );
	}

	RETURN_BOOL( res );
}