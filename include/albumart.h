#ifndef ALBUMART_H
#define ALBUMART_H

#include <gtkmm.h>
#include <giomm/resource.h>
//#include <epoxy/gl.h>
//#include <epoxy/glx.h>
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glext.h>

/**
 * Shows album art
 */
class AlbumArt{
public:
    AlbumArt()=default;
    virtual ~AlbumArt()=default;
    void setGlArea(Gtk::GLArea * area);
    void load_image(const char * data,int width,int height);
protected:
    Gtk::GLArea *area{nullptr};
    GLuint m_Buffer {0};
    GLuint m_Program {0};
    GLuint m_Vao {0};
    GLuint m_Mvp {0};
    GLuint m_TexturePresent {0};
    GLuint m_Texture{0};
    void realize();
    void unrealize();
    bool render(const Glib::RefPtr<Gdk::GLContext>& /* context */);
    void draw_rectangle();
    void init_buffers();
    void init_shaders(const std::string& vertex_path, const std::string& fragment_path);
};

#endif //ALBUMART_H
