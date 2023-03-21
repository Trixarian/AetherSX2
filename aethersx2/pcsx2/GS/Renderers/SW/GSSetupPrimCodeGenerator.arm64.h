#pragma once

#include "GSScanlineEnvironment.h"

#include "vixl/aarch64/macro-assembler-aarch64.h"

class GSSetupPrimCodeGenerator2
{
public:
	GSSetupPrimCodeGenerator2(vixl::aarch64::MacroAssembler& armAsm_, void* param, uint64 key);
	void Generate();

private:
	void Depth_NEON();
	void Texture_NEON();
	void Color_NEON();

	vixl::aarch64::MacroAssembler& armAsm;

	GSScanlineSelector m_sel;
	GSScanlineLocalData& m_local;
	bool m_rip;
	bool many_regs;

	struct
	{
		uint32 z : 1, f : 1, t : 1, c : 1;
	} m_en;
};
