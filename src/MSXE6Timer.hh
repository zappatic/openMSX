// $Id$

/*
 * This class implements a simple timer, used in a TurboR.
 * This is a 16-bit (up) counter running at 255681Hz (3.58MHz / 14).
 * The least significant byte can be read from port 0xE6.
 *     most                                         0xE7.
 * Writing a random value to port 0xE6 resets the counter.
 * Writing to port 0xE7 has no effect.
 */

#ifndef MSXE6TIMER_HH
#define MSXE6TIMER_HH

#include "MSXDevice.hh"
#include "Clock.hh"

namespace openmsx {

class MSXE6Timer : public MSXDevice
{
public:
	MSXE6Timer(MSXMotherBoard& motherBoard, const XMLElement& config);

	virtual void reset(const EmuTime& time);
	virtual byte readIO(word port, const EmuTime& time);
	virtual void writeIO(word port, byte value, const EmuTime& time);
	virtual byte peekIO(word port, const EmuTime& time) const;

	template<typename Archive>
	void serialize(Archive& ar, unsigned version);

private:
	Clock<3579545, 14> reference; // (3.58 / 14)Mhz
};
REGISTER_MSXDEVICE(MSXE6Timer, "E6Timer");

} // namespace openmsx

#endif
