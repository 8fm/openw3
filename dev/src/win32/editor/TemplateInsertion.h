#pragma once

////////////////////////////////////////////////////
// CTemplateInsertion
class CTemplateInsertion 
{
	DECLARE_RTTI_SIMPLE_CLASS( CTemplateInsertion );
public:
	CTemplateInsertion(){}
	virtual void OnTemplateInserted( const TemplateInfo *const templateInfo, const Vector& clickPosition, CLayer* layer, CResource* baseResource ) {}
protected:
	CEntity* CreateEntity( const String& templateName, CLayer* layer, const Vector& spawnPosition, CResource* baseResource, Bool detachFromTemplate )const;
};
BEGIN_CLASS_RTTI( CTemplateInsertion );
END_CLASS_RTTI();

////////////////////////////////////////////////////
// CTemplateInsertion
class CSwarmTemplateInsertion  : public CTemplateInsertion
{
	DECLARE_RTTI_SIMPLE_CLASS( CSwarmTemplateInsertion );
public:
	CSwarmTemplateInsertion(){}
	void OnTemplateInserted( const TemplateInfo *const templateInfo, const Vector& clickPosition, CLayer* layer, CResource* baseResource ) override;
private:
	void AddSwarmToLayer(	const String & speciesName, CEntity *const lairEntity,
							Float lairScale, Float lairHeight, Float triggerEntityZ, Float poiEntityZ,
							const TemplateInfo *const templateInfo, const Vector& clickPosition, CLayer* layer, CResource* baseResource );
};
BEGIN_CLASS_RTTI( CSwarmTemplateInsertion );
	PARENT_CLASS( CTemplateInsertion );
END_CLASS_RTTI();