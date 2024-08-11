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

#include "IObjectProxy.h"

#include "AkParameters.h"
#include "AkRTPC.h"

struct AkFXParamBlob
{
	void * pBlob;
	AkUInt32 uBlobSize;
	bool bInterpretAsString;
	bool bInterpretAsFxData;
	
	//
	AkFXParamBlob()
	:bInterpretAsString(false)
	,bInterpretAsFxData(false)
	{}

	template <class	T>
	bool Serialize( T& in_rSerializer ) const
	{
		void * pSerialized;
		AkUInt32 uSerializedSize;

		T paramSerializer( in_rSerializer.GetSwapEndian() );

		if ( bInterpretAsString )
		{
			paramSerializer.Put( (const AkUtf16*)pBlob );
		}
		else if ( bInterpretAsFxData )
		{
			// This section was added to fix 
			// WG-18526
			// When the data arrives from a plug-in serializer, the data is already byteswapped.
			// The problem was if the blob size was either 8,4,3 or 1 (see below)
			// the Data was byteswapped twice.
			pSerialized = pBlob;
			uSerializedSize = uBlobSize;
		}
		else
		{
			switch( uBlobSize )
			{
			case 8:
				paramSerializer.Put( *(AkInt64*) pBlob );
				break;
		
			case 4:
				paramSerializer.Put( *(AkUInt32*) pBlob );
				break;
		
			case 2:
				paramSerializer.Put( *(AkUInt16*) pBlob );
				break;
		
			case 1:
				paramSerializer.Put( *(AkUInt8*) pBlob );
				break;
		
			default:
				pSerialized = pBlob;
				uSerializedSize = uBlobSize;
				break;
			}
		}

		if( paramSerializer.GetWrittenSize() > 0 )
		{
			pSerialized = paramSerializer.GetWrittenBytes();
			uSerializedSize = paramSerializer.GetWrittenSize();
		}

		return in_rSerializer.Put( pSerialized, uSerializedSize );
	}

	template <class	T>
	bool Deserialize( T& in_rSerializer )
	{
		return in_rSerializer.Get( pBlob, uBlobSize );
	}
};

class IFxBaseProxy : virtual public IObjectProxy
{
	DECLARE_BASECLASS( IObjectProxy );
public:
	virtual void SetFX( 
		AkPluginID in_FXID
		) = 0;

	virtual void SetFXParam( 
		AkFXParamBlob in_blobParams, // Placed first due to align-4 constraint on the blob
		AkPluginParamID in_uParamID
		) = 0;

	virtual void SetRTPC( 
		AkRtpcID			in_RTPC_ID,
		AkRTPC_ParameterID	in_ParamID,
		AkUniqueID			in_RTPCCurveID,
		AkCurveScaling		in_eScaling,
		AkRTPCGraphPoint* in_pArrayConversion = NULL, 
		AkUInt32 in_ulConversionArraySize = 0
		) = 0;

	virtual void UnsetRTPC( 
		AkRTPC_ParameterID in_ParamID,
		AkUniqueID in_RTPCCurveID
		) = 0;

	virtual void SetMediaID( 
		AkUInt32 in_uIdx, 
		AkUniqueID in_mediaID 
		) = 0;

	virtual void LoadPluginMedia( 
		AkUInt32 in_uIdx, 
		const AkOSChar* in_pszSourceFile, 
		AkUniqueID in_mediaID, 
		unsigned long in_hash 
		) = 0;
	
	virtual void ReleaseAllPluginMedia() = 0;

	enum MethodIDs
	{
		MethodSetFX = __base::LastMethodID,
		MethodSetFXParam,

		MethodSetRTPC,
		MethodUnsetRTPC,
		MethodSetMediaID,

		LastMethodID
	};
};
#endif // #ifndef AK_OPTIMIZED
