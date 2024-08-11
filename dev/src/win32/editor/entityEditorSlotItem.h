/**
* Copyright © 2011 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "entityPreviewPanel.h"

class CEntityEditorSlotItem : public IPreviewItem
{
public:
	CEntityEditorSlotItem( CEdEntityPreviewPanel* editorPreview, const CName& slotName );

	virtual Bool IsValid() const;

	virtual void Refresh();

	virtual void SetPositionFromPreview( const Vector& prevPos, Vector& newPos );
	virtual void SetRotationFromPreview( const EulerAngles& prevRot, EulerAngles& newRot );

	virtual IPreviewItemContainer* GetItemContainer() const;

protected:
	CEdEntityPreviewPanel*	m_preview;
	CName					m_slotName;
};
