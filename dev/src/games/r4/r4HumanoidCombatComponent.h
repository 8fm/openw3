/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

class CSoundInfoMappingHelper
{
private:

										CSoundInfoMappingHelper();
public:
	Bool								FillMapping(const CActor* parent, CName soundInfo, SSoundInfoMapping& mappingToFill) const;
	static CSoundInfoMappingHelper*		GetInstance();
private:
	void								CreateBoneIndexesCaching();

private:
	static CSoundInfoMappingHelper*		st_instance;
	THandle< CIndexed2dArray >			m_array;

	Int32								m_firstBoneIndex;
	Int32								m_lastBoneIndex;
};

struct SSoundInfoMapping
{
	DECLARE_RTTI_STRUCT( SSoundInfoMapping )

	SSoundInfoMapping()
		: m_soundTypeIdentification( CName::NONE)
		, m_soundSizeIdentification( CName::NONE)
		, m_isDefault( false )
	{
		m_boneIndexes.Clear();
	}

	CName								m_soundTypeIdentification;
	CName								m_soundSizeIdentification;
	TDynArray< Int32 >					m_boneIndexes;
	Bool								m_isDefault;

	static const SSoundInfoMapping EMPTY;
};

BEGIN_CLASS_RTTI( SSoundInfoMapping )
	PROPERTY(m_soundTypeIdentification)
	PROPERTY(m_soundSizeIdentification)
	PROPERTY(m_boneIndexes)
	PROPERTY(m_isDefault)
END_CLASS_RTTI()

class CR4HumanoidCombatComponent : public CComponent
{
	DECLARE_ENGINE_CLASS( CR4HumanoidCombatComponent, CComponent, 0 );
private:
	THashMap< CName, SSoundInfoMapping >	m_mappings;
	SSoundInfoMapping						m_default;

	CName									m_pelvisBone;

	Int32									m_firstBoneInSkeletonIndex;
	Int32									m_lastBoneInSkeletonIndex;

public:
	CR4HumanoidCombatComponent();
	~CR4HumanoidCombatComponent();

	virtual Bool UsesAutoUpdateTransform() override { return false; }

public:
	const SSoundInfoMapping& 				GetSoundInfoMapping(Int32 boneIndex) const;
	// Return true if new mapping was inserted
	Bool									InsertNewMapping(const CName& soundInfoName, const CName& soundType, const CName& soundSize);

protected:
	void									funcUpdateSoundInfo( CScriptStackFrame& stack, void* result );

	void									funcGetSoundTypeIdentificationForBone( CScriptStackFrame& stack, void* result );
	void									funcGetBoneClosestToEdge( CScriptStackFrame& stack, void* result );
	void									funcGetDefaultSoundInfoMapping( CScriptStackFrame& stack, void* result );

private:
	void									CacheSkeletonBoneIndexes(const CActor * parent);
};


BEGIN_CLASS_RTTI( CR4HumanoidCombatComponent )
	PARENT_CLASS( CComponent )

	NATIVE_FUNCTION( "UpdateSoundInfo", funcUpdateSoundInfo );
	NATIVE_FUNCTION( "GetSoundTypeIdentificationForBone", funcGetSoundTypeIdentificationForBone );
	NATIVE_FUNCTION( "GetBoneClosestToEdge", funcGetBoneClosestToEdge );
	NATIVE_FUNCTION( "GetDefaultSoundInfoMapping", funcGetDefaultSoundInfoMapping );

END_CLASS_RTTI()