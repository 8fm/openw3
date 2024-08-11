/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#ifdef USE_SCALEFORM

//////////////////////////////////////////////////////////////////////////
// IScaleformInputEventListener
//////////////////////////////////////////////////////////////////////////
class IScaleformExternalInterfaceHandler
{
protected:
	IScaleformExternalInterfaceHandler() {}
	virtual ~IScaleformExternalInterfaceHandler() {}

public:
	virtual void OnExternalInterface( GFx::Movie* movie, const SFChar* methodName, const GFx::Value* args, SFUInt argCount )=0;
};

//////////////////////////////////////////////////////////////////////////
// CScaleformExternalInterface
//////////////////////////////////////////////////////////////////////////
class CScaleformExternalInterface : public GFx::ExternalInterface
{
private:
	IScaleformExternalInterfaceHandler* m_handler;

public:
	void			SetHandler( IScaleformExternalInterfaceHandler* handler );

public:
	virtual void	Callback( GFx::Movie* movie, const SFChar* methodName, const GFx::Value* args, SFUInt argCount );
};

#endif // USE_SCALEFORM