/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma  once

///////////////////////////////////////////////////////////////////////////////

class CBehaviorGraphBlockEventNode	: public CBehaviorGraphBaseNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphBlockEventNode, CBehaviorGraphBaseNode, "Misc", "Block animation event" );

protected:
	CName m_eventToBlock;

#ifndef NO_EDITOR_GRAPH_SUPPORT
public:
	virtual String GetCaption() const;
	virtual Color GetTitleColor() const { return Color( 195, 50, 50 ); }
#endif

public:
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

};

BEGIN_CLASS_RTTI( CBehaviorGraphBlockEventNode );
	PARENT_CLASS( CBehaviorGraphBaseNode );
	PROPERTY_EDIT( m_eventToBlock, TXT("Animation event to block") );
END_CLASS_RTTI();
