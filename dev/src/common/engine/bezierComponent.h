
#pragma once

#include "IBezierOwner.h"
#include "skeletalAnimatedComponent.h"
#include "component.h"

class CBezierComponent : public CComponent, public IBezierOwner2
{
	DECLARE_ENGINE_CLASS( CBezierComponent, CComponent, 0 );

private:
	QuadraticBezierNamespace::Curve2*		m_curve;
	Color									m_color;
public:
	CBezierComponent();
	~CBezierComponent();

	virtual void OnAttached( CWorld* world );
	virtual void OnDetached( CWorld *world );
	virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flags );
	virtual void OnSerialize( IFile& file );
	virtual void OnSpawned( const SComponentSpawnInfo& spawnInfo );
public:
	QuadraticBezierNamespace::Curve2* GetBezier() { return m_curve; }
};

BEGIN_CLASS_RTTI( CBezierComponent )
	PARENT_CLASS( CComponent );
END_CLASS_RTTI();



class CBezierMovableComponent : public CSkeletalAnimatedComponent, public IBezierOwner2
{
	DECLARE_ENGINE_CLASS( CBezierMovableComponent, CSkeletalAnimatedComponent, 0 );

protected:
	Float									m_speed;

private:
	QuadraticBezierNamespace::Curve2*		m_curve;
	Color									m_color;
	Float									m_positionOnCurve;
	Matrix									FinalMatrix;

public:
	CBezierMovableComponent();
	~CBezierMovableComponent();

	virtual void OnAttached( CWorld* world );
	virtual void OnDetached( CWorld *world );
	virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flags );
	virtual void OnSerialize( IFile& file );
	virtual void OnSpawned( const SComponentSpawnInfo& spawnInfo );
	virtual void UpdateAsync( Float timeDelta );
	virtual void CalcBox( Box& box ) const;
public:
	QuadraticBezierNamespace::Curve2* GetBezier() { return m_curve; }
};

BEGIN_CLASS_RTTI( CBezierMovableComponent )
	PARENT_CLASS( CSkeletalAnimatedComponent );
	PROPERTY_EDIT( m_speed, TXT("Speed") );
END_CLASS_RTTI();