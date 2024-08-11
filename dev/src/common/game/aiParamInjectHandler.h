/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

struct SAIParametersSpawnList;

class CAiParamInjectHandler : public ISpawnEventHandler
{	
	typedef ISpawnEventHandler Super;
protected:
	THandle< IAIParameters > m_injectedParams;

public:
	DECLARE_NAMED_EVENT_HANDLER( AI )

	CAiParamInjectHandler( IAIParameters* injectedParams )
		: m_injectedParams( injectedParams )
	{
	}

	virtual void InjectAIParams( SAIParametersSpawnList& aiList );
};



class CAiSpawnSystemParamInjectHandler : public CAiParamInjectHandler
{	
public:
	CAiSpawnSystemParamInjectHandler( IAIParameters* injectedParams )
		: CAiParamInjectHandler( injectedParams )
	{
	}

	virtual void InjectAIParams( SAIParametersSpawnList& aiList ) override;
};