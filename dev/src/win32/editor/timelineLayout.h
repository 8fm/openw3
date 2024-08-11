// Copyright © 2014 CD Projekt Red. All Rights Reserved.

#pragma once

/*
Describes timeline layout.
*/
class CTimelineLayout
{
public:
	struct STrackInfo
	{
		String m_name;
		Bool m_isExpanded;
		Bool m_isHidden;
		Bool m_isPinned;
	};

	TDynArray< STrackInfo > trackInfo; // Note that order is important. May be empty for layouts that don't want to deal with individual tracks.
	
	Int32 m_verticalOffset;
	Float m_activeRangeTimeOffset;
	Float m_visibleRangeDuration;
	Float m_currentGrid;
	Float m_vertScale;
};

/*
Manages timeline layouts.
*/
class CTimelineLayoutMgr
{
public:
	CTimelineLayoutMgr();
	~CTimelineLayoutMgr();

	void StoreLayout( Uint32 sectionId, const CTimelineLayout& trackLayout );
	const CTimelineLayout* GetLayout( Uint32 sectionId ) const;

private:
	CTimelineLayoutMgr( const CTimelineLayoutMgr& );				// cctor - not defined
	CTimelineLayoutMgr& operator=( const CTimelineLayoutMgr& );		// op= - not defined

	THashMap< Uint32, CTimelineLayout > sectionIdToLayout;			// Maps section id to layout.
};

// =================================================================================================
// implementation
// =================================================================================================

/*
Ctor.
*/
RED_INLINE CTimelineLayoutMgr::CTimelineLayoutMgr()
{}

/*
Dtor.
*/
RED_INLINE CTimelineLayoutMgr::~CTimelineLayoutMgr()
{}

RED_INLINE void CTimelineLayoutMgr::StoreLayout( Uint32 sectionId, const CTimelineLayout& trackLayout )
{
	sectionIdToLayout[ sectionId ] = trackLayout;
}

RED_INLINE const CTimelineLayout* CTimelineLayoutMgr::GetLayout( Uint32 sectionId ) const
{
	return sectionIdToLayout.FindPtr( sectionId );
}
