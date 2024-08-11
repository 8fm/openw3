
#pragma once

class CEdComponentProperties : public CEdPropertiesPage
{
protected:
	THandle<CComponent> m_component;

public:
	RED_INLINE const CComponent* GetComponent() const { return m_component.GetConst(); }

	RED_INLINE void SetComponent( const CComponent* c ) { m_component = c; }
	RED_INLINE void ResetComponent() { m_component = nullptr; }

public:
	CEdComponentProperties( wxWindow* parent, CEdUndoManager* undoManager )
		: CEdPropertiesPage( parent, PropertiesPageSettings(), undoManager )
		, m_component( nullptr )
	{};

	virtual class CEdComponentProperties* QueryComponentProperties()
	{
		return this;
	}
};
