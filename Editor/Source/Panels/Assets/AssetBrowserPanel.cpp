#include "../Core/Panel.h"
#include "Artifact/Assets/AssetManager.h"
#include "Artifact/Assets/Asset.h"
#include "Artifact/Core/Paths.h"
#include "Artifact/Core/LogManager.h"

#include <imgui.h>
#include <imgui_stdlib.h>
#include <filesystem>
#include <vector>
#include <string>
#include <algorithm>

namespace Artifact::Editor {
    namespace {

        /**
         * @brief Asset browser panel - Unity-style asset viewer
         *
         * Features:
         * - Folder tree on the left
         * - Asset grid/list on the right with variable thumbnail sizes
         * - Search and filter by type
         * - Breadcrumb navigation
         * - Preview thumbnails (future enhancement)
         * - Drag-drop support (future enhancement)
         */
        class AssetBrowserPanel : public IEditorPanel {
        public:
            // Nested type definitions must come first

            struct AssetInfo {
                std::filesystem::path path;
                std::string name;
                std::uintmax_t fileSize = 0;
                Assets::AssetType type = Assets::AssetType::Unknown;
                std::string typeStr;
            };

            AssetBrowserPanel() {
                // Initialize with Assets root
                m_assetsRoot = Core::Paths::ProjectRoot() / "Assets";
                if (std::filesystem::exists(m_assetsRoot)) {
                    m_currentPath = m_assetsRoot;
                    refreshAssetList();
                }
            }

            const char* name() const override {
                return "Asset Browser";
            }

            void draw(PanelContext& ctx) override {
                const char* title = ctx.windowTitle ? ctx.windowTitle : "Asset Browser";
                if (!ImGui::Begin(title, ctx.closeable ? ctx.openFlag : nullptr)) {
                    ImGui::End();
                    return;
                }

                // Top toolbar with search and filters
                drawToolbar();

                ImGui::Separator();

                // Split view: folder tree on left, asset grid on right
                ImGui::BeginChild("##FolderTree", ImVec2(250, 0), true);
                drawFolderTree(m_assetsRoot);
                ImGui::EndChild();

                ImGui::SameLine();

                ImGui::BeginChild("##AssetView", ImVec2(0, 0), true);
                drawAssetView();
                ImGui::EndChild();

                ImGui::End();
            }

        private:
            void drawToolbar() {
                // Search bar (wider like Unity)
                ImGui::SetNextItemWidth(250.0f);
                if (ImGui::InputTextWithHint("##Search", "Search Assets", &m_searchQuery)) {
                    refreshAssetList();
                }

                ImGui::SameLine();

                // Filter by type
                const char* filterTypes[] = {"All", "Meshes", "Textures", "Shaders", "Scripts"};
                ImGui::SetNextItemWidth(120.0f);
                if (ImGui::Combo("##FilterType", &m_filterTypeIndex, filterTypes, IM_ARRAYSIZE(filterTypes))) {
                    refreshAssetList();
                }

                ImGui::SameLine();
                ImGui::Spacing();
                ImGui::SameLine();

                // Thumbnail size slider with visual feedback
                ImGui::PushItemWidth(100.0f);
                ImGui::SliderFloat("##ThumbnailSize", &m_thumbnailSize, 0.0f, 1.0f, "");
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Asset Size");
                }
                ImGui::PopItemWidth();

                // Push refresh and asset count to the right
                ImGui::SameLine();
                float remainingWidth = ImGui::GetContentRegionAvail().x;
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + remainingWidth - 150.0f);
                
                if (ImGui::Button("Refresh", ImVec2(70, 0))) {
                    refreshAssetList();
                }

                ImGui::SameLine();
                ImGui::TextDisabled("%zu assets", m_filteredAssets.size());
            }

            void drawFolderTree(const std::filesystem::path& path) {
                if (!std::filesystem::exists(path) || !std::filesystem::is_directory(path)) {
                    return;
                }

                std::filesystem::path relativePath = std::filesystem::relative(path, m_assetsRoot);
                std::string label = relativePath.empty() ? "Assets" : relativePath.filename().string();

                bool isSelected = (path == m_currentPath);
                ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;

                if (isSelected) {
                    flags |= ImGuiTreeNodeFlags_Selected;
                }

                // Check if folder has subdirectories
                bool hasChildren = false;
                try {
                    for (const auto& entry : std::filesystem::directory_iterator(path)) {
                        if (entry.is_directory()) {
                            hasChildren = true;
                            break;
                        }
                    }
                } catch (...) {}

                if (!hasChildren) {
                    flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
                }

                // Add folder icon
                std::string labelWithIcon = hasChildren ? (std::string("[+] ") + label) : (std::string("[ ] ") + label);
                bool nodeOpen = ImGui::TreeNodeEx(labelWithIcon.c_str(), flags);

                if (ImGui::IsItemClicked()) {
                    m_currentPath = path;
                    refreshAssetList();
                }

                if (nodeOpen && hasChildren) {
                    // Draw subdirectories
                    try {
                        std::vector<std::filesystem::path> subdirs;
                        for (const auto& entry : std::filesystem::directory_iterator(path)) {
                            if (entry.is_directory()) {
                                subdirs.push_back(entry.path());
                            }
                        }

                        // Sort alphabetically
                        std::sort(subdirs.begin(), subdirs.end());

                        for (const auto& subdir : subdirs) {
                            drawFolderTree(subdir);
                        }
                    } catch (...) {}

                    ImGui::TreePop();
                }
            }

            void drawAssetView() {
                // Breadcrumb navigation with back button
                drawBreadcrumbs();

                ImGui::Separator();

                if (m_filteredAssets.empty()) {
                    // Better empty state like Unity
                    ImVec2 center = ImGui::GetWindowSize();
                    center.x *= 0.5f;
                    center.y *= 0.4f;
                    ImGui::SetCursorPos(ImVec2(center.x - 50, center.y));
                    ImGui::TextDisabled("This folder is empty");
                    return;
                }

                // Grid view with variable thumbnail size (Unity doesn't have list view in asset browser)
                drawAssetGrid();
            }

            void drawBreadcrumbs() {
                // Back button (Unity-style)
                if (m_currentPath != m_assetsRoot) {
                    if (ImGui::Button("< Back")) {
                        m_currentPath = m_currentPath.parent_path();
                        refreshAssetList();
                    }
                    ImGui::SameLine();
                    ImGui::Spacing();
                    ImGui::SameLine();
                } else {
                    // Disabled back button
                    ImGui::BeginDisabled();
                    ImGui::Button("< Back");
                    ImGui::EndDisabled();
                    ImGui::SameLine();
                    ImGui::Spacing();
                    ImGui::SameLine();
                }

                std::filesystem::path relativePath = std::filesystem::relative(m_currentPath, m_assetsRoot);

                if (ImGui::SmallButton("Assets")) {
                    m_currentPath = m_assetsRoot;
                    refreshAssetList();
                }

                std::filesystem::path accumulated = m_assetsRoot;
                for (const auto& part : relativePath) {
                    ImGui::SameLine();
                    ImGui::TextDisabled(">");
                    ImGui::SameLine();

                    accumulated /= part;
                    std::string partStr = part.string();
                    if (ImGui::SmallButton(partStr.c_str())) {
                        m_currentPath = accumulated;
                        refreshAssetList();
                    }
                }
            }

            void drawAssetGrid() {
                // Calculate thumbnail size based on slider (64px to 128px range)
                const float minSize = 64.0f;
                const float maxSize = 128.0f;
                const float thumbnailSize = minSize + (maxSize - minSize) * m_thumbnailSize;
                const float padding = 8.0f;
                const float cellSize = thumbnailSize + padding * 2;
                const float panelWidth = ImGui::GetContentRegionAvail().x;
                const int columns = std::max(1, static_cast<int>(panelWidth / cellSize));

                ImGui::Columns(columns, nullptr, false);

                for (const auto& asset : m_filteredAssets) {
                    drawAssetThumbnail(asset, thumbnailSize);
                    ImGui::NextColumn();
                }

                ImGui::Columns(1);
            }

            void drawAssetList() {
                if (ImGui::BeginTable("##AssetList", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY)) {
                    ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
                    ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 100.0f);
                    ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 80.0f);
                    ImGui::TableHeadersRow();

                    for (const auto& asset : m_filteredAssets) {
                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();

                        bool selected = (m_selectedAsset == asset.path);
                        if (ImGui::Selectable(asset.name.c_str(), selected, ImGuiSelectableFlags_SpanAllColumns)) {
                            m_selectedAsset = asset.path;
                        }

                        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                            openAsset(asset);
                        }

                        ImGui::TableNextColumn();
                        ImGui::TextDisabled("%s", asset.typeStr.c_str());

                        ImGui::TableNextColumn();
                        ImGui::TextDisabled("%s", formatFileSize(asset.fileSize).c_str());
                    }

                    ImGui::EndTable();
                }
            }

            void drawAssetThumbnail(const AssetInfo& asset, float size) {
                ImGui::BeginGroup();

                bool selected = (m_selectedAsset == asset.path);
                bool hovered = false;

                // Thumbnail placeholder (future: render actual previews)
                ImVec2 thumbnailMin = ImGui::GetCursorScreenPos();
                ImVec2 thumbnailMax = ImVec2(thumbnailMin.x + size, thumbnailMin.y + size);

                ImDrawList* drawList = ImGui::GetWindowDrawList();

                // Background with Unity-style gradient
                ImU32 bgColor = selected ? IM_COL32(62, 95, 150, 255) : IM_COL32(56, 56, 56, 255);
                drawList->AddRectFilled(thumbnailMin, thumbnailMax, bgColor, 2.0f);

                // Icon based on type with better scaling
                ImU32 iconColor = getTypeColor(asset.type);
                const char* icon = getTypeIcon(asset.type);
                float iconSize = size * 0.35f;

                ImVec2 iconPos = ImVec2(
                    thumbnailMin.x + size * 0.5f - iconSize * 0.4f,
                    thumbnailMin.y + size * 0.5f - iconSize * 0.5f
                );
                drawList->AddText(ImGui::GetFont(), iconSize, iconPos, iconColor, icon);

                // Border - Unity uses subtle borders
                ImU32 borderColor = selected ? IM_COL32(58, 121, 187, 255) : IM_COL32(35, 35, 35, 255);
                drawList->AddRect(thumbnailMin, thumbnailMax, borderColor, 2.0f, 0, 1.0f);

                ImGui::InvisibleButton(asset.name.c_str(), ImVec2(size, size));

                if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                    auto root = Artifact::Core::Paths::ProjectRoot();
                    auto relative = std::filesystem::relative(asset.path, root);
                    std::string relativeStr = relative.string();
                    std::replace(relativeStr.begin(), relativeStr.end(), '\\', '/');

                    ImGui::SetDragDropPayload("ASSET_PATH", relativeStr.c_str(), relativeStr.size() + 1);

                    ImGui::Text("%s", asset.name.c_str());
                    ImGui::EndDragDropSource();
                }

                if (ImGui::IsItemHovered()) {
                    // Hover highlight
                    drawList->AddRect(thumbnailMin, thumbnailMax, IM_COL32(100, 150, 200, 180), 2.0f, 0, 2.0f);
                    
                    if (ImGui::IsMouseClicked(0)) {
                        m_selectedAsset = asset.path;
                    }
                    if (ImGui::IsMouseDoubleClicked(0)) {
                        openAsset(asset);
                    }
                    ImGui::SetTooltip("%s\n%s - %s",
                        asset.name.c_str(),
                        asset.typeStr.c_str(),
                        formatFileSize(asset.fileSize).c_str()
                    );
                }

                // Label with better wrapping
                float textWidth = size;
                ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + textWidth);
                
                // Highlight label if selected
                if (selected) {
                    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(220, 220, 220, 255));
                }
                
                ImGui::TextWrapped("%s", asset.name.c_str());
                
                if (selected) {
                    ImGui::PopStyleColor();
                }
                
                ImGui::PopTextWrapPos();

                ImGui::EndGroup();
            }

            void refreshAssetList() {
                m_assets.clear();
                m_filteredAssets.clear();

                if (!std::filesystem::exists(m_currentPath)) {
                    return;
                }

                try {
                    for (const auto& entry : std::filesystem::directory_iterator(m_currentPath)) {
                        if (entry.is_regular_file()) {
                            AssetInfo info;
                            info.path = entry.path();
                            info.name = entry.path().filename().string();
                            info.fileSize = entry.file_size();
                            info.type = getAssetTypeFromExtension(entry.path().extension().string());
                            info.typeStr = Assets::AssetTypeToString(info.type);

                            m_assets.push_back(info);
                        }
                    }
                } catch (const std::exception& e) {
                    Core::LogManager::Instance().Error("Error reading directory: {}", e.what());
                }

                // Sort alphabetically
                std::sort(m_assets.begin(), m_assets.end(), [](const AssetInfo& a, const AssetInfo& b) {
                    return a.name < b.name;
                });

                // Apply filters
                applyFilters();
            }

            void applyFilters() {
                m_filteredAssets.clear();

                for (const auto& asset : m_assets) {
                    // Filter by type
                    if (m_filterTypeIndex > 0) {
                        Assets::AssetType filterType = getFilterType(m_filterTypeIndex);
                        if (asset.type != filterType) {
                            continue;
                        }
                    }

                    // Filter by search query
                    if (!m_searchQuery.empty()) {
                        std::string nameLower = asset.name;
                        std::string queryLower = m_searchQuery;
                        std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);
                        std::transform(queryLower.begin(), queryLower.end(), queryLower.begin(), ::tolower);

                        if (nameLower.find(queryLower) == std::string::npos) {
                            continue;
                        }
                    }

                    m_filteredAssets.push_back(asset);
                }
            }

            Assets::AssetType getAssetTypeFromExtension(const std::string& ext) {
                auto it = Assets::ExtensionMap.find(ext);
                return it != Assets::ExtensionMap.end() ? it->second : Assets::AssetType::Unknown;
            }

            Assets::AssetType getFilterType(int index) {
                switch (index) {
                    case 1: return Assets::AssetType::Mesh;
                    case 2: return Assets::AssetType::Texture;
                    case 3: return Assets::AssetType::Shader;
                    case 4: return Assets::AssetType::Script;
                    default: return Assets::AssetType::Unknown;
                }
            }

            const char* getTypeIcon(Assets::AssetType type) {
                switch (type) {
                    case Assets::AssetType::Mesh: return "M";
                    case Assets::AssetType::Texture: return "T";
                    case Assets::AssetType::Shader: return "S";
                    case Assets::AssetType::Script: return "Lua";
                    case Assets::AssetType::Audio: return "A";
                    case Assets::AssetType::Scene: return "Scn";
                    case Assets::AssetType::Prefab: return "P";
                    default: return "?";
                }
            }

            ImU32 getTypeColor(Assets::AssetType type) {
                // Unity-inspired colors
                switch (type) {
                    case Assets::AssetType::Mesh: return IM_COL32(82, 176, 222, 255);      // Light blue
                    case Assets::AssetType::Texture: return IM_COL32(222, 82, 176, 255);   // Pink/Magenta
                    case Assets::AssetType::Shader: return IM_COL32(82, 222, 131, 255);    // Teal/Green
                    case Assets::AssetType::Script: return IM_COL32(255, 195, 0, 255);     // Yellow/Gold
                    case Assets::AssetType::Audio: return IM_COL32(222, 131, 82, 255);     // Orange
                    case Assets::AssetType::Scene: return IM_COL32(131, 82, 222, 255);     // Purple
                    case Assets::AssetType::Prefab: return IM_COL32(82, 131, 222, 255);    // Blue
                    default: return IM_COL32(150, 150, 150, 255);                          // Gray
                }
            }

            std::string formatFileSize(std::uintmax_t size) {
                const char* units[] = {"B", "KB", "MB", "GB"};
                int unitIndex = 0;
                double sizeDouble = static_cast<double>(size);

                while (sizeDouble >= 1024.0 && unitIndex < 3) {
                    sizeDouble /= 1024.0;
                    ++unitIndex;
                }

                char buffer[32];
                std::snprintf(buffer, sizeof(buffer), "%.2f %s", sizeDouble, units[unitIndex]);
                return buffer;
            }

            void openAsset(const AssetInfo& asset) {
                Core::LogManager::Instance().Info("Opening asset: {}", asset.path.string());

                // Future: Implement asset-specific actions
                // - Meshes: Load in viewport
                // - Textures: Show in texture viewer
                // - Scripts: Open in code editor
                // - etc.
            }

            // Member variables
            std::filesystem::path m_assetsRoot;
            std::filesystem::path m_currentPath;
            std::filesystem::path m_selectedAsset;

            std::vector<AssetInfo> m_assets;
            std::vector<AssetInfo> m_filteredAssets;

            std::string m_searchQuery;
            int m_filterTypeIndex = 0;
            float m_thumbnailSize = 1.0f; // 0.0 = smallest (64px), 1.0 = largest (128px)
            float m_folderTreeWidth = 200.0f;
        };
    }

    PanelPtr CreateAssetBrowserPanel() {
        return std::make_unique<AssetBrowserPanel>();
    }
}

