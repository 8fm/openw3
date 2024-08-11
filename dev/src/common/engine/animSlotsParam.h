#pragma once

#include "entityTemplateParams.h"

class CAnimationSlots : public CObject
{
	DECLARE_ENGINE_CLASS( CAnimationSlots, CObject, 0 );

public:
	CAnimationSlots() {};

	TDynArray< Matrix >	m_transforms;

	CName			m_name;
};

BEGIN_CLASS_RTTI( CAnimationSlots );
	PARENT_CLASS( CObject );
	PROPERTY_RO( m_name, TXT("Name") );
	PROPERTY_EDIT( m_transforms, TXT("Slave Transforms") );
END_CLASS_RTTI();

class CAnimSlotsParam : public CEntityTemplateParam
{
	DECLARE_ENGINE_CLASS( CAnimSlotsParam, CEntityTemplateParam, 0 );

protected:
	TDynArray< CAnimationSlots* > m_animationSlots;

public:
	CAnimSlotsParam() {};

	CAnimationSlots*		AddAnimationSlots( const CName& animSlotsName );
	Bool					RemoveAnimationSlots( const CName& animSlotsName );
	Bool					HasAnimSlots( const CName& animSlotsName ) const;
	const CAnimationSlots*	FindAnimationSlots( const CName& animSlotsName ) const;
	RED_INLINE Bool		IsEmpty() const { return m_animationSlots.Empty(); }

	RED_INLINE const TDynArray< CAnimationSlots* >& GetAnimationSlots() const { return m_animationSlots; }

};

BEGIN_CLASS_RTTI( CAnimSlotsParam );
	PARENT_CLASS( CEntityTemplateParam );
	PROPERTY_EDIT( m_animationSlots, TXT("Animation Slots") );
END_CLASS_RTTI();