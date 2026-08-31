#include "imgui_impl_android.h"
#include "imgui.h"
#include <android/input.h>
#include <android/log.h>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "ImGui", __VA_ARGS__)

static ImGui_ImplAndroid_Data* g_Data = nullptr;

bool ImGui_ImplAndroid_Init(struct android_app* app) {
    g_Data = IM_NEW(ImGui_ImplAndroid_Data)();
    g_Data->app = app;
    g_Data->action = 0;
    g_Data->pointCount = 0;
    ImGuiIO& io = ImGui::GetIO();
    io.MouseDrawCursor = true;
    io.PlatformName = "Android";
    return true;
}

void ImGui_ImplAndroid_Shutdown() {
    IM_DELETE(g_Data);
    g_Data = nullptr;
}

void ImGui_ImplAndroid_HandleInputEvent(AInputEvent* event) {
    if (!g_Data) return;

    int32_t type = AInputEvent_getType(event);
    if (type == AINPUT_EVENT_TYPE_MOTION) {
        int32_t action = AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
        int32_t pointerCount = AMotionEvent_getPointerCount(event);

        ImGuiIO& io = ImGui::GetIO();

        switch (action) {
        case AMOTION_EVENT_ACTION_DOWN:
        case AMOTION_EVENT_ACTION_POINTER_DOWN:
            g_Data->action = 1;
            g_Data->pointCount = pointerCount;
            for (int i = 0; i < pointerCount && i < 10; i++) {
                g_Data->points[i].x = AMotionEvent_getX(event, i);
                g_Data->points[i].y = AMotionEvent_getY(event, i);
            }
            if (pointerCount > 0) {
                io.MousePos = ImVec2(g_Data->points[0].x, g_Data->points[0].y);
                io.MouseDown[0] = true;
            }
            break;

        case AMOTION_EVENT_ACTION_UP:
        case AMOTION_EVENT_ACTION_POINTER_UP:
            g_Data->action = 0;
            io.MouseDown[0] = false;
            break;

        case AMOTION_EVENT_ACTION_MOVE:
            g_Data->pointCount = pointerCount;
            for (int i = 0; i < pointerCount && i < 10; i++) {
                g_Data->points[i].x = AMotionEvent_getX(event, i);
                g_Data->points[i].y = AMotionEvent_getY(event, i);
            }
            if (pointerCount > 0) {
                io.MousePos = ImVec2(g_Data->points[0].x, g_Data->points[0].y);
            }
            break;
        }
    } else if (type == AINPUT_EVENT_TYPE_KEY) {
        int32_t keyCode = AKeyEvent_getKeyCode(event);
        int32_t action = AKeyEvent_getAction(event);
        ImGuiIO& io = ImGui::GetIO();

        if (action == AKEY_EVENT_ACTION_DOWN) {
            if (keyCode >= AKEYCODE_0 && keyCode <= AKEYCODE_9)
                io.AddInputCharacter((unsigned int)(keyCode - AKEYCODE_0 + '0'));
            else if (keyCode == AKEYCODE_DEL)
                io.AddInputCharacter(8); // backspace
            else if (keyCode == AKEYCODE_ENTER)
                io.AddInputCharacter(13); // enter
        }
    }
}

void ImGui_ImplAndroid_NewFrame() {
    // Handled by input events above
}
