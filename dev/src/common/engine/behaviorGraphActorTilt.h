/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "behaviorGraphPivotRotationNode.h"

#pragma once

class CBehaviorGraphActorTiltNode : public CBehaviorGraphPivotRotationNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphActorTiltNode, CBehaviorGraphPivotRotationNode, "Constraints.Gameplay", "Actor tilt" );

protected:
	Float		m_scaleFactor;
	EAxis		m_scaleAxis;

	String		m_leftThighBone;
	String		m_leftShinBone;
	String		m_rightThighBone;
	String		m_rightShinBone;

	Float		m_leftThighWeight;
	Float		m_leftShinWeight;
	Float		m_rightThighWeight;
	Float		m_rightShinWeight;

protected:
	TInstanceVar< Int32 > i_leftThighBone;
	TInstanceVar< Int32 > i_leftShinBone;
	TInstanceVar< Int32 > i_rightThighBone;
	TInstanceVar< Int32 > i_rightShinBone;

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const { return String( TXT("Actor tilt") ); }
#endif

public:
	CBehaviorGraphActorTiltNode();

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphActorTiltNode );
	PARENT_CLASS( CBehaviorGraphPivotRotationNode );
	PROPERTY_EDIT( m_scaleFactor, TXT("Global factor for scaling bones") );
	PROPERTY_EDIT( m_scaleAxis, TXT("") );
	PROPERTY_CUSTOM_EDIT( m_leftThighBone, TXT(""), TXT("BehaviorBoneSelection") );
	PROPERTY_CUSTOM_EDIT( m_leftShinBone, TXT(""), TXT("BehaviorBoneSelection") );
	PROPERTY_CUSTOM_EDIT( m_rightThighBone, TXT(""), TXT("BehaviorBoneSelection") );
	PROPERTY_CUSTOM_EDIT( m_rightShinBone, TXT(""), TXT("BehaviorBoneSelection") );
	PROPERTY_EDIT( m_leftThighWeight, TXT("") );
	PROPERTY_EDIT( m_leftShinWeight, TXT("") );
	PROPERTY_EDIT( m_rightThighWeight, TXT("") );
	PROPERTY_EDIT( m_rightShinWeight, TXT("") );
END_CLASS_RTTI();
