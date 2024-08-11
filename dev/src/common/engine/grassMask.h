/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "../core/resource.h"

class CGenericGrassMask : public CResource
{
	DECLARE_ENGINE_RESOURCE_CLASS( CGenericGrassMask, CResource, "grassmask", "Generic Grass Mask" );

protected:
	Uint8*	m_grassMask;
	Uint32	m_maskRes;

public:
	CGenericGrassMask();
	virtual ~CGenericGrassMask();

	// Create mask data
	void InitGenericGrassMask( Int32 terrainRes );

	// Object serialization interface
	virtual void OnSerialize( IFile& file );

	// Getters
	Uint8* GetGrassMask() const { return m_grassMask; }
	Uint32 GetGrassMaskRes() const { return m_maskRes; }
};

BEGIN_CLASS_RTTI( CGenericGrassMask );
PARENT_CLASS( CResource );
PROPERTY( m_maskRes );
END_CLASS_RTTI();