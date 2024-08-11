/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma  once

#include "../engine/behaviorGraphNode.h"
#include "../engine/behaviorIkTwoBones.h"

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphAimingWithIKNode;

/**
 *	Base input is used only for getting base pose for aiming to know how aiming bone relates to base bone in terms of location
 */
//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphAimingWithIKNode : public CBehaviorGraphNode
									 , public IBehaviorGraphBonesPropertyOwner
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphAimingWithIKNode, CBehaviorGraphNode, "Aiming", "Aim with IK" );

protected:
	CName m_aimingBaseBoneName; // may be shoulder - same as in ik
	STwoBonesIKSolverData m_ik;

protected:
	TInstanceVar< Float > i_timeDelta;
	TInstanceVar< Int32 > i_aimingBaseBoneIdx;
	TInstanceVar< STwoBonesIKSolver > i_ik;
#ifndef NO_EDITOR
	TInstanceVar< Vector > i_debugLookAtTargetDirMS;
	TInstanceVar< Matrix > i_debugBaseAimingBaseMS;
	TInstanceVar< Matrix > i_debugBaseLowerMS;
	TInstanceVar< Matrix > i_debugInputAimingBaseMS;
	TInstanceVar< Matrix > i_debugOutputLowerMS;
#endif

public:
	CBehaviorGraphAimingWithIKNode();

protected:
	CBehaviorGraphNode* m_cachedInputNode;
	CBehaviorGraphNode* m_cachedBaseInputNode;
	CBehaviorGraphVectorValueNode* m_cachedLookAtTargetDirMSInputNode;

public:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
	virtual String GetCaption() const { return TXT( "Aim with IK" ); }
#endif

	virtual void OnGenerateFragments( CBehaviorGraphInstance& instance, CRenderFrame* frame ) const;

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler ) override;
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const override;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const override;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const override;

	virtual void GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const;
	virtual void SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const;

	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;
	virtual Bool ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const;

	virtual void CacheConnections();

public:
	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const override;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const override;
};

BEGIN_CLASS_RTTI( CBehaviorGraphAimingWithIKNode );
	PARENT_CLASS( CBehaviorGraphNode );
	PROPERTY_CUSTOM_EDIT( m_aimingBaseBoneName, TXT("Aiming base bone name"), TXT( "BehaviorBoneSelection" ) );
	PROPERTY_EDIT( m_ik, TXT("") );
	PROPERTY( m_cachedInputNode );
	PROPERTY( m_cachedBaseInputNode );
	PROPERTY( m_cachedLookAtTargetDirMSInputNode );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////
