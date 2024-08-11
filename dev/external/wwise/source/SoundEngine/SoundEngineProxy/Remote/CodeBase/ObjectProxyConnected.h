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

class CAkIndexable;
class CommandDataSerializer;

class ObjectProxyConnected
{
public:
	AkForceInline ObjectProxyConnected() : m_pIndexable( NULL ) {}
	virtual ~ObjectProxyConnected();

	virtual void HandleExecute( AkUInt16 in_uMethodID, CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer ) = 0;

	AkForceInline void SetIndexable( CAkIndexable * in_pIndexable ) { AKASSERT( m_pIndexable == NULL ); m_pIndexable = in_pIndexable; }
	AkForceInline CAkIndexable * GetIndexable() const { return m_pIndexable; }

private:
	CAkIndexable * m_pIndexable;
};

class ObjectProxyConnectedWrapper
{
public:
	AkForceInline ~ObjectProxyConnectedWrapper()
	{
		((ObjectProxyConnected *) this )->~ObjectProxyConnected();
	}

	AkForceInline ObjectProxyConnected * GetObjectProxyConnected()
	{
		return (ObjectProxyConnected *) this;
	}
};

#endif // #ifndef AK_OPTIMIZED
