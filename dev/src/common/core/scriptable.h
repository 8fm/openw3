/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "serializable.h"
#include "functionBuilder.h"
#include "referencable.h"
#include "handleMap.h"
#include "scriptableStateMachine.h"

// Event call result
enum EEventCallResult
{
	CR_EventNotFound	= -1,
	CR_EventFailed		= 0,
	CR_EventSucceeded	= 1
};

template<>
struct TClassExtractor<1>
{
	template< class T >
	RED_INLINE static CClass* ExtractClass( const T* object );
};

/// Basic class for all thing that you want to add scripting support to
class IScriptable : public ISerializable, public CScriptableStateMachine
{
public:
	// use the dynamic class extraction - a little bit hacky but saves a one virtual call
	// template specialized with 0 uses static CClass extraction (T::StaticClass())
	// we override the class extractor for _all_ derived classes to use the obj->m_class
	enum ClassExtraction
	{
		ClassExtratorType = 1
	};

	DECLARE_RTTI_SIMPLE_CLASS( IScriptable );

private:
	CClass*					m_class;						//!< Class of object
	void*					m_scriptData;					//!< Scripting data (properties)
	mutable SActiveData*	m_scriptMachineData;			//!< Script matchine data (created on request when needed)

public:
	IScriptable();
	virtual ~IScriptable();

	// Get the script data buffer
	RED_INLINE void* GetScriptPropertyData() { return m_scriptData; }

	// Get the script data buffer
	RED_INLINE const void* GetScriptPropertyData() const { return m_scriptData; }

	// Get the local (overridable) class, non virtual method, fastest
	RED_INLINE CClass* GetLocalClass() const { return m_class; }

public:
	// Find virtual function with given name in this object
	virtual const CFunction* FindFunction( IScriptable*& context, CName functionName, const Bool allowStates = true ) const;

	// Get object name for script debugger, when false is returned then get GetFriendlyName is used
	virtual Bool GetScriptDebuggerName( String& /*debugName*/ ) const { return false; }

	// Get object friendly name (used by ToString and many debug functions)
	virtual String GetFriendlyName() const;

public:
	// Serialization support (for GC mostly right now)
	virtual void OnSerialize( IFile& file );

	// Called just before a capture of this object is taken prior to script compilation
	virtual void OnScriptPreCaptureSnapshot();

	// Called just after a capture of this object is taken prior to script compilation
	virtual void OnScriptPostCaptureSnapshot();

	// Called when scripts have been successfully reloaded
	virtual void OnScriptReloaded();

	//! Collect all IScriptable objects (slow, only for script reloading support)
	static void CollectAllScriptableObjects( TDynArray< THandle< IScriptable > >& outScriptables );
	static void CollectAllDefaultScriptableObjects( TDynArray< THandle< IScriptable > >& outScriptables );

	//! AutoBinding support
	virtual Bool IsAutoBindingPropertyTypeSupported( const IRTTIType* type ) const;
	virtual Bool ValidateAutoBindProperty( const CProperty* autoBindProperty, CName bindingName ) const;
	virtual Bool ResolveAutoBindProperty( const CProperty* autoBindProperty, CName bindingName, void* resultData ) const;

public:

	// Hiding ISerializable version. We can bypass virtual function call to GetClass here !

	//! RTTI class check
	Bool IsA( const CClass* rttiClass ) const
	{
		return GetLocalClass()->IsA( rttiClass );
	}

	//! RTTI exact class check
	Bool IsExactlyA( const CClass* rttiClass ) const
	{
		return GetLocalClass() == rttiClass;
	}

	//! RTTI class check
	template< class T >
	Bool IsA() const;

	//! RTTI class check
	template< class T >
	Bool IsExactlyA() const;

public:
	//! Returns true if event with given name exists
	Bool FindEvent( CName functionName ) const;

	//! Call event, 0 params
	RED_INLINE EEventCallResult CallEvent( CName eventName );

	//! Call event, 1 param
	template< class T0 > RED_INLINE EEventCallResult CallEvent( CName eventName, const T0& data0 );

	//! Call event, 2 params
	template< class T0, class T1 > RED_INLINE EEventCallResult CallEvent( CName eventName, const T0& data0, const T1& data1 );

	//! Call event, 3 params
	template< class T0, class T1, class T2 > RED_INLINE EEventCallResult CallEvent( CName eventName, const T0& data0, const T1& data1, const T2& data2 );

	//! Call event, 4 params
	template< class T0, class T1, class T2, class T3 > RED_INLINE EEventCallResult CallEvent( CName eventName, const T0& data0, const T1& data1, const T2& data2, const T3& data3 );

	//! Call event, 5 params
	template< class T0, class T1, class T2, class T3, class T4 > RED_INLINE EEventCallResult CallEvent( CName eventName, const T0& data0, const T1& data1, const T2& data2, const T3& data3, const T4& data4 );

	//! Call event, 6 params
	template< class T0, class T1, class T2, class T3, class T4, class T5 > RED_INLINE EEventCallResult CallEvent( CName eventName, const T0& data0, const T1& data1, const T2& data2, const T3& data3, const T4& data4, const T5& data5 );

	//! Call event, 7 params
	template< class T0, class T1, class T2, class T3, class T4, class T5, class T6 > RED_INLINE EEventCallResult CallEvent( CName eventName, const T0& data0, const T1& data1, const T2& data2, const T3& data3, const T4& data4, const T5& data5, const T6& data6 );

	//! Call event, 8 params
	template< class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7 > RED_INLINE EEventCallResult CallEvent( CName eventName, const T0& data0, const T1& data1, const T2& data2, const T3& data3, const T4& data4, const T5& data5, const T6& data6, const T7& data7 );

	//! Call event, 9 params
	template< class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8 > RED_INLINE EEventCallResult CallEvent( CName eventName, const T0& data0, const T1& data1, const T2& data2, const T3& data3, const T4& data4, const T5& data5, const T6& data6, const T7& data7, const T8& data8 );

	//! Call event, 10 params
	template< class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9 > RED_INLINE EEventCallResult CallEvent( CName eventName, const T0& data0, const T1& data1, const T2& data2, const T3& data3, const T4& data4, const T5& data5, const T6& data6, const T7& data7, const T8& data8, const T9& data9 );

private:
	void funcToString( CScriptStackFrame& stack, void* result );
	void funcGetClass( CScriptStackFrame& stack, void* result );
	void funcIsA( CScriptStackFrame& stack, void* result );
	void funcIsExactlyA( CScriptStackFrame& stack, void* result );

private:
	//! Release property buffer with script properties, state machine data is not deleted
	void ReleaseScriptPropertiesBuffer();

	//! Recreate buffer with script properties
	void CreateScriptPropertiesBuffer();

	//! CScriptableStateMachine interface
	virtual IScriptable* GetStateMachineOwner() const;
	virtual SActiveData* GetStateMachineData( const Bool createIfNotFound = false ) const;

	friend class CClass;
	friend class CScriptSnapshot;
	friend class CRTTISystem;
	friend class CScriptCompiler;
};

// class extraction for ISerializable uses the direct value
template< class T >
RED_INLINE CClass* TClassExtractor<1>::ExtractClass( const T* object )
{
	return object->GetLocalClass();
}	

BEGIN_CLASS_RTTI( IScriptable );
	PARENT_CLASS( ISerializable );
	// General part
	NATIVE_FUNCTION( "ToString", funcToString );
	NATIVE_FUNCTION( "GetClass", funcGetClass );
	NATIVE_FUNCTION( "IsA", funcIsA );
	NATIVE_FUNCTION( "IsExactlyA", funcIsExactlyA );

	// State machine part - it's glued from CScriptedStateMachine
	NATIVE_FUNCTION( "PushState", CScriptableStateMachine::funcPushState );
	NATIVE_FUNCTION( "IsInState", CScriptableStateMachine::funcIsInState );
	NATIVE_FUNCTION( "PopState", CScriptableStateMachine::funcPopState );
	NATIVE_FUNCTION( "GotoState", CScriptableStateMachine::funcGotoState );
	NATIVE_FUNCTION( "GotoStateAuto", CScriptableStateMachine::funcGotoStateAuto );
	NATIVE_FUNCTION( "LogStates", CScriptableStateMachine::funcLogStates );
	NATIVE_FUNCTION( "GetState", CScriptableStateMachine::funcGetState );
	NATIVE_FUNCTION( "GetCurrentState", CScriptableStateMachine::funcGetCurrentState );
	NATIVE_FUNCTION( "GetCurrentStateName", CScriptableStateMachine::funcGetCurrentStateName );
	NATIVE_FUNCTION( "ChangeState", CScriptableStateMachine::funcChangeState );
	NATIVE_FUNCTION( "StopState", CScriptableStateMachine::funcStop );
	NATIVE_FUNCTION( "LockEntryFunction", CScriptableStateMachine::funcLockEntryFunction );
	NATIVE_FUNCTION( "SetCleanupFunction", CScriptableStateMachine::funcSetCleanupFunction );
	NATIVE_FUNCTION( "ClearCleanupFunction", CScriptableStateMachine::funcClearCleanupFunction );
	NATIVE_FUNCTION( "DebugDumpEntryFunctionCalls", CScriptableStateMachine::funcDebugDumpEntryFunctionCalls );
END_CLASS_RTTI();

// inlined part of the scriptable class (function calling templates)
#include "scriptable.inl.h"
