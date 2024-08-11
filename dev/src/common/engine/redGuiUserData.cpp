/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI 

#include "redGuiUserData.h"

namespace RedGui
{
	CRedGuiUserData::CRedGuiUserData()
		: m_userData(nullptr)
	{
		/*intentionally empty*/
	}

	CRedGuiUserData::CRedGuiUserData( RedGuiAny userData )
		: m_userData( userData )
	{
		/* intentionally empty */
	}

	CRedGuiUserData::~CRedGuiUserData()
	{
		/*intentionally empty*/
	}
	
	void CRedGuiUserData::SetUserString(const String& key, const String& value)
	{
		m_userStrings.Set(key, value);
	}

	const String& CRedGuiUserData::GetUserString(const String& key) const
	{
		const String* foundString = m_userStrings.FindPtr(key);

		if(foundString == nullptr)
		{
			return String::EMPTY;
		}

		return (*foundString);
	}

	const MapString& CRedGuiUserData::GetUserStrings() const
	{
		return m_userStrings;
	}

	void CRedGuiUserData::ClearUserString(const String& key)
	{
		m_userStrings.Erase(key);
	}

	Bool CRedGuiUserData::IsUserString(const String& key) const
	{
		return (m_userStrings.FindPtr(key) != nullptr);
	}

	void CRedGuiUserData::ClearUserStrings()
	{
		m_userStrings.Clear();
	}

	void CRedGuiUserData::SetUserData(void* data)
	{
		m_userData = data;
	}

	CRedGuiUserData::CRedGuiUserData( const CRedGuiUserData& userData )
		: m_userData(nullptr)
	{
		m_userData = userData.m_userData;
		m_userStrings = userData.m_userStrings;
	}

}	// namespace RedGui

#endif	// NO_RED_GUI
