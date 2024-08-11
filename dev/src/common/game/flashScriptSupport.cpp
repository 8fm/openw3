/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"

#include "flashScriptSupport.h"
#include "../engine/flashValueObject.h"
#include "../engine/flashValueStorage.h"
#include "../engine/flashMovie.h"
#include "../engine/guiGlobals.h"

//////////////////////////////////////////////////////////////////////////
// RTTI Boilerplate
//////////////////////////////////////////////////////////////////////////
IMPLEMENT_ENGINE_CLASS( IScriptedFlash );
IMPLEMENT_ENGINE_CLASS( CScriptedFlashObject );
IMPLEMENT_ENGINE_CLASS( CScriptedFlashSprite );
IMPLEMENT_ENGINE_CLASS( CScriptedFlashArray );
IMPLEMENT_ENGINE_CLASS( CScriptedFlashTextField );
IMPLEMENT_ENGINE_CLASS( CScriptedFlashFunction );
IMPLEMENT_ENGINE_CLASS( CScriptedFlashValueStorage );
IMPLEMENT_ENGINE_CLASS( SFlashArg );

//////////////////////////////////////////////////////////////////////////
// CScriptedFlashObjectPool
//////////////////////////////////////////////////////////////////////////
CScriptedFlashObjectPool::CScriptedFlashObjectPool()
{
}

CScriptedFlashObjectPool::~CScriptedFlashObjectPool()
{
	for ( IScriptedFlash* obj : m_poolObjects )
	{
		obj->ClearFlashObjectPool();
		delete obj;
	}
}

void CScriptedFlashObjectPool::RegisterPoolObject( IScriptedFlash* poolObject )
{
	m_poolObjects.PushBack( poolObject );
}

void CScriptedFlashObjectPool::UnregisterPoolObject( IScriptedFlash* poolObject )
{
	m_poolObjects.Remove( poolObject );
}

//////////////////////////////////////////////////////////////////////////
// IScriptedFlash
//////////////////////////////////////////////////////////////////////////
IScriptedFlash::IScriptedFlash( CScriptedFlashObjectPool& flashObjectPool )
	: m_flashObjectPool( &flashObjectPool )
{
	RED_FATAL_ASSERT( ::SIsMainThread(), "IScriptedFlash must be on the main thread!" );

	EnableReferenceCounting();
	EnableAutomaticDiscard();

	m_flashObjectPool->RegisterPoolObject( this );
}

IScriptedFlash::~IScriptedFlash()
{
	RED_FATAL_ASSERT( ::SIsMainThread(), "IScriptedFlash must be on the main thread!" );

	if ( m_flashObjectPool )
	{
		m_flashObjectPool->UnregisterPoolObject( this );
	}
}

void IScriptedFlash::ClearFlashObjectPool()
{
	m_flashObjectPool = nullptr;
	EnableAutomaticDiscard( false );
	EnableReferenceCounting( false );
}

//////////////////////////////////////////////////////////////////////////
// CScriptedFlashObject
//////////////////////////////////////////////////////////////////////////
CScriptedFlashObject::CScriptedFlashObject( CScriptedFlashObjectPool& pool, const CFlashValue& flashValue, const String& name /*= String::EMPTY*/ )
	: TBaseClass( pool )
	, m_flashValue( flashValue )
#ifndef RED_FINAL_BUILD
	, m_name( name )
#endif
{
#ifndef RED_FINAL_BUILD
	if ( ! GetFlashValue().IsFlashObject() )
	{
		GUI_ERROR(TXT("CScriptedFlashObject '%ls' not initialized with an object!"), name.AsChar() );
	}
#endif
}

CScriptedFlashObject::~CScriptedFlashObject()
{
	m_flashValue.Clear();
}

Bool CScriptedFlashObject::GetFlashObject( const String& memberName, CFlashValue& flashValue )
{
	if ( ! m_flashValue.GetMember( memberName, flashValue ) )
	{
		GUI_SCRIPT_WARN_ONCE( stack, TXT("Could not get FlashValue member '%ls'"), memberName.AsChar()  );
		return false;
	}
	else if ( flashValue.IsFlashNull() )
	{
		GUI_SCRIPT_WARN( TXT("Flash member '%ls' is null"), memberName.AsChar() );
		return false;
	}
	else if ( ! flashValue.IsFlashClosure() )
	{
		GUI_SCRIPT_WARN_ONCE( stack, TXT("Member '%ls' is not Function"), memberName.AsChar() );
		return false;
	}
	return true;
}

void CScriptedFlashObject::funcCreateFlashObject( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_OPT( String, flashClassName, TXT("Object") );
	FINISH_PARAMETERS;

	CScriptedFlashObject* newScriptedFlashObject = nullptr;
	CFlashValue tmpFlashValue;
	if ( m_flashValue.CreateObject( flashClassName, tmpFlashValue ) )
	{
		newScriptedFlashObject = new CScriptedFlashObject( GetFlashObjectPool(), tmpFlashValue, FLASH_OBJ_TXT("[Object]") );
	}

	RETURN_HANDLE( CScriptedFlashObject, newScriptedFlashObject );
}

void CScriptedFlashObject::funcCreateFlashArray( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	CScriptedFlashArray* newScriptedFlashArray = nullptr;
	CFlashValue tmpFlashValue;
	if ( m_flashValue.CreateArray( tmpFlashValue ) )
	{
		newScriptedFlashArray = new CScriptedFlashArray( GetFlashObjectPool(), tmpFlashValue, FLASH_OBJ_TXT("[Array]") );
	}

	RETURN_HANDLE( CScriptedFlashArray, newScriptedFlashArray );
}

void CScriptedFlashObject::funcGetMemberFlashObject( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, memberName, String::EMPTY );
	FINISH_PARAMETERS;

	CScriptedFlashObject* newScriptedFlashObject = nullptr;
	CFlashValue tmpFlashValue;
	if ( ! m_flashValue.GetMember( memberName, tmpFlashValue ) )
	{
		GUI_SCRIPT_WARN_ONCE( stack, TXT("Could not get FlashValue member '%ls'"), memberName.AsChar()  );
	}
	else if ( tmpFlashValue.IsFlashNull() )
	{
		GUI_SCRIPT_WARN( TXT("Flash member '%ls' is null"), memberName.AsChar() );
	}
	else if ( ! tmpFlashValue.IsFlashObject() )
	{
		GUI_SCRIPT_WARN_ONCE( stack, TXT("Member is '%ls' is not Object"), memberName.AsChar() );
	}
	else
	{
		newScriptedFlashObject = new CScriptedFlashObject( GetFlashObjectPool(), tmpFlashValue, FLASH_MBR_STR(memberName) );
	}

	RETURN_HANDLE( CScriptedFlashObject, newScriptedFlashObject );
}

void CScriptedFlashObject::funcGetMemberFlashArray( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, memberName, String::EMPTY );
	FINISH_PARAMETERS;

	CScriptedFlashArray* newScriptedFlashArray = nullptr;
	CFlashValue tmpFlashValue;
	if ( ! m_flashValue.GetMember( memberName, tmpFlashValue ) )
	{
		GUI_SCRIPT_WARN_ONCE( stack, TXT("Could not get FlashValue member '%ls'"), memberName.AsChar()  );
	}
	else if ( tmpFlashValue.IsFlashNull() )
	{
		GUI_SCRIPT_WARN( TXT("Flash member '%ls' is null"), memberName.AsChar() );
	}
	else if ( ! tmpFlashValue.IsFlashArray() )
	{
		GUI_SCRIPT_WARN_ONCE( stack, TXT("Member is '%ls' is not Array"), memberName.AsChar() );
	}
	else
	{
		newScriptedFlashArray = new CScriptedFlashArray( GetFlashObjectPool(), tmpFlashValue, FLASH_MBR_STR(memberName) );
	}

	RETURN_HANDLE( CScriptedFlashArray, newScriptedFlashArray );
}

void CScriptedFlashObject::funcGetMemberFlashFunction( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, memberName, String::EMPTY );
	FINISH_PARAMETERS;

	CScriptedFlashFunction* newScriptedFlashFunction = nullptr;
	CFlashValue tmpFlashValue;
	if ( ! m_flashValue.GetMember( memberName, tmpFlashValue ) )
	{
		GUI_SCRIPT_WARN_ONCE( stack, TXT("Could not get FlashValue member '%ls'"), memberName.AsChar()  );
	}
	else if ( tmpFlashValue.IsFlashNull() )
	{
		GUI_SCRIPT_WARN( TXT("Flash member '%ls' is null"), memberName.AsChar() );
	}
	else if ( ! tmpFlashValue.IsFlashClosure() )
	{
		GUI_SCRIPT_WARN_ONCE( stack, TXT("Member '%ls' is not Function"), memberName.AsChar() );
	}
	else
	{
		newScriptedFlashFunction = new CScriptedFlashFunction( GetFlashObjectPool(), tmpFlashValue, FLASH_MBR_STR(memberName) );
	}

	RETURN_HANDLE( CScriptedFlashFunction, newScriptedFlashFunction );
}

void CScriptedFlashObject::funcGetMemberFlashString( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, memberName, String::EMPTY );
	FINISH_PARAMETERS;

	CFlashValue tmpFlashValue;
	if ( ! m_flashValue.GetMember( memberName, tmpFlashValue ) )
	{
		GUI_SCRIPT_WARN_ONCE( stack, TXT("Could not get FlashValue member '%ls'"), memberName.AsChar()  );
	}
	else if ( ! tmpFlashValue.IsFlashString() )
	{
		GUI_SCRIPT_WARN_ONCE( stack, TXT("Member '%ls' is not String"), memberName.AsChar() );
	}

	RETURN_STRING( tmpFlashValue.GetFlashString() );
}

void CScriptedFlashObject::funcGetMemberFlashBool( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, memberName, String::EMPTY );
	FINISH_PARAMETERS;

	CFlashValue tmpFlashValue;
	if ( ! m_flashValue.GetMember( memberName, tmpFlashValue ) )
	{
		GUI_SCRIPT_WARN_ONCE( stack, TXT("Could not get FlashValue member '%ls'"), memberName.AsChar()  );
	}
	else if ( ! tmpFlashValue.IsFlashBool() )
	{
		GUI_SCRIPT_WARN_ONCE( stack, TXT("Member '%ls' is not Bool"), memberName.AsChar() );
	}

	RETURN_BOOL( tmpFlashValue.GetFlashBool() );
}

void CScriptedFlashObject::funcGetMemberFlashInt( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, memberName, String::EMPTY );
	FINISH_PARAMETERS;

	CFlashValue tmpFlashValue;
	if ( ! m_flashValue.GetMember( memberName, tmpFlashValue ) )
	{
		GUI_SCRIPT_WARN_ONCE( stack, TXT("Could not get FlashValue member '%ls'"), memberName.AsChar()  );
	}
	else if ( ! tmpFlashValue.IsFlashInt() )
	{
		GUI_SCRIPT_WARN_ONCE( stack, TXT("Member '%ls' is not int"), memberName.AsChar() );
	}

	RETURN_INT( tmpFlashValue.GetFlashInt() );
}

void CScriptedFlashObject::funcGetMemberFlashUInt( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, memberName, String::EMPTY );
	FINISH_PARAMETERS;

	CFlashValue tmpFlashValue;
	if ( ! m_flashValue.GetMember( memberName, tmpFlashValue ) )
	{
		GUI_SCRIPT_WARN_ONCE( stack, TXT("Could not get FlashValue member '%ls'"), memberName.AsChar()  );
	}
	else if ( ! tmpFlashValue.IsFlashUInt() )
	{
		GUI_SCRIPT_WARN_ONCE( stack, TXT("Member '%ls' is not uint"), memberName.AsChar() );
	}

	RETURN_INT( static_cast< Int32 >( tmpFlashValue.GetFlashUInt() ) );
}

void CScriptedFlashObject::funcGetMemberFlashNumber( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, memberName, String::EMPTY );
	FINISH_PARAMETERS;

	CFlashValue tmpFlashValue;
	if ( ! m_flashValue.GetMember( memberName, tmpFlashValue ) )
	{
		GUI_SCRIPT_WARN_ONCE( stack, TXT("Could not get FlashValue member '%ls'"), memberName.AsChar()  );
	}
	else if ( ! tmpFlashValue.IsFlashNumber() )
	{
		GUI_SCRIPT_WARN_ONCE( stack, TXT("Member '%ls' is not Number"), memberName.AsChar() );
	}

	RETURN_FLOAT( static_cast< Float >( tmpFlashValue.GetFlashNumber() ) );
}

void CScriptedFlashObject::funcSetMemberFlashObject( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, memberName, String::EMPTY );
	GET_PARAMETER( THandle< CScriptedFlashObject >, value, nullptr );
	FINISH_PARAMETERS;

	CScriptedFlashObject* scriptedFlashObject = value.Get();
	if ( scriptedFlashObject && ! m_flashValue.SetMemberFlashValue( memberName, scriptedFlashObject->m_flashValue ) )
	{
		GUI_SCRIPT_WARN( TXT("Could not set member '%ls' with an object"), memberName.AsChar() );
	}
	else if ( ! scriptedFlashObject && ! m_flashValue.SetMemberFlashNull( memberName ) )
	{
		GUI_SCRIPT_WARN( TXT("Could not set member '%ls' to null"), memberName.AsChar() );
	}
}

void CScriptedFlashObject::funcSetMemberFlashArray( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, memberName, String::EMPTY );
	GET_PARAMETER( THandle< CScriptedFlashArray >, value, nullptr );
	FINISH_PARAMETERS;

	CScriptedFlashArray* scriptedFlashArray = value.Get();
	if ( scriptedFlashArray )
	{
		if ( ! m_flashValue.SetMemberFlashValue( memberName, scriptedFlashArray->m_flashValue ) )
		{
			GUI_SCRIPT_WARN( TXT("Could not set member '%ls' with an array"), memberName.AsChar() );
		}
	}
	else
	{
		if ( ! m_flashValue.SetMemberFlashNull( memberName ) )
		{
			GUI_SCRIPT_WARN( TXT("Could not set member '%ls' to null"), memberName.AsChar() );
		}
	}
}

void CScriptedFlashObject::funcSetMemberFlashFunction( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, memberName, String::EMPTY );
	GET_PARAMETER( THandle< CScriptedFlashFunction >, value, nullptr );
	FINISH_PARAMETERS;

	CScriptedFlashFunction* scriptedFlashFunction = value.Get();
	if ( scriptedFlashFunction && ! m_flashValue.SetMemberFlashValue( memberName, scriptedFlashFunction->m_flashValue ) )
	{
		GUI_SCRIPT_WARN( TXT("Could not set member '%ls' with a function"), memberName.AsChar() );
	}
	else if ( ! scriptedFlashFunction && ! m_flashValue.SetMemberFlashNull( memberName ) )
	{
		GUI_SCRIPT_WARN( TXT("Could not set member '%ls' to null"), memberName.AsChar() );
	}
}

void CScriptedFlashObject::funcSetMemberFlashString( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, memberName, String::EMPTY );
	GET_PARAMETER( String, value, String::EMPTY );
	FINISH_PARAMETERS;

	if ( ! m_flashValue.SetMemberFlashString( memberName, value ) )
	{
		GUI_SCRIPT_WARN( TXT("Could not set member '%ls' string '%ls'"), memberName.AsChar(), value.AsChar() );
	}
}

void CScriptedFlashObject::funcSetMemberFlashBool( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, memberName, String::EMPTY );
	GET_PARAMETER( Bool, value, false );
	FINISH_PARAMETERS;

	if ( ! m_flashValue.SetMemberFlashBool( memberName, value ) )
	{
		GUI_SCRIPT_WARN( TXT("Could not set member '%ls' with Bool '%ls'"), memberName.AsChar(), value ? TXT("true") : TXT("false") );
	}
}

void CScriptedFlashObject::funcSetMemberFlashInt( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, memberName, String::EMPTY );
	GET_PARAMETER( Int32, value, 0 );
	FINISH_PARAMETERS;

	if ( ! m_flashValue.SetMemberFlashInt( memberName, value ) )
	{
		GUI_SCRIPT_WARN( TXT("Could not set member '%ls' with int '%d'"), memberName.AsChar(), value );
	}
}

void CScriptedFlashObject::funcSetMemberFlashUInt( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, memberName, String::EMPTY );
	GET_PARAMETER( Int32, value, 0 );
	FINISH_PARAMETERS;

	if ( ! m_flashValue.SetMemberFlashUInt( memberName, static_cast< Uint32 >( value ) ) )
	{
		GUI_SCRIPT_WARN( TXT("Could not set member '%ls' with uint '%u'"), memberName.AsChar(), static_cast< Uint32 >( value ) );
	}
}

void CScriptedFlashObject::funcSetMemberFlashNumber( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, memberName, String::EMPTY );
	GET_PARAMETER( Float, value, 0.f );
	FINISH_PARAMETERS;

	if ( ! m_flashValue.SetMemberFlashNumber( memberName, static_cast< Double >( value ) ) )
	{
		GUI_SCRIPT_WARN( TXT("Could not set member '%ls' with Number '%f'"), memberName.AsChar(), value );
	}
}

//////////////////////////////////////////////////////////////////////////
// CScriptedFlashSprite
//////////////////////////////////////////////////////////////////////////
CScriptedFlashSprite::CScriptedFlashSprite( CScriptedFlashObjectPool& pool, const CFlashValue& flashValue, const String& name /*= String::EMPTY*/ )
	: TBaseClass( pool, flashValue, name )
{
#ifndef RED_FINAL_BUILD
	if ( ! GetFlashValue().IsInstanceOf(TXT("flash.display.Sprite")) )
	{
		GUI_ERROR(TXT("CScriptedFlashSprite '%ls' not initialized with a sprite!"), name.AsChar() );
	}
#endif
}

// void CScriptedFlashSprite::funcIsOnStage( CScriptStackFrame& stack, void* result )
// {
// 	FINISH_PARAMETERS;
// 
// 	RETURN_BOOL( m_flashValue.IsFlashDisplayObjectOnStage() );
// }

void CScriptedFlashSprite::funcGetChildFlashSprite( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, memberName, String::EMPTY );
	FINISH_PARAMETERS;

	CScriptedFlashSprite* newScriptedFlashSprite = nullptr;
	CFlashValue tmpFlashValue;
	if ( ! GetFlashValue().GetMember( memberName, tmpFlashValue ) )
	{
		GUI_SCRIPT_WARN_ONCE( stack, TXT("Could not get FlashValue member '%ls'"), memberName.AsChar()  );
	}
	else if ( tmpFlashValue.IsFlashNull() )
	{
		GUI_SCRIPT_WARN( TXT("Flash member '%ls' is null"), memberName.AsChar() );
	}
	else if ( ! tmpFlashValue.IsFlashDisplayObject() )
	{
		GUI_SCRIPT_WARN_ONCE( stack, TXT("Member is '%ls' is not Sprite"), memberName.AsChar() );
	}
	else
	{
		newScriptedFlashSprite = new CScriptedFlashSprite( GetFlashObjectPool(), tmpFlashValue, FLASH_MBR_STR(memberName) );
	}

	RETURN_HANDLE( CScriptedFlashSprite, newScriptedFlashSprite );
}

void CScriptedFlashSprite::funcGetChildFlashTextField( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, memberName, String::EMPTY );
	FINISH_PARAMETERS;

	CScriptedFlashTextField* newScriptedFlashTextField = nullptr;
	CFlashValue tmpFlashValue;
	if ( ! GetFlashValue().GetMember( memberName, tmpFlashValue ) )
	{
		GUI_SCRIPT_WARN_ONCE( stack, TXT("Could not get FlashValue member '%ls'"), memberName.AsChar()  );
	}
	else if ( GetFlashValue().IsFlashNull() )
	{
		GUI_SCRIPT_WARN( TXT("Flash member '%ls' is null"), memberName.AsChar() );
	}
	else if ( ! GetFlashValue().IsFlashDisplayObject() ) // Better than no check
	{
		GUI_SCRIPT_WARN_ONCE( stack, TXT("Member is '%ls' is not TextField"), memberName.AsChar() );
	}
	else
	{
		newScriptedFlashTextField = new CScriptedFlashTextField( GetFlashObjectPool(), tmpFlashValue, FLASH_MBR_STR(memberName) );
	}

	RETURN_HANDLE( CScriptedFlashTextField, newScriptedFlashTextField );
}

void CScriptedFlashSprite::funcGotoAndPlayFrameNumber( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int32, frame, 0 );
	FINISH_PARAMETERS;

	if ( frame < 1 )
	{
		GUI_SCRIPT_WARN( TXT("Can't go to frame < 1 '%d'"), frame );
	}
	else if ( ! GetFlashValue().GotoAndPlay( static_cast< Uint32 >( frame ) ) )
	{
		GUI_SCRIPT_WARN( TXT("Can't go to frame '%d'"), frame );
	}
}

void CScriptedFlashSprite::funcGotoAndPlayFrameLabel( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, frame, String::EMPTY );
	FINISH_PARAMETERS;

	if ( ! GetFlashValue().GotoAndPlay( frame ) )
	{
		GUI_SCRIPT_WARN( TXT("Can't go to frame '%ls'"), frame.AsChar() );
	}
}

void CScriptedFlashSprite::funcGotoAndStopFrameNumber( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int32, frame, 0 );
	FINISH_PARAMETERS;

	if ( frame < 1 )
	{
		GUI_SCRIPT_WARN( TXT("Can't go to frame < 1 '%d'"), frame );
	}
	else if ( ! GetFlashValue().GotoAndStop( static_cast< Uint32 >( frame ) ) )
	{
		GUI_SCRIPT_WARN( TXT("Can't go to frame '%d'"), frame );
	}
}

void CScriptedFlashSprite::funcGotoAndStopFrameLabel( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, frame, String::EMPTY );
	FINISH_PARAMETERS;

	if ( ! GetFlashValue().GotoAndStop( frame ) )
	{
		GUI_SCRIPT_WARN( TXT("Can't go to frame '%ls'"), frame.AsChar() );
	}
}

void CScriptedFlashSprite::funcGetAlpha( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	Float alpha = 0.f;
	CFlashDisplayInfo flashDisplayInfo;
	if ( GetFlashValue().GetFlashDisplayInfo( flashDisplayInfo ) )
	{
		alpha = static_cast< Float >( flashDisplayInfo.GetAlpha() );
	}
	
	RETURN_FLOAT(  alpha );
}

void CScriptedFlashSprite::funcGetRotation( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	Float rotation = 0.f;
	CFlashDisplayInfo flashDisplayInfo;
	if ( GetFlashValue().GetFlashDisplayInfo( flashDisplayInfo ) )
	{
		rotation = static_cast< Float >( flashDisplayInfo.GetRotation() );
	}

	RETURN_FLOAT(  rotation );
}

void CScriptedFlashSprite::funcGetVisible( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	Bool visible = false;
	CFlashDisplayInfo flashDisplayInfo;
	if ( GetFlashValue().GetFlashDisplayInfo( flashDisplayInfo ) )
	{
		visible = flashDisplayInfo.GetVisible();
	}

	RETURN_BOOL(  visible );
}

void CScriptedFlashSprite::funcGetX( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	Float x = 0.f;
	CFlashDisplayInfo flashDisplayInfo;
	if ( GetFlashValue().GetFlashDisplayInfo( flashDisplayInfo ) )
	{
		x = static_cast< Float >( flashDisplayInfo.GetX() );
	}

	RETURN_FLOAT( x );
}

void CScriptedFlashSprite::funcGetY( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	Float y = 0.f;
	CFlashDisplayInfo flashDisplayInfo;
	if ( GetFlashValue().GetFlashDisplayInfo( flashDisplayInfo ) )
	{
		y = static_cast< Float >( flashDisplayInfo.GetY() );
	}

	RETURN_FLOAT( y );
}

void CScriptedFlashSprite::funcGetZ( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	Float z = 0.f;
	CFlashDisplayInfo flashDisplayInfo;
	if ( GetFlashValue().GetFlashDisplayInfo( flashDisplayInfo ) )
	{
		z = static_cast< Float >( flashDisplayInfo.GetZ() );
	}

	RETURN_FLOAT( z );
}

void CScriptedFlashSprite::funcGetXRotation( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	Float xrot = 0.f;
	CFlashDisplayInfo flashDisplayInfo;
	if ( GetFlashValue().GetFlashDisplayInfo( flashDisplayInfo ) )
	{
		xrot = static_cast< Float >( flashDisplayInfo.GetXRotation() );
	}

	RETURN_FLOAT( xrot );
}

void CScriptedFlashSprite::funcGetYRotation( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	Float yrot = 0.f;
	CFlashDisplayInfo flashDisplayInfo;
	if ( GetFlashValue().GetFlashDisplayInfo( flashDisplayInfo ) )
	{
		yrot = static_cast< Float >( flashDisplayInfo.GetYRotation() );
	}

	RETURN_FLOAT( yrot );
}

void CScriptedFlashSprite::funcGetXScale( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	Float xscale = 1.f;
	CFlashDisplayInfo flashDisplayInfo;
	if ( GetFlashValue().GetFlashDisplayInfo( flashDisplayInfo ) )
	{
		xscale = static_cast< Float >( flashDisplayInfo.GetXScale() );
	}

	RETURN_FLOAT( xscale );
}

void CScriptedFlashSprite::funcGetYScale( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	Float yscale = 1.f;
	CFlashDisplayInfo flashDisplayInfo;
	if ( GetFlashValue().GetFlashDisplayInfo( flashDisplayInfo ) )
	{
		yscale = static_cast< Float >( flashDisplayInfo.GetYScale() );
	}

	RETURN_FLOAT( yscale );
}

void CScriptedFlashSprite::funcGetZScale( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	Float zscale = 1.f;
	CFlashDisplayInfo flashDisplayInfo;
	if ( GetFlashValue().GetFlashDisplayInfo( flashDisplayInfo ) )
	{
		zscale = static_cast< Float >( flashDisplayInfo.GetZScale() );
	}

	RETURN_FLOAT( zscale );
}

void CScriptedFlashSprite::funcSetAlpha( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, alpha, 0.f );
	FINISH_PARAMETERS;

	CFlashDisplayInfo flashDisplayInfo;
	if ( GetFlashValue().GetFlashDisplayInfo( flashDisplayInfo ) )
	{
		flashDisplayInfo.SetAlpha( alpha );
		if ( GetFlashValue().SetFlashDisplayInfo( flashDisplayInfo ) )
		{
			return;
		}
	}

	GUI_SCRIPT_WARN( TXT("Failed to set alpha") );
}

void CScriptedFlashSprite::funcSetRotation( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, rotation, 0.f );
	FINISH_PARAMETERS;

	CFlashDisplayInfo flashDisplayInfo;
	if ( GetFlashValue().GetFlashDisplayInfo( flashDisplayInfo ) )
	{
		flashDisplayInfo.SetRotation( static_cast< Double >( rotation ) );
		if ( GetFlashValue().SetFlashDisplayInfo( flashDisplayInfo ) )
		{
			return;
		}
	}

	GUI_SCRIPT_WARN( TXT("Failed to set rotation") );
}

void CScriptedFlashSprite::funcSetVisible( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, visible, false );
	FINISH_PARAMETERS;

	CFlashDisplayInfo flashDisplayInfo;
	if ( GetFlashValue().GetFlashDisplayInfo( flashDisplayInfo ) )
	{
		flashDisplayInfo.SetVisible( visible );
		if ( GetFlashValue().SetFlashDisplayInfo( flashDisplayInfo ) )
		{
			return;
		}
	}

	GUI_SCRIPT_WARN( TXT("Failed to set visible") );
}

void CScriptedFlashSprite::funcSetPosition( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, x, 0.f );
	GET_PARAMETER( Float, y, 0.f );
	FINISH_PARAMETERS;

	CFlashDisplayInfo flashDisplayInfo;
	if ( GetFlashValue().GetFlashDisplayInfo( flashDisplayInfo ) )
	{
		flashDisplayInfo.SetPosition( static_cast< Double >( x ), static_cast< Double >( y ) );
		if ( GetFlashValue().SetFlashDisplayInfo( flashDisplayInfo ) )
		{
			return;
		}
	}

	GUI_SCRIPT_WARN( TXT("Failed to set position") );
}

void CScriptedFlashSprite::funcSetScale( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, xscale, 1.f );
	GET_PARAMETER( Float, yscale, 1.f );
	FINISH_PARAMETERS;

	CFlashDisplayInfo flashDisplayInfo;
	if ( GetFlashValue().GetFlashDisplayInfo( flashDisplayInfo ) )
	{
		flashDisplayInfo.SetScale( static_cast< Double >( xscale ), static_cast< Double >( yscale ) );
		if ( GetFlashValue().SetFlashDisplayInfo( flashDisplayInfo ) )
		{
			return;
		}
	}

	GUI_SCRIPT_WARN( TXT("Failed to set scale") );
}

void CScriptedFlashSprite::funcSetX( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, x, 0.f );
	FINISH_PARAMETERS;

	CFlashDisplayInfo flashDisplayInfo;
	if ( GetFlashValue().GetFlashDisplayInfo( flashDisplayInfo ) )
	{
		flashDisplayInfo.SetX( static_cast< Double >( x ) );
		if ( GetFlashValue().SetFlashDisplayInfo( flashDisplayInfo ) )
		{
			return;
		}
	}

	GUI_SCRIPT_WARN( TXT("Failed to set X") );
}

void CScriptedFlashSprite::funcSetY( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, y, 0.f );
	FINISH_PARAMETERS;

	CFlashDisplayInfo flashDisplayInfo;
	if ( GetFlashValue().GetFlashDisplayInfo( flashDisplayInfo ) )
	{
		flashDisplayInfo.SetY( static_cast< Double >( y ) );
		if ( GetFlashValue().SetFlashDisplayInfo( flashDisplayInfo ) )
		{
			return;
		}
	}

	GUI_SCRIPT_WARN( TXT("Failed to set Y") );
}

void CScriptedFlashSprite::funcSetZ( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, z, 0.f );
	FINISH_PARAMETERS;

	CFlashDisplayInfo flashDisplayInfo;
	if ( GetFlashValue().GetFlashDisplayInfo( flashDisplayInfo ) )
	{
		flashDisplayInfo.SetZ( static_cast< Double >( z ) );
		if ( GetFlashValue().SetFlashDisplayInfo( flashDisplayInfo ) )
		{
			return;
		}
	}

	GUI_SCRIPT_WARN( TXT("Failed to set Z") );
}

void CScriptedFlashSprite::funcSetXRotation( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, degrees, 0.f );
	FINISH_PARAMETERS;

	CFlashDisplayInfo flashDisplayInfo;
	if ( GetFlashValue().GetFlashDisplayInfo( flashDisplayInfo ) )
	{
		flashDisplayInfo.SetXRotation( static_cast< Double >( degrees ) );
		if ( GetFlashValue().SetFlashDisplayInfo( flashDisplayInfo ) )
		{
			return;
		}
	}

	GUI_SCRIPT_WARN( TXT("Failed to set X rotation") );
}

void CScriptedFlashSprite::funcSetYRotation( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, degrees, 0.f );
	FINISH_PARAMETERS;

	CFlashDisplayInfo flashDisplayInfo;
	if ( GetFlashValue().GetFlashDisplayInfo( flashDisplayInfo ) )
	{
		flashDisplayInfo.SetYRotation( static_cast< Double >( degrees ) );
		if ( GetFlashValue().SetFlashDisplayInfo( flashDisplayInfo ) )
		{
			return;
		}
	}

	GUI_SCRIPT_WARN( TXT("Failed to set Y rotation") );
}

void CScriptedFlashSprite::funcSetXScale( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, xscale, 1.f );
	FINISH_PARAMETERS;

	CFlashDisplayInfo flashDisplayInfo;
	if ( GetFlashValue().GetFlashDisplayInfo( flashDisplayInfo ) )
	{
		flashDisplayInfo.SetXScale( static_cast< Double >( xscale ) );
		if ( GetFlashValue().SetFlashDisplayInfo( flashDisplayInfo ) )
		{
			return;
		}
	}

	GUI_SCRIPT_WARN( TXT("Failed to set X scale") );
}

void CScriptedFlashSprite::funcSetYScale( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, yscale, 1.f );
	FINISH_PARAMETERS;

	CFlashDisplayInfo flashDisplayInfo;
	if ( GetFlashValue().GetFlashDisplayInfo( flashDisplayInfo ) )
	{
		flashDisplayInfo.SetYScale( static_cast< Double >( yscale ) );
		if ( GetFlashValue().SetFlashDisplayInfo( flashDisplayInfo ) )
		{
			return;
		}
	}

	GUI_SCRIPT_WARN( TXT("Failed to set Y scale") );
}

void CScriptedFlashSprite::funcSetZScale( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, zscale, 1.f );
	FINISH_PARAMETERS;

	CFlashDisplayInfo flashDisplayInfo;
	if ( GetFlashValue().GetFlashDisplayInfo( flashDisplayInfo ) )
	{
		flashDisplayInfo.SetZScale( static_cast< Double >( zscale ) );
		if ( GetFlashValue().SetFlashDisplayInfo( flashDisplayInfo ) )
		{
			return;
		}
	}

	GUI_SCRIPT_WARN( TXT("Failed to set Z scale") );
}

//////////////////////////////////////////////////////////////////////////
// CScriptedFlashArray
//////////////////////////////////////////////////////////////////////////
CScriptedFlashArray::CScriptedFlashArray( CScriptedFlashObjectPool& pool, const CFlashValue& flashValue, const String& name /*= String::EMPTY*/  )
	: TBaseClass( pool )
	, m_flashValue( flashValue )
#ifndef RED_FINAL_BUILD
	, m_name( name )
#endif
{
#ifndef RED_FINAL_BUILD
	if ( ! GetFlashValue().IsFlashArray() )
	{
		GUI_ERROR(TXT("CScriptedFlashArray '%ls' not initialized with an array!"), name.AsChar() );
	}
#endif
}

CScriptedFlashArray::~CScriptedFlashArray()
{
	m_flashValue.Clear();
}

void CScriptedFlashArray::funcClearElements( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	if ( ! m_flashValue.ClearArrayElements() )
	{
		GUI_SCRIPT_WARN( TXT("Failed to clear array elements") );
	}
}

void CScriptedFlashArray::funcGetLength( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_INT( static_cast< Int32 >( m_flashValue.GetArrayLength() ) );
}

void CScriptedFlashArray::funcSetLength( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int32, length, 0 );
	FINISH_PARAMETERS;

	if ( length < 0 )
	{
		GUI_SCRIPT_WARN( TXT("Array length '%d' cannot be negative"), length );
	}
	else if ( ! m_flashValue.SetArrayLength( static_cast< Uint32 >( length ) ) )
	{
		GUI_SCRIPT_WARN( TXT("Failed to set array length '%d'"), length  );
	}
}

Bool CScriptedFlashArray::GetArrayElement( Int32 index, CFlashValue& outFlashValue )
{
	if ( index < 0 )
	{
		GUI_SCRIPT_WARN( TXT("Array index '%d' cannot be negative"), index );
		return false;
	}
	else if ( ! m_flashValue.GetArrayElement( static_cast< Uint32 >( index ), outFlashValue ) )
	{
		GUI_SCRIPT_WARN( TXT("Failed to get array element '%d'"), index );
		return false;
	}
	return true;
}

void CScriptedFlashArray::funcGetElementFlashBool( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int32, index, 0 );
	FINISH_PARAMETERS;

	CFlashValue tmpFlashValue;
	if ( GetArrayElement( index, tmpFlashValue ) && ! tmpFlashValue.IsFlashBool() )
	{
		GUI_SCRIPT_WARN_ONCE( stack, TXT("Array element '%d' is not Bool"), index );
	}
	RETURN_BOOL( tmpFlashValue.GetFlashBool() );
}

void CScriptedFlashArray::funcGetElementFlashInt( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int32, index, 0 );
	FINISH_PARAMETERS;

	CFlashValue tmpFlashValue;
	if ( GetArrayElement( index, tmpFlashValue ) && ! tmpFlashValue.IsFlashInt() )
	{
		GUI_SCRIPT_WARN_ONCE( stack, TXT("Array element '%d' is not int"), index );
	}

	RETURN_INT( tmpFlashValue.GetFlashInt() );
}

void CScriptedFlashArray::funcGetElementFlashUInt( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int32, index, 0 );
	FINISH_PARAMETERS;

	CFlashValue tmpFlashValue;
	if ( GetArrayElement( index, tmpFlashValue ) && ! tmpFlashValue.IsFlashUInt() )
	{
		GUI_SCRIPT_WARN_ONCE( stack, TXT("Array element '%d' is not uint"), index );
	}

	RETURN_INT( static_cast< Int32 >( tmpFlashValue.GetFlashUInt() ) );
}

void CScriptedFlashArray::funcGetElementFlashNumber( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int32, index, 0 );
	FINISH_PARAMETERS;

	CFlashValue tmpFlashValue;
	if ( GetArrayElement( index, tmpFlashValue ) && ! tmpFlashValue.IsFlashNumber() )
	{
		GUI_SCRIPT_WARN_ONCE( stack, TXT("Array element '%d' is not Number"), index );
	}

	RETURN_FLOAT( static_cast< Float >( tmpFlashValue.GetFlashNumber() ) );
}

void CScriptedFlashArray::funcGetElementFlashString( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int32, index, 0 );
	FINISH_PARAMETERS;

	CFlashValue tmpFlashValue;
	if ( GetArrayElement( index, tmpFlashValue ) && ! tmpFlashValue.IsFlashString() )
	{
		GUI_SCRIPT_WARN_ONCE( stack, TXT("Array element '%d' is not String"), index );
	}

	RETURN_STRING( tmpFlashValue.GetFlashString() );
}

void CScriptedFlashArray::funcGetElementFlashObject( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int32, index, 0 );
	FINISH_PARAMETERS;

	CScriptedFlashObject* newScriptedFlashValue = nullptr;
	CFlashValue tmpFlashValue;
	if ( ! GetArrayElement( index, tmpFlashValue ) )
	{
		GUI_SCRIPT_WARN_ONCE( stack, TXT("Array element '%d' is not Object"), index );
	}
	else if ( tmpFlashValue.IsFlashNull() )
	{
		GUI_SCRIPT_WARN( TXT("Array element '%d' is null"), index );
	}
	else if ( ! tmpFlashValue.IsFlashObject() )
	{
		GUI_SCRIPT_WARN_ONCE( stack, TXT("Array element is '%d' is not an object"), index );
	}
	else
	{
		newScriptedFlashValue = new CScriptedFlashObject( GetFlashObjectPool(), tmpFlashValue, FLASH_MBR_STR(String::Printf(TXT("[Object] @ Array %ls[%i]"), m_name.AsChar(), index)) );;
	}

	RETURN_HANDLE( CScriptedFlashObject, newScriptedFlashValue );
}

void CScriptedFlashArray::funcPopBack( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	CFlashValue unusedFlashValue;
	m_flashValue.PopArrayBack( unusedFlashValue );
}

void CScriptedFlashArray::funcSetElementFlashObject( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int32, index, 0 );
	GET_PARAMETER( THandle< CScriptedFlashObject >, value, nullptr );
	FINISH_PARAMETERS;

	CScriptedFlashObject* scriptedFlashObject = value.Get();

	if ( index < 0 )
	{
		GUI_SCRIPT_WARN( TXT("Array index '%d' cannot be negative"), index );
	}
	else if ( scriptedFlashObject && ! m_flashValue.SetArrayElementFlashValue( static_cast< Uint32 >( index ), scriptedFlashObject->m_flashValue ) )
	{
		GUI_SCRIPT_WARN( TXT("Failed to set object at array element '%d'"), index );
	}
	else if ( ! scriptedFlashObject && m_flashValue.SetArrayElementFlashNull( static_cast< Uint32 >( index ) ) )
	{
		GUI_SCRIPT_WARN( TXT("Failed to set null at array element '%d'"), index );
	}
}

void CScriptedFlashArray::funcSetElementFlashString( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int32, index, 0 );
	GET_PARAMETER( String, value, String::EMPTY );
	FINISH_PARAMETERS;

	if ( index < 0 )
	{
		GUI_SCRIPT_WARN( TXT("Array index '%d' cannot be negative"), index );
	}
	else if ( ! m_flashValue.SetArrayElementFlashString( static_cast< Uint32 >( index ), value ) )
	{
		GUI_SCRIPT_WARN( TXT("Failed to set string '%ls' at array element '%d'"), value.AsChar(), index );
	}
}

void CScriptedFlashArray::funcSetElementFlashBool( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int32, index, 0 );
	GET_PARAMETER( Bool, value, false );
	FINISH_PARAMETERS;

	if ( index < 0 )
	{
		GUI_SCRIPT_WARN( TXT("Array index '%d' cannot be negative"), index );
	}
	else if ( ! m_flashValue.SetArrayElementFlashBool( static_cast< Uint32 >( index ), value ) )
	{
		GUI_SCRIPT_WARN( TXT("Failed to set bool at array element '%d'"), index );
	}
}

void CScriptedFlashArray::funcSetElementFlashInt( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int32, index, 0 );
	GET_PARAMETER( Int32, value, false );
	FINISH_PARAMETERS;

	if ( index < 0 )
	{
		GUI_SCRIPT_WARN( TXT("Array index '%d' cannot be negative"), index );
	}
	else if ( ! m_flashValue.SetArrayElementFlashInt( static_cast< Uint32 >( index ), value ) )
	{
		GUI_SCRIPT_WARN( TXT("Failed to set int at array element '%d'"), index );
	}
}

void CScriptedFlashArray::funcSetElementFlashUInt( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int32, index, 0 );
	GET_PARAMETER( Int32, value, false );
	FINISH_PARAMETERS;

	if ( index < 0 )
	{
		GUI_SCRIPT_WARN( TXT("Array index '%d' cannot be negative"), index );
	}
	else if ( ! m_flashValue.SetArrayElementFlashUInt( static_cast< Uint32 >( index ), static_cast< Uint32 >( value ) ) )
	{
		GUI_SCRIPT_WARN( TXT("Failed to set uint at array element '%d'"), index );
	}
}

void CScriptedFlashArray::funcSetElementFlashNumber( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int32, index, 0 );
	GET_PARAMETER( Float, value, false );
	FINISH_PARAMETERS;

	if ( index < 0 )
	{
		GUI_SCRIPT_WARN( TXT("Array index '%d' cannot be negative"), index );
	}
	else if ( ! m_flashValue.SetArrayElementFlashNumber( static_cast< Uint32 >( index ), static_cast< Float >( value ) ) )
	{
		GUI_SCRIPT_WARN( TXT("Failed to set Number at array element '%d'"), index );
	}
}

void CScriptedFlashArray::funcPushBackFlashObject( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CScriptedFlashObject >, value, nullptr );
	FINISH_PARAMETERS;

	CScriptedFlashObject* scriptedFlashObject = value.Get();

	if ( scriptedFlashObject && ! m_flashValue.PushArrayBackFlashValue( scriptedFlashObject->m_flashValue ) )
	{
		GUI_SCRIPT_WARN( TXT("Failed to push back object") );
	}
	else if ( ! scriptedFlashObject && ! m_flashValue.PushArrayBackFlashNull() )
	{
		GUI_SCRIPT_WARN( TXT("Failed to push back null") );
	}
}

void CScriptedFlashArray::funcPushBackFlashString( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, value, String::EMPTY );
	FINISH_PARAMETERS;

	if ( ! m_flashValue.PushArrayBackFlashString( value ) )
	{
		GUI_SCRIPT_WARN( TXT("Failed to push back string '%ls'"), value.AsChar() );
	}
}

void CScriptedFlashArray::funcPushBackFlashBool( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, value, false );
	FINISH_PARAMETERS;

	if ( ! m_flashValue.PushArrayBackFlashBool( value ) )
	{
		GUI_SCRIPT_WARN( TXT("Failed to push back bool") );
	}
}

void CScriptedFlashArray::funcPushBackFlashInt( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int32, value, false );
	FINISH_PARAMETERS;

	if ( ! m_flashValue.PushArrayBackFlashInt( value ) )
	{
		GUI_SCRIPT_WARN( TXT("Failed to push back int") );
	}
}

void CScriptedFlashArray::funcPushBackFlashUInt( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int32, value, false );
	FINISH_PARAMETERS;

	if ( ! m_flashValue.PushArrayBackFlashUInt( static_cast< Uint32 >( value ) ) )
	{
		GUI_SCRIPT_WARN( TXT("Failed to push back uint") );
	}
}

void CScriptedFlashArray::funcPushBackFlashNumber( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, value, false );
	FINISH_PARAMETERS;

	if ( ! m_flashValue.PushArrayBackFlashNumber( static_cast< Double >( value ) ) )
	{
		GUI_SCRIPT_WARN( TXT("Failed to push back Number") );
	}
}

void CScriptedFlashArray::funcRemoveElement( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int32, index, 0 );
	FINISH_PARAMETERS;

	if ( index < 0 )
	{
		GUI_SCRIPT_WARN( TXT("Array index '%d' cannot be negative"), index );
	}
	else if ( ! m_flashValue.RemoveArrayElement( static_cast< Uint32 >( index ) ) )
	{
		GUI_SCRIPT_WARN( TXT("Could not remove array element at index '%d'"), index );
	}
}

void CScriptedFlashArray::funcRemoveElements( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int32, index, 0 );
	GET_PARAMETER_OPT( Int32, count, -1 );
	FINISH_PARAMETERS;

	if ( index < 0 )
	{
		GUI_SCRIPT_WARN( TXT("Array index '%d' cannot be negative"), index );
	}
	else if ( count < -1 )
	{
		GUI_SCRIPT_WARN( TXT("Element count '%d' cannot be less than minus one"), count );
	}
	else if ( ! m_flashValue.RemoveArrayElements( static_cast< Uint32 >( index ), count ) )
	{
		GUI_SCRIPT_WARN( TXT("Could not remove array elements at index '%d' count '%d'"), index, count );
	}
}

//////////////////////////////////////////////////////////////////////////
// CScriptedFlashTextField
//////////////////////////////////////////////////////////////////////////
CScriptedFlashTextField::CScriptedFlashTextField( CScriptedFlashObjectPool& pool, const CFlashValue& flashValue, const String& name /*= String::EMPTY*/ )
	: TBaseClass( pool )
	, m_flashValue( flashValue )
#ifndef RED_FINAL_BUILD
	, m_name( name )
#endif
{
	RED_UNUSED( name );
#ifndef RED_FINAL_BUILD
	if ( ! GetFlashValue().IsInstanceOf(TXT("flash.text.TextField")) )
	{
		GUI_ERROR(TXT("CScriptedFlashTextField '%ls' not initialized with a text field!"), name.AsChar() );
	}
#endif
}

CScriptedFlashTextField::~CScriptedFlashTextField()
{
	m_flashValue.Clear();
}

void CScriptedFlashTextField::funcGetText( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_STRING( m_flashValue.GetText() );
}

void CScriptedFlashTextField::funcGetTextHtml( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_STRING( m_flashValue.GetTextHtml() );
}

void CScriptedFlashTextField::funcSetText( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, text, String::EMPTY );
	FINISH_PARAMETERS;

	if ( ! m_flashValue.SetText( text ) )
	{
		GUI_SCRIPT_WARN( TXT("Could not set text '%ls'"), text.AsChar() );
	}
}

void CScriptedFlashTextField::funcSetTextHtml( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, htmlText, String::EMPTY );
	FINISH_PARAMETERS;

	if ( ! m_flashValue.SetTextHtml( htmlText) ) 
	{
		GUI_SCRIPT_WARN( TXT("Could not set htmlText '%ls'"), htmlText.AsChar() );
	}
}

//////////////////////////////////////////////////////////////////////////
// CScriptedFlashFunction
//////////////////////////////////////////////////////////////////////////
CScriptedFlashFunction::CScriptedFlashFunction( CScriptedFlashObjectPool& pool, const CFlashValue& flashValue, const String& name /*= String::EMPTY*/ )
	: TBaseClass( pool )
	, m_flashValue( flashValue )
#ifndef RED_FINAL_BUILD
	, m_name( name )
#endif
{
	RED_UNUSED( name );
#ifndef RED_FINAL_BUILD
	if ( ! flashValue.IsFlashClosure() )
	{
		GUI_ERROR(TXT("CScriptedFlashFunction '%ls' not initialized with a closure!"), name.AsChar() );
	}
#endif
}

CScriptedFlashFunction::~CScriptedFlashFunction()
{
	m_flashValue.Clear();
}

void CScriptedFlashFunction::funcInvokeSelf( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	
	if ( ! m_flashValue.InvokeSelf() )
	{
		GUI_SCRIPT_WARN( TXT("Failed to invoke function") );
	}
}

void CScriptedFlashFunction::funcInvokeSelfOneArg( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SFlashArg, arg0, SFlashArg() );
	FINISH_PARAMETERS;
	
	if ( ! m_flashValue.InvokeSelf( GetFlashValueFromArg( arg0 ) ) )
	{
		GUI_SCRIPT_WARN( TXT("Failed to invoke function") );
	}
}

void CScriptedFlashFunction::funcInvokeSelfTwoArgs( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SFlashArg, arg0, SFlashArg() );
	GET_PARAMETER( SFlashArg, arg1, SFlashArg() );
	FINISH_PARAMETERS;

	if ( ! m_flashValue.InvokeSelf( GetFlashValueFromArg( arg0 ), GetFlashValueFromArg( arg1 ) ) )
	{
		GUI_SCRIPT_WARN( TXT("Failed to invoke function") );
	}
}

void CScriptedFlashFunction::funcInvokeSelfThreeArgs( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SFlashArg, arg0, SFlashArg() );
	GET_PARAMETER( SFlashArg, arg1, SFlashArg() );
	GET_PARAMETER( SFlashArg, arg2, SFlashArg() );
	FINISH_PARAMETERS;

	if ( ! m_flashValue.InvokeSelf( GetFlashValueFromArg( arg0 ), GetFlashValueFromArg( arg1 ), GetFlashValueFromArg( arg2 ) ) )
	{
		GUI_SCRIPT_WARN( TXT("Failed to invoke function") );
	}
}

void CScriptedFlashFunction::funcInvokeSelfFourArgs( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SFlashArg, arg0, SFlashArg() );
	GET_PARAMETER( SFlashArg, arg1, SFlashArg() );
	GET_PARAMETER( SFlashArg, arg2, SFlashArg() );
	GET_PARAMETER( SFlashArg, arg3, SFlashArg() );
	FINISH_PARAMETERS;

	if ( ! m_flashValue.InvokeSelf( GetFlashValueFromArg( arg0 ), GetFlashValueFromArg( arg1 ), GetFlashValueFromArg( arg2 ), GetFlashValueFromArg( arg3 ) ) )
	{
		GUI_SCRIPT_WARN( TXT("Failed to invoke function") );
	}
}

void CScriptedFlashFunction::funcInvokeSelfFiveArgs( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SFlashArg, arg0, SFlashArg() );
	GET_PARAMETER( SFlashArg, arg1, SFlashArg() );
	GET_PARAMETER( SFlashArg, arg2, SFlashArg() );
	GET_PARAMETER( SFlashArg, arg3, SFlashArg() );
	GET_PARAMETER( SFlashArg, arg4, SFlashArg() );
	FINISH_PARAMETERS;

	if ( ! m_flashValue.InvokeSelf( GetFlashValueFromArg( arg0 ), GetFlashValueFromArg( arg1 ), GetFlashValueFromArg( arg2 ), GetFlashValueFromArg( arg3 ), GetFlashValueFromArg( arg4 )  ) )
	{
		GUI_SCRIPT_WARN( TXT("Failed to invoke function") );
	}
}


void CScriptedFlashFunction::funcInvokeSelfSixArgs( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SFlashArg, arg0, SFlashArg() );
	GET_PARAMETER( SFlashArg, arg1, SFlashArg() );
	GET_PARAMETER( SFlashArg, arg2, SFlashArg() );
	GET_PARAMETER( SFlashArg, arg3, SFlashArg() );
	GET_PARAMETER( SFlashArg, arg4, SFlashArg() );
	GET_PARAMETER( SFlashArg, arg5, SFlashArg() );
	FINISH_PARAMETERS;

	if ( ! m_flashValue.InvokeSelf( GetFlashValueFromArg( arg0 ), GetFlashValueFromArg( arg1 ), GetFlashValueFromArg( arg2 ), GetFlashValueFromArg( arg3 ), GetFlashValueFromArg( arg4 ), GetFlashValueFromArg( arg5 ) ) )
	{
		GUI_SCRIPT_WARN( TXT("Failed to invoke function") );
	}
}

void CScriptedFlashFunction::funcInvokeSelfSevenArgs( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SFlashArg, arg0, SFlashArg() );
	GET_PARAMETER( SFlashArg, arg1, SFlashArg() );
	GET_PARAMETER( SFlashArg, arg2, SFlashArg() );
	GET_PARAMETER( SFlashArg, arg3, SFlashArg() );
	GET_PARAMETER( SFlashArg, arg4, SFlashArg() );
	GET_PARAMETER( SFlashArg, arg5, SFlashArg() );
	GET_PARAMETER( SFlashArg, arg6, SFlashArg() );
	FINISH_PARAMETERS;

	if ( ! m_flashValue.InvokeSelf( GetFlashValueFromArg( arg0 ), GetFlashValueFromArg( arg1 ), GetFlashValueFromArg( arg2 ), GetFlashValueFromArg( arg3 ), GetFlashValueFromArg( arg4 ), GetFlashValueFromArg( arg5 ), GetFlashValueFromArg( arg6 ) ) )
	{
		GUI_SCRIPT_WARN( TXT("Failed to invoke function") );
	}
}

void CScriptedFlashFunction::funcInvokeSelfEightArgs( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SFlashArg, arg0, SFlashArg() );
	GET_PARAMETER( SFlashArg, arg1, SFlashArg() );
	GET_PARAMETER( SFlashArg, arg2, SFlashArg() );
	GET_PARAMETER( SFlashArg, arg3, SFlashArg() );
	GET_PARAMETER( SFlashArg, arg4, SFlashArg() );
	GET_PARAMETER( SFlashArg, arg5, SFlashArg() );
	GET_PARAMETER( SFlashArg, arg6, SFlashArg() );
	GET_PARAMETER( SFlashArg, arg7, SFlashArg() );
	FINISH_PARAMETERS;

	if ( ! m_flashValue.InvokeSelf( GetFlashValueFromArg( arg0 ), GetFlashValueFromArg( arg1 ), GetFlashValueFromArg( arg2 ), GetFlashValueFromArg( arg3 ), GetFlashValueFromArg( arg4 ), GetFlashValueFromArg( arg5 ), GetFlashValueFromArg( arg6 ), GetFlashValueFromArg( arg7 ) ) )
	{
		GUI_SCRIPT_WARN( TXT("Failed to invoke function") );
	}
}

void CScriptedFlashFunction::funcInvokeSelfNineArgs( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SFlashArg, arg0, SFlashArg() );
	GET_PARAMETER( SFlashArg, arg1, SFlashArg() );
	GET_PARAMETER( SFlashArg, arg2, SFlashArg() );
	GET_PARAMETER( SFlashArg, arg3, SFlashArg() );
	GET_PARAMETER( SFlashArg, arg4, SFlashArg() );
	GET_PARAMETER( SFlashArg, arg5, SFlashArg() );
	GET_PARAMETER( SFlashArg, arg6, SFlashArg() );
	GET_PARAMETER( SFlashArg, arg7, SFlashArg() );
	GET_PARAMETER( SFlashArg, arg8, SFlashArg() );
	FINISH_PARAMETERS;

	if ( ! m_flashValue.InvokeSelf( GetFlashValueFromArg( arg0 ), GetFlashValueFromArg( arg1 ), GetFlashValueFromArg( arg2 ), GetFlashValueFromArg( arg3 ), GetFlashValueFromArg( arg4 ), GetFlashValueFromArg( arg5 ), GetFlashValueFromArg( arg6 ), GetFlashValueFromArg( arg7 ), GetFlashValueFromArg( arg8 ) ) )
	{
		GUI_SCRIPT_WARN( TXT("Failed to invoke function") );
	}
}

CFlashValue CScriptedFlashFunction::GetFlashValueFromArg( const SFlashArg& flashArg ) const
{
	CFlashValue flashValue;
	switch ( flashArg.m_valType )
	{
	case SFlashArg::FlashArgType_Bool:
		flashValue.SetFlashBool( flashArg.u.m_boolVal );
		break;
	case SFlashArg::FlashArgType_Int:
		flashValue.SetFlashInt( flashArg.u.m_intVal );
		break;
	case SFlashArg::FlashArgType_UInt:
		flashValue.SetFlashUInt( flashArg.u.m_uintVal );
		break;
	case SFlashArg::FlashArgType_Number:
		flashValue.SetFlashNumber( static_cast< Double >( flashArg.u.m_numberVal ) );
		break;
	case SFlashArg::FlashArgType_String:
		VERIFY( m_flashValue.CreateString( flashArg.m_stringVal, flashValue ) );
		break;
	default:
		HALT( "Unknown flash value type!" );
		break;
	}
	return flashValue;
}

//////////////////////////////////////////////////////////////////////////
// CScriptedFlashValueStorage
//////////////////////////////////////////////////////////////////////////
CScriptedFlashValueStorage::CScriptedFlashValueStorage( CScriptedFlashObjectPool& pool, CFlashValueStorage* flashValueStorage, CFlashMovie* flashValueStorageMovie )
	: TBaseClass( pool )
	, m_flashValueStorage( nullptr )
	, m_flashValueStorageMovie( flashValueStorageMovie )
{
	ASSERT( flashValueStorage );
	ASSERT( flashValueStorageMovie );

	m_flashValueStorage = flashValueStorage;
	if ( m_flashValueStorage )
	{
		m_flashValueStorage->AddRef();
	}

	m_flashValueStorageMovie = flashValueStorageMovie;
	if ( m_flashValueStorageMovie )
	{
		m_flashValueStorageMovie->AddRef();
	}
}

CScriptedFlashValueStorage::~CScriptedFlashValueStorage()
{
	ASSERT( m_flashValueStorageMovie );
	ASSERT( m_flashValueStorage );

	if ( m_flashValueStorageMovie )
	{
		m_flashValueStorageMovie->Release();
	}

	if ( m_flashValueStorage )
	{
		m_flashValueStorage->Release();
	}
}

void CScriptedFlashValueStorage::funcSetFlashObject( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, key, String::EMPTY );
	GET_PARAMETER( THandle< CScriptedFlashObject >, value, nullptr );
	GET_PARAMETER_OPT( Int32, index, -1 );
	FINISH_PARAMETERS;

	ASSERT( m_flashValueStorage );

	const CScriptedFlashObject* scriptedFlashObject = value.Get();
	CFlashValue nullFlashValue;
	nullFlashValue.SetFlashNull();

	if ( scriptedFlashObject && ! m_flashValueStorage->SetFlashValue( key, scriptedFlashObject->m_flashValue, index ) )
	{
		GUI_SCRIPT_WARN( TXT("Failed to set object for storage on key '%ls'"), key.AsChar() );
	}
	else if ( ! scriptedFlashObject && ! m_flashValueStorage->SetFlashValue( key, nullFlashValue, index ) )
	{
		GUI_SCRIPT_WARN( TXT("Failed to set null for storage on key '%ls'"), key.AsChar() );
	}
}

void CScriptedFlashValueStorage::funcSetFlashArray( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, key, String::EMPTY );
	GET_PARAMETER( THandle< CScriptedFlashArray >, value, nullptr );
	FINISH_PARAMETERS;

	const CScriptedFlashArray* scriptedFlashArray = value.Get();
	CFlashValue nullFlashValue;
	nullFlashValue.SetFlashNull();

	ASSERT( m_flashValueStorage );
	if ( scriptedFlashArray && ! m_flashValueStorage->SetFlashValue( key, scriptedFlashArray->m_flashValue ) )
	{
		GUI_SCRIPT_WARN( TXT("Failed to set array for storage on key '%ls'"), key.AsChar() );
	}
	else if ( ! scriptedFlashArray && ! m_flashValueStorage->SetFlashValue( key, nullFlashValue ) )
	{
		GUI_SCRIPT_WARN( TXT("Failed to set null for storage on key '%ls'"), key.AsChar() );
	}
}

void CScriptedFlashValueStorage::funcSetFlashString( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, key, String::EMPTY );
	GET_PARAMETER( String, value, String::EMPTY );
	GET_PARAMETER_OPT( Int32, index, -1 );
	FINISH_PARAMETERS;

	CFlashString* flashString = m_flashValueStorageMovie->CreateString( value );
	ASSERT( flashString );

	ASSERT( m_flashValueStorage );
	if ( ! m_flashValueStorage->SetFlashValue( key, flashString->AsFlashValue(), index ) )
	{
		GUI_SCRIPT_WARN( TXT("Failed to set string '%ls' for storage on key '%ls'"), value.AsChar(), key.AsChar() );
	}

	flashString->Release();
}

void CScriptedFlashValueStorage::funcSetFlashBool( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, key, String::EMPTY );
	GET_PARAMETER( Bool, value, false );
	GET_PARAMETER_OPT( Int32, index, -1 );
	FINISH_PARAMETERS;

	ASSERT( m_flashValueStorage );
	if ( ! m_flashValueStorage->SetFlashValue( key, CFlashValue( value ), index ) )
	{
		GUI_SCRIPT_WARN( TXT("Failed to set bool for storage on key '%ls'"), key.AsChar() );
	}
}

void CScriptedFlashValueStorage::funcSetFlashInt( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, key, String::EMPTY );
	GET_PARAMETER( Int32, value, 0 );
	GET_PARAMETER_OPT( Int32, index, -1 );
	FINISH_PARAMETERS;

	ASSERT( m_flashValueStorage );
	if ( ! m_flashValueStorage->SetFlashValue( key, CFlashValue( value ), index ) )
	{
		GUI_SCRIPT_WARN( TXT("Failed to set int for storage on key '%ls'"), key.AsChar() );
	}
}

void CScriptedFlashValueStorage::funcSetFlashUInt( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, key, String::EMPTY );
	GET_PARAMETER( Int32, value, false );
	GET_PARAMETER_OPT( Int32, index, -1 );
	FINISH_PARAMETERS;

	ASSERT( m_flashValueStorage );
	if ( ! m_flashValueStorage->SetFlashValue( key, CFlashValue( static_cast< Uint32 >( value ) ), index ) )
	{
		GUI_SCRIPT_WARN( TXT("Failed to set uint for storage on key '%ls'"), key.AsChar() );
	}
}

void CScriptedFlashValueStorage::funcSetFlashNumber( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, key, String::EMPTY );
	GET_PARAMETER( Float, value, 0.f );
	GET_PARAMETER_OPT( Int32, index, -1 );
	FINISH_PARAMETERS;

	ASSERT( m_flashValueStorage );
	if ( ! m_flashValueStorage->SetFlashValue( key, CFlashValue( static_cast< Double >( value ) ), index ) )
	{
		GUI_SCRIPT_WARN( TXT("Failed to set number for storage on key '%ls'"), key.AsChar() );
	}
}

void CScriptedFlashValueStorage::funcCreateTempFlashObject( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_OPT( String, flashClassName, TXT("Object") );
	FINISH_PARAMETERS;

	CFlashObject* flashObjectRef = m_flashValueStorageMovie->CreateObject( flashClassName );
	ASSERT( flashObjectRef );

	CScriptedFlashObject* newScriptedFlashObject = new CScriptedFlashObject( GetFlashObjectPool(), flashObjectRef->AsFlashValue(), FLASH_OBJ_TXT("[Temp Object]") );
	flashObjectRef->Release();

	RETURN_HANDLE( CScriptedFlashObject, newScriptedFlashObject );
}

void CScriptedFlashValueStorage::funcCreateTempFlashArray( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	CFlashArray* flashArrayRef = m_flashValueStorageMovie->CreateArray();
	ASSERT( flashArrayRef );

	CScriptedFlashArray* newScriptedFlashArray = new CScriptedFlashArray( GetFlashObjectPool(), flashArrayRef->AsFlashValue(), FLASH_OBJ_TXT("[Temp Array]") );
	flashArrayRef->Release();

	RETURN_HANDLE( CScriptedFlashArray, newScriptedFlashArray );
}

//////////////////////////////////////////////////////////////////////////
// RegisterFlashFunctions
//////////////////////////////////////////////////////////////////////////
static void funcFlashArgBool( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, value, false );
	FINISH_PARAMETERS;

	SFlashArg arg;
	arg.u.m_boolVal = value;
	arg.m_valType = SFlashArg::FlashArgType_Bool;
	RETURN_STRUCT( SFlashArg, arg );
}

static void funcFlashArgInt( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int32, value, 0 );
	FINISH_PARAMETERS;

	SFlashArg arg;
	arg.u.m_intVal = value;
	arg.m_valType = SFlashArg::FlashArgType_Int;
	RETURN_STRUCT( SFlashArg, arg );
}

static void funcFlashArgUInt( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int32, value, 0 );
	FINISH_PARAMETERS;

	SFlashArg arg;
	arg.u.m_uintVal = static_cast< Uint32 >( value );
	arg.m_valType = SFlashArg::FlashArgType_UInt;
	RETURN_STRUCT( SFlashArg, arg );
}

static void funcFlashArgNumber( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, value, 0 );
	FINISH_PARAMETERS;

	SFlashArg arg;
	arg.u.m_numberVal = value;
	arg.m_valType = SFlashArg::FlashArgType_Number;
	RETURN_STRUCT( SFlashArg, arg );
}

static void funcFlashArgString( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, value, String::EMPTY );
	FINISH_PARAMETERS;

	SFlashArg arg;
	arg.m_stringVal = value;
	arg.m_valType = SFlashArg::FlashArgType_String;
	RETURN_STRUCT( SFlashArg, arg );
}

static void funcNameToFlashUInt( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, value, CName::NONE);
	FINISH_PARAMETERS;

	RETURN_INT( static_cast< Int32 >( value.GetIndex() ) );
}

static void funcItemToFlashUInt( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SItemUniqueId, value, SItemUniqueId::INVALID );
	FINISH_PARAMETERS;

	RETURN_INT( static_cast< Int32 >( value.GetValue() ) );
}

void RegisterFlashFunctions()
{
	NATIVE_GLOBAL_FUNCTION( "FlashArgBool",		funcFlashArgBool );
	NATIVE_GLOBAL_FUNCTION( "FlashArgInt",		funcFlashArgInt );
	NATIVE_GLOBAL_FUNCTION( "FlashArgUInt",		funcFlashArgUInt );
	NATIVE_GLOBAL_FUNCTION( "FlashArgNumber",	funcFlashArgNumber );
	NATIVE_GLOBAL_FUNCTION( "FlashArgString",	funcFlashArgString );
	NATIVE_GLOBAL_FUNCTION( "NameToFlashUInt",	funcNameToFlashUInt );
	NATIVE_GLOBAL_FUNCTION( "ItemToFlashUInt",	funcItemToFlashUInt )
}
