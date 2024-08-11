#pragma once

// Simple go-between utility class so that the scripts can get easy access to the time
class CTimerScriptKeyword : public CObject
{
	DECLARE_ENGINE_CLASS( CTimerScriptKeyword, CObject, 0 )

public:
	CTimerScriptKeyword();
	~CTimerScriptKeyword();

	RED_INLINE void SetTimer( Float delta, Float deltaUnscaled )
	{
		m_timeDelta = delta;
		m_timeDeltaUnscaled = deltaUnscaled;
	}

private:
	Float m_timeDelta;
	Float m_timeDeltaUnscaled;
};

BEGIN_CLASS_RTTI( CTimerScriptKeyword )
	PARENT_CLASS( CObject )
	PROPERTY_NOSERIALIZE( m_timeDelta );
	PROPERTY_NOSERIALIZE( m_timeDeltaUnscaled );
END_CLASS_RTTI();
