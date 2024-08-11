/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RESOURCE_USAGE_INFO

#include "names.h"
#include "math.h"

// Resource spatial usage collector
// This is in core but is closely tied with the CLayer/CEntity/CComponent system
class IResourceUsageCollector
{
public:
	virtual ~IResourceUsageCollector() {};

	// push/pop layer
	virtual void PushLayer( const String& layerName ) = 0;
	virtual void PopLayer() = 0;

	// push/pop entity
	virtual void PushEntity( const String& entityName, const CName entityClass, const Vector& position ) = 0;
	virtual void PopEntity() = 0;

	// push/pop component
	virtual void PushComponent( const String& componentName, const CName componentClass, const Vector& position ) = 0;
	virtual void PopComponent() = 0;

	// report flags
	virtual void ReportLayerFlag( const CName flagName, const Bool flagValue ) = 0;
	virtual void ReportEntityFlag( const CName flagName, const Bool flagValue ) = 0;
	virtual void ReportComponentFlag( const CName flagName, const Bool flagValue ) = 0;

	// report bounding box for current component
	virtual void ReportBoundingBox( const Box& box ) = 0;

	// report template resource (entity template)
	virtual void ReportTemplate( const String& templatePath ) = 0;

	// report visibility distance for current component
	virtual void ReportVisibilityDistance( const Float minRange=0.0f, const Float maxRange=FLT_MAX ) = 0;

	// report resource usage, with optional visibility range
	virtual void ReportResourceUsage( class CResource* res ) = 0;
	virtual void ReportResourceUsage( const String& depotPath ) = 0;
};

#endif // NO_RESOURCE_USAGE_INFO
