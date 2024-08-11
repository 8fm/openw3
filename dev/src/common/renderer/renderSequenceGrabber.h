/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

enum ESaveFormat : CEnum::TValueType;

class CRenderSceneEx;

/// Grabber of sequence of screenshots
class CRenderScreenshotSequnceGrabber
{
protected:
	String		m_baseName;		//!< Base name of screenshot
	String		m_pattern;		//!< Screenshot naming pattern
	Uint32		m_frameIndex;	//!< Index of frame
	ESaveFormat	m_saveFormat;	//!< format to save frame to (PNG, DDS, BMP)
	Bool		m_useUbersampling;

public:
	CRenderScreenshotSequnceGrabber( ESaveFormat saveFormat, Bool ubersampling );
	~CRenderScreenshotSequnceGrabber();

	//! Grab screenshot
	void GrabScreenshot( CRenderFrame* frame, CRenderSceneEx* scene, Bool flushRender );

	String GetBaseName() const;
	String GetPattern() const;
	Uint32 GetFrameIndex() const;
};

extern CRenderScreenshotSequnceGrabber* GScreenshotSequence;
