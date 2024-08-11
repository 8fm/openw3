/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "memory.h"
#include "property.h"

/// Default value of class property
class CDefaultValue
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Engine );

	friend class CScriptCompiler;
	friend class CRTTISerializer;

protected:
	CProperty*					m_property;		//!< Property
	CVariant					m_data;			//!< Value ( serialized using property type )
	TDynArray< CDefaultValue* >	m_subValues;	//!< Sub values ( structure fields, array elements )
	CClass*						m_inlineClass;	//!< Class of object to create

public:
	CDefaultValue();
	CDefaultValue( CProperty* prop );
	~CDefaultValue();

	/// SERIALIZATION ///

	// Gets property
	RED_INLINE CProperty* GetProperty() const
	{ return m_property; }

	// Sets property
	RED_INLINE void SetProperty( CProperty* prop )
	{ m_property = prop; }

	// Gets subvalues
	RED_INLINE const TDynArray< CDefaultValue* >& GetSubValues() const
	{ return m_subValues; }

	//////////////////////////////////////////////////////////////////////////

	//! Get the string representation
	void ToString() const;

	//! Apply to object
	Bool Apply( IScriptable* owner, void* object, Bool createInlinedObjects ) const;

	//! Low-level apply
	Bool Apply( IScriptable* owner, const IRTTIType* type, void* data, Bool createInlinedObjects ) const;

protected:
	//! Add sub value
	void AddSubValue( CDefaultValue* value );

	//! Set inline class
	void SetInlineClass( CClass* inlineClass );

	//! Set value
	void SetValue( const CVariant& value );
};
