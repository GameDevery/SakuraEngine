#include "imgui_sail.h"
#include "imgui.h"
#include "common/utils.h"

namespace ImGui
{
namespace Sail
{
extern const unsigned int SourceSansProRegular_compressed_size = 149392;
extern const unsigned int SourceSansProRegular_compressed_data[]; // defined later in the file

void LoadFont(float size)
{
    const char8_t* font_path = u8"./../resources/font/SourceSansPro-Regular.ttf";
    uint32_t *     font_bytes, font_length;
    read_bytes(font_path, &font_bytes, &font_length);
    ImFontConfig cfg = {};
    cfg.SizePixels   = 16.f;
    cfg.OversampleH = cfg.OversampleV = 1;
    cfg.PixelSnapH                    = true;
    ImGui::GetIO().Fonts->AddFontFromMemoryTTF(
        font_bytes,
        font_length,
        cfg.SizePixels,
        &cfg
    );
    ImGui::GetIO().Fonts->Build();
}

void StyleColorsSail()
{
    ImGuiStyle* style   = &ImGui::GetStyle();
    style->GrabRounding = 4.0f;

    ImVec4* colors                         = style->Colors;
    colors[ImGuiCol_Text]                  = ColorConvertU32ToFloat4(Sail::GRAY800); // text on hovered controls is gray900
    colors[ImGuiCol_TextDisabled]          = ColorConvertU32ToFloat4(Sail::GRAY500);
    colors[ImGuiCol_WindowBg]              = ColorConvertU32ToFloat4(Sail::GRAY100);
    colors[ImGuiCol_ChildBg]               = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg]               = ColorConvertU32ToFloat4(Sail::GRAY50); // not sure about this. Note: applies to tooltips too.
    colors[ImGuiCol_Border]                = ColorConvertU32ToFloat4(Sail::GRAY300);
    colors[ImGuiCol_BorderShadow]          = ColorConvertU32ToFloat4(Sail::Static::NONE); // We don't want shadows. Ever.
    colors[ImGuiCol_FrameBg]               = ColorConvertU32ToFloat4(Sail::GRAY75);       // this isnt right, Sail does not do this, but it's a good fallback
    colors[ImGuiCol_FrameBgHovered]        = ColorConvertU32ToFloat4(Sail::GRAY50);
    colors[ImGuiCol_FrameBgActive]         = ColorConvertU32ToFloat4(Sail::GRAY200);
    colors[ImGuiCol_TitleBg]               = ColorConvertU32ToFloat4(Sail::GRAY300); // those titlebar values are totally made up, Sail does not have this.
    colors[ImGuiCol_TitleBgActive]         = ColorConvertU32ToFloat4(Sail::GRAY200);
    colors[ImGuiCol_TitleBgCollapsed]      = ColorConvertU32ToFloat4(Sail::GRAY400);
    colors[ImGuiCol_MenuBarBg]             = ColorConvertU32ToFloat4(Sail::GRAY100);
    colors[ImGuiCol_ScrollbarBg]           = ColorConvertU32ToFloat4(Sail::GRAY100); // same as regular background
    colors[ImGuiCol_ScrollbarGrab]         = ColorConvertU32ToFloat4(Sail::GRAY400);
    colors[ImGuiCol_ScrollbarGrabHovered]  = ColorConvertU32ToFloat4(Sail::GRAY600);
    colors[ImGuiCol_ScrollbarGrabActive]   = ColorConvertU32ToFloat4(Sail::GRAY700);
    colors[ImGuiCol_CheckMark]             = ColorConvertU32ToFloat4(Sail::BLUE500);
    colors[ImGuiCol_SliderGrab]            = ColorConvertU32ToFloat4(Sail::GRAY700);
    colors[ImGuiCol_SliderGrabActive]      = ColorConvertU32ToFloat4(Sail::GRAY800);
    colors[ImGuiCol_Button]                = ColorConvertU32ToFloat4(Sail::GRAY75); // match default button to Sail's 'Action Button'.
    colors[ImGuiCol_ButtonHovered]         = ColorConvertU32ToFloat4(Sail::GRAY50);
    colors[ImGuiCol_ButtonActive]          = ColorConvertU32ToFloat4(Sail::GRAY200);
    colors[ImGuiCol_Header]                = ColorConvertU32ToFloat4(Sail::BLUE400);
    colors[ImGuiCol_HeaderHovered]         = ColorConvertU32ToFloat4(Sail::BLUE500);
    colors[ImGuiCol_HeaderActive]          = ColorConvertU32ToFloat4(Sail::BLUE600);
    colors[ImGuiCol_Separator]             = ColorConvertU32ToFloat4(Sail::GRAY400);
    colors[ImGuiCol_SeparatorHovered]      = ColorConvertU32ToFloat4(Sail::GRAY600);
    colors[ImGuiCol_SeparatorActive]       = ColorConvertU32ToFloat4(Sail::GRAY700);
    colors[ImGuiCol_ResizeGrip]            = ColorConvertU32ToFloat4(Sail::GRAY400);
    colors[ImGuiCol_ResizeGripHovered]     = ColorConvertU32ToFloat4(Sail::GRAY600);
    colors[ImGuiCol_ResizeGripActive]      = ColorConvertU32ToFloat4(Sail::GRAY700);
    colors[ImGuiCol_PlotLines]             = ColorConvertU32ToFloat4(Sail::BLUE400);
    colors[ImGuiCol_PlotLinesHovered]      = ColorConvertU32ToFloat4(Sail::BLUE600);
    colors[ImGuiCol_PlotHistogram]         = ColorConvertU32ToFloat4(Sail::BLUE400);
    colors[ImGuiCol_PlotHistogramHovered]  = ColorConvertU32ToFloat4(Sail::BLUE600);
    colors[ImGuiCol_TextSelectedBg]        = ColorConvertU32ToFloat4((Sail::BLUE400 & 0x00FFFFFF) | 0x33000000);
    colors[ImGuiCol_DragDropTarget]        = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavCursor]             = ColorConvertU32ToFloat4((Sail::GRAY900 & 0x00FFFFFF) | 0x0A000000);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg]     = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);
    colors[ImGuiCol_CheckMark]             = ColorConvertU32ToFloat4(Sail::GRAY50);
    colors[ImGuiCol_Tab]                   = ColorConvertU32ToFloat4(Sail::GRAY300);
    colors[ImGuiCol_TabSelected]           = ColorConvertU32ToFloat4(Sail::BLUE500);
    colors[ImGuiCol_TabHovered]            = ColorConvertU32ToFloat4(Sail::BLUE700);
    colors[ImGuiCol_TabDimmed]             = ColorConvertU32ToFloat4(Sail::GRAY400);
    colors[ImGuiCol_TabDimmedSelected]     = ColorConvertU32ToFloat4(Sail::BLUE700);
}

} // namespace Sail
} // namespace ImGui