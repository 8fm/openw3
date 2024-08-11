#pragma once

#include "behTreeNode.h"

////////////////////////////////////////////////////////////////////////
class CBehTreeScriptedInstance : CScriptThread::IListener
{
protected:
	
	TDynArray< SBehTreeEvenListeningData >	m_listenedEventList;
	THandle< IBehTreeTask >					m_taskInstance;
	CScriptThread*							m_thread;
	CPropertyDataBuffer						m_threadReturnValue;
	Uint16									m_taskFlags;
	Uint16									m_scriptStateFlags : 15;
	Bool									m_autoRunMain : 1;
public:

	enum eUpdateResult
	{
		U_FAILURE = IBehTreeNodeInstance::BTTO_FAILED,
		U_SUCCESS = IBehTreeNodeInstance::BTTO_SUCCESS,
		U_CONTINUE
	};

	enum eScriptStateFlags
	{
		SS_DEFAULT					= 0,
		SS_RUNNING					= 0x0001,
		SS_RETURNED					= 0x0002,
		SS_FAILED					= 0x0004,
		SS_NOABORT					= 0x0008,
		SS_RUNMAIN					= 0x0010,
	};

	CBehTreeScriptedInstance( CBehTreeSpawnContext& context, IBehTreeTaskDefinition* taskDefinition, IBehTreeNodeInstance* nodeInstance, Uint16& flags, Bool runMainOnActivation );
	virtual ~CBehTreeScriptedInstance();

	////////////////////////////////////////////////////////////////////
	// Typical forward
	void ScriptInitialize( const IBehTreeTaskDefinition* taskDefinition, CBehTreeSpawnContext& context );
	eUpdateResult ScriptUpdate();
	Bool ScriptActivate();
	void ScriptDeactivate();
	Bool ScriptIsAvailable();
	void ScriptOnCompletion( IBehTreeNodeInstance::eTaskOutcome e );
	Int32 ScriptEvaluate( Int32 defaultPriority );
	Bool ScriptOnEvent( CBehTreeEvent& e );
	Bool ScriptOnListenedEvent( CBehTreeEvent& e );
	void ScriptOnDestruction();
	void ScriptOnEventListenerAdded( const SBehTreeEvenListeningData & event );

	////////////////////////////////////////////////////////////////////
	// CScriptThread::IListener interface
	void OnScriptThreadKilled( CScriptThread * thread, Bool finished ) override;
	String GetDebugName() const override;
	void ReviveMain();

	void ScriptCompletedItself( Bool success );
	virtual CBehTreeInstance* ScriptGetOwner() const = 0;
	virtual IBehTreeNodeInstance* ScriptGetNode() = 0;
};

////////////////////////////////////////////////////////////////////////
class IBehTreeTask : public IScriptable
{
	DECLARE_RTTI_SIMPLE_CLASS( IBehTreeTask );	

protected:
	struct ExtractedVar
	{
		CName						m_variableName;
		CVariant					m_variant;

		Bool				operator<( const ExtractedVar& v ) const	{ return m_variableName < v.m_variableName; }
	};
	typedef TSortedArray< ExtractedVar > ExtractedVars;

	CBehTreeScriptedInstance*			m_instance;
	CBehTreeEvent*						m_event;
	ExtractedVars						m_extractedVars;
	Bool								m_isActive;

public:
	enum eTaskFlags
	{
		FLAG_HAS_MAIN						= 0x0001,
		FLAG_HAS_ON_ACTIVATE				= 0x0002,
		FLAG_HAS_ON_DEACTIVATE				= 0x0004,
		FLAG_HAS_ON_COMPLETION				= 0x0008,
		FLAG_HAS_ON_ANIM_EVENT				= 0x0010,
		FLAG_HAS_ON_GAMEPLAY_EVENT			= 0x0020,
		FLAG_HAS_IS_AVAILABLE				= 0x0040,
		FLAG_HAS_EVALUATE					= 0x0080,
		FLAG_HAS_ON_LISTENED_GAMEPLAY_EVENT	= 0x0200,
		FLAG_HAS_INITIALIZE					= 0x0400,
		FLAGS_UNINITIALIZED					= 0xffff
	};

	IBehTreeTask();
	~IBehTreeTask();

	Uint16 ComputeTaskFlags() const;

	// Get task name
	void GetTaskName( String& str ) const;

	//! Set instance data - for active running tasks
	void SetInstanceData( CBehTreeScriptedInstance* data )				{ m_instance = data; }

	//! Is behavior that task represents currently active?
	void SetActive( Bool b )											{ m_isActive = b; }

	CBehTreeInstance* GetInstance() const								{ return m_instance->ScriptGetOwner(); }

	//! Event handling
	void SetBehTreeEvent( CBehTreeEvent& e )							{ m_event = &e; }
	void ClearBehTreeEvent()											{ m_event = NULL; }
	CName& GetEventParamCName( CName& defaultVal );
	Int32 GetEventParamInt( Int32 defaultVal );
	Float GetEventParamFloat( Float defaultVal );
	THandle< IScriptable > GetEventParamCObject();

protected:
	//void funcSetResultValue( CScriptStackFrame& stack, void* result );
	void funcGetActor( CScriptStackFrame& stack, void* result );
	void funcGetNPC( CScriptStackFrame& stack, void* result );
	void funcGetLocalTime( CScriptStackFrame& stack, void* result );
	void funcSetNamedTarget( CScriptStackFrame& stack, void* result );
	void funcGetNamedTarget( CScriptStackFrame& stack, void* result );
	void funcGetActionTarget( CScriptStackFrame& stack, void* result );
	void funcSetActionTarget( CScriptStackFrame& stack, void* result );
	void funcGetCombatTarget( CScriptStackFrame& stack, void* result );
	void funcSetCombatTarget( CScriptStackFrame& stack, void* result );
	void funcRunMain( CScriptStackFrame& stack, void* result );
	void funcComplete( CScriptStackFrame& stack, void* result );
	void funcSetEventRetvalCName( CScriptStackFrame& stack, void* result );
	void funcSetEventRetvalFloat( CScriptStackFrame& stack, void* result );
	void funcSetEventRetvalInt( CScriptStackFrame& stack, void* result );
	void funcGetEventParamBaseDamage( CScriptStackFrame& stack, void* result );
	void funcGetEventParamObject( CScriptStackFrame& stack, void* result );
	void funcGetEventParamCName( CScriptStackFrame& stack, void* result );
	void funcGetEventParamFloat( CScriptStackFrame& stack, void* result );
	void funcGetEventParamInt( CScriptStackFrame& stack, void* result );
	void funcUnregisterFromAnimEvent( CScriptStackFrame& stack, void* result );
	void funcUnregisterFromGameplayEvent( CScriptStackFrame& stack, void* result );
	void funcSetIsInCombat( CScriptStackFrame& stack, void* result );
	void funcSetCustomTarget( CScriptStackFrame& stack, void* result );
	void funcGetCustomTarget( CScriptStackFrame& stack, void* result );
	void funcGetReactionEventInvoker( CScriptStackFrame& stack, void* result );
	void funcRequestStorageItem( CScriptStackFrame& stack, void* result );
	void funcFindStorageItem( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( IBehTreeTask );
	PARENT_CLASS( IScriptable );
	PROPERTY( m_isActive );
	NATIVE_FUNCTION( "GetActor", funcGetActor );
	NATIVE_FUNCTION( "GetNPC", funcGetNPC );
	NATIVE_FUNCTION( "GetLocalTime", funcGetLocalTime );
	NATIVE_FUNCTION( "SetNamedTarget", funcSetNamedTarget );
	NATIVE_FUNCTION( "GetNamedTarget", funcGetNamedTarget );
	NATIVE_FUNCTION( "GetActionTarget", funcGetActionTarget );
	NATIVE_FUNCTION( "SetActionTarget", funcSetActionTarget );
	NATIVE_FUNCTION( "GetCombatTarget", funcGetCombatTarget );
	NATIVE_FUNCTION( "SetCombatTarget", funcSetCombatTarget );
	NATIVE_FUNCTION( "RunMain", funcRunMain );
	NATIVE_FUNCTION( "Complete", funcComplete );
	NATIVE_FUNCTION( "SetEventRetvalCName", funcSetEventRetvalCName );
	NATIVE_FUNCTION( "SetEventRetvalFloat", funcSetEventRetvalFloat );
	NATIVE_FUNCTION( "SetEventRetvalInt", funcSetEventRetvalInt );
	NATIVE_FUNCTION( "GetEventParamCName", funcGetEventParamCName );
	NATIVE_FUNCTION( "GetEventParamBaseDamage", funcGetEventParamBaseDamage );
	NATIVE_FUNCTION( "GetEventParamFloat", funcGetEventParamFloat );
	NATIVE_FUNCTION( "GetEventParamInt", funcGetEventParamInt );
	NATIVE_FUNCTION( "GetEventParamObject", funcGetEventParamObject );
	NATIVE_FUNCTION( "UnregisterFromAnimEvent", funcUnregisterFromAnimEvent );
	NATIVE_FUNCTION( "UnregisterFromGameplayEvent", funcUnregisterFromGameplayEvent );
	NATIVE_FUNCTION( "SetIsInCombat", funcSetIsInCombat );
	NATIVE_FUNCTION( "SetCustomTarget", funcSetCustomTarget );
	NATIVE_FUNCTION( "GetCustomTarget", funcGetCustomTarget );
	NATIVE_FUNCTION( "GetReactionEventInvoker", funcGetReactionEventInvoker );
	NATIVE_FUNCTION( "RequestStorageItem", funcRequestStorageItem );
	NATIVE_FUNCTION( "FindStorageItem", funcFindStorageItem );
END_CLASS_RTTI();

////////////////////////////////////////////////////////////////////////
class IBehTreeObjectDefinition : public IScriptable
{
	DECLARE_RTTI_SIMPLE_CLASS( IBehTreeObjectDefinition );
protected:
	struct PropertyTranslation
	{
		IProperty*					m_propertyDefinition;
		IProperty*					m_propertyInstance;
	};
	typedef TDynArray< PropertyTranslation > DefinitionTranslation;

	mutable CBehTreeSpawnContext*	m_context;							// its very hacky and ugly, but we dont want to export all spawning context to scripts

	CName							m_instanceClass;
	Bool							m_hasOnSpawn;
	DefinitionTranslation			m_translation;


public:
	IBehTreeObjectDefinition()
		: m_hasOnSpawn( false )											{ EnableReferenceCounting( true ); }

	Bool					OnPropertyTypeMismatch( CName propertyName, IProperty* existingProperty, const CVariant& readValue ) override;
	void					OnScriptReloaded() override;
	CClass*					GetInstancedClass();

	Bool					HasOnSpawn() const							{ return m_hasOnSpawn; }

	void					OnPostLoad() override;

#ifndef NO_EDITOR
	void					OnCreatedInEditor() override;

	void					Refactor();
	static void				RefactorAll();
#endif

protected:
	virtual Bool			IsPropertyTranslated( CProperty* property ) const;
	void					InitializeTranslation();
	void					Translate( IScriptable* objPtr, CBehTreeSpawnContext& context ) const;

	THandle< IScriptable >	SpawnInstanceBase( CBehTreeSpawnContext& context ) const;

private:
	void					funcGetValFloat( CScriptStackFrame& stack, void* result );
	void					funcGetValInt( CScriptStackFrame& stack, void* result );
	void					funcGetValEnum( CScriptStackFrame& stack, void* result );
	void					funcGetValString( CScriptStackFrame& stack, void* result );	
	void					funcGetValCName( CScriptStackFrame& stack, void* result );
	void					funcGetValBool( CScriptStackFrame& stack, void* result );
	void					funcGetObjectByVar( CScriptStackFrame& stack, void* result );
	void					funcGetAIParametersByClassName( CScriptStackFrame& stack, void* result );

	void					funcSetValFloat( CScriptStackFrame& stack, void* result );
	void					funcSetValInt( CScriptStackFrame& stack, void* result );
	void					funcSetValString( CScriptStackFrame& stack, void* result );
	void					funcSetValCName( CScriptStackFrame& stack, void* result );
	void					funcSetValBool( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( IBehTreeObjectDefinition );
	PARENT_CLASS( IScriptable );
	PROPERTY( m_instanceClass );
	NATIVE_FUNCTION( "SetValFloat", funcSetValFloat );
	NATIVE_FUNCTION( "SetValInt", funcSetValInt );
	NATIVE_FUNCTION( "SetValString", funcSetValString );
	NATIVE_FUNCTION( "SetValCName", funcSetValCName );
	NATIVE_FUNCTION( "SetValBool", funcSetValBool );
	NATIVE_FUNCTION( "GetValFloat", funcGetValFloat );
	NATIVE_FUNCTION( "GetValInt", funcGetValInt );
	NATIVE_FUNCTION( "GetValEnum", funcGetValEnum );
	NATIVE_FUNCTION( "GetValString", funcGetValString );
	NATIVE_FUNCTION( "GetValCName", funcGetValCName );
	NATIVE_FUNCTION( "GetValBool", funcGetValBool );
	NATIVE_FUNCTION( "GetObjectByVar", funcGetObjectByVar );
	NATIVE_FUNCTION( "GetAIParametersByClassName", funcGetAIParametersByClassName );
END_CLASS_RTTI();

////////////////////////////////////////////////////////////////////////
class IBehTreeTaskDefinition : public IBehTreeObjectDefinition
{
	DECLARE_RTTI_SIMPLE_CLASS( IBehTreeTaskDefinition );
protected:
	TDynArray< CName >						m_listenToGameplayEvents;
	TDynArray< CName >						m_listenToAnimEvents;
private:
	mutable IBehTreeNodeInstance*			m_nodeInstance;
	mutable CBehTreeScriptedInstance*		m_nodeScriptInstance;

protected:
	virtual Bool			IsPropertyTranslated( CProperty* property ) const;

public:
	IBehTreeTaskDefinition()
		: m_nodeInstance( NULL )										{}

	void					Initialize();
	// Get task name
	void					GetTaskName( String& outName ) const;
	static void				GetTaskNameStatic( CClass* classId, String& outName );

	THandle< IBehTreeTask > SpawnInstance( CBehTreeSpawnContext& context, IBehTreeNodeInstance* instance, CBehTreeScriptedInstance* scriptInstance ) const;
	void					OnSpawn( CBehTreeSpawnContext& context, CBehTreeScriptedInstance* scriptedInstance, THandle< IBehTreeTask >& task ) const;

	void					OnSerialize( IFile &file ) override;

private:
	void					funcListenToAnimEvent( CScriptStackFrame& stack, void* result );
	void					funcListenToGameplayEvent( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( IBehTreeTaskDefinition )
	PARENT_CLASS( IBehTreeObjectDefinition )
	PROPERTY( m_listenToGameplayEvents )
	PROPERTY( m_listenToAnimEvents )
	NATIVE_FUNCTION( "ListenToAnimEvent", funcListenToAnimEvent )
	NATIVE_FUNCTION( "ListenToGameplayEvent", funcListenToGameplayEvent )
END_CLASS_RTTI()

////////////////////////////////////////////////////////////////////////
class IBehTreeConditionalTaskDefinition : public IBehTreeTaskDefinition
{
	DECLARE_RTTI_SIMPLE_CLASS( IBehTreeConditionalTaskDefinition );
public:
	IBehTreeConditionalTaskDefinition() {}
};

BEGIN_CLASS_RTTI( IBehTreeConditionalTaskDefinition );
	PARENT_CLASS( IBehTreeTaskDefinition );
END_CLASS_RTTI();
