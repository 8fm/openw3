/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma  once

#include "behaviorGraphAnimationNode.h"
#include "behaviorGraphMimicNodes.h"
#include "behaviorGraphMimicsAnimationNode.h"
#include "behaviorGraphOutputNode.h"
#include "behaviorGraphParentInputNode.h"

class CBehaviorGraphValueNode;

class CBehaviorGraphMimicsAnimationNode : public CBehaviorGraphAnimationNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphMimicsAnimationNode, CBehaviorGraphAnimationNode, "Mimic.Animation", "Animation" );

public:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();

	virtual String GetCaption() const;
	virtual Color GetTitleColor() const { return Color( 128, 0, 128 ); }
#endif

	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual Bool IsMimic() const { return true; }
};

BEGIN_CLASS_RTTI( CBehaviorGraphMimicsAnimationNode );
	PARENT_CLASS( CBehaviorGraphAnimationNode );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphMimicsEventAnimationNode	: public CBehaviorGraphMimicsAnimationNode
												, public IBehaviorGraphBonesPropertyOwner
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphMimicsEventAnimationNode, CBehaviorGraphMimicsAnimationNode, "Mimic.Animation", "Event Animation" );

protected:
	CName					m_eventName;

protected:
	TInstanceVar< Uint32 >	i_event;
	TInstanceVar< Bool >	i_running;
	TInstanceVar< Bool >	i_animationFinished;

protected:
	CBehaviorGraphNode*		m_cachedInputNode;

public:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const override;
	virtual void OnSpawned( const GraphBlockSpawnInfo& info ) override;
	virtual void OnRebuildSockets();
#endif

	CBehaviorGraphMimicsEventAnimationNode();

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler ) override;
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const override;

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const override;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const override;

	virtual void GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const override;
	virtual void SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const override;

	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const override;
	virtual Bool ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const override;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const override;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const override;

	virtual void CacheConnections() override;

	virtual CBehaviorGraph* GetParentGraph() override;

	virtual void OnAnimationFinished( CBehaviorGraphInstance& instance ) const override;

private:
	Bool HasAnimation( CBehaviorGraphInstance& instance ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphMimicsEventAnimationNode );
	PARENT_CLASS( CBehaviorGraphMimicsAnimationNode );
	PROPERTY_CUSTOM_EDIT( m_eventName, TXT(""), TXT("BehaviorEventEdition") );
	PROPERTY( m_cachedInputNode );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphMimicsGeneratorNode : public CBehaviorGraphNode
										, public IBehaviorGraphBonesPropertyOwner
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphMimicsGeneratorNode, CBehaviorGraphNode, "Mimic.Animation", "Generator" );

protected:
	String							m_trackName;
	Float							m_weight;

protected:
	TInstanceVar< Int32 >			i_trackIndex;
	TInstanceVar< Float >			i_weight;

protected:
	CBehaviorGraphValueNode*		m_cachedWeightVariableNode;
	CBehaviorGraphValueNode*		m_cachedPoseNumVariableNode;

public:
	CBehaviorGraphMimicsGeneratorNode();

	virtual CSkeleton* GetBonesSkeleton( CAnimatedComponent* component ) const;

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const;

#ifndef NO_EDITOR_GRAPH_SUPPORT

	virtual void OnPropertyPostChange( IProperty *prop );

	virtual void OnRebuildSockets();

	virtual String GetCaption() const { return String( TXT("Mimic generator") ); }
	virtual Color GetTitleColor() const { return Color( 128, 0, 128 ); }

#endif

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual void GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const;
	virtual void SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const;

	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;
	virtual Bool ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

	virtual void CacheConnections();

	virtual Bool IsMimic() const { return true; }

protected:
	void CacheTrack( CBehaviorGraphInstance& instance ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphMimicsGeneratorNode );
	PARENT_CLASS( CBehaviorGraphNode );
	PROPERTY_CUSTOM_EDIT( m_trackName, TXT("Name of the float track"), TXT("BehaviorTrackSelection"));
	PROPERTY_EDIT( m_weight, TXT("") );
	PROPERTY( m_cachedWeightVariableNode );
	PROPERTY( m_cachedPoseNumVariableNode );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

enum EMimicGeneratorType
{
	MGT_Null,
	MGT_Damp,
	MGT_Cheeks,
	MGT_HeadSin,
};

BEGIN_ENUM_RTTI( EMimicGeneratorType );
	ENUM_OPTION( MGT_Null );
	ENUM_OPTION( MGT_Damp );
	ENUM_OPTION( MGT_Cheeks );
	ENUM_OPTION( MGT_HeadSin );
END_ENUM_RTTI();

class CBehaviorGraphMimicsModifierNode	: public CBehaviorGraphBaseMimicNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphMimicsModifierNode, CBehaviorGraphBaseMimicNode, "Mimic.Animation", "Generator Adv" );

protected:
	Float								m_weight;
	EMimicGeneratorType					m_type;

protected:
	TInstanceVar< Float >				i_weight;
	TInstanceVar< TDynArray< Float > >	i_params;

protected:
	CBehaviorGraphValueNode*		m_cachedWeightVariableNode;

public:
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
	virtual String GetCaption() const;
#endif

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

	virtual void CacheConnections();

protected:
	Uint32 GetParamsNum() const;

	void ModifyFloatTracks_Null( SBehaviorGraphOutput &output, Float weight, TDynArray< Float >& params ) const;
	void ModifyFloatTracks_Damp( SBehaviorGraphOutput &output, Float weight, TDynArray< Float >& params ) const;
	void ModifyFloatTracks_Cheeks( SBehaviorGraphOutput &output, Float weight, TDynArray< Float >& params ) const;
	void ModifyFloatTracks_HeadSin( SBehaviorGraphOutput &output, Float weight, TDynArray< Float >& params ) const;
	//...
};

BEGIN_CLASS_RTTI( CBehaviorGraphMimicsModifierNode );
	PARENT_CLASS( CBehaviorGraphBaseMimicNode );
	PROPERTY_EDIT( m_type, TXT("Type") );
	PROPERTY( m_cachedWeightVariableNode );
END_CLASS_RTTI();
