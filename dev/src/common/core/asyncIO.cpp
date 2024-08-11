/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "profiler.h"

#include "asyncIO.h"
#include "asyncLoadToken.h"
#include "jobGenericJobs.h"
#include "fileSys.h"
#include "loadingJobManager.h"

#define FATAL_ASSERT(x) RED_FATAL_ASSERT( (x), #x )

class CAsyncIOJob : public CJobLoadData
{
private:
	struct SInitInfo : JobLoadDataInitInfo
	{
		SInitInfo( CAsyncLoadToken* asyncLoadToken, IFile* file )
		{
			m_sourceFile = file;
			m_offset = asyncLoadToken->GetFileOffset();
			m_size = asyncLoadToken->GetFileReadSize();
			m_buffer = asyncLoadToken->GetDestinationBuffer();
		}
	};

private:
	CAsyncLoadToken* m_asyncLoadToken;

	virtual EJobResult Process()
	{
		const EJobResult processResult = CJobLoadData::Process();
		const CAsyncLoadToken::EAsyncResult asyncResult = processResult == JR_Finished ? 
							CAsyncLoadToken::EAsyncResult::eAsyncResult_Success :
							CAsyncLoadToken::EAsyncResult::eAsyncResult_Error;

		m_asyncLoadToken->Signal( asyncResult );

		return processResult;
	}

public:
	CAsyncIOJob( CAsyncLoadToken* asyncLoadToken, IFile* file )
		: CJobLoadData( SInitInfo(asyncLoadToken, file ) )
		, m_asyncLoadToken( asyncLoadToken )
	{
		asyncLoadToken->AddRef();
	}

	~CAsyncIOJob()
	{
		m_asyncLoadToken->Release();
	}
};

//////////////////////////////////////////////////////////////////////////
// CAsyncIO
//////////////////////////////////////////////////////////////////////////
CDeprecatedIO* GDeprecatedIO;

CDeprecatedIO::CDeprecatedIO()
{
}

CDeprecatedIO::~CDeprecatedIO()
{
}

Bool CDeprecatedIO::Init()
{
	return true;
}

void CDeprecatedIO::Shutdown()
{
}

void CDeprecatedIO::LoadAsync( CAsyncLoadToken* asyncLoadToken )
{

	IFile* file = GFileManager->CreateFileReader( asyncLoadToken->GetAbsoluteFilePath(), FOF_Buffered | FOF_AbsolutePath );
	if ( ! file )
	{
		ERR_CORE(TXT("Failed to open %ls for reading!"), asyncLoadToken->GetAbsoluteFilePath().AsChar() );
	}
	else
	{
		CJobLoadData* job = new CAsyncIOJob( asyncLoadToken, file );
		SJobManager::GetInstance().Issue( job );
		job->Release();
	}
}

void CDeprecatedIO::TryCancel( CAsyncLoadToken* asyncLoadToken )
{
	RED_UNUSED( asyncLoadToken );
}
