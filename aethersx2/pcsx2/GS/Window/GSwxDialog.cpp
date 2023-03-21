/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2021 PCSX2 Dev Team
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

#include "PrecompiledHeader.h"
#include "GSwxDialog.h"
#include "gui/AppConfig.h"

#ifdef _WIN32
#include "GS/Renderers/DX11/D3D.h"
#endif

using namespace GSSettingsDialog;

namespace
{
	void add_tooltip(wxWindow* widget, int tooltip)
	{
		if (tooltip != -1)
			widget->SetToolTip(dialog_message(tooltip));
	}

	void add_settings_to_array_string(const std::vector<GSSetting>& s, wxArrayString& arr)
	{
		for (const GSSetting& setting : s)
		{
			if (!setting.note.empty())
				arr.Add(setting.name + " (" + setting.note + ")");
			else
				arr.Add(setting.name);
		}
	}

	size_t get_config_index(const std::vector<GSSetting>& s, int value)
	{
		for (size_t i = 0; i < s.size(); i++)
		{
			if (s[i].value == value)
				return i;
		}
		return 0;
	}

	void set_config_from_choice(const wxChoice* choice, const std::vector<GSSetting>& s, const char* str)
	{
		int idx = choice->GetSelection();

		if (idx == wxNOT_FOUND)
			return;

		theApp.SetConfig(str, s[idx].value);
	}

	void add_label(wxWindow* parent, wxSizer* sizer, const char* str, int tooltip = -1, wxSizerFlags flags = wxSizerFlags().Centre().Right(), long style = wxALIGN_RIGHT | wxALIGN_CENTRE_HORIZONTAL)
	{
		auto* temp_text = new wxStaticText(parent, wxID_ANY, str, wxDefaultPosition, wxDefaultSize, style);
		add_tooltip(temp_text, tooltip);
		sizer->Add(temp_text, flags);
	}

	/// wxBoxSizer with padding
	template <typename OuterSizer>
	struct PaddedBoxSizer
	{
		OuterSizer* outer;
		wxBoxSizer* inner;

		// Make static analyzers happy (memory management is handled by WX)
		// (There's no actual reason we couldn't use the default copy constructor, except cppcheck screams when you do and it's easier to delete than implement one)
		PaddedBoxSizer(const PaddedBoxSizer&) = delete;

		template <typename... Args>
		PaddedBoxSizer(wxOrientation orientation, Args&&... args)
			: outer(new OuterSizer(orientation, std::forward<Args>(args)...))
			, inner(new wxBoxSizer(orientation))
		{
			wxSizerFlags flags = wxSizerFlags().Expand();
#ifdef __APPLE__
			if (!std::is_same<OuterSizer, wxStaticBoxSizer>::value) // wxMac already adds padding to static box sizers
#endif
				flags.Border();
			outer->Add(inner, flags);
		}

		wxBoxSizer* operator->() { return inner; }
	};

	struct CheckboxPrereq
	{
		wxCheckBox* box;
		explicit CheckboxPrereq(wxCheckBox* box)
			: box(box)
		{
		}

		bool operator()()
		{
			return box->GetValue();
		}
	};
} // namespace

GSUIElementHolder::GSUIElementHolder(wxWindow* window)
	: m_window(window)
{
}

wxStaticText* GSUIElementHolder::addWithLabel(wxControl* control, UIElem::Type type, wxSizer* sizer, const char* label, const char* config_name, int tooltip, std::function<bool()> prereq, wxSizerFlags flags)
{
	add_tooltip(control, tooltip);
	wxStaticText* text = new wxStaticText(m_window, wxID_ANY, label, wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT);
	add_tooltip(text, tooltip);
	sizer->Add(text, wxSizerFlags().Centre().Right());
	sizer->Add(control, flags);
	m_elems.emplace_back(type, control, config_name, prereq);
	return text;
}

wxCheckBox* GSUIElementHolder::addCheckBox(wxSizer* sizer, const char* label, const char* config_name, int tooltip, std::function<bool()> prereq)
{
	wxCheckBox* box = new wxCheckBox(m_window, wxID_ANY, label);
	add_tooltip(box, tooltip);
	if (sizer)
		sizer->Add(box);
	m_elems.emplace_back(UIElem::Type::CheckBox, box, config_name, prereq);
	return box;
}

std::pair<wxChoice*, wxStaticText*> GSUIElementHolder::addComboBoxAndLabel(wxSizer* sizer, const char* label, const char* config_name, const std::vector<GSSetting>* settings, int tooltip, std::function<bool()> prereq)
{
	wxArrayString temp;
	add_settings_to_array_string(*settings, temp);
	wxChoice* choice = new GSwxChoice(m_window, wxID_ANY, wxDefaultPosition, wxDefaultSize, temp, settings);
	return std::make_pair(choice, addWithLabel(choice, UIElem::Type::Choice, sizer, label, config_name, tooltip, prereq));
}

wxSpinCtrl* GSUIElementHolder::addSpin(wxSizer* sizer, const char* config_name, int min, int max, int initial, int tooltip, std::function<bool()> prereq)
{
	wxSpinCtrl* spin = new wxSpinCtrl(m_window, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, min, max, initial);
	add_tooltip(spin, tooltip);
	if (sizer)
		sizer->Add(spin, wxSizerFlags(1));
	m_elems.emplace_back(UIElem::Type::Spin, spin, config_name, prereq);
	return spin;
}

std::pair<wxSpinCtrl*, wxStaticText*> GSUIElementHolder::addSpinAndLabel(wxSizer* sizer, const char* label, const char* config_name, int min, int max, int initial, int tooltip, std::function<bool()> prereq)
{
	wxSpinCtrl* spin = new wxSpinCtrl(m_window, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, min, max, initial);
	return std::make_pair(spin, addWithLabel(spin, UIElem::Type::Spin, sizer, label, config_name, tooltip, prereq, wxSizerFlags().Centre().Left().Expand()));
}

std::pair<wxSlider*, wxStaticText*> GSUIElementHolder::addSliderAndLabel(wxSizer* sizer, const char* label, const char* config_name, int min, int max, int initial, int tooltip, std::function<bool()> prereq)
{
	wxSlider* slider = new wxSlider(m_window, wxID_ANY, initial, min, max, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_VALUE_LABEL);
	return std::make_pair(slider, addWithLabel(slider, UIElem::Type::Slider, sizer, label, config_name, tooltip, prereq));
}

std::pair<wxFilePickerCtrl*, wxStaticText*> GSUIElementHolder::addFilePickerAndLabel(wxSizer* sizer, const char* label, const char* config_name, int tooltip, std::function<bool()> prereq)
{
	wxFilePickerCtrl* picker = new wxFilePickerCtrl(m_window, wxID_ANY);
	return std::make_pair(picker, addWithLabel(picker, UIElem::Type::File, sizer, label, config_name, tooltip, prereq));
}

std::pair<wxDirPickerCtrl*, wxStaticText*> GSUIElementHolder::addDirPickerAndLabel(wxSizer* sizer, const char* label, const char* config_name, int tooltip, std::function<bool()> prereq)
{
	wxDirPickerCtrl* picker = new wxDirPickerCtrl(m_window, wxID_ANY);
	return std::make_pair(picker, addWithLabel(picker, UIElem::Type::Directory, sizer, label, config_name, tooltip, prereq));
}

void GSUIElementHolder::Load()
{
	for (const UIElem& elem : m_elems)
	{
		switch (elem.type)
		{
			case UIElem::Type::CheckBox:
				static_cast<wxCheckBox*>(elem.control)->SetValue(theApp.GetConfigB(elem.config));
				break;
			case UIElem::Type::Choice:
			{
				GSwxChoice* choice = static_cast<GSwxChoice*>(elem.control);
				choice->SetSelection(get_config_index(choice->settings, theApp.GetConfigI(elem.config)));
				break;
			}
			case UIElem::Type::Spin:
				static_cast<wxSpinCtrl*>(elem.control)->SetValue(theApp.GetConfigI(elem.config));
				break;
			case UIElem::Type::Slider:
				static_cast<wxSlider*>(elem.control)->SetValue(theApp.GetConfigI(elem.config));
				break;
			case UIElem::Type::File:
			case UIElem::Type::Directory:
			{
				auto* picker = static_cast<wxFileDirPickerCtrlBase*>(elem.control);
				picker->SetInitialDirectory(theApp.GetConfigS(elem.config));
				picker->SetPath(theApp.GetConfigS(elem.config));
				break;
			}
		}
	}
}

void GSUIElementHolder::Save()
{
	for (const UIElem& elem : m_elems)
	{
		switch (elem.type)
		{
			case UIElem::Type::CheckBox:
				theApp.SetConfig(elem.config, static_cast<wxCheckBox*>(elem.control)->GetValue());
				break;
			case UIElem::Type::Choice:
			{
				GSwxChoice* choice = static_cast<GSwxChoice*>(elem.control);
				set_config_from_choice(choice, choice->settings, elem.config);
				break;
			}
			case UIElem::Type::Spin:
				theApp.SetConfig(elem.config, static_cast<wxSpinCtrl*>(elem.control)->GetValue());
				break;
			case UIElem::Type::Slider:
				theApp.SetConfig(elem.config, static_cast<wxSlider*>(elem.control)->GetValue());
				break;
			case UIElem::Type::File:
			case UIElem::Type::Directory:
				theApp.SetConfig(elem.config, static_cast<wxFileDirPickerCtrlBase*>(elem.control)->GetPath());
				break;
		}
	}
}

void GSUIElementHolder::Update()
{
	for (const UIElem& elem : m_elems)
	{
		if (elem.prereq)
			elem.control->Enable(elem.prereq());
	}
}

void GSUIElementHolder::DisableAll()
{
	for (const UIElem& elem : m_elems)
	{
		if (elem.prereq)
			elem.control->Enable(false);
	}
}

RendererTab::RendererTab(wxWindow* parent)
	: wxPanel(parent, wxID_ANY)
	, m_ui(this)
{
	const int space = wxSizerFlags().Border().GetBorderInPixels();
	auto hw_prereq = [this]{ return m_is_hardware; };
	auto sw_prereq = [this]{ return !m_is_hardware; };

	PaddedBoxSizer<wxBoxSizer> tab_box(wxVERTICAL);
	PaddedBoxSizer<wxStaticBoxSizer> hardware_box(wxVERTICAL, this, "Hardware Mode");
	PaddedBoxSizer<wxStaticBoxSizer> software_box(wxVERTICAL, this, "Software Mode");

	auto* hw_checks_box = new wxWrapSizer(wxHORIZONTAL);

	m_ui.addCheckBox(hw_checks_box, "GPU Palette Conversion",          "paltex",                   IDC_PALTEX,          hw_prereq);
	m_ui.addCheckBox(hw_checks_box, "Conservative Buffer Allocation",  "conservative_framebuffer", IDC_CONSERVATIVE_FB, hw_prereq);
	m_ui.addCheckBox(hw_checks_box, "Accurate Destination Alpha Test", "accurate_date",            IDC_ACCURATE_DATE,   hw_prereq);

	auto* hw_choice_grid = new wxFlexGridSizer(2, space, space);

	m_internal_resolution = m_ui.addComboBoxAndLabel(hw_choice_grid, "Internal Resolution:", "upscale_multiplier", &theApp.m_gs_upscale_multiplier, -1, hw_prereq).first;

	m_ui.addComboBoxAndLabel(hw_choice_grid, "Anisotropic Filtering:", "MaxAnisotropy",  &theApp.m_gs_max_anisotropy, IDC_AFCOMBO,   hw_prereq);
	m_ui.addComboBoxAndLabel(hw_choice_grid, "Dithering (PgDn):",      "dithering_ps2",  &theApp.m_gs_dithering,      IDC_DITHERING, hw_prereq);
	m_ui.addComboBoxAndLabel(hw_choice_grid, "Mipmapping (Insert):",   "mipmap_hw",      &theApp.m_gs_hw_mipmapping,  IDC_MIPMAP_HW, hw_prereq);
	m_ui.addComboBoxAndLabel(hw_choice_grid, "CRC Hack Level:",        "crc_hack_level", &theApp.m_gs_crc_level,      IDC_CRC_LEVEL, hw_prereq);

	m_blend_mode = m_ui.addComboBoxAndLabel(hw_choice_grid, "Blending Accuracy:", "accurate_blending_unit", &theApp.m_gs_acc_blend_level, IDC_ACCURATE_BLEND_UNIT, hw_prereq);
#ifdef _WIN32
	m_blend_mode_d3d11 = m_ui.addComboBoxAndLabel(hw_choice_grid, "Blending Accuracy:", "accurate_blending_unit_d3d11", &theApp.m_gs_acc_blend_level_d3d11, IDC_ACCURATE_BLEND_UNIT_D3D11, hw_prereq);
#endif

	hardware_box->Add(hw_checks_box, wxSizerFlags().Centre());
	hardware_box->AddSpacer(space);
	hardware_box->Add(hw_choice_grid, wxSizerFlags().Centre());

	auto* sw_checks_box = new wxWrapSizer(wxHORIZONTAL);
	m_ui.addCheckBox(sw_checks_box, "Auto Flush",              "autoflush_sw", IDC_AUTO_FLUSH_SW, sw_prereq);
	m_ui.addCheckBox(sw_checks_box, "Edge Antialiasing (Del)", "aa1",          IDC_AA1,           sw_prereq);
	m_ui.addCheckBox(sw_checks_box, "Mipmapping",              "mipmap",       IDC_MIPMAP_SW,     sw_prereq);

	software_box->Add(sw_checks_box, wxSizerFlags().Centre());
	software_box->AddSpacer(space);

	// Rendering threads
	auto* thread_box = new wxFlexGridSizer(2, space, space);
	m_ui.addSpinAndLabel(thread_box, "Extra Rendering threads:", "extrathreads", 0, 32, 2, IDC_SWTHREADS, sw_prereq);
	software_box->Add(thread_box, wxSizerFlags().Centre());

	tab_box->Add(hardware_box.outer, wxSizerFlags().Expand());
	tab_box->Add(software_box.outer, wxSizerFlags().Expand());

	SetSizerAndFit(tab_box.outer);
}

void RendererTab::UpdateBlendMode(GSRendererType renderer)
{
#ifdef _WIN32
	if (renderer == GSRendererType::DX11)
	{
		m_blend_mode_d3d11.first ->Show();
		m_blend_mode_d3d11.second->Show();
		m_blend_mode.first ->Hide();
		m_blend_mode.second->Hide();
	}
	else
	{
		m_blend_mode_d3d11.first ->Hide();
		m_blend_mode_d3d11.second->Hide();
		m_blend_mode.first ->Show();
		m_blend_mode.second->Show();
	}
#endif
}

HacksTab::HacksTab(wxWindow* parent)
	: wxPanel(parent, wxID_ANY)
	, m_ui(this)
{
	const int space = wxSizerFlags().Border().GetBorderInPixels();
	PaddedBoxSizer<wxBoxSizer> tab_box(wxVERTICAL);

	auto* hacks_check_box = m_ui.addCheckBox(tab_box.inner, "Enable User Hacks", "UserHacks");
	CheckboxPrereq hacks_check(hacks_check_box);
	auto upscale_hacks_prereq = [this, hacks_check_box]{ return !m_is_native_res && hacks_check_box->GetValue(); };

	PaddedBoxSizer<wxStaticBoxSizer> rend_hacks_box   (wxVERTICAL, this, "Renderer Hacks");
	PaddedBoxSizer<wxStaticBoxSizer> upscale_hacks_box(wxVERTICAL, this, "Upscale Hacks");

	auto* rend_hacks_grid    = new wxFlexGridSizer(2, space, space);
	auto* upscale_hacks_grid = new wxFlexGridSizer(3, space, space);

	// Renderer Hacks
	m_ui.addCheckBox(rend_hacks_grid, "Auto Flush",                "UserHacks_AutoFlush",                  IDC_AUTO_FLUSH_HW,     hacks_check);
	m_ui.addCheckBox(rend_hacks_grid, "Fast Texture Invalidation", "UserHacks_DisablePartialInvalidation", IDC_FAST_TC_INV,       hacks_check);
	m_ui.addCheckBox(rend_hacks_grid, "Disable Depth Emulation",   "UserHacks_DisableDepthSupport",        IDC_TC_DEPTH,          hacks_check);
	m_ui.addCheckBox(rend_hacks_grid, "Frame Buffer Conversion",   "UserHacks_CPU_FB_Conversion",          IDC_CPU_FB_CONVERSION, hacks_check);
	m_ui.addCheckBox(rend_hacks_grid, "Disable Safe Features",     "UserHacks_Disable_Safe_Features",      IDC_SAFE_FEATURES,     hacks_check);
	m_ui.addCheckBox(rend_hacks_grid, "Memory Wrapping",           "wrap_gs_mem",                          IDC_MEMORY_WRAPPING,   hacks_check);
	m_ui.addCheckBox(rend_hacks_grid, "Preload Frame Data",        "preload_frame_with_gs_data",           IDC_PRELOAD_GS,        hacks_check);

	// Upscale
	m_ui.addCheckBox(upscale_hacks_grid, "Align Sprite",   "UserHacks_align_sprite_X",  IDC_ALIGN_SPRITE,    upscale_hacks_prereq);
	m_ui.addCheckBox(upscale_hacks_grid, "Merge Sprite",   "UserHacks_merge_pp_sprite", IDC_MERGE_PP_SPRITE, upscale_hacks_prereq);
	m_ui.addCheckBox(upscale_hacks_grid, "Wild Arms Hack", "UserHacks_WildHack",        IDC_WILDHACK,        upscale_hacks_prereq);

	auto* rend_hack_choice_grid    = new wxFlexGridSizer(2, space, space);
	auto* upscale_hack_choice_grid = new wxFlexGridSizer(2, space, space);
	rend_hack_choice_grid   ->AddGrowableCol(1);
	upscale_hack_choice_grid->AddGrowableCol(1);

	// Renderer Hacks:
	m_ui.addComboBoxAndLabel(rend_hack_choice_grid, "Half Screen Fix:",     "UserHacks_HalfPixelOffset", &theApp.m_gs_generic_list, IDC_HALF_SCREEN_TS, hacks_check);
	m_ui.addComboBoxAndLabel(rend_hack_choice_grid, "Trilinear Filtering:", "UserHacks_TriFilter",       &theApp.m_gs_trifilter,    IDC_TRI_FILTER,     hacks_check);

	// Skipdraw Range
	add_label(this, rend_hack_choice_grid, "Skipdraw Range:", IDC_SKIPDRAWHACK);
	auto* skip_box = new wxBoxSizer(wxHORIZONTAL);
	skip_x_spin = m_ui.addSpin(skip_box, "UserHacks_SkipDraw_Offset", 0, 10000, 0, IDC_SKIPDRAWOFFSET, hacks_check);
	skip_y_spin = m_ui.addSpin(skip_box, "UserHacks_SkipDraw",        0, 10000, 0, IDC_SKIPDRAWHACK,   hacks_check);

	rend_hack_choice_grid->Add(skip_box, wxSizerFlags().Expand());

	// Upscale Hacks:
	m_ui.addComboBoxAndLabel(upscale_hack_choice_grid, "Half-Pixel Offset:", "UserHacks_Half_Bottom_Override", &theApp.m_gs_offset_hack, IDC_OFFSETHACK,   upscale_hacks_prereq);
	m_ui.addComboBoxAndLabel(upscale_hack_choice_grid, "Round Sprite:",      "UserHacks_round_sprite_offset",  &theApp.m_gs_hack,        IDC_ROUND_SPRITE, upscale_hacks_prereq);

	// Texture Offsets
	add_label(this, upscale_hack_choice_grid, "Texture Offsets:", IDC_TCOFFSETX);
	auto* tex_off_box = new wxBoxSizer(wxHORIZONTAL);
	add_label(this, tex_off_box, "X:", IDC_TCOFFSETX, wxSizerFlags().Centre());
	tex_off_box->AddSpacer(space);
	m_ui.addSpin(tex_off_box, "UserHacks_TCOffsetX", 0, 10000, 0, IDC_TCOFFSETX, hacks_check);
	tex_off_box->AddSpacer(space);
	add_label(this, tex_off_box, "Y:", IDC_TCOFFSETY, wxSizerFlags().Centre());
	tex_off_box->AddSpacer(space);
	m_ui.addSpin(tex_off_box, "UserHacks_TCOffsetY", 0, 10000, 0, IDC_TCOFFSETY, hacks_check);

	upscale_hack_choice_grid->Add(tex_off_box, wxSizerFlags().Expand());

	rend_hacks_box->Add(rend_hacks_grid, wxSizerFlags().Centre());
	rend_hacks_box->AddSpacer(space);
	rend_hacks_box->Add(rend_hack_choice_grid, wxSizerFlags().Expand());

	upscale_hacks_box->Add(upscale_hacks_grid, wxSizerFlags().Centre());
	upscale_hacks_box->AddSpacer(space);
	upscale_hacks_box->Add(upscale_hack_choice_grid, wxSizerFlags().Expand());

	tab_box->Add(rend_hacks_box.outer, wxSizerFlags().Expand());
	tab_box->Add(upscale_hacks_box.outer, wxSizerFlags().Expand());

	SetSizerAndFit(tab_box.outer);
}

void HacksTab::DoUpdate()
{
	m_ui.Update();

	if (skip_x_spin->GetValue() == 0)
		skip_y_spin->SetValue(0);
	if (skip_y_spin->GetValue() < skip_x_spin->GetValue())
		skip_y_spin->SetValue(skip_x_spin->GetValue());
}

RecTab::RecTab(wxWindow* parent)
	: wxPanel(parent, wxID_ANY)
	, m_ui(this)
{
	const int space = wxSizerFlags().Border().GetBorderInPixels();
	PaddedBoxSizer<wxBoxSizer> tab_box(wxVERTICAL);

	auto* record_check = m_ui.addCheckBox(tab_box.inner, "Enable Recording (F12)", "capture_enabled");
	CheckboxPrereq record_prereq(record_check);
	PaddedBoxSizer<wxStaticBoxSizer> record_box(wxVERTICAL, this, "Recording");
	auto* record_grid_box = new wxFlexGridSizer(2, space, space);
	record_grid_box->AddGrowableCol(1);

	// Resolution
	add_label(this, record_grid_box, "Resolution:");
	auto* res_box = new wxBoxSizer(wxHORIZONTAL);
	m_ui.addSpin(res_box, "CaptureWidth",  256, 8192, 640, -1, record_prereq);
	m_ui.addSpin(res_box, "CaptureHeight", 256, 8192, 480, -1, record_prereq);

	record_grid_box->Add(res_box, wxSizerFlags().Expand());

	m_ui.addSpinAndLabel(record_grid_box, "Saving Threads:",        "capture_threads",       1, 32, 4, -1, record_prereq);
	m_ui.addSpinAndLabel(record_grid_box, "PNG Compression Level:", "png_compression_level", 1,  9, 1, -1, record_prereq);

	m_ui.addDirPickerAndLabel(record_grid_box, "Output Directory:", "capture_out_dir", -1, record_prereq);

	record_box->Add(record_grid_box, wxSizerFlags().Expand());

	tab_box->Add(record_box.outer, wxSizerFlags().Expand());
	SetSizerAndFit(tab_box.outer);
}

PostTab::PostTab(wxWindow* parent)
	: wxPanel(parent, wxID_ANY)
	, m_ui(this)
{
	const int space = wxSizerFlags().Border().GetBorderInPixels();
	PaddedBoxSizer<wxBoxSizer> tab_box(wxVERTICAL);
	PaddedBoxSizer<wxStaticBoxSizer> shader_box(wxVERTICAL, this, "Custom Shader");

	m_ui.addCheckBox(shader_box.inner, "Texture Filtering of Display", "linear_present", IDC_LINEAR_PRESENT);
	m_ui.addCheckBox(shader_box.inner, "FXAA Shader (PgUp)",           "fxaa",           IDC_FXAA);

	CheckboxPrereq shade_boost_check(m_ui.addCheckBox(shader_box.inner, "Enable Shade Boost", "ShadeBoost", IDC_SHADEBOOST));

	PaddedBoxSizer<wxStaticBoxSizer> shade_boost_box(wxVERTICAL, this, "Shade Boost");
	auto* shader_boost_grid = new wxFlexGridSizer(2, space, space);
	shader_boost_grid->AddGrowableCol(1);

	m_ui.addSliderAndLabel(shader_boost_grid, "Brightness:", "ShadeBoost_Brightness", 0, 100, 50, -1, shade_boost_check);
	m_ui.addSliderAndLabel(shader_boost_grid, "Contrast:",   "ShadeBoost_Contrast",   0, 100, 50, -1, shade_boost_check);
	m_ui.addSliderAndLabel(shader_boost_grid, "Saturation:", "ShadeBoost_Saturation", 0, 100, 50, -1, shade_boost_check);

	shade_boost_box->Add(shader_boost_grid, wxSizerFlags().Expand());
	shader_box->Add(shade_boost_box.outer, wxSizerFlags().Expand());

	CheckboxPrereq ext_shader_check(m_ui.addCheckBox(shader_box.inner, "Enable External Shader", "shaderfx", IDC_SHADER_FX));

	PaddedBoxSizer<wxStaticBoxSizer> ext_shader_box(wxVERTICAL, this, "External Shader (Home)");
	auto* ext_shader_grid = new wxFlexGridSizer(2, space, space);
	ext_shader_grid->AddGrowableCol(1);

	m_ui.addFilePickerAndLabel(ext_shader_grid, "GLSL fx File:", "shaderfx_glsl", -1, ext_shader_check);
	m_ui.addFilePickerAndLabel(ext_shader_grid, "Config File:",  "shaderfx_conf", -1, ext_shader_check);

	ext_shader_box->Add(ext_shader_grid, wxSizerFlags().Expand());
	shader_box->Add(ext_shader_box.outer, wxSizerFlags().Expand());

	// TV Shader
	auto* tv_box = new wxFlexGridSizer(2, space, space);
	tv_box->AddGrowableCol(1);
	m_ui.addComboBoxAndLabel(tv_box, "TV Shader:", "TVShader", &theApp.m_gs_tv_shaders);
	shader_box->Add(tv_box, wxSizerFlags().Expand());

	tab_box->Add(shader_box.outer, wxSizerFlags().Expand());
	SetSizerAndFit(tab_box.outer);
}

OSDTab::OSDTab(wxWindow* parent)
	: wxPanel(parent, wxID_ANY)
	, m_ui(this)
{
	const int space = wxSizerFlags().Border().GetBorderInPixels();
	PaddedBoxSizer<wxBoxSizer> tab_box(wxVERTICAL);

	PaddedBoxSizer<wxStaticBoxSizer> font_box(wxVERTICAL, this, "Visuals");
	auto* font_grid = new wxFlexGridSizer(2, space, space);
	font_grid->AddGrowableCol(1);

	m_ui.addSliderAndLabel(font_grid, "Scale:",   "osd_scale", 50, 300, 100, -1);

	font_box->Add(font_grid, wxSizerFlags().Expand());
	tab_box->Add(font_box.outer, wxSizerFlags().Expand());

	PaddedBoxSizer<wxStaticBoxSizer> log_box(wxVERTICAL, this, "Log Messages");
	auto* log_grid = new wxFlexGridSizer(2, space, space);
	log_grid->AddGrowableCol(1);

	m_ui.addCheckBox(log_grid, "Show Messages", "osd_show_messages", -1);
	m_ui.addCheckBox(log_grid, "Show Speed", "osd_show_speed", -1);
	m_ui.addCheckBox(log_grid, "Show FPS", "osd_show_fps", -1);
	m_ui.addCheckBox(log_grid, "Show CPU Usage", "osd_show_cpu", -1);
	m_ui.addCheckBox(log_grid, "Show Resolution", "osd_show_resolution", -1);
	m_ui.addCheckBox(log_grid, "Show Statistics", "osd_show_gs_stats", -1);

	log_box->Add(log_grid, wxSizerFlags().Expand());
	tab_box->Add(log_box.outer, wxSizerFlags().Expand());

	SetSizerAndFit(tab_box.outer);
}

DebugTab::DebugTab(wxWindow* parent)
	: wxPanel(parent, wxID_ANY)
	, m_ui(this)
{
	const int space = wxSizerFlags().Border().GetBorderInPixels();
	PaddedBoxSizer<wxBoxSizer> tab_box(wxVERTICAL);

	auto ogl_hw_prereq = [this]{ return m_is_ogl_hw; };

	if (g_Conf->DevMode || IsDevBuild)
	{
		PaddedBoxSizer<wxStaticBoxSizer> debug_box(wxVERTICAL, this, "Debug");
		auto* debug_check_box = new wxWrapSizer(wxHORIZONTAL);
		m_ui.addCheckBox(debug_check_box, "GLSL compilation", "debug_glsl_shader");
		m_ui.addCheckBox(debug_check_box, "Use Debug Device", "debug_device");
		m_ui.addCheckBox(debug_check_box, "Dump GS data", "dump");

		auto* debug_save_check_box = new wxWrapSizer(wxHORIZONTAL);
		m_ui.addCheckBox(debug_save_check_box, "Save RT",      "save");
		m_ui.addCheckBox(debug_save_check_box, "Save Frame",   "savef");
		m_ui.addCheckBox(debug_save_check_box, "Save Texture", "savet");
		m_ui.addCheckBox(debug_save_check_box, "Save Depth",   "savez");

		debug_box->Add(debug_check_box);
		debug_box->Add(debug_save_check_box);

		auto* dump_grid = new wxFlexGridSizer(2, space, space);

		start_dump_spin = m_ui.addSpinAndLabel(dump_grid, "Start of Dump:", "saven", 0, pow(10, 9),    0).first;
		end_dump_spin   = m_ui.addSpinAndLabel(dump_grid, "End of Dump:",   "savel", 0, pow(10, 5), 5000).first;

		debug_box->AddSpacer(space);
		debug_box->Add(dump_grid);

		tab_box->Add(debug_box.outer, wxSizerFlags().Expand());
	}

	PaddedBoxSizer<wxStaticBoxSizer> ogl_box(wxVERTICAL, this, "OpenGL");
	auto* ogl_grid = new wxFlexGridSizer(2, space, space);
	m_ui.addComboBoxAndLabel(ogl_grid, "Geometry Shader:",  "override_geometry_shader",                &theApp.m_gs_generic_list, IDC_GEOMETRY_SHADER_OVERRIDE, ogl_hw_prereq);
	m_ui.addComboBoxAndLabel(ogl_grid, "Image Load Store:", "override_GL_ARB_shader_image_load_store", &theApp.m_gs_generic_list, IDC_IMAGE_LOAD_STORE,         ogl_hw_prereq);
	m_ui.addComboBoxAndLabel(ogl_grid, "Sparse Texture:",   "override_GL_ARB_sparse_texture",          &theApp.m_gs_generic_list, IDC_SPARSE_TEXTURE,           ogl_hw_prereq);
	ogl_box->Add(ogl_grid);

	tab_box->Add(ogl_box.outer, wxSizerFlags().Expand());

	SetSizerAndFit(tab_box.outer);
}

void DebugTab::DoUpdate()
{
	m_ui.Update();
	if (!end_dump_spin || !start_dump_spin)
		return;
	if (end_dump_spin->GetValue() < start_dump_spin->GetValue())
		end_dump_spin->SetValue(start_dump_spin->GetValue());
}

Dialog::Dialog()
	: wxDialog(nullptr, wxID_ANY, "Graphics Settings", wxDefaultPosition, wxDefaultSize, wxCAPTION | wxCLOSE_BOX | wxRESIZE_BORDER)
	, m_ui(this)
{
	const int space = wxSizerFlags().Border().GetBorderInPixels();
	auto* padding = new wxBoxSizer(wxVERTICAL);
	m_top_box = new wxBoxSizer(wxVERTICAL);

	auto* top_grid = new wxFlexGridSizer(2, space, space);
	top_grid->SetFlexibleDirection(wxHORIZONTAL);

	m_renderer_select = m_ui.addComboBoxAndLabel(top_grid, "Renderer:", "Renderer", &theApp.m_gs_renderers).first;
	m_renderer_select->Bind(wxEVT_CHOICE, &Dialog::OnRendererChange, this);

#ifdef _WIN32
	add_label(this, top_grid, "Adapter:");
	m_adapter_select = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, {});
	top_grid->Add(m_adapter_select, wxSizerFlags().Expand());
#endif

	m_ui.addComboBoxAndLabel(top_grid, "Interlacing (F5):", "interlace", &theApp.m_gs_interlace);
	m_ui.addComboBoxAndLabel(top_grid, "Texture Filtering:", "filter", &theApp.m_gs_bifilter, IDC_FILTER);

	auto* book = new wxNotebook(this, wxID_ANY, wxDefaultPosition, wxDefaultSize);

	m_renderer_panel = new RendererTab(book);
	m_hacks_panel = new HacksTab(book);
	m_rec_panel = new RecTab(book);
	m_post_panel = new PostTab(book);
	m_osd_panel = new OSDTab(book);
	m_debug_panel = new DebugTab(book);

	book->AddPage(m_renderer_panel, "Renderer", true);
	book->AddPage(m_hacks_panel, "Hacks");
	book->AddPage(m_post_panel, "Shader");
	book->AddPage(m_osd_panel, "OSD");
	book->AddPage(m_rec_panel, "Recording");
	book->AddPage(m_debug_panel, "Advanced");

	m_top_box->Add(top_grid, wxSizerFlags().Centre());
	m_top_box->Add(book, wxSizerFlags().Expand());

	padding->Add(m_top_box, wxSizerFlags().Expand().Border());

	m_top_box->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL), wxSizerFlags().Right());

	SetSizerAndFit(padding);
	Bind(wxEVT_CHECKBOX, &Dialog::CallUpdate, this);
	Bind(wxEVT_SPINCTRL, &Dialog::CallUpdate, this);
	Bind(wxEVT_CHOICE,   &Dialog::CallUpdate, this);
}

Dialog::~Dialog()
{
}

void Dialog::CallUpdate(wxCommandEvent&)
{
	Update();
}

void Dialog::OnRendererChange(wxCommandEvent&)
{
	RendererChange();
	Update();
}

GSRendererType Dialog::GetSelectedRendererType()
{
	int index = m_renderer_select->GetSelection();

	// there is no currently selected renderer or the combo box has more entries than the renderer list or the current selection is negative
	// make sure you haven't made a mistake initializing everything
	ASSERT(index < static_cast<int>(theApp.m_gs_renderers.size()) || index >= 0);

	const GSRendererType type = static_cast<GSRendererType>(
		theApp.m_gs_renderers[index].value
	);

	return type;
}

void Dialog::RendererChange()
{
#ifdef _WIN32
	GSRendererType renderer = GetSelectedRendererType();
	m_adapter_select->Clear();

	if (renderer == GSRendererType::DX11)
	{
		auto factory = D3D::CreateFactory(false);
		auto adapter_list = D3D::GetAdapterList(factory.get());

		const std::string current_adapter(theApp.GetConfigS("adapter"));

		for (const auto name : adapter_list)
		{
			m_adapter_select->Insert(
				convert_utf8_to_utf16(name), m_adapter_select->GetCount()
			);
			if (current_adapter == name)
				m_adapter_select->SetSelection(m_adapter_select->GetCount() - 1);
		}

		m_adapter_select->Enable();
	}
	else
	{
		m_adapter_select->Disable();
	}
	m_renderer_panel->UpdateBlendMode(renderer);

	m_renderer_panel->Layout(); // The version of wx we use on Windows is dumb and something prevents relayout from happening to notebook pages
#endif
}

void Dialog::Load()
{
	m_ui.Load();
#ifdef _WIN32
	GSRendererType renderer = GSRendererType(theApp.GetConfigI("Renderer"));
	if (renderer == GSRendererType::Auto)
		renderer = D3D::ShouldPreferD3D() ? GSRendererType::DX11 : GSRendererType::OGL;
	m_renderer_select->SetSelection(get_config_index(theApp.m_gs_renderers, static_cast<int>(renderer)));
#endif

	RendererChange();

	m_hacks_panel->Load();
	m_renderer_panel->Load();
	m_rec_panel->Load();
	m_post_panel->Load();
	m_osd_panel->Load();
	m_debug_panel->Load();
}

void Dialog::Save()
{
	m_ui.Save();
#ifdef _WIN32
	// only save the adapter when it makes sense to
	// prevents changing the adapter, switching to another renderer and saving
	if (GetSelectedRendererType() == GSRendererType::DX11)
		theApp.SetConfig("adapter", m_adapter_select->GetStringSelection().c_str());
#endif

	m_hacks_panel->Save();
	m_renderer_panel->Save();
	m_rec_panel->Save();
	m_post_panel->Save();
	m_osd_panel->Save();
	m_debug_panel->Save();
}

void Dialog::Update()
{
	GSRendererType renderer = GetSelectedRendererType();
	if (renderer == GSRendererType::Null)
	{
		m_ui.DisableAll();
		m_renderer_select->Enable();
		m_hacks_panel->m_ui.DisableAll();
		m_renderer_panel->m_ui.DisableAll();
		m_rec_panel->m_ui.DisableAll();
		m_post_panel->m_ui.DisableAll();
		m_osd_panel->m_ui.DisableAll();
		m_debug_panel->m_ui.DisableAll();
	}
	else
	{
		// cross-tab dependencies yay
		bool is_hw = renderer == GSRendererType::OGL || renderer == GSRendererType::DX11 || renderer == GSRendererType::VK;
		bool is_upscale = m_renderer_panel->m_internal_resolution->GetSelection() != 0;
		m_hacks_panel->m_is_native_res = !is_hw || !is_upscale;
		m_renderer_panel->m_is_hardware = is_hw;
		m_debug_panel->m_is_ogl_hw = renderer == GSRendererType::OGL;

		m_ui.Update();
		m_hacks_panel->DoUpdate();
		m_renderer_panel->DoUpdate();
		m_rec_panel->DoUpdate();
		m_post_panel->DoUpdate();
		m_osd_panel->DoUpdate();
		m_debug_panel->DoUpdate();
	}
}

bool RunwxDialog()
{
	Dialog GSSettingsDialog;

	GSSettingsDialog.Load();
	GSSettingsDialog.Update();
	if (GSSettingsDialog.ShowModal() == wxID_OK)
		GSSettingsDialog.Save();

	return true;
}
