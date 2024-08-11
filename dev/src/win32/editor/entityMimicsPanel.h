
#pragma once

#include "patToolPanel.h"
#include "../../common/engine/behaviorGraphAnimationMixerSlot.h"

class CEdEntityEditorMimicsPanel : public CPatToolPanel
{
	DECLARE_EVENT_TABLE();

	THandle< CEntity >				m_entity;
	CBehaviorMixerSlotInterface		m_slot;

	Bool							m_isActive;
	TDynArray< Float >				m_tracks;

public:
	CEdEntityEditorMimicsPanel( wxWindow* parent );

	void Activate( Bool flag );
	void SetEntity( CEntity* e );

protected:
	virtual void OnControlsPreChanged() override;
	virtual void OnControlsChanging() override;
	virtual void OnControlsPostChanged() override;

private:
	void RefreshLogic();
};
