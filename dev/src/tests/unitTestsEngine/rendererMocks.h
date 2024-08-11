/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#ifndef _UNIT_TEST_ENGINE_RENDERER_MOCKS_H_
#define _UNIT_TEST_ENGINE_RENDERER_MOCKS_H_

#include "../../common/nullrender/nullRender.h"
#include "../../common/engine/renderProxy.h"
#include "../../common/engine/renderObject.h"
#include "../../common/engine/renderCommandInterface.h"

class CRenderMock : public CNullRender
{
public:

};

class CRenderObjectMock : public IRenderObject
{};

class CRenderSceneMock : public IRenderScene
{
public:
	MOCK_CONST_METHOD0( GetRenderStats, SceneRenderingStats () );

};

class CRenderCommandMock : public IRenderCommand
{
public:
	
	MOCK_METHOD0( Commit, void() );
	MOCK_METHOD0( Execute, void() );
};

const Char* GpuApi::GetShaderIncludePath() { return TXT(""); }
const Char* GpuApi::GetShaderRootPath() { return TXT(""); }

#endif
