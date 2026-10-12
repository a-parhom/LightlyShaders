/*
    Renders the LightlyShaders shaders from the source tree on the GPU (surfaceless EGL)
    and checks properties that must hold for any version of them.

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <epoxy/egl.h>
#include <epoxy/gl.h>

#include <QFile>
#include <QTest>

#include <cmath>
#include <functional>
#include <vector>

// The shaders include files from KWin's resources, which aren't installed. These stand-ins pass
// colors through unchanged, the tests are about the shape and the outlines.
static const char *s_colorManagementStub =
    "vec4 sourceEncodingToNitsInDestinationColorspace(vec4 c) { return c; }\n"
    "vec4 nitsToDestinationEncoding(vec4 c) { return c; }\n"
    "vec3 doTonemapping(vec3 c) { return c; }\n";
static const char *s_saturationStub =
    "uniform float saturation;\n"
    "vec4 adjustSaturation(vec4 c) { return c; }\n";

// Same as KWin's base.vert with TRAIT_MAP_TEXTURE
static const char *s_windowVertexShader =
    "#version 140\n"
    "in vec4 position;\n"
    "in vec4 texcoord;\n"
    "out vec2 texcoord0;\n"
    "uniform mat4 modelViewProjectionMatrix;\n"
    "void main() { texcoord0 = texcoord.st; gl_Position = modelViewProjectionMatrix * position; }\n";

struct Color {
    float r, g, b, a;
};

// One rendered image, pixel (x, y) with y up like the texture coordinates the shaders see
struct Image {
    int width = 0;
    int height = 0;
    std::vector<float> data;

    Color at(int x, int y) const
    {
        const float *p = &data[(y * width + x) * 4];
        return {p[0], p[1], p[2], p[3]};
    }
};

static float maxDifference(const Color &a, const Color &b)
{
    return std::max({std::fabs(a.r - b.r), std::fabs(a.g - b.g), std::fabs(a.b - b.b), std::fabs(a.a - b.a)});
}

struct WindowParams {
    float frameWidth = 200;
    float frameHeight = 150;
    // Shadow around the frame, the texture covers frame and shadow
    float shadowLeft = 0;
    float shadowTop = 0;
    float shadowRight = 0;
    float shadowBottom = 0;
    float radius = 11;
    float shadowSampleOffset = 2;
    float innerOutlineWidth = 1;
    float outerOutlineWidth = 1;
    bool drawInnerOutline = false;
    bool drawOuterOutline = false;
    Color innerOutlineColor{1, 1, 1, 0.3f};
    Color outerOutlineColor{0, 0, 0, 0.3f};
    bool squircle = false;
    int squircleRatio = 5;

    int textureWidth() const
    {
        return int(frameWidth + shadowLeft + shadowRight);
    }
    int textureHeight() const
    {
        return int(frameHeight + shadowTop + shadowBottom);
    }
};

class ShaderTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();

    void windowCenterIsUnchanged();
    void windowCornersAreCut();
    void windowCornersMirrorEachOther();
    void translucentCornerEdgesStayPremultiplied();
    void wideOutlinesDontFillTheCorner();
    void innerOutlineWiderThanTheRadiusCoversTheCorner();
    void outerOutlineSitsOnTheEdge();
    void outlineHasNoGapAtHalfPixelRadius();
    void shadowPassesThrough();
    void sideWithoutShadowIsCut();

    void blurCircleMatchesKWin();
    void blurSquircleOnlyChangesCorners();

private:
    static QByteArray readSource(const QString &relativePath);
    static QByteArray preprocess(const QByteArray &source);
    GLuint link(const QByteArray &vertex, const QByteArray &fragment);
    Image render(GLuint program,
                 int width,
                 int height,
                 const std::vector<float> &quad,
                 const std::vector<unsigned char> &texture,
                 const std::function<void(GLuint)> &setUniforms);
    Image renderWindow(const WindowParams &params, const std::vector<unsigned char> &texture);
    Image renderBlur(GLuint program, float radius, float exponent);
    static std::vector<unsigned char> solidTexture(int width, int height, const Color &premultiplied);
    // An opaque frame surrounded by a faint shadow, laid out like the texture KWin hands the shader
    static std::vector<unsigned char> windowWithShadowTexture(const WindowParams &params);

    GLuint m_windowProgram = 0;
    GLuint m_blurProgram = 0;
    GLuint m_blurReferenceProgram = 0;
};

QByteArray ShaderTest::readSource(const QString &relativePath)
{
    QFile file(QStringLiteral(SOURCE_DIR "/") + relativePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qFatal("Can't read %s", qPrintable(file.fileName()));
    }
    return file.readAll();
}

// Mirrors what KWin does to effect shaders on a desktop GL context: its own #version line,
// trait defines and resolved includes
QByteArray ShaderTest::preprocess(const QByteArray &source)
{
    QByteArray result = "#version 140\n#define TRAIT_MAP_TEXTURE 1\n";
    for (const QByteArray &line : source.split('\n')) {
        if (line.startsWith("#version")) {
            continue;
        } else if (line.startsWith("#include \"colormanagement.glsl\"")) {
            result += s_colorManagementStub;
        } else if (line.startsWith("#include \"saturation.glsl\"")) {
            result += s_saturationStub;
        } else if (line.startsWith("#include")) {
            qFatal("Unexpected include: %s", line.constData());
        } else {
            result += line + '\n';
        }
    }
    return result;
}

static GLuint compile(GLenum type, const QByteArray &source)
{
    const GLuint shader = glCreateShader(type);
    const char *data = source.constData();
    glShaderSource(shader, 1, &data, nullptr);
    glCompileShader(shader);
    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[4096];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        qFatal("Shader compile error:\n%s", log);
    }
    return shader;
}

GLuint ShaderTest::link(const QByteArray &vertex, const QByteArray &fragment)
{
    const GLuint program = glCreateProgram();
    glAttachShader(program, compile(GL_VERTEX_SHADER, vertex));
    glAttachShader(program, compile(GL_FRAGMENT_SHADER, fragment));
    glBindAttribLocation(program, 0, "position");
    glBindAttribLocation(program, 1, "texcoord");
    glBindFragDataLocation(program, 0, "fragColor");
    glLinkProgram(program);
    GLint ok = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[4096];
        glGetProgramInfoLog(program, sizeof(log), nullptr, log);
        qFatal("Shader link error:\n%s", log);
    }
    return program;
}

void ShaderTest::initTestCase()
{
    auto getPlatformDisplay = reinterpret_cast<PFNEGLGETPLATFORMDISPLAYEXTPROC>(eglGetProcAddress("eglGetPlatformDisplayEXT"));
    EGLDisplay display = getPlatformDisplay ? getPlatformDisplay(EGL_PLATFORM_SURFACELESS_MESA, EGL_DEFAULT_DISPLAY, nullptr) : EGL_NO_DISPLAY;
    if (display == EGL_NO_DISPLAY || !eglInitialize(display, nullptr, nullptr) || !eglBindAPI(EGL_OPENGL_API)) {
        QSKIP("No surfaceless EGL display available");
    }
    const EGLint attributes[] =
        {EGL_CONTEXT_MAJOR_VERSION, 3, EGL_CONTEXT_MINOR_VERSION, 3, EGL_CONTEXT_OPENGL_PROFILE_MASK, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT, EGL_NONE};
    EGLContext context = eglCreateContext(display, EGL_NO_CONFIG_KHR, EGL_NO_CONTEXT, attributes);
    if (context == EGL_NO_CONTEXT || !eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, context)) {
        QSKIP("No OpenGL 3.3 core context available");
    }
    qInfo("Rendering on %s", glGetString(GL_RENDERER));

    m_windowProgram = link(s_windowVertexShader, preprocess(readSource(QStringLiteral("src/lightlyshaders/shaders/lightlyshaders.frag"))));

    const QByteArray blurVertex = preprocess(readSource(QStringLiteral("src/blur/shaders/onscreen_rounded.vert")));
    const QByteArray blurFragment = preprocess(readSource(QStringLiteral("src/blur/shaders/onscreen_rounded.frag")));
    m_blurProgram = link(blurVertex, blurFragment);

    // KWin's original measures the corner distance with length(), which is the p = 2 case
    const QByteArray lpDistance = "float corner_distance = pow(pow(outside.x, cornerExponent) + pow(outside.y, cornerExponent), 1.0 / cornerExponent);";
    QVERIFY2(blurFragment.contains(lpDistance), "The blur corner distance changed, update this test");
    QByteArray reference = blurFragment;
    reference.replace(lpDistance, "float corner_distance = length(outside);");
    m_blurReferenceProgram = link(blurVertex, reference);
}

std::vector<unsigned char> ShaderTest::solidTexture(int width, int height, const Color &premultiplied)
{
    std::vector<unsigned char> texture(width * height * 4);
    for (int i = 0; i < width * height; ++i) {
        texture[i * 4 + 0] = std::lround(premultiplied.r * 255);
        texture[i * 4 + 1] = std::lround(premultiplied.g * 255);
        texture[i * 4 + 2] = std::lround(premultiplied.b * 255);
        texture[i * 4 + 3] = std::lround(premultiplied.a * 255);
    }
    return texture;
}

// Two triangles covering the target, each vertex a position followed by texture coordinates
static std::vector<float> quad(float width, float height)
{
    return {0, 0, 0, 0, width, 0, 1, 0, width, height, 1, 1, 0, 0, 0, 0, width, height, 1, 1, 0, height, 0, 1};
}

std::vector<unsigned char> ShaderTest::windowWithShadowTexture(const WindowParams &params)
{
    const int width = params.textureWidth();
    const int height = params.textureHeight();
    std::vector<unsigned char> texture(width * height * 4);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            // Texture rows go up from the bottom, so the frame starts after the bottom shadow
            const bool inFrame = x >= params.shadowLeft && x < params.shadowLeft + params.frameWidth && y >= params.shadowBottom
                && y < params.shadowBottom + params.frameHeight;
            unsigned char *pixel = &texture[(y * width + x) * 4];
            if (inFrame) {
                pixel[0] = pixel[1] = pixel[2] = 200;
                pixel[3] = 255;
            } else {
                pixel[0] = pixel[1] = pixel[2] = 0;
                pixel[3] = 40;
            }
        }
    }
    return texture;
}

Image ShaderTest::render(GLuint program,
                         int width,
                         int height,
                         const std::vector<float> &vertices,
                         const std::vector<unsigned char> &texture,
                         const std::function<void(GLuint)> &setUniforms)
{
    GLuint vao, vbo, inputTexture, target, framebuffer;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 16, nullptr);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 16, reinterpret_cast<void *>(8));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glGenTextures(1, &inputTexture);
    glBindTexture(GL_TEXTURE_2D, inputTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenTextures(1, &target);
    glBindTexture(GL_TEXTURE_2D, target);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, target, 0);
    glViewport(0, 0, width, height);
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(program);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, inputTexture);
    setUniforms(program);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    Image image{width, height, std::vector<float>(width * height * 4)};
    glReadPixels(0, 0, width, height, GL_RGBA, GL_FLOAT, image.data.data());

    glDeleteFramebuffers(1, &framebuffer);
    glDeleteTextures(1, &target);
    glDeleteTextures(1, &inputTexture);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    return image;
}

Image ShaderTest::renderWindow(const WindowParams &params, const std::vector<unsigned char> &texture)
{
    const int width = params.textureWidth();
    const int height = params.textureHeight();
    return render(m_windowProgram, width, height, quad(width, height), texture, [&params, width, height](GLuint p) {
        const float projection[16] = {2.0f / width, 0, 0, 0, 0, 2.0f / height, 0, 0, 0, 0, 1, 0, -1, -1, 0, 1};
        glUniformMatrix4fv(glGetUniformLocation(p, "modelViewProjectionMatrix"), 1, GL_FALSE, projection);
        glUniform1i(glGetUniformLocation(p, "sampler"), 0);
        glUniform2f(glGetUniformLocation(p, "frame_size"), params.frameWidth, params.frameHeight);
        glUniform2f(glGetUniformLocation(p, "expanded_size"), params.textureWidth(), params.textureHeight());
        glUniform2f(glGetUniformLocation(p, "shadow_origin"), params.shadowLeft, params.shadowBottom);
        glUniform1f(glGetUniformLocation(p, "radius"), params.radius);
        glUniform1f(glGetUniformLocation(p, "shadow_sample_offset"), params.shadowSampleOffset);
        glUniform1f(glGetUniformLocation(p, "inner_outline_width"), params.innerOutlineWidth);
        glUniform1f(glGetUniformLocation(p, "outer_outline_width"), params.outerOutlineWidth);
        glUniform1i(glGetUniformLocation(p, "draw_inner_outline"), params.drawInnerOutline);
        glUniform1i(glGetUniformLocation(p, "draw_outer_outline"), params.drawOuterOutline);
        const Color &inner = params.innerOutlineColor;
        const Color &outer = params.outerOutlineColor;
        glUniform4f(glGetUniformLocation(p, "inner_outline_color"), inner.r, inner.g, inner.b, inner.a);
        glUniform4f(glGetUniformLocation(p, "outer_outline_color"), outer.r, outer.g, outer.b, outer.a);
        glUniform1i(glGetUniformLocation(p, "squircle_ratio"), params.squircleRatio);
        glUniform1i(glGetUniformLocation(p, "is_squircle"), params.squircle);
        glUniform4f(glGetUniformLocation(p, "modulation"), 1, 1, 1, 1);
    });
}

void ShaderTest::windowCenterIsUnchanged()
{
    const WindowParams params;
    const Color color{0.2f, 0.4f, 0.6f, 1};
    const Image image = renderWindow(params, solidTexture(params.textureWidth(), params.textureHeight(), color));
    QVERIFY(maxDifference(image.at(100, 75), color) < 0.01f);
}

void ShaderTest::windowCornersAreCut()
{
    WindowParams params;
    for (const bool squircle : {false, true}) {
        params.squircle = squircle;
        const Image image = renderWindow(params, solidTexture(params.textureWidth(), params.textureHeight(), {1, 1, 1, 1}));
        const int w = image.width - 1;
        const int h = image.height - 1;
        for (const auto &[x, y] : {std::pair{0, 0}, {w, 0}, {0, h}, {w, h}}) {
            QVERIFY2(image.at(x, y).a < 0.01f, "The outermost corner pixel must be cut away");
        }
        // Next to the corner along the edges, past the rounding, the window is opaque
        QVERIFY(image.at(int(params.radius) + 1, 0).a > 0.99f);
        QVERIFY(image.at(0, int(params.radius) + 1).a > 0.99f);
    }
}

void ShaderTest::windowCornersMirrorEachOther()
{
    WindowParams params;
    params.drawInnerOutline = true;
    params.drawOuterOutline = true;
    params.innerOutlineWidth = 2;
    params.outerOutlineWidth = 3;
    for (const bool shadow : {false, true}) {
        // The shadow is symmetric too, otherwise the corners can't be compared
        const float s = shadow ? 20 : 0;
        params.shadowLeft = params.shadowRight = params.shadowTop = params.shadowBottom = s;
        const Image image = renderWindow(params, solidTexture(params.textureWidth(), params.textureHeight(), {0.1f, 0.5f, 0.3f, 1}));
        float worst = 0;
        const int reach = int(s + params.radius) + 4;
        for (int y = 0; y < reach; ++y) {
            for (int x = 0; x < reach; ++x) {
                const Color bottomLeft = image.at(x, y);
                worst = std::max(worst, maxDifference(bottomLeft, image.at(image.width - 1 - x, y)));
                worst = std::max(worst, maxDifference(bottomLeft, image.at(x, image.height - 1 - y)));
                worst = std::max(worst, maxDifference(bottomLeft, image.at(image.width - 1 - x, image.height - 1 - y)));
            }
        }
        QVERIFY2(worst < 1e-4f, qPrintable(QStringLiteral("Corners differ by %1").arg(worst)));
    }
}

void ShaderTest::translucentCornerEdgesStayPremultiplied()
{
    // KWin's window textures are premultiplied, cutting the corner has to scale all channels alike
    const WindowParams params;
    const auto texture = solidTexture(params.textureWidth(), params.textureHeight(), {0.15f, 0.25f, 0.35f, 0.5f});
    const float ratioR = texture[0] / float(texture[3]);
    const float ratioB = texture[2] / float(texture[3]);
    const Image image = renderWindow(params, texture);

    int edgePixels = 0;
    for (int y = 0; y < int(params.radius); ++y) {
        for (int x = 0; x < int(params.radius); ++x) {
            const Color c = image.at(x, y);
            if (c.a > 0.02f && c.a < 0.48f) {
                ++edgePixels;
                QVERIFY(std::fabs(c.r / c.a - ratioR) < 0.02f);
                QVERIFY(std::fabs(c.b / c.a - ratioB) < 0.02f);
            }
        }
    }
    QVERIFY2(edgePixels > 0, "Expected antialiased pixels along the corner");
}

void ShaderTest::wideOutlinesDontFillTheCorner()
{
    // Outlines as wide as the corner radius. Regressions: the inner outline's circle got a negative
    // radius and drew a blob of outline color into the corner, and the straight outlines were only
    // drawn up to one corner radius away from the edge, so the inner one disappeared.
    WindowParams params;
    params.radius = 3;
    params.innerOutlineWidth = 3;
    params.outerOutlineWidth = 3;
    params.drawInnerOutline = true;
    params.drawOuterOutline = true;
    params.innerOutlineColor = {1, 0, 0, 1};
    params.outerOutlineColor = {0, 1, 0, 1};
    const Image image = renderWindow(params, solidTexture(params.textureWidth(), params.textureHeight(), {0, 0, 1, 1}));

    for (int y = 0; y < 3; ++y) {
        for (int x = 0; x < 3; ++x) {
            QVERIFY2(image.at(x, y).r < 0.05f, "Inner outline color inside the corner");
        }
    }
    // Without a shadow the outer outline is 0 to 3 pixels in from the edge, the inner one 3 to 6
    QVERIFY(maxDifference(image.at(1, 75), {0, 1, 0, 1}) < 0.01f);
    QVERIFY(maxDifference(image.at(4, 75), {1, 0, 0, 1}) < 0.01f);
    QVERIFY(maxDifference(image.at(100, 4), {1, 0, 0, 1}) < 0.01f);
    QVERIFY(maxDifference(image.at(8, 75), {0, 0, 1, 1}) < 0.01f);
}

void ShaderTest::innerOutlineWiderThanTheRadiusCoversTheCorner()
{
    // With a shadow the inner outline starts at the frame's edge. When it's wider than the corner
    // radius its inner circle has a negative radius, which must cover nothing; squaring it made
    // that circle cover the corner and erased the outline there.
    WindowParams params;
    params.shadowLeft = params.shadowRight = params.shadowTop = params.shadowBottom = 20;
    params.radius = 4;
    params.innerOutlineWidth = 8;
    params.drawInnerOutline = true;
    params.innerOutlineColor = {1, 0, 0, 1};
    const Image image = renderWindow(params, solidTexture(params.textureWidth(), params.textureHeight(), {0, 0, 1, 1}));

    // Next to the corner's center, well inside the outline band
    const Color nearCenter = image.at(20 + 3, 20 + 3);
    QVERIFY2(maxDifference(nearCenter, {1, 0, 0, 1}) < 0.01f, "The inner outline must cover the corner");
}

void ShaderTest::outerOutlineSitsOnTheEdge()
{
    WindowParams params;
    params.drawOuterOutline = true;
    params.outerOutlineWidth = 2;
    params.outerOutlineColor = {0, 1, 0, 1};
    const Color color{0.5f, 0.5f, 0.5f, 1};
    const Image image = renderWindow(params, solidTexture(params.textureWidth(), params.textureHeight(), color));

    // Without a shadow the outer outline is the outermost 2 pixels of the frame
    QVERIFY(maxDifference(image.at(0, 75), {0, 1, 0, 1}) < 0.01f);
    QVERIFY(maxDifference(image.at(1, 75), {0, 1, 0, 1}) < 0.01f);
    QVERIFY(maxDifference(image.at(3, 75), color) < 0.01f);
    QVERIFY(maxDifference(image.at(100, 0), {0, 1, 0, 1}) < 0.01f);
}

void ShaderTest::outlineHasNoGapAtHalfPixelRadius()
{
    // Fractional scales give radii like 7.5, the pixel row whose center is exactly at the radius
    // belongs neither to the corner nor to the straight edge unless the edge includes it
    WindowParams params;
    // Power of two sizes so the interpolated position lands exactly on the radius
    params.frameWidth = 256;
    params.frameHeight = 128;
    params.radius = 7.5f;
    params.drawOuterOutline = true;
    params.outerOutlineColor = {0, 1, 0, 1};
    const Image image = renderWindow(params, solidTexture(params.textureWidth(), params.textureHeight(), {0.5f, 0.5f, 0.5f, 1}));

    QVERIFY(maxDifference(image.at(0, 7), {0, 1, 0, 1}) < 0.01f);
    QVERIFY(maxDifference(image.at(7, 0), {0, 1, 0, 1}) < 0.01f);
}

void ShaderTest::shadowPassesThrough()
{
    WindowParams params;
    params.shadowLeft = 20;
    params.shadowRight = 20;
    params.shadowTop = 10;
    params.shadowBottom = 30;
    const Color color{0.3f, 0.3f, 0.3f, 1};
    const Image image = renderWindow(params, solidTexture(params.textureWidth(), params.textureHeight(), color));

    // Away from the frame's edges the shadow and the window are drawn as they are
    QVERIFY(maxDifference(image.at(5, 100), color) < 0.01f);
    QVERIFY(maxDifference(image.at(120, 5), color) < 0.01f);
    QVERIFY(maxDifference(image.at(120, 100), color) < 0.01f);
}

void ShaderTest::sideWithoutShadowIsCut()
{
    // Regression: shadows can be lopsided. With none at the top the rounded off area was filled
    // from outside the texture, which repeats the frame's edge, so the top corners stayed square.
    WindowParams params;
    params.shadowLeft = 20;
    params.shadowRight = 20;
    params.shadowBottom = 30;
    params.shadowTop = 0;
    params.drawOuterOutline = true;
    params.outerOutlineWidth = 2;
    params.outerOutlineColor = {0, 1, 0, 1};
    const Image image = renderWindow(params, windowWithShadowTexture(params));

    const int left = int(params.shadowLeft);
    const int right = int(params.shadowLeft + params.frameWidth) - 1;
    const int bottom = int(params.shadowBottom);
    const int top = int(params.shadowBottom + params.frameHeight) - 1;

    // The top corners are cut away, there is no shadow there to fill them with
    QVERIFY2(image.at(left, top).a < 0.05f, "Top left corner must be cut");
    QVERIFY2(image.at(right, top).a < 0.05f, "Top right corner must be cut");
    // The bottom corners are filled with the shadow from below, like before
    QVERIFY(std::fabs(image.at(left, bottom).a - 40 / 255.0f) < 0.02f);
    QVERIFY(std::fabs(image.at(right, bottom).a - 40 / 255.0f) < 0.02f);

    // Without a shadow above, the outer outline is drawn inside the top edge, with one below it
    // is drawn on the shadow just outside the bottom edge
    QVERIFY(maxDifference(image.at(left + 100, top), {0, 1, 0, 1}) < 0.01f);
    QVERIFY(maxDifference(image.at(left + 100, bottom - 1), {0, 1, 0, 1}) < 0.01f);
}

Image ShaderTest::renderBlur(GLuint program, float radius, float exponent)
{
    const int width = 320;
    const int height = 240;
    std::vector<unsigned char> texture(width * height * 4);
    for (int i = 0; i < width * height; ++i) {
        texture[i * 4 + 0] = (i * 7) & 0xff;
        texture[i * 4 + 1] = (i * 13) & 0xff;
        texture[i * 4 + 2] = (i * 29) & 0xff;
        texture[i * 4 + 3] = 255;
    }
    // The blur passes its vertices in pixels, which the corner distance relies on
    return render(program, width, height, quad(width, height), texture, [=](GLuint p) {
        const float projection[16] = {2.0f / width, 0, 0, 0, 0, 2.0f / height, 0, 0, 0, 0, 1, 0, -1, -1, 0, 1};
        const float identity[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
        glUniformMatrix4fv(glGetUniformLocation(p, "modelViewProjectionMatrix"), 1, GL_FALSE, projection);
        glUniformMatrix4fv(glGetUniformLocation(p, "colorMatrix"), 1, GL_FALSE, identity);
        glUniform1i(glGetUniformLocation(p, "texUnit"), 0);
        glUniform1f(glGetUniformLocation(p, "offset"), 3);
        glUniform2f(glGetUniformLocation(p, "halfpixel"), 0.5f / width, 0.5f / height);
        glUniform1f(glGetUniformLocation(p, "opacity"), 1);
        glUniform4f(glGetUniformLocation(p, "box"), width / 2.0f, height / 2.0f, width / 2.0f, height / 2.0f);
        glUniform4f(glGetUniformLocation(p, "cornerRadius"), radius, radius, radius, radius);
        glUniform1f(glGetUniformLocation(p, "cornerExponent"), exponent);
    });
}

void ShaderTest::blurCircleMatchesKWin()
{
    for (const float radius : {0.0f, 3.0f, 11.0f, 40.0f}) {
        const Image ours = renderBlur(m_blurProgram, radius, 2);
        const Image reference = renderBlur(m_blurReferenceProgram, radius, 2);
        float worst = 0;
        for (int y = 0; y < ours.height; ++y) {
            for (int x = 0; x < ours.width; ++x) {
                worst = std::max(worst, maxDifference(ours.at(x, y), reference.at(x, y)));
            }
        }
        QVERIFY2(worst < 1e-3f, qPrintable(QStringLiteral("radius %1 differs by %2").arg(radius).arg(worst)));
    }
}

void ShaderTest::blurSquircleOnlyChangesCorners()
{
    const float radius = 30;
    const Image circle = renderBlur(m_blurProgram, radius, 2);
    const Image squircle = renderBlur(m_blurProgram, radius, 5);
    int changedInCorners = 0;
    for (int y = 0; y < circle.height; ++y) {
        for (int x = 0; x < circle.width; ++x) {
            const bool inCorner = (x < radius || x >= circle.width - radius) && (y < radius || y >= circle.height - radius);
            const float difference = maxDifference(circle.at(x, y), squircle.at(x, y));
            if (!inCorner) {
                QVERIFY2(difference < 1e-3f, "A squircle must only change the corners");
            } else if (difference > 1e-3f) {
                ++changedInCorners;
            }
        }
    }
    QVERIFY2(changedInCorners > 0, "A squircle must change the corners");
}

QTEST_GUILESS_MAIN(ShaderTest)

#include "shadertest.moc"
