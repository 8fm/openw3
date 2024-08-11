/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "entityColorVariant.h"
#include "../core/loadingJob.h"
#include "../core/memoryFileReader.h"
#include "../core/dependencyLoader.h"
#include "entityTemplatePreloadedEffects.h"
#include "../core/loadingJob.h"

class CJobEffectPreloading : public ILoadJob
{
public:
	CJobEffectPreloading( const SharedDataBuffer sourceData, CEntityTemplatePreloadedEffects* entityTemplatePreloadedEffects );
	~CJobEffectPreloading();

	virtual const Char* GetDebugName() const override { return TXT("EffectPreloading"); }
	virtual EJobResult Process();

private:
	const SharedDataBuffer m_sourceDataBuffer;	// Entity's data buffer with effects
	CEntityTemplatePreloadedEffects* m_entityTemplatePreloadedEffects;		// Entity which we are loading for

};
