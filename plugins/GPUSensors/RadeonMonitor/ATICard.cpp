/*
 *  ATICard.cpp
 *  FakeSMCRadeon
 *
 *  Created by Slice on 24.07.10.
 *  Copyright 2010 Applelife.ru. All rights reserved.
 *
 */

#include "ATICard.h"
#include "radeon_chipinfo_gen.h"
//#include "Sensor.h"
OSDefineMetaClassAndStructors(ATICard, OSObject)

bool ATICard::initialize()
{
	rinfo = (RADEONCardInfo*)IOMalloc(sizeof(RADEONCardInfo));
	VCard->setMemoryEnable(true);
	/*	
	 // PCI dump
	 for (int i=0; i<0xff; i +=16) {
		IOLog("%02lx: ", (long unsigned int)i);
		for (int j=0; j<16; j += 4) {
			IOLog("%08lx ", (long unsigned int)VCard->configRead32(i+j));
		}
		IOLog("\n");
	 }
	 */
  //Test for ElCapitan problem
/*  long unsigned int j = VCard->configRead32(0xAC);
  IOLog("PMIR value: %08lx\n", j);
  VCard->configWrite32(0xAC, (j & (~0x1)) | 0x2);
 */
  //
	for (UInt32 i = 0; (mmio = VCard->mapDeviceMemoryWithIndex(i)); i++) {
		long unsigned int mmio_base_phys = mmio->getPhysicalAddress();
		// Make sure we  select MMIO registers
		if (((mmio->getLength()) <= 0x00040000) && (mmio_base_phys != 0))
			break;
	}
	if (mmio)	{
		mmio_base = (volatile UInt8 *)mmio->getVirtualAddress();
	} 
	else {
		InfoLog(" have no mmio\n ");
		return false;
	}
	
	if(!getRadeonInfo())
		return false;
  
  if (rinfo->ChipFamily == CHIP_FAMILY_HAWAII) {
    IOMemoryMap *   mmio5;
    mmio5 = VCard->mapDeviceMemoryWithIndex(5);
    if (mmio5 && mmio5->getPhysicalAddress() != 0) {
      mmio = mmio5;
      mmio_base = (volatile UInt8 *)mmio->getVirtualAddress();
    }    
  }

	switch (rinfo->ChipFamily) {
		case CHIP_FAMILY_R600:
		case CHIP_FAMILY_RV610:
		case CHIP_FAMILY_RV630:
		case CHIP_FAMILY_RV670:
			//setup_R6xx();
			tempFamily = R6xx;
			break;
		case CHIP_FAMILY_R700:
		case CHIP_FAMILY_R710:
		case CHIP_FAMILY_R730:
		case CHIP_FAMILY_RV740:
		case CHIP_FAMILY_RV770:
		case CHIP_FAMILY_RV790:
			//setup_R7xx();
			tempFamily = R7xx;
			break;
		case CHIP_FAMILY_Evergreen:
			//setup_Evergreen();
			tempFamily = R8xx;
			break;
        case   CHIP_FAMILY_PITCAIRN:
        case   CHIP_FAMILY_TAHITI:
        case   CHIP_FAMILY_VERDE:
			tempFamily = R9xx;
            break;
        case   CHIP_FAMILY_HAWAII:
        case   CHIP_FAMILY_OLAND:
        case   CHIP_FAMILY_BONAIRE:
        case   CHIP_FAMILY_HAINAN:
        case   CHIP_FAMILY_TONGA:

			tempFamily = RCIx;
			break;
			
		default:
			InfoLog("sorry, but your card %04lx is not supported!\n", (long unsigned int)(rinfo->device_id));
			return false;
	}
	
	return true;
}

bool ATICard::getRadeonInfo()
{
	UInt16 devID = chipID & 0xffff;
	RADEONCardInfo *devices = radeon_device_list;
	//rinfo = new RADEONCardInfo;
	while (devices->device_id != NULL) {
		//IOLog("check %d/n", devices->device_id ); //Debug
		if ((devices->device_id & 0xffff) == devID ) {
			//			rinfo->device_id = devID;
			rinfo->device_id = devices->device_id;
			rinfo->ChipFamily = devices->ChipFamily;
			family = devices->ChipFamily;
			rinfo->igp = devices->igp;
			rinfo->is_mobility = devices->is_mobility;
			IOLog(" Found ATI Radeon %04lx\n", (long unsigned int)devID);
			return true;
		}
		devices++;
	}
if (((devID >= 0x67A0) && (devID <= 0x6800)) ||  //Hawaii
             ((devID & 0xFF00) == 0x6900) ||  //Volcanic Island ?
             ((devID >= 0x6640) && (devID < 0x6670)))  { //Bonair & Hainan
    rinfo->device_id = devID;
    rinfo->ChipFamily = CHIP_FAMILY_HAWAII;
    family = CHIP_FAMILY_HAWAII;
    rinfo->igp = 0;
    rinfo->is_mobility = false;
    IOLog(" Common ATI Radeon like HAWAII DID=%04lx\n", (long unsigned int)devID);
    return true;

} else   if (((devID >= 0x6780) && (devID <= 0x6840)) || //Tahiti
         //    ((devID >= 0x6660) && (devID < 0x6670)) ||  //Hainan
             ((devID >= 0x6600) && (devID < 0x6640)) ) { //Oland
  rinfo->device_id = devID;
  rinfo->ChipFamily = CHIP_FAMILY_PITCAIRN;
  family = CHIP_FAMILY_PITCAIRN;
  rinfo->igp = 0;
  rinfo->is_mobility = false;
  IOLog(" Common ATI Radeon like PITCAIRN DID=%04lx\n", (long unsigned int)devID);
  return true;
} else if (((devID & 0xFF00) == 0x6700) || ((devID & 0xFF00) == 0x6800)) {
    rinfo->device_id = devID;
    rinfo->ChipFamily = CHIP_FAMILY_Evergreen;
    family = CHIP_FAMILY_Evergreen;
    rinfo->igp = 0;
    rinfo->is_mobility = false;
    IOLog(" Common ATI Radeon like Evergreen DID=%04lx\n", (long unsigned int)devID);
    //    IOLog("sorry, not supported yet, please report DeviceID=0x%x\n", devID);
    return true;
  }

	InfoLog("Unknown DeviceID!\n");
	return false;
}
/*
void ATICard::setup_R6xx()
{
	char key[5];
	int id = GetNextUnusedKey(KEY_FORMAT_GPU_DIODE_TEMPERATURE, key);
	if (id == -1) {
		InfoLog("No new GPU SMC key!\n");
		return;
	}
	card_number = id;
	tempSensor = new R6xxTemperatureSensor(this, id, key, TYPE_SP78, 2);
	Caps = GPU_TEMP_MONITORING;	
}

void ATICard::setup_R7xx()
{
	char key[5];
	int id = GetNextUnusedKey(KEY_FORMAT_GPU_DIODE_TEMPERATURE, key);
	if (id == -1) {
		InfoLog("No new GPU SMC key!\n");
		return;
	}
	card_number = id;
	tempSensor = new R7xxTemperatureSensor(this, id, key, TYPE_SP78, 2);
	Caps = GPU_TEMP_MONITORING;
}

void ATICard::setup_Evergreen()
{
	char key[5];
	int id = GetNextUnusedKey(KEY_FORMAT_GPU_DIODE_TEMPERATURE, key);
	if (id == -1) {
		InfoLog("No new GPU SMC key!\n");
		return;
	}
	card_number = id;
	tempSensor = new EverTemperatureSensor(this, id, key, TYPE_SP78, 2);
	Caps = GPU_TEMP_MONITORING;
}
*/
UInt32 ATICard::read32(UInt32 reg)
{
	return INVID(reg);
}

void ATICard::write32(UInt32 reg, UInt32 val)
{
	return OUTVID(reg, val);
}

//linux 4.14
#define mmSMC_IND_INDEX_11                            0x1AC
#define mmSMC_IND_DATA_11                             0x1AD

#define mmPCIE_INDEX                                                                                   0x000c
#define mmPCIE_DATA

//read_ind_pcie ->
/*
WREG32(mmPCIE_INDEX, reg);
(void)RREG32(mmPCIE_INDEX);
r = RREG32(mmPCIE_DATA);
*/

UInt32 ATICard::read_ind(UInt32 reg)
{
    //	unsigned long flags;
	UInt32 r;
    
    //	spin_lock_irqsave(&rdev->smc_idx_lock, flags);
/*	OUTVID(TN_SMC_IND_INDEX_0, (reg));
	r = INVID(TN_SMC_IND_DATA_0);
 */
  OUTVID(mmSMC_IND_INDEX_11, (reg));
  r = INVID(mmSMC_IND_DATA_11);
    //	spin_unlock_irqrestore(&rdev->smc_idx_lock, flags);
	return r;
}

IOReturn ATICard::R6xxTemperatureSensor(UInt16* data)
{
	UInt32 temp, actual_temp = 0;
	for (int i=0; i<1000; i++) {  //attempts to ready
		temp = (read32(CG_THERMAL_STATUS) & ASIC_T_MASK) >> ASIC_T_SHIFT;	
		if ((temp >> 7) & 1)
			actual_temp = 0;
		else {
			actual_temp = temp & 0xff; //(temp >> 1)
			break;
		}
		IOSleep(10);
	}
	*data = (UInt16)(actual_temp & 0x1ff);
	//data[1] = 0;
	return kIOReturnSuccess; 
	
}

IOReturn ATICard::R7xxTemperatureSensor(UInt16* data)
{
	UInt32 temp, actual_temp = 0;
	for (int i=0; i<1000; i++) {  //attempts to ready
		temp = (read32(CG_MULT_THERMAL_STATUS) & ASIC_TM_MASK) >> ASIC_TM_SHIFT;	
		if ((temp >> 9) & 1)
			actual_temp = 0;
		else {
			actual_temp = (temp >> 1) & 0xff;
			break;
		}
		IOSleep(10);
	}
	
	*data = (UInt16)(actual_temp & 0x1ff);
	//data[1] = 0;
	return kIOReturnSuccess;
}

IOReturn ATICard::EverTemperatureSensor(UInt16* data)
{
	UInt32 temp, actual_temp = 0;
	for (int i=0; i<1000; i++) {  //attempts to ready
		temp = (read32(CG_MULT_THERMAL_STATUS) & ASIC_TM_MASK) >> ASIC_TM_SHIFT;	
		if ((temp >> 10) & 1)
			actual_temp = 0;
		else if ((temp >> 9) & 1)
			actual_temp = 255;
		else {
			actual_temp = (temp >> 1) & 0xff;
			break;
		}
		IOSleep(10);
	}
	
	*data = (UInt16)(actual_temp & 0x1ff);
	//data[1] = 0;
	return kIOReturnSuccess;
}

IOReturn ATICard::TahitiTemperatureSensor(UInt16* data)
{
	UInt32 temp, actual_temp = 0;
	for (int i=0; i<1000; i++) {  //attempts to ready
		temp = (read32(CG_SI_THERMAL_STATUS) & CTF_TEMP_MASK) >> CTF_TEMP_SHIFT;
		if ((temp >> 10) & 1)
			actual_temp = 0;
		else if ((temp >> 9) & 1)
			actual_temp = 255;
		else {
			actual_temp = temp; //(temp >> 1) & 0xff;
			break;
		}
		IOSleep(10);
	}
	
	*data = (UInt16)(actual_temp & 0x1ff);
	//data[1] = 0;
	return kIOReturnSuccess;
}

IOReturn ATICard::HawaiiTemperatureSensor(UInt16* data)
{
	UInt32 temp, actual_temp = 0;
	for (int i=0; i<1000; i++) {  //attempts to ready
		temp = (read_ind(CG_CI_MULT_THERMAL_STATUS) & CI_CTF_TEMP_MASK) >> CI_CTF_TEMP_SHIFT;
		if ((temp >> 10) & 1)
			actual_temp = 0;
		else if ((temp >> 9) & 1)
			actual_temp = 255;
		else {
			actual_temp = temp & 0x1ff; //(temp >> 1) & 0xff;
			break;
		}
		IOSleep(10);
	}
	
	*data = (UInt16)(actual_temp & 0x1ff);
	//data[1] = 0;
	return kIOReturnSuccess;
}

