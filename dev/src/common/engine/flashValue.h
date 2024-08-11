/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once

class CFlashFunction;

#ifndef USE_SCALEFORM
#	define USE_NULL_FLASHVALUE
#endif

class CFlashDisplayInfo;

enum EFlashValueType
{
	FVT_Undefined,
	FVT_Null,
	FVT_Bool,
	FVT_Int,
	FVT_UInt,
	FVT_Number,
	FVT_String,
	FVT_Object,
	FVT_Array,
	FVT_DisplayObject,
	FVT_Closure,
};

//////////////////////////////////////////////////////////////////////////
// CFlashValue
//////////////////////////////////////////////////////////////////////////
class CFlashValue
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_GUI );

private:
#ifdef USE_SCALEFORM
	friend class CFlashStringScaleform;
	friend class CFlashObjectScaleform;
	friend class CFlashArrayScaleform;
	GFx::Value				m_gfxValue;
#endif

public:
#ifdef USE_SCALEFORM
	CFlashValue( const GFx::Value& gfxValue );

	const GFx::Value& AsGFxValue() const { return m_gfxValue; }
#endif

public:
	void	Clear();

public:
	void	SetUserData( void* data );
	void*	GetUserData() const;

public:
	EFlashValueType	GetFlashValueType() const;

public:
	Bool	IsInstanceOf( const String& className ) const;

public:
	Bool	IsFlashArray() const;
	Bool	IsFlashBool() const;
	Bool	IsFlashClosure() const;
	Bool	IsFlashDisplayObject() const;
	Bool	IsFlashDisplayObjectOnStage() const;
	Bool	IsFlashInt() const;
	Bool	IsFlashNull() const;
	Bool	IsFlashNumber() const;
	Bool	IsFlashNumeric() const;
	Bool	IsFlashObject() const;
	Bool	IsFlashOrphaned() const;
	Bool	IsFlashString() const;
	Bool	IsFlashUndefined() const;
	Bool	IsFlashUInt() const;

public:
	Bool	GetFlashBool() const;
	Int32	GetFlashInt() const;
	Uint32	GetFlashUInt() const;
	Double	GetFlashNumber() const;
	String	GetFlashString() const;

	void	SetFlashNull();
	void	SetFlashBool( Bool value );
	void	SetFlashInt( Int32 value );
	void	SetFlashUInt( Uint32 value );
	void	SetFlashNumber( Double value );

public:
	//!< Object
	Bool	CreateObject( const String& className, CFlashValue& /*[out]*/ outObject ) const;
	Bool	CreateArray( CFlashValue& /*[out]*/ outArray ) const;
	Bool	CreateString( const String& value, CFlashValue& /*[out]*/ outString ) const;

	Bool	GetMember( const String& memberName, CFlashValue& /*[out]*/ outValue ) const;

	Bool	SetMemberFlashValue( const String& memberName, const CFlashValue& value );
	Bool	SetMemberFlashNull( const String& memberName );
	Bool	SetMemberFlashFunction( const String& memberName, const CFlashFunction& value );
	Bool	SetMemberFlashString( const String& memberName, const String& value );
	Bool	SetMemberFlashBool( const String& memberName, Bool value );
	Bool	SetMemberFlashInt( const String& memberName, Int32 value );
	Bool	SetMemberFlashUInt( const String& memberName, Uint32 value );
	Bool	SetMemberFlashNumber( const String& memberName, Double value );

public:
	//!< Array
	Bool	ClearArrayElements();
	
	Uint32	GetArrayLength() const;
	Bool	SetArrayLength( Uint32 length );
	
	Bool	GetArrayElement( Uint32 index, CFlashValue& /*[out]*/ outValue ) const;
	Bool	PopArrayBack( CFlashValue& /*[out]*/ outValue );
	
	Bool	SetArrayElementFlashValue( Uint32 index, const CFlashValue& value );
	Bool	SetArrayElementFlashNull( Uint32 index );
	Bool	SetArrayElementFlashString( Uint32 index, const String& value );
	Bool	SetArrayElementFlashBool( Uint32 index, Bool value );
	Bool	SetArrayElementFlashInt( Uint32 index, Int32 value );
	Bool	SetArrayElementFlashUInt( Uint32 index, Uint32 value );
	Bool	SetArrayElementFlashNumber( Uint32 index, Double value );

	Bool	PushArrayBackFlashValue( const CFlashValue& value );
	Bool	PushArrayBackFlashNull();
	Bool	PushArrayBackFlashString( const String& value );
	Bool	PushArrayBackFlashBool( Bool value );
	Bool	PushArrayBackFlashInt( Int32 value );
	Bool	PushArrayBackFlashUInt( Uint32 value );
	Bool	PushArrayBackFlashNumber( Double value );

	Bool	RemoveArrayElement( Uint32 index );
	Bool	RemoveArrayElements( Uint32 index, Int32 count = -1 );

public:
	//!< DisplayObject
	//Bool	GetColorTransform()
	Bool	GetFlashDisplayInfo( CFlashDisplayInfo& displayInfo );
	Bool	SetFlashDisplayInfo( const CFlashDisplayInfo& displayInfo );

	//Bool GetDisplayMatrix
	//Bool GetWorldMatrix

public:
	//!< MovieClip
	Bool	GotoAndPlay( Uint32 frame );
	Bool	GotoAndPlay( const String& frame );
	Bool	GotoAndStop( Uint32 frame );
	Bool	GotoAndStop( const String& frame );

public:
	//!< Text field
	String	GetText() const;
	String	GetTextHtml() const;
	Bool	SetText( const String& text );
	Bool	SetTextHtml( const String& htmlText );

public:
	//!< Function
	Bool	InvokeSelf( CFlashValue* outResult = nullptr );
	Bool	InvokeSelf( const CFlashValue& arg0, CFlashValue* outResult = nullptr );
	Bool	InvokeSelf( const CFlashValue& arg0, const CFlashValue& arg1, CFlashValue* outResult = nullptr );
	Bool	InvokeSelf( const CFlashValue& arg0, const CFlashValue& arg1, const CFlashValue& arg2, CFlashValue* outResult = nullptr );
	Bool	InvokeSelf( const CFlashValue& arg0, const CFlashValue& arg1, const CFlashValue& arg2, const CFlashValue& arg3, CFlashValue* outResult = nullptr );
	Bool	InvokeSelf( const CFlashValue& arg0, const CFlashValue& arg1, const CFlashValue& arg2, const CFlashValue& arg3, const CFlashValue& arg4, CFlashValue* outResult = nullptr );
	Bool	InvokeSelf( const CFlashValue& arg0, const CFlashValue& arg1, const CFlashValue& arg2, const CFlashValue& arg3, const CFlashValue& arg4, const CFlashValue& arg5, CFlashValue* outResult = nullptr );
	Bool	InvokeSelf( const CFlashValue& arg0, const CFlashValue& arg1, const CFlashValue& arg2, const CFlashValue& arg3, const CFlashValue& arg4, const CFlashValue& arg5, const CFlashValue& arg6, CFlashValue* outResult = nullptr );
	Bool	InvokeSelf( const CFlashValue& arg0, const CFlashValue& arg1, const CFlashValue& arg2, const CFlashValue& arg3, const CFlashValue& arg4, const CFlashValue& arg5, const CFlashValue& arg6, const CFlashValue& arg7, CFlashValue* outResult = nullptr );
	Bool	InvokeSelf( const CFlashValue& arg0, const CFlashValue& arg1, const CFlashValue& arg2, const CFlashValue& arg3, const CFlashValue& arg4, const CFlashValue& arg5, const CFlashValue& arg6, const CFlashValue& arg7, const CFlashValue& arg8, CFlashValue* outResult = nullptr );

public:
			CFlashValue();
			CFlashValue( const CFlashValue& other );
			CFlashValue( Bool value );
			CFlashValue( Int32 value );
			CFlashValue( Uint32 value );
			CFlashValue( Double value );

	
	RED_INLINE Bool	operator==( const CFlashValue& rhs ) const
	{
#ifdef USE_SCALEFORM
		return m_gfxValue == rhs.m_gfxValue;
#else
		return false;
#endif
	}

	RED_INLINE Bool	operator!=( const CFlashValue& rhs ) const
	{
#ifdef USE_SCALEFORM
		return !( m_gfxValue == rhs.m_gfxValue );
#else
		return false;
#endif
	}
	
	RED_INLINE const CFlashValue&	operator=( const CFlashValue& other )
	{
#ifdef USE_SCALEFORM
		m_gfxValue = other.m_gfxValue;
		return *this;
#else
		return *this;
#endif
	}
};

//////////////////////////////////////////////////////////////////////////
// CFlashDisplayInfo
//////////////////////////////////////////////////////////////////////////
class CFlashDisplayInfo
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_GUI );

private:
#ifdef USE_SCALEFORM
	friend class CFlashValue;
	GFx::Value::DisplayInfo		m_gfxDisplayInfo;
#endif

public:
	void	Clear();
	Double	GetAlpha() const;
	Double	GetRotation() const;
	Bool	GetVisible() const;
	Double	GetX() const;
	Double	GetY() const;
	Double	GetZ() const;
	Double	GetXRotation() const;
	Double	GetYRotation() const;
	Double	GetXScale() const;
	Double	GetYScale() const;
	Double	GetZScale() const;

	void	SetAlpha( Double alpha );
	void	SetPosition( Double x, Double y );
	void	SetRotation( Double degrees );
	void	SetScale( Double xscale, Double yscale );
	void	SetVisible( Bool visible );
	void	SetX( Double x );
	void	SetY( Double y );
	void	SetZ( Double z );
	void	SetXRotation( Double degrees );
	void	SetYRotation( Double degrees );
	void	SetXScale( Double xscale );
	void	SetYScale( Double yscale );
	void	SetZScale( Double zscale );
};

//////////////////////////////////////////////////////////////////////////
// Inlines
//////////////////////////////////////////////////////////////////////////
#ifdef USE_SCALEFORM
#	include "flashValueScaleform.inl"
#endif