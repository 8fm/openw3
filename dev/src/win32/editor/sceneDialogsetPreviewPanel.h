/**
* Copyright c 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CStorySceneDialogset;

class CEdSceneDialogsetPreviewPanel : public CEdPreviewPanel
{
protected:
	CStorySceneDialogset*	m_dialogset;
	Uint32					m_selectedCameraNumber;
	CEntity*				m_placeableEntity;
	TDynArray< CEntity* >	m_characterGhosts;

public:
	CEdSceneDialogsetPreviewPanel( wxWindow* parent, CStorySceneDialogset* dialogset );
	~CEdSceneDialogsetPreviewPanel();

	void SetSelectedCameraNumber( Uint32 cameraNumber ) { m_selectedCameraNumber = cameraNumber; }
	void TogglePlaceables( Bool enable );
	void ToggleCharacters( Bool enable );

public:
	virtual void OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame );
};