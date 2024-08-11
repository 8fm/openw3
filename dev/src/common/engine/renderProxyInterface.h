/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

class IRenderProxy;

/// Render Proxy Interface
class IRenderProxyInterface
{
public:
	virtual Uint32			GetNumberOfRenderProxies() const = 0;
	virtual IRenderProxy*	GetRenderProxy( Uint32 i ) = 0;

	virtual void SetProxiesVisible( Bool flag ) = 0;

	virtual void RefreshRenderProxies() = 0;
};
