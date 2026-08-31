#pragma once
#include "imgui.h"

IMGUI_IMPL_API bool ImGui_ImplOpenGL3_Init(const char* glsl_version);
IMGUI_IMPL_API void ImGui_ImplOpenGL3_Shutdown();
IMGUI_IMPL_API void ImGui_ImplOpenGL3_NewFrame();
IMGUI_IMPL_API void ImGui_ImplOpenGL3_RenderDrawData(ImDrawData* draw_data);
