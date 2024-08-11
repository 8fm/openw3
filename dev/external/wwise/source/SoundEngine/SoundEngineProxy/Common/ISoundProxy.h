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

#include "IParameterNodeProxy.h"

struct AkAudioFormat;

class ISoundProxy : virtual public IParameterNodeProxy
{
	DECLARE_BASECLASS( IParameterNodeProxy );
public:
	virtual void SetSource( AkUniqueID in_sourceID, AkUInt32 in_PluginID, const AkOSChar* in_szFileName, AkFileID in_uCacheID ) = 0;

	virtual void SetSource( 
		AkUniqueID in_sourceID 
		) = 0;

	virtual void IsZeroLatency( bool in_bIsZeroLatency ) = 0;

	virtual void SetEnvelope( AkUniqueID in_sourceID, AkFileParser::EnvelopePoint * in_pEnvelope, AkUInt32 in_uNumPoints, AkReal32 in_fMaxEnvValue ) = 0;

	enum MethodIDs
	{
		MethodSetSource = __base::LastMethodID,
		MethodSetSource_Plugin,
		MethodIsZeroLatency,
		MethodSetEnvelope,

		LastMethodID
	};
};
#endif // #ifndef AK_OPTIMIZED
