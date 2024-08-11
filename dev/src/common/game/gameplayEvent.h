#pragma once

///////////////////////////////////////////////////////////////////////////////

struct SGameplayEventData
{
	void*			m_customData;
	IRTTIType*		m_customDataType;

	template < class TClass >
	RED_INLINE TClass* Get() const
	{
		if ( m_customData && m_customDataType && m_customDataType->GetType() == RT_Class )
		{
			CClass* classId = (CClass*)m_customDataType;
			return Cast< TClass > ( classId, m_customData );
		}
		return NULL;
	}
};
