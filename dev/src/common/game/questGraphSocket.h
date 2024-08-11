/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */
#pragma once
#include "../engine/graphSocket.h"

class CQuestGraphSocket : public CGraphSocket
{
	DECLARE_RTTI_SIMPLE_CLASS( CQuestGraphSocket  );

public:

#ifndef NO_EDITOR_GRAPH_SUPPORT

	//! CGraphSocket interface
	virtual Bool CanConnectTo( CGraphSocket *otherSocket );

#endif

};

BEGIN_CLASS_RTTI( CQuestGraphSocket )
	PARENT_CLASS( CGraphSocket )
END_CLASS_RTTI()

////////////////////////////////////////////////

class CQuestCutControlGraphSocket : public CGraphSocket
{
	DECLARE_RTTI_SIMPLE_CLASS( CQuestCutControlGraphSocket );

public:

#ifndef NO_EDITOR_GRAPH_SUPPORT

	//! CGraphSocket interface
	virtual Bool CanConnectTo( CGraphSocket *otherSocket );
	virtual Color GetLinkColor() const { return Color( 225, 83, 47 ); }

#endif

};

BEGIN_CLASS_RTTI( CQuestCutControlGraphSocket )
	PARENT_CLASS( CGraphSocket )
END_CLASS_RTTI()

////////////////////////////////////////////////

#ifndef NO_EDITOR_GRAPH_SUPPORT

// Spawn info for scene sockets
class CQuestGraphSocketSpawnInfo : public GraphSocketSpawnInfo
{

public:

	//! Constructor for output sockets ( links )
	CQuestGraphSocketSpawnInfo( const CName& name, 
		ELinkedSocketDirection direction, 
		ELinkedSocketPlacement placement, 
		CClass *socketClass = ClassID< CQuestGraphSocket >() )
		: GraphSocketSpawnInfo( socketClass )
	{
		m_name = name;
		m_direction = direction;
		m_placement = placement;
		m_isMultiLink = (direction == LSD_Input);	
		if ( placement == LSP_Center )
		{
			m_isNoDraw = true;
			m_forceDrawConnections = true;
			m_isMultiLink = true;
			m_isVisible = true;
			m_isVisibleByDefault = true;

			if ( socketClass == ClassID< CQuestGraphSocket >() )
			{
				m_canStartLink = false;
			}
		}
	}
};

#endif
