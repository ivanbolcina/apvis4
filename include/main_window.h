#ifndef MAINWINDOW_H_
#define MAINWINDOW_H_

#include <gtkmm.h>
#include <string>
#include "albumart.h"
#include "freqband.h"

class VU;
class AlbumArt;
class FrequencyBand;

///Displays main window
class MainWindow : public Gtk::Window
{
public:
	MainWindow(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& refGlade);
	virtual ~MainWindow();
    void onClose();
    bool update(int id);
    void onSettings();
private:
    void update_image();
    int image_id{0};
    Glib::RefPtr<Gtk::Builder> builder;
	Gtk::Button *buttonClose;
    Gtk::Label *lSongTitle;
    Gtk::Label *lArtist;
    Gtk::Label *lAlbum;
    Gtk::GLArea *glAlbum;
    Gtk::GLArea *glSpectre;
    Gtk::ProgressBar *pbChannel[2];
    std::shared_ptr<VU> vu;
    Gtk::ProgressBar *pbProgress;
    Gtk::Stack *sStack;
    AlbumArt albumart;
    FrequencyBand frequencyBand;
};

#endif // MAINWINDOW_H_
