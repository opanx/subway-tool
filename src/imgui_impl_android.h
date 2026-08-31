#pragma once
#include "imgui.h"
#include <android/input.h>

struct ImGui_ImplAndroid_Data {
    int action;
    int pointCount;
    struct { float x, y; } points[10];
};

IMGUI_IMPL_API bool ImGui_ImplAndroid_Init(void* app);
IMGUI_IMPL_API void ImGui_ImplAndroid_Shutdown();
IMGUI_IMPL_API void ImGui_ImplAndroid_NewFrame();
IMGUI_IMPL_API void ImGui_ImplAndroid_HandleInputEvent(AInputEvent* event);
