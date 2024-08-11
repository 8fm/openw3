
#include "../../common/core/kdTreeMediator.h"
#include "expEdge.h"

#pragma once

class ExpTreeMediator : public kdTreeMediator< ExpTree >
{
	typedef TDynArray< Int32, MC_Gameplay >	TEdgeList;
	
	ExpTree::PointSet	m_pointSet;
	TEdgeList			m_edges;
	Int32				m_idx;

public:
	ExpTreeMediator();

	Bool AddEdge( const Vector& p1, const Vector& p2, Int32 id );

	Int32 FindEdge( Int32 nnId ) const;

	void Clear();

	void RestoreFromCookedFile( IFile& reader );

#ifndef NO_EDITOR
	void CookToFile( IFile& writer ) const;
	Uint32 ComputeCookedDataSize() const;
#endif // NO_EDITOR

public: // kdTreeMediator
	virtual Bool GetPointSet( const ExpTree::PointSet*& pointSet ) const
	{
		if ( m_pointSet.GetPointNum() > 0 )
		{
			pointSet = &m_pointSet;
			return true;
		}
		return false;
	}
};
