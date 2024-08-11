/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#ifndef _TEST_ENGINE_H_
#define _TEST_ENGINE_H_

#include "drawsTestUtils.h"

class CTestUtils;

class CBaseTest
{
public:
	CBaseTest( String& name );

	virtual ~CBaseTest();

	virtual Bool Initialize() = 0;

	virtual Bool Execute( CTestUtils& context ) = 0;

	RED_INLINE const String& GetName() const { return m_testName; }

protected:

	Uint32 m_stride;
	Uint32 m_offset;

	Float m_time;

	String m_testName;

	// LookAtLH view matrix
	RedMatrix4x4 LookAtLH( RedVector3 eye, RedVector3 target, RedVector3 up );
};

class CTestEngine
{
public:
	CTestEngine( const CommandParameters& params, const GpuApi::SwapChainRef& swapChainRef );
	~CTestEngine();

	// Initialize texture & stuff
	Bool Initialize();

	Bool RegisterTests();
	int Start();

private:
	CommandParameters m_params;
	GpuApi::SwapChainRef m_swapChainRef;

	TDynArray< CBaseTest* > m_tests;

	GpuApi::TextureRef m_renderTargetTexture;
	GpuApi::TextureRef m_depthStencilTexture;

	const Vector m_clearColor;
	Int32 m_exitValue;
	Int32 m_currentTest;
	Bool m_lastFrameInput;

	// Wether we are rendering to texture or to the backbuffer
	void SetRenderTargetBackBuffer();
	void SetRenderTargetTexture();

	// Load the reference texture from file
	Bool LoadTextureFromFile( const Char* path, GpuApi::TextureRef& outRef );
	
	// Save the created texture to file
	void SaveTextureToFile( GpuApi::TextureRef& ref, const String& testName );

	// Compare pixel by pixel the created texture with a reference file
	Bool CompareWithReferenceFile( const String& testName, Char* errorMsg );

	// WinAPI message based loop for runtime test
	void RuntimeLoop();

	// Single loop for automated tests
	void SingleLoop();

	void RunTests();

	void ResetViewport();
	void ResetDevice();
};

#endif