/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once


class CQuestBehaviorSyncGraphSocket : public CGraphSocket
{
	DECLARE_RTTI_SIMPLE_CLASS( CQuestBehaviorSyncGraphSocket );

public:

#ifndef NO_EDITOR_GRAPH_SUPPORT

	//! CGraphSocket interface
	virtual Bool CanConnectTo( CGraphSocket *otherSocket );
	virtual Color GetLinkColor() const { return Color( 225, 225, 225 ); }

#endif

};

BEGIN_CLASS_RTTI( CQuestBehaviorSyncGraphSocket )
	PARENT_CLASS( CGraphSocket )
	END_CLASS_RTTI()
