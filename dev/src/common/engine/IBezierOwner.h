
#pragma once

#include "EngineBezierTools.h"

class IBezierOwner2
{
public:
	virtual ~IBezierOwner2(){}
	virtual QuadraticBezierNamespace::Curve2* GetBezier() = 0;
};

class IBezierOwner3
{
public:
	virtual ~IBezierOwner3(){}
	virtual QuadraticBezierNamespace::Curve3* GetBezier() = 0;
};