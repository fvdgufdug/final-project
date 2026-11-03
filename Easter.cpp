#include "Globals.h"

#define PL_MPEG_IMPLEMENTATION
#include "pl_mpeg.h"

#include <vector>
#include <cstdio>

const int kKonamiSequence[10] = {
    GLFW_KEY_UP, GLFW_KEY_UP, GLFW_KEY_DOWN, GLFW_KEY_DOWN,
    GLFW_KEY_LEFT, GLFW_KEY_RIGHT, GLFW_KEY_LEFT, GLFW_KEY_RIGHT,
    GLFW_KEY_B, GLFW_KEY_A
};

int g_konamiProgress = 0;

namespace
{
    const char* kVideoFile = "nevergonna.mpg";

    const float kTitleH   = 20.0f;
    const float kVideoW   = 320.0f;
    const float kMargin   = 16.0f;

    plm_t*              g_plm      = nullptr;
    unsigned int        g_tex      = 0;
    unsigned int        g_prog     = 0;
    unsigned int        g_vao      = 0;
    unsigned int        g_vbo      = 0;
    int                 g_vw       = 0;
    int                 g_vh       = 0;
    std::vector<unsigned char> g_rgb;

    bool  g_active   = false;
    float g_x        = 0.0f;
    float g_y        = 0.0f;
    bool  g_placed   = false;
    bool  g_dragging = false;
    double g_dragDX  = 0.0, g_dragDY = 0.0;

    float picH() { return (g_vw > 0) ? kVideoW * (float)g_vh / (float)g_vw : kVideoW * 9.0f / 16.0f; }

    void onAudioSamples(plm_t*, plm_samples_t* samples, void*)
    {
        audioVideoPush(samples->interleaved, (int)samples->count);
    }

    void onVideoFrame(plm_t*, plm_frame_t* frame, void*)
    {
        if (g_rgb.empty()) return;
        plm_frame_to_rgb(frame, g_rgb.data(), g_vw * 3);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glBindTexture(GL_TEXTURE_2D, g_tex);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, g_vw, g_vh, GL_RGB, GL_UNSIGNED_BYTE, g_rgb.data());
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    }

    bool initGpu()
    {
        if (g_prog) return true;
        g_prog = loadShaderProgram("shaders/video.vert", "shaders/video.frag");
        if (!g_prog) return false;
        glGenVertexArrays(1, &g_vao);
        glGenBuffers(1, &g_vbo);
        glBindVertexArray(g_vao);
        glBindBuffer(GL_ARRAY_BUFFER, g_vbo);
        glBufferData(GL_ARRAY_BUFFER, 6 * 4 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glBindVertexArray(0);
        return true;
    }
}

bool videoActive() { return g_active; }

VideoRect videoRect()
{
    VideoRect r;
    r.x = g_x; r.y = g_y; r.w = kVideoW; r.titleH = kTitleH; r.picH = picH();
    return r;
}

void videoStop()
{
    g_active = false;
    g_dragging = false;
    audioVideoClear();
    if (g_plm) { plm_destroy(g_plm); g_plm = nullptr; }
    if (g_tex) { glDeleteTextures(1, &g_tex); g_tex = 0; }
    g_rgb.clear();
}

void videoStart(GLFWwindow* window)
{
    if (g_active) { videoStop(); return; }

    g_plm = plm_create_with_filename(kVideoFile);
    if (!g_plm)
    {
        printf("[video] cannot open '%s' (put it next to the executable)\n", kVideoFile);
        printf("[video] convert once with: ffmpeg -i nevergonna.mp4 -vf scale=320:-2 -c:v mpeg1video -b:v 1500k -c:a mp2 -ar 44100 -ac 2 -b:a 128k nevergonna.mpg\n");
        pushToast("Video missing: nevergonna.mpg (see console)");
        return;
    }
    if (!initGpu())
    {
        printf("[video] shader build failed\n");
        plm_destroy(g_plm); g_plm = nullptr;
        return;
    }

    g_vw = plm_get_width(g_plm);
    g_vh = plm_get_height(g_plm);
    if (g_vw <= 0 || g_vh <= 0)
    {
        printf("[video] '%s' has no usable MPEG-1 video stream\n", kVideoFile);
        plm_destroy(g_plm); g_plm = nullptr;
        pushToast("Video file is not MPEG-1 (see console)");
        return;
    }

    if (plm_get_num_audio_streams(g_plm) > 0)
    {
        plm_set_audio_enabled(g_plm, TRUE);
        plm_set_audio_stream(g_plm, 0);
        plm_set_audio_lead_time(g_plm, 0.05);
        plm_set_audio_decode_callback(g_plm, onAudioSamples, nullptr);
        if (plm_get_samplerate(g_plm) != 44100)
            printf("[video] audio is %d Hz, expected 44100 Hz: re-convert with -ar 44100\n", plm_get_samplerate(g_plm));
    }
    else
    {
        plm_set_audio_enabled(g_plm, FALSE);
        printf("[video] '%s' has no audio track (re-convert without -an)\n", kVideoFile);
    }
    plm_set_loop(g_plm, TRUE);
    plm_set_video_decode_callback(g_plm, onVideoFrame, nullptr);
    audioVideoClear();

    g_rgb.assign((size_t)g_vw * g_vh * 3, 0);
    glGenTextures(1, &g_tex);
    glBindTexture(GL_TEXTURE_2D, g_tex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, g_vw, g_vh, 0, GL_RGB, GL_UNSIGNED_BYTE, g_rgb.data());
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    int winW = 0, winH = 0;
    glfwGetWindowSize(window, &winW, &winH);
    if (!g_placed)
    {
        g_x = kMargin;
        g_y = (float)winH - kTitleH - picH() - kMargin - 40.0f;
        if (g_y < kMargin) g_y = kMargin;
        g_placed = true;
    }
    g_active = true;
    pushToast("Video window: drag the title bar (TAB for cursor), X or V closes");
}

void videoUpdate(GLFWwindow* window, float dt)
{
    if (!g_active || !g_plm) return;

    if (dt > 0.1f) dt = 0.1f;
    plm_decode(g_plm, (double)dt);

    if (g_dragging)
    {
        double mx, my;
        glfwGetCursorPos(window, &mx, &my);
        int winW, winH;
        glfwGetWindowSize(window, &winW, &winH);
        float w = kVideoW, h = kTitleH + picH();
        g_x = (float)(mx - g_dragDX);
        g_y = (float)(my - g_dragDY);
        if (g_x < 0) g_x = 0;
        if (g_y < 0) g_y = 0;
        if (g_x > winW - w) g_x = (float)winW - w;
        if (g_y > winH - h) g_y = (float)winH - h;
    }
}

bool videoHandleMouse(GLFWwindow* window, int button, int action)
{
    if (!g_active || cursorDisabled) return false;
    if (button != GLFW_MOUSE_BUTTON_LEFT) return false;

    if (action == GLFW_RELEASE)
    {
        bool was = g_dragging;
        g_dragging = false;
        return was;
    }

    double mx, my;
    glfwGetCursorPos(window, &mx, &my);
    float w = kVideoW, h = kTitleH + picH();
    if (mx < g_x || mx > g_x + w || my < g_y || my > g_y + h) return false;

    if (my <= g_y + kTitleH && mx >= g_x + w - kTitleH)
    {
        videoStop();
        return true;
    }
    if (my <= g_y + kTitleH)
    {
        g_dragging = true;
        g_dragDX = mx - g_x;
        g_dragDY = my - g_y;
    }
    return true;
}

void videoDraw(int winW, int winH)
{
    if (!g_active || !g_plm || !g_prog) return;

    float x0 = g_x, y0 = g_y + kTitleH, x1 = g_x + kVideoW, y1 = g_y + kTitleH + picH();
    float v[24] = {
        x0, y0, 0.0f, 0.0f,
        x1, y0, 1.0f, 0.0f,
        x1, y1, 1.0f, 1.0f,
        x0, y0, 0.0f, 0.0f,
        x1, y1, 1.0f, 1.0f,
        x0, y1, 0.0f, 1.0f
    };

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);

    glUseProgram(g_prog);
    glm::mat4 ortho = glm::ortho(0.0f, (float)winW, (float)winH, 0.0f, -1.0f, 1.0f);
    glUniformMatrix4fv(glGetUniformLocation(g_prog, "uOrtho"), 1, GL_FALSE, glm::value_ptr(ortho));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, g_tex);
    glUniform1i(glGetUniformLocation(g_prog, "uTex"), 0);

    glBindVertexArray(g_vao);
    glBindBuffer(GL_ARRAY_BUFFER, g_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(v), v);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    glEnable(GL_DEPTH_TEST);
}

void triggerKonamiEasterEgg()
{
    pushToast("Never gonna give you up...");
    audioPlayUiBlip();
    videoStart(glfwGetCurrentContext());
}

void feedKonamiKey(int key)
{
    if (key == kKonamiSequence[g_konamiProgress])
    {
        ++g_konamiProgress;
        if (g_konamiProgress >= (int)(sizeof(kKonamiSequence) / sizeof(kKonamiSequence[0])))
        {
            g_konamiProgress = 0;
            triggerKonamiEasterEgg();
        }
    }
    else
    {
        g_konamiProgress = (key == kKonamiSequence[0]) ? 1 : 0;
    }
}
