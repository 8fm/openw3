/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CGraphConnection;
class CGraphBlock;

#ifndef NO_EDITOR_GRAPH_SUPPORT

/// Socket flags
enum ELinkedSocketFlags
{
	LSF_MultiLinks			= FLAG( 0 ),	//!< Socket can accept multiple links
	LSF_Visible				= FLAG( 1 ),	//!< Socket is visible now
	LSF_VisibleByDefault	= FLAG( 2 ),	//!< Socket is visible by default
	LSF_CanHide				= FLAG( 3 ),	//!< Socket can be hidden
	LSF_NoDraw				= FLAG( 4 ),	//!< Do not draw this socket even if visible
	LSF_CanStartLink		= FLAG( 5 ),	//!< Socket can start link
	LSF_CanEndLink			= FLAG( 6 ),	//!< Socket can end link
	LSF_CaptionHidden		= FLAG( 7 ),	//!< Socket caption is visible
	LSF_ForceDrawConnections= FLAG( 8 ),	//!< Force drawing of connections, even if socket is not visible
};

/// Socket placement
enum ELinkedSocketPlacement : CEnum::TValueType
{
	LSP_Left,		//!< Place socket at the left side of the block
	LSP_Right,		//!< Place socket at the right side of the block
	LSP_Bottom,		//!< Place socket at the bottom of the block
	LSP_Center,		//!< Place socket at the center
	LSP_Title,		//!< Place socket at the title bar of the block
};

BEGIN_ENUM_RTTI( ELinkedSocketPlacement );
	ENUM_OPTION( LSP_Left );
	ENUM_OPTION( LSP_Right );
	ENUM_OPTION( LSP_Bottom );
	ENUM_OPTION( LSP_Center );
	ENUM_OPTION( LSP_Title );
END_ENUM_RTTI();

/// Socket data direction
enum ELinkedSocketDirection  : CEnum::TValueType
{
	LSD_Input,
	LSD_Output,
	LSD_Variable,
};

BEGIN_ENUM_RTTI( ELinkedSocketDirection );
	ENUM_OPTION( LSD_Input );
	ENUM_OPTION( LSD_Output );
	ENUM_OPTION( LSD_Variable );
END_ENUM_RTTI();

/// Socket draw style
enum ELinkedSocketDrawStyle : CEnum::TValueType
{
	LSDS_Default,
	LSDS_Arrow,
	LSDS_InArrow,
};

BEGIN_ENUM_RTTI( ELinkedSocketDrawStyle );
	ENUM_OPTION( LSDS_Default );
	ENUM_OPTION( LSDS_Arrow );
	ENUM_OPTION( LSDS_InArrow );
END_ENUM_RTTI();

#endif

/// Graph socket
class CGraphSocket : public ISerializable
{
	DECLARE_RTTI_SIMPLE_CLASS_WITH_POOL( CGraphSocket, MemoryPool_SmallObjects, MC_Graph );

protected:
	CGraphBlock*					m_block;			//!< Parent block
	CName							m_name;				//!< Socket name
	TDynArray< CGraphConnection* >	m_connections;		//!< Connections to other sockets

#ifndef NO_EDITOR_GRAPH_SUPPORT

protected:
	Uint32							m_flags;			//!< Socket flags
	ELinkedSocketPlacement			m_placement;		//!< Socket placement
	ELinkedSocketDirection			m_direction;		//!< Socket data direction
	ELinkedSocketDrawStyle			m_drawStyle;		//!< Socket draw style
	String							m_caption;			//!< Socket caption (if different then name)
	Color							m_color;			//!< Socket drawing color

#endif

public:
	//! Get graph block that owns us
	RED_INLINE CGraphBlock* GetBlock() const { return m_block; }

	//! Get socket name
	RED_INLINE const CName& GetName() const { return m_name; }

	//! Get connections
	RED_INLINE const TDynArray< CGraphConnection* >& GetConnections() const { return m_connections; }

#ifndef NO_EDITOR_GRAPH_SUPPORT

public:
	//! Get socket flags
	RED_INLINE Uint32 GetFlags() const { return m_flags; }

	//! Is socket visible
	RED_INLINE Bool IsVisible() const { return ( m_flags & LSF_Visible ) != 0; }

	//! Is socket visible by default
	RED_INLINE Bool IsVisibleByDefault() const { return ( m_flags & LSF_VisibleByDefault ) != 0; }

	//! Can socket have multiple links ?
	RED_INLINE Bool IsMultiLink() const { return ( m_flags & LSF_MultiLinks ) != 0; }

	//! Is this a no draw socket
	RED_INLINE Bool IsNoDraw() const { return ( m_flags & LSF_NoDraw ) != 0; }

	//! Can socket be hidden ?
	RED_INLINE Bool CanHide() const { return ( m_flags & LSF_CanHide ) != 0; }

	//! Can socket start link
	RED_INLINE Bool CanStartLink() const { return ( m_flags & LSF_CanStartLink ) != 0; }

	//! Can socket end link
	RED_INLINE Bool CanEndLink() const { return ( m_flags & LSF_CanEndLink ) != 0; }

	//! Can draw socket caption
	RED_INLINE Bool CanDrawCaption() const { return ( m_flags & LSF_CaptionHidden ) == 0; }

	//! force connections drawing from invisible sockets
	RED_INLINE Bool ForceDrawConnections() const { return ( m_flags & LSF_ForceDrawConnections ) != 0; } 

	//! Get socket color
	RED_INLINE const Color& GetColor() const { return m_color; }

	//! Get socket placement
	RED_INLINE ELinkedSocketPlacement GetPlacement() const { return m_placement; }

	//! Get socket data direction
	RED_INLINE ELinkedSocketDirection GetDirection() const { return m_direction; }

	//! Get socket draw style
	RED_INLINE ELinkedSocketDrawStyle GetDrawStyle() const { return m_drawStyle; }

public:
	CGraphSocket();
	virtual ~CGraphSocket();

	//! Connect to given socket. Returns the newly created connection or nullptr on failure.
	//! Note: The result contains only the one connection from the created pair, the one in which 'this'
	//!       is on the 'source' end. There is always also the second one, pointing in the opposite direction
	CGraphConnection* ConnectTo( CGraphSocket *socket, Bool active = true );

	//! Disconnect from given socket
	void DisconnectFrom( CGraphSocket *socket );

	//! Break all links
	void BreakAllLinks();

	//! Break all links to given block
	void BreakAllLinksTo( CGraphBlock *block );

public:
	//! Can we connect to given socket
	virtual Bool CanConnectTo( CGraphSocket *otherSocket )=0;

	//! Get color of links at this socket
	virtual Color GetLinkColor() const;

	//! Ignore during GC to save time (temporary flag until GC is removed)
	virtual const Bool CanIgnoreInGC() const override RED_FINAL { return true; }

#endif
	
public:
	//! Do we have any active/hidden connections at all ?
	Bool HasConnections( Bool includeHidden = false ) const;

	//! Do we have any active/hidden connection to given socket ?
	Bool HasConnectionsTo( CGraphSocket *socket, Bool includeHidden = false ) const;

	//! Do we have any active/hidden connection to given block ?
	Bool HasConnectionsTo( CGraphBlock *block, Bool includeHidden = false ) const;

	//! Remove all empty connections
	void CleanupConnections();

#ifndef NO_EDITOR_GRAPH_SUPPORT

	//! Should this socket be drawn ?
	Bool ShouldDraw() const;

	//! Set socket visibility
	void SetSocketVisibility( Bool visible );

	//! Set socket caption
	void SetCaption( const String &caption );

	//! Get socket caption
	void GetCaption( String& caption ) const;

	//! For all sockets the serialization parent is the block object
	virtual ISerializable* GetSerializationParent() const;

	// When ISerializable is deserialized an a parent is known this method is called
	virtual void RestoreSerializationParent( ISerializable* parent );

#endif

public:
	//! Bind socket to block
	virtual void BindToBlock( CGraphBlock* block );

	//! Loaded from file
	virtual void OnPostLoad();

#ifndef NO_EDITOR_GRAPH_SUPPORT

	//! Socket was added to block
	virtual void OnSpawned( CGraphBlock* block, const class GraphSocketSpawnInfo& info );

	//! Socket was removed from block
	virtual void OnDestroyed();

	//! Socket was hidden
	virtual void OnHide();

	//! Socket was shown
	virtual void OnShow();

	//! New connection
	virtual void OnConnectionCreated( CGraphConnection* connection );

	//! Connection has been broken
	virtual void OnConnectionBroken( CGraphConnection* connection );

#endif
};

BEGIN_ABSTRACT_CLASS_RTTI( CGraphSocket );
	PARENT_CLASS( ISerializable );
	PROPERTY( m_block ); // acts as old "parent" link
	PROPERTY( m_name );
	PROPERTY( m_connections );
#ifndef NO_EDITOR_GRAPH_SUPPORT
	PROPERTY_NOT_COOKED( m_flags );
	PROPERTY_NOT_COOKED( m_placement );
	PROPERTY_NOT_COOKED( m_caption );
	PROPERTY_NOT_COOKED( m_color );
	PROPERTY_NOT_COOKED( m_direction );
	PROPERTY_NOT_COOKED( m_drawStyle );
#endif
END_CLASS_RTTI();

