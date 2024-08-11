
#include "../../common/core/kdTree.h"
#include "../../common/core/kdTreePointSet.h"

#pragma once

#define MAX_EXPLORATIONS 10240

template < Int32 MAX_BITS >
class ExpFilter
{
protected:
	Bool				m_filterOff;
	TBitSet< MAX_BITS > m_bits;

public:
	RED_INLINE void SetFilterBit( Int32 idx ) { m_bits.Set( idx ); }
	RED_INLINE void ClearFilterBit( Int32 idx ) { m_bits.Clear( idx ); }
	RED_INLINE Bool GetFilterBit( Int32 idx ) const { return m_bits.Get( idx ); }
	
	Bool FilterFunc( Int32 idx ) const { return m_filterOff || GetFilterBit( idx ); }
};

typedef TkdTree< Float, Float, Int32, FloatPointSet< 2 >, kdTreeAllocMemClass< MC_Gameplay >, const ExpFilter< MAX_EXPLORATIONS > > ExpTree;
