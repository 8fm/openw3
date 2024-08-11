#pragma once

#include "aiStorage.h"

class CBehTreeCounterData
{
	DECLARE_RTTI_SIMPLE_CLASS( CBehTreeCounterData );
protected:
	Int32 m_counter;
public:
	CBehTreeCounterData()					
		: m_counter(0)
	{}

	Int32		GetCounter() const												{ return m_counter; }
	void		SetCounter( Int32 val )											{ m_counter = val; }
	void		ModifyCounter( Int32 delta )									{ m_counter += delta; }
	void		ModifyCounterNoNegative( Int32 delta )							{ m_counter = Max( 0, m_counter + delta ); }
	void		Reset( )														{ m_counter = 0; }

	class CInitializer : public CAIStorageItem::CNamedInitializer
	{
	public:
		CInitializer( CName name )
			: CAIStorageItem::CNamedInitializer( name ) {}
		void InitializeItem( CAIStorageItem& item ) const override;
		IRTTIType* GetItemType() const override;
	};
};

BEGIN_NODEFAULT_CLASS_RTTI( CBehTreeCounterData );
END_CLASS_RTTI();

class CBehTreeCounterDataPtr : public TAIStoragePtr< CBehTreeCounterData >
{
	typedef TAIStoragePtr< CBehTreeCounterData > Super;
public:
	CBehTreeCounterDataPtr( CAIStorage* storage, CName name );

	CBehTreeCounterDataPtr()
		: Super()																{}
	CBehTreeCounterDataPtr( const CBehTreeCounterDataPtr& p )
		: Super( p )															{}

	CBehTreeCounterDataPtr( CBehTreeCounterDataPtr&& p )
		: Super( Move( p ) )													{}
};