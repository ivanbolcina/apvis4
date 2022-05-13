#include <iostream>
#include <gtkmm.h>

#include <plog/Log.h>
#include <plog/Init.h>
#include <plog/Formatters/TxtFormatter.h>
#include <plog/Appenders/ConsoleAppender.h>

#include "main_window.h"

using namespace std;

Glib::Dispatcher dispatcher;
MainWindow * singleImageWindow;

int main(int argc, char** argv) {
    static plog::ConsoleAppender<plog::TxtFormatter> consoleAppender;
    plog::init(plog::debug, &consoleAppender);
    Gtk::Main app(argc, argv);
    Glib::RefPtr<Gtk::Builder> builder = Gtk::Builder::create_from_resource("/glade/main_window.glade");
    builder->get_widget_derived("MainWindow", singleImageWindow);
	Gtk::Main::run(*singleImageWindow);
}