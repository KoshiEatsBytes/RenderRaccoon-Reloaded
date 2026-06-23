
#pragma once
#include "RR.h"
#include <vector>
#include <string>

// Artefact front end UI
// Hopefully resolution agnostic
// Uses ImGui and ImPlot to draw on screen
class MainMenuScene : public RR::Scene
{
public:
    explicit MainMenuScene();
    ~MainMenuScene() override;

protected:
    bool Init() override;
    void PreUpdate(float _deltaTime) override;
    void Update(float _deltaTime) override;
    void LateUpdate(float _deltaTime) override;
    void Destroy() override;
    void OnGui() override;

private:
    void DrawTopBar();

    // benchmark tab
    void DrawBenchmarkPanel();
    void DrawMethodologyPanel();
    void DrawDescription(float _height);
    void DrawOptimizationToggles();
    void DrawSceneSelect();
    void DrawCustomSeed();

    // Analyzer tab
    void DrawAnalyzerPanel();
    void DrawResultWindows();

    // Compare tab
    void DrawComparePanel();


    // Determinitic, custom and free roam windows shared


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
    struct ResultTile
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
        int id = 0;
        bool loaded  = false;

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
    float m_closeBtAlignment = 3.0f;

    // BENCHMARK TAB VIEW ----------------------------------------------------------------------------------------------
    RR::RunInfo m_runInfo;
    int  m_selectedBenchmark = -1;
    int  m_selectedScene     = 0;
    char m_seedBuffer[16]    = "1234567890";

    float m_benchmarkBtHeight = 3.7f;
    float m_benchBtSpacing    = 5.0f;
    float m_benchBtFontSize   = 1.2f;
    float m_titleFontSize     = 1.8f;
    float m_startBtSize       = 2.0f;
    float m_startBtFontSize   = 1.2f;
    float m_subTitleFontSize  = 1.6f;
    float m_descFontSize      = 1.0f;
    float m_descHeight        = 0.55f;
    float m_optionsWidth      = 0.6f;
    float m_optTogFontSize    = 1.2f;
    float m_sceneSeedFontSize = 1.2f;

    // ANALYZER TAB VIEW -----------------------------------------------------------------------------------------------
    std::vector<RunEntry> m_runFiles;
    bool m_filterDeterministicOnly = false;
    bool m_filterValidOnly         = true;
    bool m_runListDirty            = true;
    bool m_sortAscending           = true;
    int  m_selectedRunIndex        = -1;

    // result panel docking
    bool  m_analyzerPanelOpen     = true;
    float m_analyzerWidth         = 0.0f;
    float m_lastAnalyzerWidth     = -1.0f;
    float m_minAnalyzerWidth      = 0.12f;
    float m_maxAnalyzerWidth      = 0.50f;
    float m_resultTitleFontSize   = 1.2f;
    float m_sortHeightPlaceHolder = 5.7f;
    float m_sortItemFontSize      = 1.2f;
    float m_selectableFontSize    = 1.0f;

    // Floating result windows
    std::vector<ResultTile> m_openResults;
    int m_nextResultId              = 0;
    float m_tileGapPix              = 4.0f;
    float m_tileSliceFactor         = 0.5f; // half the screen
    float m_tileColorFactorInactive = 0.30f;
    float m_tileColorFactorActive   = 0.50f;
    float m_metadataFontSize        = 1.0f;
    float m_pcInfoFontSize          = 1.0f;
    float m_statsFontSize           = 1.0f;
    float m_togglesFontSize         = 1.0f;
    float m_metricGapSize           = 16.0f;
    float m_graphLineWeightAnalyze  = 1.5f;

    // COMPARE TAB VIEW ------------------------------------------------------------------------------------------------
    std::vector<CompareSlot> m_compareSlots;
    bool  m_compareFitPending      = true;
    bool  m_compareShowDelta       = false;
    int   m_compareBaselineId      = -1;
    int   m_nextCompareId          = 0;
    int   m_maxCompareSlotOpen     = 7;
    float m_tableControlWidth      = 8.0f;
    float m_tableBaseWidth         = 6.0f;
    float m_graphLineWeightCompare = 1.5f;
    float m_compareCtrlFontSize    = 1.0f;
    float m_compareTableFontSize   = 1.0f;
    float m_compareGraphFontSize   = 1.0f;
};





























































