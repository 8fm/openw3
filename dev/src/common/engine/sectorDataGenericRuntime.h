/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "collisionCache.h"
#include "sectorData.h"
#include "sectorDataObjects.h"

/// Sector streaming runtime for mesh stuff
class CSectorDataObjectMesh : public TSectorDataObject< SectorData::PackedMesh >
{
public:
	CSectorDataObjectMesh();
	virtual EResult Stream( const Context& context, const Bool asyncPipeline ) override;
	virtual EResult Unstream( const Context& context, const Bool asyncPipeline ) override;

private:
	class IRenderProxy*			m_proxy;
	CSectorDataResourceRef		m_resourceToUse;
};

/// Sector streaming runtime for collision object
class CSectorDataObjectCollision : public TSectorDataObject< SectorData::PackedCollision >
{
public:
	CSectorDataObjectCollision();
	virtual ~CSectorDataObjectCollision();
	virtual EResult Stream( const Context& context, const Bool asyncPipeline ) override;
	virtual EResult Unstream( const Context& context, const Bool asyncPipeline ) override;

private:
	Int32						m_physicsBodyIndex;
	CompiledCollisionPtr		m_collisionToUse;
	CSectorDataResourceRef		m_resourceToUse;

	Uint32						m_invalidAreaID;
	class CPhysicsWorld*		m_invalidAreaPhysicsWorld;

	void CreateInvalidAreaIfNotThere( const Context& context );
	void ReleaseInvalidArea();

	class CPhysicsTileWrapper* CreatePhysicsTileForPos( const Context& context, const Vector2& pos ) const;
};

/// Sector streaming runtime for decal
class CSectorDataObjectDecal : public TSectorDataObject< SectorData::PackedDecal >
{
public:
	CSectorDataObjectDecal();
	virtual EResult Stream( const Context& context, const Bool asyncPipeline ) override;
	virtual EResult Unstream( const Context& context, const Bool asyncPipeline ) override;

private:
	class IRenderProxy*			m_proxy;
	CSectorDataResourceRef		m_resourceToUse;
};

/// Sector streaming runtime for dimmer
class CSectorDataObjectDimmer : public TSectorDataObject< SectorData::PackedDimmer >
{
public:
	CSectorDataObjectDimmer();
	virtual EResult Stream( const Context& context, const Bool asyncPipeline ) override;
	virtual EResult Unstream( const Context& context, const Bool asyncPipeline ) override;

private:
	class IRenderProxy*			m_proxy;
};

/// Sector streaming runtime for point light
class CSectorDataObjectPointLight : public TSectorDataObject< SectorData::PackedLight >
{
public:
	CSectorDataObjectPointLight();
	virtual EResult Stream( const Context& context, const Bool asyncPipeline ) override;
	virtual EResult Unstream( const Context& context, const Bool asyncPipeline ) override;

private:
	class IRenderProxy*			m_proxy;
};

/// Sector streaming runtime for spot light
class CSectorDataObjectSpotLight : public TSectorDataObject< SectorData::PackedSpotLight >
{
public:
	CSectorDataObjectSpotLight();
	virtual EResult Stream( const Context& context, const Bool asyncPipeline ) override;
	virtual EResult Unstream( const Context& context, const Bool asyncPipeline ) override;

private:
	class IRenderProxy*			m_proxy;
	CSectorDataResourceRef		m_resourceToUse;
};
