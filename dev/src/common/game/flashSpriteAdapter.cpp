/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"

#include "../../common/engine/flashMovie.h"
#include "../../common/engine/flashValueStorage.h"

#include "flashScriptFunctionCalling.h"
#include "flashBindingAdapter.h"
#include "flashSpriteAdapter.h"
#include "flashMovieAdapter.h"
#include "../engine/guiGlobals.h"

//////////////////////////////////////////////////////////////////////////
// CFlashSpriteAdapter
//////////////////////////////////////////////////////////////////////////
CFlashSpriteAdapter::CFlashSpriteAdapter( CFlashMovie* flashMovie, const CFlashValue& flashSprite, CGuiObject* adaptedObject )
{
	ASSERT( flashMovie );
	m_flashMovie = flashMovie;
	if ( m_flashMovie )
	{
		m_flashMovie->AddRef();

		m_flashMovieAdapter = static_cast< CFlashMovieAdapter* >( GetFlashMovie()->GetUserData() );
		ASSERT( m_flashMovieAdapter );
		if ( m_flashMovieAdapter )
		{
			m_flashMovieAdapter->AddRef();
		}

		m_flashValueStorage = m_flashMovie->CreateFlashValueStorage();
		ASSERT( m_flashValueStorage );
		adaptedObject->RegisterFlashValueStorage( m_flashValueStorage );
	}

	m_flashSprite = flashSprite;
	
	ASSERT( m_flashSprite.IsFlashDisplayObject() );
	if ( m_flashSprite.IsFlashDisplayObject() )
	{
		ASSERT( ! m_flashSprite.GetUserData() );
		if ( m_flashSprite.GetUserData() )
		{
			GUI_ERROR( TXT("Registering a Flash sprite multiple times!") );
		}
		m_flashSprite.SetUserData( this );
	}	

	ASSERT( adaptedObject );
	m_adaptedObject = adaptedObject;
}

Bool CFlashSpriteAdapter::Init()
{
	ASSERT( GetFlashSprite().IsFlashDisplayObject() );
	RegisterFlashFunctions( GetFlashSprite() );
	return true;
}

void CFlashSpriteAdapter::RegisterFlashFunctions( CFlashValue& flashSprite )
{
	PC_SCOPE( RegisterFlashFunctions );

	ASSERT( GetFlashMovie() );
	if ( ! GetFlashMovie() )
	{
		return;
	}

	CFlashMovieAdapter* flashMovieAdapter = static_cast< CFlashMovieAdapter* >( GetFlashMovie()->GetUserData() );
	ASSERT( flashMovieAdapter );
	if ( ! flashMovieAdapter )
	{
		return;
	}

	typedef CFlashMovieAdapter::TFlashFunctionMap TFlashFunctionMap;

	const TFlashFunctionMap& flashFunctionMap = flashMovieAdapter->GetFunctionMap();
	for ( TFlashFunctionMap::const_iterator it = flashFunctionMap.Begin(); it != flashFunctionMap.End(); ++it )
	{
		const Char* memberName = it->m_first;
		const CFlashFunction* flashFunction = it->m_second;
		ASSERT( memberName && flashFunction );

		CFlashValue checkIfExistsToAvoidScaryFlashExceptionErrorMessagesInLogFile;
		if ( ! flashSprite.GetMember( memberName, checkIfExistsToAvoidScaryFlashExceptionErrorMessagesInLogFile ) )
		{
			GUI_WARN( TXT("Failed to set member flash function '%ls'. Doesn't exist in sprite"), memberName );
		}
		else if ( ! flashSprite.SetMemberFlashFunction( memberName, *flashFunction ) )
		{
			GUI_WARN( TXT("Failed to set member flash function '%ls'"), memberName );
		}
	}
}

CFlashSpriteAdapter::~CFlashSpriteAdapter()
{
	ASSERT( m_flashValueStorage );
	if ( m_flashValueStorage )
	{
		for ( TFlashBindingAdapterList::const_iterator it = m_flashBindingAdapterList.Begin(); it != m_flashBindingAdapterList.End(); ++it )
		{
			CFlashBindingAdapter* flashBindingAdapter = *it;
			ASSERT( flashBindingAdapter );
			VERIFY( m_flashValueStorage->UnregisterFlashValueBindingHandler( flashBindingAdapter->GetKey(), flashBindingAdapter ) );
			VERIFY( flashBindingAdapter->Release() == 0 );
		}
	}

	m_flashBindingAdapterList.ClearFast();

	ASSERT( m_flashValueStorage );
	if ( m_flashValueStorage )
	{
		CGuiObject* adaptedObject = m_adaptedObject.Get();
		ASSERT( adaptedObject );
		if ( adaptedObject )
		{
			adaptedObject->UnregisterFlashValueStorage( m_flashValueStorage );
		}

		VERIFY( m_flashValueStorage->Release() == 0 );
		m_flashValueStorage = nullptr;
	}
	
	m_flashSprite.SetUserData( nullptr );
	m_flashSprite.Clear();

	ASSERT( m_flashMovieAdapter );
	if ( m_flashMovieAdapter )
	{
		m_flashMovieAdapter->Release();
		m_flashMovieAdapter = nullptr;
	}	

	ASSERT( m_flashMovie );
	if ( m_flashMovie )
	{
		m_flashMovie->Release();
		m_flashMovie = nullptr;
	}
}

void CFlashSpriteAdapter::OnDestroy()
{
}

void CFlashSpriteAdapter::RegisterDataBinding( const String& key, const CFlashValue& flashClosure, const CFlashValue& flashBoundObject, const CFlashValue& flashIsGlobal )
{
// 	ASSERT( ! m_isInTick );
// 	if ( m_isInTick )
// 	{
// 		return;
// 	}

	//ASSERT( flashMovie == m_flashMovie );

	ASSERT( m_flashValueStorage );
	if ( ! m_flashValueStorage )
	{
		return;
	}

	CFlashBindingAdapter* flashBindingAdapter = new CFlashBindingAdapter( key, m_flashValueStorage, flashClosure, flashBoundObject, flashIsGlobal );

	struct FlashBindingAdapterFinder : private Red::System::NonCopyable
	{
		const CFlashBindingAdapter& m_flashBindingAdapter;

		Bool operator()( const CFlashBindingAdapter* otherFlashBindingAdapter ) const { return otherFlashBindingAdapter && *otherFlashBindingAdapter == m_flashBindingAdapter; }

		FlashBindingAdapterFinder( const CFlashBindingAdapter& flashBindingAdapter) : m_flashBindingAdapter( flashBindingAdapter ) {}
	};

	TFlashBindingAdapterList::const_iterator findIt = ::FindIf( m_flashBindingAdapterList.Begin(), m_flashBindingAdapterList.End(), FlashBindingAdapterFinder( *flashBindingAdapter ) );
	if ( findIt != m_flashBindingAdapterList.End() )
	{
		CFlashBindingAdapter* otherFlashBindingAdapter = *findIt;
		ASSERT( otherFlashBindingAdapter );
		GUI_ERROR( TXT("Attempted to register same Flash closure/Object pair for HUD data binding key '%ls' already registered with binding key '%ls'"), key.AsChar(), otherFlashBindingAdapter->GetKey().AsChar() );
		VERIFY( flashBindingAdapter->Release() == 0 );
		return;
	}

	if( ! m_flashValueStorage->RegisterFlashValueBindingHandler( key, flashBindingAdapter ) )
	{
		GUI_ERROR( TXT("Failed to register flash value binding handler '%ls'"), key.AsChar() );
		VERIFY( flashBindingAdapter->Release() == 0 );
		return;
	}

	m_flashBindingAdapterList.PushBack( flashBindingAdapter );
}

void CFlashSpriteAdapter::UnregisterDataBinding( const String& key, const CFlashValue& flashClosure, const CFlashValue& flashBoundObject )
{

}

void CFlashSpriteAdapter::RegisterChild( const String& childName, const CFlashValue& flashChild )
{
	CGuiObject* parent = m_adaptedObject.Get();
	ASSERT( parent );
	if ( ! parent )
	{
		GUI_ERROR( TXT("GUI object doesn't exist! Can't register component '%ls'"), childName.AsChar() );
		return;
	}

	CGuiObject* child = parent->GetChild( childName );
	ASSERT( child );
	if ( ! child )
	{
		GUI_ERROR( TXT("Could not find '%ls' to register with parent"), childName.AsChar() );
		return;
	}

	if ( ! child->InitWithFlashSprite( flashChild ) )
	{
		GUI_ERROR( TXT("Failed to register Flash for child '%ls'"), childName.AsChar() );
		return;
	}
}

void CFlashSpriteAdapter::UnregisterChild( const CFlashValue& flashComponent )
{

}

void CFlashSpriteAdapter::CallGameEvent( const String& eventName, const CFlashValue& eventArgs )
{

// 	ASSERT( ! m_isInTick );
// 	if ( m_isInTick )
// 	{
// 		return;
// 	}


	CGuiObject* context = m_adaptedObject.Get();
	ASSERT( context );
	if ( ! context )
	{
		GUI_ERROR( TXT("GUI object doesn't exist! Can't call event '%ls'"), eventName.AsChar() );
		return;
	}

	//CHANGEME: Optimize...
	TDynArray< CFlashValue > fixmeEventArgs;
	if ( eventArgs.IsFlashArray() )
	{
		fixmeEventArgs.Reserve( eventArgs.GetArrayLength() );
		const Uint32 len = eventArgs.GetArrayLength();
		for ( Uint32 i = 0; i < len; ++i )
		{
			fixmeEventArgs.PushBack( CFlashValue() );
			VERIFY( eventArgs.GetArrayElement( i, fixmeEventArgs.Back() ) );
		}
	}

	//FIXME: Dynamic CName - could tag gui events and cache their cname value so have a string-name lookup.
	Flash::CallEvent( context, CName( eventName ), fixmeEventArgs );
}

void CFlashSpriteAdapter::RegisterRenderTarget( const String& sceneName, Uint32 width, Uint32 height )
{
	CGuiObject* context = m_adaptedObject.Get();
	ASSERT( context );
	if ( ! context )
	{
		GUI_ERROR( TXT("RegisterRenderTarget: GUI object doesn't exist!") );
		return;
	}

	if ( ! context->RegisterRenderTarget( sceneName, width, height ) )
	{
		GUI_ERROR( TXT("RegisterRenderTarget: Failed to create target '%ls' width=%u, height=%u"), sceneName.AsChar(), width, height );
	}
}

void CFlashSpriteAdapter::UnregisterRenderTarget( const String& sceneName )
{
	CGuiObject* context = m_adaptedObject.Get();
	ASSERT( context );
	if ( ! context )
	{
		GUI_ERROR( TXT("UnregisterRenderTarget: GUI object doesn't exist!") );
		return;
	}

	if ( ! context->UnregisterRenderTarget( sceneName ) )
	{
		GUI_ERROR( TXT("UnregisterRenderTarget: Failed to create target '%ls' width=%u, height=%u"), sceneName.AsChar() );
	}
}
