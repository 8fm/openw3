
#pragma once

#include "behaviorGraphNode.h"
#include "../engine/skeletonMapper.h"
#include "behaviorIncludes.h"

class IBehaviorGraphRetargetCharacterNodeMethod : public CObject, public IBehaviorGraphBonesPropertyOwner
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IBehaviorGraphRetargetCharacterNodeMethod, CObject );

public:
	virtual void BuildDataLayout( InstanceDataLayoutCompiler& compiler ) {}
	virtual void InitInstance( CBehaviorGraphInstance& instance ) const {}

	virtual void Update( SBehaviorUpdateContext& context, CBehaviorGraphInstance& instance, Float timeDelta ) const {}
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput& output, Float weight ) const = 0;

	virtual CSkeleton* GetBonesSkeleton( CAnimatedComponent* component ) const		{ return NULL; }
	virtual TDynArray<SBehaviorGraphBoneInfo>* GetBonesProperty()					{ return NULL; }
};

BEGIN_ABSTRACT_CLASS_RTTI( IBehaviorGraphRetargetCharacterNodeMethod );
	PARENT_CLASS( CObject );
END_CLASS_RTTI();

class CBehaviorGraphRetargetCharacterNodeMethod_Scale : public IBehaviorGraphRetargetCharacterNodeMethod
{
	DECLARE_ENGINE_CLASS( CBehaviorGraphRetargetCharacterNodeMethod_Scale, IBehaviorGraphRetargetCharacterNodeMethod, 0 );

private:
	Float	m_scaleFactor;

public:
	CBehaviorGraphRetargetCharacterNodeMethod_Scale();

	virtual void BuildDataLayout( InstanceDataLayoutCompiler& compiler ) {}
	virtual void InitInstance( CBehaviorGraphInstance& instance ) const {}

	virtual void Update( SBehaviorUpdateContext& context, CBehaviorGraphInstance& instance, Float timeDelta ) const {}
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput& output, Float weight ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphRetargetCharacterNodeMethod_Scale );
	PARENT_CLASS( IBehaviorGraphRetargetCharacterNodeMethod );
	PROPERTY_EDIT( m_scaleFactor, TXT("Scale factor") );
END_CLASS_RTTI();

class CBehaviorGraphRetargetCharacterNodeMethod_Skeleton : public IBehaviorGraphRetargetCharacterNodeMethod
{
	DECLARE_ENGINE_CLASS( CBehaviorGraphRetargetCharacterNodeMethod_Skeleton, IBehaviorGraphRetargetCharacterNodeMethod, 0 );

protected:
	THandle< CSkeleton >				m_skeleton;
	CName								m_pelvisBoneName;
	TDynArray< SBehaviorGraphBoneInfo >	m_scaleOnlyBones;

protected:
	TInstanceVar< Float >				i_motionScale;
	TInstanceVar< TDynArray< Int32 > >	i_translationBones;
	TInstanceVar< TDynArray< Int32 > >	i_scaleBones;

public:
	CBehaviorGraphRetargetCharacterNodeMethod_Skeleton();

	virtual void BuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void InitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void Update( SBehaviorUpdateContext& context, CBehaviorGraphInstance& instance, Float timeDelta ) const {}
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput& output, Float weight ) const;

	virtual CSkeleton* GetBonesSkeleton( CAnimatedComponent* component ) const		{ return m_skeleton.Get(); }
	virtual TDynArray<SBehaviorGraphBoneInfo>* GetBonesProperty()					{ return &m_scaleOnlyBones; }
};

BEGIN_CLASS_RTTI( CBehaviorGraphRetargetCharacterNodeMethod_Skeleton );
	PARENT_CLASS( IBehaviorGraphRetargetCharacterNodeMethod );
	PROPERTY_EDIT( m_skeleton, TXT("Skeleton that we are retargeting from") );
	PROPERTY_CUSTOM_EDIT_NAME( m_scaleOnlyBones, TXT("Bones with only scale"), String::EMPTY, TXT("BehaviorBoneMultiSelection") );	
END_CLASS_RTTI();

class CBehaviorGraphRetargetCharacterNodeMethod_SkeletonMapper : public IBehaviorGraphRetargetCharacterNodeMethod
{
	DECLARE_ENGINE_CLASS( CBehaviorGraphRetargetCharacterNodeMethod_SkeletonMapper, IBehaviorGraphRetargetCharacterNodeMethod, 0 );

private:
	THandle< CSkeleton >	m_skeleton;

private:
	TInstanceVar< CSkeleton2SkeletonMapper* > i_mapper;

public:
	virtual void BuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void InitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void Update( SBehaviorUpdateContext& context, CBehaviorGraphInstance& instance, Float timeDelta ) const {}
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput& output, Float weight ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphRetargetCharacterNodeMethod_SkeletonMapper );
	PARENT_CLASS( IBehaviorGraphRetargetCharacterNodeMethod );
	PROPERTY_EDIT( m_skeleton, TXT("Skeleton A") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphRetargetCharacterNode : public CBehaviorGraphBaseNode, public IBehaviorGraphBonesPropertyOwner
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphRetargetCharacterNode, CBehaviorGraphBaseNode, "Retarget", "Retarget character" );

protected:
	IBehaviorGraphRetargetCharacterNodeMethod*		m_method;

protected:
	TInstanceVar< Float >						i_weight;

protected:
	CBehaviorGraphValueNode*					m_cachedValueNode;

public:
	CBehaviorGraphRetargetCharacterNode();

public:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
	virtual String GetCaption() const { return String( TXT("Retarget character") ); }
#endif

public:
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const;

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

	virtual void CacheConnections();

	virtual CSkeleton* GetBonesSkeleton( CAnimatedComponent* component ) const;
	virtual TDynArray<SBehaviorGraphBoneInfo>* GetBonesProperty();
};

BEGIN_CLASS_RTTI( CBehaviorGraphRetargetCharacterNode );
	PARENT_CLASS( CBehaviorGraphBaseNode );
	PROPERTY( m_cachedValueNode );
	PROPERTY_INLINED( m_method, TXT("Method") );
END_CLASS_RTTI();
