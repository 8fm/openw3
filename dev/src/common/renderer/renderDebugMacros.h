#pragma once

struct Batch;
class CRenderElement_MeshChunk;

class IBatchModifier
{
	friend class BatchModifier;
private:
	IBatchModifier* m_next;

public:
	IBatchModifier();
	virtual ~IBatchModifier();
	virtual void ModifyBatch( Batch *b ) = 0;
	virtual void ModifyMeshSelection( CRenderElement_MeshChunk* meshChunk, Vector &color, Vector& effect ) {};
};

class BatchModifier
{
private:
	IBatchModifier* m_head;
public:
	BatchModifier();
	~BatchModifier() {};

	void ModifyBatch( Batch* b );
	void ModifyMeshSelection( CRenderElement_MeshChunk* meshChunk, Vector& color, Vector& effect );
	void AddBatchModifier( IBatchModifier* );
	void RemoveBatchModifier( IBatchModifier* );
};

typedef TSingleton< BatchModifier > SBatchModifier;
