/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "renderObject.h"

///////////////////////////////////////////////////////////

class IRender;

/// Fence in the rendering command buffer
class IRenderFence : public IRenderObject
{
protected:
	IRenderFence();
	virtual ~IRenderFence();

public:
	//! Wait for fence to be flushed
	virtual void FlushFence()=0;
};

///////////////////////////////////////////////////////////

/// Render fence helper, issues a fence and waits for it
class CRenderFenceHelper
{
private:
	IRender*		m_interface;
	IRenderFence*	m_fence;

public:
	CRenderFenceHelper( IRender* render );
	~CRenderFenceHelper();

	// Flush renderer
	void Flush();
};

///////////////////////////////////////////////////////////
