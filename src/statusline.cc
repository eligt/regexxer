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

#include "statusline.h"
#include "stringutils.h"
#include "translation.h"

#include <gdk/gdkkeysyms.h>
#include <gtkmm.h>
#include <locale>
#include <sstream>
#include <stdexcept>

namespace Regexxer
{

/**** Regexxer::CounterBox *************************************************/

class CounterBox : public Gtk::Frame
{
public:
  explicit CounterBox(const Glib::ustring& label);
  virtual ~CounterBox();

  void set_index(int index);
  void set_count(int count);

private:
  enum { MIN_RANGE = 100 };

  Gtk::Label*         label_index_;
  Gtk::Label*         label_count_;
  int                 digits_range_;
  int                 widest_digit_;
  int                 second_widest_digit_;
  std::wostringstream stringstream_;

  Glib::ustring number_to_string(int number);

  void recalculate_label_width();
  void on_label_style_updated();
};

CounterBox::CounterBox(const Glib::ustring& label)
:
  label_index_          (0),
  label_count_          (0),
  digits_range_         (MIN_RANGE),
  widest_digit_         (0),
  second_widest_digit_  (9)
{
  using namespace Gtk;

  set_shadow_type(SHADOW_ETCHED_IN);

  Box *const paddingbox = new Box();
  add(*manage(paddingbox));

  Box *const box = new Box();
  paddingbox->pack_start(*manage(box), PACK_SHRINK, 2);

  box->pack_start(*manage(new Label(label + ' ')), PACK_SHRINK);

  label_index_ = new Label("", ALIGN_END, ALIGN_CENTER);
  box->pack_start(*manage(label_index_), PACK_SHRINK);

  box->pack_start(*manage(new Label("/")), PACK_SHRINK, 2);

  label_count_ = new Label("", ALIGN_START, ALIGN_CENTER);
  box->pack_start(*manage(label_count_), PACK_SHRINK);

  label_index_->set_single_line_mode(true);
  label_count_->set_single_line_mode(true);

  label_index_->signal_style_updated().connect(
      sigc::mem_fun(*this, &CounterBox::on_label_style_updated));

  try // don't abort if the user-specified locale doesn't exist
  {
    stringstream_.imbue(std::locale(""));
  }
  catch (const std::runtime_error& error)
  {
    g_warning("%s", error.what());
  }

  set_index(0);
  set_count(0);

  paddingbox->show_all();
}

CounterBox::~CounterBox()
{}

void CounterBox::set_index(int index)
{
  label_index_->set_text(number_to_string(index));

  // Work around a bug in GTK+ that causes right-aligned labels to be
  // cut off at the end.  Forcing resize seems to solve the problem.
  check_resize();
}

void CounterBox::set_count(int count)
{
  int range = digits_range_;

  while (range <= count)
    range *= 10;

  while (range > MIN_RANGE && range / 10 > count)
    range /= 10;

  if (range != digits_range_)
  {
    digits_range_ = range;
    recalculate_label_width();
  }

  label_count_->set_text(number_to_string(count));
}

Glib::ustring CounterBox::number_to_string(int number)
{
  stringstream_.str(std::wstring());
  stringstream_ << number;
  return Util::wstring_to_utf8(stringstream_.str());
}

/*
 * This tricky piece of code calculates the optimal label width for any
 * number < digits_range_.  It does assume the decimal system is used,
 * but it relies neither on the string representation of the digits nor
 * on the font of the label.
 */
void CounterBox::recalculate_label_width()
{
  int widest_number = 0;

  if (widest_digit_ != 0)
  {
    // E.g. range 1000 becomes 222 if '2' is the widest digit of the font.
    widest_number = ((digits_range_ - 1) / 9) * widest_digit_;
  }
  else // eeeks, 0 has to be special-cased
  {
    g_assert(second_widest_digit_ != 0);

    // E.g. range 1000 becomes 900 if '9' is the 2nd-widest digit.
    widest_number = (digits_range_ / 10) * second_widest_digit_;
  }

  int width = 0, height = 0, xpad = 0;

  const Glib::ustring text = number_to_string(widest_number);
  label_index_->create_pango_layout(text)->get_pixel_size(width, height);

  xpad = label_index_->get_margin_start() + label_index_->get_margin_end();
  label_index_->set_size_request(width + 2 * xpad, -1);

  xpad = label_count_->get_margin_start() + label_count_->get_margin_end();
  label_count_->set_size_request(width + 2 * xpad, -1);
}

/*
 * The code relies on both labels having the same style.
 * I think that's a quite safe assumption to make ;)
 */
void CounterBox::on_label_style_updated()
{
  const Glib::RefPtr<Pango::Layout> layout = label_index_->create_pango_layout("");

  widest_digit_        = 0;
  second_widest_digit_ = 9;

  int max_width        = 0;
  int second_max_width = 0;

  int width  = 0;
  int height = 0;

  for (int digit = 0; digit <= 9; ++digit)
  {
    layout->set_text(number_to_string(digit));
    layout->get_pixel_size(width, height);

    if (width > max_width)
    {
      max_width = width;
      widest_digit_ = digit;
    }
    else if (width > second_max_width)
    {
      second_max_width = width;
      second_widest_digit_ = digit;
    }
  }

  recalculate_label_width();
}

/**** Regexxer::StatusLine *************************************************/

StatusLine::StatusLine()
:
  Gtk::Box        (Gtk::ORIENTATION_HORIZONTAL, 3),
  stop_button_    (0),
  progressbar_    (0),
  file_counter_   (0),
  match_counter_  (0),
  statusbar_      (0)
{
  using namespace Gtk;

  // The statusbar looks ugly if the stop button gets its default size,
  // so let's reduce the button's size request to a reasonable amount.

  std::string style = "GtkButton#regexxer-stop-button\n"
                      "{\n"
                      "  padding-left: 0;"
                      "  -GtkWidget-interior-focus: 0;\n"
                      "  -GtkWidget-focus-padding: 0;\n"
                      "}";
  Glib::RefPtr<Gtk::CssProvider> css = Gtk::CssProvider::create();
  css->load_from_data(style);

  stop_button_ = new Button(_("Stop"));
  pack_start(*manage(stop_button_), PACK_SHRINK);
  stop_button_->set_name("regexxer-stop-button");
  stop_button_->get_style_context()->add_provider(css, 1);

  progressbar_ = new ProgressBar();
  pack_start(*manage(progressbar_), PACK_SHRINK);

  file_counter_ = new CounterBox(_("File:"));
  pack_start(*manage(file_counter_), PACK_SHRINK);

  match_counter_ = new CounterBox(_("Match:"));
  pack_start(*manage(match_counter_), PACK_SHRINK);

  statusbar_ = new Statusbar();
  pack_start(*manage(statusbar_), PACK_EXPAND_WIDGET);

  stop_button_->set_sensitive(false);
  stop_button_->signal_clicked().connect(signal_cancel_clicked.make_slot());

  progressbar_->set_pulse_step(0.025);

  stop_button_->get_accessible()->set_description(_("Cancels the running search"));

  show_all_children();
}

StatusLine::~StatusLine()
{}

void StatusLine::set_file_index(int file_index)
{
  file_counter_->set_index(file_index);
}

void StatusLine::set_file_count(int file_count)
{
  file_counter_->set_count(file_count);
}

void StatusLine::set_match_index(int match_index)
{
  match_counter_->set_index(match_index);
}

void StatusLine::set_match_count(int match_count)
{
  match_counter_->set_count(match_count);
}

void StatusLine::set_file_encoding(const std::string& file_encoding)
{
  statusbar_->pop();

  const Glib::ustring encoding = file_encoding;
  g_return_if_fail(encoding.is_ascii());

  statusbar_->push(encoding);

  // Work around a bug in GTK+ that causes right-aligned labels (note that
  // the status bar text is right-aligned in RTL locales) to be cut off at
  // the end.  Forcing resize seems to solve the problem.
  statusbar_->check_resize();
}

void StatusLine::pulse_start()
{
  stop_button_->set_sensitive(true);
}

void StatusLine::pulse()
{
  progressbar_->pulse();
}

void StatusLine::pulse_stop()
{
  progressbar_->set_fraction(0.0);
  stop_button_->set_sensitive(false);
}

void StatusLine::on_hierarchy_changed(Gtk::Widget* previous_toplevel)
{
  if (Gtk::Window *const window = dynamic_cast<Gtk::Window*>(previous_toplevel))
  {
    stop_button_->remove_accelerator(
        window->get_accel_group(), GDK_KEY_Escape, Gdk::ModifierType(0));
  }

  Gtk::Box::on_hierarchy_changed(previous_toplevel);

  if (Gtk::Window *const window = dynamic_cast<Gtk::Window*>(get_toplevel()))
  {
    stop_button_->add_accelerator(
        "activate", window->get_accel_group(),
        GDK_KEY_Escape, Gdk::ModifierType(0), Gtk::AccelFlags(0));
  }
}

} // namespace Regexxer
