/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "aiStorage.h"


class CBehTreeGuardAreaData
{
	DECLARE_RTTI_SIMPLE_CLASS( CBehTreeGuardAreaData );
protected:
	THandle< CAreaComponent >					m_baseGuardArea;
	THandle< CAreaComponent >					m_basePursuitArea;

	THandle< CAreaComponent >					m_guardArea;
	THandle< CAreaComponent >					m_pursuitArea;

	THandle< CActor >							m_lastTargetNoticedAtGuardArea;

	Float										m_basePursuitRange;
	Float										m_pursuitRange;

	Bool										m_isInImmediateState;
	Bool										m_isRetreating;

public:
	CBehTreeGuardAreaData()
		: m_basePursuitRange( 15.f )
		, m_pursuitRange( 15.f )
		, m_isInImmediateState( false )
		, m_isRetreating( false )													{}
	~CBehTreeGuardAreaData()														{}

	Bool						IsInPursueArea( const Vector& pos, CAreaComponent* guardArea );		// NOTICE: paremetr 'guardArea' looks like overkill but its important to handle situation when guardArea == nullptr on top level, which wouldn't be such obvious otherwise
	static Bool					IsInGuardArea( const Vector& pos, CAreaComponent* guardArea );

	CAreaComponent*				GetGuardArea() const								{ return m_guardArea.Get(); }
	void						SetGuardArea( CAreaComponent* areComponent )		{ m_guardArea = areComponent; }
	CAreaComponent*				GetPursuitArea() const								{ return m_pursuitArea.Get(); }
	Float						GetPursuitRange() const								{ return m_pursuitRange; }

	Bool						HasNoticedTargetAtGuardArea( CActor* actor ) const	{ return m_lastTargetNoticedAtGuardArea.Get() == actor; }
	void						NoticeTargetAtGuardArea( CActor* actor )			{ m_lastTargetNoticedAtGuardArea = actor; }
	void						ClearTargetNoticedAtGuardArea()						{ m_lastTargetNoticedAtGuardArea = nullptr; }

	void						SetupBaseState( CAreaComponent* guardArea, CAreaComponent* pursuitArea, Float pursuitRange );
	void						SetupImmediateState( CAreaComponent* guardArea, CAreaComponent* pursuitArea, Float pursuitRange );

	void						SetupBaseState( CAreaComponent* guardArea, CAreaComponent* pursuitArea )								{ SetupBaseState( guardArea, pursuitArea, m_pursuitRange ); }
	void						SetupImmediateState( CAreaComponent* guardArea, CAreaComponent* pursuitArea )							{ SetupImmediateState( guardArea, pursuitArea, m_pursuitRange ); }

	void						ResetImmediateState();

	void						SetIsRetreating( Bool b )							{ m_isRetreating = b; }
	Bool						IsRetreating() const								{ return m_isRetreating; }

	static CBehTreeGuardAreaData* Find( CAIStorage* storage );

	struct CInitializer : public CAIStorageItem::CInitializer
	{
		CName					GetItemName() const override;
		void					InitializeItem( CAIStorageItem& item ) const override;
		IRTTIType*				GetItemType() const override;
	};
};

BEGIN_NODEFAULT_CLASS_RTTI( CBehTreeGuardAreaData )
END_CLASS_RTTI()


class CBehTreeGuardAreaDataPtr : public TAIStoragePtr< CBehTreeGuardAreaData >
{
	typedef TAIStoragePtr< CBehTreeGuardAreaData > Super;
public:
	CBehTreeGuardAreaDataPtr( CAIStorage* storage )
		: Super( CBehTreeGuardAreaData::CInitializer(), storage )					{}

	CBehTreeGuardAreaDataPtr()
		: Super()																	{}

	void operator=( CBehTreeGuardAreaDataPtr&& g )									{ Super::operator=( Move( g ) ); }
	void operator=( const CBehTreeGuardAreaDataPtr& g )								{ Super::operator=( g ); }
};