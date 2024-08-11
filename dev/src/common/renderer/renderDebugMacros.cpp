#include "build.h"


IBatchModifier::IBatchModifier()
{
	SBatchModifier::GetInstance().AddBatchModifier( this );
}

IBatchModifier::~IBatchModifier()
{
	SBatchModifier::GetInstance().RemoveBatchModifier( this );
}

void BatchModifier::ModifyBatch( Batch* b )
{
	IBatchModifier* curr = m_head;
	while ( curr )
	{
		curr->ModifyBatch( b );
		curr = curr->m_next;
	}
}

void BatchModifier::ModifyMeshSelection( CRenderElement_MeshChunk* meshChunk, Vector& color, Vector& effect )
{
	IBatchModifier* curr = m_head;
	while ( curr )
	{
		curr->ModifyMeshSelection( meshChunk, color, effect );
		curr = curr->m_next;
	}
}

void BatchModifier::AddBatchModifier( IBatchModifier* b )
{
	if ( !m_head )
	{
		m_head = b;
		b->m_next = NULL;
	}
	else
	{
		IBatchModifier* curr = m_head;
		while ( curr->m_next != NULL )
		{
			curr = curr->m_next;
		}
		curr->m_next = b;
		b->m_next = NULL;
	}
}

void BatchModifier::RemoveBatchModifier( IBatchModifier* b )
{
	if ( m_head == b )
	{
		m_head = m_head->m_next;
	}
	else
	{
		IBatchModifier* curr = m_head;
		while ( curr && curr->m_next != b )
		{
			ASSERT( curr );
			curr = curr->m_next;
		}
		if ( !curr )
		{
			return;
		}

		curr->m_next = b->m_next;
	}
}

BatchModifier::BatchModifier()
{
	m_head = NULL;
}

