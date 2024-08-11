/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "previewItem.h"


class CEdEntityPreviewPanel;

class CEntityEditorWoundItem : public IPreviewItem
{
public:
	CEntityEditorWoundItem( CEdEntityPreviewPanel* editorPreview, const CName& woundName );

	virtual Bool IsValid() const;

	virtual void Refresh();

	virtual void SetPositionFromPreview( const Vector& prevPos, Vector& newPos );
	virtual void SetRotationFromPreview( const EulerAngles& prevRot, EulerAngles& newRot );
	virtual void SetScaleFromPreview( const Vector& prevScale, Vector& newScale );

	virtual IPreviewItemContainer* GetItemContainer() const;

	virtual void DrawGizmo( CRenderFrame* frame );

protected:
	CEdEntityPreviewPanel*	m_preview;
	CName					m_woundName;
};
