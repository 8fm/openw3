#pragma once

#include "speechBuffer.h"
#include "skeletalAnimation.h"

class LanguagePack
{
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_SmallObjects, MC_Dialog );

	friend class CLocalizationManager;

protected:
	LanguagePack();
	~LanguagePack();

public:
	RED_INLINE const String& GetText() const
	{ return m_textString; }

	RED_INLINE void SetText( const String& value )
	{ m_textString = value; }

	RED_INLINE Uint32 GetTextSize() const
	{ return static_cast< Uint32 >( m_textString.DataSize() ); }

	RED_INLINE const SpeechBuffer& GetSpeechBuffer() const
	{ return m_speechBuffer; }

	RED_INLINE SpeechBuffer& GetSpeechBuffer()
	{ return m_speechBuffer; }

	RED_INLINE Uint32 GetVoiceoverSize() const
	{ return static_cast< Uint32 >( m_speechBuffer.GetSize() ); }

	RED_INLINE const CSkeletalAnimation* GetLipsync() const
	{ return m_lipsync.Get(); }

	RED_INLINE CSkeletalAnimation* GetLipsync()
	{ return m_lipsync.Get(); }

	void SetLipsync( CSkeletalAnimation* value );

	RED_INLINE Uint32 GetLipsyncSize() const
	{ return m_lipsync.IsValid() ? m_lipsync->GetSizeOfAnimBuffer() : 0; }

	RED_INLINE Uint32 GetPackSize() const
	{
		return GetTextSize() + GetVoiceoverSize() + GetLipsyncSize();
	}

	RED_INLINE void Lock()
	{
		ASSERT( m_lockCount >= 0 );
		++m_lockCount;
	}

	RED_INLINE void Unlock()
	{
		ASSERT( m_lockCount >= 0 );
		--m_lockCount;
	}

	// DIALOG_TOMSIN_TODO - dlaczego to nie jest uzywane!? Jaka logika zatem decyduje kiedy to usunac i dlaczego
	Bool IsLocked()
	{
		return m_lockCount > 0;
	}

#ifndef NO_EDITOR
	RED_INLINE const String& GetLipsyncFileName() const
	{ return m_lipsyncFilename; }

	RED_INLINE void SetLipsyncFileName( const String& value )
	{ m_lipsyncFilename = value; }

	RED_INLINE void SetVoiceoverFileName( const String& value )
	{ m_voiceoverFilename = value; }

	RED_INLINE const String& GetStringKey() const
	{ return m_stringKey; }

	RED_INLINE void SetStringKey( const String& value )
	{ m_stringKey = value; }

#endif
	
	RED_INLINE const String& GetVoiceoverFileName() const
	{ return m_voiceoverFilename; }

private:
	String	m_textString;

	SpeechBuffer					m_speechBuffer;
	THandle< CSkeletalAnimation >	m_lipsync;
	
	Uint32	m_lockCount;
	
#ifndef NO_EDITOR
	String  m_stringKey;
	String	m_lipsyncFilename;
#endif

	String	m_voiceoverFilename;
};
