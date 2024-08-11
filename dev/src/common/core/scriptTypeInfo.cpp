/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "scriptTypeInfo.h"
#include "class.h"

CScriptTypeInfo::CScriptTypeInfo()
	: m_isConst( true )
	, m_type( RT_Void )
{
};

CScriptTypeInfo::CScriptTypeInfo( CName typeName, ERTTITypeType type, Bool isConst )
	: m_isConst( isConst )
	, m_name( typeName )
	, m_type( type )
{
}

CScriptTypeInfo::CScriptTypeInfo( IRTTIType* realType, Bool isConst )
	: m_isConst( isConst )
	, m_name( realType->GetName() )
	, m_type( realType->GetType() )
{
}

IRTTIType* CScriptTypeInfo::GetRTTIType() const
{
	IRTTIType* type = SRTTI::GetInstance().FindType( m_name );
	ASSERT( type && type->GetType() == m_type );
	return type;
}

String CScriptTypeInfo::ToString() const
{
	return m_name.AsString();
}

CScriptTypeInfo CScriptTypeInfo::GetInnerType() const
{
	ASSERT( m_type == RT_Array );
	CRTTIArrayType* arrayType = ( CRTTIArrayType* ) GetRTTIType();
	return CScriptTypeInfo( arrayType->GetInnerType(), m_isConst );
}

Bool CScriptTypeInfo::CanCast( const CScriptTypeInfo& srcType, const CScriptTypeInfo& destType, Bool forced, String& errorMessage )
{
	// Same type
	if ( srcType.m_name == destType.m_name )
	{
		return true;
	}

	// We can cast only within the same class
	if ( srcType.m_type == destType.m_type )
	{
		CName srcName = srcType.m_name;
		CName destName = destType.m_name;

		// Simple types
		if ( srcType.m_type == RT_Simple || srcType.m_type == RT_Fundamental )
		{
			if ( srcName == GetTypeName<Int32>() )
			{
				if ( destName == GetTypeName<Float>() && forced )
				{
					return true;
				}
				else if ( destName == GetTypeName<Bool>() )
				{
					return true;
				}
				else if ( destName == GetTypeName<String>() && forced )
				{
					return true;
				}
			}
			else if ( srcName == GetTypeName<Float>() )
			{
				if ( destName == GetTypeName<Int32>() && forced )
				{
					return true;
				}
				else if ( destName == GetTypeName<Bool>() )
				{
					return true;
				}
				else if ( destName == GetTypeName<String>() && forced )
				{
					return true;
				}
			}
			else if ( srcName == GetTypeName<String>() )
			{
				if ( destName == GetTypeName<Float>() && forced )
				{
					return true;
				}
				else if ( destName == GetTypeName<Bool>() && forced )
				{
					return true;
				}
				else if ( destName == GetTypeName<Int32>() && forced )
				{
					return true;
				}
				else if ( destName == GetTypeName<CName>() && forced )
				{
					return true;
				}
			}
			else if ( srcName == GetTypeName<CName>() )
			{
				if ( destName == GetTypeName<String>() )
				{
					return true;
				}
				else if ( destName == GetTypeName<Bool>() )
				{
					return true;
				}
			}
			else if ( srcName == GetTypeName<Bool>() )
			{
				if ( destName == GetTypeName<String>() && forced )
				{
					return true;
				}
			}
		}

		// We can cast between handles
		if ( srcType.m_type == RT_Handle )
		{
			// Get pointed types
			CRTTIHandleType* srcPointedType = ( CRTTIHandleType* ) srcType.GetRTTIType();
			CClass* srcClass = ( CClass* )srcPointedType->GetPointedType();
			CRTTIHandleType* destPointedType = ( CRTTIHandleType* ) destType.GetRTTIType();
			CClass* destClass = ( CClass* )destPointedType->GetPointedType();

			// Upcast is always valid
			if ( srcClass->IsA( destClass ) )
			{
				return true;
			}

			// Downcast is valid when forced
			if ( destClass->IsA( srcClass ) && forced )
			{
				return true;
			}
		}

		// We can cast between pointer
		if ( srcType.m_type == RT_Pointer )
		{
			// Get pointed types
			CRTTIPointerType* srcPointedType = ( CRTTIPointerType* ) srcType.GetRTTIType();
			CClass* srcClass = ( CClass* )srcPointedType->GetPointedType();
			CRTTIPointerType* destPointedType = ( CRTTIPointerType* ) destType.GetRTTIType();
			CClass* destClass = ( CClass* )destPointedType->GetPointedType();

			// Upcast is always valid
			if ( srcClass->IsA( destClass ) )
			{
				return true;
			}

			// Downcast is valid when forced
			if ( destClass->IsA( srcClass ) && forced )
			{
				return true;
			}
		}
	}

	// No cast
	errorMessage = String::Printf( TXT("Cannot cast from '%ls' to '%ls'"), srcType.ToString().AsChar(), destType.ToString().AsChar() );
	return false;
}