/*
 *	Copyright (C) 2020 Pcsx2 Team
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "GSwxDialog.h"

void add_settings_to_array_string(const std::vector<GSSetting>& s, wxArrayString &arr)
{
    for(const GSSetting &setting : s)
    {
        if (!setting.note.empty()) 
            arr.Add(setting.name + " (" + setting.note + ")");
        else
            arr.Add(setting.name);
    }
}

size_t get_config_index(const std::vector<GSSetting>& s, const char *str)
{
    int value = theApp.GetConfigI(str);

	for(size_t i = 0; i < s.size(); i++)
    {
        if (s[i].value == value) return i;
    }
    return 0;
}

void add_label(wxWindow *parent, wxSizer *sizer, const char *str, long style = wxALIGN_RIGHT | wxALIGN_CENTRE_HORIZONTAL, wxSizerFlags flags = wxSizerFlags().Right().DoubleBorder())
{
    auto *temp_text = new wxStaticText(parent, wxID_ANY, str, wxDefaultPosition, wxDefaultSize, style);
    sizer->Add(temp_text, flags);
}

void add_combo_box(wxWindow *parent, wxSizer *sizer, wxChoice *&choice, const std::vector<GSSetting>& s, wxSizerFlags flags = wxSizerFlags().Expand().Left())
{
    wxArrayString temp;
    add_settings_to_array_string(s, temp);
    choice = new wxChoice(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, temp);
    sizer->Add(choice, flags);
}

void add_label_and_combo_box(wxWindow *parent, wxSizer *sizer, wxChoice *&choice, const char *str, const std::vector<GSSetting>& s)
{
    add_label(parent, sizer, str);
    add_combo_box(parent, sizer, choice, s);
}

RendererTab::RendererTab(wxWindow *parent)
    : wxPanel(parent, wxID_ANY)
{
    auto *tab_box = new wxBoxSizer(wxVERTICAL);
    auto *hardware_box = new wxStaticBoxSizer(wxVERTICAL, this, "Hardware Mode");
    auto *software_box = new wxStaticBoxSizer(wxVERTICAL, this, "Software Mode");

    eight_bit_check = new wxCheckBox(this, wxID_ANY, "Allow 8 bit textures");
    framebuffer_check = new wxCheckBox(this, wxID_ANY, "Large Framebuffer");

    auto *hw_choice_grid = new wxGridSizer(2, 0, 0);

    add_label_and_combo_box(this, hw_choice_grid, m_res_select, "Internal Resolution:", theApp.m_gs_upscale_multiplier);
    add_label_and_combo_box(this, hw_choice_grid, m_anisotropic_select, "Anisotropic Filtering:", theApp.m_gs_hw_mipmapping);
    add_label_and_combo_box(this, hw_choice_grid, m_mipmap_select, "Mipmapping (Insert):", theApp.m_gs_max_anisotropy);
    add_label_and_combo_box(this, hw_choice_grid, m_crc_select, "CRC Hack Level:", theApp.m_gs_crc_level);
    add_label_and_combo_box(this, hw_choice_grid, m_date_select, "DATE Accuracy:", theApp.m_gs_acc_date_level);
    add_label_and_combo_box(this, hw_choice_grid, m_blend_select, "Blending Accuracy:", theApp.m_gs_acc_blend_level);

    auto *top_checks_box = new wxWrapSizer(wxHORIZONTAL);
    top_checks_box->Add(eight_bit_check, wxSizerFlags().Centre());
    top_checks_box->Add(framebuffer_check, wxSizerFlags().Centre());

    hardware_box->Add(top_checks_box, wxSizerFlags().Centre());
    hardware_box->Add(hw_choice_grid, wxSizerFlags().Centre());

    auto *bottom_checks_box = new wxWrapSizer(wxHORIZONTAL);
    flush_check = new wxCheckBox(this, wxID_ANY, "Auto Flush");
    edge_check = new wxCheckBox(this, wxID_ANY, "Edge Antialiasing (Del)");
    mipmap_check = new wxCheckBox(this, wxID_ANY, "Mipmapping");

    bottom_checks_box->Add(flush_check, wxSizerFlags().Centre());
    bottom_checks_box->Add(edge_check, wxSizerFlags().Centre());
    bottom_checks_box->Add(mipmap_check, wxSizerFlags().Centre());
    software_box->Add(bottom_checks_box, wxSizerFlags().Centre());

    // Rendering threads
    auto *thread_box = new wxBoxSizer(wxHORIZONTAL);
    
    add_label(this, thread_box, "Extra Rendering threads:");
    thread_spin = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 32, 2);
    thread_box->Add(thread_spin, wxSizerFlags().Centre());
    software_box->Add(thread_box, wxSizerFlags().Centre());

    tab_box->Add(hardware_box, wxSizerFlags().Centre().Expand());
    tab_box->Add(software_box, wxSizerFlags().Centre().Expand());

    SetSizerAndFit(tab_box);
}

void RendererTab::Load()
{
    // All values checked.
    eight_bit_check->SetValue(theApp.GetConfigB("paltex"));
    framebuffer_check->SetValue(theApp.GetConfigB("large_framebuffer"));
    flush_check->SetValue(theApp.GetConfigB("autoflush_sw"));
    edge_check->SetValue(theApp.GetConfigB("aa1"));
    mipmap_check->SetValue(theApp.GetConfigB("mipmap"));

    m_res_select->SetSelection(get_config_index(theApp.m_gs_upscale_multiplier, "upscale_multiplier"));
    m_anisotropic_select->SetSelection(get_config_index(theApp.m_gs_max_anisotropy, "MaxAnisotropy"));
    m_mipmap_select->SetSelection(get_config_index(theApp.m_gs_hw_mipmapping, "mipmap_hw"));
    m_crc_select->SetSelection(get_config_index(theApp.m_gs_crc_level, "crc_hack_level"));
    m_date_select->SetSelection(get_config_index(theApp.m_gs_acc_date_level, "accurate_date"));
    m_blend_select->SetSelection(get_config_index(theApp.m_gs_acc_blend_level, "accurate_blending_unit"));
    thread_spin->SetValue(theApp.GetConfigI("extrathreads"));
}

void RendererTab::Save()
{

}

HacksTab::HacksTab(wxWindow *parent)
    : wxPanel(parent, wxID_ANY)
{
    auto *tab_box = new wxBoxSizer(wxVERTICAL);

    hacks_check = new wxCheckBox(this, wxID_ANY, "Enable User Hacks");
    
    auto *rend_hacks_box = new wxStaticBoxSizer(wxVERTICAL, this, "Renderer Hacks");
    auto *upscale_hacks_box = new wxStaticBoxSizer(wxVERTICAL, this, "Upscale Hacks");

    auto *rend_hacks_grid = new wxFlexGridSizer(2, 0, 0);
    auto *upscale_hacks_grid = new wxFlexGridSizer(3, 0, 0);

    // Renderer Hacks
    auto_flush_check = new wxCheckBox(this, wxID_ANY, "Auto Flush");
    fast_inv_check = new wxCheckBox(this, wxID_ANY, "Fast Texture Invalidation");
    dis_depth_check = new wxCheckBox(this, wxID_ANY, "Disable Depth Emulation");
    fb_convert_check = new wxCheckBox(this, wxID_ANY, "Frame Buffer Conversion");
    dis_safe_features_check = new wxCheckBox(this, wxID_ANY, "Disable Safe Features");
    mem_wrap_check = new wxCheckBox(this, wxID_ANY, "Memory Wrapping");
    preload_gs_check = new wxCheckBox(this, wxID_ANY, "Preload Frame Data");

    // Upscale
    align_sprite_check = new wxCheckBox(this, wxID_ANY, "Align Sprite");
    merge_sprite_check = new wxCheckBox(this, wxID_ANY, "Merge Sprite");
    wild_arms_check = new wxCheckBox(this, wxID_ANY, "Wild Arms Hack");

    rend_hacks_grid->Add(auto_flush_check);
    rend_hacks_grid->Add(fast_inv_check);
    rend_hacks_grid->Add(dis_depth_check);
    rend_hacks_grid->Add(fb_convert_check);
    rend_hacks_grid->Add(dis_safe_features_check);
    rend_hacks_grid->Add(mem_wrap_check);
    rend_hacks_grid->Add(preload_gs_check);

    upscale_hacks_grid->Add(align_sprite_check);
    upscale_hacks_grid->Add(merge_sprite_check);
    upscale_hacks_grid->Add(wild_arms_check);

    auto *rend_hack_choice_grid = new wxFlexGridSizer(2, 0, 0);
    auto *upscale_hack_choice_grid = new wxFlexGridSizer(2, 0, 0);

    // Renderer Hacks:
    add_label_and_combo_box(this, rend_hack_choice_grid, m_half_select, "Half Screen Fix:", theApp.m_gs_generic_list);
    add_label_and_combo_box(this, rend_hack_choice_grid, m_tri_select, "Trilinear Filtering:", theApp.m_gs_trifilter);

    // Skipdraw Range
    add_label(this, rend_hack_choice_grid, "Skipdraw Range:");
    auto *skip_box = new wxBoxSizer(wxHORIZONTAL);
    skip_x_spin = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 10000, 0);
    skip_y_spin = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 10000, 0);
    skip_box->Add(skip_x_spin);
    skip_box->Add(skip_y_spin);

    rend_hack_choice_grid->Add(skip_box);

    // Upscale Hacks:
    add_label_and_combo_box(this, upscale_hack_choice_grid, m_gs_offset_hack_select, "Half-Pixel Offset:", theApp.m_gs_offset_hack);
    add_label_and_combo_box(this, upscale_hack_choice_grid, m_round_hack_select, "Round Sprite:", theApp.m_gs_hack);

    // Texture Offsets
    add_label(this, upscale_hack_choice_grid, "Texture Offsets:");
    auto *tex_off_box = new wxBoxSizer(wxHORIZONTAL);
    tex_off_x_spin = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 10000, 0);
    tex_off_y_spin = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 10000, 0);
    tex_off_box->Add(tex_off_x_spin);
    tex_off_box->Add(tex_off_y_spin);

    upscale_hack_choice_grid->Add(tex_off_box);

    rend_hacks_box->Add(rend_hacks_grid);
    rend_hacks_box->Add(new wxStaticLine(this, wxID_ANY), wxSizerFlags().Centre().Expand().Border(5));
    rend_hacks_box->Add(rend_hack_choice_grid, wxSizerFlags().Expand());

    upscale_hacks_box->Add(upscale_hacks_grid);
    upscale_hacks_box->Add(new wxStaticLine(this, wxID_ANY), wxSizerFlags().Centre().Expand().Border(5));
    upscale_hacks_box->Add(upscale_hack_choice_grid, wxSizerFlags().Expand());

    tab_box->Add(hacks_check, wxSizerFlags().Left());
    tab_box->Add(rend_hacks_box, wxSizerFlags().Centre().Expand());
    tab_box->Add(upscale_hacks_box, wxSizerFlags().Centre().Expand());

    SetSizerAndFit(tab_box);
}

void HacksTab::Load()
{
    hacks_check->SetValue(theApp.GetConfigB("UserHacks"));

    align_sprite_check->SetValue(theApp.GetConfigB("UserHacks_align_sprite_X"));
    fb_convert_check->SetValue(theApp.GetConfigB("UserHacks_CPU_FB_Conversion"));
    auto_flush_check->SetValue(theApp.GetConfigB("UserHacks_AutoFlush"));
    mem_wrap_check->SetValue(theApp.GetConfigB("wrap_gs_mem"));
    dis_depth_check->SetValue(theApp.GetConfigB("UserHacks_DisableDepthSupport"));
    merge_sprite_check->SetValue(theApp.GetConfigB("UserHacks_merge_pp_sprite"));
    dis_safe_features_check->SetValue(theApp.GetConfigB("UserHacks_Disable_Safe_Features"));
    preload_gs_check->SetValue(theApp.GetConfigB("preload_frame_with_gs_data"));
    fast_inv_check->SetValue(theApp.GetConfigB("UserHacks_DisablePartialInvalidation"));
    wild_arms_check->SetValue(theApp.GetConfigB("UserHacks_WildHack"));

    m_half_select->SetSelection(get_config_index(theApp.m_gs_offset_hack, "UserHacks_HalfPixelOffset"));
    m_tri_select->SetSelection(get_config_index(theApp.m_gs_trifilter, "UserHacks_TriFilter"));
    m_gs_offset_hack_select->SetSelection(get_config_index(theApp.m_gs_generic_list, "UserHacks_Half_Bottom_Override"));
    m_round_hack_select->SetSelection(get_config_index(theApp.m_gs_hack, "UserHacks_round_sprite_offset"));

    skip_x_spin->SetValue(theApp.GetConfigI("UserHacks_SkipDraw_Offset"));
    skip_y_spin->SetValue(theApp.GetConfigI("UserHacks_SkipDraw"));
    tex_off_x_spin->SetValue(theApp.GetConfigI("UserHacks_TCOffsetX"));
    tex_off_y_spin->SetValue(theApp.GetConfigI("UserHacks_TCOffsetY"));
}

void HacksTab::Save()
{
    
}

RecTab::RecTab(wxWindow *parent)
    : wxPanel(parent, wxID_ANY)
{
    auto *tab_box = new wxBoxSizer(wxVERTICAL);

    record_check = new wxCheckBox(this, wxID_ANY, "Enable Recording (with F12)");
    auto *record_box = new wxStaticBoxSizer(wxVERTICAL, this, "Recording");
    auto *record_grid_box = new wxFlexGridSizer(2, 0, 0);

    // Resolution
    add_label(this, record_grid_box, "Resolution:");
    auto *res_box = new wxBoxSizer(wxHORIZONTAL);
    res_x_spin = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 256, 8192, 640);
    res_y_spin = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 256, 8192, 480);
    res_box->Add(res_x_spin);
    res_box->Add(res_y_spin);

    record_grid_box->Add(res_box);

    add_label(this, record_grid_box, "Saving Threads:");
    thread_spin = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 32, 4);
    record_grid_box->Add(thread_spin);

    add_label(this, record_grid_box, "PNG Compression Level:");
    png_spin = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 9, 1);
    record_grid_box->Add(png_spin);

    add_label(this, record_grid_box, "Output Directory:");
    dir_select = new wxDirPickerCtrl(this, wxID_ANY);
    record_grid_box->Add(dir_select, wxSizerFlags().Expand());

    record_box->Add(record_grid_box, wxSizerFlags().Centre().Expand());

    tab_box->Add(record_check, wxSizerFlags().Left());
    tab_box->Add(record_box, wxSizerFlags().Centre().Expand());
    SetSizerAndFit(tab_box);
}

void RecTab::Load()
{
    record_check->SetValue(theApp.GetConfigB("capture_enabled"));

    res_x_spin->SetValue(theApp.GetConfigI("CaptureWidth"));
    res_y_spin->SetValue(theApp.GetConfigI("CaptureHeight"));
    thread_spin->SetValue(theApp.GetConfigI("capture_threads"));
    png_spin->SetValue(theApp.GetConfigI("png_compression_level"));

    dir_select->SetInitialDirectory(theApp.GetConfigS("capture_out_dir"));
}

void RecTab::Save()
{
    
}

PostTab::PostTab(wxWindow *parent)
    : wxPanel(parent, wxID_ANY)
{
    auto *tab_box = new wxBoxSizer(wxVERTICAL);
    auto *shader_box = new wxStaticBoxSizer(wxVERTICAL, this, "Custom Shader");

    tex_filter_check = new wxCheckBox(this, wxID_ANY, "Texture Filtering of Display");
    fxaa_check = new wxCheckBox(this, wxID_ANY, "FXAA Shader (PgUp)");

    shader_box->Add(tex_filter_check);
    shader_box->Add(fxaa_check);

    shade_boost_check = new wxCheckBox(this, wxID_ANY, "Enable Shade Boost");
    shader_box->Add(shade_boost_check);

    auto *shade_boost_box = new wxStaticBoxSizer(wxVERTICAL, this, "Shade Boost");
    auto *shader_boost_grid = new wxFlexGridSizer(2, 0, 0);
    
    add_label(this, shader_boost_grid, "Brightness:");
    sb_brightness_slider = new wxSlider(this, wxID_ANY, 50, 0, 100, wxDefaultPosition, wxSize(250,-1), wxSL_HORIZONTAL | wxSL_VALUE_LABEL);
    shader_boost_grid->Add(sb_brightness_slider, wxSizerFlags().Expand().Shaped());


    add_label(this, shader_boost_grid, "Contrast:");
    sb_contrast_slider = new wxSlider(this, wxID_ANY, 50, 0, 100, wxDefaultPosition, wxSize(250,-1), wxSL_HORIZONTAL | wxSL_VALUE_LABEL);
    shader_boost_grid->Add(sb_contrast_slider, wxSizerFlags().Centre().Expand());

    add_label(this, shader_boost_grid, "Saturation:");
    sb_saturation_slider = new wxSlider(this, wxID_ANY, 50, 0, 100, wxDefaultPosition, wxSize(250,-1), wxSL_HORIZONTAL | wxSL_VALUE_LABEL);
    shader_boost_grid->Add(sb_saturation_slider, wxSizerFlags().Centre().Expand());

    shade_boost_box->Add(shader_boost_grid, wxSizerFlags().Centre());
    shader_box->Add(shade_boost_box, wxSizerFlags().Expand().Centre());

    ext_shader_check = new wxCheckBox(this, wxID_ANY, "Enable External Shader");
    shader_box->Add(ext_shader_check);

    auto *ext_shader_box = new wxStaticBoxSizer(wxVERTICAL, this, "External Shader (Home)");
    auto *ext_shader_grid = new wxFlexGridSizer(4, 0, 0);

    ext_shader_grid->Add(new wxStaticText(this, wxID_ANY, "GLSL fx File:", wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT), wxSizerFlags().Centre());
    glsl_select = new wxDirPickerCtrl(this, wxID_ANY);
    ext_shader_grid->Add(glsl_select, wxSizerFlags().Expand());

    ext_shader_grid->Add(new wxStaticText(this, wxID_ANY, "Config File:", wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT), wxSizerFlags().Centre());
    config_select = new wxDirPickerCtrl(this, wxID_ANY);
    ext_shader_grid->Add(config_select, wxSizerFlags().Expand());

    ext_shader_box->Add(ext_shader_grid, wxSizerFlags().Centre());
    shader_box->Add(ext_shader_box, wxSizerFlags().Expand().Centre());

    // TV Shader
    auto *tv_box = new wxBoxSizer(wxHORIZONTAL);
    add_label_and_combo_box(this, tv_box, m_tv_select, "TV Shader (F7):", theApp.m_gs_tv_shaders);
    shader_box->Add(tv_box);

    tab_box->Add(shader_box, wxSizerFlags().Centre().Expand());
    SetSizerAndFit(tab_box);
}

void PostTab::Load()
{
    tex_filter_check->SetValue(theApp.GetConfigB("linear_present"));
    fxaa_check->SetValue(theApp.GetConfigB("fxaa"));
    shade_boost_check->SetValue(theApp.GetConfigB("ShadeBoost"));
    ext_shader_check->SetValue(theApp.GetConfigB("shaderfx"));

    m_tv_select->SetSelection(get_config_index(theApp.m_gs_tv_shaders, "TVShader"));

    glsl_select->SetInitialDirectory(theApp.GetConfigS("shaderfx_glsl"));
    config_select->SetInitialDirectory(theApp.GetConfigS("shaderfx_conf"));

    sb_brightness_slider->SetValue(theApp.GetConfigI("ShadeBoost_Brightness"));
    sb_contrast_slider->SetValue(theApp.GetConfigI("ShadeBoost_Contrast"));
    sb_saturation_slider->SetValue(theApp.GetConfigI("ShadeBoost_Saturation"));
}

void PostTab::Save()
{
    
}

OSDTab::OSDTab(wxWindow *parent)
    : wxPanel(parent, wxID_ANY)
{
    auto *tab_box = new wxBoxSizer(wxVERTICAL);

    monitor_check = new wxCheckBox(this, wxID_ANY, "Enable Monitor");
    tab_box->Add(monitor_check);

    auto *font_box = new wxStaticBoxSizer(wxVERTICAL, this, "Font");
    auto *font_grid = new wxFlexGridSizer(2, 0, 0);

    add_label(this, font_grid, "Size:");
    size_spin = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 100, 25);
    font_grid->Add(size_spin, wxSizerFlags().Expand());

    add_label(this, font_grid, "Red:");
    red_slider = new wxSlider(this, wxID_ANY, 0, 0, 255, wxDefaultPosition, wxSize(250,-1), wxSL_HORIZONTAL | wxSL_VALUE_LABEL);
    font_grid->Add(red_slider, wxSizerFlags().Expand().Shaped());

    add_label(this, font_grid, "Green:");
    green_slider = new wxSlider(this, wxID_ANY, 0, 0, 255, wxDefaultPosition, wxSize(250,-1), wxSL_HORIZONTAL | wxSL_VALUE_LABEL);
    font_grid->Add(green_slider, wxSizerFlags().Expand().Shaped());

    add_label(this, font_grid, "Blue:");
    blue_slider = new wxSlider(this, wxID_ANY, 0, 0, 255, wxDefaultPosition, wxSize(250,-1), wxSL_HORIZONTAL | wxSL_VALUE_LABEL);
    font_grid->Add(blue_slider, wxSizerFlags().Expand().Shaped());

    add_label(this, font_grid, "Opacity:");
    opacity_slider = new wxSlider(this, wxID_ANY, 100, 0, 100, wxDefaultPosition, wxSize(250,-1), wxSL_HORIZONTAL | wxSL_VALUE_LABEL);
    font_grid->Add(opacity_slider, wxSizerFlags().Expand().Shaped());

    font_box->Add(font_grid, wxSizerFlags().Centre().Expand());
    tab_box->Add(font_box, wxSizerFlags().Centre());

    log_check = new wxCheckBox(this, wxID_ANY, "Enable Log");
    tab_box->Add(log_check);

    auto *log_box = new wxStaticBoxSizer(wxVERTICAL, this, "Log Messages");
    auto *log_grid = new wxFlexGridSizer(2, 0, 0);

    add_label(this, log_grid, "Timeout (seconds):");
    timeout_spin = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 2, 10, 4);
    log_grid->Add(timeout_spin);

    add_label(this, log_grid, "Max On-Screen Messages:");
    max_spin = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 10, 2);
    log_grid->Add(max_spin);

    log_box->Add(log_grid, wxSizerFlags().Centre().Expand());
    tab_box->Add(log_box, wxSizerFlags().Centre());

    SetSizerAndFit(tab_box);
}

void OSDTab::Load()
{
    monitor_check->SetValue(theApp.GetConfigB("osd_monitor_enabled"));
    log_check->SetValue(theApp.GetConfigB("osd_log_enabled"));

    size_spin->SetValue(theApp.GetConfigI("osd_fontsize"));
    timeout_spin->SetValue(theApp.GetConfigI("osd_log_timeout"));
    max_spin->SetValue(theApp.GetConfigI("osd_max_log_messages"));

    red_slider->SetValue(theApp.GetConfigI("osd_color_r"));
    green_slider->SetValue(theApp.GetConfigI("osd_color_g"));
    blue_slider->SetValue(theApp.GetConfigI("osd_color_b"));
    opacity_slider->SetValue(theApp.GetConfigI("osd_color_opacity"));

}

void OSDTab::Save()
{
    
}

DebugTab::DebugTab(wxWindow *parent)
    : wxPanel(parent, wxID_ANY)
{
    auto *tab_box = new wxBoxSizer(wxVERTICAL);

    auto *debug_box = new wxStaticBoxSizer(wxVERTICAL, this, "Debug");
    auto *debug_check_box = new wxWrapSizer(wxHORIZONTAL);

    glsl_debug_check = new wxCheckBox(this, wxID_ANY, "GLSL compilation");
    debug_check_box->Add(glsl_debug_check);

    gl_debug_check = new wxCheckBox(this, wxID_ANY, "Print GL error");
    debug_check_box->Add(gl_debug_check);

    gs_dump_check = new wxCheckBox(this, wxID_ANY, "Dump GS data");
    debug_check_box->Add(gs_dump_check);

    auto *debug_save_check_box = new wxWrapSizer(wxHORIZONTAL);

    gs_save_check = new wxCheckBox(this, wxID_ANY, "Save RT");
    debug_save_check_box->Add(gs_save_check);

    gs_savef_check = new wxCheckBox(this, wxID_ANY, "Save Frame");
    debug_save_check_box->Add(gs_savef_check);

    gs_savet_check = new wxCheckBox(this, wxID_ANY, "Save Texture");
    debug_save_check_box->Add(gs_savet_check);

    gs_savez_check = new wxCheckBox(this, wxID_ANY, "Save Depth");
    debug_save_check_box->Add(gs_savez_check);

    debug_box->Add(debug_check_box);
    debug_box->Add(debug_save_check_box);

    auto *dump_grid = new wxFlexGridSizer(4, 0, 0);

    add_label(this, dump_grid, "Start of Dump:");
    start_dump_spin = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, pow(10, 9), 0);
    dump_grid->Add(start_dump_spin);

    add_label(this, dump_grid, "End of Dump:");
    end_dump_spin = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, pow(10, 5), 5000);
    dump_grid->Add(end_dump_spin);

    debug_box->Add(dump_grid);

    auto *ogl_box = new wxStaticBoxSizer(wxVERTICAL, this, "OpenGL");
    auto *ogl_grid = new wxFlexGridSizer(2, 0, 0);
    add_label_and_combo_box(this, ogl_grid, m_geo_shader_select, "Geometry Shader:", theApp.m_gs_generic_list);
    add_label_and_combo_box(this, ogl_grid, m_image_load_store_select, "Image Load Store:", theApp.m_gs_generic_list);
    add_label_and_combo_box(this, ogl_grid, m_sparse_select, "Sparse Texture:", theApp.m_gs_generic_list);
    ogl_box->Add(ogl_grid);
    
    tab_box->Add(debug_box, wxSizerFlags().Centre().Expand());
    tab_box->Add(ogl_box, wxSizerFlags().Centre().Expand());

    SetSizerAndFit(tab_box);
}

void DebugTab::Load()
{
    glsl_debug_check->SetValue(theApp.GetConfigB("debug_glsl_shader"));
    gl_debug_check->SetValue(theApp.GetConfigB("debug_opengl"));
    gs_dump_check->SetValue(theApp.GetConfigB("dump"));
    gs_save_check->SetValue(theApp.GetConfigB("save"));
    gs_savef_check->SetValue(theApp.GetConfigB("savef"));
    gs_savet_check->SetValue(theApp.GetConfigB("savet"));
    gs_savez_check->SetValue(theApp.GetConfigB("savez"));

    start_dump_spin->SetValue(theApp.GetConfigI("saven"));
    end_dump_spin->SetValue(theApp.GetConfigI("savel"));

    m_geo_shader_select->SetSelection(get_config_index(theApp.m_gs_generic_list, "override_geometry_shader"));
    m_image_load_store_select->SetSelection(get_config_index(theApp.m_gs_generic_list, "override_GL_ARB_shader_image_load_store"));
    m_sparse_select->SetSelection(get_config_index(theApp.m_gs_generic_list, "override_GL_ARB_sparse_texture"));
}

void DebugTab::Save()
{
    
}

Dialog::Dialog()
    : wxDialog(nullptr, wxID_ANY, "GSdx Config", wxDefaultPosition, wxDefaultSize, wxCAPTION | wxCLOSE_BOX)
{
    auto *padding = new wxBoxSizer(wxVERTICAL);
    m_top_box = new wxBoxSizer(wxVERTICAL);

    // Right now, I'm hardcoding the location of the picture.
	wxString fileLocation = wxStandardPaths::Get().GetExecutablePath();
	fileLocation = wxFileName(fileLocation).GetPath() + wxT("/plugins/logo-ogl.bmp");

    wxBitmap logo(fileLocation, wxBITMAP_TYPE_BMP);
    m_top_box->Add(new wxStaticBitmap(this, wxID_ANY, logo), wxSizerFlags().Centre());

    auto *top_grid = new wxFlexGridSizer(2, 0, 0);
    top_grid->SetFlexibleDirection(wxHORIZONTAL);

    add_label_and_combo_box(this, top_grid, m_renderer_select, "Renderer:", theApp.m_gs_renderers);

    add_label(this, top_grid, "Adapter:");
    wxArrayString m_adapter_str;
    //add_settings_to_array_string(theApp.m_gs_renderers, m_adapter_str);
    m_adapter_str.Add("Default");
    m_adapter_select = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_adapter_str);
    top_grid->Add(m_adapter_select, wxSizerFlags().Expand());
    m_adapter_select->Disable();

    add_label_and_combo_box(this, top_grid, m_interlace_select, "Interlacing(F5):", theApp.m_gs_interlace);
    add_label_and_combo_box(this, top_grid, m_texture_select, "Texture Filtering:", theApp.m_gs_bifilter);

    auto *book = new wxNotebook(this, wxID_ANY, wxDefaultPosition, wxSize(450,-1));

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
    book->AddPage(m_debug_panel, "Debug/OGL");
    book->SetPadding(wxSize(0,0));

    m_top_box->Add(top_grid, wxSizerFlags().Centre());
    m_top_box->Add(book, wxSizerFlags().Centre().Expand());

    padding->Add(m_top_box, wxSizerFlags().Centre().Expand().Border(wxALL, 5));

    SetSizerAndFit(padding);

}

Dialog::~Dialog()
{
}

void Dialog::Load()
{
    m_renderer_select->SetSelection(get_config_index(theApp.m_gs_renderers, "Renderer"));
    m_adapter_select->SetSelection(0);
    m_interlace_select->SetSelection(get_config_index(theApp.m_gs_interlace, "interlace"));
    m_texture_select->SetSelection(get_config_index(theApp.m_gs_bifilter, "filter"));

    m_hacks_panel->Load();
    m_renderer_panel->Load();
}

void Dialog::Save()
{
}

bool RunwxDialog()
{
    Dialog GSSettingsDialog;

    GSSettingsDialog.Load();
    GSSettingsDialog.ShowModal();
    GSSettingsDialog.Save();

    return true;
}
