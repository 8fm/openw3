
#pragma once
#include "extAnimDurationEvent.h"
#include "../core/curveData.h"

class CExtAnimMorphEvent	: public CExtAnimDurationEvent
							, public ICurveDataOwner
{
	DECLARE_RTTI_SIMPLE_CLASS( CExtAnimMorphEvent )

	CName			m_morphComponentId;
	Bool			m_invertWeight;

	Bool			m_useCurve;
	SCurveData		m_curve;

public:
	CExtAnimMorphEvent();
	CExtAnimMorphEvent( const CName& eventName, const CName& animationName, Float startTime, Float duration, const String& trackName );

	virtual void Start( const CAnimationEventFired& info, CAnimatedComponent* component ) const override;
	virtual void Process( const CAnimationEventFired& info, CAnimatedComponent* component ) const override;
	virtual void Stop( const CAnimationEventFired& info, CAnimatedComponent* component ) const override;

public: // ICurveDataOwner
	virtual TDynArray< SCurveData* >* GetCurvesData() { return nullptr; }
	virtual SCurveData* GetCurveData() { return &m_curve; }

private:
	void SetMorphWeight( Float w, CAnimatedComponent* component ) const;
};

BEGIN_CLASS_RTTI( CExtAnimMorphEvent );
	PARENT_CLASS( CExtAnimDurationEvent );
	PROPERTY_EDIT( m_morphComponentId, TXT("") );
	PROPERTY_EDIT( m_invertWeight, TXT("") );
	PROPERTY_EDIT( m_useCurve, String::EMPTY );
	PROPERTY_CUSTOM_EDIT( m_curve, String::EMPTY, TXT("BaseCurveDataEditor") );
END_CLASS_RTTI();
