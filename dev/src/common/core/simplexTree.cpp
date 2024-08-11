/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "simplexTree.h"
#include "object.h"


IMPLEMENT_ENGINE_CLASS( SSimplexTreeStruct );

SSimplexTreeStruct::SSimplexTreeStruct()
{
	m_parent = -1;
	m_positiveStruct = -1;
	m_negativeStruct = -1;
	m_positiveID = -1;
	m_negativeID = -1;
	m_normalX = 1.0f;
	m_normalY = 0.0f;
	m_offset = 0.0f;
	m_reserved = -1;
}
void SSimplexTreeStruct::Set( Float px, Float py, Float dx, Float dy )
{
	//m_positiveID = id;
	//m_negativeID = id;
	m_normalX =  dy;
	m_normalY = -dx;
	m_offset = -( px*m_normalX + py*m_normalY );
}

CSimplexTreeNode::CSimplexTreeNode()
{
	SSimplexTreeStruct root;
	m_nodes.PushBack( root );
}

CSimplexTreeNode::~CSimplexTreeNode()
{

}

Int32 CSimplexTreeNode::CreatePositiveStruct( Int32 par, Int32 id, Float px, Float py, Float dx, Float dy )
{
	SSimplexTreeStruct nod;
	nod.Set( px, py, dx, dy );
	nod.m_parent = par;
	Int32 ind = m_nodes.Size();
	m_nodes.PushBack( nod );
	m_nodes[par].m_positiveStruct = ind;
	m_nodes[par].m_positiveID = id;
	return ind;
}

Int32 CSimplexTreeNode::CreateNegativeStruct( Int32 par, Int32 id, Float px, Float py, Float dx, Float dy )
{
	SSimplexTreeStruct nod;
	nod.Set( px, py, dx, dy );
	nod.m_parent = par;
	Int32 ind = m_nodes.Size();
	m_nodes.PushBack( nod );
	m_nodes[par].m_negativeStruct = ind;
	m_nodes[par].m_negativeID = id;
	return ind;
}

void CSimplexTreeNode::RemoveNode( Int32 ind )
{
	if( ind>0 )
	{
		m_nodes[ ind ].m_reserved = -666;
		RemoveNodes666();
	}
}

void CSimplexTreeNode::RemoveNodes666( )
{
	const Int32 numn = (Int32)m_nodes.Size();
	Int32 i;

	for( i=1;i<numn;++i )
	{
		if( m_nodes[ m_nodes[i].m_parent ].m_reserved == -666 )
		{
			m_nodes[i].m_reserved = -666;
		}
	}
	TDynArray<Int32> indices;
	indices.Resize( numn );
	Int32 n = 0;
	for( i=0;i<numn;++i )
	{
		indices[i] = n;
		if( m_nodes[i].m_reserved==-666 )
		{
			indices[i] = -1;
		}
		else
		{
			n++;
		}
	}
	for( i=0;i<numn;++i )
	{
		if( m_nodes[i].m_parent>=0 ){	m_nodes[i].m_parent = indices[m_nodes[i].m_parent]; }
		if( m_nodes[i].m_positiveStruct>=0 ){	m_nodes[i].m_positiveStruct = indices[m_nodes[i].m_positiveStruct]; }
		if( m_nodes[i].m_negativeStruct>=0 ){	m_nodes[i].m_negativeStruct = indices[m_nodes[i].m_negativeStruct]; }
	}
	for( i=numn-1;i>0;i-- )
	{
		if( m_nodes[i].m_reserved == -666 )
		{
			m_nodes.RemoveAt( i );
		}
	}
}

Int32 CSimplexTreeNode::FindNodeAtPoint( Float x, Float y )
{
	Int32 ind = 0;
	while( m_nodes.Size()!=0 )
	{
		Float gr = m_nodes[ind].Gradient( x, y );
		if( gr>0.0f )
		{
			if( m_nodes[ind].m_positiveStruct>=0 )
			{
				ind =  m_nodes[ind].m_positiveStruct;
				continue;
			}
			else
			{
				return ind;
			}
		}
		else
		{
			if( m_nodes[ind].m_negativeStruct>=0 )
			{
				ind =  m_nodes[ind].m_negativeStruct;
				continue;
			}
			else
			{
				return ind;
			}
		}
	}
	return -1;
}

Int32 CSimplexTreeNode::FindIDAtPoint( Float x, Float y )
{
	Int32 ind = FindNodeAtPoint( x, y );
	if( ind > 0 && ind < m_nodes.SizeInt() )
	{
		Float gr = m_nodes[ind].Gradient( x, y );
		return (gr>=0.0f ? m_nodes[ind].m_positiveID : m_nodes[ind].m_negativeID ); 
	}
	else
		return -1;	
}