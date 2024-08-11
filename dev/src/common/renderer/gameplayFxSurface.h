
#pragma once

#include "build.h"

#include "gameplayFx.h"
#include "renderTextureArray.h"

class CSurfacePostFX : public IGameplayEffect
#ifndef NO_EDITOR_EVENT_SYSTEM
	, public IEdEventListener
#endif
{
private:
	Float							m_timeSinceStart;		
	Vector							m_fillColor;

	Vector							m_playerPosition;

	TDynArray< SSurfacePostFXGroup >	m_groups;
	Uint32								m_activeGroups;


public:
	CSurfacePostFX( CGameplayEffects* parent, const Vector &fillColor = Vector::ZEROS );

	~CSurfacePostFX();
	
	//-------------------------------------------
	// Common FX methods

	virtual void Init( );

	virtual void Shutdown();

	virtual Bool Apply( CRenderCollector &collector, const CRenderFrameInfo &frameInfo, ERenderTargetName rtnColor, ERenderTargetName rtnTarget );
		
	virtual void Tick( Float time );	

	//-------------------------------------------

	void Add( const Vector& position, Float fadeInTime, Float fadeOutTime, Float activeTime, Float range, Uint32 type );

	static Uint8 GetUsedStencilBits();

	void SetFillColor( const Vector& fillColor );

private:
	GpuApi::BufferRef										m_cBuf;
	TStaticArray<Vector, SURFACE_POST_GROUPS_MAX >			m_cBufPtr;
	void UpdateBuffer();

public:
#ifndef NO_EDITOR_EVENT_SYSTEM
	virtual void DispatchEditorEvent( const CName& name, IEdEventData* data );
#endif

};