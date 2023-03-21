#pragma once

#include "GSScanlineEnvironment.h"

#include "vixl/aarch64/macro-assembler-aarch64.h"

class GSDrawScanlineCodeGenerator2
{
public:
	GSDrawScanlineCodeGenerator2(vixl::aarch64::MacroAssembler& armAsm_, void* param, uint64 key);
	void Generate();

private:
	void Init_NEON();
	void Step_NEON();
	void TestZ_NEON(const vixl::aarch64::VRegister& temp1, const vixl::aarch64::VRegister& temp2);
	void SampleTexture_NEON();
	void Wrap_NEON(const vixl::aarch64::VRegister& uv0);
	void Wrap_NEON(const vixl::aarch64::VRegister& uv0, const vixl::aarch64::VRegister& uv1);
	void SampleTextureLOD_NEON();
	void WrapLOD_NEON(const vixl::aarch64::VRegister& uv0);
	void WrapLOD_NEON(const vixl::aarch64::VRegister& uv0, const vixl::aarch64::VRegister& uv1);
	void AlphaTFX_NEON();
	void ReadMask_NEON();
	void TestAlpha_NEON();
	void ColorTFX_NEON();
	void Fog_NEON();
	void ReadFrame_NEON();
	void TestDestAlpha_NEON();
	void WriteMask_NEON();
	void WriteZBuf_NEON();
	void AlphaBlend_NEON();
	void WriteFrame_NEON();
	void ReadPixel_NEON(const vixl::aarch64::VRegister& dst, const vixl::aarch64::WRegister& addr);
	void WritePixel_NEON(const vixl::aarch64::VRegister& src, const vixl::aarch64::WRegister& addr, const vixl::aarch64::WRegister& mask, bool high, bool fast, int psm, int fz);
	void WritePixel_NEON(const vixl::aarch64::VRegister& src, const vixl::aarch64::WRegister& addr, uint8 i, int psm);
	void ReadTexel_NEON(int pixels, int mip_offset = 0);
	void ReadTexel_NEON(const vixl::aarch64::VRegister& dst, const vixl::aarch64::VRegister& addr, uint8 i);

	void modulate16(const vixl::aarch64::VRegister& a, const vixl::aarch64::VRegister& f, uint8 shift);
	void lerp16(const vixl::aarch64::VRegister& a, const vixl::aarch64::VRegister& b, const vixl::aarch64::VRegister& f, uint8 shift);
	void lerp16_4(const vixl::aarch64::VRegister& a, const vixl::aarch64::VRegister& b, const vixl::aarch64::VRegister& f);
	void mix16(const vixl::aarch64::VRegister& a, const vixl::aarch64::VRegister& b, const vixl::aarch64::VRegister& temp);
	void clamp16(const vixl::aarch64::VRegister& a, const vixl::aarch64::VRegister& temp);
	void alltrue(const vixl::aarch64::VRegister& test);
	void blend(const vixl::aarch64::VRegister& a, const vixl::aarch64::VRegister& b, const vixl::aarch64::VRegister& mask);
	void blendr(const vixl::aarch64::VRegister& b, const vixl::aarch64::VRegister& a, const vixl::aarch64::VRegister& mask);
	void blend8(const vixl::aarch64::VRegister& a, const vixl::aarch64::VRegister& b, const vixl::aarch64::VRegister& mask, const vixl::aarch64::VRegister& temp);
	void blend8r(const vixl::aarch64::VRegister& b, const vixl::aarch64::VRegister& a, const vixl::aarch64::VRegister& mask, const vixl::aarch64::VRegister& temp);
	void split16_2x8(const vixl::aarch64::VRegister& l, const vixl::aarch64::VRegister& h, const vixl::aarch64::VRegister& src);

	vixl::aarch64::MacroAssembler& armAsm;

	GSScanlineSelector m_sel;
	GSScanlineLocalData& m_local;

	vixl::aarch64::Label m_step_label;
};
