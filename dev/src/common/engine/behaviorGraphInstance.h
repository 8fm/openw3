/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "behaviorVariable.h"
#include "behaviorGraph.h"

struct SBehaviorSlotSetup;
class IBehaviorGraphSlotInterface;
class ISlotAnimationListener;
class CBehaviorGraphTopLevelNode;
class CBehaviorManualSlotInterface;
class CBehaviorMixerSlotInterface;
class CBehaviorPointCloudLookAtInterface;
class IAnimationConstraint;
class CBehaviorGraphNode;
class CSkeletalAnimationSetEntry;
class CNode;
class IPoseSlotListener;
class CRenderFrame;
struct SBehaviorUpdateContext;
struct SBehaviorSampleContext;
struct CSyncInfo;

enum EBlendType : CEnum::TValueType;

class CBehaviorGraphDelayedEvent
{
	// Handle function prototype. 'const' is optional.
	void Handle( CBehaviorGraphInstance* /*instance*/ ) const							{ ASSERT( false ); ASSUME( false ); }
};

class CBehaviorGraphEventHandler
{
public:
	virtual void HandleDelayedEvents( CBehaviorGraphInstance* instance ) = 0;

	virtual ~CBehaviorGraphEventHandler()											{}
};

class CBehaviorSyncInfo
{
public:
	CName	m_instanceName;
	CName	m_id;
	CName	m_animation;
	Float	m_time;

	CBehaviorSyncInfo() : m_time( 0.f ) {}

	Bool IsOk() const
	{
		return m_id != CName::NONE;
	}
};

// Synchronization tags. For more info about synchronization, check CBehaviorGraphStateMachineNode's GetOutboundSyncTags and ApplyInboundSyncTags
struct SBehaviorSyncTags
{
public:
	TStaticArray< CName, 32 > m_syncTags;

	void Add( const CName& tag ) { m_syncTags.PushBack( tag ); }

	Bool DoesContainAnyTags() const { return ! m_syncTags.Empty(); }
};

// variables storage for syncing
struct SBehaviorGraphInstanceStoredVariable
{
	// TODO replace to Names as it will be enough to find graph
	const CBehaviorGraph* m_graph;
	CName m_name;
	Float m_floatValue;
	Vector m_vectorValue;

	SBehaviorGraphInstanceStoredVariable()
		:	m_graph( NULL )
	{}

	SBehaviorGraphInstanceStoredVariable( const CBehaviorGraph* _graph, CName _name, Float _value )
		:	m_graph( _graph )
		,	m_name( _name )
		,	m_floatValue( _value )
	{}

	SBehaviorGraphInstanceStoredVariable( const CBehaviorGraph* _graph, CName _name, const Vector& _value )
		:	m_graph( _graph )
		,	m_name( _name )
		,	m_vectorValue( _value )
	{}
};

// temporary variable storage, it has to be fast, that's why there's 256 size assumed
struct SBehaviorGraphInstanceStoredVariables
{
	static const Int32 NUM_VARIABLES = 256;
	Int32 m_floatVariablesNum;
	Int32 m_vectorVariablesNum;
	Int32 m_internalFloatVariablesNum;
	Int32 m_internalVectorVariablesNum;
	SBehaviorGraphInstanceStoredVariable m_floatVariables[NUM_VARIABLES];
	SBehaviorGraphInstanceStoredVariable m_vectorVariables[NUM_VARIABLES];
	SBehaviorGraphInstanceStoredVariable m_internalFloatVariables[NUM_VARIABLES];
	SBehaviorGraphInstanceStoredVariable m_internalVectorVariables[NUM_VARIABLES];

	SBehaviorGraphInstanceStoredVariables();

	void AddFloatVariable( const CBehaviorGraph* _graph, CName _name, Float _value );
	void AddVectorVariable( const CBehaviorGraph* _graph, CName _name, const Vector& _value );
	void AddInternalFloatVariable( const CBehaviorGraph* _graph, CName _name, Float _value );
	void AddInternalVectorVariable( const CBehaviorGraph* _graph, CName _name, const Vector& _value );
};

class IBehaviorGraphInstanceEditorListener
{
public:
	virtual Bool CanBeRelinked() const { return false; }
	virtual void RequestToRelink() {}
	virtual void Relink( CBehaviorGraphInstance* /*newInstance*/ ) {}

public:
	virtual void OnPaused( Bool /*paused*/ ) {}
	virtual void OnReset() {}

	virtual void OnUnbind() {}

	virtual void OnActivated() {}
	virtual void OnDeactivated() {}

	virtual void OnPreUpdateInstance( Float& /*dt*/ ) {}
	virtual void OnPostUpdateInstance( Float /*dt*/ ) {}

	virtual void OnPreSampleInstance() {}
	virtual void OnPostSampleInstance( const SBehaviorGraphOutput& /*pose*/ ) {}

	virtual void OnGenerateEvent( const CName& /*event*/ ) {}
	virtual void OnGenerateForceEvent( const CName& /*event*/ ) {}

	virtual void OnSetVectorValue( CName /*name*/, const Vector& /*value*/ ) {}
	virtual void OnSetFloatValue( CName /*name*/, const Float /*value*/ ) {}

	virtual void OnResetFloatValue( CName /*name*/ ) {}
	virtual void OnResetVectorValue( CName /*name*/ ) {}

	virtual void OnSetInternalVectorValue( const CName /*name*/, const Vector& /*value*/ ) {}
	virtual void OnSetInternalFloatValue( const CName /*name*/, const Float /*value*/ ) {}

	virtual void OnResetInternalFloatValue( CName /*name*/ ) {}
	virtual void OnResetInternalVectorValue( CName /*name*/ ) {}

	virtual void OnSetInternalFloatValue( const String& /*name*/, Int32 /*id*/, const Float /*value*/ ) {}
	virtual void OnSetInternalFloatValue( Int32 /*id*/, const Float /*value*/ ) {}

	virtual void OnPlaySlotAnimation( const CBehaviorGraphNode* /*slot*/, const CName& /*animation*/, const SBehaviorSlotSetup* /*slotSetup*/ ) {}
	virtual void OnPlaySlotAnimation( const CBehaviorGraphNode* /*slot*/, const CSkeletalAnimationSetEntry* /*animation*/, const SBehaviorSlotSetup* /*slotSetup*/ ) {}
	virtual void OnStopSlotAnimation( const CBehaviorGraphNode* /*slot*/ ) {}

	virtual void OnActivateConstraint( const IAnimationConstraint* /*constraint*/ ) {}
	virtual void OnDeactivateConstraint( const IAnimationConstraint* /*constraint*/ ) {}

	virtual void OnSetSlotPose( const CBehaviorGraphNode* /*slot*/, Float /*blend*/, EBlendType /*type*/ ) {}
	virtual void OnResetSlotPose( const CBehaviorGraphNode* /*slot*/ ) {}

	virtual void OnEventProcessed( const CName& /*event*/ ) {}
	virtual void OnNotifyOfNodesActivation( const CName& /*name*/ ) {}
	virtual void OnNotifyOfNodesDeactivation( const CName& /*name*/ ) {}

	virtual void OnNodeUpdate( const CBehaviorGraphNode* /*node*/ ) {}
	virtual void OnNodeSample( const CBehaviorGraphNode* /*node*/ ) {}

	virtual void OnNodeActivated( const CBehaviorGraphNode* /*node*/ ) {}
	virtual void OnNodeDeactivated( const CBehaviorGraphNode* /*node*/ ) {}
};

class IBehaviorGraphInstanceListener
{
public:
	virtual void OnBehaviorEventProcessed( const CName& eventName ) = 0;
	virtual void OnBehaviorActivationNotify( const CName& activationName ) = 0;
	virtual void OnBehaviorDeactivationNotify( const CName& deactivationName ) = 0;
};

class CBehaviorGraphInstanceSnapshot;
class CBehaviorGraphInstance : public CObject
{
	DECLARE_ENGINE_CLASS( CBehaviorGraphInstance, CObject, CF_AlwaysTransient );

private:
	CName									m_instanceName;					//!< Name

	Bool									m_active;						//!< Is graph active

	// TODO Tomsin: What is that? A single float timer... 
	Float									m_timeActive;

	CAnimatedComponent*						m_animatedComponent;			//!< Animated component we are attached to

	TDynArray< IAnimationConstraint* >		m_constraints;					//!< Animation constraints

	THandle< CBehaviorGraph >				m_graph;						//!< Source graph this instace was created from

	InstanceBuffer*							m_data;							//!< Runtime data for graph
	const CBehaviorGraphTopLevelNode*		m_root;							//!< Graph's root
	const CBehaviorGraphStateMachineNode*	m_defaultStateMachine;			//!< Graph's default state machine

	IBehaviorGraphInstanceEditorListener*	m_editorListener;				//!< Editor Listener

	TDynArray< CName >						m_processedEvents;				//!< Processed events
	TDynArray< CName >						m_activationNotifications;		//!< Notifications
	TDynArray< CName >						m_deactivationNotifications;
	TDynArray< SBehaviorGraphScriptNotification > m_scriptEventNotifications;

	TDynArray< IBehaviorGraphInstanceListener* > m_listeners;

#ifndef NO_EDITOR_GRAPH_SUPPORT
	Bool									m_isOpenInEditor;
#endif

	enum EInstanceInternalState
	{
		IIS_Updating,
		IIS_Sampling,
		IIS_Waiting,
	};

	EInstanceInternalState					m_internalState;

	CBehaviorGraphEventHandler*				m_eventHandler;

public:
	CBehaviorGraphInstance();
	virtual ~CBehaviorGraphInstance();

	//! Binds a graph to this instance
	void Bind( const CName& name, const THandle< CBehaviorGraph >& graph, const InstanceDataLayout& dataLayout, CAnimatedComponent* component );

	//! Unbinds this instance from the shared graph.
	void Unbind();

	//! Is binded
	Bool IsBinded() const;

	//! Activate graph
	void Activate();

	//! Deactivate graph
	void Deactivate();

	//! Reset and deactivate graph - use with care
	void DeactivateAndReset();

	//! Is graph active
	Bool IsActive() const;

	//! Return time this behavior graph instance is active
	Float GetTimeActive() const { return m_timeActive; }

	//! Simple get for event handler
	CBehaviorGraphEventHandler* GetEventHandler() const { return m_eventHandler; }

	//! Simple set for event handler
	void SetEventHandler( CBehaviorGraphEventHandler* eventHandler ) { ASSERT( !m_eventHandler ); m_eventHandler = eventHandler; }

	//! Get data for instance variable ( slow, debug )
	template< class T >
	T& operator[]( const TInstanceVar<T>& var )
	{
		ASSERT( m_data );
		return m_data->operator []( var );
	}

	//! Get data for instance variable ( slow, debug )
	template< class T >
	const T& operator[]( const TInstanceVar<T>& var ) const
	{
		ASSERT( m_data );
		return m_data->operator []( var );
	}

	//! Get data buffer - for editor
	RED_INLINE InstanceBuffer& GetInstanceBuffer() { return *m_data; }

	//! Fill variables
	void FillVariables( const CBehaviorGraph* graph );

	//! Get size
	Uint32 GetSize() const;
	
public:
	//! Get instance name
	RED_INLINE const CName& GetInstanceName() const { return m_instanceName; }

	//! Generate editor fragments
	void GenerateEditorFragments( CRenderFrame* frame );

#ifndef NO_EDITOR_GRAPH_SUPPORT
	//! Used by nodes to know whether this instance is part of editor
	Bool IsOpenInEditor() const { return m_isOpenInEditor; }
#else
	Bool IsOpenInEditor() const { return false; }
#endif

public:
	//! Find slots
	CBehaviorGraphPoseSlotNode*				FindPoseSlot( const CName &slotName ) const;
	CBehaviorGraphAnimationBaseSlotNode*	FindAnimSlot( const CName &slotName, Bool onlyActive = true ) const;

	// TODO should be protected:
	//! Get all nodes of class
	template< class T > void GetNodesOfClass( TDynArray< T* >& nodes ) const;

	//! Find bone by name
	Int32 FindBoneByName( const CName& boneName ) const;
	Int32 FindBoneByName( const Char* boneName ) const;

	//! Find node by name
	CBehaviorGraphNode*	FindNodeByName( const String &name, Bool onlyActive = false ) const;
	CBehaviorGraphNode*	FindNodeByName( const String &name , CBehaviorGraphNode* startNode, Bool recursive, Bool onlyActive = false ) const;

public:
	//! Update instance
	void Update( SBehaviorUpdateContext& context, Float timeDelta );

	//! Sample instance
	void Sample( SBehaviorSampleContext& context, SBehaviorGraphOutput& pose );

	//! Reset
	void Reset();

	//! Reset variables
	void ResetVariables();

	//! Generate behavior event
	Bool GenerateEvent( const CName& name );

	//! generate behavior force event
	Bool GenerateForceEvent( const CName& name );

	//! Clear blocks activation alphas
	void ClearActivationAlphas();

	//! Process blocks activation alphas
	void ProcessActivationAlphas();

	//! Update cached animations pointers
	void UpdateCachedAnimationPointers();

public:
	//! Add listener
	void AddListener( IBehaviorGraphInstanceListener* listener );

	//! Remove listener
	Bool RemoveListener( IBehaviorGraphInstanceListener* listener );

#ifndef NO_EDITOR
	//! Set listener
	void SetEditorListener( IBehaviorGraphInstanceEditorListener* listener );

	//! Remove listener
	void RemoveEditorListener();

	//! Has listener
	Bool HasEditorListener() const;

	//! Get listener
	IBehaviorGraphInstanceEditorListener* GetEditorListener() const;
#endif

public:
	//! Get animated component
	CAnimatedComponent* GetAnimatedComponentUnsafe() { return m_animatedComponent; }

	//! Get animated component ( const )
	const CAnimatedComponent* GetAnimatedComponent() const { return m_animatedComponent; }

	//! Get behavior graph - for editor only
	CBehaviorGraph* GetGraph();

	//! Get behavior graph - for editor only
	const CBehaviorGraph* GetGraph() const;

public:
	//! Activate dynamic animation constraint
	Bool ActivateConstraint( const CNode* target, const CName activationVariableName, const CName variableToControlName, Float timeout = 0.f );

	//! Activate static animation constraint
	Bool ActivateConstraint( const Vector &target, const CName activationVariableName, const CName variableToControlName, Float timeout = 0.f );

	// Activate animation constraint with bone
	Bool ActivateConstraint( const CAnimatedComponent* target, const Int32 boneIndex, const CName activationVariableName, const CName variableToControlName, Bool useOffset, const Matrix& offsetMatrix, Float timeout = 0.f );

	// Deactivate behavior animation constraint
	Bool DeactivateConstraint( const CName activationVariableName );

	// Has behavior animation constraint
	Bool HasConstraint( const CName activationVariableName ) const;

	// Get behavior animation constraint target
	Bool GetConstraintTarget( const CName activationVariableName, Vector& value );

	//! Get number of constraints
	Uint32 GetConstraintsNum() const;

	//! Change constraint's target
	Bool ChangeConstraintTarget( const CNode* target, const CName activationVariableName, const CName variableToControlName, Float timeout );
	Bool ChangeConstraintTarget( const Vector &target, const CName activationVariableName, const CName variableToControlName, Float timeout );
	Bool ChangeConstraintTarget( const CAnimatedComponent* target, const Int32 boneIndex, const CName activationVariableName, const CName variableToControlName, Bool useOffset, const Matrix& offsetMatrix, Float timeout );

public:

	Bool HasFloatValue( const CName name ) const;
	Bool HasInternalFloatValue( const CName name ) const;
	Bool HasVectorValue( const CName name ) const;
	const Float* GetFloatValuePtr( const CName name ) const;
	const Float* GetInternalFloatValuePtr( const CName name ) const;
	const Vector* GetVectorValuePtr( const CName name ) const;

	//! Get vector value
	Vector GetVectorValue( const CName name ) const;

	//! Get float value
	Float GetFloatValue( const CName name, Float defVal = 0.0f ) const;

	//! Set vector value
	Bool SetVectorValue( const CName name, const Vector& value);

	//! Set float value
	Bool SetFloatValue( const CName name, Float value);

	//! Reset float value
	Float ResetFloatValue( CName name );

	//! Reset vector value
	Vector ResetVectorValue( CName name );

	//! Get float variables num
	Uint32 GetFloatValuesNum();

	//! Get vector variables num
	Uint32 GetVectorValuesNum();

	//! Get min and max float value
	Float GetFloatValueMin( CName name ) const;
	Float GetFloatValueMax( CName name ) const;
	Float GetFloatValueDefault( CName name ) const;

	//! Get min and max value value
	Vector GetVectorValueMin( CName name ) const;
	Vector GetVectorValueMax( CName name ) const;
	Vector GetVectorValueDefault( CName name ) const;

	//! Get internal vector value
	Vector GetInternalVectorValue( const CName name ) const;

	//! Get internal float value
	Float GetInternalFloatValue( const CName name ) const;

	//! Set internal vector value
	Bool SetInternalVectorValue( const CName name, const Vector& value);

	//! Set internal float value
	Bool SetInternalFloatValue( const CName name, Float value);

	//! Reset internal float value
	Float ResetInternalFloatValue( CName name );

	//! Reset internal vector value
	Vector ResetInternalVectorValue( CName name );

	//! Get internal float variables num
	Uint32 GetInternalFloatValuesNum();

	//! Get internal vector variables num
	Uint32 GetInternalVectorValuesNum();

	//! Get min and max internal float value
	Float GetInternalFloatValueMin( CName varName ) const;
	Float GetInternalFloatValueMax( CName varName ) const;
	Float GetInternalFloatValueDefault( CName varName ) const;

	//! Get min and max internal vector value
	Vector GetInternalVectorValueMin( CName varName ) const;
	Vector GetInternalVectorValueMax( CName varName ) const;
	Vector GetInternalVectorValueDefault( CName varName ) const;

	//! Get event id
	Uint32 GetEventId( const CName& name ) const;

	//! Get event name
	const CName& GetEventName( Uint32 id ) const;

	//! Get vector variable type
	EVectorVariableType GetVectorVariableType( CName name ) const;

#ifndef NO_EDITOR_GRAPH_SUPPORT
	void EnumVectorVariableNames( TDynArray< CName >& names ) const;
	void EnumVariableNames( TDynArray< CName >& names, Bool onlyModifiableByEffect = false ) const;
	void EnumEventNames( TDynArray< CName> & names, Bool onlyModifiableByEffect = false ) const;
	void EnumInternalVectorVariableNames( TDynArray< CName >& names ) const;
	void EnumInternalVariableNames( TDynArray< CName >& names, Bool onlyModifiableByEffect = false ) const;
#endif

public:
	//! Create instance snapshot
	CBehaviorGraphInstanceSnapshot* CreateSnapshot( CObject* parent, Bool storeBuffer = false );

	//! Restore instance snapshot
	Bool RestoreSnapshot( const CBehaviorGraphInstanceSnapshot* snapshot );

public:
	void OnEventProcessed( const CName& name );
	Bool IsEventProcessed( const CName& name ) const;

	void NotifyOfNodesActivation( const CName& name );
	void NotifyOfNodesDeactivation( const CName& name );

	Bool ActivationNotificationReceived( const CName& name ) const;
	Bool DeactivationNotificationReceived( const CName& name ) const;

	Uint32 GetEventProcessedNum() const;
	Uint32 GetActivationNotificationNum() const;
	Uint32 GetDeactivationNotificationNum() const;

	void NotifyOfScriptedNodesNotification( const CName& name, const CName & sourceState );
	
	const TDynArray< SBehaviorGraphScriptNotification >& GetScriptedNodesNotifications() const;

public:
	Bool GetSyncInfo( CBehaviorSyncInfo& info );
	Bool SynchronizeTo( const CBehaviorSyncInfo& info );
	Bool IsSynchronizing();

	Bool GetOutboundSyncTags( SBehaviorSyncTags& tags, const CBehaviorGraphStateMachineNode* startingWithStateMachine = nullptr );
	Bool ApplyInboundSyncTags( SBehaviorSyncTags& tags, const CBehaviorGraphStateMachineNode* afterStateMachine = nullptr );

	void StoreInstanceVariables( SBehaviorGraphInstanceStoredVariables& vars );
	void RestoreInstanceVariables( const SBehaviorGraphInstanceStoredVariables& vars );

public:
	Bool GetSyncInfo( CSyncInfo& info );
	Bool SynchronizeTo( const CSyncInfo& info );

public:
	void OnOpenInEditor();

	Bool HasDefaultStateMachine() const;
	Bool SetStateInDefaultStateMachine( const String& state );
	String GetCurrentStateInDefaultStateMachine() const;

	String GetCurrentStateInStateMachine( const String& stateMachine ) const;

	Bool PlaySlotAnimation( const CName& slot, const CName& animation, const SBehaviorSlotSetup* slotSetup = NULL );
	Bool PauseSlotAnimation( const CName& slot, Bool pause );
	Bool PlaySlotAnimation( const CName& slot, CSkeletalAnimationSetEntry* skeletalAnimation, const SBehaviorSlotSetup* slotSetup = NULL, Bool onlyActive = true );
	Bool StopSlotAnimation( const CName& slot, Float blendOutTime = 0.0f, Bool onlyActive = true );
	Bool HasSlotAnimation( const CName& slot, Bool onlyActive = true ) const;
	Bool GetSlot( const CName& slot, IBehaviorGraphSlotInterface& slotInterface );
	Bool GetSlot( const CName& slot, CBehaviorManualSlotInterface& slotInterface, Bool onlyActive = true );
	Bool GetSlot( const CName& slot, CBehaviorMixerSlotInterface& slotInterface, Bool onlyActive = true );
	Bool SetNeedRefreshSyncTokensOnSlot( const CName& slot, Bool value );
#ifndef NO_EDITOR_GRAPH_SUPPORT
	Bool AppendSyncTokenForEntityOnSlot( const CName& slot, const CEntity* entity );
#endif
	Bool HasSlotListener( ISlotAnimationListener* listener ) const;

	const CName& GetSlotAnimation( const CName& slot ) const;
	void EnumAnimationSlots( TDynArray< String >& slots ) const;

	Bool SetSlotPose( const CName& slot, const CAnimatedComponent* componentWithPoseLS, Float blendTime, EBlendType type, IPoseSlotListener* l = NULL );
	Bool SetSlotPose( const CName& slot, const TDynArray< AnimQsTransform >&poseLS, const TDynArray< Float >& floatTracks, Float blendTime, EBlendType type, IPoseSlotListener* l = NULL, const Matrix& localToWorld = Matrix::IDENTITY );
	Bool ResetSlotPose( const CName& slot );
	Bool DetachSlotListener( const CName& slot, ISlotAnimationListener* listener );
	Bool IsPoseSlotActive( const CName& slot ) const;
	void EnumPoseSlots( TDynArray< String >& slots ) const;

	Bool GetLookAt( const CName& nodeId, CBehaviorPointCloudLookAtInterface& nodeInterface );

	template< class _nodeClass, class _interfaceClass >
	Bool GetInterface( const CName& nodeName, _interfaceClass& nodeInterface, Bool onlyActive = true )
	{
		ASSERT( m_internalState == IIS_Waiting );

		_nodeClass* node = Cast< _nodeClass >( FindNodeByName( nodeName.AsString(), onlyActive ) );
		if ( node )
		{
			nodeInterface.Init( node, this );

			return true;
		}

		return false;
	}

private:
	//! Internal reset (without activation/deactivation)
	void ResetInternal();
};

BEGIN_CLASS_RTTI_EX( CBehaviorGraphInstance, CF_AlwaysTransient );
	PARENT_CLASS( CObject );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphHistoryLogger : public IBehaviorGraphInstanceEditorListener
{

};

class CBehaviorGraphSimpleLogger : public IBehaviorGraphInstanceEditorListener
{
public:
	enum EMessageCategory 
	{ 
		MC_Events,
		MC_Variables,
		MC_Notifications,
		MC_States,
		MC_Instance,
		MC_Last
	};

	struct SMessage
	{
		String	m_text;
		Float	m_time;
		Uint32	m_color;
	};
	typedef TDynArray< SMessage > tMessageArray;

	static const Uint32 COLOR_DEFAULT			= 0;
	static const Uint32 COLOR_ON				= 1;
	static const Uint32 COLOR_OFF				= 2;
	static const Uint32 COLOR_EVENT_ANIM		= 3;
	static const Uint32 COLOR_EVENT			= 4;
	static const Uint32 COLOR_EVENT_FORCE		= 5;
	static const Uint32 COLOR_VAR_FLOAT		= 6;
	static const Uint32 COLOR_VAR_VECTOR		= 7;

protected:
	Float			m_messageTimeDuration;
	tMessageArray*	m_messages;

	const CBehaviorGraphInstance* m_instance;

public:
	CBehaviorGraphSimpleLogger( const CBehaviorGraphInstance* instance );
	virtual ~CBehaviorGraphSimpleLogger();

	void Reset();

	Float GetMsgDuration() const;
	void SetMsgDuration( Float duration );

	const tMessageArray& GetMessages( EMessageCategory category ) const;

protected:
	void Msg( const String& text, Uint32 category, Uint32 color );
	void MsgWithPrefix( const String& prefix, const String& text, Uint32 category, Uint32 color );

public:
	virtual void OnPreUpdateInstance( Float& dt );
	virtual void OnPostSampleInstance( const SBehaviorGraphOutput& pose );

	virtual void OnGenerateEvent( const CName& event );
	virtual void OnGenerateForceEvent( const CName& event );

	virtual void OnSetVectorValue( CName name, const Vector& value );
	virtual void OnSetFloatValue( CName name, const Float value );
	virtual void OnResetFloatValue( CName name );
	virtual void OnResetVectorValue( CName name );

	virtual void OnActivateConstraint( const IAnimationConstraint* constraint );
	virtual void OnDeactivateConstraint( const IAnimationConstraint* constraint );

	virtual void OnPlaySlotAnimation( const CBehaviorGraphNode* slot, const CName& animation, const SBehaviorSlotSetup* slotSetup );
	virtual void OnPlaySlotAnimation( const CBehaviorGraphNode* slot, const CSkeletalAnimationSetEntry* animation, const SBehaviorSlotSetup* slotSetup );

	virtual void OnStopSlotAnimation( const CBehaviorGraphNode* slot );

	virtual void OnSetSlotPose( const CBehaviorGraphNode* slot, Float blend, EBlendType type );
	virtual void OnResetSlotPose( const CBehaviorGraphNode* slot );

	virtual void OnEventProcessed( const CName& event );
	virtual void OnNotifyOfNodesActivation( const CName& name );
	virtual void OnNotifyOfNodesDeactivation( const CName& name );

	virtual void OnNodeUpdate( const CBehaviorGraphNode* node );
};
