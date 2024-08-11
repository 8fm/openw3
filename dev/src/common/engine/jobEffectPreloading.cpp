/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "jobEffectPreloading.h"

EJobResult CJobEffectPreloading::Process()
{
	// Deserialize cached buffer
	TDynArray< CObject* > loadedObjects;
	CMemoryFileReader reader( static_cast<const Uint8*>( m_sourceDataBuffer.GetData() ), m_sourceDataBuffer.GetSize(), 0 );
	CDependencyLoader loader( reader, NULL );

	// Load
	DependencyLoadingContext loadingContext;
	loadingContext.m_parent = NULL; 
	loadingContext.m_loadSoftDependencies = true;
	{	
		PC_SCOPE( Load );
		if ( !loader.LoadObjects( loadingContext ) )
		{
			WARN_CORE( TXT("Deserialization failed") );
			return JR_Failed;
		}
	}

	// Post load them
	{
		PC_SCOPE( PostLoad );
		loader.PostLoad();
	}

	CObject* object = loadingContext.m_loadedRootObjects[0];
	RED_ASSERT( object->IsA<CFXDefinition>(), TXT("Loaded object is not a CFXDefinition") );
	m_entityTemplatePreloadedEffects->AddPreloadedEffect( static_cast<CFXDefinition*>( object ) );

	return JR_Finished;
}

CJobEffectPreloading::~CJobEffectPreloading()
{
	m_entityTemplatePreloadedEffects->Release();
}

CJobEffectPreloading::CJobEffectPreloading(const SharedDataBuffer sourceData, CEntityTemplatePreloadedEffects* entityTemplatePreloadedEffects) : ILoadJob( JP_Resources, true )
	, m_sourceDataBuffer( sourceData )
	, m_entityTemplatePreloadedEffects( entityTemplatePreloadedEffects )
{
	entityTemplatePreloadedEffects->AddRef();
}
