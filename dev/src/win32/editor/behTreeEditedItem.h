#pragma once

class CBehTree;

////////////////////////////////////////////////////////////////////////////////
// abstract class for objects edited via beh tree editor
class IBehTreeEditedItem
{
public:
	virtual ~IBehTreeEditedItem()												{}
	////////////////////////////////////////////////////////////////////////////
	virtual void MarkModified() = 0;
	virtual void Save() = 0;
	virtual void SetRootNode( IBehTreeNodeDefinition* node ) = 0;
	virtual IBehTreeNodeDefinition* GetRootNode() = 0;
	virtual Bool CanModify() = 0;
	virtual void GetName( wxString& outName, Bool shortName ) = 0;
	virtual void OnResourceReload( CResource* res ) = 0;
	virtual CBehTree* GetRes();
	virtual void SetRes( CBehTree* res );
	virtual Bool CheckOut();
	virtual CObject* GetParentObject() = 0;
};

////////////////////////////////////////////////////////////////////////////////

class CBehTreeEditedResource : public IBehTreeEditedItem
{
protected:
	CBehTree*				m_res;
public:
	CBehTreeEditedResource( CBehTree* res );
	~CBehTreeEditedResource();

	void MarkModified() override;
	void Save() override;
	void SetRootNode( IBehTreeNodeDefinition* node ) override;
	IBehTreeNodeDefinition* GetRootNode() override;
	Bool CanModify() override;;
	void GetName( wxString& outName, Bool shortName ) override;
	void OnResourceReload( CResource* res ) override;
	CBehTree* GetRes() override;
	void SetRes( CBehTree* res ) override;
	Bool CheckOut() override;
	CObject* GetParentObject() override;
};
////////////////////////////////////////////////////////////////////////////////
class CBehTreeEditedProperty : public IBehTreeEditedItem
{
protected:
	THandle< CObject >									m_object;
	CProperty*											m_property;
	THandle< IBehTreeNodeDefinition >					m_rootNode;
	wxString											m_name;

	void RefreshRootNode();
public:
	CBehTreeEditedProperty( CObject* parentObject, CProperty* prop );
	~CBehTreeEditedProperty();

	void MarkModified() override;
	void Save() override;
	void SetRootNode( IBehTreeNodeDefinition* node ) override;
	IBehTreeNodeDefinition* GetRootNode() override;
	Bool CanModify() override;
	void GetName( wxString& outName, Bool shortName ) override;
	void OnResourceReload( CResource* res ) override;
	CObject* GetParentObject() override;
};

