/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CRenderSkinningData;
class CSkinManagerFlushChangesJob;

/// Manager of texture atlases
class CRenderSkinManager : public IRenderResource
{
private:
	static Bool					m_useWorkers;
	Vector						m_defaultBindData;

protected:
	GpuApi::BufferRef					m_skinBuffer;		//!< skinning data buffer
	Uint32								m_maxBonesPerChunk; //!< maximum bones per mesh chunk
	Uint32								m_maxChunks;		//!< maximum chunks that have m_maxBonesPerChunk
	Red::Threads::CMutex				m_mutex;			//!< Access mutex
	TDynArray<CRenderSkinningData*>		m_resourceList;		//!< list of objects
	Matrix*								m_lockedBuf;
	Red::Threads::AtomicOps::TAtomic32	m_lockedOffset;
	
	friend class CSkinManagerFlushChangesJob;
	CSkinManagerFlushChangesJob*		m_flushChangeJob;

public:
	CRenderSkinManager(Uint32 maxBonesPerChunk, Uint32 maxChunks);
	~CRenderSkinManager();

	//! Start flush changes before rendering
	void StartFlushChanges();

	void FlushChanges();

	//Finish Job
	void FinishWorker();

	//! Finish flush changes before rendering
	void FinishFlushChanges();

	void Register(CRenderSkinningData* skinData);
	void Unregister(CRenderSkinningData* skinData);

	void BindSkinningBuffer();
	const Vector& GetDefaultBindData() const { return m_defaultBindData; }

	//////////////////////////////IRenderResource/////////////////////////////
	// Get resource category
	virtual CName GetCategory() const;
	//////////////////////////////////////////////////////////////////////////

private:
	void AppendSkinningData( CRenderSkinningData* skinData );
};
