/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "fxDefinition.h"
#include "../redThreads/redThreadsThread.h"

class CEntityTemplatePreloadedEffects
{
public:
	CEntityTemplatePreloadedEffects();

	void AddPreloadedEffect( CFXDefinition* effect );
	void FlushPreloadedEffectsTo( TDynArray< CFXDefinition* >& effects, CObject* entityTemplate );
	void AddRef();
	void Release();
	void CollectForGC( IFile& file );

private:
	TDynArray< CFXDefinition* >	m_effectsPreloaded;			// Effects preloaded, but not yet flushed to m_effects
	Red::Threads::CAtomic<Uint32> m_refCount;				// Reference counter, so the class will be deleted without multithread issues
	static Red::Threads::CMutex s_effectsPreloadedMutex;	// One static mutex for all Entity Templates for preloading effects

};