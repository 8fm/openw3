/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_EDITOR_GRAPH_SUPPORT

/// Helper that helps with rebuilding graph block connections
class GraphConnectionRebuilder
{
protected:
	/// Cached connection info
	struct Connection
	{
		CName			m_source;			// Source socket
		CName			m_destination;		// Destination socket
		CGraphBlock*	m_block;			// Block
		Bool			m_active;

		RED_INLINE Connection() : m_block( NULL ) {};
		RED_INLINE Connection( const CName& src, const CName& dest, CGraphBlock* block, Bool isactive )
			: m_source( src )
			, m_destination( dest )
			, m_block( block )
			, m_active( isactive )
		{};
	};

	/// Cached socket info
	struct Socket
	{
		CName			m_name;
		Bool			m_isVisible;

		RED_INLINE Socket() : m_isVisible( false ) {};
		RED_INLINE Socket( const CName& name, Bool isVisible )
			: m_name( name )
			, m_isVisible( isVisible )
		{};
	};

	void StoreSocket( CGraphSocket* socket, const CName& socketName );
	void RestoreConnections( CGraphBlock* block ) const;

public:
	typedef THashMap< CName, CName > TRenameMap;

protected:
	TDynArray< Connection >		m_connections;
	TDynArray< Socket >			m_sockets;
	CGraphBlock*				m_block;

public:
	GraphConnectionRebuilder( CGraphBlock* block );
	GraphConnectionRebuilder( CGraphBlock* block, TRenameMap* map );
	~GraphConnectionRebuilder();
};

class GraphConnectionRelinker : public GraphConnectionRebuilder
{
public:
	GraphConnectionRelinker( CGraphBlock* from, CGraphBlock* to );
	~GraphConnectionRelinker();

protected:
	CGraphBlock*				m_destBlock;
};

#endif
