#pragma once
#include "triggerManager.h"
#include "component.h"

class CSoundListenerComponent : public CComponent
{
	DECLARE_ENGINE_CLASS( CSoundListenerComponent, CObject, 0 )

protected:
	Vector m_position;
	ITriggerActivator* m_activator;

	TDynArray< const void* > m_influencedTriggerObjectsIds;

	struct STriggerRatio 
	{
		const void* m_trigger;
		float m_ratio;

		STriggerRatio( const void* trigger, float ratio ) : m_trigger( trigger ), m_ratio( ratio ) {}
		STriggerRatio() : m_trigger( 0 ) {}

		RED_INLINE Bool operator==( const STriggerRatio& trigger )
		{
			return (m_trigger == trigger.m_trigger) && (m_ratio == trigger.m_ratio);
		}

	};

	TDynArray< STriggerRatio > m_state;

public:
	CSoundListenerComponent();
	virtual ~CSoundListenerComponent();
	
	void Flush();

	void UpdatePosition( const Vector& forward, const Vector& up, const Vector& position );
	const Vector& GetPosition() { return m_position; }

	Uint32 GetNumOccupiedTrigger() const { return m_activator ? m_activator->GetNumOccupiedTrigger() : 0; }
	const ITriggerObject* GetOccupiedTrigger( Uint32 index ) const { return m_activator ? m_activator->GetOccupiedTrigger( index ) : 0; }
	
	void PushInfluencedTrigger( const void* ptr );
	Uint32 GetInfluencedTriggerCount() const { return m_influencedTriggerObjectsIds.Size(); }
	const void* GetInfluencedTrigger( Uint32 index );
};


BEGIN_CLASS_RTTI( CSoundListenerComponent )
	PARENT_CLASS( CComponent )
END_CLASS_RTTI()
