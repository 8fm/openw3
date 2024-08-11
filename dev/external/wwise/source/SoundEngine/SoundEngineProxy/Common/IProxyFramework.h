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

/// \file 
/// Proxy Framework interface.
/// \sa
/// - \ref initialization_comm
/// - \ref framerendering_comm
/// - \ref termination_comm

namespace AK
{
	namespace Comm
	{
		class INotificationChannel;
		class IRendererProxy;

		/// Proxy Framework interface.
		/// \sa
		/// - \ref initialization_comm
		/// - \ref framerendering_comm
		/// - \ref termination_comm
		class IProxyFramework
		{
		protected:
			/// Virtual destructor on interface to avoid warnings.
			virtual ~IProxyFramework(){}

		public:
				
			/// Initialize framework.
			/// This method has to be called once, before any other method call. 
			/// \sa
			/// - \ref initialization_comm
			virtual void Init() = 0;

			/// Terminate framework.
			/// This method has to be called once, before Destroy().
			/// \sa
			/// - \ref termination_comm
			virtual void Term() = 0;

			/// Destroy framework.
			/// \sa
			/// - \ref termination_comm
			virtual void Destroy() = 0;

			/// Set notification channel.
			virtual void SetNotificationChannel( 
				INotificationChannel* in_pNotificationChannel	///< Pointer to channel interface (see ICommunicationCentral::GetNotificationChannel()).
				) = 0;
			
			/// Get renderer proxy.
			/// \return Pointer to renderer proxy interface.
			virtual IRendererProxy* GetRendererProxy() = 0;
		};
	}
}

#endif // #ifndef AK_OPTIMIZED
