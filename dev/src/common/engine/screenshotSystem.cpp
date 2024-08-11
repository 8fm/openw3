#include "build.h"
#include "screenshotSystem.h"
#include "renderer.h"
#include "renderFrameInfo.h"

IMPLEMENT_RTTI_ENUM( EScreenshotRenderFlags );
IMPLEMENT_RTTI_ENUM( ESaveFormat );

CScreenshotSystem::CScreenshotSystem()
	: m_currentRequest( NULL )
	, m_currentRequestFinished( false )
{
	m_lastScreenshotNumber = GetLastScreenshotNumber();
}

CScreenshotSystem::~CScreenshotSystem()
{
}

void CScreenshotSystem::TakeScreenshot( const SScreenshotParameters& parameters, Bool* requestFinished, Bool& status )
{
	/*
	if ( parameters.IsUberScreenshotRequired() )
	{
		if ( requestFinished )
		{
			*requestFinished = true;
		}
	}
	else
	{*/
		GRender->TakeScreenshot( parameters, requestFinished, status );
	//}	
}

String CScreenshotSystem::GetNextScreenshotName( const String& extension )
{
	return String::Printf( TXT("%sscreenshots\\screenshot%d.%s"), GFileManager->GetBaseDirectory().AsChar(), m_lastScreenshotNumber++, extension.AsChar() );
}

Uint32 CScreenshotSystem::GetLastScreenshotNumber()
{
	static TDynArray< String > extensions;
	extensions.PushBack( TXT("*.bmp") );
	extensions.PushBack( TXT("*.png") );
	extensions.PushBack( TXT("*.jpg") );
	extensions.PushBack( TXT("*.dds") );
	
	// Get all existing screenshots
	TDynArray< String > screenPaths;
	GFileManager->CreatePath( GFileManager->GetBaseDirectory() + TXT( "screenshots\\" ) );
	for ( Uint32 i = 0; i < extensions.Size(); ++i )
	{
		GFileManager->FindFiles( GFileManager->GetBaseDirectory() + TXT( "screenshots\\" ), extensions[ i ].AsChar(), screenPaths, false );
	}

	// Find index of last screenshot
	Int32 lastScreenNum = 0;
	for( Uint32 i = 0; i < screenPaths.Size(); ++i )
	{
		CFilePath path ( screenPaths[ i ] );
		const String& filename = path.GetFileName();

		Int32 startIndex = path.GetFileName().Size() - 2; // (path.GetFileName().Size() - 1) gives the string termination character (NULL, 0), so start from the last ACTUAL character
		Uint32 numPartLength = 0;
		while ( startIndex >= 0 && IsNumber( filename[ startIndex ] ) )
		{
			--startIndex;
			++numPartLength;
		}

		String numString = path.GetFileName().RightString( numPartLength );

		Int32 num = 0;
		if ( FromString( numString, num ) )
		{
			if ( num > lastScreenNum )
			{
				lastScreenNum = num;
			}
		}
	}

	return lastScreenNum + 1; // lastScreenNum is already present in the directory, choose the next one
}

void CScreenshotSystem::TakeSimpleScreenshot( const SScreenshotParameters& parameters, Uint32 saveFlags )
{
#ifdef RED_PLATFORM_CONSOLE
	RED_ASSERT( ( saveFlags & SCF_SaveToBuffer ) != 0, TXT("Saving to file is not supported on the consoles") );
	RED_ASSERT( parameters.m_buffer, TXT("No buffer allocated for screenshot") );
#endif

	GRender->TakeQuickScreenshot( parameters );
}

void CScreenshotSystem::TakeSimpleScreenshotNow( const SScreenshotParameters& parameters, Uint32 saveFlags )
{
#ifdef RED_PLATFORM_CONSOLE
	RED_ASSERT( ( saveFlags & SCF_SaveToBuffer ) != 0, TXT("Saving to file is not supported on the consoles") );
	RED_ASSERT( parameters.m_buffer, TXT("No buffer allocated for screenshot") );
#endif

	GRender->TakeQuickScreenshotNow( parameters );
}

void CScreenshotSystem::RequestScreenshot( const SScreenshotParameters& parameters, Uint32 saveFlags, Uint32 renderFlags, Functor4< void, const String&, const void*, Bool, const String& >& callback )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_requestMutex );

	SScreenshotRequest request;
	request.m_parameters	= parameters;
	request.m_saveFlags		= saveFlags;
	request.m_renderFlags	= renderFlags;
	request.m_callback		= callback;

	m_requestsQueue.Push( request );
}

String FormatToExtension( ESaveFormat format = SF_BMP )
{
	switch ( format )
	{
		case SF_BMP: return TXT( "bmp" );
		case SF_DDS: return TXT( "dds" );
		case SF_JPG: return TXT( "jpg" );
		case SF_PNG: return TXT( "png" );
	}

	ASSERT( false, TXT("Unknown format!") );
	return TXT("");
}

void CScreenshotSystem::Tick()
{
	if ( IsTakingScreenshot() )
	{
		if ( m_currentRequestFinished )
		{
			CurrentRequestFinished();
		}
	}

	if ( !m_requestsQueue.Empty() )
	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_requestMutex );

		SScreenshotRequest& request = m_requestsQueue.Front();
		IssueRequest( request );
		request.m_callback.Release();
		m_requestsQueue.Pop();
	}
}

void CScreenshotSystem::Flush()
{
	while ( !m_requestsQueue.Empty() )
	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_requestMutex );

		SScreenshotRequest request = m_requestsQueue.Front();
		m_requestsQueue.Pop();

		IssueRequest( request );

		GRender->Flush();

		CurrentRequestFinished();
	}
}

EShowFlags Map( Uint32 renderFlag )
{
	switch ( renderFlag )
	{
	case SRF_PostProcess:	return SHOW_PostProcess;
	case SRF_GUI:			return SHOW_GUI;
	case SRF_Debug:			return SHOW_VisualDebug;
	}

	ASSERT( false, TXT("Invalid render flag to map") );
	return SHOW_MAX_INDEX;
}


void CScreenshotSystem::OverrideShowFlags( CRenderFrameInfo& frameInfo )
{
	if ( IsTakingScreenshot() )
	{
		ASSERT( m_currentRequest );
		
		const CEnum* srfEnum = SRTTI::GetInstance().FindEnum( CNAME( EScreenshotRenderFlags ) );
		if ( srfEnum )
		{
			const TDynArray< CName >& srfNames = srfEnum->GetOptions();

			for ( Uint32 i = 0; i < srfNames.Size(); ++i )
			{
				const CName& srfFlagName = srfNames[ i ];
				
				Int32 srfFlagValue;
				if ( srfEnum->FindValue( CName( srfFlagName ), srfFlagValue ) )
				{
					Bool isSet = ( m_currentRequest->m_renderFlags & srfFlagValue ) != 0;
					frameInfo.SetShowFlag( Map( srfFlagValue ), isSet );
				}
			}
		}
	}
}

void CScreenshotSystem::IssueRequest( SScreenshotRequest& request )
{
	// store copy - the reference from parameter will go out of scope until OnFinished is called
	m_currentRequest = new SScreenshotRequest( request );
	m_currentRequestFinished = false;

	// 3 channels of Uint32
	size_t bufferSize = m_currentRequest->m_parameters.m_width * m_currentRequest->m_parameters.m_height * sizeof( Uint32 ) * 3;
	m_currentRequest->m_parameters.m_buffer = (Uint32*)RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_Temporary, bufferSize );

	if ( m_currentRequest->m_saveFlags & SCF_SaveToDisk )
	{
		if ( m_currentRequest->m_saveFlags & SCF_UseProvidedFilename )
		{
			ASSERT( !m_currentRequest->m_parameters.m_fileName.Empty() );
		}
		else
		{
			// generate filename
			m_currentRequest->m_parameters.m_fileName = GetNextScreenshotName( FormatToExtension( m_currentRequest->m_parameters.m_saveFormat ) );
		}
	}
	else if ( m_currentRequest->m_saveFlags & SCF_SaveToBuffer )
	{
	}

	TakeScreenshot( m_currentRequest->m_parameters, &m_currentRequestFinished, m_currentRequest->m_status );
}

void CScreenshotSystem::CurrentRequestFinished()
{
	void* buffer = NULL;
	if ( m_currentRequest->m_saveFlags & SCF_SaveToDisk )
	{
		m_currentRequest->m_callback( m_currentRequest->m_parameters.m_fileName, NULL, m_currentRequest->m_status, TXT("") );
	}
	else if ( m_currentRequest->m_saveFlags & SCF_SaveToBuffer )
	{
		m_currentRequest->m_callback( TXT(""), buffer, m_currentRequest->m_status, TXT("") );
	}

	RED_MEMORY_FREE( MemoryPool_Default, MC_Temporary, m_currentRequest->m_parameters.m_buffer );
	m_currentRequest->m_parameters.m_buffer = NULL;

	m_currentRequestFinished = true;

	delete m_currentRequest;
	m_currentRequest = NULL;
}