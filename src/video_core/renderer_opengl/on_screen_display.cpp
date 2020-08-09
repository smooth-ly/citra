#include "video_core/renderer_opengl/on_screen_display.h"

#include <memory>

#include "common/timer.h"
#include "core/core.h"
#include "core/frontend/framebuffer_layout.h"
#include "core/settings.h"
#include "video_core/renderer_opengl/gl_resource_manager.h"
#include "video_core/renderer_opengl/gl_state.h"
#include "video_core/renderer_opengl/gl_vars.h"

namespace OSD {

class RasterFont {
public:
    bool Initialize(float scale);
    void AddMessage(const std::string& message, MessageType type, u32 duration, u32 color);
    void Draw(const Layout::FramebufferLayout& layout);

private:
    void UpdateDebugInfo();
    void DrawText(std::string_view text, u32 color);

    GLint uniform_texture_id;
    GLint uniform_color_id;
    GLint uniform_offset_id;

    OpenGL::OGLVertexArray vertex_array;
    OpenGL::OGLBuffer vertex_buffer;
    OpenGL::OGLTexture texture;
    OpenGL::OGLProgram shader;

    struct Message {
        std::string text;
        MessageType type;
        u64 timestamp;
        u32 color;
    };
    std::vector<Message> messages;

    struct DrawInfo {
        float scaled_density;
        float font_width;
        float font_height;
        float border_x;
        float border_y;
        float shadow_x;
        float shadow_y;
        float screen_width;
        float screen_height;
        float start_x;
        float start_y;
    } draw_info;

    u64 debug_timestamp;
};

static const int CHARACTER_WIDTH = 8;
static const int CHARACTER_HEIGHT = 13;
static const int CHARACTER_OFFSET = 32;
static const int CHARACTER_COUNT = 95;

static const u8 rasters[CHARACTER_COUNT][CHARACTER_HEIGHT] = {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x18, 0x18, 0x00, 0x00, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x36, 0x36, 0x36, 0x36},
    {0x00, 0x00, 0x00, 0x66, 0x66, 0xff, 0x66, 0x66, 0xff, 0x66, 0x66, 0x00, 0x00},
    {0x00, 0x00, 0x18, 0x7e, 0xff, 0x1b, 0x1f, 0x7e, 0xf8, 0xd8, 0xff, 0x7e, 0x18},
    {0x00, 0x00, 0x0e, 0x1b, 0xdb, 0x6e, 0x30, 0x18, 0x0c, 0x76, 0xdb, 0xd8, 0x70},
    {0x00, 0x00, 0x7f, 0xc6, 0xcf, 0xd8, 0x70, 0x70, 0xd8, 0xcc, 0xcc, 0x6c, 0x38},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x1c, 0x0c, 0x0e},
    {0x00, 0x00, 0x0c, 0x18, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x18, 0x0c},
    {0x00, 0x00, 0x30, 0x18, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x18, 0x30},
    {0x00, 0x00, 0x00, 0x00, 0x99, 0x5a, 0x3c, 0xff, 0x3c, 0x5a, 0x99, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x18, 0x18, 0x18, 0xff, 0xff, 0x18, 0x18, 0x18, 0x00, 0x00},
    {0x00, 0x00, 0x30, 0x18, 0x1c, 0x1c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x38, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x60, 0x60, 0x30, 0x30, 0x18, 0x18, 0x0c, 0x0c, 0x06, 0x06, 0x03, 0x03},
    {0x00, 0x00, 0x3c, 0x66, 0xc3, 0xe3, 0xf3, 0xdb, 0xcf, 0xc7, 0xc3, 0x66, 0x3c},
    {0x00, 0x00, 0x7e, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x78, 0x38, 0x18},
    {0x00, 0x00, 0xff, 0xc0, 0xc0, 0x60, 0x30, 0x18, 0x0c, 0x06, 0x03, 0xe7, 0x7e},
    {0x00, 0x00, 0x7e, 0xe7, 0x03, 0x03, 0x07, 0x7e, 0x07, 0x03, 0x03, 0xe7, 0x7e},
    {0x00, 0x00, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0xff, 0xcc, 0x6c, 0x3c, 0x1c, 0x0c},
    {0x00, 0x00, 0x7e, 0xe7, 0x03, 0x03, 0x07, 0xfe, 0xc0, 0xc0, 0xc0, 0xc0, 0xff},
    {0x00, 0x00, 0x7e, 0xe7, 0xc3, 0xc3, 0xc7, 0xfe, 0xc0, 0xc0, 0xc0, 0xe7, 0x7e},
    {0x00, 0x00, 0x30, 0x30, 0x30, 0x30, 0x18, 0x0c, 0x06, 0x03, 0x03, 0x03, 0xff},
    {0x00, 0x00, 0x7e, 0xe7, 0xc3, 0xc3, 0xe7, 0x7e, 0xe7, 0xc3, 0xc3, 0xe7, 0x7e},
    {0x00, 0x00, 0x7e, 0xe7, 0x03, 0x03, 0x03, 0x7f, 0xe7, 0xc3, 0xc3, 0xe7, 0x7e},
    {0x00, 0x00, 0x00, 0x38, 0x38, 0x00, 0x00, 0x38, 0x38, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x30, 0x18, 0x1c, 0x1c, 0x00, 0x00, 0x1c, 0x1c, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x06, 0x0c, 0x18, 0x30, 0x60, 0xc0, 0x60, 0x30, 0x18, 0x0c, 0x06},
    {0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x60, 0x30, 0x18, 0x0c, 0x06, 0x03, 0x06, 0x0c, 0x18, 0x30, 0x60},
    {0x00, 0x00, 0x18, 0x00, 0x00, 0x18, 0x18, 0x0c, 0x06, 0x03, 0xc3, 0xc3, 0x7e},
    {0x00, 0x00, 0x3f, 0x60, 0xcf, 0xdb, 0xd3, 0xdd, 0xc3, 0x7e, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0xc3, 0xc3, 0xc3, 0xc3, 0xff, 0xc3, 0xc3, 0xc3, 0x66, 0x3c, 0x18},
    {0x00, 0x00, 0xfe, 0xc7, 0xc3, 0xc3, 0xc7, 0xfe, 0xc7, 0xc3, 0xc3, 0xc7, 0xfe},
    {0x00, 0x00, 0x7e, 0xe7, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xe7, 0x7e},
    {0x00, 0x00, 0xfc, 0xce, 0xc7, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc7, 0xce, 0xfc},
    {0x00, 0x00, 0xff, 0xc0, 0xc0, 0xc0, 0xc0, 0xfc, 0xc0, 0xc0, 0xc0, 0xc0, 0xff},
    {0x00, 0x00, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xfc, 0xc0, 0xc0, 0xc0, 0xff},
    {0x00, 0x00, 0x7e, 0xe7, 0xc3, 0xc3, 0xcf, 0xc0, 0xc0, 0xc0, 0xc0, 0xe7, 0x7e},
    {0x00, 0x00, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xff, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3},
    {0x00, 0x00, 0x7e, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x7e},
    {0x00, 0x00, 0x7c, 0xee, 0xc6, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06},
    {0x00, 0x00, 0xc3, 0xc6, 0xcc, 0xd8, 0xf0, 0xe0, 0xf0, 0xd8, 0xcc, 0xc6, 0xc3},
    {0x00, 0x00, 0xff, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0},
    {0x00, 0x00, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xdb, 0xff, 0xff, 0xe7, 0xc3},
    {0x00, 0x00, 0xc7, 0xc7, 0xcf, 0xcf, 0xdf, 0xdb, 0xfb, 0xf3, 0xf3, 0xe3, 0xe3},
    {0x00, 0x00, 0x7e, 0xe7, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xe7, 0x7e},
    {0x00, 0x00, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xfe, 0xc7, 0xc3, 0xc3, 0xc7, 0xfe},
    {0x00, 0x00, 0x3f, 0x6e, 0xdf, 0xdb, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0x66, 0x3c},
    {0x00, 0x00, 0xc3, 0xc6, 0xcc, 0xd8, 0xf0, 0xfe, 0xc7, 0xc3, 0xc3, 0xc7, 0xfe},
    {0x00, 0x00, 0x7e, 0xe7, 0x03, 0x03, 0x07, 0x7e, 0xe0, 0xc0, 0xc0, 0xe7, 0x7e},
    {0x00, 0x00, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0xff},
    {0x00, 0x00, 0x7e, 0xe7, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3},
    {0x00, 0x00, 0x18, 0x3c, 0x3c, 0x66, 0x66, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3},
    {0x00, 0x00, 0xc3, 0xe7, 0xff, 0xff, 0xdb, 0xdb, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3},
    {0x00, 0x00, 0xc3, 0x66, 0x66, 0x3c, 0x3c, 0x18, 0x3c, 0x3c, 0x66, 0x66, 0xc3},
    {0x00, 0x00, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3c, 0x3c, 0x66, 0x66, 0xc3},
    {0x00, 0x00, 0xff, 0xc0, 0xc0, 0x60, 0x30, 0x7e, 0x0c, 0x06, 0x03, 0x03, 0xff},
    {0x00, 0x00, 0x3c, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x3c},
    {0x00, 0x03, 0x03, 0x06, 0x06, 0x0c, 0x0c, 0x18, 0x18, 0x30, 0x30, 0x60, 0x60},
    {0x00, 0x00, 0x3c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x3c},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc3, 0x66, 0x3c, 0x18},
    {0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x38, 0x30, 0x70},
    {0x00, 0x00, 0x7f, 0xc3, 0xc3, 0x7f, 0x03, 0xc3, 0x7e, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0xfe, 0xc3, 0xc3, 0xc3, 0xc3, 0xfe, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0},
    {0x00, 0x00, 0x7e, 0xc3, 0xc0, 0xc0, 0xc0, 0xc3, 0x7e, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x7f, 0xc3, 0xc3, 0xc3, 0xc3, 0x7f, 0x03, 0x03, 0x03, 0x03, 0x03},
    {0x00, 0x00, 0x7f, 0xc0, 0xc0, 0xfe, 0xc3, 0xc3, 0x7e, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x30, 0x30, 0x30, 0x30, 0x30, 0xfc, 0x30, 0x30, 0x30, 0x33, 0x1e},
    {0x7e, 0xc3, 0x03, 0x03, 0x7f, 0xc3, 0xc3, 0xc3, 0x7e, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xfe, 0xc0, 0xc0, 0xc0, 0xc0},
    {0x00, 0x00, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00, 0x00, 0x18, 0x00},
    {0x38, 0x6c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x00, 0x00, 0x0c, 0x00},
    {0x00, 0x00, 0xc6, 0xcc, 0xf8, 0xf0, 0xd8, 0xcc, 0xc6, 0xc0, 0xc0, 0xc0, 0xc0},
    {0x00, 0x00, 0x7e, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x78},
    {0x00, 0x00, 0xdb, 0xdb, 0xdb, 0xdb, 0xdb, 0xdb, 0xfe, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0xc6, 0xc6, 0xc6, 0xc6, 0xc6, 0xc6, 0xfc, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x7c, 0xc6, 0xc6, 0xc6, 0xc6, 0xc6, 0x7c, 0x00, 0x00, 0x00, 0x00},
    {0xc0, 0xc0, 0xc0, 0xfe, 0xc3, 0xc3, 0xc3, 0xc3, 0xfe, 0x00, 0x00, 0x00, 0x00},
    {0x03, 0x03, 0x03, 0x7f, 0xc3, 0xc3, 0xc3, 0xc3, 0x7f, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xe0, 0xfe, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0xfe, 0x03, 0x03, 0x7e, 0xc0, 0xc0, 0x7f, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x1c, 0x36, 0x30, 0x30, 0x30, 0x30, 0xfc, 0x30, 0x30, 0x30, 0x00},
    {0x00, 0x00, 0x7e, 0xc6, 0xc6, 0xc6, 0xc6, 0xc6, 0xc6, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x18, 0x3c, 0x3c, 0x66, 0x66, 0xc3, 0xc3, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0xc3, 0xe7, 0xff, 0xdb, 0xc3, 0xc3, 0xc3, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0xc3, 0x66, 0x3c, 0x18, 0x3c, 0x66, 0xc3, 0x00, 0x00, 0x00, 0x00},
    {0xc0, 0x60, 0x60, 0x30, 0x18, 0x3c, 0x66, 0x66, 0xc3, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0xff, 0x60, 0x30, 0x18, 0x0c, 0x06, 0xff, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x0f, 0x18, 0x18, 0x18, 0x38, 0xf0, 0x38, 0x18, 0x18, 0x18, 0x0f},
    {0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18},
    {0x00, 0x00, 0xf0, 0x18, 0x18, 0x18, 0x1c, 0x0f, 0x1c, 0x18, 0x18, 0x18, 0xf0},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x8f, 0xf1, 0x60, 0x00, 0x00, 0x00}};

static const char* s_vertexShaderSrc = R"(uniform vec2 charSize;
uniform vec2 offset;
in vec2 rawpos;
in vec2 rawtex0;
out vec2 uv0;
void main() {
    gl_Position = vec4(rawpos + offset, 0, 1);
    uv0 = rawtex0 * charSize;
})";

static const char* s_fragmentShaderSrc = R"(uniform sampler2D samp0;
uniform vec4 color;
in vec2 uv0;
out vec4 ocol0;
void main() {
    ocol0 = texture(samp0, uv0) * color;
})";

bool RasterFont::Initialize(float scale) {
    debug_timestamp = 0;

    // A scaling factor for fonts displayed on the display.
    draw_info.scaled_density = scale;

    // Generate the texture
    texture.Create();

    // Generate VBO handle for drawing
    vertex_buffer.Create();

    // Generate VAO
    vertex_array.Create();

    // generate shader
    std::string frag_source;
    if (OpenGL::GLES) {
        frag_source = OpenGL::fragment_shader_precision_OES;
        frag_source += s_fragmentShaderSrc;
    } else {
        frag_source = s_fragmentShaderSrc;
    }
    shader.Create(s_vertexShaderSrc, frag_source.c_str());

    // apply
    OpenGL::OpenGLState state = OpenGL::OpenGLState::GetCurState();
    OpenGL::OpenGLState prev_state = state;
    state.draw.shader_program = shader.handle;
    state.draw.vertex_array = vertex_array.handle;
    state.draw.vertex_buffer = vertex_buffer.handle;
    state.texture_units[0].texture_2d = texture.handle;
    state.Apply();

    // init texture
    std::vector<u32> texture_data(CHARACTER_WIDTH * CHARACTER_COUNT * CHARACTER_HEIGHT);
    for (int y = 0; y < CHARACTER_HEIGHT; y++) {
        for (int c = 0; c < CHARACTER_COUNT; c++) {
            for (int x = 0; x < CHARACTER_WIDTH; x++) {
                bool pixel = (0 != (rasters[c][y] & (1 << (CHARACTER_WIDTH - x - 1))));
                texture_data[CHARACTER_WIDTH * CHARACTER_COUNT * y + CHARACTER_WIDTH * c + x] =
                    pixel ? -1 : 0;
            }
        }
    }
    glActiveTexture(GL_TEXTURE0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, CHARACTER_WIDTH * CHARACTER_COUNT, CHARACTER_HEIGHT, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, texture_data.data());

    // bound uniforms
    uniform_texture_id = glGetUniformLocation(shader.handle, "samp0");
    uniform_color_id = glGetUniformLocation(shader.handle, "color");
    uniform_offset_id = glGetUniformLocation(shader.handle, "offset");
    glUniform1i(uniform_texture_id, 0);
    glUniform2f(glGetUniformLocation(shader.handle, "charSize"), 1.0F / GLfloat(CHARACTER_COUNT),
                1.0F);

    // generate VBO & VAO
    GLuint attrib_position = glGetAttribLocation(shader.handle, "rawpos");
    GLuint attrib_tex_coord = glGetAttribLocation(shader.handle, "rawtex0");
    glEnableVertexAttribArray(attrib_position);
    glVertexAttribPointer(attrib_position, 2, GL_FLOAT, 0, sizeof(GLfloat) * 4, nullptr);
    glEnableVertexAttribArray(attrib_tex_coord);
    glVertexAttribPointer(attrib_tex_coord, 2, GL_FLOAT, 0, sizeof(GLfloat) * 4,
                          (GLfloat*)nullptr + 2);

    prev_state.Apply();

    return true;
}

void RasterFont::DrawText(std::string_view text, u32 color) {
    int usage = 0;
    GLfloat x = draw_info.start_x;
    GLfloat y = draw_info.start_y;
    std::vector<GLfloat> vertices(text.length() * 6 * 4);

    for (const char& c : text) {
        if (c == '\n') {
            x = draw_info.start_x;
            y -= draw_info.font_height + draw_info.border_y;
            continue;
        }

        // do not print spaces, they can be skipped easily
        if (c == ' ') {
            x += draw_info.font_width + draw_info.border_x;
            continue;
        }

        if (c < CHARACTER_OFFSET || c >= CHARACTER_COUNT + CHARACTER_OFFSET)
            continue;

        vertices[usage++] = x;
        vertices[usage++] = y;
        vertices[usage++] = GLfloat(c - CHARACTER_OFFSET);
        vertices[usage++] = 0.0f;

        vertices[usage++] = x + draw_info.font_width;
        vertices[usage++] = y;
        vertices[usage++] = GLfloat(c - CHARACTER_OFFSET + 1);
        vertices[usage++] = 0.0f;

        vertices[usage++] = x + draw_info.font_width;
        vertices[usage++] = y + draw_info.font_height;
        vertices[usage++] = GLfloat(c - CHARACTER_OFFSET + 1);
        vertices[usage++] = 1.0f;

        vertices[usage++] = x;
        vertices[usage++] = y;
        vertices[usage++] = GLfloat(c - CHARACTER_OFFSET);
        vertices[usage++] = 0.0f;

        vertices[usage++] = x + draw_info.font_width;
        vertices[usage++] = y + draw_info.font_height;
        vertices[usage++] = GLfloat(c - CHARACTER_OFFSET + 1);
        vertices[usage++] = 1.0f;

        vertices[usage++] = x;
        vertices[usage++] = y + draw_info.font_height;
        vertices[usage++] = GLfloat(c - CHARACTER_OFFSET);
        vertices[usage++] = 1.0f;

        x += draw_info.font_width + draw_info.border_x;
    }
    draw_info.start_y = y - draw_info.font_height - draw_info.border_y;

    if (!usage) {
        return;
    }

    GLsizeiptr need_size = usage * sizeof(GLfloat);
    // prefer `glBufferData` than `glBufferSubData` on mobile device
    glBufferData(GL_ARRAY_BUFFER, need_size, vertices.data(), GL_STREAM_DRAW);

    // text shadow
    glUniform2f(uniform_offset_id, draw_info.shadow_x, draw_info.shadow_y);
    glUniform4f(uniform_color_id, 0.0f, 0.0f, 0.0f, GLfloat((color >> 24) & 0xff) / 255.f);
    glDrawArrays(GL_TRIANGLES, 0, usage / 4);

    // text
    glUniform2f(uniform_offset_id, 0.0f, 0.0f);
    glUniform4f(uniform_color_id, GLfloat((color >> 16) & 0xff) / 255.f,
                GLfloat((color >> 8) & 0xff) / 255.f, GLfloat((color >> 0) & 0xff) / 255.f,
                GLfloat((color >> 24) & 0xff) / 255.f);
    glDrawArrays(GL_TRIANGLES, 0, usage / 4);
}

void RasterFont::AddMessage(const std::string& message, MessageType type, u32 duration, u32 color) {
    u64 now = static_cast<u64>(Common::Timer::GetTimeMs().count());
    if (type != MessageType::Typeless) {
        auto iter = messages.begin();
        while (iter != messages.end()) {
            if (iter->type == type) {
                iter->text = message;
                iter->timestamp = now + duration;
                iter->color = color;
                return;
            }
            ++iter;
        }
    }
    messages.emplace_back(Message{message, type, now + duration, color});
}

void RasterFont::UpdateDebugInfo() {
    Core::PerfStats::Results stats = Core::System::GetInstance().GetAndResetPerfStats();
    std::string text = fmt::format(
        "FPS:{:>2} - VPS:{:>2} - SPD:{:>2}", static_cast<int>(stats.game_fps),
        static_cast<int>(stats.system_fps), static_cast<int>(stats.emulation_speed * 100.0));

    AddMessage(text, MessageType::FPS, Duration::FOREVER, Color::BLUE);
}

void RasterFont::Draw(const Layout::FramebufferLayout& layout) {
    draw_info.screen_width = static_cast<float>(layout.width);
    draw_info.screen_height = static_cast<float>(layout.height);
    draw_info.font_width =
        draw_info.scaled_density * CHARACTER_WIDTH / draw_info.screen_width * 2.0f;
    draw_info.font_height =
        draw_info.scaled_density * CHARACTER_HEIGHT / draw_info.screen_height * 2.0f;
    draw_info.border_x = draw_info.font_width / 2.0f;
    draw_info.border_y = draw_info.font_height / 2.0f;
    draw_info.shadow_x = draw_info.font_width / 4.0f;
    draw_info.shadow_y = -draw_info.font_height / 6.0f;
    draw_info.start_x = draw_info.font_width - 1.0f;
    draw_info.start_y = 1.0f - draw_info.font_height * 1.1f;

    OpenGL::OpenGLState state = OpenGL::OpenGLState::GetCurState();
    state.draw.shader_program = shader.handle;
    state.draw.vertex_array = vertex_array.handle;
    state.draw.vertex_buffer = vertex_buffer.handle;
    state.texture_units[0].texture_2d = texture.handle;
    state.blend.enabled = true;
    state.blend.src_rgb_func = GL_SRC_COLOR;
    state.blend.src_a_func = GL_ZERO;
    state.blend.dst_rgb_func = GL_ONE_MINUS_SRC_ALPHA;
    state.blend.dst_a_func = GL_ONE;
    state.Apply();

    glActiveTexture(GL_TEXTURE0);

    u64 now = static_cast<u64>(Common::Timer::GetTimeMs().count());

    if (now - debug_timestamp > 500) {
        UpdateDebugInfo();
        debug_timestamp = now;
    }

    auto iter = messages.begin();
    while (iter != messages.end()) {
        if (iter->timestamp < now) {
            iter = messages.erase(iter);
        } else {
            DrawText(iter->text, iter->color);
            ++iter;
        }
    }
}

static std::unique_ptr<RasterFont> s_raster_font;

void Initialize() {
    s_raster_font = std::make_unique<RasterFont>();
    s_raster_font->Initialize(1.8f);
    // fps placeholder
    AddMessage("", MessageType::FPS, Duration::FOREVER, Color::BLUE);
}

void AddMessage(const std::string& message, MessageType type, u32 duration, u32 color) {
    if (Settings::values.show_fps) {
        s_raster_font->AddMessage(message, type, duration, color);
    }
}

void DrawMessage(const Layout::FramebufferLayout& layout) {
    if (Settings::values.show_fps) {
        s_raster_font->Draw(layout);
    }
}

void Shutdown() {
    s_raster_font.reset();
}

} // namespace OSD
