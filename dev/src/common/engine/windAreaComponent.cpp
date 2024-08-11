/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "windAreaComponent.h"
#include "weatherManager.h"
#include "game.h"
#include "world.h"
#include "environmentDefinition.h"
#include "../core/dataError.h"
#include "entity.h"
#include "layer.h"
#include "utils.h"

IMPLEMENT_ENGINE_CLASS( CWindAreaComponent );

CWindAreaComponent::CWindAreaComponent()	
{
}

void CWindAreaComponent::OnAttached( CWorld* world )
{
	ASSERT( world );	
	TBaseClass::OnAttached( world );
	CEnvironmentManager* env = world->GetEnvironmentManager();
	if( env )
	{
		CWeatherManager* wmen = env->GetWeatherManager();
		if( wmen )
		{
			wmen->RegisterCWindAreaComponent( this );
		}
	}
}

void CWindAreaComponent::OnDetached( CWorld* world )
{
	ASSERT( world );	
	// Base detach
	TBaseClass::OnDetached( world );
	CEnvironmentManager* env = world->GetEnvironmentManager();
	if( env )
	{
		CWeatherManager* wmen = env->GetWeatherManager();
		if( wmen )
		{
			wmen->UnRegisterCWindAreaComponent( this );
		}
	}
}

// Since the CWaterComponent base type changed at some point (and thus, all the data), we need to manually handle
// the data when the serialised file is too old. Fix for TTP W3#13716
void CWindAreaComponent::OnSerialize( IFile& file )
{
	if( file.GetVersion() < VER_IDLINES_REFACTOR )
	{
		HandleOldFileVersion();
	}
	else
	{
		TBaseClass::OnSerialize( file );
	}
}

// Since the CWaterComponent base type changed at some point (and thus, all the data), we need to manually handle
// the data when the serialised file is too old. Fix for TTP W3#13716
void CWindAreaComponent::OnSerializeAdditionalData( IFile& file )
{
	if( file.GetVersion() < VER_IDLINES_REFACTOR )
	{
		HandleOldFileVersion();
	}
	else
	{
		TBaseClass::OnSerialize( file );		
	}
}

void CWindAreaComponent::OnEditorEndVertexEdit()
{
	TBaseClass::OnEditorEndVertexEdit();
}

void CWindAreaComponent::HandleOldFileVersion() const
{
	RED_HALT( "CWaterComponent data is from an unsupported version, and can not be loaded! This must be fixed!" );
	DATA_HALT( DES_Uber, CResourceObtainer::GetResource( this ), TXT( "Water" ), TXT( "This CWaterComponent needs to be removed! The file version is no longer supported" ) );
}

