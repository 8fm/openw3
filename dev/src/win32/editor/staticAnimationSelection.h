
#pragma once

class CEdStaticAnimationSelection : public CListSelection
{
public:
	CEdStaticAnimationSelection( CPropertyItem* item );

	virtual void CreateControls( const wxRect &propRect, TDynArray< wxControl* >& outSpawnedControls ) override;

	virtual const CSkeletalAnimationSet* GetAnimationSet() const = 0;
};

class CEdGameplayMimicSelection : public CEdStaticAnimationSelection
{
public:
	CEdGameplayMimicSelection( CPropertyItem* item );

	virtual const CSkeletalAnimationSet* GetAnimationSet() const;
};