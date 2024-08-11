/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#ifdef USE_SCALEFORM

//////////////////////////////////////////////////////////////////////////
// IScaleformUserEventHandler
//////////////////////////////////////////////////////////////////////////
class IScaleformUserEventHandler
{
protected:
	IScaleformUserEventHandler() {}
	virtual ~IScaleformUserEventHandler() {}

public:
	virtual void OnUserEvent( GFx::Movie* movie, const GFx::Event& event )=0;
};

//////////////////////////////////////////////////////////////////////////
// IScaleformUserEventHandler
//////////////////////////////////////////////////////////////////////////
class CScaleformUserEvent : public GFx::UserEventHandler
{
private:
	IScaleformUserEventHandler* m_handler;

public:
	void			SetHandler( IScaleformUserEventHandler* handler );

public:
	virtual void	HandleEvent( GFx::Movie* movie, const GFx::Event& event);
};

#endif // USE_SCALEFORM