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

//////////////////////////////////////////////////////////////////////
//
// AkSrcFileServices.h
//
// Services shared by streamed sources.
//
//////////////////////////////////////////////////////////////////////

#ifndef _AK_SRC_FILE_SERVICES_H_
#define _AK_SRC_FILE_SERVICES_H_

namespace AK
{
    namespace SrcFileServices
	{
		// Returns:
		// - AK_DataReady: Prebuffering is ready.
		// - AK_NoDataReady: Prebuffering is not ready.
		// - AK_Fail: Fatal IO error.
		inline AKRESULT IsPrebufferingReady( AK::IAkAutoStream * in_pStream, AkUInt32 in_uSizeLeft )
		{
			AkUInt32 uBuffering;
			AKRESULT eBufferingResult = in_pStream->QueryBufferingStatus( uBuffering );

			if ( eBufferingResult == AK_DataReady
				|| eBufferingResult == AK_NoDataReady )
			{
				if ( uBuffering + in_uSizeLeft >= in_pStream->GetNominalBuffering() )
				{
					// Returned "data ready" with buffering _above_ threshold.
					return AK_DataReady;
				}
				else
				{
					// Returned "data ready" with buffering _below_ threshold.
					return AK_NoDataReady;
				}
			}
			else if ( eBufferingResult == AK_NoMoreData )
			{
				// Returned "no more data". Prebuffering is ready.
				return AK_DataReady;
			}
			else 
			{
				// All other cases.
				return eBufferingResult;
			}
		}
	}
}
#endif //_AK_SRC_FILE_SERVICES_H_
