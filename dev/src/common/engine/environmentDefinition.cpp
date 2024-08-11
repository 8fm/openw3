/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "environmentDefinition.h"
#include "bitmapTexture.h"

IMPLEMENT_ENGINE_CLASS( CEnvironmentDefinition );

CEnvironmentDefinition::CEnvironmentDefinition()
{	
}

CEnvironmentDefinition::~CEnvironmentDefinition()
{	
}

void CEnvironmentDefinition::SetAreaEnvironmentParams( const CAreaEnvironmentParams & ep )
{
	m_envParams = ep;
}

void CEnvironmentDefinition::GetAdditionalInfo( TDynArray< String >& info ) const
{
	TBaseClass::GetAdditionalInfo( info );
}

void CEnvironmentDefinition::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );
	// update env manager //todo	
}

CEnvironmentDefinition* CEnvironmentDefinition::Create( const FactoryInfo& data )
{	
	// Create new bitmap object
	CEnvironmentDefinition* obj = data.CreateResource();

#ifndef NO_EDITOR
	// Setup initial data
	obj->m_envParams = data.defaultEnvParams;
#endif

	// Done
	return obj;
}

void CEnvironmentDefinition::OnPostLoad()
{
	TBaseClass::OnPostLoad();

	// Force load balance maps. Don't want to async load these on-demand, because that may result in popping.
	// Do it in OnPostLoad, so that it won't stall main thread when we eventually use it.
	//
	// Not switching to THandle, because it may be even better to try some form of streaming on these later.
	// Unloading presents a problem, since render textures assume the "resident" data is already loaded, and
	// if it isn't (such as if we re-create the render texture) will force a sync-load. Would basically need
	// to unload the entire CBitmapTexture, while maintaining a TSoftHandle...
	m_envParams.m_finalColorBalance.m_balanceMap0.Get();
	m_envParams.m_finalColorBalance.m_balanceMap1.Get();
}

void CEnvironmentDefinition::OnSave()
{
}