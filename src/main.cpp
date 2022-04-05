#include <iostream>
#include <gtkmm.h>

#include "main_window.h"

using namespace std;

Glib::Dispatcher dispatcher;
MainWindow * singleImageWindow;

int main(int argc, char** argv) {
    Gtk::Main app(argc, argv);
    Glib::RefPtr<Gtk::Builder> builder = Gtk::Builder::create_from_resource("/glade/main_window.glade");
    builder->get_widget_derived("MainWindow", singleImageWindow);
	Gtk::Main::run(*singleImageWindow);
}