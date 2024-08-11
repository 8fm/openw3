/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../../common/engine/globalEventsManager.h"
#include "../../common/core/set.h"

class CR4GlobalEventsScriptsDispatcher : public CObject, public IGlobalEventsListener
{
	DECLARE_RTTI_SIMPLE_CLASS( CR4GlobalEventsScriptsDispatcher );

	typedef TSet< EGlobalEventCategory > TCategories;
	TCategories		m_registeredCategories;

public:

	CR4GlobalEventsScriptsDispatcher();
	~CR4GlobalEventsScriptsDispatcher();

	//! IGlobalEventsListener
	void OnGlobalEvent( EGlobalEventCategory eventCategory, EGlobalEventType eventType, const SGlobalEventParam& param ) override;
	
private:

	template < typename T >
	Bool RegisterForCategory( EGlobalEventCategory eventCategory, const T& filter );
	template < typename T >
	Bool UnregisterFromCategory( EGlobalEventCategory eventCategory, const T& filter );
	template < typename T >
	Bool AddFilterForCategory( EGlobalEventCategory eventCategory, const T& filter );
	template < typename T >
	Bool RemoveFilterFromCategory( EGlobalEventCategory eventCategory, const T& filter );

public:

	void funcRegisterForCategoryFilterName( CScriptStackFrame& stack, void* result );
	void funcRegisterForCategoryFilterNameArray( CScriptStackFrame& stack, void* result );
	void funcRegisterForCategoryFilterString( CScriptStackFrame& stack, void* result );
	void funcRegisterForCategoryFilterStringArray( CScriptStackFrame& stack, void* result );
	void funcUnregisterFromCategoryFilterName( CScriptStackFrame& stack, void* result );
	void funcUnregisterFromCategoryFilterNameArray( CScriptStackFrame& stack, void* result );
	void funcUnregisterFromCategoryFilterString( CScriptStackFrame& stack, void* result );
	void funcUnregisterFromCategoryFilterStringArray( CScriptStackFrame& stack, void* result );
	void funcAddFilterNameForCategory( CScriptStackFrame& stack, void* result );
	void funcAddFilterNameArrayForCategory( CScriptStackFrame& stack, void* result );
	void funcAddFilterStringForCategory( CScriptStackFrame& stack, void* result );
	void funcAddFilterStringArrayForCategory( CScriptStackFrame& stack, void* result );
	void funcRemoveFilterNameFromCategory( CScriptStackFrame& stack, void* result );
	void funcRemoveFilterNameArrayFromCategory( CScriptStackFrame& stack, void* result );
	void funcRemoveFilterStringFromCategory( CScriptStackFrame& stack, void* result );
	void funcRemoveFilterStringArrayFromCategory( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CR4GlobalEventsScriptsDispatcher );
	PARENT_CLASS( CObject );
	NATIVE_FUNCTION( "RegisterForCategoryFilterName", funcRegisterForCategoryFilterName );
	NATIVE_FUNCTION( "RegisterForCategoryFilterNameArray", funcRegisterForCategoryFilterNameArray );
	NATIVE_FUNCTION( "RegisterForCategoryFilterString", funcRegisterForCategoryFilterString );
	NATIVE_FUNCTION( "RegisterForCategoryFilterStringArray", funcRegisterForCategoryFilterStringArray );
	NATIVE_FUNCTION( "UnregisterFromCategoryFilterName", funcUnregisterFromCategoryFilterName );
	NATIVE_FUNCTION( "UnregisterFromCategoryFilterNameArray", funcUnregisterFromCategoryFilterNameArray );
	NATIVE_FUNCTION( "UnregisterFromCategoryFilterString", funcUnregisterFromCategoryFilterString );
	NATIVE_FUNCTION( "UnregisterFromCategoryFilterStringArray", funcUnregisterFromCategoryFilterStringArray );
	NATIVE_FUNCTION( "AddFilterNameForCategory", funcAddFilterNameForCategory );
	NATIVE_FUNCTION( "AddFilterNameArrayForCategory", funcAddFilterNameArrayForCategory );
	NATIVE_FUNCTION( "AddFilterStringForCategory", funcAddFilterStringForCategory );
	NATIVE_FUNCTION( "AddFilterStringArrayForCategory", funcAddFilterStringArrayForCategory );
	NATIVE_FUNCTION( "RemoveFilterNameFromCategory", funcRemoveFilterNameFromCategory );
	NATIVE_FUNCTION( "RemoveFilterNameArrayFromCategory", funcRemoveFilterNameArrayFromCategory );
	NATIVE_FUNCTION( "RemoveFilterStringFromCategory", funcRemoveFilterStringFromCategory );
	NATIVE_FUNCTION( "RemoveFilterStringArrayFromCategory", funcRemoveFilterStringArrayFromCategory );
END_CLASS_RTTI();

template < typename T >
Bool CR4GlobalEventsScriptsDispatcher::RegisterForCategory( EGlobalEventCategory eventCategory, const T& filter )
{
	if ( GGame == nullptr || GGame->GetGlobalEventsManager() == nullptr )
	{
		return false;
	}
	if ( m_registeredCategories.Exist( eventCategory ) )
	{
		return false;
	}
	GGame->GetGlobalEventsManager()->AddFilteredListener( eventCategory, this, filter );
	m_registeredCategories.Insert( eventCategory );
	return true;
}

template < typename T >
Bool CR4GlobalEventsScriptsDispatcher::UnregisterFromCategory( EGlobalEventCategory eventCategory, const T& filter )
{
	if ( GGame == nullptr || GGame->GetGlobalEventsManager() == nullptr )
	{
		return false;
	}
	if ( !m_registeredCategories.Exist( eventCategory ) )
	{
		return false;
	}
	GGame->GetGlobalEventsManager()->RemoveFilteredListener( eventCategory, this, filter );
	m_registeredCategories.Erase( eventCategory );
	return true;
}

template < typename T >
Bool CR4GlobalEventsScriptsDispatcher::AddFilterForCategory( EGlobalEventCategory eventCategory, const T& filter )
{
	if ( GGame == nullptr || GGame->GetGlobalEventsManager() == nullptr )
	{
		return false;
	}
	if ( !m_registeredCategories.Exist( eventCategory ) )
	{
		return RegisterForCategory( eventCategory, filter );
	}
	return GGame->GetGlobalEventsManager()->AddFilterForListener( eventCategory, this, filter );
}

template < typename T >
Bool CR4GlobalEventsScriptsDispatcher::RemoveFilterFromCategory( EGlobalEventCategory eventCategory, const T& filter )
{
	if ( GGame == nullptr || GGame->GetGlobalEventsManager() == nullptr )
	{
		return false;
	}
	if ( !m_registeredCategories.Exist( eventCategory ) )
	{
		return false;
	}
	return GGame->GetGlobalEventsManager()->RemoveFilterFromListener( eventCategory, this, filter );
}
