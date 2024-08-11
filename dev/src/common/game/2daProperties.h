#pragma once
#include "../core/2darray.h"

class CActionPointCategories2dPropertyOwner : public I2dArrayPropertyOwner
{
public:
	void Get2dArrayPropertyAdditionalProperties( IProperty *property, SConst2daValueProperties &valueProperties ) override;
	Bool SortChoices() const override;
};

class CAttitude2dPropertyOwner : public I2dArrayPropertyOwner
{
public:
	void Get2dArrayPropertyAdditionalProperties( IProperty *property, SConst2daValueProperties &valueProperties ) override;
	Bool SortChoices() const override;
};