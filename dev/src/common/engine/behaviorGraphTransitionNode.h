/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma  once

#include "behaviorGraphNode.h"
#include "behaviorIncludes.h"

class CBehaviorGraphValueNode;
class CBehaviorGraphStateNode;

/// Abstract condition of behavior state transition
class IBehaviorStateTransitionCondition : public CObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IBehaviorStateTransitionCondition, CObject );

public:
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& /*compiler*/ ) {}
	virtual void OnInitInstance( CBehaviorGraphInstance& /*instance*/ ) const {}
	virtual void OnReleaseInstance( CBehaviorGraphInstance& /*instance*/ ) const {}

	//! check if condition is satisfied
	virtual Bool Check( CBehaviorGraphInstance& instance ) const = 0;

	//! check if condition is satisfied but do not reset
	virtual Bool Test( CBehaviorGraphInstance& instance ) const = 0;

	//! check if condition is satisfied but do not reset
	virtual void HierarchicalTest( CBehaviorGraphInstance& instance, String& conditions ) const;

	//! process event
	virtual Bool ProcessEvent( CBehaviorGraphInstance& /*instance*/, const CBehaviorEvent &/*event*/ ) const { return false; }

#ifndef NO_EDITOR_GRAPH_SUPPORT
	//! parent block is rebuilding its sockets
	virtual void OnRebuildBlockSockets( CBehaviorGraphNode * /*parent*/ ) {}
#endif

	//! reset condition
	virtual void Reset( CBehaviorGraphInstance& /*instance*/ ) const {}

	//! called on activation of parent block
	virtual void OnActivated( CBehaviorGraphInstance& /*instance*/ ) const {}

	//! called on deactivation of parent block
	virtual void OnDeactivated( CBehaviorGraphInstance& /*instance*/ ) const {}

	//! called on activation of parent block's start input
	virtual void OnStartBlockActivated( CBehaviorGraphInstance& /*instance*/ ) const {}

	//! called on deactivation of parent block's start input
	virtual void OnStartBlockDeactivated( CBehaviorGraphInstance& /*instance*/ ) const {}

	//! called on update of parent block's start input
	virtual void OnStartBlockUpdate( SBehaviorUpdateContext& /*context*/, CBehaviorGraphInstance& /*instance*/, Float /*timeDelta*/ ) const {}

	//! called after pose was sampled
	virtual void OnPoseSampled( CBehaviorGraphInstance& /*instance*/, const SBehaviorGraphOutput& /*pose*/ ) const {}

	//! process activation alpha
	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& /*instance*/, Float /*alpha*/ ) const {}

	//! generate new name of value socket
	virtual CName GenerateValueSocketName() const;

	//! get the parent node for this transition
	virtual CBehaviorGraphNode* GetParentNode() const;	

	//! Can we convert to given transition ?
	virtual Bool CanConvertTo( const IBehaviorStateTransitionCondition* /*transition*/ ) const { return false; }

	//! Copy data from one transition to another
	virtual void CopyDataFrom(const IBehaviorStateTransitionCondition* transition) = 0;

	//! Get the list of available conversion for this transition node
	virtual void EnumConversions( TDynArray<IBehaviorStateTransitionCondition*>& /*conversions*/ ) const {}

	virtual Bool Contain( const IBehaviorStateTransitionCondition* transition ) const;
	virtual Bool ContainClass( const CName& className, Bool hierarchical ) const;

	//! Cache connections to other behavior blocks
	virtual void CacheConnections();

	virtual void GetUsedVariablesAndEvents( TDynArray<CName>& /*var*/, TDynArray<CName>& /*vecVar*/, TDynArray<CName>& /*events*/, TDynArray<CName>& /*intVar*/, TDynArray<CName>& /*intVecVar*/ ) const {}

	//! Get caption for display
	virtual void GetCaption( TDynArray< String >& captions, Bool getCaptionTests, CBehaviorGraphInstance* instance = nullptr ) const = 0;
	
	String GetCaptionTest( CBehaviorGraphInstance* instance ) const;

	virtual Bool UseEvent( const CName& /*event*/, Bool /*hierarchical*/ ) const { return false; }
	virtual Bool UseVariable( const String& /*var*/, Bool /*hierarchical*/ ) const { return false; }
};

BEGIN_ABSTRACT_CLASS_RTTI( IBehaviorStateTransitionCondition );
	PARENT_CLASS( CObject );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

enum ECompareFunc
{
	CF_Equal,
	CF_NotEqual,
	CF_Less,
	CF_LessEqual,
	CF_Greater,
	CF_GreaterEqual
};

BEGIN_ENUM_RTTI( ECompareFunc );
	ENUM_OPTION( CF_Equal );
	ENUM_OPTION( CF_NotEqual );
	ENUM_OPTION( CF_Less );
	ENUM_OPTION( CF_LessEqual );
	ENUM_OPTION( CF_Greater );
	ENUM_OPTION( CF_GreaterEqual );
END_ENUM_RTTI();

class CVariableValueStateTransitionCondition : public IBehaviorStateTransitionCondition
{
	DECLARE_ENGINE_CLASS( CVariableValueStateTransitionCondition, IBehaviorStateTransitionCondition, 0 );

protected:
	CBehaviorGraphValueNode*	m_cachedVariableNode;

protected:
	CName			m_socketName;			//!< Name of the input socket
	Float			m_compareValue;			//!< Value to compare with
	ECompareFunc	m_compareFunc;			//!< How to compare values
	Bool			m_useAbsoluteValue;		//!< Use absolute value

public:
	CVariableValueStateTransitionCondition();

	virtual Bool Check( CBehaviorGraphInstance& instance ) const;
	virtual Bool Test( CBehaviorGraphInstance& instance ) const { return Check( instance ); }

#ifndef NO_EDITOR_GRAPH_SUPPORT

	virtual void OnPropertyPostChange( IProperty *property );
	virtual void OnRebuildBlockSockets( CBehaviorGraphNode *parent );

#endif

	virtual void OnStartBlockActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnStartBlockDeactivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnStartBlockUpdate( SBehaviorUpdateContext& context, CBehaviorGraphInstance& instance, Float timeDelta ) const;

	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;

	virtual void CopyDataFrom(const IBehaviorStateTransitionCondition* transition);

	virtual void GetCaption( TDynArray< String >& captions, Bool getCaptionTests = false, CBehaviorGraphInstance* instance = nullptr ) const;

	virtual void CacheConnections();
};

BEGIN_CLASS_RTTI( CVariableValueStateTransitionCondition )
	PARENT_CLASS( IBehaviorStateTransitionCondition );
	PROPERTY_EDIT( m_compareValue, TXT("Value to compare with") );
	PROPERTY_EDIT( m_compareFunc, TXT("Compare function to use") );
	PROPERTY_EDIT( m_socketName, TXT("Socket name") );	
	PROPERTY_EDIT( m_useAbsoluteValue, TXT("Use absolute value") );
	PROPERTY( m_cachedVariableNode );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CInternalVariableStateTransitionCondition : public IBehaviorStateTransitionCondition
{
	DECLARE_ENGINE_CLASS( CInternalVariableStateTransitionCondition, IBehaviorStateTransitionCondition, 0 );

protected:
	CName			m_variableName;
	Float			m_compareValue;
	ECompareFunc	m_compareFunc;

public:
	CInternalVariableStateTransitionCondition();

	virtual Bool Check( CBehaviorGraphInstance& instance ) const;
	virtual Bool Test( CBehaviorGraphInstance& instance ) const { return Check( instance ); }

	virtual void CopyDataFrom(const IBehaviorStateTransitionCondition* transition);

	virtual void GetCaption( TDynArray< String >& captions, Bool getCaptionTests = false, CBehaviorGraphInstance* instance = nullptr ) const;

	virtual void GetUsedVariablesAndEvents( TDynArray<CName>& /*var*/, TDynArray<CName>& /*vecVar*/, TDynArray<CName>& /*events*/, TDynArray<CName>& /*intVar*/, TDynArray<CName>& /*intVecVar*/ ) const;
};

BEGIN_CLASS_RTTI( CInternalVariableStateTransitionCondition )
	PARENT_CLASS( IBehaviorStateTransitionCondition );
	PROPERTY_CUSTOM_EDIT( m_variableName, TXT("Variable name"), TXT("BehaviorInternalVariableSelection") );
	PROPERTY_EDIT( m_compareValue, TXT("Value to compare with") );
	PROPERTY_EDIT( m_compareFunc, TXT("Compare function to use") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

/**
 *	priority of comparing
 *		compare parent value (if given)
 *		value from socket
 *		compare value
 */

class CParentInputValueStateTransitionCondition : public IBehaviorStateTransitionCondition
{
	DECLARE_ENGINE_CLASS( CParentInputValueStateTransitionCondition, IBehaviorStateTransitionCondition, 0 );

protected:
	CName			m_socketName;
	CName			m_parentValueName;
	CName			m_compareParentInputName;
	Float			m_compareValue;
	Bool			m_useAbsoluteValue;
	ECompareFunc	m_compareFunc;
	Float			m_epsilon;

protected:
	CBehaviorGraphValueNode*		m_cachedParentInput;
	CBehaviorGraphValueNode*		m_cachedCompareParentInput;
	CBehaviorGraphValueNode*		m_cachedTestedValue;

public:
	CParentInputValueStateTransitionCondition();

	virtual Bool Check( CBehaviorGraphInstance& instance ) const;
	virtual Bool Test( CBehaviorGraphInstance& instance ) const { return Check( instance ); }

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnPropertyPostChange( IProperty *property );
	virtual void OnRebuildBlockSockets( CBehaviorGraphNode *parent );
#endif

	virtual void OnStartBlockActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnStartBlockDeactivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnStartBlockUpdate( SBehaviorUpdateContext& context, CBehaviorGraphInstance& instance, Float timeDelta ) const;

	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;

	virtual void CopyDataFrom(const IBehaviorStateTransitionCondition* transition);

	virtual void GetCaption( TDynArray< String >& captions, Bool getCaptionTests, CBehaviorGraphInstance* instance = nullptr ) const;

	virtual void CacheConnections();
};

BEGIN_CLASS_RTTI( CParentInputValueStateTransitionCondition )
	PARENT_CLASS( IBehaviorStateTransitionCondition );
	PROPERTY_CUSTOM_EDIT( m_parentValueName, TXT("Parent value name"), TXT("BehaviorParentValueInputTransitionConditionSelection") );
	PROPERTY_EDIT( m_compareFunc, TXT("Compare function to use. Engine value ? value to compare.") );
	PROPERTY_CUSTOM_EDIT( m_compareParentInputName, TXT("Compare parent value name"), TXT("BehaviorParentValueInputTransitionConditionSelection") );
	PROPERTY_EDIT( m_socketName, TXT("Socket name") );
	PROPERTY_EDIT( m_compareValue, TXT("Value to compare with") );
	PROPERTY_EDIT( m_useAbsoluteValue, TXT("Use absolute value") );
	PROPERTY_EDIT( m_epsilon, TXT("Epsilon used for equal/not equal") );
	PROPERTY( m_cachedParentInput );
	PROPERTY( m_cachedCompareParentInput );
	PROPERTY( m_cachedTestedValue );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CAnimationEndCondition : public IBehaviorStateTransitionCondition
{
	DECLARE_ENGINE_CLASS( CAnimationEndCondition, IBehaviorStateTransitionCondition, 0 );

private:
	Bool								m_useTransitionTimeOffset;
	Float								m_backTimeOffset;

private:
	TInstanceVar< Bool >				i_fulfilled;
	TInstanceVar< CBehaviorGraphNode* >	i_sourceNode;
	TInstanceVar< Float >				i_transitionDuration;

public:
	CAnimationEndCondition();

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual Bool Check( CBehaviorGraphInstance& instance ) const;
	virtual Bool Test( CBehaviorGraphInstance& instance ) const { return Check( instance ); }

	virtual void OnStartBlockActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnStartBlockDeactivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnStartBlockUpdate( SBehaviorUpdateContext& context, CBehaviorGraphInstance& instance, Float timeDelta ) const;

	virtual void Reset( CBehaviorGraphInstance& instance ) const;

	virtual void CopyDataFrom(const IBehaviorStateTransitionCondition* transition) {}

	virtual void GetCaption( TDynArray< String >& captions, Bool getCaptionTests, CBehaviorGraphInstance* instance = nullptr ) const { captions.PushBack( ( getCaptionTests ? GetCaptionTest( instance ) : TXT("") ) + TXT("AnimEnd") ); }

private:
	void FindSourceNodeAndTransitionDuration( CBehaviorGraphNode*& node, Float& duration ) const;
};

BEGIN_CLASS_RTTI( CAnimationEndCondition )
	PARENT_CLASS( IBehaviorStateTransitionCondition );
	PROPERTY_EDIT( m_useTransitionTimeOffset, TXT("Transition will be activated 'transition duration' time earlier") );
	PROPERTY_EDIT( m_backTimeOffset, String::EMPTY );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CDelayStateTransitionCondition : public IBehaviorStateTransitionCondition
{
	DECLARE_ENGINE_CLASS( CDelayStateTransitionCondition, IBehaviorStateTransitionCondition, 0 );

protected:
	Float					m_delayTime;
	Bool					m_resetTime;

protected:
	TInstanceVar< Float >	i_currTime;

public:
	CDelayStateTransitionCondition();

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	Bool Check( CBehaviorGraphInstance& instance ) const;
	Bool Test( CBehaviorGraphInstance& instance ) const { return Check( instance ); }

	void Reset( CBehaviorGraphInstance& instance ) const;

	void OnStartBlockActivated( CBehaviorGraphInstance& instance ) const;
	void OnStartBlockDeactivated( CBehaviorGraphInstance& instance ) const;
	void OnStartBlockUpdate( SBehaviorUpdateContext& context, CBehaviorGraphInstance& instance, Float timeDelta ) const;

	void CopyDataFrom(const IBehaviorStateTransitionCondition* transition);

	virtual void GetCaption( TDynArray< String >& captions, Bool getCaptionTests, CBehaviorGraphInstance* instance = nullptr ) const;
};

BEGIN_CLASS_RTTI( CDelayStateTransitionCondition )
	PARENT_CLASS( IBehaviorStateTransitionCondition );
	PROPERTY_EDIT( m_delayTime, TXT("Delay time") );
	PROPERTY_EDIT( m_resetTime, TXT("Reset time when reset block") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CTimeThresholdStateTransitionCondition : public IBehaviorStateTransitionCondition
{
	DECLARE_ENGINE_CLASS( CTimeThresholdStateTransitionCondition, IBehaviorStateTransitionCondition, 0 );

protected:
	Float					m_maxActivationTime;
	Float					m_minActivationTime;
	Bool					m_resetTime;

protected:
	TInstanceVar< Float >	i_currTime;

public:
	CTimeThresholdStateTransitionCondition();

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	Bool Check( CBehaviorGraphInstance& instance ) const;
	Bool Test( CBehaviorGraphInstance& instance ) const { return Check( instance ); }

	void Reset( CBehaviorGraphInstance& instance ) const;

	void OnStartBlockActivated( CBehaviorGraphInstance& instance ) const;
	void OnStartBlockDeactivated( CBehaviorGraphInstance& instance ) const;
	void OnStartBlockUpdate( SBehaviorUpdateContext& context, CBehaviorGraphInstance& instance, Float timeDelta ) const; 

	void CopyDataFrom(const IBehaviorStateTransitionCondition* transition);

	virtual void GetCaption( TDynArray< String >& captions, Bool getCaptionTests, CBehaviorGraphInstance* instance = nullptr ) const;
};

BEGIN_CLASS_RTTI( CTimeThresholdStateTransitionCondition )
	PARENT_CLASS( IBehaviorStateTransitionCondition );
	PROPERTY_EDIT_RANGE( m_minActivationTime, TXT("Activation start time"), 0.0f, FLT_MAX );
	PROPERTY_EDIT_RANGE( m_maxActivationTime, TXT("Activation end time"), 0.0f, FLT_MAX );
	PROPERTY_EDIT( m_resetTime, TXT("Reset time when reset block") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CEventStateTransitionCondition : public IBehaviorStateTransitionCondition, public IBehaviorGraphProperty
{
	DECLARE_ENGINE_CLASS( CEventStateTransitionCondition, IBehaviorStateTransitionCondition, 0 );

protected:
	CName					m_eventName;

protected:
	TInstanceVar< Uint32 >	i_eventId;
	TInstanceVar< Bool >	i_eventOccured;

public:
	CEventStateTransitionCondition();

	void OnPropertyPostChange( IProperty *property );

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	Bool Check( CBehaviorGraphInstance& instance ) const;
	Bool Test( CBehaviorGraphInstance& instance ) const { return Check( instance ); }

	void Reset( CBehaviorGraphInstance& instance ) const;

	void OnDeactivated( CBehaviorGraphInstance& instance ) const;

	Bool ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const;	

	void OnStartBlockActivated( CBehaviorGraphInstance& instance ) const;
	void OnStartBlockDeactivated( CBehaviorGraphInstance& instance ) const;

	void CopyDataFrom(const IBehaviorStateTransitionCondition* transition);

	void GetUsedVariablesAndEvents( TDynArray<CName>& var, TDynArray<CName>& vecVar, TDynArray<CName>& events, TDynArray<CName>& intVar, TDynArray<CName>& intVecVar ) const;
	
	virtual void GetCaption( TDynArray< String >& captions, Bool getCaptionTests, CBehaviorGraphInstance* instance = nullptr ) const;

	virtual Bool UseEvent( const CName& event, Bool hierarchical ) const;

	//! IBehaviorGraphProperty implementation
	virtual CBehaviorGraph* GetParentGraph();

public:
	void SetEventName( const CName& eventName );
};

BEGIN_CLASS_RTTI( CEventStateTransitionCondition );
	PARENT_CLASS( IBehaviorStateTransitionCondition );
	PROPERTY_CUSTOM_EDIT( m_eventName, TXT("Event to trigger transition"), TXT("BehaviorEventEdition") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CAnimEventTransitionCondition : public IBehaviorStateTransitionCondition
{
	DECLARE_ENGINE_CLASS( CAnimEventTransitionCondition, IBehaviorStateTransitionCondition, 0 );

protected:
	CName					m_eventName;

protected:
	TInstanceVar< Bool >	i_eventOccured;
	TInstanceVar< Bool >	i_poseProvided;

public:
	CAnimEventTransitionCondition();

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	Bool Check( CBehaviorGraphInstance& instance ) const;
	Bool Test( CBehaviorGraphInstance& instance ) const { return Check( instance ); }

	void Reset( CBehaviorGraphInstance& instance ) const;

	void OnDeactivated( CBehaviorGraphInstance& instance ) const;

	void OnStartBlockActivated( CBehaviorGraphInstance& instance ) const;
	void OnStartBlockDeactivated( CBehaviorGraphInstance& instance ) const;

	void OnPoseSampled( CBehaviorGraphInstance& instance, const SBehaviorGraphOutput& pose ) const;

	void CopyDataFrom(const IBehaviorStateTransitionCondition* transition);

	virtual void GetCaption( TDynArray< String >& captions, Bool getCaptionTests, CBehaviorGraphInstance* instance = nullptr ) const;

	virtual Bool UseEvent( const CName& event, Bool hierarchical ) const;

public:
	void SetEventName( const CName& eventName );
};

BEGIN_CLASS_RTTI( CAnimEventTransitionCondition );
	PARENT_CLASS( IBehaviorStateTransitionCondition );
	PROPERTY_EDIT( m_eventName, TXT("Anim event name") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////
/// condition that is always satisfied
class CAlwaysTransitionCondition : public IBehaviorStateTransitionCondition
{
	DECLARE_ENGINE_CLASS( CAlwaysTransitionCondition, IBehaviorStateTransitionCondition, 0 );

public:
	Bool Check( CBehaviorGraphInstance& instance ) const { return true; }
	Bool Test( CBehaviorGraphInstance& instance ) const { return true; }

	void CopyDataFrom(const IBehaviorStateTransitionCondition* transition) {}

	virtual void GetCaption( TDynArray< String >& captions, Bool getCaptionTests, CBehaviorGraphInstance* instance = nullptr ) const { captions.PushBack( ( getCaptionTests ? GetCaptionTest( instance ) : TXT("") ) + TXT("Always") ); }
};

DEFINE_SIMPLE_RTTI_CLASS( CAlwaysTransitionCondition, IBehaviorStateTransitionCondition );

//////////////////////////////////////////////////////////////////////////
/// checks if character is in ragdoll
class CIsRagdolledTransitionCondition : public IBehaviorStateTransitionCondition
{
	DECLARE_ENGINE_CLASS( CIsRagdolledTransitionCondition, IBehaviorStateTransitionCondition, 0 );

public:
	Bool Check( CBehaviorGraphInstance& instance ) const;
	Bool Test( CBehaviorGraphInstance& instance ) const;

	void CopyDataFrom(const IBehaviorStateTransitionCondition* transition) {}

	virtual void GetCaption( TDynArray< String >& captions, Bool getCaptionTests, CBehaviorGraphInstance* instance = nullptr ) const { captions.PushBack( ( getCaptionTests ? GetCaptionTest( instance ) : TXT("") ) + TXT("Is ragdolled") ); }
};

DEFINE_SIMPLE_RTTI_CLASS( CIsRagdolledTransitionCondition, IBehaviorStateTransitionCondition );

//////////////////////////////////////////////////////////////////////////
/// multi condition
class CMultiTransitionCondition : public IBehaviorStateTransitionCondition
{
	DECLARE_ENGINE_ABSTRACT_CLASS( CMultiTransitionCondition, IBehaviorStateTransitionCondition );

protected:
	TDynArray< IBehaviorStateTransitionCondition* >	m_conditions;

public:
	RED_INLINE const TDynArray< IBehaviorStateTransitionCondition* >& GetConditions() const { return m_conditions; }

#ifndef NO_EDITOR_GRAPH_SUPPORT
	void OnPropertyPostChange( IProperty *prop );
#endif
	void OnPostLoad();

public:
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;
	virtual void OnReleaseInstance( CBehaviorGraphInstance& instance ) const;

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildBlockSockets( CBehaviorGraphNode *parent );
#endif

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

	virtual void OnStartBlockDeactivated( CBehaviorGraphInstance& instance ) const; 
	virtual void OnStartBlockUpdate( SBehaviorUpdateContext& context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void OnPoseSampled( CBehaviorGraphInstance& instance, const SBehaviorGraphOutput& pose ) const;

	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;

	virtual Bool Contain(const IBehaviorStateTransitionCondition* transition) const;
	virtual Bool ContainClass( const CName& className, Bool hierarchical ) const;

	virtual void CacheConnections();

	virtual void GetUsedVariablesAndEvents( TDynArray<CName>& var, TDynArray<CName>& vecVar, TDynArray<CName>& events, TDynArray<CName>& intVar, TDynArray<CName>& intVecVar ) const;

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void GetCaption( TDynArray< String >& captions, Bool getCaptionTests, CBehaviorGraphInstance* instance = nullptr ) const;
#endif

	virtual Bool UseEvent( const CName& event, Bool hierarchical ) const;
	virtual Bool UseVariable( const String& var, Bool hierarchical ) const;
};

BEGIN_ABSTRACT_CLASS_RTTI( CMultiTransitionCondition );
	PARENT_CLASS( IBehaviorStateTransitionCondition );
	PROPERTY_INLINED( m_conditions, TXT("Conditions") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////
/// composite condition
class CCompositeTransitionCondition : public CMultiTransitionCondition
{
	DECLARE_ENGINE_CLASS( CCompositeTransitionCondition, CMultiTransitionCondition, 0 );

protected:
	TInstanceVar< Uint32 >	i_currentCondition;

public:
	CCompositeTransitionCondition();

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual Bool Check( CBehaviorGraphInstance& instance ) const;
	virtual Bool Test( CBehaviorGraphInstance& instance ) const;

	virtual void Reset( CBehaviorGraphInstance& instance ) const;

	virtual Bool ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const;

	virtual void OnStartBlockActivated( CBehaviorGraphInstance& instance ) const;

	virtual void HierarchicalTest( CBehaviorGraphInstance& instance, String& conditions ) const;

	virtual Bool CanConvertTo( const IBehaviorStateTransitionCondition* transition ) const;
	virtual void CopyDataFrom( const IBehaviorStateTransitionCondition* transition );
	virtual void EnumConversions( TDynArray<IBehaviorStateTransitionCondition*>& conversions ) const;

	virtual void GetCaption( TDynArray< String >& captions, Bool getCaptionTests, CBehaviorGraphInstance* instance = nullptr ) const;
};

BEGIN_CLASS_RTTI( CCompositeTransitionCondition );
	PARENT_CLASS( CMultiTransitionCondition );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////
/// composite condition
class CCompositeSimultaneousTransitionCondition : public CMultiTransitionCondition
{
	DECLARE_ENGINE_CLASS( CCompositeSimultaneousTransitionCondition, CMultiTransitionCondition, 0 );	

public:
	CCompositeSimultaneousTransitionCondition();

	virtual Bool Check( CBehaviorGraphInstance& instance ) const;
	virtual Bool Test( CBehaviorGraphInstance& instance ) const;

	virtual void Reset( CBehaviorGraphInstance& instance ) const;

	virtual Bool ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const;

	virtual void OnStartBlockActivated( CBehaviorGraphInstance& instance ) const;

	virtual void HierarchicalTest( CBehaviorGraphInstance& instance, String& conditions ) const;

	virtual Bool CanConvertTo( const IBehaviorStateTransitionCondition* transition ) const;
	virtual void CopyDataFrom( const IBehaviorStateTransitionCondition* transition );
	virtual void EnumConversions( TDynArray<IBehaviorStateTransitionCondition*>& conversions ) const;

	virtual void GetCaption( TDynArray< String >& captions, Bool getCaptionTests, CBehaviorGraphInstance* instance = nullptr ) const;
};

BEGIN_CLASS_RTTI( CCompositeSimultaneousTransitionCondition );
	PARENT_CLASS( CMultiTransitionCondition );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

struct SBehaviorGraphTransitionSetInternalVariable
{
	DECLARE_RTTI_STRUCT( SBehaviorGraphTransitionSetInternalVariable );

	CName	m_variableName;		//!< Name of the variable
	Float	m_value;

	SBehaviorGraphTransitionSetInternalVariable();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	String GetCaption() const;
#endif
};

BEGIN_CLASS_RTTI( SBehaviorGraphTransitionSetInternalVariable );
	PROPERTY_CUSTOM_EDIT( m_variableName, TXT("Variable name"), TXT("BehaviorInternalVariableSelection") );
	PROPERTY_EDIT( m_value, TXT("") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphStateTransitionNode : public CBehaviorGraphNode
{	
	DECLARE_BEHAVIOR_ABSTRACT_CLASS( CBehaviorGraphStateTransitionNode, CBehaviorGraphNode );

protected:
	IBehaviorStateTransitionCondition*	m_transitionCondition;
	Float								m_transitionPriority;
	Bool								m_isEnabled;
	TDynArray<SBehaviorGraphTransitionSetInternalVariable> m_setInternalVariables;

protected:
	CBehaviorGraphStateNode*			m_cachedStartStateNode;
	CBehaviorGraphStateNode*			m_cachedEndStateNode;

public:
	RED_INLINE Bool IsEnabled() const { return m_isEnabled; }

public:
	CBehaviorGraphStateTransitionNode();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	//! property was changed in editor
	virtual void OnPropertyPostChange( IProperty *property );

	//! rebuild node sockets
	virtual void OnRebuildSockets();

	//! Get block shape
	virtual EGraphBlockShape GetBlockShape() const;

	//! Get block depth group
	virtual EGraphBlockDepthGroup GetBlockDepthGroup() const;

	//! Get block border color
	virtual Color GetBorderColor() const;

	//! Get client area color
	virtual Color GetClientColor() const;
#endif

	//! Build block data layout
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );

	//! Initialize instance buffer
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	//! Destroy instance buffer
	virtual void OnReleaseInstance( CBehaviorGraphInstance& instance ) const;
	virtual Bool IsOnReleaseInstanceManuallyOverridden() const override { return true; }

	//! called on reset
	virtual void OnReset( CBehaviorGraphInstance& instance ) const;

	//! called on activation of block
	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;

	//! called on deactivation of block
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

	//! called on activation of start
	virtual void OnStartBlockActivated( CBehaviorGraphInstance& instance ) const;

	//! called on deactivation of start block
	virtual void OnStartBlockDeactivated( CBehaviorGraphInstance& instance ) const;

	//! called on update of start block
	virtual void OnStartBlockUpdate( SBehaviorUpdateContext& context, CBehaviorGraphInstance& instance, Float timeDelta ) const;

	// called after sampled pose
	virtual void OnPoseSampled( CBehaviorGraphInstance& instance, const SBehaviorGraphOutput& pose ) const;

	//! process external event
	virtual Bool ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const;

	//! process activation alpha
	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;

	//! process activation alpha of transition start block
	virtual void StartBlockProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;	

	//! cache connections
	virtual void CacheConnections();

	//! Preload animations
	virtual Bool PreloadAnimations( CBehaviorGraphInstance& instance ) const;

	virtual void GetUsedVariablesAndEvents( TDynArray<CName>& var, TDynArray<CName>& vecVar, TDynArray<CName>& events, TDynArray<CName>& intVar, TDynArray<CName>& intVecVar ) const;

public:
	Float GetPriority() const { return m_transitionPriority; }

	CBehaviorGraphStateNode* GetSourceState();
	CBehaviorGraphStateNode* GetDestState();

	const CBehaviorGraphStateNode* GetSourceState() const;
	const CBehaviorGraphStateNode* GetDestState() const;

	virtual CBehaviorGraphStateNode* GetCloserState();
	virtual const CBehaviorGraphStateNode* GetCloserState() const;

#ifndef NO_EDITOR_GRAPH_SUPPORT
	void ConvertTransitionConditonTo(const CBehaviorGraphStateTransitionNode* newTransitionOwner);
	void ConvertTransitionConditonTo(const IBehaviorStateTransitionCondition* newTransition);
	void GetSetInternalVariableCaptions( TDynArray< String >& captions ) const;
#endif
	virtual void CopyFrom(const CBehaviorGraphStateTransitionNode* node);

public:
	//! Check if the transition condition is satisfied
	virtual Bool CheckTransitionCondition( CBehaviorGraphInstance& instance ) const;

	//! Test if the transition condition is satisfied
	virtual Bool TestTransitionCondition( CBehaviorGraphInstance& instance ) const ;

	//! Test if the transition condition is satisfied - slow!
	virtual void HierarchicalConditionsTest( CBehaviorGraphInstance& instance, String& conditions ) const;

	const IBehaviorStateTransitionCondition* GetTransitionCondition() const { return m_transitionCondition; }
};

BEGIN_ABSTRACT_CLASS_RTTI( CBehaviorGraphStateTransitionNode );
	PARENT_CLASS( CBehaviorGraphNode )
	PROPERTY_EDIT( m_transitionPriority, TXT("Transition priority (lower active is selected)") );
	PROPERTY_EDIT( m_isEnabled, TXT("Is this transition enabled") );
	PROPERTY_INLINED( m_transitionCondition, TXT("When transition should occur") );
	PROPERTY_EDIT( m_setInternalVariables, TXT("Internal variables that are set on activation of transition") );
	PROPERTY( m_cachedStartStateNode );
	PROPERTY( m_cachedEndStateNode );
END_CLASS_RTTI();
