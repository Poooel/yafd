#include "yafd.h"

#include "IconsFontAwesome4.h"
#include "fontawesome4.h"
#include "imgui.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <unordered_map>
#include <vector>

namespace yafd {
namespace {
ImFont* fontAwesome4 = nullptr;

enum class SortColumn {
    Name,
    Size,
    Modified
};

struct DialogState {
        std::filesystem::path current_path = std::filesystem::current_path();
        std::string           selected_file;
        bool                  is_open          = false;
        SortColumn            sort_column      = SortColumn::Name;
        bool                  sort_ascending   = true;
        bool                  just_changed_dir = false;
};

struct FileItem {
        std::string                     name;
        bool                            is_directory;
        uintmax_t                       size;
        std::filesystem::file_time_type modified_time;
};

// Store states by dialog name
static std::unordered_map<std::string, DialogState> s_dialog_states;

// Helper function to format file time
std::string FormatFileTime(const std::filesystem::file_time_type& filetime) {
    auto sctp =
        std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            filetime - std::filesystem::file_time_type::clock::now() +
            std::chrono::system_clock::now()
        );

    std::time_t tt = std::chrono::system_clock::to_time_t(sctp);
    std::tm*    tm = std::localtime(&tt);

    std::ostringstream ss;
    ss << std::put_time(tm, "%Y-%m-%d %H:%M");
    return ss.str();
}

// Helper function to sort file items
void SortFileItems(
    std::vector<FileItem>& items, SortColumn sort_column, bool ascending
) {
    std::sort(
        items.begin(),
        items.end(),
        [sort_column, ascending](const FileItem& a, const FileItem& b) {
            int compare = 0;
            switch (sort_column) {
                case SortColumn::Name:
                    compare = a.name.compare(b.name);
                    break;
                case SortColumn::Size:
                    compare = (a.size < b.size)   ? -1
                              : (a.size > b.size) ? 1
                                                  : 0;
                    break;
                case SortColumn::Modified:
                    {
                        // Convert to duration since epoch for reliable
                        // comparison
                        auto a_time =
                            std::chrono::clock_cast<std::chrono::system_clock>(
                                a.modified_time
                            )
                                .time_since_epoch();
                        auto b_time =
                            std::chrono::clock_cast<std::chrono::system_clock>(
                                b.modified_time
                            )
                                .time_since_epoch();
                        compare = (a_time < b_time)   ? -1
                                  : (a_time > b_time) ? 1
                                                      : 0;
                        break;
                    }
            }
            return ascending ? (compare < 0) : (compare > 0);
        }
    );
}
} // namespace

void AddIcons() {
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->AddFontDefault();

    constexpr float baseFontSize = 13.0f;
    constexpr float iconFontSize = baseFontSize * 2.0f / 3.0f;

    static constexpr ImWchar iconsRanges[] = { ICON_MIN_FA, ICON_MAX_16_FA, 0 };
    ImFontConfig             iconsConfig;
    iconsConfig.PixelSnapH       = true;
    iconsConfig.GlyphMinAdvanceX = iconFontSize;
    iconsConfig.GlyphOffset      = ImVec2(0.0f, 2.0f);
    fontAwesome4                 = io.Fonts->AddFontFromMemoryCompressedTTF(
        Font::FontAwesome4CompressedData,
        Font::FontAwesome4CompressedSize,
        iconFontSize,
        &iconsConfig,
        iconsRanges
    );
}

bool FileDialog(const char* name, std::string* selected_path) {
    auto& state    = s_dialog_states[name];
    bool  selected = false;

    if (!state.is_open) {
        state.is_open = true;
        ImGui::OpenPopup(name);
    }

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal(
            name, nullptr, ImGuiWindowFlags_AlwaysAutoResize
        )) {
        // File list table
        static const float ICON_COLUMN_WIDTH = 30.0f;
        static const float NAME_COLUMN_WIDTH = 300.0f;
        static const float SIZE_COLUMN_WIDTH = 70.0f;
        static const float DATE_COLUMN_WIDTH = 140.0f;

        const float total_width = ICON_COLUMN_WIDTH + NAME_COLUMN_WIDTH +
                                  SIZE_COLUMN_WIDTH + DATE_COLUMN_WIDTH + 20;

        // Breadcrumb navigation
        std::vector<std::filesystem::path> path_components;
        std::filesystem::path              current = state.current_path;

        // Build path components from root to current
        while (current != current.parent_path()) {
            path_components.push_back(current);
            current = current.parent_path();
        }
        if (path_components.empty() || current != path_components.back()) {
            path_components.push_back(current);
        }
        std::reverse(path_components.begin(), path_components.end());

        // Calculate total width needed
        float total_breadcrumb_width = ImGui::CalcTextSize("/").x;

        // Add widths of all components
        for (size_t i = 1; i < path_components.size(); ++i) {
            total_breadcrumb_width +=
                ImGui::CalcTextSize(" > ").x; // separator width
            total_breadcrumb_width +=
                ImGui::CalcTextSize(
                    path_components[i].filename().string().c_str()
                )
                    .x;
            total_breadcrumb_width +=
                ImGui::GetStyle().FramePadding.x * 2; // Button padding
            total_breadcrumb_width +=
                ImGui::GetStyle().ItemSpacing.x; // Spacing between items
        }

        // Tell ImGui how wide our content is
        ImGui::SetNextWindowContentSize(ImVec2(total_breadcrumb_width, 0.0f));

        /// Start of horizontal scrolling region for breadcrumbs
        ImGui::BeginChild(
            "##breadcrumb_scroll",
            ImVec2(
                total_width, ImGui::GetFrameHeight()
            ), // Use total_width here
            false,
            ImGuiWindowFlags_NoScrollbar
        );

        // Handle mouse wheel for horizontal scrolling
        if (ImGui::IsWindowHovered()) {
            float wheel = ImGui::GetIO().MouseWheel;
            if (wheel != 0) {
                float scroll_amount =
                    wheel * 50.0f; // Adjust multiplier to control scroll speed
                ImGui::SetScrollX(ImGui::GetScrollX() - scroll_amount);
            }
        }

        // If we just changed directory, scroll to the end
        if (state.just_changed_dir) {
            ImGui::SetScrollX(99999.0f); // Will be clamped to max
            state.just_changed_dir =
                false; // Reset flag so user can scroll freely
        }

        // Render the actual breadcrumbs
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));

        if (ImGui::Button("/")) {
            state.current_path     = "/";
            state.just_changed_dir = true;
        }

        for (size_t i = 1; i < path_components.size(); ++i) {
            ImGui::SameLine(0.0f, 2.0f);
            if (fontAwesome4) {
                ImGui::PushFont(fontAwesome4);
                ImGui::TextUnformatted(ICON_FA_CHEVRON_RIGHT);
                ImGui::PopFont();
            } else {
                ImGui::TextUnformatted(">");
            }
            ImGui::SameLine(0.0f, 2.0f);
            if (ImGui::Button(path_components[i].filename().string().c_str())) {
                state.current_path     = path_components[i];
                state.just_changed_dir = true;
            }
        }
        ImGui::PopStyleColor(1);

        ImGui::EndChild();

        // Get directory contents and sort
        std::vector<FileItem> files;
        try {
            for (const auto& entry:
                 std::filesystem::directory_iterator(state.current_path)) {
                FileItem item;
                item.name          = entry.path().filename().string();
                item.is_directory  = entry.is_directory();
                item.size          = item.is_directory ? 0 : entry.file_size();
                item.modified_time = entry.last_write_time();
                files.push_back(item);
            }
        } catch (const std::filesystem::filesystem_error& e) {
            ImGui::TextColored(
                ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error accessing directory!"
            );
        }

        SortFileItems(files, state.sort_column, state.sort_ascending);

        // Calculate height for 10 rows
        const float row_height =
            ImGui::GetTextLineHeight() + (ImGui::GetStyle().CellPadding.y * 2);
        const float header_height =
            row_height + ImGui::GetStyle().ItemSpacing.y;
        const float table_height = (row_height * 10) + header_height;

        if (ImGui::BeginTable(
                "Files",
                4,
                ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable |
                    ImGuiTableFlags_Reorderable | ImGuiTableFlags_SortMulti |
                    ImGuiTableFlags_Sortable | ImGuiTableFlags_ScrollY,
                ImVec2(total_width, table_height)
            )) {
            ImGui::TableSetupColumn(
                "",
                ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort,
                ICON_COLUMN_WIDTH
            );
            ImGui::TableSetupColumn(
                "Name",
                ImGuiTableColumnFlags_WidthFixed |
                    ImGuiTableColumnFlags_DefaultSort,
                NAME_COLUMN_WIDTH
            );
            ImGui::TableSetupColumn(
                "Size", ImGuiTableColumnFlags_WidthFixed, SIZE_COLUMN_WIDTH
            );
            ImGui::TableSetupColumn(
                "Modified", ImGuiTableColumnFlags_WidthFixed, DATE_COLUMN_WIDTH
            );
            ImGui::TableHeadersRow();

            // Handle sorting
            if (ImGuiTableSortSpecs* sorts_specs = ImGui::TableGetSortSpecs()) {
                if (sorts_specs->SpecsDirty) {
                    if (sorts_specs->SpecsCount > 0) {
                        const ImGuiTableColumnSortSpecs* sort_spec =
                            &sorts_specs->Specs[0];
                        state.sort_ascending = sort_spec->SortDirection ==
                                               ImGuiSortDirection_Ascending;
                        switch (sort_spec->ColumnIndex) {
                            case 1:
                                state.sort_column = SortColumn::Name;
                                break;
                            case 2:
                                state.sort_column = SortColumn::Size;
                                break;
                            case 3:
                                state.sort_column = SortColumn::Modified;
                                break;
                        }
                        SortFileItems(
                            files, state.sort_column, state.sort_ascending
                        );
                    }
                    sorts_specs->SpecsDirty = false;
                }
            }

            for (const auto& item: files) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();

                // Icon column
                if (fontAwesome4) {
                    ImGui::PushFont(fontAwesome4);
                    if (item.is_directory) {
                        ImGui::TextUnformatted(ICON_FA_FOLDER);
                    } else {
                        ImGui::TextUnformatted(ICON_FA_FILE_O);
                    }
                    ImGui::PopFont();
                } else {
                    if (item.is_directory) {
                        ImGui::TextUnformatted("[D]");
                    } else {
                        ImGui::TextUnformatted("[F]");
                    }
                }

                // Name column
                ImGui::TableNextColumn();
                const bool is_selected =
                    (!item.is_directory && state.selected_file == item.name);
                if (ImGui::Selectable(
                        item.name.c_str(),
                        is_selected,
                        ImGuiSelectableFlags_SpanAllColumns |
                            ImGuiSelectableFlags_AllowDoubleClick
                    )) {
                    if (ImGui::IsMouseDoubleClicked(0)) {
                        if (item.is_directory) {
                            state.current_path /= item.name;
                            state.selected_file.clear();
                            state.just_changed_dir = true;
                        } else {
                            state.selected_file = item.name;
                            *selected_path =
                                (state.current_path / state.selected_file)
                                    .string();
                            ImGui::CloseCurrentPopup();
                            state.is_open = false;
                            selected      = true;
                        }
                    } else if (item.is_directory) {
                        state.selected_file.clear();
                    } else {
                        state.selected_file = item.name;
                    }
                }

                // Size column
                ImGui::TableNextColumn();
                if (!item.is_directory) {
                    const auto filesize = item.size;
                    if (filesize < 1024) {
                        ImGui::Text("%lu B", filesize);
                    } else if (filesize < 1024 * 1024) {
                        ImGui::Text("%.1f KB", filesize / 1024.0);
                    } else if (filesize < 1024 * 1024 * 1024) {
                        ImGui::Text("%.1f MB", filesize / (1024.0 * 1024.0));
                    } else {
                        ImGui::Text(
                            "%.1f GB", filesize / (1024.0 * 1024.0 * 1024.0)
                        );
                    }
                }

                // Modified time
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(
                    FormatFileTime(item.modified_time).c_str()
                );
            }

            ImGui::EndTable();
        }

        // Buttons
        if (ImGui::Button("OK", ImVec2(120, 0)) &&
            !state.selected_file.empty()) {
            *selected_path =
                (state.current_path / state.selected_file).string();
            ImGui::CloseCurrentPopup();
            state.is_open = false;
            selected      = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            selected_path->clear();
            ImGui::CloseCurrentPopup();
            state.is_open = false;
            selected      = true;
        }

        // Close on ESC
        if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            ImGui::CloseCurrentPopup();
            selected_path->clear();
            ImGui::CloseCurrentPopup();
            state.is_open = false;
            selected      = true;
        }

        ImGui::EndPopup();
    }

    return selected;
}
} // namespace yafd
