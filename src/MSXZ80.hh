// $Id$

#ifndef __MSXZ80_HH__
#define __MSXZ80_HH__

#include "emutime.hh"
#include "Z80.hh"
#include "MSXCPUDevice.hh"
#include "MSXCPU.hh"
class MSXCPU;

class MSXZ80 : public MSXCPUDevice, Z80Interface
{
	public:
		// constructor and destructor
		MSXZ80();
		virtual ~MSXZ80();
		
		//MSXCPUDevice
		void init();
		void reset();
		//void IRQ(bool irq);
		void executeUntilTarget();

		//Z80Interface
		byte readIO(word port);
		void writeIO (word port, byte value);
		byte readMem(word address);
		void writeMem(word address, byte value);

	private:
		Z80 *z80;
};
#endif //__MSXZ80_HH__
