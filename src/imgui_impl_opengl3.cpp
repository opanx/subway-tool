#include "imgui_impl_opengl3.h"
#include "imgui.h"
#include <GLES3/gl3.h>
#include <android/log.h>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "ImGuiGL", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "ImGuiGL", __VA_ARGS__)

static GLuint g_ShaderHandle = 0;
static GLint  g_AttribLocationTex = 0;
static GLint  g_AttribLocationProjMtx = 0;
static GLuint g_AttribLocationVtxPos = 0;
static GLuint g_AttribLocationVtxUV = 0;
static GLuint g_AttribLocationVtxColor = 0;
static GLuint g_VboHandle = 0, g_VaoHandle = 0, g_EboHandle = 0;

static const char* vertex_shader =
    "#version 300 es\n"
    "precision mediump float;\n"
    "layout(location = 0) in vec2 Position;\n"
    "layout(location = 1) in vec2 UV;\n"
    "layout(location = 2) in vec4 Color;\n"
    "uniform mat4 ProjMtx;\n"
    "out vec2 Frag_UV;\n"
    "out vec4 Frag_Color;\n"
    "void main() {\n"
    "    Frag_UV = UV;\n"
    "    Frag_Color = Color;\n"
    "    gl_Position = ProjMtx * vec4(Position.xy, 0.0, 1.0);\n"
    "}\n";

static const char* fragment_shader =
    "#version 300 es\n"
    "precision mediump float;\n"
    "in vec2 Frag_UV;\n"
    "in vec4 Frag_Color;\n"
    "uniform sampler2D Texture;\n"
    "layout(location = 0) out vec4 Out_Color;\n"
    "void main() {\n"
    "    Out_Color = Frag_Color * texture(Texture, Frag_UV.st);\n"
    "}\n";

bool ImGui_ImplOpenGL3_Init(const char* /*glsl_version*/) {
    ImGuiIO& io = ImGui::GetIO();

    // Create shaders
    g_ShaderHandle = glCreateProgram();
    GLuint vert = glCreateShader(GL_VERTEX_SHADER);
    GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);

    glShaderSource(vert, 1, &vertex_shader, nullptr);
    glCompileShader(vert);
    GLint ok;
    glGetShaderiv(vert, GL_COMPILE_STATUS, &ok);
    if (!ok) { LOGE("Vertex shader compile failed"); return false; }

    glShaderSource(frag, 1, &fragment_shader, nullptr);
    glCompileShader(frag);
    glGetShaderiv(frag, GL_COMPILE_STATUS, &ok);
    if (!ok) { LOGE("Fragment shader compile failed"); return false; }

    glAttachShader(g_ShaderHandle, vert);
    glAttachShader(g_ShaderHandle, frag);
    glLinkProgram(g_ShaderHandle);

    glGetProgramiv(g_ShaderHandle, GL_LINK_STATUS, &ok);
    if (!ok) { LOGE("Shader link failed"); return false; }

    g_AttribLocationTex = glGetUniformLocation(g_ShaderHandle, "Texture");
    g_AttribLocationProjMtx = glGetUniformLocation(g_ShaderHandle, "ProjMtx");
    g_AttribLocationVtxPos = (GLuint)glGetAttribLocation(g_ShaderHandle, "Position");
    g_AttribLocationVtxUV = (GLuint)glGetAttribLocation(g_ShaderHandle, "UV");
    g_AttribLocationVtxColor = (GLuint)glGetAttribLocation(g_ShaderHandle, "Color");

    // Create buffers
    glGenBuffers(1, &g_VboHandle);
    glGenBuffers(1, &g_EboHandle);
    glGenVertexArrays(1, &g_VaoHandle);

    // Setup VAO
    glBindVertexArray(g_VaoHandle);
    glBindBuffer(GL_ARRAY_BUFFER, g_VboHandle);
    glEnableVertexAttribArray(g_AttribLocationVtxPos);
    glEnableVertexAttribArray(g_AttribLocationVtxUV);
    glEnableVertexAttribArray(g_AttribLocationVtxColor);

    glVertexAttribPointer(g_AttribLocationVtxPos, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)offsetof(ImDrawVert, pos));
    glVertexAttribPointer(g_AttribLocationVtxUV, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)offsetof(ImDrawVert, uv));
    glVertexAttribPointer(g_AttribLocationVtxColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert), (GLvoid*)offsetof(ImDrawVert, col));

    glBindVertexArray(0);

    // Create font texture
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    io.Fonts->SetTexID((ImTextureID)(intptr_t)tex);

    LOGI("ImGui OpenGL3 initialized: %dx%d font texture", width, height);
    return true;
}

void ImGui_ImplOpenGL3_Shutdown() {
    if (g_VaoHandle) glDeleteVertexArrays(1, &g_VaoHandle);
    if (g_VboHandle) glDeleteBuffers(1, &g_VboHandle);
    if (g_EboHandle) glDeleteBuffers(1, &g_EboHandle);
    if (g_ShaderHandle) glDeleteProgram(g_ShaderHandle);
    g_VaoHandle = g_VboHandle = g_EboHandle = g_ShaderHandle = 0;
}

void ImGui_ImplOpenGL3_NewFrame() {
    // Nothing needed - handled by ImGui
}

void ImGui_ImplOpenGL3_RenderDrawData(ImDrawData* draw_data) {
    ImGuiIO& io = ImGui::GetIO();
    int fb_width = (int)(io.DisplaySize.x * io.DisplayFramebufferScale.x);
    int fb_height = (int)(io.DisplaySize.y * io.DisplayFramebufferScale.y);
    if (fb_width == 0 || fb_height == 0) return;

    draw_data->ScaleClipRects(io.DisplayFramebufferScale);

    // Setup state
    GLboolean last_enable_blend = glIsEnabled(GL_BLEND);
    GLboolean last_enable_cull_face = glIsEnabled(GL_CULL_FACE);
    GLboolean last_enable_scissor = glIsEnabled(GL_SCISSOR_TEST);
    GLboolean last_enable_depth_test = glIsEnabled(GL_DEPTH_TEST);

    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glEnable(GL_SCISSOR_TEST);
    glDisable(GL_DEPTH_TEST);

    // Setup orthographic projection
    float L = 0.0f;
    float R = io.DisplaySize.x;
    float T = 0.0f;
    float B = io.DisplaySize.y;
    const float proj[] = {
        2.0f/(R-L),   0.0f,         0.0f,   0.0f,
        0.0f,         2.0f/(T-B),   0.0f,   0.0f,
        0.0f,         0.0f,        -1.0f,   0.0f,
        (R+L)/(L-R), (T+B)/(B-T),  0.0f,   1.0f,
    };

    glUseProgram(g_ShaderHandle);
    glUniform1i(g_AttribLocationTex, 0);
    glUniformMatrix4fv(g_AttribLocationProjMtx, 1, GL_FALSE, proj);

    glBindVertexArray(g_VaoHandle);

    for (int n = 0; n < draw_data->CmdListsCount; n++) {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];

        // Upload vertex buffer
        glBindBuffer(GL_ARRAY_BUFFER, g_VboHandle);
        glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)cmd_list->VtxBuffer.Size * sizeof(ImDrawVert),
                     cmd_list->VtxBuffer.Data, GL_STREAM_DRAW);

        // Upload index buffer
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_EboHandle);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx),
                     cmd_list->IdxBuffer.Data, GL_STREAM_DRAW);

        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++) {
            const ImDrawCmd& cmd = cmd_list->CmdBuffer[cmd_i];
            if (cmd.UserCallback) {
                cmd.UserCallback(cmd_list, &cmd);
            } else {
                ImVec4 clip_rect = ImVec4(cmd.ClipRect.x, cmd.ClipRect.y, cmd.ClipRect.z, cmd.ClipRect.w);

                // Apply scissor
                glScissor(
                    (int)clip_rect.x,
                    (int)(fb_height - clip_rect.w),
                    (int)(clip_rect.z - clip_rect.x),
                    (int)(clip_rect.w - clip_rect.y)
                );

                // Bind texture
                glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)cmd.TextureId);

                // Draw
                glDrawElements(GL_TRIANGLES, (GLsizei)cmd.ElemCount,
                              sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT,
                              (void*)(intptr_t)(cmd.IdxOffset * sizeof(ImDrawIdx)));
            }
        }
    }

    // Restore state
    if (last_enable_blend) glEnable(GL_BLEND); else glDisable(GL_BLEND);
    if (last_enable_cull_face) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
    if (last_enable_scissor) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
    if (last_enable_depth_test) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);

    glBindVertexArray(0);
    glUseProgram(0);
}
