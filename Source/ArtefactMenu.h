
#pragma once
#include "RR.h"
#include <vector>
#include <string>

// Artefact front end UI
// Hopefully resolution agnostic
// Uses ImGui and ImPlot to draw on screen
class ArtefactMenu : public RR::Scene
{
public:
    explicit ArtefactMenu();
    ~ArtefactMenu() override;

protected:
    bool Init() override;
    void PreUpdate(float _deltaTime) override;
    void Update(float _deltaTime) override;
    void LateUpdate(float _deltaTime) override;
    void Destroy() override;
    void OnGui() override;

private:
    void DrawTopBar();

    void DrawBenchmarkPanel();
    void DrawMethodologyPanel();

    void DrawAnalyzerPanel();
    void DrawComparePanel();
    void DrawResultWindow();

    // Determinitic, custom and free roam windows shared
    void DrawDescription(float _height);
    void DrawOptimizationToggles();
    void DrawSceneSelect();
    void DrawCustomSeed();

    void RefreshRunList();

    enum class TopView
    {
        NONE,
        BENCHMARK,
        ANALYZER,
        COMPARE
    };

    // Result window entry
    struct RunEntry
    {
        std::string relPath;
        std::string name;
        RR::RunInfo info;
    };

    // Floating result window for analyzer
    struct ResultWindow
    {
        bool open      = true;
        bool placed    = false;
        bool userMoved = false;
        int id = 0;

        std::string label;
        RR::BenchmarkRun runData;
        std::vector<float> frameTimes;
    };

    // Slot for comapre view, up to 7
    struct CompareSlot
    {
        bool visible = true;
        bool loaded  = true;
        int id = 0;

        std::string relPath;
        std::string name;
        RR::BenchmarkRun runData;
        std::vector<float> frameTimes;
    };

    TopView m_view = TopView::NONE;
    float  m_uiScalePending = 1.8f;
    float  m_uiMinScale     = 0.5f;
    float  m_uiMaxScale     = 4.0f;
    float  m_uiScale        = 1.8f;

    // TOP BAR ---------------------------------------------------------------------------------------------------------
    float m_topViewScaleIdle = 1.4f;
    float m_buttonPadding    = 3.0f;
    float m_sliderWidth      = 2.0f;
    float m_closeBtWidth     = 2.0f;
    float m_closeBtAlignment = 3.0f; // Multiplier

    // BENCHMARK TAB VIEW ----------------------------------------------------------------------------------------------
    RR::RunInfo m_runInfo;
    int m_selectedBenchmark = -1;
    int m_selectedScene     = 0;
    char m_seedBuffer[16]   = "";

    float m_benchmarkBtHeight = 2.5f;
    float m_benchBtSpacing    = 5.0f;

    // ANALYZER TAB VIEW -----------------------------------------------------------------------------------------------
    std::vector<RunEntry> m_runFiles;
    bool m_filterDeterminiticOnly = false;
    bool m_filterValidOnly        = true;
    bool m_runListDirty           = true;
    bool m_sortAscending          = true;
    int  m_selectedRunIndex       = -1;

    // result panel docking
    bool  m_analyzerPanelOpen = true;
    float m_analyzerWidth     = 0.0f;
    float m_lastAnalyzerWidth = -1.0f;

    // Floating result windows
    std::vector<ResultWindow> m_openResults;
    int m_nextResultId = 0;

    // COMPARE TAB VIEW ------------------------------------------------------------------------------------------------
    std::vector<CompareSlot> m_compareSlots;
    bool m_compareFitPending = true;
    bool m_compareShowDelta  = false;
    int  m_compareBaselineId = -1;
    int  m_nextCompareId     = 0;
};





























































