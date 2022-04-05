#include "settings.h"
#include <cstdlib>
#include <chrono>
#include <fstream>
#include <string>
#include <nlohmann/json.hpp>
#include <iostream>

using namespace std;
using json = nlohmann::json;

Settings::Settings(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& refGlade): Gtk::Dialog(cobject), builder(refGlade) {
    std::ifstream inputconfig("settings.json");
    string metafile = "";
    if (inputconfig.good()) {
        json settings;
        inputconfig >> settings;
        metafile = settings["metafile"];
    }
    if (metafile=="") metafile="/tmp/shairport-sync-metadata";
    this->builder->get_widget("tMetafile", tMetafile);
    tMetafile->set_text(metafile);
    Gtk::Button *bOK;
    this->builder->get_widget("bOK", bOK);
    bOK->signal_clicked().connect(sigc::mem_fun(*this, &Settings::onOK));
    Gtk::Button *bCancel;
    this->builder->get_widget("bCancel", bCancel);
    bCancel->signal_clicked().connect(sigc::mem_fun(*this, &Settings::onClose));

}

void Settings::onOK() {
    json settings;
    settings["metafile"]=std::string(tMetafile->get_text());
    std::ofstream outputsetting("settings.json");
    settings >> outputsetting;
    outputsetting.flush();
    outputsetting.close();
    Gtk::Dialog::close();
}

void Settings::onClose() {
    std::cout<<"closing"<<std::endl;
    Gtk::Dialog::close();
}