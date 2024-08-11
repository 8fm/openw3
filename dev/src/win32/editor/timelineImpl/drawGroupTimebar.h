// Copyright © 2013 CD Projekt Red. All Rights Reserved.

#pragma once

#include "drawGroup.h"

// =================================================================================================
namespace TimelineImpl {
// =================================================================================================

/*
Group for drawing time bar.
*/
class CDrawGroupTimebar : public CDrawGroup
{
public:
	CDrawGroupTimebar(CEdTimeline& owner, CDrawBuffer* drawBuffer);
	virtual ~CDrawGroupTimebar();

	virtual void Draw();
	virtual Int32 GetPreferredHeight() const;

private:
	CDrawGroupTimebar(const CDrawGroupTimebar&);				// cctor - not defined
	CDrawGroupTimebar& operator=(const CDrawGroupTimebar&);		// op= - not defined
};

// =================================================================================================
// implementation
// =================================================================================================

RED_INLINE CDrawGroupTimebar::CDrawGroupTimebar(CEdTimeline& owner, CDrawBuffer* drawBuffer)
: CDrawGroup(owner, drawBuffer)
{}

RED_INLINE CDrawGroupTimebar::~CDrawGroupTimebar()
{}

RED_INLINE Int32 CDrawGroupTimebar::GetPreferredHeight() const
{
	return 35;
}

// =================================================================================================
} // namespace TimelineImpl
// =================================================================================================
