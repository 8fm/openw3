/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#pragma once

/////////////////////////////////////////////////////////////////////////
#ifdef USE_SCALEFORM
//////////////////////////////////////////////////////////////////////////

#include "renderer.h"
#include "renderScaleform.h"
#include "flashMovie.h"

class IRenderScene;
class CRenderFrame;
class CScaleformTextureCacheQueue;

template
	< typename RTGuiCommandType >
class CGuiRenderCommand
{
private:
	RED_INLINE static void QueueCommand( SF::Render::ThreadCommand* command )
	{
		if ( GRender && GRender->GetRenderScaleform() )
		{
			GRender->GetRenderScaleform()->SendThreadCommand( command );
		}
	}

public:
	RED_INLINE static void Send()
	{
		QueueCommand( SF::Ptr<SF::Render::ThreadCommand>( *SF_NEW RTGuiCommandType ) );
	}

	template
		< typename A0 >
		RED_INLINE static void Send( const A0& arg0 )
	{
		QueueCommand( SF::Ptr<SF::Render::ThreadCommand>( *SF_NEW RTGuiCommandType( arg0 ) ) );
	}

	template
		< typename A0 >
		RED_INLINE static void Send( A0& arg0 )
	{
		QueueCommand( SF::Ptr<SF::Render::ThreadCommand>( *SF_NEW RTGuiCommandType( arg0 ) ) );
	}

	template
		< typename A0
		, typename A1 >
		RED_INLINE static void Send(const A0& arg0, const A1& arg1)
	{
		QueueCommand( SF::Ptr<SF::Render::ThreadCommand>( *SF_NEW RTGuiCommandType( arg0, arg1 ) ) );
	}

	template
		< typename A0
		, typename A1
		, typename A2 >
		RED_INLINE static void Send(const A0& arg0, const A1& arg1, const A2& arg2)
	{
		QueueCommand( SF::Ptr<SF::Render::ThreadCommand>( *SF_NEW RTGuiCommandType( arg0, arg1, arg2 ) ) );
	}
};

//////////////////////////////////////////////////////////////////////////

class CGuiRenderCommand_ReleaseMovie : public SF::Render::ThreadCommand
{
public:
	CGuiRenderCommand_ReleaseMovie( SF::Ptr< GFx::Movie >& flashMovieRef );

public:
	virtual void Execute();

private:
	SF::Ptr< GFx::Movie > m_flashMovie;
};

typedef CGuiRenderCommand_ReleaseMovie TReleaseMovie;

//////////////////////////////////////////////////////////////////////////

class CGuiRenderCommand_SetGlyphCacheParams : public SF::Render::ThreadCommand
{
public:
	CGuiRenderCommand_SetGlyphCacheParams( const SF::Render::GlyphCacheParams& params, volatile Bool* pResult );

public:
	virtual void Execute();

private:
	SF::Render::GlyphCacheParams	m_params;
	volatile Bool*					m_pResult;
};

typedef CGuiRenderCommand_SetGlyphCacheParams TSetGlyphCacheParams;

//////////////////////////////////////////////////////////////////////////

class CGuiRenderCommand_ShutdownSystem : public SF::Render::ThreadCommand
{
public:
	virtual void Execute();
};

typedef CGuiRenderCommand_ShutdownSystem TShutdownSystem;

//////////////////////////////////////////////////////////////////////////

class CGuiRenderCommand_HandleDeviceLost : public SF::Render::ThreadCommand
{
public:
	virtual void Execute();
};

typedef CGuiRenderCommand_HandleDeviceLost THandleDeviceLost;

//////////////////////////////////////////////////////////////////////////

class CGuiRenderCommand_HandleDeviceReset : public SF::Render::ThreadCommand
{
public:
	virtual void Execute();
};

typedef CGuiRenderCommand_HandleDeviceReset THandleDeviceReset;

//////////////////////////////////////////////////////////////////////////

class CGuiRenderCommand_AddDisplayHandle : public SF::Render::ThreadCommand
{
public:
	CGuiRenderCommand_AddDisplayHandle( const GFx::MovieDisplayHandle& handle, const SFlashMovieLayerInfo& flashMovieLayerInfo );
	virtual void Execute();

private:
	GFx::MovieDisplayHandle m_hMovieDisplay;
	SFlashMovieLayerInfo m_flashMovieLayerInfo;
};

typedef CGuiRenderCommand_AddDisplayHandle TAddDisplayHandle;

//////////////////////////////////////////////////////////////////////////

class CGuiRenderCommand_RemoveDisplayHandle : public SF::Render::ThreadCommand
{
public:
	CGuiRenderCommand_RemoveDisplayHandle( const GFx::MovieDisplayHandle& handle, const SFlashMovieLayerInfo& flashMovieLayerInfo );
	virtual void Execute();

private:
	GFx::MovieDisplayHandle m_hMovieDisplay;
	SFlashMovieLayerInfo m_flashMovieLayerInfo;
};

typedef CGuiRenderCommand_RemoveDisplayHandle TRemoveDisplayHandle;

//////////////////////////////////////////////////////////////////////////

class CGuiRenderCommand_MoveDisplayHandle : public SF::Render::ThreadCommand
{
public:
	CGuiRenderCommand_MoveDisplayHandle( const GFx::MovieDisplayHandle& handle, const SFlashMovieLayerInfo& flashMovieLayerInfo );
	virtual void Execute();

private:
	GFx::MovieDisplayHandle m_hMovieDisplay;
	SFlashMovieLayerInfo m_flashMovieLayerInfo;
};

typedef CGuiRenderCommand_MoveDisplayHandle TMoveDisplayHandle;

//////////////////////////////////////////////////////////////////////////

class CGuiRenderCommand_CreateStreamingPlaceholderTextures : public SF::Render::ThreadCommand
{
public:
	CGuiRenderCommand_CreateStreamingPlaceholderTextures( CScaleformTextureCacheQueue*	queue );
	virtual void Execute();

private:
	CScaleformTextureCacheQueue*	m_queue;
};

typedef CGuiRenderCommand_MoveDisplayHandle TMoveDisplayHandle;

/////////////////////////////////////////////////////////////////////////
#endif // USE_SCALEFORM
//////////////////////////////////////////////////////////////////////////