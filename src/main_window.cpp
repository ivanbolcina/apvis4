#include <iostream>

#include <giomm/resource.h>
//#include <epoxy/gl.h>
#include <plog/Log.h>

#include "main_window.h"
#include "appstate.h"
#include "readinfo.h"
#include "vumeter.h"
#include "albumart.h"
#include "utils.h"
#include "settings.h"

using namespace std;

MainWindow::MainWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &refGlade)
        : Gtk::Window(cobject), builder(refGlade) {
    //ui
    //this->set_border_width(10);
   // this->builder->get_widget("closeButton", this->buttonClose);
  //  this->buttonClose->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::onClose));
    this->builder->get_widget("lSongTitle", this->lSongTitle);
    this->builder->get_widget("lArtist", this->lArtist);
    this->builder->get_widget("lAlbum", this->lAlbum);
    this->builder->get_widget("pbLeft", this->pbChannel[0]);
    this->builder->get_widget("pbRight", this->pbChannel[1]);
    this->builder->get_widget("pbProgress",this->pbProgress);
    this->builder->get_widget("sStack",this->sStack);
    Gtk::ImageMenuItem * bQuit;
    Gtk::ImageMenuItem * bSettings;
    this->builder->get_widget("bQuit", bQuit);
    bQuit->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::onClose));
    this->builder->get_widget("bSettings", bSettings);
    bSettings->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::onSettings));
    sStack->set_visible_child("pWait");
    //reading
    initialise_decoding_table();
    MetaReaderInstance.startreading();
    vu = make_shared<VU>();
    vu->start();
    //update data
    sigc::slot<bool()> my_slot = sigc::bind(sigc::mem_fun(*this,
                                                          &MainWindow::update), 0);
    auto conn = Glib::signal_timeout().connect(my_slot, 10);
    //album art
    this->builder->get_widget("glAlbum", this->glAlbum);
    albumart.setGlArea(glAlbum);
    //frequency band
    this->builder->get_widget("glSpectre", this->glSpectre);
    frequencyBand.setGlArea(glSpectre);
    frequencyBand.setSampleData(vu->getSample());
    //CSS
    Glib::ustring data = "window {background: #333333;color:white;} .bar {-GtkProgressBar-min-horizontal-bar-height: 15px;color:white;} progressbar progress {color:white;background-color:gray;margin-bottom:13px;}";
    auto css = Gtk::CssProvider::create();
    if (not css->load_from_data(data)) {
        cerr << "Failed to load css\n";
        std::exit(1);
    }
    auto screen = Gdk::Screen::get_default();
    auto ctx = this->get_style_context();
    ctx->add_provider_for_screen(screen, css, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

}

MainWindow::~MainWindow() {
    vu->end();
    MetaReaderInstance.stopreading();
}

void MainWindow::onClose() {
    Window::close();
}

void MainWindow::update_image() {
    if (state.image_id == image_id) return;
    image_id = state.image_id;
    int size = state.image.size();
    albumart.load_image(nullptr, 0, 0);
    this->glAlbum->queue_render();
    if (size == 0) return;
    GError *error = nullptr;
    auto loader = gdk_pixbuf_loader_new();
    gdk_pixbuf_loader_write(loader, state.image.data(), size, &error);
    if (error != NULL) {
        PLOG_ERROR << error->message;
        g_clear_error(&error);
        return;
    }
    gdk_pixbuf_loader_close(loader, &error);
    if (error != NULL) {
        PLOG_ERROR << error->message;
        g_clear_error(&error);
        return;
    }
    auto pixbuf = gdk_pixbuf_loader_get_pixbuf(loader);
    error = nullptr;
    gdk_pixbuf_loader_close(loader, &error);
    if (error != NULL) {
        PLOG_ERROR << error->message;
        g_clear_error(&error);
        return;
    }
    guchar *data = gdk_pixbuf_get_pixels(pixbuf);
    int w = gdk_pixbuf_get_width(pixbuf);
    int h = gdk_pixbuf_get_height(pixbuf);
    albumart.load_image(reinterpret_cast<const char *>(data), w, h);
}

bool MainWindow::update(int id) {
    state.read();
    vu->tick();
    auto list = vu->get();
    auto cnt = 0;
    for (auto itm: list) {
        pbChannel[cnt++]->set_fraction(itm->fraction);
        if (cnt == 2) break;
    }
    vu->getSample()->volume_in_decibels=state.volume_in_decibels;
    lSongTitle->set_label(state.song_name);
    if (state.song_name!="" && sStack->get_visible_child_name()!="pDisplay"){
        sStack->set_visible_child("pDisplay");
    }
    lAlbum->set_label(state.album);
    lArtist->set_label(state.artist);
    state.update_progres();
    pbProgress->set_fraction(state.progress);
    update_image();
    this->glSpectre->queue_render();
    return true;
}

void MainWindow::onSettings() {
    Settings * settings;
    Glib::RefPtr<Gtk::Builder> builder = Gtk::Builder::create_from_resource("/glade/settings.glade");
    builder->get_widget_derived("Settings", settings);
    settings->show();
    MetaReaderInstance.configchanged();
}