LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := panxcz_subway

LOCAL_CFLAGS := -std=c++17
LOCAL_CFLAGS += -fvisibility=hidden
LOCAL_CPPFLAGS := -std=c++17
LOCAL_CPPFLAGS += -fvisibility=hidden
LOCAL_CFLAGS += -DUSE_OPENGL
LOCAL_CPPFLAGS += -DUSE_OPENGL
LOCAL_CPPFLAGS += -w -s -Wno-error=format-security -fvisibility=hidden -std=c++17
LOCAL_CPPFLAGS += -Wno-error=c++11-narrowing -fpermissive -Wall -fexceptions

LOCAL_C_INCLUDES += $(LOCAL_PATH)/include
LOCAL_C_INCLUDES += $(LOCAL_PATH)/include/ImGui
LOCAL_C_INCLUDES += $(LOCAL_PATH)/include/ImGui/backends
LOCAL_C_INCLUDES += $(LOCAL_PATH)/include/Memory
LOCAL_C_INCLUDES += $(LOCAL_PATH)/include/native_surface

LOCAL_SRC_FILES := \
    src/main.cpp \
    src/Android_draw/draw.cpp \
    src/Android_touch/TouchHelperA.cpp \
    src/ImGui/imgui.cpp \
    src/ImGui/imgui_draw.cpp \
    src/ImGui/imgui_widgets.cpp \
    src/ImGui/imgui_tables.cpp \
    src/ImGui/imgui_demo.cpp \
    src/ImGui/backends/imgui_impl_android.cpp \
    src/ImGui/backends/imgui_impl_opengl3.cpp \
    src/Memory/Memory.cpp \
    src/Memory/MemoryTools.cpp

LOCAL_LDLIBS := -llog -landroid -lEGL -lGLESv3

include $(BUILD_EXECUTABLE)
