
#pragma once

#include "behaviorGraphMimicNodes.h"

class CBehaviorGraphValueNode;

enum EMimicNodePoseType
{
	MNPS_Pose,
	MNPS_Filter,
	MNPS_CustomFilter_Full,
	MNPS_CustomFilter_Lipsync,
};

BEGIN_ENUM_RTTI( EMimicNodePoseType );
	ENUM_OPTION( MNPS_Pose );
	ENUM_OPTION( MNPS_Filter );
	ENUM_OPTION( MNPS_CustomFilter_Full );
	ENUM_OPTION( MNPS_CustomFilter_Lipsync );
END_ENUM_RTTI();

class CBehaviorGraphMimicPoseNode : public CBehaviorGraphBaseMimicNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphMimicPoseNode, CBehaviorGraphBaseMimicNode, "Mimic", "Pose" );

protected:
	EMimicNodePoseType				m_poseType;
	CName							m_poseName;

protected:
	TInstanceVar< Int32 >			i_selectedPose;
	TInstanceVar< Float >			i_weight;

protected:
	CBehaviorGraphValueNode*		m_cachedPoseValueNode;
	CBehaviorGraphValueNode*		m_cachedPoseWeightNode;

public:
	CBehaviorGraphMimicPoseNode();

	Bool IsPoseType() const { return m_poseType == MNPS_Pose; }

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();

	virtual String GetCaption() const { return String( TXT("Add pose") ); }
	virtual Color GetTitleColor() const { return Color( 128, 0, 128 ); }
	
	Int32 GetSelectedPose( CBehaviorGraphInstance& instance ) const;
#endif

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );

	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

	virtual void CacheConnections();

	virtual Bool IsMimic() const { return true; }
};

BEGIN_CLASS_RTTI( CBehaviorGraphMimicPoseNode );
	PARENT_CLASS( CBehaviorGraphBaseMimicNode );
	PROPERTY_EDIT( m_poseType, TXT("Pose type") );
	PROPERTY_EDIT( m_poseName, TXT("Pose name, this prop will be checked if input slot 'Pose' is empty") );
	PROPERTY( m_cachedPoseValueNode );
	PROPERTY( m_cachedPoseWeightNode );
END_CLASS_RTTI();
