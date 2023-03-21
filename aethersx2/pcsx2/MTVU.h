/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2010  PCSX2 Dev Team
 *
 *  PCSX2 is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU Lesser General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with PCSX2.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once
#include "System/SysThreads.h"
#include "Vif.h"
#include "Vif_Dma.h"
#include "VUmicro.h"

#define MTVU_LOG(...) do{} while(0)
//#define MTVU_LOG DevCon.WriteLn

// Notes:
// - This class should only be accessed from the EE thread...
// - buffer_size must be power of 2
// - ring-buffer has no complete pending packets when read_pos==write_pos
class VU_Thread : public pxThread {
	static const s32 buffer_size = (_1mb * 16) / sizeof(s32);

	u32 buffer[buffer_size];
	// Note: keep atomic on separate cache line to avoid CPU conflict
	__aligned(64) std::atomic<bool> isBusy;   // Is thread processing data?
	__aligned(64) std::atomic<int> m_ato_read_pos; // Only modified by VU thread
	__aligned(64) std::atomic<int> m_ato_write_pos;    // Only modified by EE thread
	__aligned(64) int  m_read_pos; // temporary read pos (local to the VU thread)
	int  m_write_pos; // temporary write pos (local to the EE thread)
	Mutex     mtxBusy;
	Semaphore semaEvent;
	BaseVUmicroCPU*& vuCPU;
	VURegs&          vuRegs;

public:
	__aligned16  vifStruct        vif;
	__aligned16  VIFregisters     vifRegs;
	Semaphore semaXGkick;
	std::atomic<unsigned int> vuCycles[4]; // Used for VU cycle stealing hack
	u32 vuCycleIdx;  // Used for VU cycle stealing hack

	enum InterruptFlag {
		InterruptFlagFinish = 1 << 0,
		InterruptFlagSignal = 1 << 1,
		InterruptFlagLabel  = 1 << 2,
		InterruptFlagVUEBit = 1 << 3,
		InterruptFlagVUTBit = 1 << 4,
	};

	std::atomic<u32> mtvuInterrupts; // Used for GS Signal, Finish etc, plus VU End/T-Bit
	std::atomic<u64> gsLabel; // Used for GS Label command
	std::atomic<u64> gsSignal; // Used for GS Signal command

	VU_Thread(BaseVUmicroCPU*& _vuCPU, VURegs& _vuRegs);
	virtual ~VU_Thread();

	void Reset();

	// Get MTVU to start processing its packets if it isn't already
	void KickStart(bool forceKick = false);

	// Used for assertions...
	bool IsDone();

	// Waits till MTVU is done processing
	void WaitVU();

	void Get_MTVUChanges();

	void ExecuteVU(u32 vu_addr, u32 vif_top, u32 vif_itop);

	void VifUnpack(vifStruct& _vif, VIFregisters& _vifRegs, u8* data, u32 size);

	// Writes to VU's Micro Memory (size in bytes)
	void WriteMicroMem(u32 vu_micro_addr, void* data, u32 size);

	// Writes to VU's Data Memory (size in bytes)
	void WriteDataMem(u32 vu_data_addr, void* data, u32 size);

	void WriteVIRegs(REG_VI* viRegs);

	void WriteVFRegs(VECTOR* vfRegs);
	
	void WriteCol(vifStruct& _vif);

	void WriteRow(vifStruct& _vif);

protected:
	void ExecuteTaskInThread();

private:
	void ExecuteRingBuffer();

	void WaitOnSize(s32 size);
	void ReserveSpace(s32 size);

	s32 GetReadPos();
	s32 GetWritePos();

	u32* GetWritePtr();

	void CommitWritePos();
	void CommitReadPos();

	u32 Read();
	void Read(void* dest, u32 size);
	void ReadRegs(VIFregisters* dest);

	void Write(u32 val);
	void Write(void* src, u32 size);
	void WriteRegs(VIFregisters* src);

	u32 Get_vuCycles();
};

extern __aligned16 VU_Thread vu1Thread;
