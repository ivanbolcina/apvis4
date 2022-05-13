#ifndef FREQBAND_H
#define FREQBAND_H

#include <gtkmm.h>
#include <giomm/resource.h>
//#include <epoxy/gl.h>
//#include <epoxy/glx.h>
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glext.h>
#include "vumeter.h"

///Shows frequency bars
class FrequencyBand{
public:

    ///Constructor
    FrequencyBand();

    ///Destructor
    virtual ~FrequencyBand();

    ///Links to GTK OpenGL area
    void setGlArea(Gtk::GLArea * area);

    ///Sets data to be shown
    void setSampleData(std::shared_ptr<SampleData> sdata);
protected:

    ///target OpenGL area where graphics is rendered
    Gtk::GLArea *area{nullptr};

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

    ///pointer to texture data
    unsigned int *data{nullptr};

    ///opengl start
    void realize();

    ///opengl stop
    void unrealize();

    ///opengl render
    bool render(const Glib::RefPtr<Gdk::GLContext>& /* context */);

    ///prepares texture
    void prepare_texture();

    //on resize handle event
    void resize(const int w,const int h);

    ///draws lines
    void draw_rectangle();

    ///init buffers for OpenGL objects
    void init_buffers();

    ///init shader programs
    void init_shaders(const std::string& vertex_path, const std::string& fragment_path);

    ///raw sound data
    std::shared_ptr<SampleData> sample{nullptr};
};
#endif //FREQBAND_H
