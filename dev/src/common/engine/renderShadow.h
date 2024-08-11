/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CRenderFrame;

/// Projected shadow info
class CRenderShadow
{
	friend class CDX9Render;
	friend class CXenonRender;
	friend class CRenderInterface;

protected:
	CRenderFrame*		m_parentFrame;		//!< Owner
	CRenderFrame*		m_shadowFrame;		//!< Shadow rendering frame
	Box					m_casterBox;		//!< Bounding box of shadow caster
	Uint32				m_resolution;		//!< Resolution to use

public:
	CRenderShadow( CRenderFrame* parentFrame, CRenderFrame* shadowFrame, const Box& casterBox, Uint32 resolution );
	~CRenderShadow();

	// Get shadow rendering frame
	RED_INLINE CRenderFrame* GetShadowFrame() const { return m_shadowFrame; }

	// Get shadow resolution
	RED_INLINE Uint32 GetResolution() const { return m_resolution; }
};