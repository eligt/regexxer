/* $Id$
 *
 * Copyright (c) 2002  Daniel Elstner  <daniel.elstner@gmx.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License VERSION 2 as
 * published by the Free Software Foundation.  You are not allowed to
 * use any other version of the license; unless you got the explicit
 * permission from the author to do so.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef REGEXXER_FILELIST_H_INCLUDED
#define REGEXXER_FILELIST_H_INCLUDED

#include <iosfwd>
#include <gtkmm/treepath.h>
#include <gtkmm/treeview.h>

#include "filebuffer.h"
#include "sharedptr.h"

namespace Gtk  { class ListStore; }
namespace Pcre { class Pattern;   }


namespace Regexxer
{

struct FileInfo : public Util::SharedObject
{
  std::string              fullname;
  Glib::ustring            encoding;
  Glib::RefPtr<FileBuffer> buffer;

  explicit FileInfo(const std::string& fullname_);
  ~FileInfo();
};

typedef Util::SharedPtr<FileInfo> FileInfoPtr;


class FileList : public Gtk::TreeView
{
public:
  FileList();
  virtual ~FileList();

  void find_files(const Glib::ustring& dirname,
                  const Glib::ustring& pattern,
                  bool recursive, bool hidden);
  void stop_find_files();

  void select_first_file();
  bool select_next_file(bool move_forward);

  void find_matches(Pcre::Pattern& pattern, bool multiple);
  long get_match_count() const;
  void replace_all_matches(const Glib::ustring& substitution);

  SigC::Signal2<void,FileInfoPtr,BoundState>  signal_switch_buffer;
  SigC::Signal1<void,long>                    signal_match_count_changed;

private:
  struct FindData;

  Glib::RefPtr<Gtk::ListStore>  liststore_;
  bool                          find_running_;
  bool                          find_stop_;
  long                          sum_matches_;
  SigC::Connection              conn_match_count_;
  Gtk::TreePath                 path_match_first_;
  Gtk::TreePath                 path_match_last_;

  void find_recursively(const std::string& dirname, FindData& find_data);

  void on_selection_changed();
  void on_buffer_match_count_changed(int match_count);

  void load_file(const Util::SharedPtr<FileInfo>& fileinfo);
  Glib::RefPtr<FileBuffer> load_stream(std::istream& input);
  Glib::RefPtr<FileBuffer> convert_stream(std::istream& input, Glib::IConv& iconv);
};

} // namespace Regexxer

#endif

