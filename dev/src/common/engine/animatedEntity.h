
#pragma once
#include "entity.h"
#include "animTickableInterface.h"

class CAnimatedEntity : public CEntity,
						public IAnimAsyncTickable
{
	DECLARE_ENGINE_CLASS( CAnimatedEntity, CEntity, 0 );

	Box		m_box;		// Box for all components
	Int8	m_state;

public:
	CAnimatedEntity();

	virtual void OnAttached( CWorld* world );
	virtual void OnAttachFinished( CWorld* world );
	virtual void OnDetached( CWorld* world );

protected:
	virtual Bool InitializeAnimatedEntity();
	void Deinitialize();

	virtual void OnInitialized();

	void ForceFirstFrame();

public: // IAnimAsyncTickable
	virtual EAsyncAnimPriority GetPriority() const;
	virtual Box GetBox() const;
	virtual void DoAsyncTick( Float dt );
};

BEGIN_CLASS_RTTI( CAnimatedEntity );
	PARENT_CLASS( CEntity );
END_CLASS_RTTI();
