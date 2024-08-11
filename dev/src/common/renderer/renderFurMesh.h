/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "renderDynamicResource.h"
#include "renderResourceIterator.h"


class CFurMeshResource;

#ifdef USE_NVIDIA_FUR

#include "../../../external/NvidiaHair/include/GFSDK_HairWorks.h"


class CRenderFurMesh_Hairworks : public IDynamicRenderResource, public TRenderResourceListWithCache< CRenderFurMesh_Hairworks >
{
	DECLARE_RENDER_RESOURCE_ITERATOR;

private:
	GFSDK_HairAssetID				m_assetID;

public:
	CRenderFurMesh_Hairworks();
	~CRenderFurMesh_Hairworks();


	RED_INLINE GFSDK_HairAssetID GetAssetID() const { return m_assetID; }


	virtual CName GetCategory() const override;

	virtual String GetDisplayableName() const override;

	virtual void OnDeviceLost() override;
	virtual void OnDeviceReset() override;


	static CRenderFurMesh_Hairworks* Create( const CFurMeshResource* furResource, Uint64 partialRegistrationHash );
};

#endif
