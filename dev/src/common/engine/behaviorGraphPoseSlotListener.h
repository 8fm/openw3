/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma  once

class CBehaviorGraphPoseSlotNode;
class CBehaviorGraphInstance;

class IPoseSlotListener
{
public:
	virtual void OnPoseSlotEnd( const CBehaviorGraphPoseSlotNode * sender, CBehaviorGraphInstance& instance ) = 0;
};
