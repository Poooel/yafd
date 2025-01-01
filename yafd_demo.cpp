#include "imgui.h"
#include "yafd.h"

#include <string>

namespace yafd {
void ShowDemoWindow(bool* p_open) {
    // Set up window flags
    ImGuiWindowFlags window_flags = 0;
    if (!ImGui::Begin("YAFD Demo", p_open, window_flags)) {
        ImGui::End();
        return;
    }

    //-------------------------------------------------------------------------
    // Basic usage
    //-------------------------------------------------------------------------
    ImGui::TextWrapped(
        "YAFD (Yet Another File Dialog) provides simple file dialogs that "
        "follow ImGui's immediate mode API style."
    );
    ImGui::Spacing();

    static bool        show_basic_dialog = false;
    static std::string basic_selected_file;
    if (ImGui::Button("Open File Dialog")) {
        show_basic_dialog = true;
        basic_selected_file.clear(); // Reset selection
    }
    if (show_basic_dialog) {
        if (yafd::FileDialog("Choose File##Basic", &basic_selected_file)) {
            show_basic_dialog = false;
        }
    }
    if (!basic_selected_file.empty()) {
        ImGui::Text("Selected: %s", basic_selected_file.c_str());
    }

    //-------------------------------------------------------------------------
    // Multiple dialogs
    //-------------------------------------------------------------------------
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::TextWrapped(
        "You can have multiple file dialogs, each with their own state:"
    );

    static bool        show_dialog1 = false, show_dialog2 = false;
    static std::string dialog1_file, dialog2_file;

    ImGui::BeginGroup();
    if (ImGui::Button("Open Dialog 1")) {
        show_dialog1 = true;
        dialog1_file.clear();
    }
    if (show_dialog1) {
        if (yafd::FileDialog("Choose File##Dialog1", &dialog1_file)) {
            show_dialog1 = false;
        }
    }
    if (!dialog1_file.empty()) {
        ImGui::Text("Dialog 1: %s", dialog1_file.c_str());
    }
    ImGui::EndGroup();

    ImGui::BeginGroup();
    if (ImGui::Button("Open Dialog 2")) {
        show_dialog2 = true;
        dialog2_file.clear();
    }
    if (show_dialog2) {
        if (yafd::FileDialog("Choose File##Dialog2", &dialog2_file)) {
            show_dialog2 = false;
        }
    }
    if (!dialog2_file.empty()) {
        ImGui::Text("Dialog 2: %s", dialog2_file.c_str());
    }
    ImGui::EndGroup();

    //-------------------------------------------------------------------------
    // Code examples
    //-------------------------------------------------------------------------
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::Text("Code");
    ImGui::Spacing();

    ImGui::TextWrapped("Basic usage:");
    ImGui::Spacing();
    const char* basic_code =
        "static bool show_dialog = false;\n"
        "static std::string selected_file;\n\n"
        "if (ImGui::Button(\"Open File Dialog\")) {\n"
        "    show_dialog = true;\n"
        "}\n\n"
        "if (show_dialog) {\n"
        "    if (yafd::FileDialog(\"Choose File\", &selected_file)) {\n"
        "        show_dialog = false;\n"
        "        // Handle selected file\n"
        "    }\n"
        "}";
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
    ImGui::BeginChild(
        "Code1", ImVec2(0, ImGui::GetTextLineHeightWithSpacing() * 11), true
    );
    ImGui::TextUnformatted(basic_code);
    ImGui::EndChild();
    ImGui::PopStyleColor();

    ImGui::End();
}
} // namespace yafd
