/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"
#include "crowdAreaComponent.h"

IMPLEMENT_ENGINE_CLASS( CCrowdAreaComponent );

CCrowdAreaComponent::CCrowdAreaComponent()
{
}

CCrowdAreaComponent::~CCrowdAreaComponent()
{
}

Color CCrowdAreaComponent::CalcLineColor() const
{
	return Validate() ? Color::LIGHT_GREEN : Color::RED;
}

Bool CCrowdAreaComponent::Validate() const
{
	// TODO: add any validation code needed here
	return true;
}
