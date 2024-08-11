/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#pragma once

struct SGameplayEventParamCName
{
	DECLARE_RTTI_STRUCT( SGameplayEventParamCName );
	CName m_value;
	
	SGameplayEventParamCName() 
		:	m_value( CName::NONE )
	{
	}

	SGameplayEventParamCName( CName& param ) 
		:	m_value( param )
	{
	}
};

BEGIN_CLASS_RTTI( SGameplayEventParamCName );	
	PROPERTY( m_value );
END_CLASS_RTTI();

struct SGameplayEventParamInt
{
	DECLARE_RTTI_STRUCT( SGameplayEventParamInt );
	Int32 m_value;

	SGameplayEventParamInt() 
		:	m_value( 0 )
	{
	}

	SGameplayEventParamInt( Int32 param ) 
		:	m_value( param )
	{
	}
};

BEGIN_CLASS_RTTI( SGameplayEventParamInt );	
	PROPERTY( m_value );
END_CLASS_RTTI();

struct SGameplayEventParamFloat
{
	DECLARE_RTTI_STRUCT( SGameplayEventParamFloat );
	Float m_value;

	SGameplayEventParamFloat() 
		:	m_value( 0.0f )
	{
	}

	SGameplayEventParamFloat( Float param ) 
		:	m_value( param )
	{
	}
};

BEGIN_CLASS_RTTI( SGameplayEventParamFloat );	
	PROPERTY( m_value );
END_CLASS_RTTI();

struct SGameplayEventParamObject
{
	DECLARE_RTTI_STRUCT( SGameplayEventParamObject );
	THandle< IScriptable >	m_value;

	SGameplayEventParamObject() 
		:	m_value( NULL )
	{
	}

	SGameplayEventParamObject( const THandle< IScriptable >& param ) 
		:	m_value( param )
	{
	}
};

BEGIN_CLASS_RTTI( SGameplayEventParamObject );	
	PROPERTY( m_value );
END_CLASS_RTTI();