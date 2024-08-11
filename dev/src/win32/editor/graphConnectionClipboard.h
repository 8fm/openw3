/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CGraphConnectionClipboard
{
public:
	struct Connection
	{
		CName			m_source;			// Source socket
		CName			m_destination;		// Destination socket
		Bool			m_isActive;			// Is connection active
		CGraphBlock*	m_block;			// Block

		Connection() : m_block( nullptr ) {};

		Connection( const CName& src, const CName& dest, CGraphBlock* block, Bool active )
			: m_source( src )
			, m_destination( dest )
			, m_isActive( active )
			, m_block( block )
		{};
	};

	TDynArray< Connection >	m_connections;
	CGraphSocket*			m_socket;

public:
	CGraphConnectionClipboard()
		: m_socket( nullptr )
		{}

	~CGraphConnectionClipboard() {}
};
