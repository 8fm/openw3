#pragma once

#ifdef USE_UMBRA

#include "../core/task.h"
#include "umbraStructures.h"

// forward declarations
class IRenderScene;

class CJobCreateTomeCollection : public CTask
{
public:
	RED_INLINE CUmbraTomeCollection*	GetTomeCollectionWrapper() const	{ return m_tomeCollectionWrapper; }
	RED_INLINE IRenderScene*			GetRenderScene() const				{ return m_renderScene; }
	RED_INLINE TVisibleChunksIndices&	GetRemapTable()						{ return m_remapTable; }
	RED_INLINE TObjectIDToIndexMap&		GetObjectIdToIndexMap()				{ return m_objectIdToIndex; }
	RED_INLINE const Bool				GetResult() const					{ return m_result; }

#ifndef RED_FINAL_BUILD
	RED_INLINE size_t					GetScratchAllocationPeak() const { return m_scratchAllocationPeak; }
	RED_INLINE size_t					GetTomeCollectionSize() const	{ return m_tomeCollectionSize; }
#endif // RED_FINAL_BUILD

public:
	CJobCreateTomeCollection( const TUmbraTileArray& tiles, IRenderScene* renderScene, const CUmbraTomeCollection* oldTomeCollectionWrapper );
	~CJobCreateTomeCollection();

	void							Run();
	void							ClearJobInfoFromTomes();
	void							RemoveTomeCollection();

#ifndef NO_DEBUG_PAGES
	virtual const Char*				GetDebugName() const { return TXT("CreateUmbraTomeCollection"); }
	virtual Uint32					GetDebugColor() const { return COLOR_UINT32( 255, 0, 255 ); }
#endif

private:
	CUmbraTomeCollection*				m_tomeCollectionWrapper;
	const CUmbraTomeCollection*			m_oldTomeCollectionWrapper;
	TVisibleChunksIndices				m_remapTable;
	const TUmbraTileArray&				m_tiles;
	IRenderScene*						m_renderScene;
	Bool								m_result;
	TObjectIDToIndexMap					m_objectIdToIndex;

#ifndef RED_FINAL_BUILD
	size_t								m_scratchAllocationPeak;
	size_t								m_tomeCollectionSize;
#endif // RED_FINAL_BUILD
};

#endif // USE_UMBRA
