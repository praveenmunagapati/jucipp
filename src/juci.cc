#include "juci.h"
#include "singletons.h"

using namespace std; //TODO: remove

void app::init_logging() {
  boost::log::add_common_attributes();
  auto log_dir = Singleton::Config::main()->juci_home_path()/"log"/"juci.log";
  boost::log::add_file_log(boost::log::keywords::file_name = log_dir,
               boost::log::keywords::auto_flush = true);
  JINFO("Logging initalized");
}

int app::on_command_line(const Glib::RefPtr<Gio::ApplicationCommandLine> &cmd) {
  Glib::set_prgname("juci");
  Glib::OptionContext ctx("[PATH ...]");
  Glib::OptionGroup gtk_group(gtk_get_option_group(true));
  ctx.add_group(gtk_group);
  int argc;
  char **argv = cmd->get_arguments(argc);
  ctx.parse(argc, argv);
  if(argc>=2) {
    for(int c=1;c<argc;c++) {
      boost::filesystem::path p(argv[c]);
      if(boost::filesystem::exists(p)) {
        p=boost::filesystem::canonical(p);
        if(boost::filesystem::is_regular_file(p))
          files.emplace_back(p);
        else if(boost::filesystem::is_directory(p))
          directories.emplace_back(p);
      }
      else
        std::cerr << "Path " << p << " does not exist." << std::endl;
    }
  }
  activate();
  return 0;
}

void app::on_activate() {
  add_window(*Singleton::window());
  Singleton::window()->show();
  bool first_directory=true;
  for(auto &directory: directories) {
    if(first_directory) {
      Singleton::directories()->open(directory);
      first_directory=false;
    }
    else {
      std::string files_in_directory;
      for(auto it=files.begin();it!=files.end();) {
        if(it->generic_string().substr(0, directory.generic_string().size()+1)==directory.generic_string()+'/') {
          files_in_directory+=" "+it->string();
          it=files.erase(it);
        }
        else
          it++;
      }
      std::thread another_juci_app([this, directory, files_in_directory](){
        Singleton::terminal()->async_print("Executing: juci "+directory.string()+files_in_directory);
        Singleton::terminal()->execute("juci "+directory.string()+files_in_directory, ""); //TODO: do not open pipes here, doing this after Juci compiles on Windows
      });
      another_juci_app.detach();
    }
  }
  for(auto &file: files)
    Singleton::window()->notebook.open(file);
}

app::app() : Gtk::Application("no.sout.juci", Gio::APPLICATION_NON_UNIQUE | Gio::APPLICATION_HANDLES_COMMAND_LINE) {
  init_logging();
}

int main(int argc, char *argv[]) {
  return app().run(argc, argv);
}
