// $Id$

#include "NationalFDC.hh"
#include "WD2793.hh"
#include "CPU.hh"


NationalFDC::NationalFDC(MSXConfig::Device *config, const EmuTime &time)
	: MSXFDC(config, time), MSXDevice(config, time)
{
	emptyRom = new byte[CPU::CACHE_LINE_SIZE];
	memset(emptyRom, 255, CPU::CACHE_LINE_SIZE);
	
	// real thing uses MB8877A, but it is compatible with WD2793
	//  TODO check completely compatible
	controller = new WD2793(config, time);
}

NationalFDC::~NationalFDC()
{
	delete controller;
	delete[] emptyRom;
}

void NationalFDC::reset(const EmuTime &time)
{
	controller->reset(time);
}

byte NationalFDC::readMem(word address, const EmuTime &time)
{
	byte value;
	switch (address & 0x3FFF) {
	case 0x3FB8:
		value = controller->getStatusReg(time);
		break;
	case 0x3FB9:
		value = controller->getTrackReg(time);
		break;
	case 0x3FBA:
		value = controller->getSectorReg(time);
		break;
	case 0x3FBB:
		value = controller->getDataReg(time);
		break;
	case 0x3FBC:
		// Drive control IRQ and DRQ lines are not connected to Z80 interrupt request
		// bit 7: intrq
		// bit 6: !dtrq
		value = 0x40;
		if (controller->getIRQ(time))  value |= 0x80;
		if (controller->getDTRQ(time)) value &= ~0x40;
		break;
	default:
		if (address < 0x8000) {
			// ROM only visible in 0x0000-0x7FFF
			value = MSXFDC::readMem(address, time);
		} else {
			value = 255;
		}
		break;
	}
	//PRT_DEBUG("NationalFDC read 0x" << std::hex << (int)address << " 0x" << (int)value << std::dec);
	return value;
}


void NationalFDC::writeMem(word address, byte value, const EmuTime &time)
{
	//PRT_DEBUG("NationalFDC write 0x" << std::hex << (int)address << " 0x" << (int)value << std::dec);
	switch (address & 0x3FFF) {
	case 0x3FB8:
		controller->setCommandReg(value, time);
		break;
	case 0x3FB9:
		controller->setTrackReg(value, time);
		break;
	case 0x3FBA:
		controller->setSectorReg(value, time);
		break;
	case 0x3FBB:
		controller->setDataReg(value, time);
		break;
	case 0x3FBC:
		//bit 0 -> select drive 0
		//bit 1 -> select drive 1
		//bit 2 -> side select
		//bit 3 -> motor on
		WD2793::DriveNum drive;
		switch (value & 3) {
			case 1:
				drive = WD2793::DRIVE_A;
				break;
			case 2:
				drive = WD2793::DRIVE_B;
				break;
			default:
				drive = WD2793::NO_DRIVE;
		}
		controller->setDriveSelect(drive, time);
		controller->setSideSelect((value & 0x04), time);
		controller->setMotor((value & 0x08), time);
		break;
	}
}

byte* NationalFDC::getReadCacheLine(word start)
{
	if ((start & 0x3FB8 & CPU::CACHE_LINE_HIGH) == (0x3FB8 & CPU::CACHE_LINE_HIGH))
		// FDC at 0x7FB8-0x7FBC (also mirrored)
		return NULL;
	if (start < 0x8000) {
		// ROM at 0x0000-0x7FFF
		return MSXFDC::getReadCacheLine(start);
	} else {
		return emptyRom;
	}
}
