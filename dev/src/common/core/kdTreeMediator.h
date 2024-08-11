

#include "kdTreeTypes.h"

#pragma once

template< class P >
class kdTreePointSetTempl
{
public:
	RED_INLINE const P operator[] ( Int32 i ) const;
	RED_INLINE Int32 GetPointDim() const;
	RED_INLINE Int32 GetPointNum() const;
};

template< class TTree >
class kdTreeMediator
{
public:
	virtual Bool GetPointSet( const typename TTree::PointSet*& pointSet ) const = 0;
};
