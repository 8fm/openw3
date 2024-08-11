#pragma  once

#include "../../common/engine/behaviorGraphInstance.h"
#include "../../common/engine/behaviorGraphStateNode.h"

class CBehaviorGraphScriptStateNode : public CBehaviorGraphStateNode
{	
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphScriptStateNode, CBehaviorGraphStateNode, "State machine", "Script State" );	

protected:
	CName	m_nameAsName;					//!< Node's name as CName to provide forwarding of state name without copying etc.

	CName	m_activationScriptEvent;
	CName	m_deactivationScriptEvent;
	CName	m_becomesCurrentStateScriptEvent;
	CName	m_noLongerCurrentStateScriptEvent;
	CName	m_fullyBlendedInScriptEvent;

public:
	// TODO: Currently event system is unplugged. We can plug it in as soon as vs is over.
	class CEvent : public CBehaviorGraphDelayedEvent
	{
	public:
		CName			m_name;
		void			Handle( CBehaviorGraphInstance* instance ) const;
	};

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const;
	virtual Color GetTitleColor() const;
#endif
	
	virtual void OnPostLoad();
	virtual void OnPropertyPostChange( IProperty* property );

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void OnBecomesCurrentState( CBehaviorGraphInstance& instance ) const override;
	virtual void OnNoLongerCurrentState( CBehaviorGraphInstance& instance ) const override;
	virtual void OnFullyBlendedIn( CBehaviorGraphInstance& instance ) const override;

protected:
	virtual void SendEvent( CBehaviorGraphInstance& instance, const CName& event ) const;

	void OnUpdateName();
};

BEGIN_CLASS_RTTI( CBehaviorGraphScriptStateNode );
	PARENT_CLASS( CBehaviorGraphStateNode );
	PROPERTY( m_nameAsName );
	PROPERTY_EDIT( m_activationScriptEvent, TXT("Name of event called to script on activation.") );
	PROPERTY_EDIT( m_deactivationScriptEvent, TXT("Name of event called to script on deactivation.") );
	PROPERTY_EDIT( m_becomesCurrentStateScriptEvent, TXT("Name of event called to script when node becomes current state of state machine.") );
	PROPERTY_EDIT( m_noLongerCurrentStateScriptEvent, TXT("Name of event called to script when node is no longer current state of state machine (or state machine becomes inactive).") );
	PROPERTY_EDIT( m_fullyBlendedInScriptEvent, TXT("Name of event called to script when state is fully blended in, weight is 1.0.") );
END_CLASS_RTTI();

class CBehaviorGraphScriptStateReportingNode : public CBehaviorGraphScriptStateNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphScriptStateReportingNode, CBehaviorGraphScriptStateNode, "State machine", "Script reporting state" );	
protected:
	CName m_stateName;
public:
	class CEvent : public CBehaviorGraphDelayedEvent
	{
	public:
		CName			m_eventType;
		CName			m_stateName;
		void			Handle( CBehaviorGraphInstance* instance ) const;
	};

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const;
	virtual Color GetTitleColor() const;
#endif

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
};

BEGIN_CLASS_RTTI( CBehaviorGraphScriptStateReportingNode );
PARENT_CLASS( CBehaviorGraphScriptStateNode );
PROPERTY_EDIT( m_stateName, TXT("Name of the state to be reported") );
END_CLASS_RTTI();

class CBehaviorGraphScriptComponentStateNode : public CBehaviorGraphScriptStateNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphScriptComponentStateNode, CBehaviorGraphScriptStateNode, "State machine", "Component Script State" );

protected:
	CName	m_componentName;

protected:
	TInstanceVar< THandle<CComponent> >	i_component;

public:
	class CEvent : public CBehaviorGraphDelayedEvent
	{
	public:
		TInstanceVar< THandle<CComponent> >		i_component;
		CName									m_event;
		void			Handle( CBehaviorGraphInstance* instance ) const;
	};

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const;
	virtual Color GetTitleColor() const;
#endif

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;

protected:
	virtual void SendEvent( CBehaviorGraphInstance& instance, const CName& event ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphScriptComponentStateNode );
	PARENT_CLASS( CBehaviorGraphScriptStateNode );
	PROPERTY_EDIT( m_componentName, TXT("Name of component that events are sent to.") );
END_CLASS_RTTI();