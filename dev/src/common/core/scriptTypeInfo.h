/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "rttiType.h"

/// Info about type
class CScriptTypeInfo
{
protected:
	ERTTITypeType	m_type;		//!< RTTI type
	CName			m_name;		//!< Type name
	Bool			m_isConst;	//!< Type is const

public:
	//! Get base type
	RED_INLINE ERTTITypeType GetType() const { return m_type; }

	//! Get the type name ( can be used to access IRTTIType* )
	RED_INLINE CName GetName() const { return m_name; }

	//! Is this type const ?
	RED_INLINE Bool IsConst() const { return m_isConst; }

	//! Is the an array
	RED_INLINE Bool IsArray() const { return m_type == RT_Array; }

	//! Is the a handle
	RED_INLINE Bool IsHandle() const { return m_type == RT_Handle; }

public:
	//! No type
	CScriptTypeInfo();

	//! Create from real type
	CScriptTypeInfo( IRTTIType* realType, Bool isConst );

	//! Manual create
	CScriptTypeInfo( CName typeName, ERTTITypeType type, Bool isConst );

public:
	//! Get RTTI type
	IRTTIType* GetRTTIType() const;

	//! Get array inner type
	CScriptTypeInfo GetInnerType() const;

	//! Describe
	String ToString() const;

public:
	//! Can we cast from one type to the other
	static Bool CanCast( const CScriptTypeInfo& srcType, const CScriptTypeInfo& destType, Bool forced, String& errorMessage );
};