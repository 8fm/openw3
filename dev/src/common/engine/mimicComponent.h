
#pragma once

#include "mimicFac.h"
#include "animatedComponent.h"


#define USE_GMPL_MIMICS
//#define DEBUG_MIMICS

enum EMimicLOD
{
	EML_Lod_Unknown,
	EML_Lod0_MimicHigh,
	EML_Lod1_MimicLow,
	EML_Lod2_NeckHead,
};

class CMimicFace;

class CNormalBlendComponent;
class CAnimMimicParam;
struct SMimicPostProcessData;

class CMimicComponent : public CAnimatedComponent
{
	DECLARE_ENGINE_CLASS( CMimicComponent, CAnimatedComponent, 0 );

protected:
	const static Float		LOD_1_TO_2_DIST2;
	const static Float		LOD_2_TO_1_DIST2;

	const static Uint32		LOD_1_NUM_BONES = 30;
	const static Uint32		LOD_2_NUM_BONES = 6;

protected:
	THandle< CMimicFace >	m_mimicFace;				// Main mimics.
	THandle< CMimicFace >	m_categoryMimics;			// Additional, category mimics. Only for poses and filters.
	CName					m_attachmentSlotName;

protected:
	EMimicLOD			m_mimicLod;
	EMimicLOD			m_requestedMimicLod;

	Int32				m_cachedHeadIdx;
	Int32				m_cachedNeckIdx;

	TDynArray< CNormalBlendComponent* > m_cachedNBComponents;

	Bool				m_firstUpdate;						// We need to have it because of crazy appearance system
	Bool				m_requestRefreshAllMeshedWithLod0;	// We need to have it because of crazy appearance system

protected:
	Bool				m_tempCanUseLod;
	
public:
	CMimicComponent();

	virtual void OnInitialized() override;
	virtual void OnAttached( CWorld* world ) override;
	virtual void OnDetached( CWorld *world ) override;
	virtual void OnAppearanceChanged( Bool added ) override;
	virtual void OnStreamIn() override;

	virtual Bool ShouldAddToTickGroups() const override;
	virtual Bool UsesAutoUpdateTransform() { return true; }

protected:
	virtual void OnPostInitializationDone() override;

	virtual void OnUpdateTransformComponent( SUpdateTransformContext& context, const Matrix& prevLocalToWorld ) override;

public: // Controls
	Bool MimicHighOn();
	void MimicHighOff();
	Bool HasMimicHigh() const;

public: // Resources
	virtual CSkeleton* GetSkeleton() const override;
	virtual CSkeleton* GetMimicSkeleton() const override;
	
	Int32 GetLodBoneNum() const override;

	RED_INLINE const CMimicFace* GetMimicFace() const	{ return m_mimicFace.Get(); }
	CExtendedMimics GetExtendedMimics() const;

	RED_INLINE const CName& GetAttachmentSlotName() const { return m_attachmentSlotName; }

public: // Callbacks from animation system
	virtual void OnParentUpdatedAttachedAnimatedObjectsLS( Float dt, SBehaviorGraphOutput* poseLS, const BoneMappingContainer& bones ) override;
	virtual void OnParentUpdatedAttachedAnimatedObjectsLSAsync( Float dt, SBehaviorGraphOutput* poseLS, const BoneMappingContainer& bones ) override;
	virtual void OnParentUpdatedAttachedAnimatedObjectsLSSync( Float dt, SBehaviorGraphOutput* poseLS, const BoneMappingContainer& bones ) override;
	virtual void OnParentUpdatedAttachedAnimatedObjectsLSConstrainted( Float dt, SBehaviorGraphOutput* poseLS, const BoneMappingContainer& bones ) override;
	virtual void OnParentUpdatedAttachedAnimatedObjectsWS( const CAnimatedComponent* parent, SBehaviorGraphOutput* poseLS, TDynArray< Matrix >* poseMS, TDynArray< Matrix >* poseWS, const BoneMappingContainer& bones ) override;

public: // Mimic interface, we don't need them but we have it because of existing systems eg. dialog system
	Bool PlayLipsyncAnimation( CSkeletalAnimationSetEntry* skeletalAnimation, Float offset = 0.f );
	Bool StopLipsyncAnimation();

	Bool PlayMimicAnimation( const CName& animation, const CName& slot = CName::NONE, Float blendTime = 0.0f, Float offset = 0.0f );
	void StopMimicAnimation( const CName& slot = CName::NONE );
	Bool HasMimicAnimation( const CName& slot = CName::NONE ) const;

	Bool SetMimicVariable( const CName varName, Float value );

	Bool RaiseMimicEvent( const CName& eventName, Bool force );

private: // Internal
	void ProcessMimicData( const SMimicPostProcessData* data, SBehaviorGraphOutput* pose, SBehaviorGraphOutput* parentsPose, const BoneMappingContainer& mapping );

	void ForceMeshHiResLOD( Bool flag );
	
	void SetMimicLod( EMimicLOD lod );
	void SetMimicLodSmooth( EMimicLOD lod );
	void UpdateMimicLodState();
	void InternalUpdateDestLodState( const Vector& cameraPosition, Float camFovFactor );

	void CacheNBComponentsAndSetupAreas();

	CAnimMimicParam* GetTemplateMimicParams();
	CAnimMimicParam* GetParentTemplateMimicParams();

	Float GetMimicInternalVariable( const CName varName ) const;

public:
	void ApplyMimicParams();
	virtual void OnParentAttachmentAdded( IAttachment* attachment ) override;

public: // Editor only
#ifndef NO_EDITOR
	void Editor_RefreshCameraPosition( const Vector& cameraPosition, Float cameraFov );
	void Editor_GetStateDesc( String& desc ) const;
#endif
};

BEGIN_CLASS_RTTI( CMimicComponent )
	PARENT_CLASS( CAnimatedComponent );
	PROPERTY_EDIT( m_mimicFace, TXT("Mimic face") );
	PROPERTY_EDIT( m_categoryMimics, TXT("Additional, category mimics") );
	PROPERTY_EDIT( m_attachmentSlotName, TXT("") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////
