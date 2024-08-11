// Copyright © 2013 CD Projekt Red. All Rights Reserved.

#pragma once

#include "drawGroup.h"

// =================================================================================================
namespace TimelineImpl {
// =================================================================================================

/*
Group for drawing frequently changing elements.
*/
class CDrawGroupVolatile : public CDrawGroup
{
public:
	CDrawGroupVolatile(CEdTimeline& owner, CDrawBuffer* drawBuffer);
	virtual ~CDrawGroupVolatile();

	virtual void Draw();
	virtual Int32 GetPreferredHeight() const;

private:
	CDrawGroupVolatile(const CDrawGroupVolatile&);				// cctor - not defined
	CDrawGroupVolatile& operator=(const CDrawGroupVolatile&);	// op= - not defined
};

// =================================================================================================
// implementation
// =================================================================================================

RED_INLINE CDrawGroupVolatile::CDrawGroupVolatile(CEdTimeline& owner, CDrawBuffer* drawBuffer)
: CDrawGroup(owner, drawBuffer)
{}

RED_INLINE CDrawGroupVolatile::~CDrawGroupVolatile()
{}

// =================================================================================================
} // namespace TimelineImpl
// =================================================================================================
