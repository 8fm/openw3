/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI 

#include "redGuiTypes.h"

namespace RedGui
{
	class CRedGuiUserData
	{
		DECLARE_CLASS_MEMORY_POOL( MemoryPool_RedGui, MC_RedGuiInternals );
	public:
		CRedGuiUserData();
		CRedGuiUserData( RedGuiAny userData );
		CRedGuiUserData(const CRedGuiUserData& userData);
		virtual ~CRedGuiUserData();

		// Set user string
		void SetUserString(const String& key, const String& value);

		// Get user string or "" if not found
		const String& GetUserString(const String& key) const;

		// Get map of all user strings
		const MapString& GetUserStrings() const;

		// Delete user string
		void ClearUserString(const String& key);

		// Return true if user string with such key exist
		Bool IsUserString(const String& key) const;

		// Delete all user strings
		void ClearUserStrings();

		// Set any user data to store inside
		void SetUserData(void* data);

		// Get user data and cast it to ReturnType
		template <typename ReturnType>
		ReturnType* GetUserData()
		{
			return static_cast<ReturnType*>(m_userData);
		}

	protected:
		MapString m_userStrings;
		void* m_userData;
	};

}	// namespace RedGui

#endif	// NO_RED_GUI
