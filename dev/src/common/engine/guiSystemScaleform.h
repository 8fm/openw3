/**
 * Copyright © 2012 CD Projekt Red. All Rights Reserved.
 */

#pragma once

//////////////////////////////////////////////////////////////////////////
#ifdef USE_SCALEFORM
//////////////////////////////////////////////////////////////////////////

#include "guiSystemScaleform.h"
#include "scaleformMemory.h"
#include "guiDesc.h"

//////////////////////////////////////////////////////////////////////////

#define USE_GFX_VIDEO
//#define USE_GFX_WWISE FIXME: Scaleform needs recompiling and their project fixing.

#if defined ( W2_PLATFORM_WINBOX )
#	define USE_GFX_XA2
#endif


#if !defined( NO_GFX_AS2 )
#	define USE_GFX_AS2
#endif

#if !defined( NO_GFX_AS3 )
#	define USE_GFX_AS3
#endif

class CGuiSystemScaleform
	: public CGuiSystem
	, private GFx::System
{
private:
	static CScaleformSysAlloc	ms_scaleformAllocator;

private:
	struct KeyInput
	{
		GFx::Event::EventType	m_eventType;
		SF::Key::Code			m_keyCode;
	};

	struct MouseInput
	{
		GFx::Event::EventType	m_eventType;
		Uint32					m_button;
		Float					m_scrollValue;
	};

	struct AnalogInput
	{
		SF::PadKeyCode			m_padKeyCode;
	};

	struct InputQueueEntry
	{
		enum Type
		{
			InputTypeKey,
			InputTypeMouse,
			InputTypeAnalog,
		};

		union Input
		{
			KeyInput	m_keyInput;
			MouseInput	m_mouseInput;
			AnalogInput m_analogInput;
		};

		Input	u;
		Type	m_type;

		InputQueueEntry() {}
		InputQueueEntry( const KeyInput& keyInput ) : m_type( InputTypeKey ) { u.m_keyInput = keyInput; }
		InputQueueEntry( const MouseInput& mouseInput ) : m_type( InputTypeMouse ) { u.m_mouseInput = mouseInput; }
		InputQueueEntry( const AnalogInput& analogInput ) : m_type( InputTypeAnalog ) { u.m_analogInput = analogInput; }
	};

private:
	struct SAxisPerTickData
	{
		Vector2 m_leftStick; 
		Vector2 m_rightStick;
		Float	m_leftTrigger;
		Float	m_rightTrigger;

		SAxisPerTickData()
			: m_leftStick( 0.0f, 0.0f )
			, m_rightStick( 0.0f, 0.0f )
			, m_leftTrigger( 0.0f )
			, m_rightTrigger( 0.0f )
		{
		}

		FORCE_INLINE void Clear() { m_leftStick.X = m_leftStick.Y = m_rightStick.X = m_rightStick.Y = m_leftTrigger = m_rightTrigger = 0.0f; }
	};

	struct SMouseState
	{
		Float m_x;
		Float m_y;

		SMouseState() : m_x( 0.f ), m_y( 0.f ) {}
		FORCE_INLINE void Clear() { m_x = m_y = 0.f; }
	};

	struct SKeyModifierState
	{
		SF::KeyModifiers m_modifiers;

		//TBD: Can add other modifiers if really needed
		FORCE_INLINE void SetShiftPressed( Bool value )  { m_modifiers.SetShiftPressed( value ); }
		FORCE_INLINE void SetCtrlPressed( Bool value )   { m_modifiers.SetCtrlPressed( value ); }
		FORCE_INLINE void SetAltPressed( Bool value )    { m_modifiers.SetAltPressed( value ); }
		FORCE_INLINE SF::KeyModifiers GetModifiers() const { return m_modifiers; }
		FORCE_INLINE void Clear() { m_modifiers.Reset(); }
	};

	struct SViewport
	{
		Uint32 m_x;
		Uint32 m_y;
		Uint32 m_width;
		Uint32 m_height;

		SViewport() : m_x( 0 ), m_y( 0 ), m_width( 0 ), m_height( 0 ) {}
	};

private:
	TDynArray< InputQueueEntry > m_inputQueue;

private:
	struct SMovieDesc
	{
		SF::Ptr<GFx::Movie> m_movie;
		Float m_swfWidth;
		Float m_swfHeight;
	};

	struct SAllocHandle
	{
		SAllocHandle()
			: m_guiDesc( 0 )
			, m_callAdvance( false )
		{
		}
		class CGuiDesc*						m_guiDesc;
		Bool								m_callAdvance;
		SF::Array< SMovieDesc >	m_movies;
	};

private:
	SMouseState			m_mouseState;
	SKeyModifierState	m_keyModifierState;
	SAxisPerTickData	m_axisPerTickData;

	SViewport			m_viewport;

	typedef SF::Array<SAllocHandle> TAllocHandleArray;
	TAllocHandleArray m_allocHandles;

private:
	Bool m_hasTextureManager;
	Bool m_drawTextRendering;
	GFx::Loader m_defaultLoader;
	SF::Ptr< GFx::DrawTextManager > m_drawTextManager;

private:
	struct SAnalogEventFinder
	{
		SF::PadKeyCode m_padKeyCode;

		SAnalogEventFinder( SF::PadKeyCode padKeyCode )
				: m_padKeyCode( padKeyCode )
		{}

		Bool operator()( const InputQueueEntry& other ) const
		{
			if ( other.m_type == InputQueueEntry::InputTypeAnalog )
			{
				SF::PadKeyCode otherPadKeyCode = other.u.m_analogInput.m_padKeyCode;
				return m_padKeyCode == otherPadKeyCode;
			}
			return false;
		}
	};

private:
	FORCE_INLINE void QueueKeyboardInput( GFx::Event::EventType eventType, SF::Key::Code keyCode );
	FORCE_INLINE void QueueMouseInput( GFx::Event::EventType eventType, Uint32 button, Float scrollValue = 0.0f );	
	void QueueAnalogInput( SF::PadKeyCode padKeyCode );	
	void ProcessInput();
	void ClearInput();

private:
	void UpdateViewportLayout();
	void HandleMouseEvent( const GFx::MouseEvent& event );
	void HandleKeyEvent( const GFx::KeyEvent& event );
	void HandleGamePadAnalogEvent( const GFx::GamePadAnalogEvent& event );
	void RegisterFonts();

public:
	CGuiSystemScaleform();
	virtual ~CGuiSystemScaleform();

	virtual Bool OnViewportTrack( Int32 x, Int32 y );
	virtual Bool OnViewportInput( EInputKey key, EInputAction action, Float data );
	virtual void OnViewportSetDimensions( Uint32 x, Uint32 y, Uint32 width, Uint32 height );
	virtual void OnViewportGenerateFragments( CRenderFrame* frame );

	virtual Bool Init();
	virtual void Tick( Float timeDelta );
	virtual void Shutdown();
	virtual class CGuiBoundObject* CreateBoundObject( class CGuiDesc* guiDesc );
	virtual void DestroyBoundObject( class CGuiBoundObject* boundObject );
	virtual void PauseBoundObject( class CGuiBoundObject* boundObject );
	virtual void UnpauseBoundObject( class CGuiBoundObject* boundObject );
	virtual void GetMovieRootLoadParams( const class CGuiBoundObject* boundObject, TDynArray< SGuiMovieRootLoadParam >& loadParams /*[out]*/ );

	virtual GFx::DrawTextManager* GetDrawTextManager();

#ifndef FINAL
public:
	static void DebugAbortLoading();
#endif

private:
	void InitDrawTextManager();
	Bool InitTextureManager();
};

//////////////////////////////////////////////////////////////////////////
#endif // USE_SCALEFORM
//////////////////////////////////////////////////////////////////////////
