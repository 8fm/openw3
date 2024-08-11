#pragma once

#include "../core/dataBuffer.h"

/// A buffer for VO speech
class SpeechBuffer : public DataBuffer
{
public:
	//! Get the sound duration
	RED_INLINE Float GetDuration() const { return m_duration; }

	//! Get data compression
	RED_INLINE Uint32 GetCompression() const { return m_compression; }

	//! Sets precalculated duration
	RED_INLINE void SetDuration( Float duration ) { m_duration = duration; }

	//! Is the buffer loaded
	RED_INLINE const Bool IsLoaded() const { return (GetData() != nullptr); }

	//! Construct empty
	SpeechBuffer();

	//! Constructs data buffer from existing data
	SpeechBuffer( const SpeechBuffer& other );

	//! Assignment operator
	SpeechBuffer& operator=( const SpeechBuffer& other );

	//! Clear data
	void Clear();

	//! Load manualy
	Bool LoadFromFile( const String &filename );

	//! Serialization
	void Serialize( IFile& file );

private:
	Float		m_duration;		//!< Duration of the sound ( in seconds );
	Uint32		m_compression;  //!< Type of sound compression

	Float CalculateOggDuration() const;
	Float CalculateRIFFDuration() const;
	Float CalculateMp2Duration() const;
	Float CalculateXmaDuration() const;
};
