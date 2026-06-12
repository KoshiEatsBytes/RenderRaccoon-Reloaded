
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
    else if (m_view == TopView::ANALYZER)
    {
        DrawAnalyzerPanel();
        DrawResultWindows();
    }

}

// PRIVATE -------------------------------------------------------------------------------------------------------------

namespace SHARED
{
    constexpr ImGuiWindowFlags kPanelFlags =
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBringToFrontOnFocus;

    constexpr float kLeftSize = 0.2f;
    constexpr int kMaxOpenWindows = 2;

    const ImVec4 kLabelColor = ImVec4(0.70f, 0.70f, 0.73f, 1.0f);
    const ImVec4 kValueColor = ImVec4(0.97f, 0.82f, 0.40f, 1.0f);

    // Per-run accent Okabe-Ito (donwloaded from internet)
    ImVec4 RunColor(int _id)
    {
        static const ImVec4 kPalette[] = {
            ImVec4(0.902f, 0.624f, 0.000f, 1.0f),
            ImVec4(0.337f, 0.706f, 0.914f, 1.0f),
            ImVec4(0.000f, 0.620f, 0.451f, 1.0f),
            ImVec4(0.941f, 0.894f, 0.259f, 1.0f),
            ImVec4(0.000f, 0.447f, 0.698f, 1.0f),
            ImVec4(0.835f, 0.369f, 0.000f, 1.0f),
            ImVec4(0.800f, 0.475f, 0.655f, 1.0f),
        };
        return kPalette[_id % IM_ARRAYSIZE(kPalette)];
    }

    float GetBaseFontSize()
    {
        const ImGuiStyle& style = ImGui::GetStyle();
        const float fontSize = ImGui::GetFontSize();
        const float scale    = style.FontScaleMain * style.FontScaleDpi;
        const float base     = scale > 0.0f ? fontSize / scale : fontSize;
        return base;
    }

    // Center text in the current window
    void CenteredText(const char* _text, float _size = 1.0f)
    {
        ImGui::PushFont(ImGui::GetFont(), GetBaseFontSize() * _size);
        const float textWidth = ImGui::CalcTextSize(_text).x;

        // Centre
        ImGui::SetCursorPosX((ImGui::GetWindowWidth() - textWidth) * 0.5f);
        ImGui::TextUnformatted(_text);
        ImGui::PopFont();
    }

    void CenteredTitle(const char* _text, float _size = 1.5f)
    {
        CenteredText(_text, _size);
    }

    bool TabButton(const char* _label, bool _active, const ImVec2& _size = {0,0})
    {
        // Highlight when active
        if (_active) ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
        const bool clicked = ImGui::Button(_label, _size);
        if (_active) ImGui::PopStyleColor();

        return clicked;
    }

    // Display label for a run "NAME MM/DD HH:MM:SS"
    std::string PrettyName(const std::string& _file, const std::string& _name, const std::string& _scene)
    {
        auto isDig = [](char ch) { return ch >= '0' && ch <= '9'; };
        std::vector<std::string> nums;

        for (std::size_t i = 0; i < _file.size(); )
            if (isDig(_file[i]))
            {
                std::size_t j = i;
                while (j < _file.size() && isDig(_file[j])) ++j;
                nums.push_back(_file.substr(i, j - i));
                i = j;
            }
            else ++i;

        // Run name, fall back to scene if invalid
        std::string out = !_name.empty() ? _name : (_scene.empty() ? "run" : _scene);

        // Drop year
        if (nums.size() >= 6)
            out += " " + nums[1] + "/" + nums[2] + " " + nums[3] + ":" + nums[4] + ":" + nums[5];

        return out;
    }
}

// TOP BAR -------------------------------------------------------------------------------------------------------------

void ArtefactMenu::DrawTopBar()
{
    const bool idle = m_view == TopView::NONE;
    const ImGuiStyle& style = ImGui::GetStyle();
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2( style.ItemSpacing.x + m_buttonPadding, style.ItemSpacing.y));

    // When nothing is open, enlarge the font slightly
    if (idle)
    {
        // Increase font by given multiplier
        ImGui::PushFont(ImGui::GetFont(), SHARED::GetBaseFontSize() * m_topViewScaleIdle);
    }

    if (!ImGui::BeginMainMenuBar())
    {
        if (idle) ImGui::PopFont();
        return;
    }

    if (SHARED::TabButton("BENCHMARK", m_view == TopView::BENCHMARK))
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
    if (SHARED::TabButton("ANALYZE", m_view == TopView::ANALYZER))
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
    if (SHARED::TabButton("COMPARE", m_view == TopView::COMPARE))
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
    bool CenteredMenuButton(const char* _label, bool _active, float _height, float _padding, float _fSize)
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
        ImGui::PushFont(ImGui::GetFont(), SHARED::GetBaseFontSize() * _fSize);

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
        ImGui::PopFont();

        return clicked;
    }

    unsigned int RandomSeed()
    {
        static std::mt19937 rng{ std::random_device{}() };
        return rng();
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
        if (BT::CenteredMenuButton("DETERMINISTIC\nBENCHMARK", m_selectedBenchmark == 0,
            btHeight, m_benchBtSpacing, m_benchBtFontSize))
        {
            m_selectedBenchmark = 0;
        }
        if (BT::CenteredMenuButton("CUSTOM\nBENCHMARK", m_selectedBenchmark == 1,
            btHeight, m_benchBtSpacing, m_benchBtFontSize))
        {
            m_selectedBenchmark = 1;
        }

        // Free roam - display on bottom
        ImGui::SetCursorPosY(ImGui::GetWindowHeight() - ImGui::GetStyle().WindowPadding.y - btHeight);
        if (BT::CenteredMenuButton("FREE ROAM", m_selectedBenchmark == 2,
            btHeight, m_benchBtSpacing, m_benchBtFontSize))
        {
            m_selectedBenchmark = 2;
        }

        ImGui::End();
    }
}

void ArtefactMenu::DrawMethodologyPanel()
{
    const ImGuiViewport* viewPort = ImGui::GetMainViewport();
    const float leftWidth = viewPort->WorkSize.x * SHARED::kLeftSize;
    const float gap       = ImGui::GetStyle().ItemSpacing.x;

    // Place panel on side of select column
    ImGui::SetNextWindowPos (ImVec2(viewPort->WorkPos.x + leftWidth + gap, viewPort->WorkPos.y));
    ImGui::SetNextWindowSize(ImVec2(viewPort->WorkSize.x - leftWidth - gap, viewPort->WorkSize.y));

    if (ImGui::Begin("##methodology_panel", nullptr, SHARED::kPanelFlags))
    {
        // Set panel title on int, should be enum. eehhh good enough
        const char* title = "BENCHMARK";
        switch (m_selectedBenchmark)
        {
            case 0: title = "DETERMINISTIC BENCHMARK"; break;
            case 1: title = "CUSTOM BENCHMARK";        break;
            case 2: title = "FREE ROAM";               break;
            default: break;
        }
        SHARED::CenteredTitle(title, m_titleFontSize);
        ImGui::Separator();

        // Lay out desc and start bt
        const ImGuiStyle& style = ImGui::GetStyle();
        const float startHeight = ImGui::GetFrameHeight() * m_startBtSize;
        const float bodyHeight  = ImGui::GetContentRegionAvail().y - startHeight - style.ItemSpacing.y;

        if (m_selectedBenchmark == 0)
        {
            // Deterministic, no customization
            DrawDescription(bodyHeight);
        }
        else
        {
            // calculate division between desc and options
            const float descHeight = bodyHeight * m_descHeight;
            const float optionsHeight = bodyHeight - descHeight - style.ItemSpacing.y;

            DrawDescription(descHeight);

            if (ImGui::BeginChild("##options_row", ImVec2(0.0f, optionsHeight)))
            {
                const float togglesWidth = ImGui::GetContentRegionAvail().x * m_optionsWidth;

                // Draws checkboxes for bench
                if (ImGui::BeginChild("##toggles", ImVec2(togglesWidth, 0.0f), ImGuiChildFlags_Borders))
                {
                    DrawOptimizationToggles();
                }
                ImGui::EndChild();

                ImGui::SameLine();

                // Draws either scene select of custom seed
                if (ImGui::BeginChild("##right_options", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders))
                {
                    ImGui::PushFont(ImGui::GetFont(), SHARED::GetBaseFontSize() * m_sceneSeedFontSize);

                    if (m_selectedBenchmark == 1)
                        DrawSceneSelect();
                    else
                        DrawCustomSeed();

                    ImGui::PopFont();
                }
                ImGui::EndChild();
            }
            ImGui::EndChild();
        }

        // Start the bench we selected
        const char* actionLabel = m_selectedBenchmark == 2 ? "ENTER FREE ROAM" : "START BENCHMARK";

        ImGui::PushFont(ImGui::GetFont(), SHARED::GetBaseFontSize() * m_startBtFontSize);
        if (ImGui::Button(actionLabel, ImVec2(-FLT_MIN, startHeight)))
        {
            switch (m_selectedBenchmark)
            {
                case 0:
                    RR::Log("Pressed start bench deterministic");
                    // When pressed on Start Bench Deterministic
                    break;

                case 1:
                    RR::Log("Pressed start bench custom");
                    // When pressed on Start bench custom
                    break;

                case 2:
                    RR::Log("Pressed enter free roam");
                    // When pressed on Enter Free Roam
                    break;

                default:
                    break;
            }
        }
        ImGui::PopFont();
    }
    ImGui::End();
}

void ArtefactMenu::DrawDescription(float _height)
{
    if (ImGui::BeginChild("##description", ImVec2(0.0f, _height), ImGuiChildFlags_Borders))
    {
        std::string subTitle;
        std::string context;

        // right headline per mode
        switch (m_selectedBenchmark)
        {
            case 0:
                subTitle = "Methodology:";
                break;

            case 1:
                subTitle = "Description:";
                break;

            case 2:
                subTitle = "Description:";
                break;

            default:
                break;
        }

        // right headline per mode
        switch (m_selectedBenchmark)
        {
            case 0:
                context = "This is the deterministic benchmark. \nThis test will run a "
                          "determined set of optimization combinations on a fixed camera "
                          "path and log how it runs.";
                break;

            case 1:
                context = "This is the custom benchmark settings panel. \n"
                          "From here you can choose set of combinations to test on various scenes, "
                          "however custom benchmarks can be analyzed and compared but wont count"
                          "as valid, as data wont be the same between devices and users.";
                break;

            case 2:
                context = "This is the free roam panel. \nFeel free to pick the optimizations"
                          "you want to see in action and roam around the map to assess visually, "
                          "no logging will happen, this is purely to try the optimizations";
                break;

            default:
                break;
        }



        ImGui::PushFont(ImGui::GetFont(), SHARED::GetBaseFontSize() * m_subTitleFontSize);
        ImGui::TextUnformatted(subTitle.c_str());
        ImGui::PopFont();

        ImGui::Spacing();

        ImGui::PushFont(ImGui::GetFont(), SHARED::GetBaseFontSize() * m_descFontSize);
        ImGui::TextWrapped(context.c_str());
        ImGui::PopFont();

    }
    ImGui::EndChild();
}

void ArtefactMenu::DrawOptimizationToggles()
{
    ImGui::PushFont(ImGui::GetFont(), SHARED::GetBaseFontSize() * m_optTogFontSize);
    SHARED::CenteredText("OPTIMIZATION TOGGLES");
    ImGui::Separator();
    ImGui::Spacing();

    if (ImGui::BeginTable("##toggles_grid", 2))
    {
        ImGui::TableNextColumn(); ImGui::Checkbox("Level Of Detail",  &m_runInfo.lod);
        ImGui::TableNextColumn(); ImGui::Checkbox("Multi-Threading",  &m_runInfo.async);
        ImGui::TableNextColumn(); ImGui::Checkbox("Smart Scheduling", &m_runInfo.scheduling);
        ImGui::TableNextColumn(); ImGui::Checkbox("LOD caching",      &m_runInfo.lodCache);
        ImGui::TableNextColumn(); ImGui::Checkbox("Greedy Meshing",   &m_runInfo.greedy);
        ImGui::EndTable();
    }
    ImGui::PopFont();
}

void ArtefactMenu::DrawSceneSelect()
{
    SHARED::CenteredText("SCENE SELECT");
    ImGui::Separator();
    ImGui::Spacing();

    // Placeholder for scene selection
    static const char* scenes[] = { "SCENE 1", "SCENE 2", "SCENE 3", "SCENE 4" };
    for (int i = 0; i < IM_ARRAYSIZE(scenes); ++i)
    {
        if (ImGui::Selectable(scenes[i], m_selectedScene == i))
        {
            m_selectedScene = i;
        }

    }
    //ImGui::TextDisabled("list etc etc.... (only 1 selectable)");
}

void ArtefactMenu::DrawCustomSeed()
{
    SHARED::CenteredText("CUSTOM SEED");
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextUnformatted("Insert seed:");
    ImGui::SetNextItemWidth(-FLT_MIN);
    ImGui::InputTextWithHint("##seed", m_seedBuffer, m_seedBuffer,
        sizeof(m_seedBuffer), ImGuiInputTextFlags_CharsDecimal);

    ImGui::Spacing();
    if (ImGui::Button("RANDOMIZE", ImVec2(-FLT_MIN, 0.0f)))
    {
        std::snprintf(m_seedBuffer, sizeof(m_seedBuffer), "%u", BT::RandomSeed());
    }
}

// ANALYZER TAB --------------------------------------------------------------------------------------------------------

namespace AT
{
    std::string FormatFloat(const char* _fmt, float _v)
    {
        char bytes[40];
        std::snprintf(bytes, sizeof(bytes), _fmt, _v);
        return bytes;
    }

    std::string TrimGpu(const std::string& _full)
    {
        // No valid data
        if (_full.empty()) return "unknown";

        // Grab everything before the first '/'
        std::string result = _full.substr(0, _full.find('/'));

        // Strip unecessary vendor prefixes
        // const char* prefixes[] = {
        //     "NVIDIA GeForce ",
        //     "NVIDIA ",
        //     "AMD Radeon ",
        //     "AMD ",
        //     "Radeon ",
        //     "Intel(R) Arc(TM) ",
        //     "Intel(R) ",
        //     "Intel "
        // };
        //
        // for (const char* prefix : prefixes)
        // {
        //     if (result.rfind(prefix, 0) == 0)
        //     {
        //         result.erase(0, std::strlen(prefix));
        //         break;
        //     }
        // }

        // Trim whitespace
        const auto start = result.find_first_not_of(' ');
        const auto end   = result.find_last_not_of(' ');

        if (start == std::string::npos)
            return _full; // nothing left, return original

        return result.substr(start, end - start + 1);
    }

    std::string TrimCpu(const std::string& _full)
    {
        if (_full.empty()) return "unknown";

        std::string result = _full;

        // Remove trademark/registered symbols
        const char* symbols[] = { "(R)", "(TM)", "(tm)" };
        for (const char* symbol : symbols)
        {
            std::size_t pos;
            while ((pos = result.find(symbol)) != std::string::npos)
                result.erase(pos, std::strlen(symbol));
        }

        // Cut unecessary cpu data
        const char* cutoffs[] = { " @", " Processor", " CPU", " with " };
        for (const char* cutoff : cutoffs)
        {
            const auto pos = result.find(cutoff);
            if (pos != std::string::npos)
                result.erase(pos);
        }

        // Remove core and count - other field for that
        const auto corePos = result.find("-Core");
        if (corePos != std::string::npos)
        {
            const auto spacePos = result.rfind(' ', corePos);
            result.erase(spacePos == std::string::npos ? corePos : spacePos);
        }

        // Collapse any double spaces left behind
        std::size_t pos;
        while ((pos = result.find("  ")) != std::string::npos)
        {
            result.erase(pos, 1);
        }

        // Trim leading and trailing whitespace
        const auto start = result.find_first_not_of(' ');
        const auto end   = result.find_last_not_of(' ');

        if (start == std::string::npos)
            return _full;

        return result.substr(start, end - start + 1);
    }

    void MetricField(const char* _label, const std::string& _value)
    {
        ImGui::TableNextColumn(); ImGui::TextColored(SHARED::kLabelColor, "%s", _label);
        ImGui::TableNextColumn(); ImGui::TextColored(SHARED::kValueColor, "%s", _value.c_str());
    }

    void DrawResultContent(const RR::BenchmarkRun& _runData, const std::vector<float>& _frameTimes,
        const ImVec4& _accent, float mtFontSize, float pcInfoFontSize, float metricGapSize,
        float graphLineWeight, float statsFontSize, float togglesFontSize)
    {
        const RR::FrameStats& stats = _runData.stats;
        const RR::RunInfo&    info  = _runData.info;

        auto lowFps = [](float _ms) -> float {
            return _ms > 0.0f ? 1000.0f / _ms : 0.0f;
        };

        ImGui::PushFont(ImGui::GetFont(), SHARED::GetBaseFontSize() * mtFontSize);
        // Metadata, scene, seed and config
        ImGui::Text("%s", info.scene.c_str());
        ImGui::SameLine(); ImGui::TextDisabled("·"); ImGui::SameLine();
        ImGui::Text("seed %u", info.seed);
        ImGui::SameLine(); ImGui::TextDisabled("·"); ImGui::SameLine();
        ImGui::Text("%s", info.config.c_str());

        // Validity badge
        const bool   valid = info.completed && info.config != "Debug";
        const char*  badge = valid ? "VALID" : (!info.completed ? "PARTIAL RUN" : "DEBUG — INVALID");
        const ImVec4 badgeColor = valid ? ImVec4(0.40f, 0.85f, 0.45f, 1.0f) : ImVec4(0.95f, 0.42f, 0.42f, 1.0f);
        const float  badgeWidth = ImGui::CalcTextSize(badge).x;

        // Draw Badge
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - badgeWidth);
        ImGui::TextColored(badgeColor, "%s", badge);
        ImGui::PopFont();

        // device line, cpu gpu & cores
        ImGui::PushFont(ImGui::GetFont(), SHARED::GetBaseFontSize() * pcInfoFontSize);
        ImGui::TextColored(SHARED::kLabelColor, "%s  ·  %s  ·  %u cores",
                           TrimGpu(info.gpuName).c_str(), TrimCpu(info.cpuName).c_str(), info.coreCount);
        ImGui::PopFont();
        ImGui::Separator();

        ImGui::PushFont(ImGui::GetFont(), SHARED::GetBaseFontSize() * statsFontSize);
        if (ImGui::BeginTable("##stats", 17, ImGuiTableFlags_SizingFixedFit))
        {
            auto Label = [](const char* _s) {
                ImGui::TableNextColumn(); ImGui::TextColored(SHARED::kLabelColor, "%s", _s);
            };

            auto Metric = [](const char* _l, const std::string& _v) {
                MetricField(_l, _v);
            };

            auto Gap = [&] {
                ImGui::TableNextColumn(); ImGui::Dummy(ImVec2(metricGapSize, 0.0f));
            };

            // Frame rate and stutter
            ImGui::TableNextRow();
            Label("FRAMERATE (fps)");
            Metric("Avg",  FormatFloat("%.0f", stats.avgFps));
            Metric("10%",  FormatFloat("%.0f", lowFps(stats.low10Pc)));
            Metric("5%",   FormatFloat("%.0f", lowFps(stats.low5Pc)));
            Metric("1%",   FormatFloat("%.0f", lowFps(stats.low1Pc)));
            Metric("0.1%", FormatFloat("%.0f", lowFps(stats.low01Pc)));
            Gap(); Label("STUTTER");
            Metric("Spikes", FormatFloat("%.0f", static_cast<float>(stats.stutterCount)));
            Metric("Frames", FormatFloat("%.0f", static_cast<float>(stats.frameCount)));

            // Frame time and latency
            ImGui::TableNextRow();
            Label("FRAMETIME (ms)");
            Metric("Avg", FormatFloat("%.2f", stats.avgFrameTimeMs));
            Metric("Min", FormatFloat("%.2f", stats.minFrameTimeMs));
            Metric("Max", FormatFloat("%.2f", stats.maxFrameTimeMs));
            Metric("Std", FormatFloat("%.2f", stats.stdDeviationMs));
            ImGui::TableNextColumn(); ImGui::TableNextColumn();   // empty column
            Gap(); Label("LATENCY");
            Metric("CPU", FormatFloat("%.2f", stats.avgCpuMs));
            Metric("GPU", FormatFloat("%.2f", stats.avgGpuMs));

            ImGui::EndTable();
        }
        ImGui::PopFont();

        ImGui::Spacing();

        // Graph drawing - using ImPlot
        const float frameHeight = ImGui::GetFrameHeightWithSpacing();
        const float graphHeight = ImGui::GetContentRegionAvail().y - frameHeight - ImGui::GetStyle().ItemSpacing.y;
        if (ImGui::BeginChild("##graph", ImVec2(0.0f, graphHeight > 0.0f ? graphHeight : 0.0f), ImGuiChildFlags_Borders))
        {
            if (_frameTimes.empty())
            {
                ImGui::TextDisabled("(no samples)");
            }
            else if (ImPlot::BeginPlot("##frametime", ImVec2(-1.0f, -1.0f),
                                       ImPlotFlags_NoTitle | ImPlotFlags_NoLegend))
            {
                // Hide X axis, make it log scale for better visualization
                ImPlot::SetupAxes(nullptr, "ms", ImPlotAxisFlags_NoDecorations, ImPlotAxisFlags_AutoFit);
                ImPlot::SetupAxisScale(ImAxis_Y1, ImPlotScale_Log10);

                ImPlotSpec spec;
                spec.LineColor  = _accent;
                spec.LineWeight = graphLineWeight;
                ImPlot::PlotLine("FrameTime", _frameTimes.data(), static_cast<int>(_frameTimes.size()),
                                 1.0, 0.0, spec);
                ImPlot::EndPlot();
            }
        }
        ImGui::EndChild();

        // Optimization toggles
        ImGui::PushFont(ImGui::GetFont(), SHARED::GetBaseFontSize() * togglesFontSize);
        auto tag = [](const char* _label, bool _on)
        {
            const ImVec4 color = _on ? ImVec4(0.47f, 0.78f, 1.0f, 1.0f) : ImVec4(0.45f, 0.45f, 0.45f, 1.0f);
            ImGui::TextColored(color, "%s %s", _on ? "[x]" : "[ ]", _label);
        };

        tag("LOD", info.lod);                ImGui::SameLine();
        tag("Multi-Threading", info.async);  ImGui::SameLine();
        tag("Scheduling", info.scheduling);  ImGui::SameLine();
        tag("LOD Cache", info.lodCache);     ImGui::SameLine();
        tag("Greedy", info.greedy);
        ImGui::PopFont();
    }
}

void ArtefactMenu::DrawAnalyzerPanel()
{
    if (m_runListDirty) RefreshRunList();

    const ImGuiViewport* viewPort = ImGui::GetMainViewport();

    if (!m_analyzerPanelOpen)
    {
        ImGui::SetNextWindowPos(viewPort->WorkPos, ImGuiCond_Always);
        if (ImGui::Begin("##analyzer_tab", nullptr, SHARED::kPanelFlags | ImGuiWindowFlags_AlwaysAutoResize))
        {
            if (ImGui::Button(">>")) m_analyzerPanelOpen = true;
            m_analyzerWidth = ImGui::GetWindowWidth();
        }
        ImGui::End();
        return;
    }

    // Dock result to left so can be collapsed for more space
    const float minWidth = viewPort->WorkSize.x * m_minAnalyzerWidth;
    const float maxWidth = viewPort->WorkSize.x * m_maxAnalyzerWidth;

    ImGui::SetNextWindowPos(viewPort->WorkPos, ImGuiCond_Always);
    ImGui::SetNextWindowSizeConstraints(
        ImVec2(minWidth, viewPort->WorkSize.y),
        ImVec2(maxWidth, viewPort->WorkSize.y));
    ImGui::SetNextWindowSize(
        ImVec2(viewPort->WorkSize.x * SHARED::kLeftSize, viewPort->WorkSize.y),
        ImGuiCond_FirstUseEver);

    // Make only right edge draggable
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBringToFrontOnFocus;

    if (ImGui::Begin("##analyzer_panel", nullptr, flags))
    {
        m_analyzerWidth = ImGui::GetWindowWidth();

        // collapse button
        if (ImGui::Button("<<")) m_analyzerPanelOpen = false;
        ImGui::SameLine();
        SHARED::CenteredText("RESULTS", m_resultTitleFontSize);
        ImGui::Separator();

        const ImGuiStyle& style = ImGui::GetStyle();
        const float sortHeight = ImGui::GetFrameHeightWithSpacing() * m_sortHeightPlaceHolder;
        const float listHeight = ImGui::GetContentRegionAvail().y - sortHeight - style.ItemSpacing.y;

        if (ImGui::BeginChild("##results", ImVec2(0.0f, listHeight), ImGuiChildFlags_Borders))
        {
            if (m_runFiles.empty())
            {
                ImGui::TextDisabled("(no benchmark runs found)");
            }
            else
            {
                std::vector<int> shown;
                shown.reserve(m_runFiles.size());

                for (int i = 0; i < static_cast<int>(m_runFiles.size()); i++)
                {
                    const RR::RunInfo& info = m_runFiles[i].info;

                    // Exclude depending on current tags
                    if (m_filterValidOnly && !(info.completed && info.config != "Debug")) continue;
                    if (m_filterDeterministicOnly && !info.deterministic)                 continue;
                    shown.push_back(i);
                }

                // sort asc or desc
                std::ranges::sort(shown, [this](int a, int b) -> bool
                {
                    if (m_sortAscending)
                    {
                        return m_runFiles[a].name < m_runFiles[b].name;
                    }
                    return m_runFiles[a].name > m_runFiles[b].name;
                });

                for (int index : shown)
                {
                    ImGui::PushFont(ImGui::GetFont(), SHARED::GetBaseFontSize() * m_selectableFontSize);

                    if (ImGui::Selectable(SHARED::PrettyName(
                        m_runFiles[index].name,
                        m_runFiles[index].info.name,
                        m_runFiles[index].info.scene).c_str(),
                        m_selectedRunIndex == index))
                    {
                        m_selectedRunIndex = index;
                        auto& fileSys = RR::Engine::GetInstance().GetFileSystem();

                        RR::BenchmarkRun runData = RR::BenchmarkParser::ParseBenchmarkCsv(
                            fileSys.LoadOutputFileText(m_runFiles[index].relPath));

                        // Populate a result window
                        ResultTile window;
                        window.id    = m_nextResultId++;
                        window.label = m_runFiles[index].name;

                        window.frameTimes.reserve(runData.samples.size());

                        for (const RR::FrameSample& sample : runData.samples)
                        {
                            window.frameTimes.push_back(sample.frameTimeMs);
                        }
                        window.runData = std::move(runData);
                        m_openResults.push_back(std::move(window));

                        // Cap max windows at 2
                        if (m_openResults.size() > SHARED::kMaxOpenWindows)
                        {
                            m_openResults.erase(m_openResults.begin());
                            for (ResultTile& wind : m_openResults)
                            {
                                wind.placed = false;
                                wind.userMoved = false;
                            }
                        }
                    }

                    ImGui::PopFont();
                }
            }
            ImGui::EndChild();

            SHARED::CenteredText("SORT", m_resultTitleFontSize);
            ImGui::Separator();
            ImGui::PushFont(ImGui::GetFont(), SHARED::GetBaseFontSize() * m_sortItemFontSize);
            ImGui::Checkbox("Valid Only",         &m_filterValidOnly);
            ImGui::Checkbox("Deterministic Only", &m_filterDeterministicOnly);

            // Sort list
            if (SHARED::TabButton("ASCENDING",  m_sortAscending,  ImVec2(-FLT_MIN, 0.0f)))
                m_sortAscending = true;
            if (SHARED::TabButton("DESCENDING", !m_sortAscending, ImVec2(-FLT_MIN, 0.0f)))
                m_sortAscending = false;
            ImGui::PopFont();
        }
    }
    ImGui::End();
}

void ArtefactMenu::DrawResultWindows()
{
    const ImGuiViewport* viewPort = ImGui::GetMainViewport();

    // Start window tiled and take all horizontal space
    const float tilePosX    = viewPort->WorkPos.x + m_analyzerWidth;
    const float availWidth = viewPort->WorkSize.x - m_analyzerWidth;
    const float tileHeight = (viewPort->WorkSize.y - m_tileGapPix) * m_tileSliceFactor;
    const bool  panelMoved = (m_analyzerWidth != m_lastAnalyzerWidth);
    
    // quick tell if a value is noticeably different
    auto differs = [](float _a, float _b) {
        return _a - _b > 2.0f || _b - _a > 2.0f;
    };

    int i = 0;
    for (auto& tile : m_openResults)
    {
        const float tilePosY = viewPort->WorkPos.y + static_cast<float>(i) * (tileHeight + m_tileGapPix);
        const bool apply = !tile.placed || (!tile.userMoved && panelMoved);

        // force tile pos on first appearance and panel collapse if not moved yet
        if (apply)
        {
            ImGui::SetNextWindowPos (ImVec2(tilePosX, tilePosY), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(availWidth, tileHeight), ImGuiCond_Always);
            tile.placed = true;
        }

        // per-run color scheme
        const ImVec4 accent = SHARED::RunColor(tile.id);
        float& act  = m_tileColorFactorActive;
        float& uAct = m_tileColorFactorInactive;

        // Apply to BG
        ImGui::PushStyleColor(ImGuiCol_TitleBg,
            ImVec4(accent.x * uAct, accent.y * uAct, accent.z * uAct, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_TitleBgActive,
            ImVec4(accent.x * act, accent.y * act, accent.z * act, 1.0f));

        // Use filename as title
        const std::string title = SHARED::PrettyName(
            tile.label, tile.runData.info.name, tile.runData.info.scene) +
                "###result" + std::to_string(tile.id);

        // make tile movable resizable and collapsable
        if (ImGui::Begin(title.c_str(), &tile.open, ImGuiWindowFlags_NoSavedSettings))
            AT::DrawResultContent(
                tile.runData,
                tile.frameTimes,
                accent,
                m_metadataFontSize,
                m_pcInfoFontSize,
                m_metricGapSize,
                m_graphLineWeight,
                m_statsFontSize,
                m_togglesFontSize);

        // Check if user took control of the tile (clicked, collapsed, moved, etc)
        if (!apply && !tile.userMoved && !ImGui::IsWindowCollapsed())
        {
            const ImVec2 tPos  = ImGui::GetWindowPos();
            const ImVec2 tSize = ImGui::GetWindowSize();

            if (differs(tPos.x, tilePosX)    ||
                differs(tPos.y, tilePosY)    ||
                differs(tSize.x, availWidth) ||
                differs(tSize.y, tileHeight))
            {
                tile.userMoved = true;
            }
        }
        ImGui::End();
        ImGui::PopStyleColor(2);
        i++;
    }

    m_lastAnalyzerWidth = m_analyzerWidth;
    // Erase closed tiles
    std::erase_if(m_openResults, [](const ResultTile& tile) {
        return !tile.open;
    });
}

void ArtefactMenu::DrawComparePanel()
{
}

void ArtefactMenu::RefreshRunList()
{
    m_runFiles.clear();
    m_selectedRunIndex = -1;

    // Load benchmarks from disk
    auto& fileSys = RR::Engine::GetInstance().GetFileSystem();
    const auto files = fileSys.ListOutputFiles("Benchmarks", {".csv"});

    for (const auto& path : files)
    {
        const std::string relPath = path.string();
        RR::BenchmarkRun runInfo = RR::BenchmarkParser::ParseBenchmarkCsv(fileSys.LoadOutputFileText(relPath));

        // Load parsed result to vector
        m_runFiles.push_back({ relPath, path.filename().string(), runInfo.info });
    }
    m_runListDirty = false;
}






























