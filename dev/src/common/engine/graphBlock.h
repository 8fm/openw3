/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../core/math.h"
#include "../core/enum.h"
#include "../core/object.h"
#include "graphSocket.h"

#ifndef NO_EDITOR_GRAPH_SUPPORT

enum ELinkedSocketPlacement : CEnum::TValueType;
enum ELinkedSocketDirection  : CEnum::TValueType;
enum ELinkedSocketDrawStyle : CEnum::TValueType;

/// Block shape
enum EGraphBlockShape
{
	GBS_Default,
	GBS_LargeCircle,
	GBS_Slanted,
	GBS_DoubleCircle,
	GBS_Rounded,
	GBS_Triangle,
	GBS_Octagon,
	GBS_Arrow,
	GBS_ArrowLeft,
	GBS_TriangleLeft,
};

/// Block drawing depth group
enum EGraphBlockDepthGroup
{
	GBDG_Comment,
	GBDG_Background,
	GBDG_Foreground,
};

/// Socket spawn info
class GraphSocketSpawnInfo
{
protected:
	CClass*		m_class;				//!< Socket class

public:
	CName					m_name;					//!< Name of the socket
	Color					m_color;				//!< Color
	Bool					m_isVisible;			//!< Socket should be visible
	Bool					m_isVisibleByDefault;	//!< Socket is visible by default
	Bool					m_isMultiLink;			//!< Socket can accept multiple connections
	Bool					m_isNoDraw;				//!< Do not draw this socket even if visible
	Bool					m_canHide;				//!< Socket can be hidden
	Bool					m_canStartLink;			//!< Socket can start link
	Bool					m_canEndLink;			//!< Socket can end link
	Bool					m_captionHidden;		//!< Should socket caption be hidden
	Bool					m_forceDrawConnections;	//!< Should connections be force drawn
	ELinkedSocketPlacement	m_placement;			//!< Placement
	ELinkedSocketDirection	m_direction;			//!< Data direction
	ELinkedSocketDrawStyle	m_drawStyle;			//!< Draw style

	// Initialize defaults
	GraphSocketSpawnInfo( CClass* socketClass );
	// Get class of socket to spawn
	RED_INLINE CClass* GetClass() const { return m_class; }
};

#endif

/// Block spawn info
class GraphBlockSpawnInfo
{
protected:
	CClass*		m_class;				//!< Block class

public:
	Vector		m_position;				//!< Initial position
	Uint32		m_layers;				//!< Blocks layer

	// Initialize defaults
	RED_INLINE GraphBlockSpawnInfo( CClass* blockClass )
		: m_class( blockClass )
		, m_position( 0,0,0,0 )
		, m_layers( 1 )
	{};

	RED_INLINE GraphBlockSpawnInfo( CClass* blockClass, Uint32 layerFlags )
		: m_class( blockClass )
		, m_position( 0,0,0,0 )
		, m_layers( layerFlags )
	{};

	// Get class of socket to spawn
	RED_INLINE CClass* GetClass() const { return m_class; }
};

/// Graph block
//class CGraphBlock : public ISerializable			TODO: graph block cannot be a ISerializable yet :( what a waste. We need to refactor StoryScene system first
class CGraphBlock : public CObject
{
	friend class CUndoGraphSocketSnaphot;

	DECLARE_ENGINE_ABSTRACT_CLASS( CGraphBlock, CObject );

public:
	CGraphBlock();
	virtual ~CGraphBlock();

#ifndef NO_EDITOR_GRAPH_SUPPORT

public:
	Bool						m_needsLayoutUpdate;

protected:
	Uint32						m_layerFlags;		//!< Block layer flags
	Vector						m_position;			//!< Block position in graph
	Uint32						m_version;			//!< Block version

#endif // NO_EDITOR_GRAPH_SUPPORT

protected:
	TDynArray< CGraphSocket* >	m_sockets;			//!< Block sockets

public:
	//! Get sockets
	RED_INLINE const TDynArray< CGraphSocket* >& GetSockets() const { return m_sockets; }

public:

	/* Needed for debug page */

	//! Get block caption, usually block name
	virtual String GetCaption() const;

	//! Get the name of the block
	virtual String GetBlockName() const;

#ifndef NO_EDITOR_GRAPH_SUPPORT

public:
	//! Get block position in graph
	RED_INLINE const Vector& GetPosition() const { return m_position; }

	//! Set block position in graph
	RED_INLINE void SetPosition( const Vector& position ) { m_position = position; }

	//! Get block version
	RED_INLINE Uint32 GetVersion() const { return m_version; }

	//! Is in layer
	RED_INLINE Bool IsInLayer( Uint32 layer ) const { return ( m_layerFlags & layer ) != 0; }

	//! Add to layer
	RED_INLINE void AddToLayer( Uint32 layer ) { m_layerFlags |= layer; }

	//! Remove from layer
	RED_INLINE void RemoveFromLayer( Uint32 layer ) { m_layerFlags &= ~layer; }

	//! Get block size
	virtual const Vector GetSize() const { return Vector(0,0,0); }

	//! Set block size
	virtual void SetSize( Vector& /*newSize*/ ) {}

	//! Get the name of the block category ( can be a path )
	virtual String GetBlockCategory() const;

	//! Get block shape
	virtual EGraphBlockShape GetBlockShape() const;

	//! Get block depth group
	virtual EGraphBlockDepthGroup GetBlockDepthGroup() const;

	//! Get block border color
	virtual Color GetBorderColor() const;

	//! Get block outer border color
	virtual Color GetOuterBorderColor() const { return GetBorderColor(); }

	//! Should caption text be repainted in constant size (disregarding canvas scale)
	virtual Bool ShouldCaptionBeInConstantSize() const { return false; }

	//! Checks if block can be resized by user (it is also indicated by placing black rectangle in right bottom corner of block
	virtual Bool IsResizable() const { return false; }

	//! Returns true if inner area is transparent
	virtual Bool IsInnerAreaTransparent() const { return false; }

	//! Returns true if inner area is transparent
	virtual Bool IsDraggedByClickOnInnerArea() const { return true; }

	//! Returns true if block should have button that clicked freezes/unfreezes blocks on it.
	virtual Bool CanFreezeContent() const { return false; }

	//! Get client color
	virtual Color GetClientColor() const;

	//! Get title bar color
	virtual Color GetTitleColor() const;

	//! Get display value for round blocks
	virtual String GetDisplayValue() const;

	//! Should block be highlighted as active
	virtual Bool IsActivated() const;

	//! Get block activation percentage
	virtual Float GetActivationAlpha() const;

	//! Serialization
	virtual void OnSerialize( IFile& file );

	//! Loaded from file
	virtual void OnPostLoad();

	//! If true, when you move this block, you move blocks on it.
	virtual Bool IsMovingOverlayingBlocks() const { return false; }

public:
	//! Block was spawned in editor
	virtual void OnSpawned( const GraphBlockSpawnInfo& info );

	//! Block was pasted in editor after copy (wasCopied == true) or cut (wasCopied == false)
	virtual void OnPasted( Bool wasCopied );

	//! Block is destroyed in editor
	virtual void OnDestroyed();

	//! Rebuild sockets and connections
	virtual void OnRebuildSockets();

	//! Property was changed in editor
	virtual void OnPropertyPostChange( IProperty* property );

public:
	//! Get actual version of block class
	Uint32 GetClassVersion() const;

	//! Is block instance obsolete ?
	Bool IsObsolete() const;

	//! Update block version
	Bool UpdateVersion();

	//! Show specified socket
	void ShowSocket( CGraphSocket *socket );

	//! Hide specified socket
	void HideSocket( CGraphSocket *socket );

	//! Hide sockets with no connections. Returns the sockets hidden.
	TDynArray< CGraphSocket* > HideUnusedSockets();

	//! Show all sockets that should be visible by default. Returns the sockets shown.
	TDynArray< CGraphSocket* > ShowAllSockets();

	//! Invalidate block layout
	void InvalidateLayout();

	//! Add socket
	CGraphSocket* CreateSocket( const GraphSocketSpawnInfo& info );

	//! Remove socket
	void RemoveSocket( CGraphSocket* socket );

	//! Remove all sockets
	void RemoveAllSockets();

	//! Break all links to/from this block
	void BreakAllLinks();

	//! Get all child blocks from this block
	void GetChildNodesRecursively( TDynArray< CGraphBlock* > &children );

#endif // NO_EDITOR_GRAPH_SUPPORT

public:
	//! Get socket by name
	CGraphSocket* FindSocket( const String& name ) const;

	//! Get socket by name ( faster )
	CGraphSocket* FindSocket( const CName& name ) const;

public:
	//! Find socket of given type
	template< class T > T* FindSocket( const CName& name ) const
	{
		// Linear search :P
		for ( Uint32 i=0; i<m_sockets.Size(); i++ )
		{
			CGraphSocket *socket = m_sockets[ i ];
			if ( name == socket->GetName() && socket->IsA<T>() )
			{
				return static_cast<T*>( socket );
			}
		}

		// Not found
		return NULL;
	}
};

BEGIN_ABSTRACT_CLASS_RTTI( CGraphBlock );
	PARENT_CLASS( CObject );
	PROPERTY( m_sockets );
	#ifndef NO_EDITOR_GRAPH_SUPPORT
	PROPERTY_NOT_COOKED( m_position );	
	PROPERTY_NOT_COOKED( m_version );
	#endif
END_CLASS_RTTI();

/// Block socket iterator
template< class T >
class SocketIterator
{
protected:
	const TDynArray< CGraphSocket* > *		m_sockets;
	Int32										m_index;

public:
	RED_INLINE SocketIterator( CGraphBlock* block ) 
		: m_sockets( &block->GetSockets() )
		, m_index( -1 )
	{
		operator++();
	};

	RED_INLINE operator Bool() const
	{
		return m_index >= 0 && m_index < (Int32)m_sockets->Size();
	}

	RED_INLINE T* operator*()
	{
		return (T*) (*m_sockets)[ m_index ];
	}

	RED_INLINE T* operator->()
	{
		return (T*) (*m_sockets)[ m_index ];
	}

	RED_INLINE void operator++()
	{
		while ( ++m_index < (Int32)m_sockets->Size() )
		{
			CGraphSocket* socket = (*m_sockets)[ m_index ];
			if ( socket && socket->IsA< T >() )
			{
				break;
			}
		}
	}
};
