/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "names.h"
#include "scriptFileContext.h"

/// Value container for scripting value initialization
class CScriptDefaultValue
{
protected:
	typedef TDynArray< CScriptDefaultValue*, MC_ScriptCompilation, MemoryPool_ScriptCompilation > SubValues;

	CScriptFileContext		m_context;		//!< Where it was defined
	CName					m_name;			//!< Name of the value to set (used in subvalues)
	String					m_value;		//!< Value in case of direct values
	SubValues				m_subValues;	//!< Sub values ( for structures and arrays )

public:
	//! Get the name
	RED_INLINE CName GetName() const { return m_name; }

	//! Get the value string
	RED_INLINE const String& GetValue() const { return m_value; }

	//! Get the definition context
	RED_INLINE const CScriptFileContext& GetContext() const { return m_context; }

	//! Get the number of sub values
	RED_INLINE size_t GetNumSubValues() const { return m_subValues.Size(); }

	//! Get sub value
	RED_INLINE const CScriptDefaultValue* GetSubValue( Uint32 index ) const { return m_subValues[ index ]; }

public:
	//! Constructor for unnamed values
	CScriptDefaultValue( const CScriptFileContext* context, const String& value );

	//! Named value
	CScriptDefaultValue( const CScriptFileContext* context, CName name, const String& value );

	//! Destructor
	~CScriptDefaultValue();

public:
	//! Is this complex value (has sub values ?)
	RED_INLINE Bool IsComplex() const
	{
		return m_subValues.Size() > 0;
	};

	//! Is this named value ?
	RED_INLINE Bool IsNamed() const
	{
		return m_name;
	}

public:
	//! Add sub value
	void AddSubValue( CScriptDefaultValue* value );

	//! Set name of value
	void SetName( CName name );

	//! Convert to string
	String ToString() const;

	DECLARE_CLASS_MEMORY_POOL( MemoryPool_ScriptCompilation, MC_ScriptCompilation );
};
