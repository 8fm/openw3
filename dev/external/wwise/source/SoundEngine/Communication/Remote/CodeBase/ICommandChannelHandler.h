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

/// \file
/// Declaration of ICommandChannelHandler interface
/// \sa
/// - \ref initialization_comm
/// - \ref framerendering_comm
/// - \ref termination_comm

#ifndef _AK_COMM_ICOMMANDCHANNELHANDLER_H
#define _AK_COMM_ICOMMANDCHANNELHANDLER_H

#include <AK/SoundEngine/Common/AkTypes.h>

namespace AK
{
	namespace Comm
	{
		/// Command channel handler
		class ICommandChannelHandler
		{
		protected:
			/// Virtual destructor on interface to avoid warnings.
			virtual ~ICommandChannelHandler(){}

		public:

			virtual const AkUInt8* HandleExecute( const AkUInt8* in_pData, AkUInt32 & out_uReturnDataSize ) = 0;
		};
	}
}

#endif
