/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "soundPropertyEditors.h"
#include "listboxeditor.h"

//#define USE_COLLISION_GROUP_RESOURCE_EDITOR

class CEdPhysicalCollisionTypeSelector : public CListBoxEditor, public IEdEventListener
{
private:

#ifdef USE_COLLISION_GROUP_RESOURCE_EDITOR
	class CEd2dArrayEditor* m_collisionGroupResourceEditor;
#endif

public:
	CEdPhysicalCollisionTypeSelector( CPropertyItem* item );
	virtual ~CEdPhysicalCollisionTypeSelector(void);

#ifdef USE_COLLISION_GROUP_RESOURCE_EDITOR
	virtual void CreateControls( const wxRect &propertyRect, TDynArray< wxControl* >& outSpawnedControls ) override;
	virtual void CloseControls() override;
	void UpdateArrayEditorColours();
#endif

	virtual Bool GrabValue( String& displayValue ) override;

	virtual wxArrayString GetListElements();
	virtual void SelectPropertyElements();

	virtual void SelectElement( wxString element );
	virtual void DeselectElement( wxString element );


	void DispatchEditorEvent( const CName& name, IEdEventData* data );


#ifdef USE_COLLISION_GROUP_RESOURCE_EDITOR
private:
	void On2dArrayEditorClose( wxWindowDestroyEvent& event );
#endif

};

class CEdPhysicalCollisionGroupSelector : public CListBoxEditor, public IEdEventListener
{
private:

public:
	CEdPhysicalCollisionGroupSelector( CPropertyItem* item );
	virtual ~CEdPhysicalCollisionGroupSelector(void);

	virtual Bool GrabValue( String& displayValue ) override;

	virtual wxArrayString GetListElements();
	virtual void SelectPropertyElements();

	virtual void SelectElement( wxString element );
	virtual void DeselectElement( wxString element );

	void DispatchEditorEvent( const CName& name, IEdEventData* data );

};