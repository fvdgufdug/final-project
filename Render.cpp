#include "Globals.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_easy_font.h"



unsigned int compileShader(unsigned int type, const char* src)
{
    unsigned int s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr); glCompileShader(s);
    int ok; char log[512];
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) { glGetShaderInfoLog(s, 512, nullptr, log);std::cout << "Shader err:\n" << log << "\n"; }
    return s;
}



unsigned int buildShaderProgram(const char* vs, const char* fs)
{
    unsigned int v = compileShader(GL_VERTEX_SHADER, vs), f = compileShader(GL_FRAGMENT_SHADER, fs);
    unsigned int p = glCreateProgram();
    glAttachShader(p, v); glAttachShader(p, f); glLinkProgram(p);
    int ok; char log[512];
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) { glGetProgramInfoLog(p, 512, nullptr, log);std::cout << "Link err:\n" << log << "\n"; }
    glDeleteShader(v); glDeleteShader(f);
    return p;
}



unsigned int loadTexture(const char* path)
{
    unsigned int id;
    glGenTextures(1, &id); glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    stbi_set_flip_vertically_on_load(false);
    int w, h, ch;
    unsigned char* data = stbi_load(path, &w, &h, &ch, 0);
    if (data)
    {
        GLenum fmt = (ch == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, fmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Warning: could not load texture: " << path << "\n";
        unsigned char white[] = { 255,255,255 };
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, white);
    }
    stbi_image_free(data);
    return id;
}



void buildSphere(unsigned int sectors, unsigned int stacks,
    std::vector<float>& verts, std::vector<unsigned int>& idx)
{
    const float PI = 3.14159265359f;

    for (unsigned int i = 0; i <= stacks; ++i)
    {
        float phi = PI / 2.0f - i * PI / stacks;
        float xy = cosf(phi);
        float y = sinf(phi);

        for (unsigned int j = 0; j <= sectors; ++j)
        {
            float theta = j * 2.0f * PI / sectors;

            float x = xy * cosf(theta);
            float z = xy * sinf(theta);

            verts.push_back(x);
            verts.push_back(y);
            verts.push_back(z);

            verts.push_back(x);
            verts.push_back(y);
            verts.push_back(z);

            verts.push_back((float)j / sectors);
            verts.push_back((float)i / stacks);
        }
    }

    for (unsigned int i = 0; i < stacks; ++i)
    {
        unsigned int k1 = i * (sectors + 1);
        unsigned int k2 = k1 + sectors + 1;

        for (unsigned int j = 0; j < sectors; ++j, ++k1, ++k2)
        {
            if (i != 0)
            {
                idx.push_back(k1);
                idx.push_back(k2);
                idx.push_back(k1 + 1);
            }

            if (i != stacks - 1)
            {
                idx.push_back(k1 + 1);
                idx.push_back(k2);
                idx.push_back(k2 + 1);
            }
        }
    }
}



void buildRing(float inner, float outer, int segs,
    std::vector<float>& verts, std::vector<unsigned int>& idx)
{
    const float PI = 3.14159265359f;
    for (int i = 0;i <= segs;++i)
    {
        float a = 2.0f * PI * i / segs, cx = cosf(a), cz = sinf(a);
        float v = a / (2.0f * PI);
        verts.push_back(cx * outer); verts.push_back(0.0f); verts.push_back(cz * outer);
        verts.push_back(0.0f);     verts.push_back(1.0f); verts.push_back(0.0f);
        verts.push_back(1.0f);     verts.push_back(v);
        verts.push_back(cx * inner); verts.push_back(0.0f); verts.push_back(cz * inner);
        verts.push_back(0.0f);     verts.push_back(1.0f); verts.push_back(0.0f);
        verts.push_back(0.0f);     verts.push_back(v);
    }
    for (int i = 0;i < segs;++i)
    {
        int o1 = i * 2, i1 = i * 2 + 1, o2 = (i + 1) * 2, i2 = (i + 1) * 2 + 1;
        idx.push_back(o1);idx.push_back(i1);idx.push_back(o2);
        idx.push_back(o2);idx.push_back(i1);idx.push_back(i2);
    }
}



void buildOrbitCircle(int segs, std::vector<float>& verts)
{
    const float PI = 3.14159265359f;
    for (int i = 0;i < segs;++i)
    {
        float a = 2.0f * PI * i / segs;
        verts.push_back(cosf(a)); verts.push_back(0.0f); verts.push_back(sinf(a));
        verts.push_back(0.0f);    verts.push_back(1.0f); verts.push_back(0.0f);
    }
}



void buildShootingStarLine(std::vector<float>& verts)
{

    verts.push_back(0.0f); verts.push_back(0.0f); verts.push_back(0.0f);

    verts.push_back(1.0f); verts.push_back(0.0f); verts.push_back(0.0f);
}



GpuMesh uploadIndexed(const std::vector<float>& v, const std::vector<unsigned int>& idx, int stride, bool hasUV)
{
    GpuMesh m; m.indexCount = (unsigned int)idx.size();
    glGenVertexArrays(1, &m.VAO); glGenBuffers(1, &m.VBO); glGenBuffers(1, &m.EBO);
    glBindVertexArray(m.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m.VBO);
    glBufferData(GL_ARRAY_BUFFER, v.size() * sizeof(float), v.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx.size() * sizeof(unsigned int), idx.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride * sizeof(float), (void*)0); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride * sizeof(float), (void*)(3 * sizeof(float))); glEnableVertexAttribArray(1);
    if (hasUV) { glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride * sizeof(float), (void*)(6 * sizeof(float)));glEnableVertexAttribArray(2); }
    glBindVertexArray(0);
    return m;
}



GpuMesh uploadVertexOnly(const std::vector<float>& v, int fpv)
{
    GpuMesh m; m.vertexCount = (unsigned int)(v.size() / fpv);
    glGenVertexArrays(1, &m.VAO); glGenBuffers(1, &m.VBO);
    glBindVertexArray(m.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m.VBO);
    glBufferData(GL_ARRAY_BUFFER, v.size() * sizeof(float), v.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, fpv * sizeof(float), (void*)0); glEnableVertexAttribArray(0);
    if (fpv == 6) { glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));glEnableVertexAttribArray(1); }
    glBindVertexArray(0);
    return m;
}



void drawLine3D(const GpuMesh& m, const glm::vec3& a, const glm::vec3& b)
{
    float lineData[6] = { a.x,a.y,a.z, b.x,b.y,b.z };
    glBindVertexArray(m.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m.VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(lineData), lineData, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glDrawArrays(GL_LINES, 0, 2);
    glBindVertexArray(0);
}



void drawUiQuad(unsigned int vao, unsigned int vbo,
    float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3,
    unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
    unsigned char verts[4 * 16];
    auto setV = [&](int i, float x, float y) {
        float* fp = (float*)(verts + i * 16);
        fp[0] = x; fp[1] = y; fp[2] = 0.0f;
        unsigned char* cp = verts + i * 16 + 12;
        cp[0] = r; cp[1] = g; cp[2] = b; cp[3] = a;
        };
    setV(0, x0, y0); setV(1, x1, y1); setV(2, x2, y2); setV(3, x3, y3);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}



void drawUiRect(unsigned int vao, unsigned int vbo, float x, float y, float w, float h,
    unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
    drawUiQuad(vao, vbo, x, y, x + w, y, x + w, y + h, x, y + h, r, g, b, a);
}



void drawUiLine(unsigned int vao, unsigned int vbo, float x0, float y0, float x1, float y1,
    float thickness, unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
    float dx = x1 - x0, dy = y1 - y0;
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 0.0001f) return;
    float px = -dy / len * thickness * 0.5f;
    float py = dx / len * thickness * 0.5f;
    drawUiQuad(vao, vbo, x0 + px, y0 + py, x1 + px, y1 + py, x1 - px, y1 - py, x0 - px, y0 - py, r, g, b, a);
}



void drawUiText(unsigned int vao, unsigned int vbo, float x, float y, const char* text,
    unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
    static char buf[32 * 1024];
    unsigned char color[4] = { r, g, b, a };
    int quads = stb_easy_font_print(x, y, const_cast<char*>(text), color, buf, sizeof(buf));
    if (quads <= 0) return;
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, (long)quads * 4 * 16, buf);
    glDrawElements(GL_TRIANGLES, quads * 6, GL_UNSIGNED_INT, 0);
}



void showLoading(GLFWwindow* window, const char* message)
{
    glfwSetWindowTitle(window, message);

    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glfwSwapBuffers(window);
    glfwPollEvents();
}

