/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#ifndef _EFFECT_BASE_H_
#define _EFFECT_BASE_H_

#pragma once

#include "build.h"
class IEffect
{
public:
	virtual ~IEffect(){};
	virtual Bool Initialize() = 0;
	virtual void SetShaders() = 0;

	virtual void Dispatch(int x, int y, int z){ /* intentionaly empty */ };

	GpuApi::ShaderRef CreateShaderFromFile( const Char* path, GpuApi::eShaderType shaderType ) const;
};
#endif //_EFFECT_BASE_H_
