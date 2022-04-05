#ifndef SETTINGS_H
#define SETTINGS_H

#include <gtkmm.h>
#include <string>

/**
 * Settings dialog
 */
class Settings : public Gtk::Dialog{
public:
    Settings(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& refGlade);
    virtual ~Settings()=default;
    void onClose();
    void onOK();
protected:
    Glib::RefPtr<Gtk::Builder> builder;
    Gtk::Entry *tMetafile{0};

};

#endif //SETTINGS_H
