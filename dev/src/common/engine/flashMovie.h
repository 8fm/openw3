/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "flashReference.h"
#include "inputKeys.h"

//////////////////////////////////////////////////////////////////////////
// Forward declarations
//////////////////////////////////////////////////////////////////////////
class CFlashObject;
class CFlashArray;
class CFlashString;
class CFlashValueStorage;
class CFlashFunction;
class CFlashFunctionHandler;
class CFlashRenderTarget;
class CFlashValue;
class IFlashExternalInterfaceHandler;
struct Rect;

enum EFlashMovieRenderGroup
{
	//! Use this if managing layer depths is enough.
	//! Invisible during the loading screen.
	eFlashMovieRenderGroup_Default,
	
	//! Under everything, like background video
	//! Visible during the loading screen
	eFlashMovieRenderGroup_Underlay,

	//! Over everything in game, including the blackscreen fade.
	//! Visible during the loading screen. In fact, this is the loading screen.
	//! Don't use for popups or other generic UI elements.
	eFlashMovieRenderGroup_Overlay,
};

struct SFlashMovieLayerInfo
{
	EFlashMovieRenderGroup	m_renderGroup;
	Int32					m_layerDepth;

	SFlashMovieLayerInfo()
		: m_renderGroup( eFlashMovieRenderGroup_Default )
		, m_layerDepth( 0 )
	{}

	explicit SFlashMovieLayerInfo( Int32 layerDepth )
		: m_renderGroup( eFlashMovieRenderGroup_Default )
		, m_layerDepth( layerDepth )
	{}

	explicit SFlashMovieLayerInfo( EFlashMovieRenderGroup renderGroup )
		: m_renderGroup( renderGroup )
		, m_layerDepth( 0 )
	{}

	SFlashMovieLayerInfo( EFlashMovieRenderGroup renderGroup, Int32 layerDepth )
		: m_renderGroup( renderGroup )
		, m_layerDepth( layerDepth )
	{}

	Bool operator==( const SFlashMovieLayerInfo& rhs ) const
		{ return m_renderGroup == rhs.m_renderGroup && m_layerDepth == rhs.m_layerDepth; }

	Bool operator!=( const SFlashMovieLayerInfo& rhs ) const
		{ return !(*this == rhs); }
};

enum EFlashMovieOverlayDepths
{
	eFlashMovieOverlayDepth_LoadingScreen		= 0,
	eFlashMovieOverlayDepth_LoadingOverlay		= 1,
	eFlashMovieOverlayDepth_LoadingVideo		= 2,
};

enum EFlashMovieUnderlayDepths
{
	eFlashMovieUnderlayDepth_VideoPlayer		= 0,
};

//////////////////////////////////////////////////////////////////////////
// CFlashMovie
//////////////////////////////////////////////////////////////////////////
class CFlashMovie : public IFlashReference
{
	friend class CFlashPlayer;

public:
	enum EStatus
	{
		Uninitialized,
 		Failed,
 		Canceled,
 		Loading,
		Detached,
		Playing,
	};

public:
	enum EInputSourceFlags
	{
		ISF_None		= 0,
		ISF_Mouse		= FLAG(0),
		ISF_Key			= FLAG(1),
		ISF_Analog		= FLAG(2),

		ISF_All			= ISF_Mouse | ISF_Key | ISF_Analog,
	};

private:
	SFlashMovieLayerInfo	m_layerInfo;
	void*					m_userData;
	Uint32					m_inputSourceFlags;
	Uint32					m_catchupFrames;

private:
	TBitSet< EInputKey::IK_Count > m_ignoreKeys;

private:
	IFlashExternalInterfaceHandler* m_externalInterfaceOverride;

protected:
	EStatus m_status;

protected:
	CFlashMovie( const SFlashMovieLayerInfo& layerInfo, Uint32 flags = ISF_All );
	virtual ~CFlashMovie() {}

public:
	RED_INLINE EStatus		GetStatus() const { return m_status; }
	RED_INLINE void			SetUserData( void* userData ) { m_userData = userData; }
	RED_INLINE void*		GetUserData() const { return m_userData; }
	RED_INLINE Uint32		GetCatchupFrames() const { return m_catchupFrames; }
	RED_INLINE void			SetCatchupFrames( Uint32 catchupFrames ) { m_catchupFrames = catchupFrames; }

public:
	IFlashExternalInterfaceHandler* GetExternalInterfaceOverride() const { return m_externalInterfaceOverride; }
	void							SetExternalInterfaceOverride( IFlashExternalInterfaceHandler* externalInterfaceOverride )
	{ m_externalInterfaceOverride = externalInterfaceOverride; }

public:
	RED_INLINE void			SetInputSourceFlags( Uint32 flags ) { m_inputSourceFlags = flags; }
	RED_INLINE Bool			IsInputSourceEnabled( Uint32 flags ) const { return (m_inputSourceFlags & flags ) != 0; }
	
	void						SetIgnoreKeys( const TDynArray< EInputKey >& keysToSet );
	void						ClearIgnoreKeys( const TDynArray< EInputKey >& keysToClear );

	RED_INLINE void			SetIgnoreKey( EInputKey keyToSet ) { m_ignoreKeys.Set( keyToSet ); }
	RED_INLINE void			ClearIgnoreKey( EInputKey keyToClear ) { m_ignoreKeys.Clear( keyToClear ); }
	RED_INLINE void			ClearAllIgnoreKeys() { m_ignoreKeys.ClearAll(); }
	RED_INLINE Bool			IsKeyIgnored( EInputKey key ) const { return m_ignoreKeys.Get( key ); }

public:
	RED_INLINE const SFlashMovieLayerInfo& GetLayerInfo() const { return m_layerInfo; }
	void						SetLayerInfo( const SFlashMovieLayerInfo& layerInfo );

public:
	virtual void				Tick( Float timeDelta )=0;
	virtual void				Capture( Bool force = false )=0;
	virtual void				SetViewport( const Rect& viewport )=0;

public:
	virtual void				Attach()=0;
	virtual void				Detach()=0;

public:
	virtual void				SetBackgroundColor( const Color& color )=0;
	virtual void				SetBackgroundAlpha( Float alpha )=0;
	virtual Float				GetBackgroundAlpha() const=0;

public:
	virtual void				UpdateCaptureThread()=0;

public:
	virtual CFlashFunction*		CreateFunction( CFlashFunctionHandler* flashFunctionHandler )=0;
	virtual CFlashObject*		CreateObject( const String& flashClassName )=0;
	virtual CFlashArray*		CreateArray()=0;
	virtual CFlashString*		CreateString( const String& value )=0;
	virtual CFlashValueStorage*	CreateFlashValueStorage()=0;
	virtual CFlashRenderTarget*	CreateRenderTarget( const String& targetName, Uint32 width, Uint32 height )=0;

protected:
	virtual void OnLayerInfoChanged( const SFlashMovieLayerInfo& layerInfo ) { RED_UNUSED(layerInfo); }
};

