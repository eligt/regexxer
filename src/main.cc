/*
 * Copyright (c) 2002-2007  Daniel Elstner  <daniel.kitta@gmail.com>
 *
 * This file is part of regexxer.
 *
 * regexxer is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * regexxer is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with regexxer; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "globalstrings.h"
#include "mainwindow.h"
#include "miscutils.h"
#include "translation.h"

#include <popt.h>
#include <glib.h>
#include <gtk/gtkwindow.h> /* for gtk_window_set_default_icon_name() */
#include <glibmm.h>
#include <gconfmm.h>
#include <gtkmm/iconfactory.h>
#include <gtkmm/iconset.h>
#include <gtkmm/iconsource.h>
#include <gtkmm/main.h>
#include <gtkmm/stock.h>
#include <gtkmm/stockitem.h>
#include <gtkmm/window.h>

#include <cstdio>
#include <cstdlib>
#include <exception>
#include <list>

#include <config.h>


namespace
{

/*
 * Include inlined raw pixbuf data generated by gdk-pixbuf-csource.
 */
#include <ui/stockimages.h>


struct StockIconData
{
  const guint8*         data;
  int                   length;
  Gtk::BuiltinIconSize  size;
};

struct StockItemData
{
  const char*           id;
  const StockIconData*  icons;
  int                   n_icons;
  const char*           label;
};


const StockIconData stock_icon_save_all[] =
{
  { stock_save_all_16, sizeof(stock_save_all_16), Gtk::ICON_SIZE_MENU          },
  { stock_save_all_24, sizeof(stock_save_all_24), Gtk::ICON_SIZE_SMALL_TOOLBAR }
};

const StockItemData regexxer_stock_items[] =
{
  { "regexxer-save-all", stock_icon_save_all, G_N_ELEMENTS(stock_icon_save_all), N_("Save _all") }
};

const char *const locale_directory = REGEXXER_DATADIR G_DIR_SEPARATOR_S "locale";


std::auto_ptr<Regexxer::InitState> parse_command_line(int argc, char** argv)
{
  enum
  {
    NONE   = POPT_ARG_NONE,
    STRING = POPT_ARG_STRING
  };
  static const poptOption option_table[] =
  {
    { "pattern",      'p', STRING, 0, 'p', N_("Find files matching PATTERN"), N_("PATTERN") },
    { "no-recursion", 'R', NONE,   0, 'R', N_("Do not recurse into subdirectories"),      0 },
    { "hidden",       'h', NONE,   0, 'h', N_("Also find hidden files"),                  0 },
    { "regex",        'e', STRING, 0, 'e', N_("Find text matching REGEX"),    N_("REGEX")   },
    { "no-global",    'G', NONE,   0, 'G', N_("Find only the first match in a line"),     0 },
    { "ignore-case",  'i', NONE,   0, 'i', N_("Do case insensitive matching"),            0 },
    { "substitution", 's', STRING, 0, 's', N_("Replace matches with STRING"), N_("STRING")  },
    { "line-number",  'n', NONE,   0, 'n', N_("Print match location to standard output"), 0 },
    { "no-autorun",   'A', NONE,   0, 'A', N_("Do not automatically start search"),       0 },
    POPT_AUTOHELP
    POPT_TABLEEND
  };

  std::auto_ptr<Regexxer::InitState> init (new Regexxer::InitState());

  const poptContext context = poptGetContext(0, argc, const_cast<const char**>(argv),
                                             option_table, 0);

  poptSetOtherOptionHelp(context, _("[OPTION]... [FOLDER]"));

  bool autorun = true;
  int  rc;

  while ((rc = poptGetNextOpt(context)) >= 0)
    switch (rc)
    {
      case 'p': init->pattern      = Glib::locale_to_utf8(poptGetOptArg(context)); break;
      case 'e': init->regex        = Glib::locale_to_utf8(poptGetOptArg(context)); break;
      case 's': init->substitution = Glib::locale_to_utf8(poptGetOptArg(context)); break;
      case 'R': init->recursive    = false; break;
      case 'h': init->hidden       = true;  break;
      case 'G': init->global       = false; break;
      case 'i': init->ignorecase   = true;  break;
      case 'n': init->feedback     = true;  break;
      case 'A': autorun            = false; break;
    }

  if (rc < -1)
  {
    std::fprintf(stderr, "%s: %s\n%s\n",
                 poptBadOption(context, 0), poptStrerror(rc),
                 _("Try \"regexxer --help\" for more information."));
    std::exit(1);
  }

  if (const char *const folder = poptGetArg(context))
  {
    init->folder  = folder;
    init->autorun = autorun;

    if (poptPeekArg(context)) // more leftover arguments?
    {
      poptPrintUsage(context, stderr, 0);
      std::exit(1);
    }
  }

  poptFreeContext(context);

  return init;
}

void register_stock_items()
{
  const Glib::RefPtr<Gtk::IconFactory> factory = Gtk::IconFactory::create();
  const Glib::ustring domain = PACKAGE_TARNAME;

  for (unsigned int item = 0; item < G_N_ELEMENTS(regexxer_stock_items); ++item)
  {
    const StockItemData& stock = regexxer_stock_items[item];
    Gtk::IconSet icon_set;

    for (int icon = 0; icon < stock.n_icons; ++icon)
    {
      const StockIconData& icon_data = stock.icons[icon];
      Gtk::IconSource source;

      source.set_pixbuf(Gdk::Pixbuf::create_from_inline(icon_data.length, icon_data.data));
      source.set_size(icon_data.size);

      // Unset wildcarded for all but the the last icon.
      source.set_size_wildcarded(icon == stock.n_icons - 1);

      icon_set.add_source(source);
    }

    const Gtk::StockID stock_id (stock.id);

    factory->add(stock_id, icon_set);
    Gtk::Stock::add(Gtk::StockItem(stock_id, stock.label, Gdk::ModifierType(0), 0, domain));
  }

  factory->add_default();
}

void trap_gconf_exceptions()
{
  try
  {
    throw; // re-throw current exception
  }
  catch (const Gnome::Conf::Error&)
  {
    // Ignore GConf exceptions thrown from GObject signal handlers.
    // GConf itself is going print the warning message for us
    // since we set the error handling mode to CLIENT_HANDLE_ALL.
  }
}

void initialize_configuration()
{
  using namespace Gnome::Conf;

  Glib::add_exception_handler(&trap_gconf_exceptions);

  const Glib::RefPtr<Client> client = Client::get_default_client();

  client->set_error_handling(CLIENT_HANDLE_ALL);
  client->add_dir(REGEXXER_GCONF_DIRECTORY, CLIENT_PRELOAD_ONELEVEL);

  const std::list<Entry> entries (client->all_entries(REGEXXER_GCONF_DIRECTORY));

  // Issue an artificial value_changed() signal for each entry in /apps/regexxer.
  // Reusing the signal handlers this way neatly avoids the need for separate
  // startup-initialization routines.

  for (std::list<Entry>::const_iterator p = entries.begin(); p != entries.end(); ++p)
  {
    client->value_changed(p->get_key(), p->get_value());
  }
}

} // anonymous namespace


int main(int argc, char** argv)
{
  try
  {
    Gnome::Conf::init();
    Gtk::Main main_instance (&argc, &argv);
    Glib::set_application_name(PACKAGE_NAME);

    // Set the target encoding of the translation domain to UTF-8 only after
    // parse_command_line() finished, so that the help message is displayed
    // correctly on the console in locale encoding.
    Util::initialize_gettext(PACKAGE_TARNAME, locale_directory);
    std::auto_ptr<Regexxer::InitState> init_state = parse_command_line(argc, argv);
    Util::enable_utf8_gettext(PACKAGE_TARNAME);

    register_stock_items();
    gtk_window_set_default_icon_name("regexxer");

    Regexxer::MainWindow window;

    initialize_configuration();
    window.initialize(init_state);

    Gtk::Main::run(*window.get_window());
  }
  catch (const Glib::Error& error)
  {
    const Glib::ustring what = error.what();
    g_error("unhandled exception: %s", what.c_str());
  }
  catch (const std::exception& ex)
  {
    g_error("unhandled exception: %s", ex.what());
  }
  catch (...)
  {
    g_error("unhandled exception: (type unknown)");
  }

  return 0;
}
