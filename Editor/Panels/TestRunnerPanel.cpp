#include "EditorLayer.h"
#include "EditorLayer.h"
#include "Artifact/Core/GameplayTest.h"
#include "Artifact/Scripting/ScriptManager.h"

#include <chrono>
#include <exception>
#include <imgui.h>
#include <sstream>
#include <string>
#include <unordered_map>

namespace Artifact::Editor {
    namespace {
        enum class TestStatus {
            NotRun,
            Running,
            Passed,
            Failed
        };

        struct TestRunState {
            TestStatus status = TestStatus::NotRun;
            double durationSeconds = 0.0;
            std::string message;
        };

        const char *StatusLabel(TestStatus status) {
            switch (status) {
                case TestStatus::Running: return "Running";
                case TestStatus::Passed: return "Passed";
                case TestStatus::Failed: return "Failed";
                case TestStatus::NotRun:
                default: return "Not run";
            }
        }

        ImVec4 StatusColor(TestStatus status) {
            switch (status) {
                case TestStatus::Running: return ImVec4(0.95f, 0.75f, 0.2f, 1.0f);
                case TestStatus::Passed: return ImVec4(0.3f, 0.85f, 0.3f, 1.0f);
                case TestStatus::Failed: return ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
                case TestStatus::NotRun:
                default: return ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
            }
        }

        class TestRunnerPanel : public IEditorPanel {
        public:
            const char *name() const override { return "Gameplay Tests"; }

            void draw(PanelContext &ctx) override {
                const char *title = ctx.windowTitle ? ctx.windowTitle : "Gameplay Tests";
                if (!ImGui::Begin(title, ctx.closeable ? ctx.openFlag : nullptr)) {
                    ImGui::End();
                    return;
                }

                auto &registry = Core::GameplayTestRegistry::instance();
                const auto &tests = registry.tests();

                ImGui::Text("Registered tests: %d", static_cast<int>(tests.size()));
                ImGui::SameLine();
                if (ImGui::Button("Run All")) {
                    runAllTests(ctx, tests);
                }
                ImGui::SameLine();
                ImGui::Checkbox("Auto scroll log", &autoScroll_);

                ImGui::Separator();
                drawTestsTable(ctx, tests);

                ImGui::Separator();
                ImGui::TextUnformatted("Run log:");
                ImGui::BeginChild("GameplayTestLog", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);
                if (!logBuffer_.empty()) {
                    ImGui::TextUnformatted(logBuffer_.c_str());
                    if (autoScroll_ && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
                        ImGui::SetScrollHereY(1.0f);
                    }
                } else {
                    ImGui::TextDisabled("No test output yet.");
                }
                ImGui::EndChild();

                ImGui::End();
            }

        private:
            void drawTestsTable(PanelContext &ctx, const std::vector<Core::GameplayTestRegistry::Entry> &tests) {
                if (tests.empty()) {
                    ImGui::TextDisabled(
                        "No gameplay tests are registered. Use ARTIFACT_REGISTER_GAMEPLAY_TEST in your modules to add them.");
                    return;
                }

                if (ImGui::BeginTable("GameplayTestsTable", 5,
                                      ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable)) {
                    ImGui::TableSetupColumn("Run", ImGuiTableColumnFlags_WidthFixed, 60.0f);
                    ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, 90.0f);
                    ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
                    ImGui::TableSetupColumn("Duration (ms)", ImGuiTableColumnFlags_WidthFixed, 110.0f);
                    ImGui::TableSetupColumn("Details", ImGuiTableColumnFlags_WidthStretch);
                    ImGui::TableHeadersRow();

                    for (const auto &entry: tests) {
                        auto &state = testStates_[entry.identifier];

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::PushID(entry.identifier.c_str());
                        if (ImGui::Button("Run")) {
                            runSingleTest(ctx, entry);
                        }
                        ImGui::PopID();

                        ImGui::TableSetColumnIndex(1);
                        ImGui::TextColored(StatusColor(state.status), "%s", StatusLabel(state.status));

                        ImGui::TableSetColumnIndex(2);
                        ImGui::TextUnformatted(entry.displayName.c_str());
                        if (!entry.description.empty()) {
                            ImGui::TextDisabled("%s", entry.description.c_str());
                        }

                        ImGui::TableSetColumnIndex(3);
                        if (state.status == TestStatus::Running) {
                            ImGui::TextUnformatted("...");
                        } else {
                            ImGui::Text("%.3f", state.durationSeconds * 1000.0);
                        }

                        ImGui::TableSetColumnIndex(4);
                        if (!state.message.empty()) {
                            ImGui::TextWrapped("%s", state.message.c_str());
                        } else {
                            ImGui::TextDisabled("No details");
                        }
                    }

                    ImGui::EndTable();
                }
            }

            void runAllTests(PanelContext &ctx, const std::vector<Core::GameplayTestRegistry::Entry> &tests) {
                if (tests.empty()) {
                    appendLog("No tests to run.\n");
                    return;
                }
                for (const auto &entry: tests) {
                    runSingleTest(ctx, entry);
                }
            }

            void runSingleTest(PanelContext &ctx, const Core::GameplayTestRegistry::Entry &entry) {
                auto &state = testStates_[entry.identifier];
                state.status = TestStatus::Running;
                state.durationSeconds = 0.0;
                state.message.clear();

                Core::GameplayTestContext testCtx{
                    ctx.scene, *Scripting::ScriptManager::Instance().GetRuntimeSystem(), ctx.playMode
                };
                auto testInstance = entry.factory ? entry.factory() : nullptr;
                if (!testInstance) {
                    state.status = TestStatus::Failed;
                    state.message = "Factory returned null instance";
                    appendLog(entry.displayName + ": unable to create test instance.\n");
                    return;
                }

                appendLog(entry.displayName + ": running...\n");
                const auto start = std::chrono::high_resolution_clock::now();
                Core::GameplayTestResult result;
                try {
                    result = testInstance->run(testCtx);
                } catch (const std::exception &ex) {
                    result.passed = false;
                    result.message = std::string{"Exception: "} + ex.what();
                } catch (...) {
                    result.passed = false;
                    result.message = "Unknown exception";
                }
                const auto end = std::chrono::high_resolution_clock::now();
                state.durationSeconds = std::chrono::duration<double>(end - start).count();
                state.status = result.passed ? TestStatus::Passed : TestStatus::Failed;
                state.message = result.message;

                std::ostringstream oss;
                oss << entry.displayName << ": " << StatusLabel(state.status)
                        << " in " << state.durationSeconds * 1000.0 << " ms";
                if (!state.message.empty()) {
                    oss << " — " << state.message;
                }
                oss << "\n";
                appendLog(oss.str());
            }

            void appendLog(const std::string &message) {
                logBuffer_ += message;
            }

        private:
            std::unordered_map<std::string, TestRunState> testStates_;
            bool autoScroll_ = true;
            std::string logBuffer_;
        };
    } // namespace

    PanelPtr CreateTestRunnerPanel() {
        return std::make_unique<TestRunnerPanel>();
    }
} // namespace Artifact::Editor
