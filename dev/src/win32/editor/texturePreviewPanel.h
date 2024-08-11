/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "materialPreviewPanel.h"

class CEdTextureViewer;

/// Preview panel that renders material preview
class CEdTexturePreviewPanel : public CEdMaterialPreviewPanel
{
protected:
	IMaterial*			m_diffuseMaterial;
	IMaterial*			m_diffuseCubeMaterial;
	IMaterial*			m_normalMaterial;

	Int32				m_mipLevel;

public:
	CEdTexturePreviewPanel( wxWindow* parent, CResource* texture, Float lodBias );

	void SetTexture( CResource* texture, Float lodBias );

	virtual void SetMaterial( IMaterial* material ) override;

protected:
	virtual void OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame );
};