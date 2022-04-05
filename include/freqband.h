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

/**
 * Shows frequency bands
 */
class FrequencyBand{
public:
    FrequencyBand();
    virtual ~FrequencyBand();
    void setGlArea(Gtk::GLArea * area);
    void setSampleData(std::shared_ptr<SampleData> sdata);
protected:
    Gtk::GLArea *area{nullptr};
    GLuint m_Buffer {0};
    GLuint m_Program {0};
    GLuint m_Vao {0};
    GLuint m_Mvp {0};
    GLuint m_TexturePresent {0};
    GLuint m_Texture{0};
    unsigned int *data{nullptr};
    void realize();
    void unrealize();
    bool render(const Glib::RefPtr<Gdk::GLContext>& /* context */);
    void prepare_texture();
    void resize(const int w,const int h);
    void draw_rectangle();
    void init_buffers();
    void init_shaders(const std::string& vertex_path, const std::string& fragment_path);
    std::shared_ptr<SampleData> sample{nullptr};
};
#endif //FREQBAND_H
