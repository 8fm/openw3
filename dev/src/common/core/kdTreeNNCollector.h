
#pragma once

/*
	Query collector
	In:
		k - number of near neighbors to return
	Out:
		nn_idx - nearest neighbor indices (returned)
		dd - the approximate nearest neighbor
*/

template< class TTree >
class kdTreeNNCollector
{
public:
	Int32								m_nnNum;
	typename TTree::TreeIdx*		m_nnIdx;
	typename TTree::Dist*			m_nnDist;

public:
	kdTreeNNCollector( Int32 nn )
		: m_nnNum( nn )
	{
		m_nnIdx = new typename TTree::TreeIdx[ m_nnNum ];
		m_nnDist = new typename TTree::Dist[ m_nnNum ];

		for ( Int32 i=0; i<m_nnNum; ++i )
		{
			m_nnIdx[ i ] = -1;
		}
	}

	void Reset()
	{
		for ( Int32 i=0; i<m_nnNum; ++i )
		{
			m_nnIdx[ i ] = -1;
			m_nnDist[ i ] = 0.f;
		}
	}

	~kdTreeNNCollector()
	{
		delete m_nnIdx;
		delete m_nnDist;
	}
};
