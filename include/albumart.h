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

/// Shows album art.
/// Includes image of album art and shader effect.
class AlbumArt{
public:
    ///default constructor
    AlbumArt()=default;

    ///default virtual destructor
    virtual ~AlbumArt()=default;

    ///set the position of rendering of widget
    void setGlArea(Gtk::GLArea * area);

    ///pushes data inside widget
    void load_image(const char * data,int width,int height);
protected:
    ///target OpenGL area where graphics is rendered
    Gtk::GLArea *area{nullptr};
    int width;
    int height;

    ///vertex array buffer
    GLuint m_Buffer {0};

    ///shader programs
    GLuint m_Program {0};

    ///vertex array
    GLuint m_Vao {0};

    ///model-view-projection matrix
    GLuint m_Mvp {0};

    ///is texture present
    GLuint m_TexturePresent {0};

    ///texture to draw
    GLuint m_Texture{0};

    ///opengl start
    void realize();

    ///opengl stop
    void unrealize();

    ///opengl render
    bool render(const Glib::RefPtr<Gdk::GLContext>& /* context */);

    ///render subrutine
    void draw_rectangle();

    ///initializes buffers
    void init_buffers();

    ///initializes shaders
    void init_shaders(const std::string& vertex_path, const std::string& fragment_path);

    //on resize handle event
    void resize(const int w,const int h);

};

#endif //ALBUMART_H
