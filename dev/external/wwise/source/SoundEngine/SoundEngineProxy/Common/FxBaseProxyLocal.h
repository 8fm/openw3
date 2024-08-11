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

#include "ObjectProxyLocal.h"
#include "IFxBaseProxy.h"

#ifndef PROXYCENTRAL_CONNECTED
	#include <vector>
	#include <map>
#endif

class FxBaseProxyLocal 
	: public ObjectProxyLocal
	, virtual public IFxBaseProxy
{
public:
	FxBaseProxyLocal( AkUniqueID in_id, bool in_bShareSet );
	virtual ~FxBaseProxyLocal();

	// IFxBaseProxy members
	virtual void SetFX( 
		AkPluginID in_FXID
		);

	virtual void SetFXParam( 
		AkFXParamBlob in_blobParams, // Placed first due to align-4 constraint on the blob
		AkPluginParamID in_uParamID
		);

	virtual void SetRTPC( 
		AkRtpcID			in_RTPC_ID,
		AkRTPC_ParameterID	in_ParamID,
		AkUniqueID			in_RTPCCurveID,
		AkCurveScaling		in_eScaling,
		AkRTPCGraphPoint* in_pArrayConversion = NULL, 
		AkUInt32 in_ulConversionArraySize = 0
		);

	virtual void UnsetRTPC( 
		AkRTPC_ParameterID in_ParamID,
		AkUniqueID in_RTPCCurveID
		);

	virtual void SetMediaID( 
		AkUInt32 in_uIdx, 
		AkUniqueID in_mediaID 
		);

	virtual void LoadPluginMedia( 
		AkUInt32 in_uIdx, 
		const AkOSChar* in_pszSourceFile, 
		AkUniqueID in_mediaID, 
		unsigned long in_hash 
		);
	
	virtual void ReleaseAllPluginMedia();

#ifndef PROXYCENTRAL_CONNECTED
private:
	static void AddRefOrLoadData( const AkOSChar* in_pszSourceFile, AkUniqueID in_mediaID, unsigned long in_hash );
	static void ResetData( const AkOSChar* in_pszSourceFile, AkUniqueID in_mediaID, unsigned long in_hash );
	static void DecRefOrReleaseData( AkUniqueID in_mediaID );

	typedef std::vector< AkUniqueID > RefPluginVector;
	RefPluginVector m_RefedPluginMedia;

	////////////////////////////////////////////////////////////////////////////////
	struct DataInfo
	{
		AkUInt32 refcount;
		unsigned long hash;
	};
	typedef std::map< AkUniqueID, DataInfo > RefGlobalPluginSet;

	// The m_globalRefedPluginMedia member has the following purpose:
	//   While each ParameterableProxyLocal object has his own m_RefedPluginMedia, the global
	//   m_globalRefedPluginMedia allows having all the proxy to share the single ref'counted entry 
	//   point. Without this global entry point, when each node are calling the sound engine directly, 
	//   none is able to really change the currently loaded data since when using an effect
	//   shareset, multiple nodes would be independently sending a notification, and they would have 
	//   to all simultaneously release their data so it can be changed in the engine.
	static RefGlobalPluginSet m_globalRefedPluginMedia;
	////////////////////////////////////////////////////////////////////////////////
#endif
};
#endif
#endif // #ifndef AK_OPTIMIZED
