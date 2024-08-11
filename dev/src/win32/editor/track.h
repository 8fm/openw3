// Copyright © 2013 CD Projekt Red. All Rights Reserved.

#pragma once

class CEdTimeline;

struct Track
{
	String					m_name;
	Bool					m_isGroup;
	Bool					m_isExpanded;
	Bool					m_isHidden;		// whether track is hidden, note this has nothing to do with Track::m_isExpanded
	Track*					m_parent;
	TDynArray< Track* >		m_children;
	Uint32					m_depth;

	Track( String name, Uint32 depth = 0, bool isGroup = false )
		: m_name( name )
		, m_depth( depth )
		, m_isGroup( isGroup )
		, m_isExpanded( true )
		, m_isHidden( false )
		, m_parent( NULL )
	{
	}

	virtual Track* Create( String name, Uint32 depth, bool isGroup ) const;

	virtual Bool InsertTrack( Track* track, CEdTimeline *timeline );
	virtual void Delete( CEdTimeline* timeline );
	virtual void Rename( CEdTimeline* timeline, const String& name, bool reAdd = false, Uint32 newDepth = 0 );
};

struct GreaterTrack
{
	RED_INLINE Bool operator()(const Track* c1, const Track* c2) const
	{
		const Char* src = c1->m_name.AsChar();
		const Char* dst = c2->m_name.AsChar();

		int ret=0;
		while( ! (ret = (int)(*src - *dst)) && *dst)
			++src, ++dst;
		if ( *src > 0 && *dst > 0 )
		{
			if ( *src == '.' )
			{
				ret = -1;
			}
			if ( *dst == '.' )
			{
				ret = 1;
			}
		}
		return ret < 0;
	}
};

Bool IsRelationship( Track* ancestorTrack, Track* descendantTrack );
