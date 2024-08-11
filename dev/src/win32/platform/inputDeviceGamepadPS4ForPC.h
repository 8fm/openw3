/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#ifdef RED_INPUT_DEVICE_GAMEPAD_PS4FORPC

#include "../../common/engine/inputDeviceGamepad.h"
#include "../../common/engine/notifier.h"
#include "../../common/engine/inputBufferedInputEvent.h"

struct ScePadAnalogButtons;
struct ScePadAnalogStick;
struct ScePadData;

class CInputDeviceGamepadPS4ForPC : public IInputDeviceGamepad
{
public:
	CInputDeviceGamepadPS4ForPC();
	~CInputDeviceGamepadPS4ForPC();

	static Bool InitializePadLibrary();
	static void ShutdownPadLibrary();

	Bool Init();

	virtual void Update(BufferedInput& outBufferedInput) override final;
	virtual void Reset() override final;

	virtual void SetBacklightColor(const Color& color) override final;
	virtual void ResetBacklightColor() override final;
	virtual void SetPadVibrate(Float leftValue, Float rightValue) override final;

	virtual const CName GetDeviceName() const override final;

private:
	typedef Uint32 TButton;
	typedef Int32 TPortHandle;

	void Close();

	void ResetIfNecessary(BufferedInput& outBufferedInput);
	void Reset( BufferedInput& outBufferedInput );

	void ClearIfNecessary();
	void Clear();

	void UpdateButtons(BufferedInput& outBufferedInput, Uint32 buttons);
	void UpdateTriggers(BufferedInput& outBufferedInput, const ScePadAnalogButtons& analogButtons);
	void UpdateSticks(BufferedInput& outBufferedInput, const ScePadAnalogStick& leftStick, const ScePadAnalogStick& rightStick);

	Bool ReadPadState(ScePadData* padData) const;
	EKey MapButtonToKey( TButton button ) const;
	void UpdateSavedGamepadReading(Uint64 timestamp, Uint32 buttons, Uint8 connectedCount);

	static const Uint8 INITIAL_CONNECTED_COUNT = 1;
	static const Uint8 DEFAULT_DEAD_ZONE_DELTA = 13;
	static const TPortHandle INVALID_PORT_HANDLE = -1;
	static const TButton GAMEPAD_BUTTONS[];

	bool m_needsReset;
	bool m_needsClear;

	TPortHandle m_portHandle;
	Bool m_disconnected;

	Bool m_keyDownEventResetTable[ EKey::Count ];
	Float m_axisValueTable[ EAxis::Count ];

	Uint32 m_prevButtonMask;
	Uint64 m_prevTimestamp;
	Uint8 m_prevConnectedCount;
	Uint8 m_deadZoneLeft;
	Uint8 m_deadZoneRight;
};

#endif // RED_INPUT_DEVICE_GAMEPAD_PS4FORPC
