/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "flashMovie.h"

#ifdef USE_SCALEFORM

class CFlashPlayerScaleform;
class CFlashFunctionScaleform;
class IScaleformPlayer;

enum EFlashMovieFlags
{
	eFlashMovieFlags_None					= 0,
	eFlashMovieFlags_AttachOnStart			= FLAG(0),
	eFlashMovieFlags_NotifyPlayer			= FLAG(1),
};

class CFlashMovieScaleform
	: public CFlashMovie
{
	friend class CFlashFunctionScaleform;
	friend class CFlashObjectScaleform;
	friend class CFlashArrayScaleform;
	friend class CFlashStringScaleform;
	friend class CFlashValueStorageScaleform;

private:
	typedef CFlashMovie TBaseClass;

private:
	IScaleformPlayer*			m_parentWeakRef;
	SF::Ptr< GFx::Movie >		m_gfxMovie;
	Uint32						m_flags;

	// A bit of a hack, since movies on the render thread don't need lock-step update and nothing needs to explicitly call capture on them
	Bool						m_forceEnableCaptureOnAdvance;

public:
	CFlashMovieScaleform( IScaleformPlayer* parent, GFx::Movie* gfxMovie, const SFlashMovieLayerInfo& layerInfo, Uint32 flags );

protected:
	virtual ~CFlashMovieScaleform();

public:
	virtual void Tick( Float timeDelta ) override;
	virtual void Capture( Bool force ) override;
	virtual void SetViewport( const Rect& viewport ) override;

public:
 	void OnGFxKeyEvent( const GFx::KeyEvent& event );
 	void OnGFxMouseEvent( const GFx::MouseEvent& event );
 	void OnGFxGamePadAnalogEvent( const GFx::GamePadAnalogEvent& event );

public:
	virtual void				Attach() override;
	virtual void				Detach() override;
	virtual void				OnLayerInfoChanged( const SFlashMovieLayerInfo& layerInfo ) override;

public:
	virtual void				SetBackgroundColor( const Color& color ) override;
	virtual void				SetBackgroundAlpha( Float alpha ) override;
	virtual Float				GetBackgroundAlpha() const override;

public:
	virtual CFlashFunction*		CreateFunction( CFlashFunctionHandler* flashFunctionHandler ) override;
	virtual CFlashObject*		CreateObject( const String& flashClassName ) override;
	virtual CFlashArray*		CreateArray() override;
	virtual CFlashString*		CreateString( const String& value ) override;
	virtual CFlashValueStorage*	CreateFlashValueStorage() override;
	virtual CFlashRenderTarget*	CreateRenderTarget( const String& targetName, Uint32 width, Uint32 height ) override;

public:
	virtual void				UpdateCaptureThread() override;

public:
	SF::GFx::Resource*			GetGFxResource( const String& resourceName ) const;
	void						ForceUpdateGFxImages();

private:
	void DoAttach();
	void DoDetach();
};

#endif // USE_SCALEFORM