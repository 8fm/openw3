/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma  once

class CBehaviorGraphValueNode;

#include "behaviorGraphNode.h"

// class defining variable value
class CBehaviorGraphCharacterRotationNode : public CBehaviorGraphBaseNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphCharacterRotationNode, CBehaviorGraphBaseNode, "Motion", "Rotate character" );

protected:
	Vector					m_axis;
	Float					m_rotationSpeedMultiplier;

protected:
	TInstanceVar< Float	>	i_rotationDelta;
	TInstanceVar< Float	>	i_biasValue;
	TInstanceVar< Float	>	i_lastBiasValue;

protected:
	CBehaviorGraphValueNode*		m_cachedAngleVariableNode;
	CBehaviorGraphValueNode*		m_cachedControlVariableNode;
	CBehaviorGraphValueNode*		m_cachedBiasVariableNode;
	CBehaviorGraphValueNode*		m_cachedMaxAngleVariableNode;

public:
	CBehaviorGraphCharacterRotationNode();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
	virtual String GetCaption() const { return String( TXT("Character rot.") ); }
#endif

public:
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const;

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated(CBehaviorGraphInstance& instance ) const;

	virtual void CacheConnections();
};

BEGIN_CLASS_RTTI( CBehaviorGraphCharacterRotationNode );
	PARENT_CLASS( CBehaviorGraphBaseNode );
	PROPERTY_EDIT( m_axis, TXT("Rotation axis") );
	PROPERTY_EDIT( m_rotationSpeedMultiplier, TXT("Multiplier to rotation speed") );
	PROPERTY( m_cachedControlVariableNode );
	PROPERTY( m_cachedBiasVariableNode );
	PROPERTY( m_cachedAngleVariableNode );
	PROPERTY( m_cachedMaxAngleVariableNode );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphCharacterMotionToWSNode : public CBehaviorGraphBaseNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphCharacterMotionToWSNode, CBehaviorGraphBaseNode, "Motion", "Convert Motion To WS" );

public:

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const { return String( TXT("Convert Motion To WS") ); }
#endif

public:
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphCharacterMotionToWSNode );
	PARENT_CLASS( CBehaviorGraphBaseNode );
END_CLASS_RTTI();
