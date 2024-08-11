/***********************************************************************
  The content of this file includes source code for the sound engine
  portion of the AUDIOKINETIC Wwise Technology and constitutes "Level
  Two Source Code" as defined in the Source Code Addendum attached
  with this file.  Any use of the Level Two Source Code shall be
  subject to the terms and conditions outlined in the Source Code
  Addendum and the End User License Agreement for Wwise(R).

  Version: v2013.2.9  Build: 4872
  Copyright (c) 2006-2014 Audiokinetic Inc.
 ***********************************************************************/
#pragma once

#ifndef AK_OPTIMIZED
#ifndef PROXYCENTRAL_CONNECTED

#include "IObjectProxy.h"

class CAkIndexable;

class ObjectProxyLocal : virtual public IObjectProxy
{
public:
	// IObjectProxy members
	virtual void AddRef();
	virtual bool Release();

	virtual AkUniqueID GetID() const;
    virtual bool DoesNeedEndianSwap() const;

	virtual bool IsLocalProxy() const{ return true; }

protected:
	ObjectProxyLocal();
	virtual ~ObjectProxyLocal();

	void SetIndexable( CAkIndexable* in_pIndexable );
	CAkIndexable* GetIndexable() const { return m_pIndexable; }

private:
#ifndef PROXYCENTRAL_CONNECTED
	int m_refCount;
#endif

	CAkIndexable* m_pIndexable;
};

#endif
#endif // #ifndef AK_OPTIMIZED
