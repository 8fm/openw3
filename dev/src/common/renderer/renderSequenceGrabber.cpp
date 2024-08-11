/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "renderSequenceGrabber.h"
#include "renderViewport.h"
#include "../engine/screenshotSystem.h"
#include "../engine/renderFrame.h"

CRenderScreenshotSequnceGrabber* GScreenshotSequence = NULL;

CRenderScreenshotSequnceGrabber::CRenderScreenshotSequnceGrabber( ESaveFormat saveFormat, Bool ubersampling )
	: m_frameIndex( 0 )
	, m_saveFormat( saveFormat )
	, m_useUbersampling( ubersampling )
{
	static Uint32 takeIndex = 0;

	String extension;
	switch ( m_saveFormat )
	{
	case SF_BMP: extension = TXT(".bmp"); break;
	case SF_PNG: extension = TXT(".png"); break;
	case SF_DDS: extension = TXT(".dds"); break;
	default:
		break;
	}

	// Find take index
	while ( takeIndex < 1000 )
	{
		// Test take index
		String path = String::Printf( TXT("screenshots\\take%i\\frame0000.png"), takeIndex );
		String path2 = String::Printf( TXT("screenshots\\take%i\\frame0000.bmp"), takeIndex );
		String path3 = String::Printf( TXT("screenshots\\take%i\\frame0000.dds"), takeIndex );
		if ( (GFileManager->GetFileSize( path ) == 0) && (GFileManager->GetFileSize( path2 ) == 0) && (GFileManager->GetFileSize( path3 ) == 0) )
		{
			// No such take
			String fullPath = GFileManager->GetBaseDirectory() + path;
			GFileManager->CreatePath( fullPath );
			break;
		}

		// Go to next take
		takeIndex++;
	}

	// Assemble base name
	m_baseName = String::Printf( TXT("screenshots\\take%i\\"), takeIndex );
	m_pattern = TXT("frame%04d") + extension;
	LOG_RENDERER( TXT("Starting contingnous capture to '%ls'"), m_baseName.AsChar() );
}

CRenderScreenshotSequnceGrabber::~CRenderScreenshotSequnceGrabber()
{
	// Done
	LOG_RENDERER( TXT("Ending contingnous capture") );
}

void CRenderScreenshotSequnceGrabber::GrabScreenshot( CRenderFrame* frame, CRenderSceneEx* scene , Bool flushRender )
{
	// Grab screenshot only if screenshot grabbing is allowed in the frame
	if ( !frame->GetFrameInfo().m_allowSequentialCapture )
	{
		return;
	}

	// Grab normal screenshot
	String extension;
	switch ( m_saveFormat )
	{
		case SF_BMP: extension = TXT("bmp"); break;
		case SF_DDS: extension = TXT("dds"); break;
		case SF_PNG: extension = TXT("png"); break;
	default:
		ERR_RENDERER( TXT("Invalid save format. Frame will not be saved.") );
		return;
	}

	String frameName = String::Printf( TXT("%sframe%04i.%s"), m_baseName.AsChar(), m_frameIndex, extension.AsChar() );

	// Don't want to override fov
	Float fov = frame->GetFrameInfo().m_camera.GetFOV();

	Uint32 superSamplingSize = 4;

	if ( m_useUbersampling )
	{
		// temp buffer used by TakeOneUberScreenshot.
		Uint32* buffer = (Uint32*)RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_Temporary, frame->GetFrameInfo().m_width * frame->GetFrameInfo().m_height * sizeof( Uint32 ) * 3 );

		SScreenshotParameters parameters( frame->GetFrameInfo().m_width, frame->GetFrameInfo().m_height, frameName, m_saveFormat, superSamplingSize, fov, SRF_PlainScreenshot, buffer, true );

		Bool status = false;
		GetRenderer()->TakeOneUberScreenshot( frame, scene, parameters, &status );

		RED_MEMORY_FREE( MemoryPool_Default, MC_Temporary, buffer );
	}
	else
	{
		SScreenshotParameters parameters( frame->GetFrameInfo().m_width, frame->GetFrameInfo().m_height, frameName, m_saveFormat, superSamplingSize, fov, SRF_PlainScreenshot, NULL, true );

		GetRenderer()->TakeOneRegularScreenshotNow( parameters );
	}
	
	// Advance
	m_frameIndex++;
}

String CRenderScreenshotSequnceGrabber::GetBaseName() const
{
	return m_baseName;
}

String CRenderScreenshotSequnceGrabber::GetPattern() const
{
	return m_pattern;
}

Uint32 CRenderScreenshotSequnceGrabber::GetFrameIndex() const
{
	return m_frameIndex;
}