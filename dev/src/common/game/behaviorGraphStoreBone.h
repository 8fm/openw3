/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma  once

#include "../engine/behaviorGraphNode.h"

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphStoreBoneNode : public CBehaviorGraphBaseNode
								  , public IBehaviorGraphBonesPropertyOwner
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphStoreBoneNode, CBehaviorGraphBaseNode, "Utils", "Store bone" );

protected:
	String m_boneName;
	CName m_storeName;

protected:
	TInstanceVar< Int32 > i_boneIdx;

public:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const { return String::Printf(TXT( "Store '%ls' bone as '%ls'" ), m_boneName.AsChar(), m_storeName.AsChar() ); }
#endif

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler ) override;
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const override;

public:
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const override;
};

BEGIN_CLASS_RTTI( CBehaviorGraphStoreBoneNode );
	PARENT_CLASS( CBehaviorGraphBaseNode );
	PROPERTY_CUSTOM_EDIT( m_boneName, TXT("Bone"), TXT( "BehaviorBoneSelection" ) );
	PROPERTY_EDIT( m_storeName, TXT("") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphStoreAnimEventNode : public CBehaviorGraphBaseNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphStoreAnimEventNode, CBehaviorGraphBaseNode, "Utils", "Store anim event" );

protected:
	CName m_animEventName;

public:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const { return String::Printf(TXT( "Store anim event '%ls'" ), m_animEventName.AsChar() ); }
#endif

public:
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const override;
};

BEGIN_CLASS_RTTI( CBehaviorGraphStoreAnimEventNode );
	PARENT_CLASS( CBehaviorGraphBaseNode );
	PROPERTY_EDIT( m_animEventName, TXT("") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphRestoreAnimEventNode : public CBehaviorGraphBaseNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphRestoreAnimEventNode, CBehaviorGraphBaseNode, "Utils", "Restore anim event" );

protected:
	CName m_animEventName;

public:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const { return String::Printf(TXT( "Restore anim event '%ls'" ), m_animEventName.AsChar() ); }
#endif

public:
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const override;
};

BEGIN_CLASS_RTTI( CBehaviorGraphRestoreAnimEventNode );
	PARENT_CLASS( CBehaviorGraphBaseNode );
	PROPERTY_EDIT( m_animEventName, TXT("") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////
