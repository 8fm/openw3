/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "../core/mathUtils.h"
#include "../core/refCountPointer.h"


/// Env probe buffer type
enum eEnvProbeBufferTexture
{
	ENVPROBEBUFFERTEX_Albedo,
	ENVPROBEBUFFERTEX_Normals,
	ENVPROBEBUFFERTEX_GlobalShadow,
	ENVPROBEBUFFERTEX_Depth,

	ENVPROBEBUFFERTEX_MAX
};

/// IEnvProbeDataSource
class IEnvProbeDataSource
{
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_SmallObjects, MC_EnvProbeData );

private:
	typedef Red::Threads::CAtomic< Int32 > RefCount;

public:
	typedef	GpuApi::TextureRef					tFaceTexturesTable[ENVPROBEBUFFERTEX_MAX];
	typedef Red::Threads::CMutex				tLock;
	typedef Red::Threads::CScopedLock< tLock >	tScopedLock;
	enum EState
	{
		STATE_NotLoading,
		STATE_InProgress,
		STATE_Loaded,
		STATE_AllocFailed,
		STATE_Failed,
	};

private:
	RefCount						m_refCount;

public:
	IEnvProbeDataSource ()
		: m_refCount ( 0 )					{}
	virtual ~IEnvProbeDataSource ()			{}

	static tLock& GetCommunicationLock();

	void			AddRef()		{ m_refCount.Increment(); }
	void			Release()		{ if ( m_refCount.Decrement() == 0 ) { delete this; } }

	virtual Bool	IsLoadable() = 0;
	virtual	void	StartLoading( tFaceTexturesTable *faceTextures ) = 0;
	virtual void	RequestFastLoadingFinish() = 0;
	virtual	void	CancelLoading() = 0;
	virtual void	Invalidate() = 0;
	virtual EState	GetState() = 0;
	virtual void	Rewind() = 0;
};

typedef TRefCountPointer< IEnvProbeDataSource > EnvProbeDataSourcePtr;

/// Env probe gen params
class SEnvProbeGenParams
{
	DECLARE_RTTI_STRUCT( SEnvProbeGenParams );

public:
	Bool  m_useInInterior;
	Bool  m_useInExterior;
	Bool  m_isInteriorFallback;
	Float m_cullingDistance;
	Color m_ambientColor;
	Float m_ambientIntensity;
	Float m_dimmerFactor;
	Float m_fadeInDuration;
	Float m_fadeOutDuration;
	Float m_lightScaleGlobal;
	Float m_lightScaleLocals;
	Float m_fogAmount;
	SSimpleCurve m_daycycleAmbientIntensity;
	SSimpleCurve m_daycycleLightScaleLocals;
	SSimpleCurve m_daycycleEffectIntensity;

public:
	SEnvProbeGenParams ();

	Vector GetAmbientColorLinear( Float worldTime ) const;
	Float GetLightScaleLocals( Float worldTime ) const;
};

BEGIN_CLASS_RTTI( SEnvProbeGenParams )
PROPERTY_EDIT(m_useInInterior,				TXT("Use in interior") );
PROPERTY_EDIT(m_useInExterior,				TXT("Use in exterior") );
PROPERTY_EDIT(m_isInteriorFallback,			TXT("Is interior fallback") );
PROPERTY_EDIT(m_cullingDistance,			TXT("Culling distance") );
PROPERTY_EDIT(m_ambientColor,				TXT("Ambient color") );
PROPERTY_EDIT(m_ambientIntensity,			TXT("Ambient intensity") );
PROPERTY_EDIT(m_dimmerFactor,				TXT("Dimmer factor") );
PROPERTY_EDIT(m_fadeInDuration,				TXT("Fade in duration") );
PROPERTY_EDIT(m_fadeOutDuration,			TXT("Fade out duration") );
PROPERTY_EDIT(m_lightScaleGlobal,			TXT("Global light scale") );
PROPERTY_EDIT(m_lightScaleLocals,			TXT("Local lights scale") );
PROPERTY_EDIT(m_fogAmount,					TXT("Fog amount") );
PROPERTY_EDIT(m_daycycleAmbientIntensity,	TXT("Daycycle ambient intensity") );
PROPERTY_EDIT(m_daycycleLightScaleLocals,	TXT("Daycycle light scale locals") );
PROPERTY_EDIT(m_daycycleEffectIntensity,	TXT("Daycycle effect intensity") );
END_CLASS_RTTI();


/// RenderEnvProbe params
struct SEnvProbeParams
{
	SEnvProbeParams ();
	SEnvProbeParams ( Uint32 debugId, Int32 nestingLevel, Float effectIntensity, const Vector &probeGenOrigin, const Vector &probeOrigin, const Matrix &areaLocalToWorld, const Matrix &parallaxLocalToWorld, Float contribution, const Vector &areaMarginFactor, const SEnvProbeGenParams &genParams );

	Float SquaredDistance( const Vector &point ) const;
	Bool IsGlobalProbe() const;
	Float GetEffectIntensity( Float worldTime ) const;

	Uint32 m_debugId;
	Int32 m_nestingLevel;
	Float m_effectIntensity;
	Vector m_probeGenOrigin;
	Vector m_probeOrigin;
	Matrix m_areaLocalToWorld;
	Matrix m_parallaxLocalToWorld;
	Float m_contribution;
	Vector m_areaMarginFactor;
	SEnvProbeGenParams m_genParams;
};