/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class IRenderResource;

struct SSkyboxSetupParameters
{
	SSkyboxSetupParameters ();
	~SSkyboxSetupParameters ();

	void AddRefAll();
	void ReleaseAll();

	IRenderResource *m_sunMeshResource;
	IRenderResource *m_sunMaterialResource;
	IRenderResource *m_sunMaterialParamsResource;
	IRenderResource *m_moonMeshResource;
	IRenderResource *m_moonMaterialResource;
	IRenderResource *m_moonMaterialParamsResource;
	IRenderResource *m_skyboxMeshResource;
	IRenderResource *m_skyboxMaterialResource;
	IRenderResource *m_skyboxMaterialParamsResource;
	IRenderResource *m_cloudsMeshResource;
	IRenderResource *m_cloudsMaterialResource;
	IRenderResource *m_cloudsMaterialParamsResource;
};