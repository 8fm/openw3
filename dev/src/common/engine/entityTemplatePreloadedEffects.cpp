/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "entityTemplatePreloadedEffects.h"

void CEntityTemplatePreloadedEffects::Release()
{
	if( m_refCount.Decrement() == 0 )
	{
		delete this;
	}
}

void CEntityTemplatePreloadedEffects::AddRef()
{
	m_refCount.Increment();
}

void CEntityTemplatePreloadedEffects::FlushPreloadedEffectsTo(TDynArray< CFXDefinition* >& effects, CObject* entityTemplate)
{
	s_effectsPreloadedMutex.Acquire();
	Uint32 preloadedArraySize = m_effectsPreloaded.Size();
	for( Uint32 i=0; i<preloadedArraySize; ++i )
	{
		// Check if we already have that effect (i.e. just loaded by another thread (not job))
		Bool found = false;
		for ( Uint32 j = 0; j < effects.Size(); ++j )
		{
			if ( effects[j]->GetName() == m_effectsPreloaded[i]->GetName() )
			{
				found = true;
				break;
			}
		}

		// If not found, then add effect to preloaded effect array
		if ( !found )
		{
			m_effectsPreloaded[i]->SetParent( entityTemplate );
			effects.PushBack( m_effectsPreloaded[i] );
		}
		else
		{
			m_effectsPreloaded[i]->Discard();
		}
	}
	m_effectsPreloaded.Clear();
	s_effectsPreloadedMutex.Release();
}

void CEntityTemplatePreloadedEffects::CollectForGC(IFile& file) 
{
	s_effectsPreloadedMutex.Acquire();
	Uint32 preloadedArraySize = m_effectsPreloaded.Size();
	for( Uint32 i=0; i<preloadedArraySize; ++i )
	{
		file << m_effectsPreloaded[i];
	}
	s_effectsPreloadedMutex.Release();
}

void CEntityTemplatePreloadedEffects::AddPreloadedEffect(CFXDefinition* effect)
{
	s_effectsPreloadedMutex.Acquire();
	Bool exists = false;
	for( Uint32 i=0; i<m_effectsPreloaded.Size(); ++i )
	{
		if( m_effectsPreloaded[i]->GetName() == effect->GetName() )
		{
			effect->Discard();
			exists = true;
		}
	}
	if( exists == false )
	{
		m_effectsPreloaded.PushBack( effect );
	}
	s_effectsPreloadedMutex.Release();
}

CEntityTemplatePreloadedEffects::CEntityTemplatePreloadedEffects() : m_refCount( 1 )
{

}

