#include <iostream>
#include <cmath>
#include <complex>
#include <iterator>
#include "freqband.h"
#include "vumeter.h"
#include "utils.h"

using namespace std;
const double PI = 3.1415926536;
#define TEXSIZE (1024)

unsigned int bitReverse(unsigned int x, int log2n) {
    int n = 0;
    for (int i = 0; i < log2n; i++) {
        n <<= 1;
        n |= (x & 1);
        x >>= 1;
    }
    return n;
}

template<class Iter_T>
void fft(Iter_T a, Iter_T b, int log2n) {
    typedef typename iterator_traits<Iter_T>::value_type complex;
    const complex J(0, 1);
    int n = 1 << log2n;
    for (unsigned int i = 0; i < n; ++i) {
        b[bitReverse(i, log2n)] = a[i];
    }
    for (int s = 1; s <= log2n; ++s) {
        int m = 1 << s;
        int m2 = m >> 1;
        complex w(1, 0);
        complex wm = exp(-J * (PI / m2));
        for (int j = 0; j < m2; ++j) {
            for (int k = j; k < n; k += m) {
                complex t = w * b[k + m2];
                complex u = b[k];
                b[k] = u + t;
                b[k + m2] = u - t;
            }
            w *= wm;
        }
    }
}

void fft(std::shared_ptr<SampleData> sample) {
    int N = 4096;          // size of FFT and sample window
    //float Fs = 44100;        // sample rate = 44.1 kHz
    float data[N];           // input PCM data buffer
    typedef complex<double> cx;
    cx a[N];        // FFT complex buffer (interleaved real/imag)
    cx b[N];
    //get data
    auto volume=sample->volume_in_decibels;
    if (volume>0.0) volume>0;
    if (volume<-30.0) volume=-30.0;
    auto power=pow(10,(volume/30.0));
    for (int i = 0; i < N; i++) {
        data[i] = sample->data[i];
        data[i]/=power;
    }
    //window function
    for (int i = 0; i < N; i++) {
        float procentage = 1.0f * i / N;
        float w = sin(procentage * PI);
        data[i] *= w * w;
    }
    // copy real input data to complex FFT buffer
    for (int i = 0; i < N; i++) {
        a[i].imag(0);
        a[i].real(((data[i])));
    }
    fft(a, b, 12); //10=log1024
    // calculate power spectrum (magnitude) values from fft[]
    int cnt = N / 2 / TEXSIZE;
    int cnti = 0;
    while (sample->freq.size() < TEXSIZE) sample->freq.push_back(0.f);
    // sample->freq.push_back(0);
    float w = 0;
    int post = 0;
    for (int i = 0; i < N / 2; i++) {
        float proc = 1.f * i / N / 2;
        //cut 5 procent
        proc *= ((1.0 - 0.1));
        proc += 0.05f;
        //do log
        float pos = (powf(10, proc) - 1) / (10 - 1);
        //get position in vals
        pos *= N / 2;
        float re = b[(int) pos].real();
        float im = b[(int) pos].imag();
        float amp = sqrt(re * re + im * im) / 1.44;
        if (isnan(amp)) amp = 0.f;
        if (amp > 1.f) amp = 1.0f;
        if (amp < -0.f) amp = 0.f;
        w += amp;
        if (++cnti >= cnt) {
            float aver = w / cnt;
            if (aver > 1.f) aver = 1.0f;
            if (aver < 0.f) aver = 0.f;
            auto curr = sample->freq[post];
            float add = curr * 0.666;
            if (add < 0.000001) add = 0.f;
            auto grad = aver * 0.333f;
            //  if (post==0) cout <<" add << " <<aver << " " << grad << add << endl;
            grad += add;
            if (grad > 1.f) grad = 1.0f;
            if (grad < 0.f) grad = 0.f;
            sample->freq[post] = grad;
            w = 0.f;
            cnti = 0;
            post++;
        }
    }
}

static const GLfloat vertex_data[] = {
        +1, -1, -1, 1, 1, 0, 0, 0,
        -1, -1, -1, 1, 0, 0, 0, 0,
        -1, +1, -1, 1, 0, 1, 0, 0,
        +1, +1, -1, 1, 1, 1, 0, 0
};

FrequencyBand::FrequencyBand() {
    data = new unsigned int[TEXSIZE * TEXSIZE];
}

FrequencyBand::~FrequencyBand() {
    delete data;
}

void FrequencyBand::setGlArea(Gtk::GLArea *area) {
    this->area = area;
    area->signal_resize().connect(sigc::mem_fun(*this,&FrequencyBand::resize));
    area->signal_realize().connect(sigc::mem_fun(*this, &FrequencyBand::realize));
    area->signal_unrealize().connect(sigc::mem_fun(*this, &FrequencyBand::unrealize), false);
    area->signal_render().connect(sigc::mem_fun(*this, &FrequencyBand::render), false);
    width = area->get_width();
    height = area->get_height();
}

void FrequencyBand::resize(const int w,const int h){
  //  area->make_current();
   // unrealize();
   // realize();
    width = area->get_width();
    height = area->get_height();
    cout<<"resized"<<endl;
}

void FrequencyBand::realize() {
    area->make_current();
    try {
        area->throw_if_error();
        init_buffers();
        //const bool use_es = area->get_context()->get_use_es();
        const std::string vertex_path = "/shaders/aavertex.glsl";
        const std::string fragment_path = "/shaders/plainfragment.glsl";
        init_shaders(vertex_path, fragment_path);

        glGenTextures(1, &m_Texture);
        glBindTexture(GL_TEXTURE_2D, m_Texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, TEXSIZE, TEXSIZE, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

    }
    catch (const Gdk::GLError &gle) {
        cerr << "An error occured making the context current during realize:" << endl;
        cerr << gle.domain() << "-" << gle.code() << "-" << gle.what() << endl;
    }
}

void FrequencyBand::unrealize() {
    area->make_current();
    try {
        area->throw_if_error();
        if (m_Texture) {
            glBindTexture(GL_TEXTURE_2D, 0);
            glDeleteTextures(1, &m_Texture);
            m_Texture = 0;
        }
        // Delete buffers and program
        if (m_Buffer) {
            glDeleteBuffers(1, &m_Buffer);
            m_Buffer = 0;
        }
        if (m_Program) {
            glDeleteProgram(m_Program);
            m_Program=0;
        }
    }
    catch (const Gdk::GLError &gle) {
        cerr << "An error occured making the context current during unrealize" << endl;
        cerr << gle.domain() << "-" << gle.code() << "-" << gle.what() << endl;
    }
}

bool FrequencyBand::render(const Glib::RefPtr<Gdk::GLContext> & /* context */) {
    try {
        area->throw_if_error();
        glClearColor(0.2, 0.2, 0.2, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        prepare_texture();
        draw_rectangle();
        glFlush();
        return true;
    }
    catch (const Gdk::GLError &gle) {
        cerr << "An error occurred in the render callback of the GLArea" << endl;
        cerr << gle.domain() << "-" << gle.code() << "-" << gle.what() << endl;
        return false;
    }
}


void FrequencyBand::init_buffers() {
    glGenVertexArrays(1, &m_Vao);
    glBindVertexArray(m_Vao);
    glGenBuffers(1, &m_Buffer);
    glBindBuffer(GL_ARRAY_BUFFER, m_Buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_data), vertex_data, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

static GLuint create_shader(int type, const char *src) {
    auto shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    int status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {
        int log_len;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_len);
        string log_space(log_len + 1, ' ');
        glGetShaderInfoLog(shader, log_len, nullptr, (GLchar *) log_space.c_str());
        cerr << "Compile failure in " <<
             (type == GL_VERTEX_SHADER ? "vertex" : "fragment") <<
             " shader: " << log_space << endl;
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

void FrequencyBand::init_shaders(const std::string &vertex_path, const std::string &fragment_path) {
    auto vshader_bytes = Gio::Resource::lookup_data_global(vertex_path);
    if (!vshader_bytes) {
        cerr << "Failed fetching vertex shader resource" << endl;
        m_Program = 0;
        return;
    }
    gsize vshader_size{vshader_bytes->get_size()};
    auto vertex = create_shader(GL_VERTEX_SHADER,
                                (const char *) vshader_bytes->get_data(vshader_size));
    if (vertex == 0) {
        m_Program = 0;
        return;
    }
    auto fshader_bytes = Gio::Resource::lookup_data_global(fragment_path);
    if (!fshader_bytes) {
        cerr << "Failed fetching fragment shader resource" << endl;
        glDeleteShader(vertex);
        m_Program = 0;
        return;
    }
    gsize fshader_size{fshader_bytes->get_size()};
    auto fragment = create_shader(GL_FRAGMENT_SHADER,
                                  (const char *) fshader_bytes->get_data(fshader_size));
    if (fragment == 0) {
        glDeleteShader(vertex);
        m_Program = 0;
        return;
    }
    m_Program = glCreateProgram();
    glAttachShader(m_Program, vertex);
    glAttachShader(m_Program, fragment);
    glBindAttribLocation(m_Program, 0, "vertex");
    glBindAttribLocation(m_Program, 1, "texCoord");
    glLinkProgram(m_Program);
    int status;
    glGetProgramiv(m_Program, GL_LINK_STATUS, &status);
    if (status == GL_FALSE) {
        int log_len;
        glGetProgramiv(m_Program, GL_INFO_LOG_LENGTH, &log_len);
        string log_space(log_len + 1, ' ');
        glGetProgramInfoLog(m_Program, log_len, nullptr, (GLchar *) log_space.c_str());
        cerr << "Linking failure: " << log_space << endl;
        glDeleteProgram(m_Program);
        m_Program = 0;
    } else {
        m_Mvp = glGetUniformLocation(m_Program, "mvp");
        m_TexturePresent = glGetUniformLocation(m_Program, "texturePresent");
        glDetachShader(m_Program, vertex);
        glDetachShader(m_Program, fragment);
    }
    glDeleteShader(vertex);
    glDeleteShader(fragment);
}

void FrequencyBand::prepare_texture() {
    for (int i = 0; i < TEXSIZE * TEXSIZE; i++) {
        data[i] = (0 << 24) + (51 << 16) + (51 << 8) + 51;
    }
    if (sample != nullptr && sample->lenght != 0) {
        fft(sample);
        for (int i = 0; i < TEXSIZE; i++) {
            int cnt = i;
            int y = 0;
            if (cnt < sample->freq.size()) y = (TEXSIZE - 1) * fabs(sample->freq[cnt]);
            for (int j = 0; j < y; j++) {
                int pos = j * TEXSIZE + i;
                data[pos] = (0 << 24) + (180 << 16) + (180 << 8) + 180;
            }
        }
    }
    glBindTexture(GL_TEXTURE_2D, m_Texture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, TEXSIZE, TEXSIZE, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
}

void FrequencyBand::draw_rectangle() {
    float mvp[16];

    compute_mvp(mvp,
                0,
                0,
                0);
    glUseProgram(m_Program);
    glUniformMatrix4fv(m_Mvp, 1, GL_FALSE, &mvp[0]);
    glUniform1i(m_TexturePresent, (m_Texture != 0 ? 1 : 0));
    //int width = area->get_width();
    //int height = area->get_height();
    int side = min(width, height);
    glViewport((width - side), (height - side), 2 * side, 2 * side);
    glBindBuffer(GL_ARRAY_BUFFER, m_Buffer);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(GL_FLOAT), nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(GL_FLOAT), (void *) (4 * sizeof(GL_FLOAT)));
    glEnableVertexAttribArray(1);
    glBindTexture(GL_TEXTURE_2D, m_Texture);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glUseProgram(0);
}

void FrequencyBand::setSampleData(std::shared_ptr<SampleData> sdata) {
    sample = sdata;
}
