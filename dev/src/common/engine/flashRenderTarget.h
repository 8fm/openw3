/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "flashReference.h"

class CRenderFrame;
class CRenderFrameInfo;
class IRenderScene;
class IFlashRenderSceneProvider;

//////////////////////////////////////////////////////////////////////////
// SFlashRenderCamera
//////////////////////////////////////////////////////////////////////////
struct SFlashRenderTargetCamera
{
	DECLARE_RTTI_STRUCT( SFlashRenderTargetCamera );

	Vector		m_position;
	EulerAngles m_rotation;
	Float		m_fov;
	Float		m_zoom;

	SFlashRenderTargetCamera()
		: m_fov( 70.f )
		, m_zoom( 1.f )
	{}
};

//////////////////////////////////////////////////////////////////////////
// CFlashRenderTarget
//////////////////////////////////////////////////////////////////////////

class IRenderGameplayRenderTarget;

class CFlashRenderTarget : public IFlashReference
{
protected:
	CFlashRenderTarget();
	virtual ~CFlashRenderTarget();

public:
	virtual Bool IsValid() const=0;
	virtual void RenderScene( IFlashRenderSceneProvider* renderSceneProvider )=0;
	virtual const String& GetTargetName() const=0;

	virtual IRenderGameplayRenderTarget* GetRenderTarget() const = 0;
};

//////////////////////////////////////////////////////////////////////////
// RTTI Boilerplate
//////////////////////////////////////////////////////////////////////////
BEGIN_CLASS_RTTI( SFlashRenderTargetCamera );
	PROPERTY_RO( m_position, TXT("Camera position") );
	PROPERTY_RO( m_rotation, TXT("Camera rotation") );
	PROPERTY_RO( m_fov, TXT("Camera FOV") );
	PROPERTY_RO( m_zoom, TXT("Camera zoom") );
END_CLASS_RTTI();
