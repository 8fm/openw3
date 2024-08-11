/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#define DEFAULT_MATERIAL_PARAMETER_INSTANCE_ALIGNMENT 16

/// Instance of material parameter
class MaterialParameterInstance
{
protected:
	CName			m_name;			//!< Name of the parameter
	IRTTIType*		m_type;			//!< Property that defines type
	void*			m_data;			//!< Parameter value

public:
	MaterialParameterInstance();
	MaterialParameterInstance( const MaterialParameterInstance& other );
	MaterialParameterInstance( const CName& paramName, CClass* paramClass, const void* data = NULL );
	MaterialParameterInstance( const CName& paramName, const CName& typeName, const void* data = NULL );
	~MaterialParameterInstance();

	//! Get param name
	RED_INLINE const CName& GetName() const { return m_name; }

	//! Get property
	RED_INLINE IRTTIType* GetType() const { return m_type; }

	//! Get data
	RED_INLINE void* GetData() { return m_data; }

	//! Get data
	RED_INLINE const void* GetData() const { return m_data; }

public:
	//! Serialize parameter
	Bool Serialize( IFile& file );
	
	//! Assign
	MaterialParameterInstance& operator=( const MaterialParameterInstance& other );
};
