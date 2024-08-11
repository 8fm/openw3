/**
* Copyright © 2016 CD Projekt Red. All Rights Reserved.
*/

#pragma  once

#include "behaviorIncludes.h"
#include "behaviorGraphNode.h"

// Node which changes 'child' bone transform to match 'parent' bone position and orientation
class CBehaviorNodeParentChild	: public CBehaviorGraphBaseNode
								, public IBehaviorGraphBonesPropertyOwner
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorNodeParentChild, CBehaviorGraphBaseNode, "Constraints.Simple", "Parent-child" );

protected:
	String					m_parentBoneName;
	String					m_childBoneName;
	Vector					m_offset;
	Bool					m_changeOnlyTranslation;
	
protected:
	TInstanceVar< Int32 >	i_parentBoneIndex;
	TInstanceVar< Int32 >	i_childBoneIndex;

public:
	CBehaviorNodeParentChild();

public:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const override { return String( TXT( "Parent-child" ) ); }
#endif

public:
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler ) override;
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const override;
	virtual void OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const override;

	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const override;

public:
	virtual CSkeleton* GetBonesSkeleton( CAnimatedComponent* component ) const override;
};

BEGIN_CLASS_RTTI( CBehaviorNodeParentChild );
	PARENT_CLASS( CBehaviorGraphBaseNode );
	PROPERTY_CUSTOM_EDIT( m_parentBoneName, TXT( "Parent bone" ), TXT( "BehaviorBoneSelection" ) );
	PROPERTY_CUSTOM_EDIT( m_childBoneName, TXT( "Child bone" ), TXT( "BehaviorBoneSelection" ) );
	PROPERTY_EDIT( m_offset, TXT( "Translation offset" ) );
	PROPERTY_EDIT( m_changeOnlyTranslation, TXT( "Change only translation" ) );
END_CLASS_RTTI();