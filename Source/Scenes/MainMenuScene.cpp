
#include <cfloat>
#include <cstring>
#include <random>
#include <cstdio>
#include <algorithm>
#include <string>

#include "Scenes/MainMenuScene.h"

#include "BenchmarkScene.h"
#include "FreeRoamScene.h"
#include "imgui.h"
#include "implot.h"
#include "Benchmark/BenchmarkRunPresets.hpp"


// PUBLIC --------------------------------------------------------------------------------------------------------------

MainMenuScene::MainMenuScene() : Scene("Main Menu") {}

MainMenuScene::~MainMenuScene()
= default;

// PROTECTED -----------------------------------------------------------------------------------------------------------

bool MainMenuScene::Init()
{
    SetSceneUIScale(m_uiScale);
    SetCursorEnabled(true);
    SetSceneClearColor({0.0f, 0.0f, 0.0f, 1.0f});
    return true;
}

// Mandatory hooks, unused.
void MainMenuScene::PreUpdate(float _deltaTime) {}
void MainMenuScene::Update(float _deltaTime) {}
void MainMenuScene::LateUpdate(float _deltaTime) {}
void MainMenuScene::Destroy() {}

void MainMenuScene::OnGui()
{
    Scene::OnGui();

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
    else if (m_view == TopView::COMPARE)
    {
        DrawComparePanel();
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

    // Per-run accent Okabe-Ito - Colorblind safe
    static const ImVec4 kRunPalette[] = {
        ImVec4(0.902f, 0.624f, 0.000f, 1.0f),
        ImVec4(0.337f, 0.706f, 0.914f, 1.0f),
        ImVec4(0.000f, 0.620f, 0.451f, 1.0f),
        ImVec4(0.941f, 0.894f, 0.259f, 1.0f),
        ImVec4(0.000f, 0.447f, 0.698f, 1.0f),
        ImVec4(0.835f, 0.369f, 0.000f, 1.0f),
        ImVec4(0.800f, 0.475f, 0.655f, 1.0f),
        ImVec4(0.706f, 0.553f, 0.902f, 1.0f),
    };
    constexpr int kRunPaletteCount = IM_ARRAYSIZE(kRunPalette);

    ImVec4 RunColor(int _idx)
    {
        return kRunPalette[_idx % kRunPaletteCount];
    }

    // Keeps track of color slots, prevents repetition
    template <typename Range, typename Proj>
    int FreeColorIndex(const Range& _items, Proj _proj)
    {
        bool taken[kRunPaletteCount] = {};

        for (const auto& item : _items)
        {
            const int index = _proj(item);

            if (index >= 0 && index < kRunPaletteCount)
            {
                taken[index] = true;
            }
        }

        for (int i = 0; i < kRunPaletteCount; ++i)
        {
            if (!taken[i]) return i;
        }

        return 0;
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

        // Timestamp six digit groups (Y M D H M S)
        if (nums.size() >= 6)
        {
            const std::size_t n = nums.size();
            out += " " + nums[n - 5] + "/" + nums[n - 4]
                 + " " + nums[n - 3] + ":" + nums[n - 2] + ":" + nums[n - 1];
        }

        return out;
    }
}

// TOP BAR -------------------------------------------------------------------------------------------------------------

void MainMenuScene::DrawTopBar()
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
        ImGui::PopStyleVar();   
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
        }
        else
        {
            m_view = TopView::ANALYZER;
            m_runListDirty = true;
        }
    }
    if (SHARED::TabButton("COMPARE", m_view == TopView::COMPARE))
    {
        if (m_view == TopView::COMPARE)
        {
            m_view = TopView::NONE;
        }
        else
        {
            m_view = TopView::COMPARE;
            m_runListDirty = true;   // same: refresh on open
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
    if (ImGui::IsItemDeactivatedAfterEdit())
    {
        SetSceneUIScale(m_uiScalePending);
        m_uiScale = m_uiScalePending;
    }

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
        const float lineHeight = ImGui::GetTextLineHeight();

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

void MainMenuScene::DrawBenchmarkPanel()
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
    }
    ImGui::End();
}

void MainMenuScene::DrawMethodologyPanel()
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

                // Right column contains scene/seed select and render distance slider
                if (ImGui::BeginChild("##right_options", ImVec2(0.0f, 0.0f)))
                {
                    ImGui::PushFont(ImGui::GetFont(), SHARED::GetBaseFontSize() * m_sceneSeedFontSize);

                    const float rdHeight   = ImGui::GetContentRegionAvail().y * 0.35f;
                    const float modeHeight = ImGui::GetContentRegionAvail().y - rdHeight - style.ItemSpacing.y;

                    // Top is scene select or seed
                    if (ImGui::BeginChild("##mode_window", ImVec2(0.0f, modeHeight), ImGuiChildFlags_Borders))
                    {
                        if (m_selectedBenchmark == 1)
                            DrawSceneSelect();
                        else
                            DrawCustomSeed();
                    }
                    ImGui::EndChild();

                    // Bottom display render distance
                    if (ImGui::BeginChild("##render_distance_window", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders))
                    {
                        DrawRenderDistance();
                    }
                    ImGui::EndChild();

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
                {
                    using namespace DETERMINISTIC;
                    auto& eng = RR::Engine::GetInstance();
                    RR::RunInfo info = GetRunPreset(SCENE::BASELINE);

                    // Reset Scene Step
                    gCurrentSceneStep = static_cast<uInt8>(SCENE::BASELINE);

                    //Load preset, then load scene
                    eng.GetAppManager().RequestSceneLoad<BenchmarkScene>(info);
                }
                break;

                case 1:
                {
                    using namespace CUSTOM;
                    auto& eng = RR::Engine::GetInstance();

                    // Gather run data
                    WORLDGEN::WorldGenConfig genConfig;
                    RR::RunInfo runInfo = m_runInfo;
                    const SCENE scene = static_cast<SCENE>(m_selectedScene);

                    // Load custom preset data to runInfo
                    GetRunPreset(scene, runInfo, genConfig);

                    // pass to scene
                    eng.GetAppManager().RequestSceneLoad<BenchmarkScene>(runInfo, genConfig);
                }
                break;

                case 2:
                {
                    auto& eng = RR::Engine::GetInstance();
                    RR::RunInfo runInfo = m_runInfo;
                    std::string name  = "FreeRoam";
                    std::string scene = "FR";

                    // Name scene accordingly
                    NAMING::AppendDetails(runInfo, name);
                    NAMING::AppendDetails(runInfo, scene);
                    runInfo.name  = name;
                    runInfo.scene = scene;

                    // parse seed from text buffer
                    std::uint32_t seed = 0;
                    const auto result = std::from_chars(
                        m_seedBuffer, m_seedBuffer + std::strlen(m_seedBuffer), seed);

                    // check if see is valid, if so inject into scene
                    if (result.ec == std::errc{})
                    {
                        runInfo.seed = seed;
                    }

                    // Load free roam scene
                    eng.GetAppManager().RequestSceneLoad<FreeRoamScene>(runInfo);
                }
                break;

                default:
                    break;
            }
        }
        ImGui::PopFont();
    }
    ImGui::End();
}

void MainMenuScene::DrawDescription(float _height)
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
                          "From here you can choose a set of combinations to test on various scenes, "
                          "however custom benchmarks can be analyzed and compared but wont count "
                          "as valid, as data wont be the same between devices and users.";
                break;

            case 2:
                context = "This is the free roam panel. \nFeel free to pick the optimizations "
                          "you want to see in action and roam around the map to assess visually, "
                          "no logging will happen, this is purely to try the optimizations.";
                break;

            default:
                break;
        }



        ImGui::PushFont(ImGui::GetFont(), SHARED::GetBaseFontSize() * m_subTitleFontSize);
        ImGui::TextUnformatted(subTitle.c_str());
        ImGui::PopFont();

        ImGui::Spacing();

        ImGui::PushFont(ImGui::GetFont(), SHARED::GetBaseFontSize() * m_descFontSize);
        ImGui::TextWrapped("%s", context.c_str());
        ImGui::PopFont();

    }
    ImGui::EndChild();
}

void MainMenuScene::DrawOptimizationToggles()
{
    ImGui::PushFont(ImGui::GetFont(), SHARED::GetBaseFontSize() * m_optTogFontSize);
    SHARED::CenteredText("OPTIMIZATION TOGGLES");
    ImGui::Separator();
    ImGui::Spacing();

    if (ImGui::BeginTable("##toggles_grid", 2))
    {
        ImGui::TableNextColumn();
        ImGui::Checkbox("Level Of Detail",  &m_runInfo.lod);
        ImGui::TableNextColumn();
        ImGui::Checkbox("Multi-Threading",  &m_runInfo.async);
        ImGui::TableNextColumn();
        ImGui::Checkbox("Smart Scheduling", &m_runInfo.scheduling);
        ImGui::TableNextColumn(); 
        ImGui::Checkbox("LOD caching",      &m_runInfo.lodCache);
        // lod cache can't exist without lod
        if (m_runInfo.lodCache) m_runInfo.lod = true;

        ImGui::TableNextColumn();
        ImGui::Checkbox("Greedy Meshing",   &m_runInfo.greedy);
        ImGui::EndTable();
    }
    ImGui::PopFont();
}

void MainMenuScene::DrawSceneSelect()
{
    SHARED::CenteredText("SCENE SELECT");
    ImGui::Separator();
    ImGui::Spacing();

    for (int i = 0; i < IM_ARRAYSIZE(CUSTOM::kSceneNames); ++i)
    {
        if (ImGui::Selectable(CUSTOM::kSceneNames[i], m_selectedScene == i))
        {
            m_selectedScene = i;
        }
    }
}

void MainMenuScene::DrawCustomSeed()
{
    SHARED::CenteredText("CUSTOM SEED");
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextUnformatted("Insert seed:");
    ImGui::SetNextItemWidth(-FLT_MIN);
    // param order is (label, hint, buffer) - the hint only shows while the buffer is empty
    ImGui::InputTextWithHint("##seed", "e.g. 1234567890", m_seedBuffer,
        sizeof(m_seedBuffer), ImGuiInputTextFlags_CharsDecimal);

    ImGui::Spacing();
    if (ImGui::Button("RANDOMIZE", ImVec2(-FLT_MIN, 0.0f)))
    {
        std::snprintf(m_seedBuffer, sizeof(m_seedBuffer), "%u", BT::RandomSeed());
    }
}

void MainMenuScene::DrawRenderDistance()
{
    SHARED::CenteredText("RENDER DISTANCE");
    ImGui::Separator();
    ImGui::Spacing();

    // Only unlock extra chungs with LOD enabled
    const int maxRD = m_runInfo.lod ? 384 : 32;
    m_runInfo.renderDistance = std::clamp(m_runInfo.renderDistance, 2, maxRD);

    //ImGui::TextUnformatted("Custom Render Distance:");
    ImGui::SetNextItemWidth(-FLT_MIN);
    ImGui::SliderInt("##render_distance", &m_runInfo.renderDistance, 2, maxRD,
        "%d chunks", ImGuiSliderFlags_AlwaysClamp);
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
        ImGui::Text("Seed %u", info.seed);
        ImGui::SameLine(); ImGui::TextDisabled("·"); ImGui::SameLine();
        ImGui::Text("%s", info.config.c_str());
        ImGui::SameLine(); ImGui::TextDisabled("·"); ImGui::SameLine();
        ImGui::Text("Render Distance %d", info.renderDistance);

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

void MainMenuScene::DrawAnalyzerPanel()
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

                        // Re-clicking an open run should not stack an identical tile on top
                        const bool alreadyOpen = std::ranges::any_of(m_openResults,
                            [&](const ResultTile& _tile) {
                                return _tile.label == m_runFiles[index].name;
                            });

                        if (!alreadyOpen)
                        {
                            auto& fileSys = RR::Engine::GetInstance().GetFileSystem();

                            RR::BenchmarkRun runData = RR::BenchmarkParser::ParseBenchmarkCsv(
                                fileSys.LoadOutputFileText(m_runFiles[index].relPath));

                            // Populate a result window
                            ResultTile window;
                            // Assign correct color
                            window.colorIdx = SHARED::FreeColorIndex(m_openResults,
                                [](const ResultTile& tile) {
                                    return tile.colorIdx;
                                });
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
                            if (static_cast<int>(m_openResults.size()) > SHARED::kMaxOpenWindows)
                            {
                                m_openResults.erase(m_openResults.begin());
                                for (ResultTile& wind : m_openResults)
                                {
                                    wind.placed    = false;
                                    wind.userMoved = false;
                                }
                            }
                        }
                    }

                    ImGui::PopFont();
                }
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
    ImGui::End();
}

void MainMenuScene::DrawResultWindows()
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
        const ImVec4 accent = SHARED::RunColor(tile.colorIdx);
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
                m_graphLineWeightAnalyze,
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

// COMPARE TAB ---------------------------------------------------------------------------------------------------------

namespace CT
{
    enum CompareCol
    {
        COL_FPS = 1,
        COL_L10,
        COL_L5,
        COL_L1,
        COL_L01,
        COL_FT_AVG,
        COL_FT_MIN,
        COL_FT_MAX,
        COL_FT_STD,
        COL_STUT
    };

    float CompareMetric(const RR::FrameStats& _st, int _column)
    {
        const auto lowFps = [](float ms) {
            return ms > 0.0f ? 1000.0f / ms : 0.0f;
        };

        switch (_column)
        {
            case COL_FPS:    return _st.avgFps;
            case COL_L10:    return lowFps(_st.low10Pc);
            case COL_L5:     return lowFps(_st.low5Pc);
            case COL_L1:     return lowFps(_st.low1Pc);
            case COL_L01:    return lowFps(_st.low01Pc);
            case COL_FT_AVG: return _st.avgFrameTimeMs;
            case COL_FT_MIN: return _st.minFrameTimeMs;
            case COL_FT_MAX: return _st.maxFrameTimeMs;
            case COL_FT_STD: return _st.stdDeviationMs;
            case COL_STUT:   return static_cast<float>(_st.stutterCount);
            default:         return 0.0f;
        }
    }

    // Colors the value green/red based on whether it's better or worse than the base,
    void CompareCell(float _value, float _base, bool _higherIsBetter, bool _isBaseRow, bool _showDelta, const char* _fmt)
    {
        ImGui::TableNextColumn();

        // compare if a base one exists
        const bool haveBase = (_base > 0.0f) && !_isBaseRow;

        ImVec4 color = SHARED::kValueColor;
        float  relDelta = 0.0f;

        if (haveBase)
        {
            // Calculate relative difference
            relDelta = (_value - _base) / _base;

            // Only colorise if the difference is significant
            const float absDelta = relDelta < 0.0f ? -relDelta : relDelta;
            if (absDelta >= 0.005f)
            {
                const bool isBetter = _higherIsBetter ? (_value > _base) : (_value < _base);
                color = isBetter
                    ? ImVec4(0.45f, 0.85f, 0.45f, 1.0f)
                    : ImVec4(0.95f, 0.45f, 0.45f, 1.0f);
            }
        }

        if (haveBase && _showDelta)
            ImGui::TextColored(color, "%+.0f%%", relDelta * 100.0f);
        else
            ImGui::TextColored(color, _fmt, _value);
    }
}

void MainMenuScene::DrawComparePanel()
{
    if (m_runListDirty) RefreshRunList();

    const ImGuiViewport* viewPort = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos (viewPort->WorkPos);
    ImGui::SetNextWindowSize(viewPort->WorkSize);

    if (!ImGui::Begin("##compare_panel", nullptr, SHARED::kPanelFlags))
    {
        ImGui::End();
        return;
    }

    auto lowFps = [](float _ms) {
        return _ms > 0.0f ? 1000.0f / _ms : 0.0f;
    };

    // Resolve baseline, if it was removed fall back to first loaded
    const RR::FrameStats* base = nullptr;

    for (const CompareSlot& slot : m_compareSlots)
    {
        // break if found
        if (slot.loaded && slot.id == m_compareBaselineId)
        {
            base = &slot.runData.stats;
            break;
        }
    }

    // No baseline, found first loaded slot
    if (!base)
    {
        m_compareBaselineId = -1;
        for (const CompareSlot& slot : m_compareSlots)
        {
            if (slot.loaded)
            {
                m_compareBaselineId = slot.id;
                base = &slot.runData.stats;
                break;
            }
        }
    }

    bool mixed = false;
    const RR::RunInfo* ref = nullptr;

    // Check if the loaded compare slots have mismatched seeds
    for (const CompareSlot& slot : m_compareSlots)
    {
        if (!slot.loaded) continue;

        if (!ref)
        {
            ref = &slot.runData.info;
        }
        else if (slot.runData.info.seed  != ref->seed)
        {
            mixed = true;
            break;
        }
    }

    ImGui::PushFont(ImGui::GetFont(), SHARED::GetBaseFontSize() * m_compareCtrlFontSize);

    // Add Compare Slot
    if (static_cast<int>(m_compareSlots.size()) < m_maxCompareSlotOpen &&
        ImGui::Button("+ Add run"))
    {
        CompareSlot slot;
        // Assign correct color
        slot.colorIdx = SHARED::FreeColorIndex(m_compareSlots,
        [](const CompareSlot& locSlot) {
            return locSlot.colorIdx;
        });
        slot.id = m_nextCompareId++;
        m_compareSlots.push_back(slot);
    }
    // Warn if runs should not be compared
    ImGui::SameLine();
    ImGui::Checkbox("Show % vs baseline", &m_compareShowDelta);
    if (mixed)
    {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.95f, 0.55f, 0.35f, 1.0f),
                           "   (!) Mixed scene/seed - curves are not directly comparable");
    }
    else if (m_compareSlots.empty())
    {
        // gentle empty-state hint so the view explains itself
        ImGui::SameLine();
        ImGui::TextDisabled("   add up to %d runs to compare", m_maxCompareSlotOpen);
    }
    ImGui::PopFont();

    // Spreadsheet tags
    constexpr ImGuiTableFlags tFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                                       ImGuiTableFlags_Sortable | ImGuiTableFlags_Resizable |
                                       ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_SizingStretchProp;

    int removeIdx = -1;
    ImGui::PushFont(ImGui::GetFont(), SHARED::GetBaseFontSize() * m_compareTableFontSize);
    if (ImGui::BeginTable("##compare_tbl", 21, tFlags))
    {
        // narrow cols non resizable
        const float controlWidth = ImGui::GetFrameHeight() + m_tableControlWidth;
        const float baseWidth = ImGui::CalcTextSize("Base").x + ImGui::GetStyle().CellPadding.x * 2.0f + m_tableBaseWidth;
        ImGui::TableSetupColumn("##vis", ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, controlWidth);
        ImGui::TableSetupColumn("Base",  ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, baseWidth);
        ImGui::TableSetupColumn("Result", ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthStretch, 3.0f);
        ImGui::TableSetupColumn("FPS",  ImGuiTableColumnFlags_DefaultSort, 1.0f, CT::COL_FPS);
        ImGui::TableSetupColumn("10%",  0, 1.0f, CT::COL_L10);
        ImGui::TableSetupColumn("5%",   0, 1.0f, CT::COL_L5);
        ImGui::TableSetupColumn("1%",   0, 1.0f, CT::COL_L1);
        ImGui::TableSetupColumn("0.1%", 0, 1.0f, CT::COL_L01);
        ImGui::TableSetupColumn("ms",   0, 1.0f, CT::COL_FT_AVG);
        ImGui::TableSetupColumn("Min",  0, 1.0f, CT::COL_FT_MIN);
        ImGui::TableSetupColumn("Max",  0, 1.0f, CT::COL_FT_MAX);
        ImGui::TableSetupColumn("Std",  0, 1.0f, CT::COL_FT_STD);
        ImGui::TableSetupColumn("Stut", 0, 1.0f, CT::COL_STUT);
        ImGui::TableSetupColumn("LOD", ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("MT",  ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("SS",  ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("LC",  ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("GM",  ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("RD",  ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Scene", ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthStretch, 2.0f);
        ImGui::TableSetupColumn("##rm", ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, controlWidth);
        ImGui::TableHeadersRow();

        // sort the slots by the clicked column
        if (ImGuiTableSortSpecs* specs = ImGui::TableGetSortSpecs())
        {
            if (specs->SpecsDirty && specs->SpecsCount > 0)
            {
                const ImGuiTableColumnSortSpecs& spec = specs->Specs[0];
                const bool asc = spec.SortDirection == ImGuiSortDirection_Ascending;

                std::ranges::stable_sort(m_compareSlots,
                    [&](const CompareSlot& slotA, const CompareSlot& slotB)
                    {
                        if (slotA.loaded != slotB.loaded)
                            return slotA.loaded;

                        if (!slotA.loaded)
                            return false;

                        const float va = CT::CompareMetric(slotA.runData.stats, static_cast<int>(spec.ColumnUserID));
                        const float vb = CT::CompareMetric(slotB.runData.stats, static_cast<int>(spec.ColumnUserID));
                        return asc ? (va < vb) : (va > vb);
                    });

                specs->SpecsDirty = false;
            }
        }

        for (int i = 0; i < static_cast<int>(m_compareSlots.size()); ++i)
        {
            CompareSlot& slot  = m_compareSlots[i];
            const ImVec4 color = SHARED::RunColor(slot.colorIdx);
            const bool   isBase = slot.id == m_compareBaselineId;

            ImGui::PushID(slot.id);
            ImGui::TableNextRow();

            // colour swatch - tags the entry to the graphline
            ImGui::TableNextColumn();
            {
                const ImVec2 pos = ImGui::GetCursorScreenPos();
                const float  height = ImGui::GetFrameHeight();
                ImGui::GetWindowDrawList()->AddRectFilled(
                    pos, ImVec2(pos.x + height, pos.y + height),
                    ImGui::ColorConvertFloat4ToU32(color), 3.0f);

                ImGui::Dummy(ImVec2(height, height));
            }

            // baseline button (greyed out until the slot actually has data)
            ImGui::TableNextColumn();
            ImGui::BeginDisabled(!slot.loaded);
            if (ImGui::RadioButton("##base", isBase))
            {
                m_compareBaselineId = slot.id;
            }
            ImGui::EndDisabled();

            // file dropdown
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(-FLT_MIN);
            std::string preview;
            if (slot.name.empty())
            {
                preview = "(select run)";
            }
            else
            {
                preview = SHARED::PrettyName(slot.name,
                    slot.runData.info.name,
                    slot.runData.info.scene);
            }

            // Combo drop down with file
            if (ImGui::BeginCombo("##file", preview.c_str()))
            {
                // sort by validity
                ImGui::Checkbox("Valid Only", &m_filterValidOnly);
                ImGui::SameLine();
                ImGui::Checkbox("Deterministic Only", &m_filterDeterministicOnly);

                // Sort buttons in drop down menu
                if (SHARED::TabButton("Asc",  m_sortAscending))
                {
                    m_sortAscending = true;
                }
                ImGui::SameLine();

                if (SHARED::TabButton("Desc", !m_sortAscending))
                {
                    m_sortAscending = false;
                }
                ImGui::Separator();

                // Add to drop down, skip if requirements not met
                std::vector<int> shown;
                for (int run = 0; run < static_cast<int>(m_runFiles.size()); ++run)
                {
                    const RR::RunInfo& runInfo = m_runFiles[run].info;

                    if (m_filterValidOnly && !(runInfo.completed && runInfo.config != "Debug")) continue;
                    if (m_filterDeterministicOnly && !runInfo.deterministic) continue;
                    shown.push_back(run);
                }

                std::ranges::sort(shown, [this](int a, int b) {
                    if (m_sortAscending)
                    {
                        return m_runFiles[a].name < m_runFiles[b].name;
                    }
                    return m_runFiles[a].name > m_runFiles[b].name;
                });

                // Display entries as selectables and add if clicked
                for (int index : shown)
                {
                    if (ImGui::Selectable(SHARED::PrettyName(m_runFiles[index].name,
                        m_runFiles[index].info.name, m_runFiles[index].info.scene).c_str()))
                    {
                        // Load in selected
                        auto& fileSys = RR::Engine::GetInstance().GetFileSystem();

                        slot.relPath = m_runFiles[index].relPath;
                        slot.name    = m_runFiles[index].name;
                        slot.runData = RR::BenchmarkParser::ParseBenchmarkCsv(fileSys.LoadOutputFileText(slot.relPath));

                        slot.frameTimes.clear();
                        slot.frameTimes.reserve(slot.runData.samples.size());

                        for (const RR::FrameSample& sample : slot.runData.samples)
                        {
                            slot.frameTimes.push_back(sample.frameTimeMs);
                        }

                        slot.loaded = true;
                        m_compareFitPending = true;
                    }
                }
                ImGui::EndCombo();
            }

            if (slot.loaded)
            {
                // if show delta show difference
                const RR::FrameStats& st = slot.runData.stats;
                const bool showDelta = m_compareShowDelta;

                // this is truly a wall of code
                CT::CompareCell(st.avgFps,          base ? base->avgFps          : 0.0f, true,  isBase, showDelta, "%.0f");
                CT::CompareCell(lowFps(st.low10Pc), base ? lowFps(base->low10Pc) : 0.0f, true,  isBase, showDelta, "%.0f");
                CT::CompareCell(lowFps(st.low5Pc),  base ? lowFps(base->low5Pc)  : 0.0f, true,  isBase, showDelta, "%.0f");
                CT::CompareCell(lowFps(st.low1Pc),  base ? lowFps(base->low1Pc)  : 0.0f, true,  isBase, showDelta, "%.0f");
                CT::CompareCell(lowFps(st.low01Pc), base ? lowFps(base->low01Pc) : 0.0f, true,  isBase, showDelta, "%.0f");
                CT::CompareCell(st.avgFrameTimeMs,  base ? base->avgFrameTimeMs  : 0.0f, false, isBase, showDelta, "%.2f");
                CT::CompareCell(st.minFrameTimeMs,  base ? base->minFrameTimeMs  : 0.0f, false, isBase, showDelta, "%.2f");
                CT::CompareCell(st.maxFrameTimeMs,  base ? base->maxFrameTimeMs  : 0.0f, false, isBase, showDelta, "%.2f");
                CT::CompareCell(st.stdDeviationMs,  base ? base->stdDeviationMs  : 0.0f, false, isBase, showDelta, "%.2f");
                CT::CompareCell(static_cast<float>(st.stutterCount), base ? static_cast<float>(base->stutterCount) : 0.0f,    false, isBase, showDelta, "%.0f");

                const RR::RunInfo& info = slot.runData.info;
                auto techniques = [](bool _on) {
                    ImGui::TableNextColumn();
                    if (_on) ImGui::TextColored(ImVec4(0.45f, 0.85f, 0.45f, 1.0f), "X");
                };

                techniques(info.lod);
                techniques(info.async);
                techniques(info.scheduling);
                techniques(info.lodCache);
                techniques(info.greedy);

                ImGui::TableNextColumn();
                ImGui::TextColored(SHARED::kValueColor, "%d", info.renderDistance);

                ImGui::TableNextColumn();
                ImGui::TextColored(SHARED::kLabelColor, "%s", info.scene.c_str());
            }
            else
            {
                for (int col = 0; col < 17; col++) ImGui::TableNextColumn();  
            }

            ImGui::TableNextColumn();
            if (ImGui::Button("X", ImVec2(-FLT_MIN, 0.0f))) removeIdx = i;

            ImGui::PopID();
        }

        ImGui::EndTable();
    }
    ImGui::PopFont();

    // If none to remove, skip
    if (removeIdx >= 0)
    {
        m_compareSlots.erase(m_compareSlots.begin() + removeIdx);
        m_compareFitPending = true;
    }

    // overlay graph every visible run's frame-time line, color-matched to its row
    bool anyLoaded = false;
    for (const CompareSlot& slot : m_compareSlots)
    {
        if (slot.loaded && !slot.frameTimes.empty())
        {
            anyLoaded = true;
            break;
        }
    }

    ImGui::PushFont(ImGui::GetFont(), SHARED::GetBaseFontSize() * m_compareGraphFontSize);
    if (m_compareFitPending)
    {
        ImPlot::SetNextAxesToFit();
        m_compareFitPending = false;
    }
    if (ImPlot::BeginPlot("##overlay", ImVec2(-1.0f, -1.0f), ImPlotFlags_NoTitle))
    {
        ImPlot::SetupAxes(nullptr, "ms", ImPlotAxisFlags_NoDecorations, 0);
        ImPlot::SetupAxisScale(ImAxis_Y1, ImPlotScale_Log10);
        ImPlot::SetupLegend(ImPlotLocation_NorthWest);

        // While nothing is loaded, pin the axes to a sane default
        if (!anyLoaded)
            ImPlot::SetupAxesLimits(0.0, 1.0, 0.1, 100.0, ImPlotCond_Always);

        for (const CompareSlot& slot : m_compareSlots)
        {
            if (!slot.loaded || slot.frameTimes.empty()) continue;

            ImPlotSpec spec;
            spec.LineColor  = SHARED::RunColor(slot.colorIdx);
            spec.LineWeight = m_graphLineWeightCompare;

            //const std::string label = SHARED::PrettyName(slot.name, slot.runData.info.name,
            //    slot.runData.info.scene) + "##" + std::to_string(slot.id);
            const std::string label = slot.runData.info.scene;

            const int    frameNum = static_cast<int>(slot.frameTimes.size());
            const double xScale   = frameNum > 1 ? 1.0 / (frameNum - 1) : 1.0;

            ImPlot::PlotLine(label.c_str(),
                slot.frameTimes.data(),
                frameNum, xScale, 0.0, spec);
        }
        ImPlot::EndPlot();
    }
    ImGui::PopFont();
    ImGui::End();
}

// MISC ----------------------------------------------------------------------------------------------------------------

void MainMenuScene::RefreshRunList()
{
    m_runFiles.clear();
    m_selectedRunIndex = -1;

    // Load benchmarks from disk
    auto& fileSys = RR::Engine::GetInstance().GetFileSystem();
    const auto files = fileSys.ListOutputFiles("Benchmarks", {".csv"});

    for (const auto& path : files)
    {
        const std::string relPath = path.string();

        // header-only parse: the list needs metadata
        RR::BenchmarkRun runInfo = RR::BenchmarkParser::ParseBenchmarkCsv(
            fileSys.LoadOutputFileText(relPath), true);

        // Load parsed result to vector
        m_runFiles.push_back({ relPath, path.filename().string(), runInfo.info });
    }
    m_runListDirty = false;
}






























