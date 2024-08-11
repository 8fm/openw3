/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"

#include "../../common/engine/flashMovie.h"
#include "../../common/engine/flashValueStorage.h"
#include "../../common/engine/flashValue.h"

#include "flashSpriteAdapter.h"

#include "flashMovieAdapter.h"
#include "../engine/guiGlobals.h"

//////////////////////////////////////////////////////////////////////////
// CFlashMovieAdapter
//////////////////////////////////////////////////////////////////////////
CFlashMovieAdapter::SFlashFuncExportDesc CFlashMovieAdapter::sm_flashFunctionExportTable[] = {
	{ TXT("_NATIVE_registerDataBinding"), &CFlashMovieAdapter::flFnRegisterDataBinding, false },
	{ TXT("_NATIVE_unregisterDataBinding"), &CFlashMovieAdapter::flFnUnregisterDataBinding, false },
	{ TXT("_NATIVE_registerChild"), &CFlashMovieAdapter::flFnRegisterChild, false },
	{ TXT("_NATIVE_unregisterChild"), &CFlashMovieAdapter::flFnUnregisterChild, false },
	{ TXT("_NATIVE_callGameEvent"), &CFlashMovieAdapter::flFnCallGameEvent, false },
//	{ TXT("_NATIVE_callVideoEvent"), &CFlashMovieAdapter::flFnCallVideoEvent, true },
 	{ TXT("_NATIVE_registerRenderTarget"), &CFlashMovieAdapter::flFnRegisterRenderTarget, true },
 	{ TXT("_NATIVE_unregisterRenderTarget"), &CFlashMovieAdapter::flFnUnregisterRenderTarget, true },
};

CFlashMovieAdapter::CFlashMovieAdapter( CFlashMovie* flashMovie )
{
	ASSERT( flashMovie );
	m_flashMovie = flashMovie;
	if ( m_flashMovie )
	{
		m_flashMovie->AddRef();
	}

	ASSERT( flashMovie && ! flashMovie->GetUserData() );
	if ( flashMovie )
	{
		flashMovie->SetUserData( this );
	}
}

Bool CFlashMovieAdapter::Init()
{
	InitFlashFunctions();
	return true;
}

void CFlashMovieAdapter::InitFlashFunctions()
{
	PC_SCOPE( InitFlashFunctions );

	ASSERT( m_flashMovie );
	if ( ! m_flashMovie )
	{
		return;
	}

	size_t len = sizeof(sm_flashFunctionExportTable)/sizeof(sm_flashFunctionExportTable[0]);
	for ( size_t i = 0; i < len; ++i )
	{
		const Bool allowMissingExport = sm_flashFunctionExportTable[i].m_allowMissingExport;
		CFlashFunction* flashFunction = CreateFlashFunction( sm_flashFunctionExportTable[i].m_flashFuncExport );

		if ( flashFunction )
		{
			VERIFY( m_flashFunctionMap.Insert( sm_flashFunctionExportTable[i].m_memberName, flashFunction ) );
		}
		else
		{
			if ( allowMissingExport )
			{
				GUI_WARN( TXT("CFlashMovieAdapter::InitFlashFunctions: Missing export '%ls'"), sm_flashFunctionExportTable[i].m_memberName );
			}
			else
			{
				GUI_ERROR( TXT("CFlashMovieAdapter::InitFlashFunctions: Missing export '%ls'"), sm_flashFunctionExportTable[i].m_memberName );
			}
		}
	}
}

CFlashMovieAdapter::~CFlashMovieAdapter()
{	
	m_flashMovie->SetUserData( nullptr );

	ASSERT( m_flashMovie );
	if ( m_flashMovie )
	{
		VERIFY( m_flashMovie->Release() );
		m_flashMovie = nullptr;
	}
}

void CFlashMovieAdapter::OnDestroy()
{
	DiscardFlashFunctions();
}

void CFlashMovieAdapter::DiscardFlashFunctions()
{
	PC_SCOPE( DiscardFlashFunctions );

	//CHANGEME: Could do with smart pointers...
	for ( TFlashFunctionMap::const_iterator it = m_flashFunctionMap.Begin(); it != m_flashFunctionMap.End(); ++it )
	{
		CFlashFunction* flashFunction = it->m_second;
		if ( flashFunction )
		{
			VERIFY( flashFunction->Release() == 0 ); 
		}
	}
	m_flashFunctionMap.Clear();
}

CFlashFunction* CFlashMovieAdapter::CreateFlashFunction( TFlashFunctionExport flashFunc )
{
	PC_SCOPE( CreateFlashFunction );

	ASSERT( m_flashMovie );
	if ( ! m_flashMovie )
	{
		return nullptr;
	}

	CFlashFunctionHandler* flashFunctionHandler = Flash::CreateFunctionHandler( this, flashFunc );
	ASSERT( flashFunctionHandler );
	if ( ! flashFunctionHandler )
	{
		return nullptr;
	}

	CFlashFunction* flashFunction = m_flashMovie->CreateFunction( flashFunctionHandler );
	ASSERT( flashFunction );
	flashFunctionHandler->Release(); // CFlashFunction addref'd

	return flashFunction;
}

void CFlashMovieAdapter::flFnRegisterDataBinding( CFlashMovie* flashMovie, const CFlashValue& flashThis, const TDynArray< CFlashValue >& args )
{
	ASSERT( flashThis.IsFlashDisplayObject() );
	if ( ! flashThis.IsFlashDisplayObject() )
	{
		GUI_ERROR( TXT("RegisterDataBinding: Function must be bound to a Flash DisplayObject before calling.") );
		return;
	}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Backward compatibility (just for a moment) to handle SWFs published with and without changes in red\core\*.as
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////


	const Uint32 expectedNumArgs = 4;
	Uint32 numArgs = args.Size();
	if ( numArgs != expectedNumArgs
/////////////////////////////////////////////////////
// DELETE HERE
		&& numArgs != expectedNumArgs - 1
// DELETE HERE
/////////////////////////////////////////////////////
		)
	{
		GUI_ERROR( TXT("RegisterDataBinding: Incorrect number of arguments passed. Expected '%u', received '%u'"), expectedNumArgs, numArgs );
		return;
	}

	ASSERT( flashThis.IsFlashDisplayObject() );
	if ( ! flashThis.IsFlashDisplayObject() )
	{
		GUI_ERROR( TXT("RegisterDataBinding: not called from a DisplayObject") );
		return;
	}

	if ( ! args[0].IsFlashString() )
	{
		GUI_ERROR( TXT("RegisterDataBinding: Expected arg[0] to be a Flash String") );
		return;
	}
	if ( ! args[1].IsFlashClosure() )
	{
		GUI_ERROR( TXT("RegisterDataBinding: Expected arg[1] to be a Flash closure") );
		return;
	}
	if ( ! args[2].IsFlashNull() && ! args[2].IsFlashObject() )
	{
		GUI_ERROR( TXT("RegisterDataBinding: Expected arg[2] to be a Flash Object or null") );
		return;
	}
/////////////////////////////////////////////////////
// DELETE HERE
	if ( numArgs == 4 )
// DELETE HERE
/////////////////////////////////////////////////////
	if ( ! args[3].IsFlashBool() )
	{
		GUI_ERROR( TXT("RegisterDataBinding: Expected arg[3] to be a Flash Bool") );
		return;
	}

	String key = args[0].GetFlashString();
	const CFlashValue& flashClosure = args[1];
	const CFlashValue& flashBoundObject = args[2];
/////////////////////////////////////////////////////
// UNCOMMENT HERE
	//const CFlashValue& flashIsGlobal = args[3];
// UNCOMMENT HERE
/////////////////////////////////////////////////////

/////////////////////////////////////////////////////
// DELETE HERE
	CFlashValue flashIsGlobal;
	if ( numArgs == 4 )
	{
		flashIsGlobal = args[3];
	}
	else
	{
		flashIsGlobal.SetFlashBool( false );
	}
// DELETE HERE
/////////////////////////////////////////////////////

	CFlashSpriteAdapter* flashSpriteAdapter = static_cast< CFlashSpriteAdapter* >( flashThis.GetUserData() );
	ASSERT( flashSpriteAdapter );
	if ( ! flashSpriteAdapter )
	{
		GUI_ERROR( TXT("RegisterDataBinding: Flash not registered yet!") );
		return;
	}

	flashSpriteAdapter->RegisterDataBinding( key, flashClosure, flashBoundObject, flashIsGlobal );
}

void CFlashMovieAdapter::flFnUnregisterDataBinding( CFlashMovie* flashMovie, const CFlashValue& flashThis, const TDynArray< CFlashValue >& args )
{
	ASSERT( flashThis.IsFlashDisplayObject() );
	if ( ! flashThis.IsFlashDisplayObject() )
	{
		GUI_ERROR( TXT("UnregisterDataBinding: Function must be bound to a Flash DisplayObject before calling.") );
		return;
	}

}

void CFlashMovieAdapter::flFnRegisterChild( CFlashMovie* flashMovie, const CFlashValue& flashThis, const TDynArray< CFlashValue >& args )
{
	ASSERT( flashThis.IsFlashDisplayObject() );
	if ( ! flashThis.IsFlashDisplayObject() )
	{
		GUI_ERROR( TXT("RegisterChild: Function must be bound to a Flash DisplayObject before calling.") );
		return;
	}

	ASSERT( args[0].IsFlashDisplayObject() );
	ASSERT( args[1].IsFlashString() );

	const Uint32 expectedNumArgs = 2;
	Uint32 numArgs = args.Size();
	if ( numArgs != expectedNumArgs )
	{
		GUI_ERROR( TXT("RegisterChild: Incorrect number of arguments passed. Expected '%u', received '%u'"), expectedNumArgs, numArgs );
		return;
	}

	if ( ! args[0].IsFlashDisplayObject() )
	{
		GUI_ERROR( TXT("RegisterChild: Expected arg[0] to be a Flash DisplayObject") );
		return;
	}

	if ( ! args[1].IsFlashString() )
	{
		GUI_ERROR( TXT("RegisterChild: Expected arg[1] to be a Flash String") );
		return;
	}

	const CFlashValue& spriteChild = args[0];
	String childName = args[1].GetFlashString();

	CFlashSpriteAdapter* parentFlashSpriteAdapter = static_cast< CFlashSpriteAdapter* >( flashThis.GetUserData() );
	ASSERT( parentFlashSpriteAdapter );
	ASSERT( parentFlashSpriteAdapter->GetFlashMovie() == flashMovie );

	if ( spriteChild.GetUserData() )
	{
		GUI_ERROR( TXT("RegisterChild: Child '%ls' seems to already be registered"), childName.AsChar() );
		return;
	}

	if ( ! parentFlashSpriteAdapter )
	{
		GUI_ERROR( TXT("RegisterChild: Flash parent not registered yet!") );
		return;
	}
	
	parentFlashSpriteAdapter->RegisterChild( childName, spriteChild );
}

void CFlashMovieAdapter::flFnUnregisterChild( CFlashMovie* flashMovie, const CFlashValue& flashThis, const TDynArray< CFlashValue >& args )
{
	ASSERT( flashThis.IsFlashDisplayObject() );
	if ( ! flashThis.IsFlashDisplayObject() )
	{
		GUI_ERROR( TXT("UnregisterChild: Function must be bound to a Flash DisplayObject before calling.") );
		return;
	}

	const Uint32 expectedNumArgs = 0;
	Uint32 numArgs = args.Size();
	if ( numArgs != expectedNumArgs )
	{
		GUI_ERROR( TXT("UnregisterChild: Incorrect number of arguments passed. Expected '%u', received '%u'"), expectedNumArgs, numArgs );
		return;
	}

	CFlashSpriteAdapter* flashSpriteAdapter = static_cast< CFlashSpriteAdapter* >( flashThis.GetUserData() );
	ASSERT( flashSpriteAdapter );
	if ( ! flashSpriteAdapter )
	{
		GUI_ERROR( TXT("UnregisterChild: Seems as if component isn't registered") );
		return;
	}

	flashSpriteAdapter->UnregisterChild( flashThis );
}

void CFlashMovieAdapter::flFnCallGameEvent( CFlashMovie* flashMovie, const CFlashValue& flashThis, const TDynArray< CFlashValue >& args )
{
	PC_SCOPE( flFnCallGameEvent );

	// 	ASSERT( ! m_isInTick );
	// 	if ( m_isInTick )
	// 	{
	// 		return;
	// 	}
	// 
	// 	ASSERT( flashMovie == m_flashMovie );

	ASSERT( flashThis.IsFlashDisplayObject() );
	if ( ! flashThis.IsFlashDisplayObject() )
	{
		GUI_ERROR( TXT("CallGameEvent: Function must be bound to a Flash DisplayObject before calling.") );
		return;
	}

	const Uint32 expectedNumArgs = 2;
	Uint32 numArgs = args.Size();
	if ( numArgs != expectedNumArgs )
	{
		GUI_ERROR( TXT("CallGameEvent: Incorrect number of arguments passed. Expected '%u', received '%u'"), expectedNumArgs, numArgs );
		return;
	}

	ASSERT( flashThis.IsFlashDisplayObject() );
	if ( ! flashThis.IsFlashDisplayObject() )
	{
		GUI_ERROR( TXT("CallGameEvent: not called from a DisplayObject") );
		return;
	}

	if ( ! args[0].IsFlashString() )
	{
		GUI_ERROR( TXT("CallGameEvent: Expected arg[0] to be a Flash String") );
		return;
	}
	if ( !args[1].IsFlashArray() && ! args[1].IsFlashNull() )
	{
		GUI_ERROR( TXT("CallGameEvent: Expected arg[1] to be a Flash Array or null") );
		return;
	}

	//TODO: Make sure in Flash that closure/object isn't to some object outside the module's displaylist hierarchy...
	String eventName = args[0].GetFlashString();
	const CFlashValue& eventArgs = args[1];

	CFlashSpriteAdapter* flashSpriteAdapter = static_cast< CFlashSpriteAdapter* >( flashThis.GetUserData() );
	ASSERT( flashSpriteAdapter );
	if ( ! flashSpriteAdapter )
	{
		GUI_ERROR( TXT("CallGameEvent: Flash not registered yet!") );
		return;
	}

	flashSpriteAdapter->CallGameEvent( eventName, eventArgs );
}

void CFlashMovieAdapter::flFnRegisterRenderTarget( CFlashMovie* flashMovie, const CFlashValue& flashThis, const TDynArray< CFlashValue >& args )
{
	PC_SCOPE( flFnRegisterRenderTarget );

	// 	ASSERT( ! m_isInTick );
	// 	if ( m_isInTick )
	// 	{
	// 		return;
	// 	}
	// 
	// 	ASSERT( flashMovie == m_flashMovie );

	ASSERT( flashThis.IsFlashDisplayObject() );
	if ( ! flashThis.IsFlashDisplayObject() )
	{
		GUI_ERROR( TXT("RegisterRenderTarget: Function must be bound to a Flash DisplayObject before calling.") );
		return;
	}

	const Uint32 expectedNumArgs = 3;
	Uint32 numArgs = args.Size();
	if ( numArgs != expectedNumArgs )
	{
		GUI_ERROR( TXT("RegisterRenderTarget: Incorrect number of arguments passed. Expected '%u', received '%u'. Usage: RegisterRenderTarget(targetName, width height)"), expectedNumArgs, numArgs );
		return;
	}

	ASSERT( flashThis.IsFlashDisplayObject() );
	if ( ! flashThis.IsFlashDisplayObject() )
	{
		GUI_ERROR( TXT("RegisterRenderTarget: not called from a DisplayObject") );
		return;
	}

	if ( ! args[0].IsFlashString() )
	{
		GUI_ERROR( TXT("RegisterRenderTarget: Expected arg[0] to be a Flash string") );
		return;
	}
	if ( ! args[1].IsFlashUInt() )
	{
		GUI_ERROR( TXT("RegisterRenderTarget: Expected arg[1] to be a Flash uint") );
		return;
	}
	if ( !args[2].IsFlashUInt() )
	{
		GUI_ERROR( TXT("RegisterRenderTarget: Expected arg[2] to be a Flash uint") );
		return;
	}

	//TODO: Make sure in Flash that closure/object isn't to some object outside the module's displaylist hierarchy...
	String sceneName = args[0].GetFlashString();
	Uint32 width = args[1].GetFlashUInt();
	Uint32 height = args[2].GetFlashUInt();

	if ( sceneName.Empty() )
	{
		GUI_ERROR( TXT("RegisterRenderTarget: targetName cannot be empty") );
		return;
	}

	if ( width < 1 || height < 1 )
	{
		GUI_ERROR( TXT("RegisterRenderTarget: target area must be non-zero: width=%u, height=%u"), width, height );
		return;

	}

	CFlashSpriteAdapter* flashSpriteAdapter = static_cast< CFlashSpriteAdapter* >( flashThis.GetUserData() );
	ASSERT( flashSpriteAdapter );
	if ( ! flashSpriteAdapter )
	{
		GUI_ERROR( TXT("RegisterRenderTarget: Flash not registered yet!") );
		return;
	}

	flashSpriteAdapter->RegisterRenderTarget( sceneName, width, height );
}

void CFlashMovieAdapter::flFnUnregisterRenderTarget( CFlashMovie* flashMovie, const CFlashValue& flashThis, const TDynArray< CFlashValue >& args )
{
	PC_SCOPE( flFnUnregisterRenderTarget );

	// 	ASSERT( ! m_isInTick );
	// 	if ( m_isInTick )
	// 	{
	// 		return;
	// 	}
	// 
	// 	ASSERT( flashMovie == m_flashMovie );

	ASSERT( flashThis.IsFlashDisplayObject() );
	if ( ! flashThis.IsFlashDisplayObject() )
	{
		GUI_ERROR( TXT("UnregisterRenderTarget: Function must be bound to a Flash DisplayObject before calling.") );
		return;
	}

	const Uint32 expectedNumArgs = 1;
	Uint32 numArgs = args.Size();
	if ( numArgs != expectedNumArgs )
	{
		GUI_ERROR( TXT("UnregisterRenderTarget: Incorrect number of arguments passed. Expected '%u', received '%u'. Usage: UnregisterRenderScene(sceneName)"), expectedNumArgs, numArgs );
		return;
	}

	ASSERT( flashThis.IsFlashDisplayObject() );
	if ( ! flashThis.IsFlashDisplayObject() )
	{
		GUI_ERROR( TXT("UnregisterRenderTarget: not called from a DisplayObject") );
		return;
	}

	if ( ! args[0].IsFlashString() )
	{
		GUI_ERROR( TXT("UnregisterRenderTarget: Expected arg[0] to be a Flash string") );
		return;
	}

	//TODO: Make sure in Flash that closure/object isn't to some object outside the module's displaylist hierarchy...
	String targetName = args[0].GetFlashString();
	
	if ( targetName.Empty() )
	{
		GUI_ERROR( TXT("UnregisterRenderTarget: targetName cannot be empty") );
		return;
	}

	CFlashSpriteAdapter* flashSpriteAdapter = static_cast< CFlashSpriteAdapter* >( flashThis.GetUserData() );
	ASSERT( flashSpriteAdapter );
	if ( ! flashSpriteAdapter )
	{
		GUI_ERROR( TXT("UnregisterRenderTarget: Flash not registered yet!") );
		return;
	}

	flashSpriteAdapter->UnregisterRenderTarget( targetName );
}

