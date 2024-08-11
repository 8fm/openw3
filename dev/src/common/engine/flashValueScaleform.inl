/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#ifdef USE_SCALEFORM

//////////////////////////////////////////////////////////////////////////
// CFlashValue
//////////////////////////////////////////////////////////////////////////
RED_INLINE CFlashValue::CFlashValue()
{
}

RED_INLINE CFlashValue::CFlashValue( const GFx::Value& gfxValue )
	: m_gfxValue( gfxValue )
{
}

RED_INLINE CFlashValue::CFlashValue( const CFlashValue& other )
	: m_gfxValue( other.m_gfxValue )
{
}

RED_INLINE CFlashValue::CFlashValue( Bool value )
	: m_gfxValue( value )
{
}

RED_INLINE CFlashValue::CFlashValue( Int32 value )
	: m_gfxValue( static_cast< SF::SInt32 >( value ) )
{
}

RED_INLINE CFlashValue::CFlashValue( Uint32 value )
	: m_gfxValue( static_cast< SF::UInt32 >( value ) )
{
}

RED_INLINE CFlashValue::CFlashValue( Double value )
	: m_gfxValue( static_cast< SF::Double >( value ) )
{
}

RED_INLINE Bool CFlashValue::IsInstanceOf( const String& className ) const
{
	return m_gfxValue.IsInstanceOf( FLASH_TXT_TO_UTF8(className.AsChar()));
}

RED_INLINE void CFlashValue::Clear()
{
	m_gfxValue.SetUndefined();
}

RED_INLINE Bool CFlashValue::IsFlashArray() const
{
	return m_gfxValue.IsArray();
}

RED_INLINE Bool CFlashValue::IsFlashBool() const
{
	return m_gfxValue.IsBool();
}

RED_INLINE Bool CFlashValue::IsFlashClosure() const
{
	return m_gfxValue.IsClosure();
}

RED_INLINE Bool CFlashValue::IsFlashDisplayObject() const
{
	return m_gfxValue.IsDisplayObject();
}

RED_INLINE Bool CFlashValue::IsFlashDisplayObjectOnStage() const
{
	return m_gfxValue.IsDisplayObjectActive();
}

RED_INLINE Bool CFlashValue::IsFlashInt() const
{
	return m_gfxValue.IsInt();
}

RED_INLINE Bool CFlashValue::IsFlashNull() const
{
	return m_gfxValue.IsNull();
}

RED_INLINE Bool CFlashValue::IsFlashNumber() const
{
	return m_gfxValue.IsNumber();
}

RED_INLINE Bool CFlashValue::IsFlashNumeric() const
{
	return m_gfxValue.IsNumeric();
}

RED_INLINE Bool CFlashValue::IsFlashOrphaned() const
{
	return m_gfxValue.IsOrphaned();
}

RED_INLINE Bool CFlashValue::IsFlashObject() const
{
	return m_gfxValue.IsObject();
}

RED_INLINE Bool CFlashValue::IsFlashString() const
{
	return m_gfxValue.IsString();
}

RED_INLINE Bool CFlashValue::IsFlashUndefined() const
{
	return m_gfxValue.IsUndefined();
}

RED_INLINE Bool CFlashValue::IsFlashUInt() const
{
	return m_gfxValue.IsUInt();
}

RED_INLINE Bool CFlashValue::GetFlashBool() const
{
	ASSERT( m_gfxValue.IsBool() );
	if ( m_gfxValue.IsBool() )
	{
		return m_gfxValue.GetBool();
	}
	return false;
}

RED_INLINE Int32 CFlashValue::GetFlashInt() const
{
	ASSERT( m_gfxValue.IsInt() );
	if ( m_gfxValue.IsInt() )
	{
		return static_cast< Int32 >( m_gfxValue.GetInt() );
	}
	return 0;
}

RED_INLINE Uint32 CFlashValue::GetFlashUInt() const
{
	ASSERT( m_gfxValue.IsUInt() );
	if ( m_gfxValue.IsUInt() )
	{
		return static_cast< Uint32 >( m_gfxValue.GetUInt() );
	}
	return 0;
}

RED_INLINE Double CFlashValue::GetFlashNumber() const
{
	ASSERT( m_gfxValue.IsNumber() );
	if ( m_gfxValue.IsNumber() )
	{
		return static_cast< Double >( m_gfxValue.GetNumber() );
	}
	return 0.;
}

RED_INLINE String CFlashValue::GetFlashString() const
{
	ASSERT( m_gfxValue.IsString() );
	if ( m_gfxValue.IsString() )
	{
		return FLASH_UTF8_TO_TXT( m_gfxValue.GetString() );
	}
	return String::EMPTY;
}

RED_INLINE void CFlashValue::SetFlashNull()
{
	m_gfxValue.SetNull();
}

RED_INLINE void CFlashValue::SetFlashBool( Bool value )
{
	m_gfxValue.SetBoolean( value );
}

RED_INLINE void CFlashValue::SetFlashInt( Int32 value )
{
	m_gfxValue.SetInt( static_cast< SF::SInt32 >( value ) );
}

RED_INLINE void CFlashValue::SetFlashUInt( Uint32 value )
{
	m_gfxValue.SetUInt( static_cast< SF::UInt32 >( value ) );
}

RED_INLINE void CFlashValue::SetFlashNumber( Double value )
{
	m_gfxValue.SetNumber( static_cast< SF::Double >( value ) );
}

//////////////////////////////////////////////////////////////////////////
// CFlashDisplayInfo
//////////////////////////////////////////////////////////////////////////
RED_INLINE void CFlashDisplayInfo::Clear()
{
	m_gfxDisplayInfo.Clear();
}

RED_INLINE Double CFlashDisplayInfo::GetAlpha() const
{
	return m_gfxDisplayInfo.GetAlpha();
}

RED_INLINE Double CFlashDisplayInfo::GetRotation() const
{
	return static_cast< Double >( m_gfxDisplayInfo.GetRotation() );
}

RED_INLINE Bool CFlashDisplayInfo::GetVisible() const
{
	return m_gfxDisplayInfo.GetVisible();
}

RED_INLINE Double CFlashDisplayInfo::GetX() const
{
	return static_cast< Double >( m_gfxDisplayInfo.GetX() );
}

RED_INLINE Double CFlashDisplayInfo::GetY() const
{
	return static_cast< Double >( m_gfxDisplayInfo.GetY() );
}

RED_INLINE Double CFlashDisplayInfo::GetZ() const
{
	return static_cast< Double >( m_gfxDisplayInfo.GetZ() );
}

RED_INLINE Double CFlashDisplayInfo::GetXRotation() const
{
	return static_cast< Double >( m_gfxDisplayInfo.GetXRotation() );
}

RED_INLINE Double CFlashDisplayInfo::GetYRotation() const
{
	return static_cast< Double >( m_gfxDisplayInfo.GetYRotation() );
}

RED_INLINE Double CFlashDisplayInfo::GetXScale() const
{
	return static_cast< Double >( m_gfxDisplayInfo.GetXScale() );
}

RED_INLINE Double CFlashDisplayInfo::GetYScale() const
{
	return static_cast< Double >( m_gfxDisplayInfo.GetYScale() );
}

RED_INLINE Double CFlashDisplayInfo::GetZScale() const
{
	return static_cast< Double >( m_gfxDisplayInfo.GetZScale() );
}

RED_INLINE void CFlashDisplayInfo::SetAlpha( Double alpha )
{
	m_gfxDisplayInfo.SetAlpha( static_cast< SF::Double >( alpha ) );
}

RED_INLINE void CFlashDisplayInfo::SetPosition( Double x, Double y )
{
	m_gfxDisplayInfo.SetPosition( static_cast< SF::Double >( x ), static_cast< SF::Double >( y ) );
}

RED_INLINE void CFlashDisplayInfo::SetRotation( Double degrees )
{
	m_gfxDisplayInfo.SetRotation( static_cast< SF::Double >( degrees ) );
}

RED_INLINE void CFlashDisplayInfo::SetScale( Double xscale, Double yscale )
{
	m_gfxDisplayInfo.SetScale( static_cast< SF::Double >( xscale ), static_cast< SF::Double >( yscale ) );
}

RED_INLINE void CFlashDisplayInfo::SetVisible( Bool visible )
{
	m_gfxDisplayInfo.SetVisible( visible );
}

RED_INLINE void CFlashDisplayInfo::SetX( Double x )
{
	m_gfxDisplayInfo.SetX( static_cast< SF::Double >( x ) );
}

RED_INLINE void CFlashDisplayInfo::SetY( Double y )
{
	m_gfxDisplayInfo.SetY( static_cast< SF::Double >( y ) );
}

RED_INLINE void CFlashDisplayInfo::SetZ( Double z )
{
	m_gfxDisplayInfo.SetZ( static_cast< SF::Double >( z ) );
}

RED_INLINE void CFlashDisplayInfo::SetXRotation( Double degrees )
{
	m_gfxDisplayInfo.SetXRotation( static_cast< SF::Double >( degrees ) );
}

RED_INLINE void CFlashDisplayInfo::SetYRotation( Double degrees )
{
	m_gfxDisplayInfo.SetYRotation( static_cast< SF::Double >( degrees ) );
}

RED_INLINE void CFlashDisplayInfo::SetXScale( Double xscale )
{
	m_gfxDisplayInfo.SetXScale( static_cast< SF::Double >( xscale ) );
}

RED_INLINE void CFlashDisplayInfo::SetYScale( Double yscale )
{
	m_gfxDisplayInfo.SetYScale( static_cast< SF::Double >( yscale ) );
}

RED_INLINE void CFlashDisplayInfo::SetZScale( Double zscale )
{
	m_gfxDisplayInfo.SetZScale( static_cast< SF::Double >( zscale ) );
}

#endif // USE_SCALEFORM