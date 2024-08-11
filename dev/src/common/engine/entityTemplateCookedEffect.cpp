/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "entityTemplateCookedEffect.h"

IMPLEMENT_ENGINE_CLASS( CEntityTemplateCookedEffect );

void CEntityTemplateCookedEffect::LoadSync(CEntityTemplatePreloadedEffects* preloadedEffects)
{
	RED_LOG( EntityTemplate, TXT("Loading synchronously effect from cooked data: name=%ls, animName=%ls"), m_name.AsChar(), m_animName.AsChar() );

	// Deserialize cached buffer
	TDynArray< CObject* > loadedObjects;
	CMemoryFileReader reader( static_cast<const Uint8*>( m_buffer.GetData() ), m_buffer.GetSize(), 0 );
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
			return;
		}
	}

	// Post load them
	{
		PC_SCOPE( PostLoad );
		loader.PostLoad();
	}

	CObject* object = loadingContext.m_loadedRootObjects[0];
	RED_FATAL_ASSERT( object->IsA<CFXDefinition>(), "Loaded object is not a CFXDefinition" );
	preloadedEffects->AddPreloadedEffect( static_cast<CFXDefinition*>( object ) );
}

void CEntityTemplateCookedEffect::LoadAsync(CEntityTemplatePreloadedEffects* preloadedEffects)
{
	RED_LOG( EntityTemplate, TXT("Issuing asynchronous effect loading from cooked data: name=%ls, animName=%ls"), m_name.AsChar(), m_animName.AsChar() );
	auto preloadingJob = new CJobEffectPreloading( m_buffer, preloadedEffects );
	SJobManager::GetInstance().Issue( preloadingJob );
}

CEntityTemplateCookedEffect::CEntityTemplateCookedEffect(const CName& name, const CName& animName, const SharedDataBuffer& buffer)
	: m_name( name )
	, m_animName( animName )
	, m_buffer( buffer )
{
}

CEntityTemplateCookedEffect::CEntityTemplateCookedEffect()
{
}
