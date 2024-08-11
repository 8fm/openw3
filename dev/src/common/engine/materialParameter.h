/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "materialBlock.h"

/// User parametrized material block
class CMaterialParameter : public CMaterialBlock
{
	DECLARE_ENGINE_ABSTRACT_CLASS( CMaterialParameter, CMaterialBlock );

protected:
	CName		m_parameterName;		//!< Name of the parameter
	CName		m_parameterGroup;		//!< Name of the parameter group

public:
	//! Get parameter name
	RED_INLINE const CName& GetParameterName() const { return m_parameterName; }
	
	//! Get parameter group
	RED_INLINE const CName& GetParameterGroup() const { return m_parameterGroup; }

	//! Get parameter name
	void SetParameterName( const CName paramName );
public:

	//! Get parameter property
	virtual IProperty* GetParameterProperty() const=0;

public:

#ifndef NO_EDITOR_GRAPH_SUPPORT

	//! Property changed
	virtual void OnPropertyPostChange( IProperty *property );	

	//! Get title bar color
	virtual Color GetTitleColor() const;

#endif

protected:

#ifndef NO_EDITOR_GRAPH_SUPPORT

	//! Format caption
	virtual String FormatParamCaption( const String& defaultCaption ) const;

#endif

};

BEGIN_ABSTRACT_CLASS_RTTI( CMaterialParameter );
	PARENT_CLASS( CMaterialBlock );
	PROPERTY_EDIT( m_parameterName, TXT("User name of parameter") );
	PROPERTY_EDIT_NOT_COOKED( m_parameterGroup, TXT("Parameter group name") );
END_CLASS_RTTI();