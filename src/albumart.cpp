#include <iostream>
#include <plog/Log.h>
#include "albumart.h"
#include "utils.h"

using  namespace std;

static const GLfloat vertex_data[] = {
        +1, -1, -1,1,1,1,0,0,
        -1, -1, -1,1,0,1,0,0,
        -1, +1, -1,1,0,0,0,0,
        +1, +1, -1,1, 1,0,0,0
};

void AlbumArt::setGlArea(Gtk::GLArea *area) {
    this->area=area;
    area->signal_realize().connect(sigc::mem_fun(*this, &AlbumArt::realize));
    area->signal_unrealize().connect(sigc::mem_fun(*this, &AlbumArt::unrealize), false);
    area->signal_render().connect(sigc::mem_fun(*this, &AlbumArt::render), false);
}

void AlbumArt::realize()
{
    area->make_current();
    try
    {
        area->throw_if_error();
        init_buffers();
        //const bool use_es = area->get_context()->get_use_es();
        const std::string vertex_path =  "/shaders/aavertex.glsl" ;
        const std::string fragment_path = "/shaders/aafragment.glsl" ;
        init_shaders(vertex_path, fragment_path);
    }
    catch(const Gdk::GLError& gle)
    {
        PLOG_ERROR << "An error occured making the context current during realize:"  << to_string(gle.domain()) << "-" << to_string(gle.code()) << "-" << gle.what().c_str();
    }
}

void AlbumArt::unrealize()
{
    area->make_current();
    try
    {
        area->throw_if_error();
        if (m_Texture){
            glBindTexture(GL_TEXTURE_2D, 0);
            glDeleteTextures(1,&m_Texture);
            m_Texture = 0;
        }
        glDeleteBuffers(1, &m_Buffer);
        glDeleteProgram(m_Program);
    }
    catch(const Gdk::GLError& gle)
    {
        PLOG_ERROR << "An error occured making the context current during unrealize:" <<   to_string(gle.domain()) << "-" << to_string(gle.code()) << "-" << gle.what().c_str();
    }
}

bool AlbumArt::render(const Glib::RefPtr<Gdk::GLContext>& /* context */)
{
    try
    {
        area->throw_if_error();
        glClearColor(0.2, 0.2, 0.2, 1.0);
        glClear(GL_COLOR_BUFFER_BIT| GL_DEPTH_BUFFER_BIT);
        draw_rectangle();
        glFlush();
        return true;
    }
    catch(const Gdk::GLError& gle)
    {
        PLOG_ERROR << "An error occurred in the render callback of the GLArea:"  << to_string(gle.domain()) << "-" << to_string(gle.code()) << "-" << gle.what().c_str();
        return false;
    }
}


void AlbumArt::init_buffers()
{
    glGenVertexArrays(1, &m_Vao);
    glBindVertexArray(m_Vao);
    glGenBuffers(1, &m_Buffer);
    glBindBuffer(GL_ARRAY_BUFFER, m_Buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_data), vertex_data, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

static GLuint create_shader(int type, const char *src)
{
    auto shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    int status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if(status == GL_FALSE)
    {
        int log_len;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_len);
        string log_space(log_len+1, ' ');
        glGetShaderInfoLog(shader, log_len, nullptr, (GLchar*)log_space.c_str());
        PLOG_ERROR << "Compile failure in " <<
             (type == GL_VERTEX_SHADER ? "vertex" : "fragment") <<
             " shader: " << log_space;
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

void AlbumArt::init_shaders(const std::string& vertex_path, const std::string& fragment_path)
{
    auto vshader_bytes = Gio::Resource::lookup_data_global(vertex_path);
    if(!vshader_bytes)
    {
        PLOG_ERROR << "Failed fetching vertex shader resource";
        m_Program = 0;
        return;
    }
    gsize vshader_size {vshader_bytes->get_size()};
    auto vertex = create_shader(GL_VERTEX_SHADER,
                                (const char*)vshader_bytes->get_data(vshader_size));
    if(vertex == 0)
    {
        m_Program = 0;
        return;
    }
    auto fshader_bytes = Gio::Resource::lookup_data_global(fragment_path);
    if(!fshader_bytes)
    {
        PLOG_ERROR << "Failed fetching fragment shader resource";
        glDeleteShader(vertex);
        m_Program = 0;
        return;
    }
    gsize fshader_size {fshader_bytes->get_size()};
    auto fragment = create_shader(GL_FRAGMENT_SHADER,
                                  (const char*)fshader_bytes->get_data(fshader_size));
    if(fragment == 0)
    {
        glDeleteShader(vertex);
        m_Program = 0;
        return;
    }
    m_Program = glCreateProgram();
    glAttachShader(m_Program, vertex);
    glAttachShader(m_Program, fragment);
    glBindAttribLocation(m_Program,0,"vertex");
    glBindAttribLocation(m_Program,1,"texCoord");
    glLinkProgram(m_Program);
    int status;
    glGetProgramiv(m_Program, GL_LINK_STATUS, &status);
    if(status == GL_FALSE)
    {
        int log_len;
        glGetProgramiv(m_Program, GL_INFO_LOG_LENGTH, &log_len);
        string log_space(log_len+1, ' ');
        glGetProgramInfoLog(m_Program, log_len, nullptr, (GLchar*)log_space.c_str());
        PLOG_ERROR << "Linking failure: " << log_space;
        glDeleteProgram(m_Program);
        m_Program = 0;
    }
    else
    {
        m_Mvp = glGetUniformLocation(m_Program, "mvp");
        m_TexturePresent = glGetUniformLocation(m_Program, "texturePresent");
        glDetachShader(m_Program, vertex);
        glDetachShader(m_Program, fragment);
    }
    glDeleteShader(vertex);
    glDeleteShader(fragment);
}

void AlbumArt::draw_rectangle()
{
    float mvp[16];

    compute_mvp(mvp,
                0,
                0,
                0);
    glUseProgram(m_Program);
    glUniformMatrix4fv(m_Mvp, 1, GL_FALSE, &mvp[0]);
    glUniform1i(m_TexturePresent,(m_Texture!=0?1:0));
    int width = area->get_width();
    int height = area->get_height();
    int side = min(width, height);
    glViewport((width - side) , (height - side) , 2*side, 2*side);
    glBindBuffer(GL_ARRAY_BUFFER, m_Buffer);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE,8*sizeof(GL_FLOAT), nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 8*sizeof(GL_FLOAT), (void*)(4*sizeof(GL_FLOAT)));
    glEnableVertexAttribArray(1);
    glBindTexture(GL_TEXTURE_2D, m_Texture);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glUseProgram(0);
}

void AlbumArt::load_image(const char * data,int width,int height){
    if (m_Texture){
        glBindTexture(GL_TEXTURE_2D, 0);
        glDeleteTextures(1,&m_Texture);
        m_Texture = 0;
    }
    if (data== nullptr) return;
    glGenTextures(1, &m_Texture);
    glBindTexture(GL_TEXTURE_2D, m_Texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
}