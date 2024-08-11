#pragma once

#ifndef NO_EDITOR

class CSoundAdditionalListener
{
	friend class CSoundSystem;

private:
	Vector m_position;
	Vector m_up;
	Vector m_forward;
	char m_listenerIndex;

protected:
	CSoundAdditionalListener() : m_position( Vector::ZERO_3D_POINT ), m_up( Vector::ZERO_3D_POINT ), m_forward( Vector::ZERO_3D_POINT ), m_listenerIndex( -1 ) {}
	virtual ~CSoundAdditionalListener();

	void UpdateSoundListener( const Vector& cameraPosition, const Vector& up, const Vector& forward, class CSoundEmitterComponent* emitter );

public:
	const Vector& GetPosition() const { return m_position; }
	char GetIndex() const { return m_listenerIndex; }

};

#endif
