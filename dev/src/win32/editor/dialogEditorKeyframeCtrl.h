#pragma once

class CEdSceneEditor;
class CStorySceneEvent;

struct SSceneHelperReferenceFrameSettings;

class CEdSceneKeyframeCtrl
{
private:
	CEdSceneEditor*					m_mediator;
	
public:
	THandle< CEntity >				m_selectedEntity;
	CGUID							m_selectedEntityHelper;
	EngineTransform					m_selectedEntityTransform;
	CStorySceneEvent*				m_selectedEvent;

public: 
	RED_INLINE String GetSelectedEntityName()	const	{ return m_selectedEntity ? m_selectedEntity->GetName() : TXT( "" ); }

public:
	CEdSceneKeyframeCtrl( CEdSceneEditor* mediator );

	void Init();

	CEdSceneHelperEntity* SelectEntity( CEntity* entity, Bool eventSelected, const SSceneHelperReferenceFrameSettings* frameInfo = nullptr );

	void HighlightEntity( CEntity* entity, bool isSelected );
	void MoveDefaultHelperToSelectedEntity();

	void RecreateDefaultHelper();

public:
	void OnNewKeyframe();
	Bool OnHelperEntityRefreshedProperty( const CGUID& id, Bool pos, Bool rot, Bool scale = false );

private:
	const CActor* AsSceneActor( const CEntity* e ) const;
	const CEntity* AsSceneProp( const CEntity* e ) const;
	const CEntity* AsSceneLight( const CEntity* e ) const;
};
