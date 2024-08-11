/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "renderSkinManager.h"
#include "renderSkinningData.h"
#include "../core/taskDispatcher.h"
#include "../core/taskRunner.h"

Bool CRenderSkinManager::m_useWorkers = true;

class CSkinManagerFlushChangesJob : public CTask
{
public:
	CSkinManagerFlushChangesJob(CRenderSkinManager* manager);
	virtual ~CSkinManagerFlushChangesJob() override;

	CRenderSkinManager* m_manager;
private:
	//! Get short debug info
	virtual const Char* GetDebugName() const { return TXT("CSkinManagerFlushChangesJob"); };

	//! Get debug color
	virtual Uint32 GetDebugColor() const { return Color::DARK_BLUE.ToUint32(); }

	//! Process the job, is called from job thread
	virtual void Run();
};

CName CRenderSkinManager::GetCategory() const
{
	return CNAME( RenderSkinManager );
}

CRenderSkinManager::CRenderSkinManager(Uint32 maxBonesPerChunk, Uint32 maxChunks)
	: m_maxBonesPerChunk( maxBonesPerChunk )
	, m_maxChunks( maxChunks )
	, m_flushChangeJob( nullptr )
	, m_lockedBuf( nullptr )
{
	m_defaultBindData = Vector( 0.f, 0.f, 0.f, 1.f);

	// Create atlas region map buffer
	{
		// Build desc
		Uint32 numElements = m_maxBonesPerChunk * m_maxChunks;
		Uint32 elementSize = sizeof(Matrix);
		GpuApi::BufferInitData bufInitData;
		bufInitData.m_buffer = nullptr;
		bufInitData.m_elementCount = numElements;
		m_skinBuffer = GpuApi::CreateBuffer( numElements * elementSize, GpuApi::BCC_Structured, GpuApi::BUT_Dynamic, GpuApi::BAF_CPUWrite, &bufInitData );
		GpuApi::SetBufferDebugPath(m_skinBuffer, "skinbuffer");
	}
}

CRenderSkinManager::~CRenderSkinManager()
{
	// Release atlas map
	GpuApi::SafeRelease( m_skinBuffer );
}

void CRenderSkinManager::Register(CRenderSkinningData* skinData)
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_mutex );
	m_resourceList.PushBack(skinData);
}

void CRenderSkinManager::Unregister(CRenderSkinningData* skinData)
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_mutex );
	VERIFY( m_resourceList.Remove(skinData), TXT("RenderSkinningData is not registered in skinmanager") );
}

void CRenderSkinManager::AppendSkinningData( CRenderSkinningData* skinData )
{
	RED_ASSERT( skinData, TXT("No skinning data provided") );
	RED_ASSERT( m_lockedBuf, TXT("The buffer needs to be locked before appending the skinning data can happen") );
	if( !skinData || !m_lockedBuf ) return;

	Int32 lockedOffset = Red::Threads::AtomicOps::ExchangeAdd32( &m_lockedOffset, skinData->GetNumMatrices() );

	RED_ASSERT( lockedOffset < (Int32)(m_maxBonesPerChunk * m_maxChunks) );
	Matrix* buf = m_lockedBuf + lockedOffset;

	//this is for vertex constant
	Vector skinInfo( (Float)lockedOffset, 1.f, 1.f, 1.f );

	const Uint32 buffEnd = (lockedOffset + skinData->GetNumMatrices()) * sizeof(Matrix);
	const Uint32 buffCapacity = m_maxBonesPerChunk * m_maxChunks * sizeof( Matrix );
	RED_ASSERT( buffEnd <= buffCapacity );
	if ( buffEnd <= buffCapacity )
	{
		skinData->Pin( skinInfo );
		Red::System::MemoryCopy( buf, skinData->GetReadData(), skinData->GetNumMatrices()*sizeof(Matrix) );
	}
}

void CRenderSkinManager::FlushChanges()
{
	PC_SCOPE_PIX( FlushChanges );

	// release mutex if there are no buffer locked
	if( !m_lockedBuf )
	{
		return;
	}

	{
		// scoped lock for skinning buffer
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_mutex );
		if( !m_resourceList.Empty() )
		{
			//fill buffer
			TDynArray<CRenderSkinningData*>::iterator iter;
			for( iter = m_resourceList.Begin();iter!=m_resourceList.End();++iter )
			{
				AppendSkinningData( (*iter) );
			}
		}
	}
}

void CRenderSkinManager::StartFlushChanges()
{
	PC_SCOPE_PIX( SkinManager_StartFlushChanges );

	RED_ASSERT(!m_lockedBuf, TXT("Buffer already locked"));

	//lock texture
	m_lockedOffset = 0;
	m_lockedBuf = static_cast<Matrix*>( GpuApi::LockBuffer( m_skinBuffer, GpuApi::BLF_Discard, 0, m_maxBonesPerChunk * m_maxChunks * sizeof(Matrix) ) );
	
	// set identity as the first element to be able to handle meshes with no skinning data
	m_lockedBuf->SetIdentity();
	m_lockedOffset = 1;

	if( m_useWorkers )
	{
		m_flushChangeJob = new ( CTask::Root ) CSkinManagerFlushChangesJob( this );
		GTaskManager->Issue( *m_flushChangeJob );
	}
	else
	{
		FlushChanges();
	}
}

void CRenderSkinManager::FinishWorker()
{
	if( m_useWorkers && m_flushChangeJob )
	{
		PC_SCOPE_PIX( SkinManger_FinishWorker );

		CTaskDispatcher taskDispatcher( *GTaskManager );
		CTaskRunner taskRunner;
		taskRunner.RunTask( *m_flushChangeJob, taskDispatcher );

		while(!m_flushChangeJob->IsFinished())
		{
			RED_BUSY_WAIT();
			//block
		}

		m_flushChangeJob->Release();
		m_flushChangeJob = nullptr;
	}
}

void CRenderSkinManager::FinishFlushChanges()
{
	PC_SCOPE_PIX( SkinManager_FinishFlushChanges );

	FinishWorker();

	if( m_lockedBuf != nullptr )
	{
		//unlock buffer
		GpuApi::UnlockBuffer( m_skinBuffer );

		// flush locked buff top null
		m_lockedBuf = nullptr;
	}
}

void CRenderSkinManager::BindSkinningBuffer()
{
	GpuApi::BindBufferSRV( m_skinBuffer, 0, GpuApi::VertexShader );
}

CSkinManagerFlushChangesJob::CSkinManagerFlushChangesJob(CRenderSkinManager* manager)
{
	m_manager = manager;
}

CSkinManagerFlushChangesJob::~CSkinManagerFlushChangesJob()
{

}

void CSkinManagerFlushChangesJob::Run()
{
	PC_SCOPE_PIX( CSkinManagerFlushChangesJob );

	m_manager->FlushChanges();
}
