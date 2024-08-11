#pragma once



class CItemPartDefinitionComponent;

class CItemPartSlotDefinition : public CObject
{
	DECLARE_ENGINE_CLASS( CItemPartSlotDefinition, CObject, 0 );

private:
	CName						m_attachmentName;
	CName						m_slotName;
	THandle< CComponent >		m_pluggedPart;
	THandle< CEntityTemplate >	m_partToPlug;
	//Bool						m_useParentSlot;
public:

	RED_INLINE CName	GetSlotName(){ return m_slotName; }
	RED_INLINE CName	GetAttachmentName(){ return m_attachmentName ? m_attachmentName : m_slotName; }
	RED_INLINE Bool IsEmpty( ){ return m_pluggedPart.Get()==NULL; }
	RED_INLINE const THandle< CEntityTemplate >& GetEntityToPlug() const { return m_partToPlug; }
	RED_INLINE void SetEntityToPlug( const THandle< CEntityTemplate >& ent ){ m_partToPlug = ent; }

	//RED_INLINE Bool GetUseParentSlot(){ return m_useParentSlot; }
	void SetPluggedPart( CItemPartDefinitionComponent* pluggedPart );	
	CItemPartDefinitionComponent* GetPluggenPart();	
};

BEGIN_CLASS_RTTI( CItemPartSlotDefinition )
	PARENT_CLASS( CObject );
	PROPERTY_EDIT( m_slotName, TXT("Slot name") );
	PROPERTY_EDIT( m_attachmentName, TXT("Attachment name") );
	PROPERTY_EDIT( m_partToPlug, TXT("Part to plug") );
	PROPERTY( m_pluggedPart );
END_CLASS_RTTI();