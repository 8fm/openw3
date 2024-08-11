/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "pathlibNode.h"


class CSimpleBufferWriter;
class CSimpleBufferReader;


namespace PathLib
{

typedef TDynArray< CPathLink, MC_PathLib > RawLinksBuffer;


///////////////////////////////////////////////////////////////////////////////////////////////
// CLinksBuffer
///////////////////////////////////////////////////////////////////////////////////////////////
class CLinksBuffer
{
	friend class LinksIterator;
	friend class LinksErasableIterator;
	friend class ConstLinksIterator;
protected:
	RawLinksBuffer				m_buffer;
	LinkBufferIndex				m_freeList;

public:
	CLinksBuffer()
		: m_freeList( INVALID_LINK_BUFFER_INDEX )												{}
	~CLinksBuffer()																				{}

	CPathLink&					Get( LinkBufferIndex idx )										{ return m_buffer[ idx ]; }
	const CPathLink&			Get( LinkBufferIndex idx ) const								{ return m_buffer[ idx ]; }

	LinkBufferIndex				Allocate( const CPathLink& link );

	void						Resize( LinkBufferIndex count, LinkBufferIndex extraSpace );

	void						AddFlagsToEveryLink( Uint32 flags );
	void						ClearFlagsToEveryLink( Uint32 flags );
};


///////////////////////////////////////////////////////////////////////////////////////////////
// LinksIterator
///////////////////////////////////////////////////////////////////////////////////////////////
class LinksIterator	 : public Red::System::NonCopyable
{
protected:
	RawLinksBuffer&				m_links;
	LinkBufferIndex				m_index;
public:
	LinksIterator( CPathNode& pathNode );

	RED_FORCE_INLINE LinksIterator( CLinksBuffer& buffer, LinkBufferIndex firstIndex )
		: m_links( buffer.m_buffer )
		, m_index( firstIndex )																	{}

	RED_FORCE_INLINE CPathLink&	operator*() const												{ return m_links[ m_index ]; }
	RED_FORCE_INLINE CPathLink*	operator->() const												{ return &m_links[ m_index ]; }

	RED_FORCE_INLINE operator	Bool() const													{ return m_index != INVALID_LINK_BUFFER_INDEX; }
	RED_FORCE_INLINE void		operator++()													{ m_index = m_links[ m_index ].m_next; }

	LinkBufferIndex				GetIndex() const												{ return m_index; }
};


///////////////////////////////////////////////////////////////////////////////////////////////
// LinksErasableIterator
///////////////////////////////////////////////////////////////////////////////////////////////
class LinksErasableIterator : public Red::System::NonCopyable
{
	friend class CPathNode;
	friend class CPathLinkModifier;
protected:
	CLinksBuffer&				m_buffer;
	LinkBufferIndex*			m_index;

	void Erase();																				// never erase link directly - always through PathNode or CPathLinkModifier
public:
	LinksErasableIterator( CPathNode& pathNode );

	RED_FORCE_INLINE LinksErasableIterator( CLinksBuffer& buffer, LinkBufferIndex& firstIndex )
		: m_buffer( buffer )
		, m_index( &firstIndex )																{}

	RED_FORCE_INLINE CPathLink&	operator*() const												{ return m_buffer.m_buffer[ *m_index ]; }
	RED_FORCE_INLINE CPathLink*	operator->() const												{ return &m_buffer.m_buffer[ *m_index ]; }

	RED_FORCE_INLINE operator	Bool() const													{ return (*m_index) != INVALID_LINK_BUFFER_INDEX; }
	RED_FORCE_INLINE void		operator++()													{ m_index = &m_buffer.m_buffer[ *m_index ].m_next; }

	LinkBufferIndex				GetIndex() const												{ return *m_index; }
};



///////////////////////////////////////////////////////////////////////////////////////////////
// ConstLinksIterator
///////////////////////////////////////////////////////////////////////////////////////////////
class ConstLinksIterator : public Red::System::NonCopyable
{
protected:
	const RawLinksBuffer&		m_links;
	LinkBufferIndex				m_index;
public:
	ConstLinksIterator( const CPathNode& pathNode );

	RED_FORCE_INLINE ConstLinksIterator( const CLinksBuffer& buffer, LinkBufferIndex firstIndex )
		: m_links( buffer.m_buffer )
		, m_index( firstIndex )																	{}

	RED_FORCE_INLINE const CPathLink&	operator*() const										{ return m_links[ m_index ]; }
	RED_FORCE_INLINE const CPathLink*	operator->() const										{ return &m_links[ m_index ]; }

	RED_FORCE_INLINE operator			Bool() const											{ return m_index != INVALID_LINK_BUFFER_INDEX; }
	RED_FORCE_INLINE void				operator++()											{ m_index = m_links[ m_index ].m_next; }

	LinkBufferIndex						GetIndex() const										{ return m_index; }
};


/////////////////////////////////////////////////////////////////////////////////////////////////
//// CLinksList
/////////////////////////////////////////////////////////////////////////////////////////////////
//class CLinksList
//{
//protected:
//	LinkBufferIndex				m_first;
//public:
//	RED_FORCE_INLINE void		Add( CPathLink& link, LinkBufferIndex index )					{ m_first = index; link.m_next = INVALID_LINK_BUFFER_INDEX; }
//
//	void						ClearAll( CLinksBuffer& linksBuffer );
//};


///////////////////////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////////////////////


};				// namespace PathLib

