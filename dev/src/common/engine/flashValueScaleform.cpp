/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"
#include "flashValue.h"

#ifdef USE_SCALEFORM

#include "flashFunctionScaleform.h"
#include "guiGlobals.h"

//////////////////////////////////////////////////////////////////////////
// CFlashValue
//////////////////////////////////////////////////////////////////////////
EFlashValueType CFlashValue::GetFlashValueType() const
{
	EFlashValueType type = FVT_Undefined;

	switch( m_gfxValue.GetType() )
	{
	case GFx::Value::VT_Undefined:
		type = FVT_Undefined;
		break;
	case GFx::Value::VT_Null:
		type = FVT_Null;
		break;
	case GFx::Value::VT_Boolean:
		type = FVT_Bool;
		break;
	case GFx::Value::VT_Int:
		type = FVT_Int;
		break;
	case GFx::Value::VT_UInt:
		type = FVT_UInt;
		break;
	case GFx::Value::VT_Number:
		type = FVT_Number;
		break;
	case GFx::Value::VT_String:
	case GFx::Value::VT_StringW:
		type = FVT_String;
		break;
	case GFx::Value::VT_Object:
		type = FVT_Object;
		break;
	case GFx::Value::VT_Array:
		type = FVT_Array;
		break;
	case GFx::Value::VT_DisplayObject:
		type = FVT_DisplayObject;
		break;
	case GFx::Value::VT_Closure:
		type = FVT_Closure;
		break;
	default:
		break;
	}

	return type;
}

namespace // anonymous
{
	struct SScaleformUserDataWrapper : public GFx::ASUserData
	{
		void*			m_data;

		virtual void	OnDestroy( GFx::Movie* pmovie, void* pobject) override
		{
			m_data = nullptr;
			
			// Safe to release here as SF won't reference this anymore.
			Release();
		}
	};
}

void CFlashValue::SetUserData( void* data )
{
	ASSERT( m_gfxValue.IsObject() );
	if ( m_gfxValue.IsObject() )
	{
		SScaleformUserDataWrapper* userDataWrapper = static_cast< SScaleformUserDataWrapper* >( m_gfxValue.GetUserData() );
		if ( ! userDataWrapper )
		{
			userDataWrapper = SF_NEW SScaleformUserDataWrapper;
			m_gfxValue.SetUserData( userDataWrapper );
		}
		ASSERT( userDataWrapper );
		userDataWrapper->m_data = data;
	}
}

void* CFlashValue::GetUserData() const
{
	void* data = nullptr;

	ASSERT( m_gfxValue.IsObject() );
	if ( m_gfxValue.IsObject() )
	{
		SScaleformUserDataWrapper* userDataWrapper = static_cast< SScaleformUserDataWrapper* >( m_gfxValue.GetUserData() );
		if ( userDataWrapper )
		{
			data = userDataWrapper->m_data;
		}
	}

	return data;
}

Bool CFlashValue::CreateObject( const String& className, CFlashValue& /*[out]*/ outObject ) const
{
	ASSERT( m_gfxValue.IsObject() );
	ASSERT( m_gfxValue.GetMovie() );

	outObject.Clear();

	if ( m_gfxValue.IsObject() && m_gfxValue.GetMovie() )
	{
		m_gfxValue.GetMovie()->CreateObject( &outObject.m_gfxValue, FLASH_TXT_TO_UTF8(className.AsChar()) );
		if ( outObject.m_gfxValue.IsObject() )
		{
			return true;
		}
	}
	GUI_ERROR( TXT("CFlashValue::CreateObject - failed to create object '%ls'"), className.AsChar() );
	return false;
}

Bool CFlashValue::CreateArray( CFlashValue& /*[out]*/ outArray ) const
{
	ASSERT( m_gfxValue.IsObject() );
	ASSERT( m_gfxValue.GetMovie() );

	outArray.Clear();

	if ( m_gfxValue.IsObject() && m_gfxValue.GetMovie() )
	{
		m_gfxValue.GetMovie()->CreateArray( &outArray.m_gfxValue );
		if ( outArray.m_gfxValue.IsArray() )
		{
			return true;
		}
	}
	GUI_ERROR( TXT("CFlashValue::CreateArray - failed to create array") );
	return false;
}

Bool CFlashValue::CreateString( const String& value, CFlashValue& /*[out]*/ outString ) const
{
	ASSERT( m_gfxValue.IsObject() || m_gfxValue.IsClosure() );
	ASSERT( m_gfxValue.GetMovie() );

	outString.Clear();

	if ( ( m_gfxValue.IsObject() || m_gfxValue.IsClosure() ) && m_gfxValue.GetMovie() )
	{
		m_gfxValue.GetMovie()->CreateString( &outString.m_gfxValue, FLASH_TXT_TO_UTF8(value.AsChar()) );
		if ( outString.m_gfxValue.IsString() )
		{
			return true;
		}
	}
	GUI_ERROR( TXT("CFlashValue::CreateString - failed to create string '%ls'"), value.AsChar() );
	return false;
}

Bool CFlashValue::GetMember( const String& memberName, CFlashValue& /*[out]*/ outValue ) const
{
	ASSERT( m_gfxValue.IsObject() );
	outValue.Clear();
	if ( m_gfxValue.IsObject() )
	{
		if ( m_gfxValue.GetMember( FLASH_TXT_TO_UTF8(memberName.AsChar()), &outValue.m_gfxValue ) )
		{
			return true;
		}
	}
	GUI_ERROR( TXT("CFlashValue::GetMember - failed to get member '%ls'"), memberName.AsChar() );
	return false;
}

Bool CFlashValue::SetMemberFlashValue( const String& memberName, const CFlashValue& value )
{
	ASSERT( m_gfxValue.IsObject() );
	ASSERT( ! value.m_gfxValue.GetMovie() || m_gfxValue.GetMovie() == value.m_gfxValue.GetMovie() );

	if ( m_gfxValue.IsObject() && ( ! value.m_gfxValue.GetMovie() || m_gfxValue.GetMovie() == value.m_gfxValue.GetMovie() ) )
	{
		if ( m_gfxValue.SetMember( FLASH_TXT_TO_UTF8(memberName.AsChar()), value.m_gfxValue ) )
		{
			return true;
		}
	}
	GUI_ERROR( TXT("CFlashValue::SetMember - failed to set member '%ls'"), memberName.AsChar() );
	return false;
}

Bool CFlashValue::SetMemberFlashNull( const String& memberName )
{
	ASSERT( m_gfxValue.IsObject() );

	if ( m_gfxValue.IsObject() )
	{
		const GFx::Value nullGFxValue( GFx::Value::VT_Null );
		if ( m_gfxValue.SetMember( FLASH_TXT_TO_UTF8(memberName.AsChar()), nullGFxValue ) )
		{
			return true;
		}
	}
	GUI_ERROR( TXT("CFlashValue::SetMemberFlashNull - failed to set member '%ls'"), memberName.AsChar() );
	return false;
}

Bool CFlashValue::SetMemberFlashFunction( const String& memberName, const CFlashFunction& value )
{
	ASSERT( m_gfxValue.IsObject() );
	const CFlashFunctionScaleform& scaleformValue = static_cast< const CFlashFunctionScaleform& >( value );

	ASSERT( scaleformValue.m_gfxFunc.GetMovie() &&  m_gfxValue.GetMovie() == scaleformValue.m_gfxFunc.GetMovie() );

	if ( m_gfxValue.IsObject() && ( scaleformValue.m_gfxFunc.GetMovie() &&  m_gfxValue.GetMovie() == scaleformValue.m_gfxFunc.GetMovie() ) )
	{
		GFx::Value tmpGFxValue;
		if ( m_gfxValue.SetMember( FLASH_TXT_TO_UTF8(memberName.AsChar()), scaleformValue.m_gfxFunc) &&
			m_gfxValue.GetMember( FLASH_TXT_TO_UTF8(memberName.AsChar()), &tmpGFxValue ) )
		{	
			return tmpGFxValue.IsObject();
		}
	}
	GUI_ERROR( TXT("CFlashValue::SetMember - failed to set member '%ls'"), memberName.AsChar() );
	return false;
}

Bool CFlashValue::SetMemberFlashString( const String& memberName, const String& value )
{
	ASSERT( m_gfxValue.IsObject() );
	ASSERT( m_gfxValue.GetMovie() );

	if ( m_gfxValue.IsObject() && m_gfxValue.GetMovie() )
	{
		GFx::Value tmpGFxValue;
		m_gfxValue.GetMovie()->CreateString( &tmpGFxValue, FLASH_TXT_TO_UTF8(value.AsChar()) );
		ASSERT( tmpGFxValue.IsString() );
		if ( m_gfxValue.SetMember( FLASH_TXT_TO_UTF8(memberName.AsChar()), tmpGFxValue ) )
		{
			return true;
		}
	}
	GUI_ERROR( TXT("CFlashValue::SetMember - failed to set string '%ls' on member '%ls'"), value.AsChar(), memberName.AsChar() );
	return false;
}

Bool CFlashValue::SetMemberFlashBool( const String& memberName, Bool value )
{
	ASSERT( m_gfxValue.IsObject() );

	if ( m_gfxValue.IsObject() )
	{
		const GFx::Value tmpGFxValue( value );
		ASSERT( tmpGFxValue.IsBool() );
		if ( m_gfxValue.SetMember( FLASH_TXT_TO_UTF8(memberName.AsChar()), tmpGFxValue ) )
		{
			return true;
		}
	}
	GUI_ERROR( TXT("CFlashValue::SetMember - failed to set Boolean '%ls' on member '%ls'"), value ? TXT("true") : TXT("false"), memberName.AsChar() );
	return false;
}

Bool CFlashValue::SetMemberFlashInt( const String& memberName, Int32 value )
{
	ASSERT( m_gfxValue.IsObject() );

	if ( m_gfxValue.IsObject() )
	{
		const GFx::Value tmpGFxValue( static_cast< SF::SInt32 >( value ) );
		ASSERT( tmpGFxValue.IsInt() );
		if ( m_gfxValue.SetMember( FLASH_TXT_TO_UTF8(memberName.AsChar()), tmpGFxValue ) )
		{
			return true;
		}
	}
	GUI_ERROR( TXT("CFlashValue::SetMember - failed to set int '%d' on member '%ls'"), value, memberName.AsChar() );
	return false;
}

Bool CFlashValue::SetMemberFlashUInt( const String& memberName, Uint32 value )
{
	ASSERT( m_gfxValue.IsObject() );

	if ( m_gfxValue.IsObject() )
	{
		const GFx::Value tmpGFxValue( static_cast< SF::UInt32 >( value ) );
		ASSERT( tmpGFxValue.IsUInt() );
		if ( m_gfxValue.SetMember( FLASH_TXT_TO_UTF8(memberName.AsChar()), tmpGFxValue ) )
		{
			return true;
		}
	}
	GUI_ERROR( TXT("CFlashValue::SetMember - failed to set uint '%u' on member '%ls'"), value, memberName.AsChar() );
	return false;
}

Bool CFlashValue::SetMemberFlashNumber( const String& memberName, Double value )
{
	ASSERT( m_gfxValue.IsObject() );

	if ( m_gfxValue.IsObject() )
	{
		const GFx::Value tmpGFxValue( static_cast< SF::Double >( value ) );
		ASSERT( tmpGFxValue.IsNumber() );
		if ( m_gfxValue.SetMember( FLASH_TXT_TO_UTF8(memberName.AsChar()), tmpGFxValue ) )
		{
			return true;
		}
	}
	GUI_ERROR( TXT("CFlashValue::SetMember - failed to set Number '%f' on member '%ls'"), value, memberName.AsChar() );
	return false;
}

Bool CFlashValue::ClearArrayElements()
{
	ASSERT( m_gfxValue.IsArray() );
	if ( m_gfxValue.IsArray() )
	{
		if ( m_gfxValue.ClearElements() )
		{
			return true;
		}
	}
	GUI_ERROR( TXT("CFlashValue::ClearArrayElements - failed to clear elements") );
	return false;
}

Uint32 CFlashValue::GetArrayLength() const
{
	ASSERT( m_gfxValue.IsArray() );
	if ( m_gfxValue.IsArray() )
	{
		return m_gfxValue.GetArraySize();
	}
	GUI_ERROR( TXT("CFlashValue::GetArrayLength - failed to get array length") );
	return 0;
}

Bool CFlashValue::SetArrayLength( Uint32 length )
{
	ASSERT( m_gfxValue.IsArray() );
	if ( m_gfxValue.IsArray() )
	{
		if ( m_gfxValue.SetArraySize( length ) )
		{
			return true;
		}
	}
	GUI_ERROR( TXT("CFlashValue::SetArrayLength - failed to set array length") );
	return false;
}

Bool CFlashValue::GetArrayElement( Uint32 index, CFlashValue& /*[out]*/ outValue ) const
{
	ASSERT( m_gfxValue.IsArray() );

	outValue.Clear();
	if ( m_gfxValue.IsArray() )
	{
		if ( m_gfxValue.GetElement( index, &outValue.m_gfxValue ) )
		{
			return true;
		}
	}
	GUI_ERROR( TXT("CFlashValue::GetArrayElement - failed to get array element at index '%u'"), index );
	return false;
}

Bool CFlashValue::PopArrayBack( CFlashValue& /*[out]*/ outValue )
{
	ASSERT( m_gfxValue.IsArray() );

	outValue.Clear();
	if ( m_gfxValue.IsArray() )
	{
		if ( m_gfxValue.PopBack( &outValue.m_gfxValue ) )
		{
			return true;
		}
	}
	GUI_ERROR( TXT("CFlashValue::PopArrayBack - failed to pop value") );
	return false;
}

Bool CFlashValue::SetArrayElementFlashValue( Uint32 index, const CFlashValue& value )
{
	ASSERT( m_gfxValue.IsArray() );
	ASSERT( ! value.m_gfxValue.GetMovie() || m_gfxValue.GetMovie() == value.m_gfxValue.GetMovie() );

	if ( m_gfxValue.IsArray() && ( ! value.m_gfxValue.GetMovie() || m_gfxValue.GetMovie() == value.m_gfxValue.GetMovie() ) )
	{
		if ( m_gfxValue.SetElement( index, value.m_gfxValue ) )
		{
			return true;
		}
	}
	GUI_ERROR( TXT("CFlashValue::SetArrayElement - failed to set flash value at index '%u'"), index );
	return false;
}

Bool CFlashValue::SetArrayElementFlashNull( Uint32 index )
{
	ASSERT( m_gfxValue.IsArray() );

	if ( m_gfxValue.IsArray() )
	{
		const GFx::Value nullGFxValue( GFx::Value::VT_Null );
		if ( m_gfxValue.SetElement( index, nullGFxValue ) )
		{
			return true;
		}
	}
	GUI_ERROR( TXT("CFlashValue::SetArrayElementFlashNull - failed to set null at index '%u'"), index );
	return false;
}

Bool CFlashValue::SetArrayElementFlashString( Uint32 index, const String& value )
{
	ASSERT( m_gfxValue.IsArray() );
	ASSERT( m_gfxValue.GetMovie() );

	if ( m_gfxValue.IsArray() && m_gfxValue.GetMovie() )
	{
		GFx::Value tmpGFxValue;
		m_gfxValue.GetMovie()->CreateString( &tmpGFxValue, FLASH_TXT_TO_UTF8(value.AsChar()) );
		ASSERT( tmpGFxValue.IsString() );
		if ( m_gfxValue.SetElement( index, tmpGFxValue) )
		{
			return true;
		}
	}
	GUI_ERROR( TXT("CFlashValue::SetArrayElement - failed to set string '%ls' at index '%u'"), value.AsChar(), index );
	return false;
}

Bool CFlashValue::SetArrayElementFlashBool( Uint32 index, Bool value )
{
	ASSERT( m_gfxValue.IsArray() );

	if ( m_gfxValue.IsArray() )
	{
		const GFx::Value tmpGFxValue( value );
		ASSERT( tmpGFxValue.IsBool() );
		if ( m_gfxValue.SetElement( index, tmpGFxValue) )
		{
			return true;
		}
	}
	GUI_ERROR( TXT("CFlashValue::SetArrayElement - failed to set Boolean '%ls' at index '%u'"), value ? TXT("true") : TXT("false"), index );
	return false;
}

Bool CFlashValue::SetArrayElementFlashInt( Uint32 index, Int32 value )
{
	ASSERT( m_gfxValue.IsArray() );

	if ( m_gfxValue.IsArray() )
	{
		const GFx::Value tmpGFxValue( static_cast< SF::SInt32 >( value ) );
		ASSERT( tmpGFxValue.IsInt() );
		if ( m_gfxValue.SetElement( index, tmpGFxValue) )
		{
			return true;
		}
	}
	GUI_ERROR( TXT("CFlashValue::SetArrayElement - failed to set int '%d' at index '%u'"), value, index );
	return false;
}

Bool CFlashValue::SetArrayElementFlashUInt( Uint32 index, Uint32 value )
{
	ASSERT( m_gfxValue.IsArray() );

	if ( m_gfxValue.IsArray() )
	{
		const GFx::Value tmpGFxValue( static_cast< SF::UInt32>( value ) );
		ASSERT( tmpGFxValue.IsUInt() );
		if ( m_gfxValue.SetElement( index, tmpGFxValue) )
		{
			return true;
		}
	}
	GUI_ERROR( TXT("CFlashValue::SetArrayElement - failed to set uint '%u' at index '%u'"), value, index );
	return false;
}

Bool CFlashValue::SetArrayElementFlashNumber( Uint32 index, Double value )
{
	ASSERT( m_gfxValue.IsArray() );

	if ( m_gfxValue.IsArray() )
	{
		const GFx::Value tmpGFxValue( static_cast< SF::Double>( value ) );
		ASSERT( tmpGFxValue.IsNumber() );
		if ( m_gfxValue.SetElement( index, tmpGFxValue) )
		{
			return true;
		}
	}
	GUI_ERROR( TXT("CFlashValue::SetArrayElement - failed to set Number '%f' at index '%u'"), value, index );
	return false;
}

Bool CFlashValue::PushArrayBackFlashValue( const CFlashValue& value )
{
	ASSERT( m_gfxValue.IsArray() );
	ASSERT( ! value.m_gfxValue.GetMovie() || m_gfxValue.GetMovie() == value.m_gfxValue.GetMovie() );

	if ( m_gfxValue.IsArray() && ( ! value.m_gfxValue.GetMovie() || m_gfxValue.GetMovie() == value.m_gfxValue.GetMovie() ) )
	{
		if ( m_gfxValue.PushBack( value.m_gfxValue ) )
		{
			return true;
		}
	}
	GUI_ERROR( TXT("CFlashValue::PushArrayBack - failed to push back flash value") );
	return false;
}

Bool CFlashValue::PushArrayBackFlashNull()
{
	ASSERT( m_gfxValue.IsArray() );

	if ( m_gfxValue.IsArray() )
	{
		const GFx::Value nullGFxValue( GFx::Value::VT_Null );
		if ( m_gfxValue.PushBack( nullGFxValue ) )
		{
			return true;
		}
	}
	GUI_ERROR( TXT("CFlashValue::PushArrayBack - failed to push back null") );
	return false;
}

Bool CFlashValue::PushArrayBackFlashString( const String& value )
{
	ASSERT( m_gfxValue.IsArray() );
	ASSERT( m_gfxValue.GetMovie() );

	if ( m_gfxValue.IsArray() && m_gfxValue.GetMovie() )
	{
		GFx::Value tmpGFxValue;
		m_gfxValue.GetMovie()->CreateString( &tmpGFxValue, FLASH_TXT_TO_UTF8(value.AsChar()) );
		ASSERT( tmpGFxValue.IsString() );
		if ( m_gfxValue.PushBack( tmpGFxValue) )
		{
			return true;
		}
	}
	GUI_ERROR( TXT("CFlashValue::PushArrayBack - failed to push back string '%ls'"), value.AsChar() );
	return false;
}

Bool CFlashValue::PushArrayBackFlashBool( Bool value )
{
	ASSERT( m_gfxValue.IsArray() );

	if ( m_gfxValue.IsArray() )
	{
		const GFx::Value tmpGFxValue( value );
		ASSERT( tmpGFxValue.IsBool() );
		if ( m_gfxValue.PushBack( tmpGFxValue ) )
		{
			return true;
		}
	}
	GUI_ERROR( TXT("CFlashValue::PushArrayBack - failed to push back Boolean '%ls'"), value ? TXT("true") : TXT("false") );
	return false;
}

Bool CFlashValue::PushArrayBackFlashInt( Int32 value )
{
	ASSERT( m_gfxValue.IsArray() );

	if ( m_gfxValue.IsArray() )
	{
		const GFx::Value tmpGFxValue( static_cast< SF::SInt32 >( value ) );
		ASSERT( tmpGFxValue.IsInt() );
		if ( m_gfxValue.PushBack( tmpGFxValue ) )
		{
			return true;
		}
	}
	GUI_ERROR( TXT("CFlashValue::PushArrayBack - failed to push back int '%d'"), value );
	return false;
}

Bool CFlashValue::PushArrayBackFlashUInt( Uint32 value )
{
	ASSERT( m_gfxValue.IsArray() );

	if ( m_gfxValue.IsArray() )
	{
		const GFx::Value tmpGFxValue( static_cast< SF::UInt32 >( value ) );
		ASSERT( tmpGFxValue.IsUInt() );
		if ( m_gfxValue.PushBack( tmpGFxValue ) )
		{
			return true;
		}
	}
	GUI_ERROR( TXT("CFlashValue::PushArrayBack - failed to push back uint '%u'"), value );
	return false;
}

Bool CFlashValue::PushArrayBackFlashNumber( Double value )
{
	ASSERT( m_gfxValue.IsArray() );

	if ( m_gfxValue.IsArray() )
	{
		const GFx::Value tmpGFxValue( static_cast< SF::Double >( value ) );
		ASSERT( tmpGFxValue.IsNumber() );
		if ( m_gfxValue.PushBack( tmpGFxValue ) )
		{
			return true;
		}
	}
	GUI_ERROR( TXT("CFlashValue::PushArrayBack - failed to push back Number '%f'"), value );
	return false;
}

Bool CFlashValue::RemoveArrayElement( Uint32 index )
{
	ASSERT( m_gfxValue.IsArray() );

	if ( m_gfxValue.IsArray() )
	{
		if ( m_gfxValue.RemoveElement( index ) )
		{
			return true;
		}
	}
	GUI_ERROR( TXT("CFlashValue::RemoveArrayElement - failed to remove element at index '%u'"), index );
	return false;
}

Bool CFlashValue::RemoveArrayElements( Uint32 index, Int32 count /*= -1 */ )
{
	ASSERT( m_gfxValue.IsArray() );

	if ( m_gfxValue.IsArray() )
	{
		if ( m_gfxValue.RemoveElements( index, count ) )
		{
			return true;
		}
	}
	GUI_ERROR( TXT("CFlashValue::RemoveArrayElements - failed to remove elements from index '%u' with count '%d'"), index, count );
	return false;
}

Bool CFlashValue::GotoAndPlay( Uint32 frame )
{
	ASSERT( m_gfxValue.IsDisplayObject() );

	if ( m_gfxValue.IsDisplayObject() )
	{
		if ( m_gfxValue.GotoAndPlay( frame ) )
		{
			return true;
		}
	}
	GUI_ERROR( TXT("CFlashValue::GotoAndPlay - failed to go to frame '%u'"), frame );
	return false;
}

Bool CFlashValue::GotoAndPlay( const String& frame )
{
	ASSERT( m_gfxValue.IsDisplayObject() );

	if ( m_gfxValue.IsDisplayObject() )
	{
		if ( m_gfxValue.GotoAndPlay( FLASH_TXT_TO_UTF8(frame.AsChar()) ) )
		{
			return true;
		}
	}
	GUI_ERROR( TXT("CFlashValue::GotoAndPlay - failed to go to frame '%ls'"), frame.AsChar() );
	return false;
}

Bool CFlashValue::GotoAndStop( Uint32 frame )
{
	ASSERT( m_gfxValue.IsDisplayObject() );

	if ( m_gfxValue.IsDisplayObject() )
	{
		if ( m_gfxValue.GotoAndStop( frame ) )
		{
			return true;
		}
	}
	GUI_ERROR( TXT("CFlashValue::GotoAndStop - failed to go to frame '%u'"), frame );
	return false;
}

Bool CFlashValue::GotoAndStop( const String& frame )
{
	ASSERT( m_gfxValue.IsDisplayObject() );

	if ( m_gfxValue.IsDisplayObject() )
	{
		if ( m_gfxValue.GotoAndStop( FLASH_TXT_TO_UTF8(frame.AsChar()) ) )
		{
			return true;
		}
	}
	GUI_ERROR( TXT("CFlashValue::GotoAndStop - failed to go to frame '%ls'"), frame.AsChar() );
	return false;
}

Bool CFlashValue::GetFlashDisplayInfo( CFlashDisplayInfo& displayInfo )
{
	ASSERT( m_gfxValue.IsDisplayObject() );
	if ( m_gfxValue.IsDisplayObject() )
	{
		if ( m_gfxValue.GetDisplayInfo( &displayInfo.m_gfxDisplayInfo ) )
		{
			return true;
		}
	}
	GUI_ERROR( TXT("CFlashValue::GetFlashDisplayInfo - failed to get display info") );
	return false;
}

Bool CFlashValue::SetFlashDisplayInfo( const CFlashDisplayInfo& displayInfo )
{
	ASSERT( m_gfxValue.IsDisplayObject() );
	if ( m_gfxValue.IsDisplayObject() )
	{
		if ( m_gfxValue.SetDisplayInfo( displayInfo.m_gfxDisplayInfo ) )
		{
			return true;
		}
	}
	GUI_ERROR( TXT("CFlashValue::SetDisplayInfo - failed to set display info") );
	return false;
}

String CFlashValue::GetText() const
{
	ASSERT( m_gfxValue.IsDisplayObject() );

	if ( m_gfxValue.IsDisplayObject() )
	{
		GFx::Value tmpGFxValue;
		if ( m_gfxValue.GetText( &tmpGFxValue ) )
		{
			ASSERT( tmpGFxValue.IsString() );
			return FLASH_UTF8_TO_TXT( tmpGFxValue.GetString() );
		}
	}
	GUI_ERROR( TXT("CFlashValue::GetText - failed to retrieve text") );
	return String::EMPTY;
}

String CFlashValue::GetTextHtml() const
{
	ASSERT( m_gfxValue.IsDisplayObject() );

	if ( m_gfxValue.IsDisplayObject() )
	{
		GFx::Value tmpGFxValue;
		if ( m_gfxValue.GetTextHTML( &tmpGFxValue ) )
		{
			ASSERT( tmpGFxValue.IsString() );
			return FLASH_UTF8_TO_TXT( tmpGFxValue.GetString() );
		}
	}
	GUI_ERROR( TXT("CFlashValue::GetTextHtml - failed to retrieve text") );
	return String::EMPTY;
}

Bool CFlashValue::SetText( const String& text )
{
	if ( m_gfxValue.IsDisplayObject() && m_gfxValue.IsDisplayObjectActive() )
	{
		if ( m_gfxValue.SetText( FLASH_TXT_TO_UTF8(text.AsChar()) ) )
		{
			return true;
		}
	}
	GUI_ERROR( TXT("CFlashValue::SetText - failed to set text '%ls'"), text.AsChar() );
	return false;
}

Bool CFlashValue::SetTextHtml( const String& htmlText )
{
	if ( m_gfxValue.IsDisplayObject() && m_gfxValue.IsDisplayObjectActive() )
	{
		if ( m_gfxValue.SetTextHTML( FLASH_TXT_TO_UTF8(htmlText.AsChar()) ) )
		{
			return true;
		}
	}
	GUI_ERROR( TXT("CFlashValue::SetTextHtml - failed to set text '%ls'"), htmlText.AsChar() );
	return false;
}

Bool CFlashValue::InvokeSelf( CFlashValue* outResult /*= nullptr*/ )
{
	ASSERT( m_gfxValue.IsClosure() );

	if ( m_gfxValue.IsClosure() )
	{
		if ( m_gfxValue.InvokeSelf( outResult ? &outResult->m_gfxValue : nullptr ) )
		{
			return true;
		}
	}

	GUI_ERROR( TXT("CFlashValue::InvokeSelf - failed to invoke function") );
	return false;
}

Bool CFlashValue::InvokeSelf( const CFlashValue& arg0, CFlashValue* outResult /*= nullptr*/ )
{
	ASSERT( m_gfxValue.IsClosure() );

	if ( m_gfxValue.IsClosure() )
	{
		const SF::UPInt numArgs = 1;
		if ( m_gfxValue.InvokeSelf( outResult ? &outResult->m_gfxValue : nullptr, &arg0.m_gfxValue, numArgs ) )
		{
			return true;
		}
	}

	GUI_ERROR( TXT("CFlashValue::InvokeSelf - failed to invoke function") );
	return false;
}

Bool CFlashValue::InvokeSelf( const CFlashValue& arg0, const CFlashValue& arg1, CFlashValue* outResult /*= nullptr*/ )
{
	ASSERT( m_gfxValue.IsClosure() );

	if ( m_gfxValue.IsClosure() )
	{
		const GFx::Value args[] = { arg0.m_gfxValue, arg1.m_gfxValue };
		if ( m_gfxValue.InvokeSelf( outResult ? &outResult->m_gfxValue : nullptr, args, sizeof(args)/sizeof(args[0]) ) )
		{
			return true;
		}
	}

	GUI_ERROR( TXT("CFlashValue::InvokeSelf - failed to invoke function") );
	return false;
}

Bool CFlashValue::InvokeSelf( const CFlashValue& arg0, const CFlashValue& arg1, const CFlashValue& arg2, CFlashValue* outResult /*= nullptr*/ )
{
	ASSERT( m_gfxValue.IsClosure() );

	if ( m_gfxValue.IsClosure() )
	{
		const GFx::Value args[] = { arg0.m_gfxValue, arg1.m_gfxValue, arg2.m_gfxValue };
		if ( m_gfxValue.InvokeSelf( outResult ? &outResult->m_gfxValue : nullptr, args, sizeof(args)/sizeof(args[0]) ) )
		{
			return true;
		}
	}

	GUI_ERROR( TXT("CFlashValue::InvokeSelf - failed to invoke function") );
	return false;
}

Bool CFlashValue::InvokeSelf( const CFlashValue& arg0, const CFlashValue& arg1, const CFlashValue& arg2, const CFlashValue& arg3, CFlashValue* outResult /*= nullptr*/ )
{
	ASSERT( m_gfxValue.IsClosure() );

	if ( m_gfxValue.IsClosure() )
	{
		const GFx::Value args[] = { arg0.m_gfxValue, arg1.m_gfxValue, arg2.m_gfxValue, arg3.m_gfxValue };
		if ( m_gfxValue.InvokeSelf( outResult ? &outResult->m_gfxValue : nullptr, args, sizeof(args)/sizeof(args[0]) ) )
		{
			return true;
		}
	}

	GUI_ERROR( TXT("CFlashValue::InvokeSelf - failed to invoke function") );
	return false;
}

Bool CFlashValue::InvokeSelf( const CFlashValue& arg0, const CFlashValue& arg1, const CFlashValue& arg2, const CFlashValue& arg3, const CFlashValue& arg4,  CFlashValue* outResult /*= nullptr*/ )
{
	ASSERT( m_gfxValue.IsClosure() );

	if ( m_gfxValue.IsClosure() )
	{
		const GFx::Value args[] = { arg0.m_gfxValue, arg1.m_gfxValue, arg2.m_gfxValue, arg3.m_gfxValue, arg4.m_gfxValue };
		if ( m_gfxValue.InvokeSelf( outResult ? &outResult->m_gfxValue : nullptr, args, sizeof(args)/sizeof(args[0]) ) )
		{
			return true;
		}
	}

	GUI_ERROR( TXT("CFlashValue::InvokeSelf - failed to invoke function") );
	return false;
}

Bool CFlashValue::InvokeSelf( const CFlashValue& arg0, const CFlashValue& arg1, const CFlashValue& arg2, const CFlashValue& arg3, const CFlashValue& arg4, const CFlashValue& arg5,  CFlashValue* outResult /*= nullptr*/ )
{
	ASSERT( m_gfxValue.IsClosure() );

	if ( m_gfxValue.IsClosure() )
	{
		const GFx::Value args[] = { arg0.m_gfxValue, arg1.m_gfxValue, arg2.m_gfxValue, arg3.m_gfxValue, arg4.m_gfxValue, arg5.m_gfxValue };
		if ( m_gfxValue.InvokeSelf( outResult ? &outResult->m_gfxValue : nullptr, args, sizeof(args)/sizeof(args[0]) ) )
		{
			return true;
		}
	}

	GUI_ERROR( TXT("CFlashValue::InvokeSelf - failed to invoke function") );
	return false;
}

Bool CFlashValue::InvokeSelf( const CFlashValue& arg0, const CFlashValue& arg1, const CFlashValue& arg2, const CFlashValue& arg3, const CFlashValue& arg4, const CFlashValue& arg5, const CFlashValue& arg6,  CFlashValue* outResult /*= nullptr*/ )
{
	ASSERT( m_gfxValue.IsClosure() );

	if ( m_gfxValue.IsClosure() )
	{
		const GFx::Value args[] = { arg0.m_gfxValue, arg1.m_gfxValue, arg2.m_gfxValue, arg3.m_gfxValue, arg4.m_gfxValue, arg5.m_gfxValue, arg6.m_gfxValue };
		if ( m_gfxValue.InvokeSelf( outResult ? &outResult->m_gfxValue : nullptr, args, sizeof(args)/sizeof(args[0]) ) )
		{
			return true;
		}
	}

	GUI_ERROR( TXT("CFlashValue::InvokeSelf - failed to invoke function") );
	return false;
}

Bool CFlashValue::InvokeSelf( const CFlashValue& arg0, const CFlashValue& arg1, const CFlashValue& arg2, const CFlashValue& arg3, const CFlashValue& arg4, const CFlashValue& arg5, const CFlashValue& arg6, const CFlashValue& arg7,  CFlashValue* outResult /*= nullptr*/ )
{
	ASSERT( m_gfxValue.IsClosure() );

	if ( m_gfxValue.IsClosure() )
	{
		const GFx::Value args[] = { arg0.m_gfxValue, arg1.m_gfxValue, arg2.m_gfxValue, arg3.m_gfxValue, arg4.m_gfxValue, arg5.m_gfxValue, arg6.m_gfxValue, arg7.m_gfxValue };
		if ( m_gfxValue.InvokeSelf( outResult ? &outResult->m_gfxValue : nullptr, args, sizeof(args)/sizeof(args[0]) ) )
		{
			return true;
		}
	}

	GUI_ERROR( TXT("CFlashValue::InvokeSelf - failed to invoke function") );
	return false;
}

Bool CFlashValue::InvokeSelf( const CFlashValue& arg0, const CFlashValue& arg1, const CFlashValue& arg2, const CFlashValue& arg3, const CFlashValue& arg4, const CFlashValue& arg5, const CFlashValue& arg6, const CFlashValue& arg7, const CFlashValue& arg8,  CFlashValue* outResult /*= nullptr*/ )
{
	ASSERT( m_gfxValue.IsClosure() );

	if ( m_gfxValue.IsClosure() )
	{
		const GFx::Value args[] = { arg0.m_gfxValue, arg1.m_gfxValue, arg2.m_gfxValue, arg3.m_gfxValue, arg4.m_gfxValue, arg5.m_gfxValue, arg6.m_gfxValue, arg7.m_gfxValue, arg8.m_gfxValue };
		if ( m_gfxValue.InvokeSelf( outResult ? &outResult->m_gfxValue : nullptr, args, sizeof(args)/sizeof(args[0]) ) )
		{
			return true;
		}
	}

	GUI_ERROR( TXT("CFlashValue::InvokeSelf - failed to invoke function") );
	return false;
}

#endif // USE_SCALEFORM