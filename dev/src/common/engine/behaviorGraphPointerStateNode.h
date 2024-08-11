
#pragma  once

//////////////////////////////////////////////////////////////////////////

#include "behaviorGraphStateNode.h"
#include "behaviorGraphTransitionBlend.h"

class CBehaviorGraphPointerStateNode;
class CBehaviorSyncInfo;

class CBehaviorGraphPointerTransitionNode : public CBehaviorGraphStateTransitionBlendNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphPointerTransitionNode, CBehaviorGraphStateTransitionBlendNode, "State machine", "Pointer transition" );

protected:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	CBehaviorGraphPointerStateNode*	m_pointerState;
#endif

#ifndef NO_EDITOR_GRAPH_SUPPORT
	RED_INLINE const CBehaviorGraphPointerStateNode* GetPointerState() const { return m_pointerState; }
#endif

public:
	virtual Bool IsManualCreationAllowed() const { return false; }

	virtual void CacheConnections();
};

BEGIN_CLASS_RTTI( CBehaviorGraphPointerTransitionNode );
	PARENT_CLASS( CBehaviorGraphStateTransitionBlendNode );
#ifndef NO_EDITOR_GRAPH_SUPPORT
	PROPERTY_NOT_COOKED( m_pointerState );
#endif
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphPointerStateNode : public CBehaviorGraphStateNode
{	
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphPointerStateNode, CBehaviorGraphStateNode, "State machine", "Pointer state" );	

protected:
	String							m_pointedStateName;
#ifndef NO_EDITOR_GRAPH_SUPPORT
	CBehaviorGraphStateNode*		m_pointedState;
#endif

public:
	RED_INLINE const String& GetPointedStateName() const { return m_pointedStateName; }

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const;
	virtual void OnRebuildSockets();
	virtual Color GetTitleColor() const;
	virtual Bool CanBeExpanded() const;

	RED_INLINE CBehaviorGraphStateNode* GetPointedState() const { return m_pointedState; }
	RED_INLINE void SetPointedState( CBehaviorGraphStateNode* state ) { m_pointedState = state; }
#endif

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;

	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const; 

	virtual void GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const;

	virtual void SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const;

	virtual Bool ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const;

	virtual Bool GetSyncInfoForInstance( CBehaviorGraphInstance& instance, CBehaviorSyncInfo& info ) const;

	virtual Bool PreloadAnimations( CBehaviorGraphInstance& instance ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphPointerStateNode );
	PARENT_CLASS( CBehaviorGraphStateNode );
	PROPERTY_EDIT( m_pointedStateName, TXT("") );
#ifndef NO_EDITOR_GRAPH_SUPPORT
	PROPERTY_NOT_COOKED( m_pointedState );
#endif
END_CLASS_RTTI();

