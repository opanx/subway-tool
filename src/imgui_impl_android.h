#pragma once
#include "imgui.h"

// Android backend
struct ImGui_ImplAndroid_Data {
    struct android_app* app;
    int action;
    int pointCount;
    struct { float x, y; } points[10];
};

IMGUI_IMPL_API bool ImGui_ImplAndroid_Init(struct android_app* app);
IMGUI_IMPL_API void ImGui_ImplAndroid_Shutdown();
IMGUI_IMPL_API void ImGui_ImplAndroid_NewFrame();
IMGUI_IMPL_API void ImGui_ImplAndroid_HandleInputEvent(AInputEvent* event);
