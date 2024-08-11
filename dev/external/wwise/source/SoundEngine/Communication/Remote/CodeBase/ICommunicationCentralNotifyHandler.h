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
/// Declaration of ICommunicationCentralNotifyHandler interface
/// \sa
/// - \ref initialization_comm
/// - \ref framerendering_comm
/// - \ref termination_comm

#ifndef _AK_COMM_ICOMMUNICATIONCENTRALNOTIFYHANDLER_H
#define _AK_COMM_ICOMMUNICATIONCENTRALNOTIFYHANDLER_H

namespace AK
{
	namespace Comm
	{
		/// Communication central notification handler
		class ICommunicationCentralNotifyHandler
		{
		protected:
			/// Virtual destructor on interface to avoid warnings.
			virtual ~ICommunicationCentralNotifyHandler(){}

		public:
			virtual void PeerDisconnected() = 0;
		};
	}
}

#endif
