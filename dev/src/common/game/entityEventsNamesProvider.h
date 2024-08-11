/**
* Copyright ©2010 CD Projekt Red. All Rights Reserved.
*/

#pragma once

class IEntityEventsNamesProvider : public I2dArrayPropertyOwner
{
	void Get2dArrayPropertyAdditionalProperties( IProperty *property, SConst2daValueProperties &valueProperties ) override;
};