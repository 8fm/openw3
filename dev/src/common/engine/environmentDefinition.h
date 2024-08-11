/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "../core/resource.h"
#include "environmentAreaParams.h"

/// Resource with all the environment (weather conditions, post processes, lighting) data
class CEnvironmentDefinition : public CResource
{
	DECLARE_ENGINE_RESOURCE_CLASS( CEnvironmentDefinition, CResource, "env", "Environment Definition" );

public:	

	//! Resource factory data
	class FactoryInfo : public CResource::FactoryInfo< CEnvironmentDefinition >
	{
	public:
		CAreaEnvironmentParams			defaultEnvParams;

	public:
		RED_INLINE FactoryInfo()
		{
			defaultEnvParams = CAreaEnvironmentParams( EnvResetMode_CurvesDefault );
		};
	};

private:

	CAreaEnvironmentParams				m_envParams;

public:

	RED_INLINE const CAreaEnvironmentParams&		GetAreaEnvironmentParams() const { return m_envParams; }
	RED_INLINE const CAreaEnvironmentParams*		GetAreaEnvironmentParamsPtr() const { return &m_envParams; }
	void											SetAreaEnvironmentParams( const CAreaEnvironmentParams & ep );

	CEnvironmentDefinition();
	virtual ~CEnvironmentDefinition();

	//! Get additional resource info, displayed in editor
	virtual void GetAdditionalInfo( TDynArray< String >& info ) const;

	//! Property was changed
	virtual void OnPropertyPostChange( IProperty* property );

	//! Resource was loaded from file
	virtual void OnPostLoad();

	virtual void OnSave();

public:
	// Create environment definition
	static CEnvironmentDefinition* Create( const FactoryInfo& data );

};

BEGIN_CLASS_RTTI( CEnvironmentDefinition );
PARENT_CLASS( CResource );
PROPERTY_EDIT( m_envParams, TXT("Environment definition") );
END_CLASS_RTTI();