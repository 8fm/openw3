/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"

#include "flashValue.h"

#ifdef USE_NULL_FLASHVALUE

#include "flashFunction.h"

//////////////////////////////////////////////////////////////////////////
// CFlashValue
//////////////////////////////////////////////////////////////////////////
CFlashValue::CFlashValue()
{
}

CFlashValue::CFlashValue( const CFlashValue& other )
{
	RED_UNUSED( other );
}

CFlashValue::CFlashValue( Bool value )
{
	RED_UNUSED( value );
}

CFlashValue::CFlashValue( Int32 value )
{
	RED_UNUSED( value );
}

CFlashValue::CFlashValue( Uint32 value )
{
	RED_UNUSED( value );
}

CFlashValue::CFlashValue( Double value )
{
	RED_UNUSED( value );
}

EFlashValueType CFlashValue::GetFlashValueType() const
{
	return FVT_Undefined;
}

void CFlashValue::Clear()
{
}

void CFlashValue::SetUserData( void* data )
{
	RED_UNUSED( data );
}

void* CFlashValue::GetUserData() const
{
	return nullptr;
}

Bool CFlashValue::IsInstanceOf( const String& className ) const
{
	RED_UNUSED( className );
	return true;
}

Bool CFlashValue::IsFlashArray() const
{
	return false;
}

Bool CFlashValue::IsFlashBool() const
{
	return false;
}

Bool CFlashValue::IsFlashClosure() const
{
	return false;
}

Bool CFlashValue::IsFlashDisplayObject() const
{
	return false;
}

Bool CFlashValue::IsFlashDisplayObjectOnStage() const
{
	return false;
}

Bool CFlashValue::IsFlashInt() const
{
	return false;
}

Bool CFlashValue::IsFlashNull() const
{
	return false;
}

Bool CFlashValue::IsFlashNumber() const
{
	return false;
}

Bool CFlashValue::IsFlashNumeric() const
{
	return false;
}

Bool CFlashValue::IsFlashObject() const
{
	return false;
}

Bool CFlashValue::IsFlashOrphaned() const
{
	return false;
}

Bool CFlashValue::IsFlashString() const
{
	return false;
}

Bool CFlashValue::IsFlashUndefined() const
{
	return false;
}

Bool CFlashValue::IsFlashUInt() const
{
	return false;
}

Bool CFlashValue::GetFlashBool() const
{
	return false;
}

Int32 CFlashValue::GetFlashInt() const
{
	return 0;
}

Uint32 CFlashValue::GetFlashUInt() const
{
	return 0;
}

Double CFlashValue::GetFlashNumber() const
{
	return 0.;
}

String CFlashValue::GetFlashString() const
{
	return String::EMPTY;
}

void CFlashValue::SetFlashNull()
{
}

void CFlashValue::SetFlashBool( Bool value )
{
	RED_UNUSED( value );
}

void CFlashValue::SetFlashInt( Int32 value )
{
	RED_UNUSED( value );
}

void CFlashValue::SetFlashUInt( Uint32 value )
{
	RED_UNUSED( value );
}

void CFlashValue::SetFlashNumber( Double value )
{
	RED_UNUSED( value );
}

Bool CFlashValue::CreateObject( const String& className, CFlashValue& /*[out]*/ outObject ) const
{
	RED_UNUSED( className );
	RED_UNUSED( outObject );
	return false;
}

Bool CFlashValue::CreateArray( CFlashValue& /*[out]*/ outArray ) const
{
	RED_UNUSED( outArray );
	return false;
}

Bool CFlashValue::CreateString( const String& value, CFlashValue& /*[out]*/ outString ) const
{
	RED_UNUSED( value );
	RED_UNUSED( outString );
	return false;
}

Bool CFlashValue::GetMember( const String& memberName, CFlashValue& /*[out]*/ outValue ) const
{
	RED_UNUSED( memberName );
	RED_UNUSED( outValue );
	outValue.Clear();
	return false;
}

Bool CFlashValue::SetMemberFlashValue( const String& memberName, const CFlashValue& value )
{
	RED_UNUSED( memberName );
	RED_UNUSED( value );
	return false;
}

Bool CFlashValue::SetMemberFlashNull( const String& memberName )
{
	RED_UNUSED( memberName );
	return false;
}

Bool CFlashValue::SetMemberFlashFunction( const String& memberName, const CFlashFunction& value )
{
	RED_UNUSED( memberName );
	RED_UNUSED( value );
	return false;
}

Bool CFlashValue::SetMemberFlashString( const String& memberName, const String& value )
{
	RED_UNUSED( memberName );
	RED_UNUSED( value );
	return false;
}

Bool CFlashValue::SetMemberFlashBool( const String& memberName, Bool value )
{
	RED_UNUSED( memberName );
	RED_UNUSED( value );
	return false;
}

Bool CFlashValue::SetMemberFlashInt( const String& memberName, Int32 value )
{
	RED_UNUSED( memberName );
	RED_UNUSED( value );
	return false;
}

Bool CFlashValue::SetMemberFlashUInt( const String& memberName, Uint32 value )
{
	RED_UNUSED( memberName );
	RED_UNUSED( value );
	return false;
}

Bool CFlashValue::SetMemberFlashNumber( const String& memberName, Double value )
{
	RED_UNUSED( memberName );
	RED_UNUSED( value );
	return false;
}

Bool CFlashValue::ClearArrayElements()
{
	return false;
}

Uint32 CFlashValue::GetArrayLength() const
{
	return 0;
}

Bool CFlashValue::SetArrayLength( Uint32 length )
{
	RED_UNUSED( length );
	return false;
}

Bool CFlashValue::GetArrayElement( Uint32 index, CFlashValue& /*[out]*/ outValue ) const
{
	RED_UNUSED( index );
	RED_UNUSED( outValue );
	return false;
}

Bool CFlashValue::PopArrayBack( CFlashValue& /*[out]*/ outValue )
{
	RED_UNUSED( outValue );
	return false;
}

Bool CFlashValue::SetArrayElementFlashValue( Uint32 index, const CFlashValue& value )
{
	RED_UNUSED( index );
	RED_UNUSED( value );
	return false;
}

Bool CFlashValue::SetArrayElementFlashNull( Uint32 index )
{
	RED_UNUSED( index );
	return false;
}

Bool CFlashValue::SetArrayElementFlashString( Uint32 index, const String& value )
{
	RED_UNUSED( index );
	RED_UNUSED( value );
	return false;
}

Bool CFlashValue::PushArrayBackFlashNull()
{
	return false;
}

Bool CFlashValue::SetArrayElementFlashBool( Uint32 index, Bool value )
{
	RED_UNUSED( index );
	RED_UNUSED( value );
	return false;
}

Bool CFlashValue::SetArrayElementFlashInt( Uint32 index, Int32 value )
{
	RED_UNUSED( index );
	RED_UNUSED( value );
	return false;
}

Bool CFlashValue::SetArrayElementFlashUInt( Uint32 index, Uint32 value )
{
	RED_UNUSED( index );
	RED_UNUSED( value );
	return false;
}

Bool CFlashValue::SetArrayElementFlashNumber( Uint32 index, Double value )
{
	RED_UNUSED( index );
	RED_UNUSED( value );
	return false;
}

Bool CFlashValue::PushArrayBackFlashString( const String& value )
{
	RED_UNUSED( value );
	return false;
}

Bool CFlashValue::PushArrayBackFlashBool( Bool value )
{
	RED_UNUSED( value );
	return false;
}

Bool CFlashValue::PushArrayBackFlashInt( Int32 value )
{
	RED_UNUSED( value );
	return false;
}

Bool CFlashValue::PushArrayBackFlashUInt( Uint32 value )
{
	RED_UNUSED( value );
	return false;
}

Bool CFlashValue::PushArrayBackFlashNumber( Double value )
{
	RED_UNUSED( value );
	return false;
}

Bool CFlashValue::PushArrayBackFlashValue( const CFlashValue& value )
{
	RED_UNUSED( value );
	return false;
}

Bool CFlashValue::RemoveArrayElement( Uint32 index )
{
	RED_UNUSED( index );
	return false;
}

Bool CFlashValue::RemoveArrayElements( Uint32 index, Int32 count /*= -1 */ )
{
	RED_UNUSED( index );
	RED_UNUSED( count );
	return false;
}

Bool CFlashValue::GotoAndPlay( Uint32 frame )
{
	RED_UNUSED( frame );
	return false;
}

Bool CFlashValue::GotoAndPlay( const String& frame )
{
	RED_UNUSED( frame );
	return false;
}

Bool CFlashValue::GotoAndStop( Uint32 frame )
{
	RED_UNUSED( frame );
	return false;
}

Bool CFlashValue::GotoAndStop( const String& frame )
{
	RED_UNUSED( frame );
	return false;
}

Bool CFlashValue::GetFlashDisplayInfo( CFlashDisplayInfo& displayInfo )
{
	RED_UNUSED( displayInfo );
	return false;
}

Bool CFlashValue::SetFlashDisplayInfo( const CFlashDisplayInfo& displayInfo )
{
	RED_UNUSED( displayInfo );
	return false;
}

String CFlashValue::GetText() const
{
	return String::EMPTY;
}

String CFlashValue::GetTextHtml() const
{
	return String::EMPTY;
}

Bool CFlashValue::SetText( const String& text )
{
	RED_UNUSED( text );
	return false;
}

Bool CFlashValue::SetTextHtml( const String& htmlText )
{
	RED_UNUSED( htmlText );
	return false;
}

Bool CFlashValue::InvokeSelf( CFlashValue* outResult /*= nullptr*/ )
{
	RED_UNUSED( outResult );
	return false;
}

Bool CFlashValue::InvokeSelf( const CFlashValue& arg0, CFlashValue* outResult /*= nullptr*/ )
{
	RED_UNUSED( arg0 );
	RED_UNUSED( outResult );
	return false;
}

Bool CFlashValue::InvokeSelf( const CFlashValue& arg0, const CFlashValue& arg1, CFlashValue* outResult /*= nullptr*/ )
{
	RED_UNUSED( arg0 );
	RED_UNUSED( arg1 );
	RED_UNUSED( outResult );
	return false;
}

Bool CFlashValue::InvokeSelf( const CFlashValue& arg0, const CFlashValue& arg1, const CFlashValue& arg2, CFlashValue* outResult /*= nullptr*/ )
{
	RED_UNUSED( arg0 );
	RED_UNUSED( arg1 );
	RED_UNUSED( arg2 );
	RED_UNUSED( outResult );
	return false;
}

Bool CFlashValue::InvokeSelf( const CFlashValue& arg0, const CFlashValue& arg1, const CFlashValue& arg2, const CFlashValue& arg3, CFlashValue* outResult /*= nullptr*/ )
{
	RED_UNUSED( arg0 );
	RED_UNUSED( arg1 );
	RED_UNUSED( arg2 );
	RED_UNUSED( arg3 );
	RED_UNUSED( outResult );
	return false;
}

Bool CFlashValue::InvokeSelf( const CFlashValue& arg0, const CFlashValue& arg1, const CFlashValue& arg2, const CFlashValue& arg3, const CFlashValue& arg4, CFlashValue* outResult /*= nullptr*/ )
{
	RED_UNUSED( arg0 );
	RED_UNUSED( arg1 );
	RED_UNUSED( arg2 );
	RED_UNUSED( arg3 );
	RED_UNUSED( arg4 );
	RED_UNUSED( outResult );
	return false;
}

Bool CFlashValue::InvokeSelf( const CFlashValue& arg0, const CFlashValue& arg1, const CFlashValue& arg2, const CFlashValue& arg3, const CFlashValue& arg4, const CFlashValue& arg5, CFlashValue* outResult /*= nullptr*/ )
{
	RED_UNUSED( arg0 );
	RED_UNUSED( arg1 );
	RED_UNUSED( arg2 );
	RED_UNUSED( arg3 );
	RED_UNUSED( arg4 );
	RED_UNUSED( arg5 );
	RED_UNUSED( outResult );
	return false;
}

Bool CFlashValue::InvokeSelf( const CFlashValue& arg0, const CFlashValue& arg1, const CFlashValue& arg2, const CFlashValue& arg3, const CFlashValue& arg4, const CFlashValue& arg5, const CFlashValue& arg6, CFlashValue* outResult /*= nullptr*/ )
{
	RED_UNUSED( arg0 );
	RED_UNUSED( arg1 );
	RED_UNUSED( arg2 );
	RED_UNUSED( arg3 );
	RED_UNUSED( arg4 );
	RED_UNUSED( arg5 );
	RED_UNUSED( arg6 );
	RED_UNUSED( outResult );
	return false;
}

Bool CFlashValue::InvokeSelf( const CFlashValue& arg0, const CFlashValue& arg1, const CFlashValue& arg2, const CFlashValue& arg3, const CFlashValue& arg4, const CFlashValue& arg5, const CFlashValue& arg6, const CFlashValue& arg7, CFlashValue* outResult /*= nullptr*/ )
{
	RED_UNUSED( arg0 );
	RED_UNUSED( arg1 );
	RED_UNUSED( arg2 );
	RED_UNUSED( arg3 );
	RED_UNUSED( arg4 );
	RED_UNUSED( arg5 );
	RED_UNUSED( arg6 );
	RED_UNUSED( arg7 );
	RED_UNUSED( outResult );
	return false;
}

Bool CFlashValue::InvokeSelf( const CFlashValue& arg0, const CFlashValue& arg1, const CFlashValue& arg2, const CFlashValue& arg3, const CFlashValue& arg4, const CFlashValue& arg5, const CFlashValue& arg6, const CFlashValue& arg7, const CFlashValue& arg8, CFlashValue* outResult /*= nullptr*/ )
{
	RED_UNUSED( arg0 );
	RED_UNUSED( arg1 );
	RED_UNUSED( arg2 );
	RED_UNUSED( arg3 );
	RED_UNUSED( arg4 );
	RED_UNUSED( arg5 );
	RED_UNUSED( arg6 );
	RED_UNUSED( arg7 );
	RED_UNUSED( arg8 );
	RED_UNUSED( outResult );
	return false;
}

//////////////////////////////////////////////////////////////////////////
// CFlashDisplayInfo
//////////////////////////////////////////////////////////////////////////
void CFlashDisplayInfo::Clear()
{
}

Double CFlashDisplayInfo::GetAlpha() const
{
	return 1.;
}

Double CFlashDisplayInfo::GetRotation() const
{
	return 0.;
}

Bool CFlashDisplayInfo::GetVisible() const
{
	return false;
}

Double CFlashDisplayInfo::GetX() const
{
	return 0.;
}

Double CFlashDisplayInfo::GetY() const
{
	return 0.;
}

Double CFlashDisplayInfo::GetZ() const
{
	return 0.;
}

Double CFlashDisplayInfo::GetXRotation() const
{
	return 0.;
}

Double CFlashDisplayInfo::GetYRotation() const
{
	return 0.;
}

Double CFlashDisplayInfo::GetXScale() const
{
	return 1.;
}

Double CFlashDisplayInfo::GetYScale() const
{
	return 1.;
}

Double CFlashDisplayInfo::GetZScale() const
{
	return 1.;
}

void CFlashDisplayInfo::SetAlpha( Double alpha )
{
	RED_UNUSED( alpha );
}

void CFlashDisplayInfo::SetPosition( Double x, Double y )
{
	RED_UNUSED( x );
	RED_UNUSED( y );
}

void CFlashDisplayInfo::SetRotation( Double degrees )
{
	RED_UNUSED( degrees );
}

void CFlashDisplayInfo::SetScale( Double xscale, Double yscale )
{
	RED_UNUSED( xscale );
	RED_UNUSED( yscale );
}

void CFlashDisplayInfo::SetVisible( Bool visible )
{
	RED_UNUSED( visible );
}

void CFlashDisplayInfo::SetX( Double x )
{
	RED_UNUSED( x );
}

void CFlashDisplayInfo::SetY( Double y )
{
	RED_UNUSED( y );
}

void CFlashDisplayInfo::SetZ( Double z )
{
	RED_UNUSED( z );
}

void CFlashDisplayInfo::SetXRotation( Double degrees )
{
	RED_UNUSED( degrees );
}

void CFlashDisplayInfo::SetYRotation( Double degrees )
{
	RED_UNUSED( degrees );
}

void CFlashDisplayInfo::SetXScale( Double xscale )
{
	RED_UNUSED( xscale );
}

void CFlashDisplayInfo::SetYScale( Double yscale )
{
	RED_UNUSED( yscale );
}

void CFlashDisplayInfo::SetZScale( Double zscale )
{
	RED_UNUSED( zscale );
}

#endif // USE_NULL_FLASHVALUE