
#include <algorithm>

#include "BenchmarkScene.h"
#include "MainMenuScene.h"
#include "Benchmark/BenchmarkRunPresets.hpp"
#include "Components/FreeCameraComponent.h"
#include "../../Engine/Source/Voxels/ChunkManager/ChunkManager.h"
#include "Render/RenderQueue.h"
#include "imgui.h"

// LOCAL ---------------------------------------------------------------------------------------------------------------

struct TechniqueStatus
{
    const char* label;
    bool enabled;
};

// PUBLIC --------------------------------------------------------------------------------------------------------------

BenchmarkScene::BenchmarkScene(const RR::RunInfo& _runInfo, const WORLDGEN::WorldGenConfig& _config)
    : VoxelScene(_runInfo, _config)
{
}

BenchmarkScene::BenchmarkScene(const RR::RunInfo& _runInfo)
    : VoxelScene(_runInfo)
{
}

BenchmarkScene::~BenchmarkScene()
= default;

// PROTECTED -----------------------------------------------------------------------------------------------------------

void BenchmarkScene::OnInit()
{
    // Benchmarks are non-resumable from pause
    SetResumable(false);
    SetPrimaryButtonText("RESTART BENCHMARK");
    SetSecondaryButtonText("EXIT TO MAIN MENU");

    // get hold of benchmark
    m_bench = RR::Engine::GetInstance().GetAppManager().GetSubSystem<RR::BenchmarkSubSystem>();

    // Create free cam
    m_cam     = CreateObject("BenchmarkCam");
    m_camComp = m_cam->AddComponent<RR::FreeCameraComponent>();
    m_cam->SetPosition(vec3(0.f, 120.f, 0.f));
    SetMainCamera(m_cam);

    SetCursorEnabled(false);
    m_camComp->SetDiscardInput(true);

    m_path = BENCH::GetCameraPath(BENCH::CAMERA_PATH_ID::DETERMINISTIC);
    ApplyCameraSample(m_path.Sample(0.0f));
}

void BenchmarkScene::OnUpdate(float _deltaTime)
{
    if (m_paused) return;

    if (!m_warmedUp)
    {
        // start load timer of first update
        if (!m_warmUpTimerStarted)
        {
            m_warmUpStart        = std::chrono::steady_clock::now();
            m_lastProgressTime   = m_warmUpStart;
            m_warmUpTimerStarted = true;
        }

        // time remaining metri
        const float warmElapsed = std::chrono::duration<float>(
            std::chrono::steady_clock::now() - m_warmUpStart).count();

        // stuck watchdog - abort if terrain stops generating
        const float coverage = m_chunkManager->GetCoverage();
        if (coverage > m_lastCoverage)
        {
            m_lastCoverage     = coverage;
            m_lastProgressTime = std::chrono::steady_clock::now();
        }
        else if (std::chrono::duration<float>(
            std::chrono::steady_clock::now() - m_lastProgressTime).count() > kStuckSeconds)
        {
            RR::Warn("[BENCHMARK] Loading stalled — no streaming progress, aborting");
            AbortRun();
            return;
        }

        // Time remaining, if projected time consistently stays above budgets,
        // abort run to avoid staying here the entire day
        if (warmElapsed > kPaceGraceSeconds && coverage > 0.0f)
        {
            const float projected = warmElapsed / coverage;

            if (projected > kMaxWarmUpSeconds * kPaceSlack)
            {
                RR::Warn("[BENCHMARK] Time remaining projected at '", static_cast<int>(projected),
                         "s against a ", static_cast<int>(kMaxWarmUpSeconds),
                         "s time limit, aborting early!");
                AbortRun();
                return;
            }
        }

        // hold at spawn until generated
        if (!m_chunkManager->IsStreamingIdle())
        {
            ApplyCameraSample(m_path.Sample(0.0f));
            return;
        }

        // warm up complete, record load time, begin run
        m_warmedUp = true;
        m_warmUpSeconds = std::chrono::duration<float>(
            std::chrono::steady_clock::now() - m_warmUpStart).count();
        // save into runinfo
        m_runInfo.warmUpSeconds = m_warmUpSeconds;

        // draw calls at settled point
        const auto& RQ = RR::Engine::GetInstance().GetRenderQueue();
        m_runInfo.steadyDraws = static_cast<int>(RQ.GetLastFrameDraws());
        m_runInfo.steadyTris = static_cast<int>(RQ.GetLastFrameTris());

        if (!m_bench)
        {
            RR::Error("[BENCHMARK] Benchmark subsystem not present!");
            return;
        }

        m_bench->RequestStartLogging(m_runInfo, kDiscardFrames);
    }

    m_simTime += std::min(_deltaTime, kMaxDeltaTime);
    ApplyCameraSample(m_path.Sample(m_simTime));

    // stack up "slow" frames if the application is lagging too much
    if (_deltaTime > kAbortFrameSeconds)
    {
        ++m_slowFrames;
    }
    else
    {
        m_slowFrames = 0;
    }

    // check if the camera is not standing on an ungenerate chunk
    const bool nullTerrain = !m_chunkManager->IsChunkMeshedAt(m_cam->GetWorldPosition());

    if (m_slowFrames >= kAbortSlowFrames || nullTerrain)
    {
        RR::Warn("[BENCHMARK] Aborting run, sustained consistent lag or went over null terrain");
        AbortRun();
        return;
    }

    if (m_bench)
    {
        // ship scene data directly to benchmark
        m_bench->RecordSceneMetrics(
            m_simTime,
            m_cam->GetWorldPosition(),
            m_chunkManager->GetCoverage()
        );
    }

    // Path complete, proceed
    if (!m_pathComplete && m_simTime >= m_path.Duration())
    {
        // log to disk
        if (m_bench) m_bench->RequestStopLogging();

        m_pathComplete = true;
        LoadNextScene();
    }
}

void BenchmarkScene::OnGui()
{
    VoxelScene::OnGui();

    // Discard overlay not in warm up phase
    if (m_warmedUp || m_paused) return;

    const ImGuiViewport* viewPort = ImGui::GetMainViewport();

    // dim scene when displaying
    ImGui::GetBackgroundDrawList()->AddRectFilled(
        viewPort->Pos,
        ImVec2(viewPort->Pos.x + viewPort->Size.x, viewPort->Pos.y + viewPort->Size.y),
        IM_COL32(0, 0, 0, 140));

    ImGui::SetNextWindowPos(viewPort->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(viewPort->WorkSize.x * 0.6f, 0.0f));
    ImGui::SetNextWindowBgAlpha(0.85f);

    constexpr ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoDecoration    | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs;

    ImGui::Begin("##benchmark_loading", nullptr, flags);

    // normalize font with scale
    const ImGuiStyle& style = ImGui::GetStyle();
    float baseFont = ImGui::GetFontSize();
    const float uiScale = style.FontScaleMain * style.FontScaleDpi;
    if (uiScale > 0.0f) baseFont /= uiScale;

    // centred line helper
    const auto centered = [](const std::string& _text)
    {
        const float width = ImGui::CalcTextSize(_text.c_str()).x;
        ImGui::SetCursorPosX((ImGui::GetWindowWidth() - width) * 0.5f);
        ImGui::TextUnformatted(_text.c_str());
    };

    // title
    ImGui::PushFont(ImGui::GetFont(), baseFont * titleFontScale);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.75f, 0.25f, 1.0f));
    ImGui::Dummy(ImVec2(0.0f, baseFont * 0.5f));
    centered("LOADING BENCHMARK");
    ImGui::PopStyleColor();
    ImGui::PopFont();

    ImGui::Dummy(ImVec2(0.0f, baseFont * 0.75f));

    ImGui::PushFont(ImGui::GetFont(), baseFont * buttonsTextSize);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.65f, 0.65f, 0.65f, 1.0f));

    // Mark sequence step or just custom
    ImGui::PushFont(ImGui::GetFont(), baseFont * buttonsTextSize * 1.15f);
    if (m_runInfo.deterministic)
    {
        centered("DETERMINISTIC " + std::to_string(DETERMINISTIC::gCurrentSceneStep + 1) +
                 " / "            + std::to_string(static_cast<int>(DETERMINISTIC::GetSceneCount())));
    }
    else
    {
        centered("CUSTOM");
    }
    ImGui::PopFont();

    ImGui::Dummy(ImVec2(0.0f, baseFont * 0.85f));

    // Show which techniques are on
    const TechniqueStatus statuses[] =
    {
        { "LEVEL OF DETAIL", m_runInfo.lod },
        { "LOD-AGGREGATION", m_runInfo.aggregation },
        { "GREEDY-MESHING", m_runInfo.greedy },
        { "MULTI-THREADING", m_runInfo.async },
        { "ADAPTIVE-BUDGETING",m_runInfo.scheduling },
    };

    const char* renderDistanceLabel = "RENDER DISTANCE";
    const std::string renderDistance = std::to_string(m_runInfo.renderDistance);

    float labelColumnWidth = 0.0f;
    for (const TechniqueStatus& status : statuses)
    {
        labelColumnWidth = std::max(labelColumnWidth, ImGui::CalcTextSize(status.label).x);
    }
    labelColumnWidth = std::max(labelColumnWidth, ImGui::CalcTextSize(renderDistanceLabel).x);

    // normalize table
    const float separatorWidth = ImGui::CalcTextSize(" : ").x;
    const float valueColumnWidth = std::max(ImGui::CalcTextSize("OFF").x,
                                            ImGui::CalcTextSize(renderDistance.c_str()).x);

    const float tableWidth = labelColumnWidth + separatorWidth + valueColumnWidth;
    const float tableStart = (ImGui::GetWindowWidth() - tableWidth) * 0.5f;
    const float valueStart = tableStart + labelColumnWidth + separatorWidth;

    // display each technique
    for (const TechniqueStatus& status : statuses)
    {
        ImGui::SetCursorPosX(tableStart);
        ImGui::TextUnformatted(status.label);

        ImGui::SameLine(tableStart + labelColumnWidth);
        ImGui::TextUnformatted(" : ");

        ImGui::SameLine(valueStart);
        const ImVec4 color = status.enabled ? ImVec4(0.47f, 0.78f, 1.0f, 1.0f)
                                            : ImVec4(0.45f, 0.45f, 0.45f, 1.0f);
        ImGui::TextColored(color, "%s", status.enabled ? "ON" : "OFF");
    }

    ImGui::SetCursorPosX(tableStart);
    ImGui::TextUnformatted(renderDistanceLabel);

    ImGui::SameLine(tableStart + labelColumnWidth);
    ImGui::TextUnformatted(" : ");

    ImGui::SameLine(valueStart);
    ImGui::TextUnformatted(renderDistance.c_str());

    ImGui::Dummy(ImVec2(0.0f, baseFont * 0.75f));
    centered("This might take a while...");

    // Use coverage to calculate amount of chunks rendered, not precise but gives feedback
    const int span   = 2 * m_runInfo.renderDistance + 1;
    const int total  = span * span;
    const int loaded = std::min(total,static_cast<int>(m_chunkManager->GetCoverage() * static_cast<float>(total)));

    const int percentage = total > 0 ? (100 * loaded) / total : 100;

    centered("Loaded " + std::to_string(loaded) + " / " + std::to_string(total) +
             " chunks (" + std::to_string(percentage) + "%)");

    // abort cooldown
    ImGui::Dummy(ImVec2(0.0f, baseFont * 0.75f));

    float elapsed = 0.0f;
    if (m_warmUpTimerStarted)
    {
        elapsed = std::chrono::duration<float>(
            std::chrono::steady_clock::now() - m_warmUpStart).count();
    }

    // live time remainign from start, padding given as it can be unrealiatic for some seconds
    // after starting
    const float paceCoverage = m_chunkManager->GetCoverage();
    if (elapsed > 1.0f && paceCoverage > 0.0f)
    {
        const float projected = elapsed / paceCoverage;
        const int   remaining = static_cast<int>(std::max(0.0f, projected - elapsed));

        if (projected > kMaxWarmUpSeconds * kPaceSlack)
        {
            // over time budget, warn!
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.55f, 0.35f, 1.0f));
            centered("Estimated time remaining: " + std::to_string(remaining) +
                     "s (!) Might abort!");
            ImGui::PopStyleColor();
        }
        else
        {
            centered("Estimated time remaining: " + std::to_string(remaining) + "s");
        }
    }
    else
    {
        centered("Estimated time remaining: calculating...");
    }

    ImGui::PopStyleColor();
    ImGui::PopFont();
    ImGui::Dummy(ImVec2(0.0f, baseFont * 0.5f));
    ImGui::End();
}

void BenchmarkScene::OnPauseEnter()
{
    VoxelScene::OnPauseEnter();
    // Discard benchmark on pause
    if (m_bench) m_bench->RequestDiscard();
}

void BenchmarkScene::OnPausePrimary()
{
    auto& appMan = RR::Engine::GetInstance().GetAppManager();

    // if deterministic and restart prompted, reload entire sequence
    if (m_runInfo.deterministic)
    {
        using namespace DETERMINISTIC;

        gCurrentSceneStep = 0;
        gDeterministicFailures = 0;

        appMan.RequestSceneLoad<BenchmarkScene>(GetRunPreset(0));
        return;
    }

    // For custom only, restart this exact scene
    appMan.RequestSceneLoad<BenchmarkScene>(m_runInfo, m_genConfig);
}

void BenchmarkScene::OnPauseSecondary()
{
    // User has quit
    auto& appMan = RR::Engine::GetInstance().GetAppManager();
    appMan.RequestSceneLoad<MainMenuScene>(BENCH_SUCCESS::CANCELLED);
}

void BenchmarkScene::AbortRun()
{
    if (m_pathComplete) return;

    m_pathComplete = true;

    // discard fail
    if (m_bench) m_bench->RequestDiscard();

    // deterministic, remeber if any fails
    if (m_runInfo.deterministic)
    {
        ++DETERMINISTIC::gDeterministicFailures;
        LoadNextScene();
    }
    else
    {
        RR::Engine::GetInstance().GetAppManager().RequestSceneLoad<MainMenuScene>(BENCH_SUCCESS::ABORTED);
    }
}

void BenchmarkScene::LoadNextScene()
{
    auto& appMan = RR::Engine::GetInstance().GetAppManager();

    if (!m_runInfo.deterministic)
    {
        appMan.RequestSceneLoad<MainMenuScene>(BENCH_SUCCESS::CUSTOM);
        return;
    }

    using namespace DETERMINISTIC;

    // Hit end of sequence, return to main menu
    if (gCurrentSceneStep + 1 == GetSceneCount())
    {
        const BENCH_SUCCESS outcome = (gDeterministicFailures == 0)
            ? BENCH_SUCCESS::DETERMINISTIC
            : BENCH_SUCCESS::DETERMINISTIC_PARTIAL;
        RR::Success("[DETERMINISTIC BENCHMARK] Deterministic benchmark has concluded");
        appMan.RequestSceneLoad<MainMenuScene>(outcome);
        return;
    }

    // Load next scene in sequence
    gCurrentSceneStep++;
    RR::RunInfo info = GetRunPreset(gCurrentSceneStep);
    appMan.RequestSceneLoad<BenchmarkScene>(info);
}

void BenchmarkScene::ApplyCameraSample(const BENCH::CameraSample& _sample)
{
    const quat qYaw   = glm::angleAxis(glm::radians(_sample.yaw), vec3(0.f, 1.f, 0.f));
    const quat qPitch = glm::angleAxis(glm::radians(_sample.pitch), vec3(1.f, 0.f, 0.f));

    m_cam->SetWorldRotation(glm::normalize(qYaw * qPitch));
    m_cam->SetWorldPosition(_sample.position);
}










