/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma  once

#include "behaviorGraphValueNode.h"
#include "behaviorIncludes.h"

enum ETransformType
{
	TT_Translation,
	TT_Rotation,
	TT_Scale
};

BEGIN_ENUM_RTTI( ETransformType );
	ENUM_OPTION( TT_Translation )
	ENUM_OPTION( TT_Rotation )
	ENUM_OPTION( TT_Scale )
END_ENUM_RTTI()

class CBehaviorGraphGetBoneTransformNode : public CBehaviorGraphVectorValueNode
										 , public IBehaviorGraphBonesPropertyOwner
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphGetBoneTransformNode, CBehaviorGraphVectorValueNode, "Vector", "Get bone transform" );		

protected:
	String					m_boneName; 
	ETransformType			m_type;
	
protected:
	TInstanceVar< Vector >	i_vecValue;
	TInstanceVar< Int32 >		i_boneIndex;

protected:
	CBehaviorGraphNode*		m_cachedInputNode;

public:
	CBehaviorGraphGetBoneTransformNode();

#ifndef NO_EDITOR_GRAPH_SUPPORT

	virtual void OnPropertyPostChange( IProperty *prop );
	virtual void OnRebuildSockets();

	virtual String GetCaption() const;
#endif

public:
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const;

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual Float GetValue( CBehaviorGraphInstance& instance ) const { return 0.f; }
	virtual Vector GetVectorValue( CBehaviorGraphInstance& instance ) const;
	
	virtual void OnReset( CBehaviorGraphInstance& instance ) const;
	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

	void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;

	virtual void CacheConnections();
};

BEGIN_CLASS_RTTI( CBehaviorGraphGetBoneTransformNode );
	PARENT_CLASS( CBehaviorGraphVectorValueNode );
	PROPERTY_CUSTOM_EDIT( m_boneName, TXT("Name of the bone"), TXT("BehaviorBoneSelection"));
	PROPERTY_EDIT( m_type, TXT("Translation or rotation") );
	PROPERTY( m_cachedInputNode );
END_CLASS_RTTI();