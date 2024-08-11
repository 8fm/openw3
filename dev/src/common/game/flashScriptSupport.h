/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "../engine/flashValue.h"

//////////////////////////////////////////////////////////////////////////
// Forward declarations
//////////////////////////////////////////////////////////////////////////
class CFlashMovie;
class CFlashValueStorage;

class CScriptedFlashObject;
class CScriptedFlashSprite;
class CScriptedFlashArray;
class CScriptedFlashTextField;
class CScriptedFlashFunction;
class CScriptedFlashValueStorage;

#ifndef RED_FINAL_BUILD
#define FLASH_OBJ_TXT(txt) MACRO_TXT(txt)
#define FLASH_MBR_STR(str) (str)
#else
#define FLASH_OBJ_TXT(txt) MACRO_TXT("")
#define FLASH_MBR_STR(str) (String::EMPTY)
#endif

//////////////////////////////////////////////////////////////////////////
// CScriptedFlashArg
//////////////////////////////////////////////////////////////////////////
struct SFlashArg
{
	DECLARE_RTTI_STRUCT( SFlashArg );

	enum SFlashArgType
	{
		FlashArgType_Bool,
		FlashArgType_Int,
		FlashArgType_UInt,
		FlashArgType_Number,
		FlashArgType_String,
	};

	union
	{
		Bool		m_boolVal;
		Int32		m_intVal;
		Uint32		m_uintVal;
		Float		m_numberVal;
	} u;

	String			m_stringVal;
	SFlashArgType	m_valType;
};

//////////////////////////////////////////////////////////////////////////
// CScriptedFlashObjectPool
//////////////////////////////////////////////////////////////////////////
class CScriptedFlashObjectPool //: private Red::System::NonCopyable
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_ScriptObjectFlash );

private:
	TDynArray< IScriptedFlash* > m_poolObjects;

public:
	CScriptedFlashObjectPool();
	~CScriptedFlashObjectPool();
	
public:
	void RegisterPoolObject( IScriptedFlash* poolObject );
	void UnregisterPoolObject( IScriptedFlash* poolObject );
};

//////////////////////////////////////////////////////////////////////////
// IScriptedFlash
//////////////////////////////////////////////////////////////////////////
class IScriptedFlash : public IScriptable//, private Red::System::NonCopyable
{
	DECLARE_RTTI_SIMPLE_CLASS_WITH_ALLOCATOR( IScriptedFlash, MC_ScriptObjectFlash );
	NO_DEFAULT_CONSTRUCTOR( IScriptedFlash );

private:
	typedef IScriptable TBaseClass;

private:
	CScriptedFlashObjectPool*								m_flashObjectPool;

public:
	virtual ~IScriptedFlash();

protected:
	IScriptedFlash( CScriptedFlashObjectPool& flashObjectPool );

public:
	CScriptedFlashObjectPool& GetFlashObjectPool() { return *m_flashObjectPool; }
	const CScriptedFlashObjectPool& GetFlashObjectPool() const { return *m_flashObjectPool; }

public:
	void ClearFlashObjectPool();
};

//////////////////////////////////////////////////////////////////////////
// CScriptedFlashObject
//////////////////////////////////////////////////////////////////////////
class CScriptedFlashObject : public IScriptedFlash
{
	DECLARE_RTTI_SIMPLE_CLASS( CScriptedFlashObject );
	NO_DEFAULT_CONSTRUCTOR( CScriptedFlashObject );

	friend class CScriptedFlashArray;
	friend class CScriptedFlashValueStorage;

private:
	typedef IScriptedFlash TBaseClass;

private:
	CFlashValue	m_flashValue;

#ifndef RED_FINAL_BUILD
	String		m_name;
#endif

public:
	CScriptedFlashObject( CScriptedFlashObjectPool& pool, const CFlashValue& flashValue, const String& name = String::EMPTY );
	virtual ~CScriptedFlashObject();

	Bool GetFlashObject( const String& memberName, CFlashValue& flashValue );

protected:
	const CFlashValue& GetFlashValue() const { return m_flashValue; }
	CFlashValue& GetFlashValue() { return m_flashValue; }

private:
	void	funcCreateFlashObject( CScriptStackFrame& stack, void* result );
	void	funcCreateFlashArray( CScriptStackFrame& stack, void* result );

	void	funcGetMemberFlashObject( CScriptStackFrame& stack, void* result );
	void	funcGetMemberFlashArray( CScriptStackFrame& stack, void* result );
	void	funcGetMemberFlashFunction( CScriptStackFrame& stack, void* result );
	void	funcGetMemberFlashString( CScriptStackFrame& stack, void* result );
	void	funcGetMemberFlashBool( CScriptStackFrame& stack, void* result );
	void	funcGetMemberFlashInt( CScriptStackFrame& stack, void* result );
	void	funcGetMemberFlashUInt( CScriptStackFrame& stack, void* result );
	void	funcGetMemberFlashNumber( CScriptStackFrame& stack, void* result );

	void	funcSetMemberFlashObject( CScriptStackFrame& stack, void* result );
	void	funcSetMemberFlashArray( CScriptStackFrame& stack, void* result );
	void	funcSetMemberFlashFunction( CScriptStackFrame& stack, void* result );
	void	funcSetMemberFlashString( CScriptStackFrame& stack, void* result );
	void	funcSetMemberFlashBool( CScriptStackFrame& stack, void* result );
	void	funcSetMemberFlashInt( CScriptStackFrame& stack, void* result );
	void	funcSetMemberFlashUInt( CScriptStackFrame& stack, void* result );
	void	funcSetMemberFlashNumber( CScriptStackFrame& stack, void* result );
};

//////////////////////////////////////////////////////////////////////////
// CScriptedFlashSprite
//////////////////////////////////////////////////////////////////////////
class CScriptedFlashSprite : public CScriptedFlashObject
{
	DECLARE_RTTI_SIMPLE_CLASS( CScriptedFlashSprite );
	NO_DEFAULT_CONSTRUCTOR( CScriptedFlashSprite );

private:
	typedef CScriptedFlashObject TBaseClass;

public:
	CScriptedFlashSprite( CScriptedFlashObjectPool& pool, const CFlashValue& flashValue, const String& name = String::EMPTY );

private:
	/*void	funcIsOnStage( CScriptStackFrame& stack, void* result );*/ // Can imagine this being used in a polling loop... so not exposed.
	void	funcGetChildFlashSprite( CScriptStackFrame& stack, void* result );
	void	funcGetChildFlashTextField( CScriptStackFrame& stack, void* result );
	void	funcGotoAndPlayFrameNumber( CScriptStackFrame& stack, void* result );
	void	funcGotoAndPlayFrameLabel( CScriptStackFrame& stack, void* result );
	void	funcGotoAndStopFrameNumber( CScriptStackFrame& stack, void* result );
	void	funcGotoAndStopFrameLabel( CScriptStackFrame& stack, void* result );

private:
	void	funcGetAlpha( CScriptStackFrame& stack, void* result );
	void	funcGetRotation( CScriptStackFrame& stack, void* result );
	void	funcGetVisible( CScriptStackFrame& stack, void* result );
	void	funcGetX( CScriptStackFrame& stack, void* result );
	void	funcGetY( CScriptStackFrame& stack, void* result );
	void	funcGetZ( CScriptStackFrame& stack, void* result );
	void	funcGetXRotation( CScriptStackFrame& stack, void* result );
	void	funcGetYRotation( CScriptStackFrame& stack, void* result );
	void	funcGetXScale( CScriptStackFrame& stack, void* result );
	void	funcGetYScale( CScriptStackFrame& stack, void* result );
	void	funcGetZScale( CScriptStackFrame& stack, void* result );

	void	funcSetAlpha( CScriptStackFrame& stack, void* result );
	void	funcSetRotation( CScriptStackFrame& stack, void* result );
	void	funcSetVisible( CScriptStackFrame& stack, void* result );
	void	funcSetPosition( CScriptStackFrame& stack, void* result );
	void	funcSetScale( CScriptStackFrame& stack, void* result );
	void	funcSetX( CScriptStackFrame& stack, void* result );
	void	funcSetY( CScriptStackFrame& stack, void* result );
	void	funcSetZ( CScriptStackFrame& stack, void* result );
	void	funcSetXRotation( CScriptStackFrame& stack, void* result );
	void	funcSetYRotation( CScriptStackFrame& stack, void* result );
	void	funcSetXScale( CScriptStackFrame& stack, void* result );
	void	funcSetYScale( CScriptStackFrame& stack, void* result );
	void	funcSetZScale( CScriptStackFrame& stack, void* result );
};

//////////////////////////////////////////////////////////////////////////
// CScriptedFlashArray
//////////////////////////////////////////////////////////////////////////
class CScriptedFlashArray : public IScriptedFlash
{
	DECLARE_RTTI_SIMPLE_CLASS( CScriptedFlashArray );
	NO_DEFAULT_CONSTRUCTOR( CScriptedFlashArray );

	friend class CScriptedFlashObject;
	friend class CScriptedFlashValueStorage;

private:
	typedef IScriptedFlash TBaseClass;

private:
	CFlashValue	m_flashValue;

#ifndef RED_FINAL_BUILD
	String		m_name;
#endif

protected:
	const CFlashValue& GetFlashValue() const { return m_flashValue; }
	CFlashValue& GetFlashValue() { return m_flashValue; }

public:
	CScriptedFlashArray( CScriptedFlashObjectPool& pool, const CFlashValue& flashValue, const String& name = String::EMPTY );
	~CScriptedFlashArray();

private:
	Bool	GetArrayElement( Int32 index, CFlashValue& outFlashValue );

private:
	void	funcClearElements( CScriptStackFrame& stack, void* result );

	void	funcGetLength( CScriptStackFrame& stack, void* result );
	void	funcSetLength( CScriptStackFrame& stack, void* result );

	void	funcGetElementFlashBool( CScriptStackFrame& stack, void* result );
	void	funcGetElementFlashInt( CScriptStackFrame& stack, void* result );
	void	funcGetElementFlashUInt( CScriptStackFrame& stack, void* result );
	void	funcGetElementFlashNumber( CScriptStackFrame& stack, void* result );
	void	funcGetElementFlashString( CScriptStackFrame& stack, void* result );
	void	funcGetElementFlashObject( CScriptStackFrame& stack, void* result );
	void	funcPopBack( CScriptStackFrame& stack, void* result );

	void	funcSetElementFlashObject( CScriptStackFrame& stack, void* result );
	void	funcSetElementFlashString( CScriptStackFrame& stack, void* result );
	void	funcSetElementFlashBool( CScriptStackFrame& stack, void* result );
	void	funcSetElementFlashInt( CScriptStackFrame& stack, void* result );
	void	funcSetElementFlashUInt( CScriptStackFrame& stack, void* result );
	void	funcSetElementFlashNumber( CScriptStackFrame& stack, void* result );

	void	funcPushBackFlashObject( CScriptStackFrame& stack, void* result );
	void	funcPushBackFlashString( CScriptStackFrame& stack, void* result );
	void	funcPushBackFlashBool( CScriptStackFrame& stack, void* result );
	void	funcPushBackFlashInt( CScriptStackFrame& stack, void* result );
	void	funcPushBackFlashUInt( CScriptStackFrame& stack, void* result );
	void	funcPushBackFlashNumber( CScriptStackFrame& stack, void* result );

	void	funcRemoveElement( CScriptStackFrame& stack, void* result );
	void	funcRemoveElements( CScriptStackFrame& stack, void* result );
};

//////////////////////////////////////////////////////////////////////////
// CScriptedFlashTextField
//////////////////////////////////////////////////////////////////////////
class CScriptedFlashTextField : public IScriptedFlash
{
	DECLARE_RTTI_SIMPLE_CLASS( CScriptedFlashTextField );
	NO_DEFAULT_CONSTRUCTOR( CScriptedFlashTextField );

private:
	typedef IScriptedFlash TBaseClass;

private:
	CFlashValue	m_flashValue;

#ifndef RED_FINAL_BUILD
	String		m_name;
#endif

protected:
	const CFlashValue& GetFlashValue() const { return m_flashValue; }
	CFlashValue& GetFlashValue() { return m_flashValue; }

public:
	CScriptedFlashTextField( CScriptedFlashObjectPool& pool, const CFlashValue& flashValue, const String& name = String::EMPTY );
	virtual ~CScriptedFlashTextField();

private:
	void	funcGetText( CScriptStackFrame& stack, void* result );
	void	funcGetTextHtml( CScriptStackFrame& stack, void* result );
	void	funcSetText( CScriptStackFrame& stack, void* result );
	void	funcSetTextHtml( CScriptStackFrame& stack, void* result );
};

//////////////////////////////////////////////////////////////////////////
// CScriptedFlashFunction
//////////////////////////////////////////////////////////////////////////
class CScriptedFlashFunction : public IScriptedFlash
{
	DECLARE_RTTI_SIMPLE_CLASS( CScriptedFlashFunction );
	NO_DEFAULT_CONSTRUCTOR( CScriptedFlashFunction );

	friend class CScriptedFlashObject;

private:
	typedef IScriptedFlash TBaseClass;

private:
	CFlashValue	m_flashValue;

#ifndef RED_FINAL_BUILD
	String		m_name;
#endif

protected:
	const CFlashValue& GetFlashValue() const { return m_flashValue; }
	CFlashValue& GetFlashValue() { return m_flashValue; }

public:
	CScriptedFlashFunction( CScriptedFlashObjectPool& pool, const CFlashValue& flashValue, const String& name = String::EMPTY );
	virtual ~CScriptedFlashFunction();

private:
	CFlashValue GetFlashValueFromArg( const SFlashArg& flashArg ) const;

private:
	void funcInvokeSelf( CScriptStackFrame& stack, void* result );
	void funcInvokeSelfOneArg( CScriptStackFrame& stack, void* result );
	void funcInvokeSelfTwoArgs( CScriptStackFrame& stack, void* result );
	void funcInvokeSelfThreeArgs( CScriptStackFrame& stack, void* result );
	void funcInvokeSelfFourArgs( CScriptStackFrame& stack, void* result );
	void funcInvokeSelfFiveArgs( CScriptStackFrame& stack, void* result );
	void funcInvokeSelfSixArgs( CScriptStackFrame& stack, void* result );
	void funcInvokeSelfSevenArgs( CScriptStackFrame& stack, void* result );
	void funcInvokeSelfEightArgs( CScriptStackFrame& stack, void* result );
	void funcInvokeSelfNineArgs( CScriptStackFrame& stack, void* result );
};

//////////////////////////////////////////////////////////////////////////
// CScriptedFlashValueStorage
//////////////////////////////////////////////////////////////////////////
class CScriptedFlashValueStorage : public IScriptedFlash
{
	DECLARE_RTTI_SIMPLE_CLASS_WITH_ALLOCATOR( CScriptedFlashValueStorage, MC_ScriptObjectFlash )
	NO_DEFAULT_CONSTRUCTOR( CScriptedFlashValueStorage );

private:
	typedef IScriptedFlash TBaseClass;

private:
	CFlashValueStorage*	m_flashValueStorage;
	CFlashMovie*		m_flashValueStorageMovie;

public:
	CScriptedFlashValueStorage( CScriptedFlashObjectPool& pool, CFlashValueStorage* flashValueStorage, CFlashMovie* flashValueStorageMovie );
	virtual ~CScriptedFlashValueStorage();

private:
	void funcSetFlashObject( CScriptStackFrame& stack, void* result );
	void funcSetFlashArray( CScriptStackFrame& stack, void* result );
	void funcSetFlashString( CScriptStackFrame& stack, void* result );
	void funcSetFlashBool( CScriptStackFrame& stack, void* result );
	void funcSetFlashInt( CScriptStackFrame& stack, void* result );
	void funcSetFlashUInt( CScriptStackFrame& stack, void* result );
	void funcSetFlashNumber( CScriptStackFrame& stack, void* result );
	void funcCreateTempFlashObject( CScriptStackFrame& stack, void* result );
	void funcCreateTempFlashArray( CScriptStackFrame& stack, void* result );
};

//////////////////////////////////////////////////////////////////////////
// RTTI Boilerplate
//////////////////////////////////////////////////////////////////////////
BEGIN_CLASS_RTTI( SFlashArg );
END_CLASS_RTTI();

//BEGIN_ABSTRACT_CLASS_RTTI
BEGIN_CLASS_RTTI( IScriptedFlash );
PARENT_CLASS( IScriptable );
END_CLASS_RTTI();

BEGIN_CLASS_RTTI( CScriptedFlashObject );
PARENT_CLASS( IScriptedFlash );
	NATIVE_FUNCTION( "CreateFlashObject", funcCreateFlashObject );
	NATIVE_FUNCTION( "CreateFlashArray", funcCreateFlashArray );
	NATIVE_FUNCTION( "GetMemberFlashObject", funcGetMemberFlashObject );
	NATIVE_FUNCTION( "GetMemberFlashArray", funcGetMemberFlashArray );
	NATIVE_FUNCTION( "GetMemberFlashFunction", funcGetMemberFlashFunction ); 
	NATIVE_FUNCTION( "GetMemberFlashString", funcGetMemberFlashString );
	NATIVE_FUNCTION( "GetMemberFlashBool", funcGetMemberFlashBool );
	NATIVE_FUNCTION( "GetMemberFlashInt", funcGetMemberFlashInt );
	NATIVE_FUNCTION( "GetMemberFlashUInt", funcGetMemberFlashUInt );
	NATIVE_FUNCTION( "GetMemberFlashNumber", funcGetMemberFlashNumber );
	NATIVE_FUNCTION( "SetMemberFlashObject", funcSetMemberFlashObject );
	NATIVE_FUNCTION( "SetMemberFlashArray", funcSetMemberFlashArray );
	NATIVE_FUNCTION( "SetMemberFlashFunction", funcSetMemberFlashFunction ); 
	NATIVE_FUNCTION( "SetMemberFlashString", funcSetMemberFlashString );
	NATIVE_FUNCTION( "SetMemberFlashBool", funcSetMemberFlashBool );
	NATIVE_FUNCTION( "SetMemberFlashInt", funcSetMemberFlashInt );
	NATIVE_FUNCTION( "SetMemberFlashUInt", funcSetMemberFlashUInt );
	NATIVE_FUNCTION( "SetMemberFlashNumber", funcSetMemberFlashNumber );
END_CLASS_RTTI();

BEGIN_CLASS_RTTI( CScriptedFlashSprite );
PARENT_CLASS( CScriptedFlashObject );
	//NATIVE_FUNCTION( "IsOnStage", funcIsOnStage );
	NATIVE_FUNCTION( "GetChildFlashSprite", funcGetChildFlashSprite );
	NATIVE_FUNCTION( "GetChildFlashTextField", funcGetChildFlashTextField );
	NATIVE_FUNCTION( "GotoAndPlayFrameNumber", funcGotoAndPlayFrameNumber );
	NATIVE_FUNCTION( "GotoAndPlayFrameLabel", funcGotoAndPlayFrameLabel );
	NATIVE_FUNCTION( "GotoAndStopFrameNumber", funcGotoAndStopFrameNumber );
	NATIVE_FUNCTION( "GotoAndStopFrameLabel", funcGotoAndStopFrameLabel );
	NATIVE_FUNCTION( "GetAlpha", funcGetAlpha );
	NATIVE_FUNCTION( "GetRotation", funcGetRotation );
	NATIVE_FUNCTION( "GetVisible", funcGetVisible );
	NATIVE_FUNCTION( "GetX", funcGetX );
	NATIVE_FUNCTION( "GetY", funcGetY );
	NATIVE_FUNCTION( "GetZ", funcGetZ );
	NATIVE_FUNCTION( "GetXRotation", funcGetXRotation );
	NATIVE_FUNCTION( "GetYRotation", funcGetYRotation );
	NATIVE_FUNCTION( "GetXScale", funcGetXScale );
	NATIVE_FUNCTION( "GetYScale", funcGetYScale );
	NATIVE_FUNCTION( "GetZScale", funcGetZScale );
	NATIVE_FUNCTION( "SetAlpha", funcSetAlpha );
	NATIVE_FUNCTION( "SetRotation", funcSetRotation );
	NATIVE_FUNCTION( "SetVisible", funcSetVisible );
	NATIVE_FUNCTION( "SetPosition", funcSetPosition );
	NATIVE_FUNCTION( "SetScale", funcSetScale );
	NATIVE_FUNCTION( "SetX", funcSetX );
	NATIVE_FUNCTION( "SetY", funcSetY );
	NATIVE_FUNCTION( "SetZ", funcSetZ );
	NATIVE_FUNCTION( "SetXRotation", funcSetXRotation );
	NATIVE_FUNCTION( "SetYRotation", funcSetYRotation );
	NATIVE_FUNCTION( "SetXScale", funcSetXScale );
	NATIVE_FUNCTION( "SetYScale", funcSetYScale );
	NATIVE_FUNCTION( "SetZScale", funcSetZScale );
END_CLASS_RTTI();

BEGIN_CLASS_RTTI( CScriptedFlashArray );
PARENT_CLASS( IScriptedFlash );
	NATIVE_FUNCTION( "ClearElements", funcClearElements );
	NATIVE_FUNCTION( "GetLength", funcGetLength );
	NATIVE_FUNCTION( "SetLength", funcSetLength );
	NATIVE_FUNCTION( "GetElementFlashBool", funcGetElementFlashBool );
	NATIVE_FUNCTION( "GetElementFlashInt", funcGetElementFlashInt );
	NATIVE_FUNCTION( "GetElementFlashUInt", funcGetElementFlashUInt );
	NATIVE_FUNCTION( "GetElementFlashNumber", funcGetElementFlashNumber );
	NATIVE_FUNCTION( "GetElementFlashString", funcGetElementFlashString );
	NATIVE_FUNCTION( "GetElementFlashObject", funcGetElementFlashObject );
	NATIVE_FUNCTION( "PopBack", funcPopBack );
	NATIVE_FUNCTION( "SetElementFlashObject", funcSetElementFlashObject );
	NATIVE_FUNCTION( "SetElementFlashString", funcSetElementFlashString );
	NATIVE_FUNCTION( "SetElementFlashBool", funcSetElementFlashBool );
	NATIVE_FUNCTION( "SetElementFlashInt", funcSetElementFlashInt );
	NATIVE_FUNCTION( "SetElementFlashUInt", funcSetElementFlashUInt );
	NATIVE_FUNCTION( "SetElementFlashNumber", funcSetElementFlashNumber );
	NATIVE_FUNCTION( "PushBackFlashObject", funcPushBackFlashObject );
	NATIVE_FUNCTION( "PushBackFlashString", funcPushBackFlashString );
	NATIVE_FUNCTION( "PushBackFlashBool", funcPushBackFlashBool );
	NATIVE_FUNCTION( "PushBackFlashInt", funcPushBackFlashInt );
	NATIVE_FUNCTION( "PushBackFlashUInt", funcPushBackFlashUInt );
	NATIVE_FUNCTION( "PushBackFlashNumber", funcPushBackFlashNumber );
	NATIVE_FUNCTION( "RemoveElement", funcRemoveElement );
	NATIVE_FUNCTION( "RemoveElements", funcRemoveElements );
END_CLASS_RTTI();

BEGIN_CLASS_RTTI( CScriptedFlashTextField );
PARENT_CLASS( IScriptedFlash );
	NATIVE_FUNCTION( "GetText", funcGetText );
	NATIVE_FUNCTION( "GetTextHtml", funcGetTextHtml );
	NATIVE_FUNCTION( "SetText", funcSetText );
	NATIVE_FUNCTION( "SetTextHtml", funcSetTextHtml );
END_CLASS_RTTI();

BEGIN_CLASS_RTTI( CScriptedFlashFunction );
PARENT_CLASS( IScriptedFlash );
	NATIVE_FUNCTION( "InvokeSelf", funcInvokeSelf );
	NATIVE_FUNCTION( "InvokeSelfOneArg", funcInvokeSelfOneArg );
	NATIVE_FUNCTION( "InvokeSelfTwoArgs", funcInvokeSelfTwoArgs );
	NATIVE_FUNCTION( "InvokeSelfThreeArgs", funcInvokeSelfThreeArgs );
	NATIVE_FUNCTION( "InvokeSelfFourArgs", funcInvokeSelfFourArgs );
	NATIVE_FUNCTION( "InvokeSelfFiveArgs", funcInvokeSelfFiveArgs );
	NATIVE_FUNCTION( "InvokeSelfSixArgs", funcInvokeSelfSixArgs );
	NATIVE_FUNCTION( "InvokeSelfSevenArgs", funcInvokeSelfSevenArgs );
	NATIVE_FUNCTION( "InvokeSelfEightArgs", funcInvokeSelfEightArgs );
	NATIVE_FUNCTION( "InvokeSelfNineArgs", funcInvokeSelfNineArgs );
END_CLASS_RTTI();

BEGIN_CLASS_RTTI( CScriptedFlashValueStorage );
PARENT_CLASS( IScriptedFlash );
	NATIVE_FUNCTION( "SetFlashObject", funcSetFlashObject );
	NATIVE_FUNCTION( "SetFlashArray", funcSetFlashArray);
	NATIVE_FUNCTION( "SetFlashString", funcSetFlashString );
	NATIVE_FUNCTION( "SetFlashBool", funcSetFlashBool );
	NATIVE_FUNCTION( "SetFlashInt", funcSetFlashInt );
	NATIVE_FUNCTION( "SetFlashUInt", funcSetFlashUInt ); 
	NATIVE_FUNCTION( "SetFlashNumber", funcSetFlashNumber );

	// IScriptable temp! Just replace with creating from hud/menu flash from now on!
	NATIVE_FUNCTION( "CreateTempFlashArray", funcCreateTempFlashArray );
	NATIVE_FUNCTION( "CreateTempFlashObject", funcCreateTempFlashObject );
END_CLASS_RTTI();
