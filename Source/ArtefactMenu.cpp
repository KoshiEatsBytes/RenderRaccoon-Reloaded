
#include <cfloat>
#include <cstring>
#include <random>
#include <cstdio>
#include <algorithm>
#include <string>

#include "ArtefactMenu.h"
#include "imgui.h"
#include "implot.h"


// PUBLIC --------------------------------------------------------------------------------------------------------------

ArtefactMenu::ArtefactMenu() : Scene("Main Menu Scene") {}

ArtefactMenu::~ArtefactMenu()
= default;

// PROTECTED -----------------------------------------------------------------------------------------------------------

bool ArtefactMenu::Init()
{
    SetCursorEnabled(true);
    SetSceneClearColor({0.0f, 0.0f, 0.0f, 1.0f});
    return true;
}

// Mandatory hooks, unused.
void ArtefactMenu::PreUpdate(float _deltaTime) {}
void ArtefactMenu::Update(float _deltaTime) {}
void ArtefactMenu::LateUpdate(float _deltaTime) {}
void ArtefactMenu::Destroy() {}

void ArtefactMenu::OnGui()
{
    ImGui::GetStyle().FontScaleMain = m_uiScale;

    DrawTopBar();

    if (m_view == TopView::BENCHMARK)
    {
        DrawBenchmarkPanel();
        if (m_selectedBenchmark >= 0) DrawMethodologyPanel();
    }

}

// PRIVATE -------------------------------------------------------------------------------------------------------------

namespace SHARED
{
    constexpr ImGuiWindowFlags kPanelFlags =
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBringToFrontOnFocus;

    constexpr float kLeftSize = 0.2f;
}

// TOP BAR -------------------------------------------------------------------------------------------------------------

namespace TB
{
    bool TabButton(const char* _label, bool _active, const ImVec2& _size = {0,0})
    {
        // Highlight when active
        if (_active) ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
        const bool clicked = ImGui::Button(_label, _size);
        if (_active) ImGui::PopStyleColor();

        return clicked;
    }
}

void ArtefactMenu::DrawTopBar()
{
    const bool idle = m_view == TopView::NONE;
    const ImGuiStyle& style = ImGui::GetStyle();
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2( style.ItemSpacing.x + m_buttonPadding, style.ItemSpacing.y));

    // When nothing is open, enlarge the font slightly
    if (idle)
    {
        const float fontSize    = ImGui::GetFontSize();
        const float scale = style.FontScaleMain * style.FontScaleDpi;
        const float base  = scale > 0.0f ? fontSize / scale : fontSize;

        // Increase font by given multiplier
        ImGui::PushFont(ImGui::GetFont(), base * m_topViewScaleIdle);
    }

    if (!ImGui::BeginMainMenuBar())
    {
        if (idle) ImGui::PopFont();
        return;
    }

    if (TB::TabButton("BENCHMARK", m_view == TopView::BENCHMARK))
    {
        // when clicked close/open benchmark tab
        if (m_view == TopView::BENCHMARK)
        {
            m_view = TopView::NONE;
        }
        else
        {
            m_view = TopView::BENCHMARK;
            m_selectedBenchmark = -1;
        }
    }
    if (TB::TabButton("ANALYZE", m_view == TopView::ANALYZER))
    {
        if (m_view == TopView::ANALYZER)
        {
            m_view = TopView::NONE;
            m_runListDirty = true;
        }
        else
        {
            m_view = TopView::ANALYZER;
        }
    }
    if (TB::TabButton("COMPARE", m_view == TopView::COMPARE))
    {
        if (m_view == TopView::COMPARE)
        {
            m_view = TopView::NONE;
            m_runListDirty = true;
        }
        else
        {
            m_view = TopView::COMPARE;
        }
    }

    // close button and scale slider size
    const float closeWidth  = ImGui::CalcTextSize("CLOSE APP").x + style.FramePadding.x * m_closeBtWidth;
    const float sliderWidth = ImGui::CalcTextSize("UI SCALE: 0.00").x * m_sliderWidth;

    // Scale slider
    ImGui::SameLine(ImGui::GetWindowWidth() - closeWidth - sliderWidth - style.ItemSpacing.x * m_closeBtAlignment);
    ImGui::SetNextItemWidth(sliderWidth);
    ImGui::SliderFloat("##uiscale", &m_uiScalePending, m_uiMinScale, m_uiMaxScale, "UI SCALE: %.1f");
    // Apply scale after releasing
    if (ImGui::IsItemDeactivatedAfterEdit()) m_uiScale = m_uiScalePending;

    // Close Button
    ImGui::SameLine();
    if (ImGui::Button("CLOSE APP")) RR::Engine::GetInstance().SetShouldClose(true);

    ImGui::EndMainMenuBar();
    ImGui::PopStyleVar();
    if (idle) ImGui::PopFont();
}

// BENCHMARK TAB -------------------------------------------------------------------------------------------------------

namespace BT
{
    bool CenteredMenuButton(const char* _label, bool _active, float _height, float _padding)
    {
        if (_active) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
        }

        const ImGuiStyle& style = ImGui::GetStyle();
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,
            ImVec2(style.ItemSpacing.x, style.ItemSpacing.y + _padding));
        ImGui::PushID(_label);

        const bool clicked = ImGui::Button("##b", ImVec2(-FLT_MIN, _height));

        ImGui::PopStyleVar();
        ImGui::PopID();

        if (_active) {
            ImGui::PopStyleColor();
        }

        // button size data
        const float lineHeight = ImGui::GetTextLineHeight();
        const ImVec2 btMin     = ImGui::GetItemRectMin();
        const ImVec2 btMax     = ImGui::GetItemRectMax();
        const float btWidth    = btMax.x - btMin.x;

        // how many times we go newline in 1 title
        int lines = 1;
        for (const char* ch = _label; *ch; ch++)
        {
            if (*ch == '\n') lines++;
        }

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        const ImU32 color    = ImGui::GetColorU32(ImGuiCol_Text);

        // Resize button and text, and centre text when going newline BECAUSE IM GUI DOESN'T FUCKING SUPPORT IT
        float btHeight = btMin.y + (btMax.y - btMin.y - static_cast<float>(lines) * lineHeight) * 0.5f;
        for (const char* start = _label; start; )
        {
            const char* newLine   = std::strchr(start, '\n');
            const char* end       = newLine ? newLine : start + std::strlen(start);
            const float textWidth = ImGui::CalcTextSize(start, end).x;

            drawList->AddText(
                ImVec2(btMin.x + (btWidth - textWidth) * 0.5f, btHeight),
                color, start, end);
            btHeight += lineHeight;
            start = newLine ? newLine + 1 : nullptr;
        }

        return clicked;
    }
}

void ArtefactMenu::DrawBenchmarkPanel()
{
    const ImGuiViewport* viewPort = ImGui::GetMainViewport();
    const float leftWidth = viewPort->WorkSize.x * SHARED::kLeftSize;

    // Position selection tab on the left of benchmark tab
    ImGui::SetNextWindowPos(viewPort->WorkPos);
    ImGui::SetNextWindowSize(ImVec2(leftWidth, viewPort->WorkSize.y));

    if (ImGui::Begin("##benchmark_panel", nullptr, SHARED::kPanelFlags))
    {
        const float btHeight = ImGui::GetFrameHeight() * m_benchmarkBtHeight;

        // Deterministic and custom benchmark - display on top
        if (BT::CenteredMenuButton("DETERMINISTIC\nBENCHMARK", m_selectedBenchmark == 0, btHeight, m_benchBtSpacing))
        {
            m_selectedBenchmark = 0;
        }
        if (BT::CenteredMenuButton("CUSTOM\nBENCHMARK", m_selectedBenchmark == 1, btHeight, m_benchBtSpacing))
        {
            m_selectedBenchmark = 1;
        }

        // Free roam - display on bottom
        ImGui::SetCursorPosY(ImGui::GetWindowHeight() - ImGui::GetStyle().WindowPadding.y - btHeight);
        if (BT::CenteredMenuButton("FREE ROAM", m_selectedBenchmark == 2, btHeight, m_benchBtSpacing))
        {
            m_selectedBenchmark = 2;
        }

        ImGui::End();
    }
}

void ArtefactMenu::DrawMethodologyPanel()
{
}

void ArtefactMenu::DrawAnalyzerPanel()
{
}

void ArtefactMenu::DrawComparePanel()
{
}



void ArtefactMenu::DrawResultWindow()
{
}

void ArtefactMenu::DrawDescription(float _height)
{
}

void ArtefactMenu::DrawOptimizationToggles()
{
}

void ArtefactMenu::DrawSceneSelect()
{
}

void ArtefactMenu::DrawCustomSeed()
{
}

void ArtefactMenu::RefreshRunList()
{
}






























