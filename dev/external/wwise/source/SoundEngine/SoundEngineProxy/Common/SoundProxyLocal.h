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

#include "ParameterNodeProxyLocal.h"
#include "ISoundProxy.h"

class CAkSound;

class SoundProxyLocal : public ParameterNodeProxyLocal
						, virtual public ISoundProxy
{
public:
	SoundProxyLocal( AkUniqueID in_id );
	virtual ~SoundProxyLocal();

	// ISoundProxy members
	virtual void SetSource( AkUniqueID in_sourceID, AkUInt32 in_PluginID, const AkOSChar* in_szFileName, AkFileID in_uCacheID );

	virtual void SetSource( 
		AkUniqueID in_sourceID
		);

	virtual void IsZeroLatency( bool in_bIsZeroLatency );

	virtual void SetEnvelope( AkUniqueID in_sourceID, AkFileParser::EnvelopePoint * in_pEnvelope, AkUInt32 in_uNumPoints, AkReal32 in_fMaxEnvValue );
};
#endif
#endif // #ifndef AK_OPTIMIZED
