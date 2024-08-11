/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once

class IFlashStatusListener
{
public:
	virtual void OnFlashLoaded()=0;
	virtual void OnFlashUnloaded()=0;

protected:
	IFlashStatusListener() {}
	~IFlashStatusListener() {}
};