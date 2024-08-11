/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class IRestartable
{
public:
	virtual bool ShouldBeRestarted() const = 0;
};
