#pragma once

/// Socket in the scene graph used to represent the section structure
class CStorySceneGraphSocket : public CGraphSocket
{
	DECLARE_RTTI_SIMPLE_CLASS( CStorySceneGraphSocket );

protected:
	CStorySceneLinkElement*		m_linkElement;		//!< Link element that is responsible for handling connections from this socket

public:
	//! Get the scene link element bound to this socket
	RED_INLINE CStorySceneLinkElement* GetLinkElement() const { return m_linkElement; }

#ifndef NO_EDITOR_GRAPH_SUPPORT

public:
	//! Called to check if we can make connection between sockets
	virtual Bool CanConnectTo( CGraphSocket *otherSocket );

	//! Socket was added to block
	virtual void OnSpawned( class CGraphBlock* block, const class GraphSocketSpawnInfo& info );

	//! Called when link on this socket is created
	virtual void OnConnectionCreated( CGraphConnection* connection );

	//! Called when link on this socket is broken
	virtual void OnConnectionBroken( CGraphConnection* connection );

#endif

public:
	//! Set the low-level scene link element that mirrors this socket in the internal data
	RED_INLINE void SetLink( CStorySceneLinkElement* link ) { m_linkElement = link; }

	//! Set the name of the socket
	RED_INLINE void SetName( const CName& name ) { m_name = name; }

protected:
	CStorySceneSection* GetSectionFromSocket( CStorySceneGraphSocket* sceneSocket );
};

BEGIN_CLASS_RTTI( CStorySceneGraphSocket );
	PARENT_CLASS( CGraphSocket );
	PROPERTY( m_linkElement );
END_CLASS_RTTI();

////////////////////////////////////////////////

#ifndef NO_EDITOR_GRAPH_SUPPORT

// Spawn info for scene sockets
class StorySceneGraphSocketSpawnInfo : public GraphSocketSpawnInfo
{
public:
	CStorySceneLinkElement*		m_linkElement;		//!< Link element representing this socket

public:
	//! Constructor for output sockets ( links )
	StorySceneGraphSocketSpawnInfo( const CName& name, CStorySceneLinkElement* link, ELinkedSocketDirection direction )
		: GraphSocketSpawnInfo( ClassID< CStorySceneGraphSocket >() )
		, m_linkElement( link )
	{
		m_name = name;
		m_direction = direction;
		m_placement = direction == LSD_Input ? LSP_Left : LSP_Right;
		m_isMultiLink = direction == LSD_Input;
	}
};

#endif