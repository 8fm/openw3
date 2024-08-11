
#pragma once

#include "..\..\common\game\storySceneEventsCollector.h"

class CEdSceneEditor;
class CStorySceneLinkElement;
class CStorySceneControlPart;
class CStorySceneSection;

class CEdSceneFlowCtrl
{
	class CFlowEventFilter : public CStorySceneEventsCollectorFilter
	{
	public:
		virtual Bool DoFilterOut( const CStorySceneEvent* e ) const;
	};


	CEdSceneEditor*									m_mediator;
	TDynArray< const CStorySceneLinkElement* >		m_activePartsFlow;
	TDynArray< CName >								m_activePartsFlowDialogsets;
	CName											m_currentDialogset;
	THashSet< const CStorySceneLinkElement* >		m_activePartsSet;
	const CStorySceneSection*						m_selectedSection;

	CFlowEventFilter								m_flowEventsFilter;
	CStorySceneEventsCollector						m_flowCollector;

public:
	CEdSceneFlowCtrl();

	void Init( CEdSceneEditor* mediator );

	void SetSection( const CStorySceneSection* section, CStoryScenePreviewPlayer* player );
	void RecalcFlow( CStoryScenePreviewPlayer* player );
	void RecalcOnlyEvents( CStoryScenePreviewPlayer* player );

	Bool HasFlowFor( const CStorySceneSection* section ) const;
	void GetFlow( const CStorySceneSection* s, TDynArray< const CStorySceneLinkElement* >& flow, Bool withInputs );
	const CStorySceneInput* GetInputFor( const CStorySceneSection* s ) const;
	CName GetDialogsetNameFromFlowFor( const CStorySceneSection* s ) const;

	const CStorySceneEventsCollector& GetEventsFlowCollector() const;

	Bool IsBlockActivated( const CStorySceneControlPart* cp ) const;
	Float GetBlockActivationAlpha( const CStorySceneControlPart* cp ) const;

private:
	void FindAndCollectSectionFlow( const CStorySceneSection* section, TDynArray< const CStorySceneLinkElement* >& flow, Bool withInputs ) const;
	Bool FindAndCollectSectionFlow( TDynArray< const CStorySceneLinkElement* >& outFlow, TDynArray< const CStorySceneLinkElement* >& visited, Bool withInputs ) const;
	void FindSectionFlow( const CStorySceneLinkElement* currElement, const CStorySceneLinkElement* nextElement, TDynArray< const CStorySceneLinkElement* >& out ) const;

	void CollectImportantEventsForFlow( CStoryScenePreviewPlayer* player );
	void AddResetEvents( CStorySceneEventsCollector& collector, CStoryScenePreviewPlayer* player );
};