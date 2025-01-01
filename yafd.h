#pragma once
#include "imgui.h"

#include <string>

namespace yafd {
void AddIcons();
bool FileDialog(const char* name, std::string* selected_path);
void ShowDemoWindow(bool* p_open = nullptr);
} // namespace yafd
