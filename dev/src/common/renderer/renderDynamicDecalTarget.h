/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once


class CRenderDynamicDecal;
class CRenderDynamicDecalChunk;
class CRenderCollector;


class IDynamicDecalTarget
{
public:
	typedef TDynArray< CRenderDynamicDecalChunk*, MC_RenderData > DynamicDecalChunkList;

private:
	DynamicDecalChunkList m_dyndecalChunks;

public:
	virtual ~IDynamicDecalTarget();

	void AddDynamicDecal( CRenderDynamicDecal* decal );
	void OnDynamicDecalChunkDestroyed( CRenderDynamicDecalChunk* chunk );

protected:
	void ClearDynamicDecalChunks();

	// Subclasses must implement this, to add chunks to the list. Do not clear or remove elements from the output array, just PushBack new ones.
	virtual void CreateDynamicDecalChunks( CRenderDynamicDecal* decal, DynamicDecalChunkList& outChunks ) = 0;
};

