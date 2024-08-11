/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma  once

#include "behaviorGraph.h"
#include "behaviorGraphInstance.h"

class IBehaviorGraphInstanceEditorListener;
class ISlotAnimationListener;
class CBehaviorGraphStackSnapshot;
class CBehaviorManualSlotInterface;
class CBehaviorMixerSlotInterface;
class CBehaviorSyncInfo;
struct SBehaviorSyncTags;
class IBehaviorGraphSlotInterface;
struct SBehaviorSlotSetup;
struct SAnimatedComponentSyncSettings;
struct SBehaviorGraphInstanceStoredVariables;
class CBehaviorPointCloudLookAtInterface;
struct SBehaviorUpdateContext;
struct CSyncInfo;
class IPoseSlotListener;
struct SBehaviorGraphOutput;
struct SBehaviorSampleContext;
class CRenderFrame;

class CBehaviorGraphStack : public CObject
{
	DECLARE_ENGINE_CLASS( CBehaviorGraphStack, CObject, CF_AlwaysTransient );

	enum EStackInternalState
	{
		SIS_Updating,
		SIS_Sampling,
		SIS_Waiting,
	};

	class Freezer
	{
	public:
		enum EFreezeState
		{
			FFD_None,
			FFD_Freezing,
			FFD_Unfreezing,
			FFD_BlendingIn,
			FFD_BlendingOut,
		};

	private:
		EFreezeState	m_state;
		Float			m_speed;
		Float			m_progress;

	public:
		Freezer();

		void SetState( EFreezeState s, CBehaviorGraphStack* stack );
		void Reset();
		void Start( EFreezeState s, Float duration, CBehaviorGraphStack* stack );

		void Update( Float& inOutTimeDelta, CBehaviorGraphStack* stack );
		void Blend( SBehaviorGraphOutput& pose, SBehaviorGraphOutput& frozenPose ) const;

		Bool IsBusy() const;
		Bool IsFading() const;
		Bool IsBlending() const;
	};

protected:
	const TDynArray< SBehaviorGraphInstanceSlot >*	m_slots;
	TDynArray< CBehaviorGraphInstance* >			m_instances;
	TDynArray< SBehaviorGraphScriptNotification >	m_pendingScriptNotifications;
	CBehaviorGraphInstance*							m_constraintInstance;
	EStackInternalState								m_internalState;
	Uint8											m_lock;
	Bool											m_active;
	Bool											m_frozen;
	Freezer											m_freezer;

public:
	CBehaviorGraphStack();

	//! Initialize
	void Init( const TDynArray< SBehaviorGraphInstanceSlot >& instanceSlots, const CBehaviorGraph* constraintGraph, CName defaultBahaviorGraphAnimationSlotNode );

	//! Reset - use with care!
	void Reset();

	virtual void OnSerialize( IFile& file );
	virtual void OnFinalize();

public:
	//! Update behavior graphs
	void Update( SBehaviorUpdateContext* context, Float timeDelata );

	//! Sample behavior graphs
	SBehaviorGraphOutput& Sample( SBehaviorSampleContext* context );

	//! Activate
	void Activate();

	//! Deactivate
	void Deactivate();

	//! Is active
	Bool IsActive() const;

	//! Clear all stack
	void ClearAllStack();

	//! Lock stack - use with care
	void Lock( Bool flag );

	//! Is stack locked
	Bool IsLocked() const;

	//! Freeze pose
	void FreezePose();

	//! Unfreeze pose
	void UnfreezePose();

	void FreezePoseFadeIn( Float fadeInTime );
	void UnfreezePoseFadeOut( Float fadeOutTime );

	void FreezePoseBlendIn( Float blendInTime );
	void UnfreezePoseBlendOut( Float blendOutTime );

	//! Has frozen pose
	Bool HasFrozenPose() const;
	Bool IsFrozenPoseFading() const;
	Bool IsFrozenPoseBlending() const;

private:
	Bool HasCachedFrozenPose() const;
	void CreateaAndCacheFrozenPose();
	void ReleaseCacheFrozenPose();

public:
	//! Has stack pose constraints
	Bool HasPoseConstraints() const;

	//! Set constraint enable
	void SetPoseConstraintsEnable( Bool flag );

	//! Apply animation constraints
	SBehaviorGraphOutput& ApplyPoseConstraints(	Float timeDelta, SBehaviorUpdateContext* updateContext, 
							SBehaviorSampleContext* sampleContext, SBehaviorGraphOutput& inputPose );

public:
	//! Activate instances, removing all currently active behavior instances there are on the stack.
	Bool ActivateBehaviorInstances( const TDynArray< CName >& names );

	//! Activate instance, removing all currently active behavior instances there are on the stack
	// ( you'll end up with a stack that consists ONLY of this behavior )
	Bool ActivateBehaviorInstances( const CName& name );

	//! Activate and synchronize instances
	Bool ActivateAndSyncBehaviorInstances( const TDynArray< CName >& names );

	//! Activate and synchronize instances
	Bool ActivateAndSyncBehaviorInstances( const CName& name );

	//! Is synchronizing
	Bool IsSynchronizing() const;

	//! Attach instance - as opposed to the 'Activate' methods family, this method will only add 
	// a new behavior. Mind that it's your responsiblity to call Detach.
	Bool AttachBehaviorInstance( const CName& name, const SBehaviorGraphInstanceStoredVariables * copyVariablesValues = NULL );

	//! Detach instance
	Bool DetachBehaviorInstance( const CName& name );

	//! Reset instances
	void ResetBehaviorInstances();

	//! Activate all instances
	Bool ActivateAllInstances();

	//! Deactivate all instances
	Bool DeactivateAllInstances();

	//! Deactivate all with reset instances
	Bool DeactivateAndResetAllInstances();

	//! Process script notifications
	void ProcessScriptNotifications( CAnimatedComponent* componenet );

	//! Check if there are any script notifications to be called
	Bool HasAnyScriptNotifications() const;

	//! Generate editor fragments
	void GenerateEditorFragments( CRenderFrame* frame );

private:
	static void ProcessScriptNotifications( CEntity* entity, SBehaviorGraphScriptNotification const * firstNotification, Uint32 notificationsCount );

public:
	Bool PauseSlotAnimation( const CName& slot, Bool pause );
	Bool PlaySlotAnimation( const CName& slot, const CName& animation, const SBehaviorSlotSetup* slotSetup = NULL );
	Bool PlaySlotAnimation( const CName& slot, CSkeletalAnimationSetEntry* skeletalAnimation, const SBehaviorSlotSetup* slotSetup = NULL, Bool onlyActive = true );
	Bool StopSlotAnimation( const CName& slot, Float blendOutTime = 0.0f, Bool onlyActive = true );
    Bool StopAllSlotAnimation( const CName& slot, Float blendOutTime = 0.0f, Bool onlyActive = true );
	Bool HasSlotAnimation( const CName& slot, Bool onlyActive = true ) const;
	Bool GetSlot( const CName& slot, IBehaviorGraphSlotInterface& slotInterface );
	Bool GetSlot( const CName& slot, CBehaviorManualSlotInterface& slotInterface, Bool onlyActive = true );
	Bool GetSlot( const CName& slot, CBehaviorMixerSlotInterface& slotInterface, Bool onlyActive = true, Bool searchFromTheTop = true );
	Bool SetNeedRefreshSyncTokensOnSlot( const CName& slot, Bool value );
#ifndef NO_EDITOR_GRAPH_SUPPORT
	Bool AppendSyncTokenForEntityOnSlot( const CName& slot, const CEntity* entity );
#endif

	Bool SetSlotPose( const CName& slot, const CAnimatedComponent* componentWithPoseLS, Float blendTime, EBlendType type, IPoseSlotListener* l = NULL );
	Bool SetSlotPose( const CName& slot, const TDynArray< AnimQsTransform >&poseLS, const TDynArray< Float >& floatTracks, Float blendTime, EBlendType type, IPoseSlotListener* l = NULL, const Matrix& localToWorld = Matrix::IDENTITY );
	Bool ResetSlotPose( const CName& slot );
	Bool IsPoseSlotActive( const CName& slot ) const;
	Bool DetachSlotListener( const CName& slot, ISlotAnimationListener* listener );
	Bool HasSlotListener( ISlotAnimationListener* listener ) const;

	Bool GetLookAt( const CName& nodeId, CBehaviorPointCloudLookAtInterface& nodeInterface );

	template< class _nodeClass, class _interfaceClass >
	Bool GetInterface( const CName& nodeName, _interfaceClass& nodeInterface, Bool onlyActive = true, Bool searchFromTheTop = true )
	{
		ASSERT( m_internalState == SIS_Waiting );

		Int32 size = (Int32)m_instances.Size();
		if ( searchFromTheTop )
		{
			for ( Int32 i=size-1; i>=0; --i )
			{
				if ( m_instances[i]->GetInterface< _nodeClass, _interfaceClass >( nodeName, nodeInterface, onlyActive ) )
				{
					return true;
				}
			}
		}
		else
		{
			for ( Int32 i=0; i<size; ++i )
			{
				if ( m_instances[i]->GetInterface< _nodeClass, _interfaceClass >( nodeName, nodeInterface, onlyActive ) )
				{
					return true;
				}
			}
		}

		if ( m_constraintInstance && m_constraintInstance->GetInterface< _nodeClass, _interfaceClass >( nodeName, nodeInterface, onlyActive ) )
		{
			return true;
		}

		return false;
	}

public:
	//! Is event processed
	Bool IsEventProcessed( const CName& event ) const;

	//! Activation notification
	Bool ActivationNotificationReceived( const CName& notification ) const;

	//! Deactivation notification
	Bool DeactivationNotificationReceived( const CName& notification ) const;

	//! Activation notification
	Bool ActivationNotificationReceived( const CName& instance, const CName& notification ) const;

	//! Deactivation notification
	Bool DeactivationNotificationReceived( const CName& instance, const CName& notification ) const;

	//! Has any activation notification
	Bool AnyActivationNotificationsReceived() const;

	//! Has any deactivation notification
	Bool AnyDeactivationNotificationsReceived() const;

public:
	//! Set behavior variable
	Bool SetBehaviorVariable( const CName varName, Float value, Bool inAllInstances = false );

	//! Set behavior variable
	Bool SetBehaviorVariable( const CName& instance, const CName varName, Float value );

	//! Set behavior vector variable
	Bool SetBehaviorVariable( const CName varName, const Vector& value, Bool inAllInstances = false );

	//! Set behavior vector variable
	Bool SetBehaviorVariable( const CName& instance, const CName varName, const Vector& value );

public:
	//! Get behavior float value
	Float GetBehaviorFloatVariable( const CName varName, Float defValue = 0.0f ) const;

	//! Get behavior vector value
	Vector GetBehaviorVectorVariable( const CName varName ) const;

	//! Get behavior float value
	Float GetBehaviorFloatVariable( const CName& instance, const CName varName, Float defValue = 0.0f ) const;

	//! Get behavior vector value
	Vector GetBehaviorVectorVariable( const CName& instance, const CName varName ) const;

	//! Get behavior internal value
	Float GetBehaviorInternalFloatVariable( const CName varName, Float defValue = 0.0f ) const;

	Bool HasBehaviorFloatVariable( const CName name ) const;
	const Float* GetBehaviorFloatVariablePtr( const CName name ) const;
	const Float* GetBehaviorInternalFloatVariablePtr( const CName varName ) const;
	const Vector* GetBehaviorVectorVariablePtr( const CName name ) const;

public:
	//! Generate behavior event
	Bool GenerateBehaviorEvent( const CName& event );

	//! Generate behavior event for chosen instance
	Bool GenerateBehaviorEvent( const CName& instance, const CName& event );

	//! Generate behavior force event
	Bool GenerateBehaviorForceEvent( const CName& event );

	//! Generate behavior force event for chosen instance
	Bool GenerateBehaviorForceEvent( const CName& instance, const CName& event );

	//! Generate behavior event for all behaviors on stack
	Bool GenerateBehaviorStackEvent( const CName& event );

	//! Generate behavior force event for all behaviors on stack
	Bool GenerateBehaviorStackForceEvent( const CName& event );

public:
	//! Activate behavior animation constraint
	Bool ActivateConstraint( const CNode* target, const CName activationVariableName, const CName variableToControlName, Float timeout = 0.f );
	Bool ActivateConstraint( const Vector &target, const CName activationVariableName, const CName variableToControlName, Float timeout = 0.f );
	Bool ActivateConstraint( const CAnimatedComponent* target, const String& boneName, const CName activationVariableName, const CName variableToControlName, Bool useOffset, const Matrix& offsetMatrix, Float timeout = 0.f );

	//! Deactivate behavior animation constraint
	Bool DeactivateConstraint( const CName activationVariableName );

	//! Has behavior animation constraint
	Bool HasConstraint( const CName activationVariableName ) const;

	//! Get behavior animation constraint target
	Vector GetConstraintTarget( const CName activationVariableName );

	//! Change behavior animation constrint's target
	Bool ChangeConstraintTarget( const CNode* target, const CName activationVariableName, const CName variableToControlName, Float timeout );
	Bool ChangeConstraintTarget( const Vector &target, const CName activationVariableName, const CName variableToControlName, Float timeout );
	Bool ChangeConstraintTarget( const CAnimatedComponent* target, const String& bone, const CName activationVariableName, const CName variableToControlName, Bool useOffset, const Matrix& offsetMatrix, Float timeout );

public:
	//! Has instance
	Bool HasInstance( const CName& name ) const;

	//! Has active instance
	Bool HasActiveInstance( const CName& name ) const;

	//! Returns the name of currently active instance
	CName GetActiveTopInstance() const;
	CName GetActiveBottomInstance() const;

	//! Get behavior graph instances
	void GetInstances( TDynArray< CName >& instances ) const;

	//! Get behavior graph instances
	RED_INLINE Uint32 GetNumInstances() const { return m_instances.Size(); }

	//! Get behavior graph instance name
	CName const & GetInstanceName( Uint32 index ) const;

	//! Get behavior graph instances as one string
	String GetInstancesAsString() const;

	//! Has slot with name
	Bool HasInstanceSlot( const CName& name ) const;

	//! Has default state machine
	Bool HasDefaultStateMachine( const CName& instance ) const;

	//! Get state in default state machine
	String GetStateInDefaultStateMachine( const CName& instance ) const;

public:
	Bool GetSyncInfo( CSyncInfo& sync ) const;
	Bool SynchronizeTo( const CSyncInfo& sync, const SAnimatedComponentSyncSettings& ass );
	Bool SynchronizeTo( SBehaviorSyncTags& syncTags );

public:
	//! Create stack snapshot
	CBehaviorGraphStackSnapshot* CreateSnapshot( CObject* parent );

	//! Restore stack snapshot
	Bool RestoreSnapshot( const CBehaviorGraphStackSnapshot* snapshot );

public: // for editor
	//! Get instance by name ( editor )
	CBehaviorGraphInstance* GetBehaviorGraphInstance( const CName& name ) const;

	//! Activate instance ( editor )
	Bool ActivateBehaviorInstance( CBehaviorGraph* graph, const CName& name, const TDynArray< CName >* newStack = NULL );

	//! Activate instance ( only for Paksas controllers )
	Bool AttachBehaviorInstance( CBehaviorGraph* graph, const CName& name );

	//! Get behavior instances based on graph
	void GetBehaviorInstancesBasedOn( const CBehaviorGraph* graph, TDynArray< CBehaviorGraphInstance* >& instances ) const;

	//! Recreate instance
	CBehaviorGraphInstance* RecreateBehaviorInstance( const CBehaviorGraph* graph, CBehaviorGraphInstance* instance,  const CName& name = CName::NONE );

public:
	//! Reset cached animation pointers
	void ResetAnimationCache();
	Bool IsGraphAvailable( const CName& name ) const { return FindSlot( name ) != nullptr; }

protected:
	void Clear();

	Bool InternalActivateBehaviorInstances( const TDynArray< CName >& names, Bool sync );
	Bool InternalActivateBehaviorInstance( CBehaviorGraph* graph, const CName& name, Bool clearStack, const TDynArray< CName >* newStack );

	String DumpStackSyncInfo() const;
	String DumpStackSyncInfo( const TDynArray< CName >& names ) const;

	void RefreshUpdateMode();
	CAnimatedComponent* GetAnimatedComponent() const;
	const SBehaviorGraphInstanceSlot* FindSlot( const CName& name ) const;
	Bool HasBehaviorInstanceListener( const CName& instanceName ) const;
	Bool GetBehaviorInstanceListener( const CName& instanceName ) const;
	THandle< CBehaviorGraph > LoadBehaviorGraph( const CName& instanceName, const CAnimatedComponent* ac );
};

BEGIN_CLASS_RTTI_EX( CBehaviorGraphStack, CF_AlwaysTransient );
	PARENT_CLASS( CObject );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

#ifndef NO_EDITOR

class BehaviorListenersSwitcher
{
	struct SwitcherElem
	{
		CName									m_name;
		IBehaviorGraphInstanceEditorListener*	m_listener;
	};

	TDynArray< SwitcherElem >	m_elems;
	TDynArray< CName >			m_instancesName;

public:
	void OnInstancePush( CBehaviorGraphInstance* instance );
	void OnInstancePop( CBehaviorGraphInstance* instance );

	void Process( TDynArray< CBehaviorGraphInstance* >& instances );
};

#endif
