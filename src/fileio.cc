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

#include "fileio.h"
#include "filebuffer.h"
#include "stringutils.h"

#include <glib.h>
#include <glibmm.h>


namespace
{

using Regexxer::FileBuffer;

enum { BUFSIZE = 4096 };

const char fallback_encoding[] = "ISO-8859-1";


Glib::RefPtr<FileBuffer> load_iochannel(const Glib::RefPtr<Glib::IOChannel>& input)
{
  const Glib::RefPtr<FileBuffer> text_buffer = FileBuffer::create();
  FileBuffer::iterator text_end (text_buffer->end());

  const Glib::ScopedPtr<char> inbuf (g_new(char, BUFSIZE));
  gsize bytes_read = 0;

  while(input->read(inbuf.get(), BUFSIZE, bytes_read) == Glib::IO_STATUS_NORMAL)
  {
    if(Util::contains_null(inbuf.get(), inbuf.get() + bytes_read))
      return Glib::RefPtr<FileBuffer>();

    text_end = text_buffer->insert(text_end, inbuf.get(), inbuf.get() + bytes_read);
  }

  g_assert(bytes_read == 0);

  return text_buffer;
}

void save_iochannel(const Glib::RefPtr<Glib::IOChannel>& output, const Glib::RefPtr<FileBuffer>& buffer)
{
  FileBuffer::iterator start = buffer->begin();
  FileBuffer::iterator stop  = start;

  for(; start; start = stop)
  {
    stop.forward_chars(BUFSIZE); // inaccurate, but doesn't matter
    const Glib::ustring chunk (buffer->get_text(start, stop));

    gsize bytes_written = 0;
    const Glib::IOStatus status = output->write(chunk.data(), chunk.bytes(), bytes_written);

    g_assert(status == Glib::IO_STATUS_NORMAL);
    g_assert(bytes_written == chunk.bytes());
  }
}

Glib::RefPtr<FileBuffer> load_try_encoding(const Glib::RefPtr<Glib::IOChannel>& input,
                                           const std::string& encoding)
{
  input->seek(0);

  try
  {
    input->set_encoding(encoding);
    return load_iochannel(input);
  }
  catch(const Glib::ConvertError&)
  {}

  return Glib::RefPtr<FileBuffer>();
}

} // anonymous namespace


namespace Regexxer
{

/**** Regexxer::FileInfo ***************************************************/

FileInfo::FileInfo(const std::string& fullname_)
:
  fullname    (fullname_),
  load_failed (false)
{}

FileInfo::~FileInfo()
{}


/**** Regexxer -- file I/O functions ***************************************/

void load_file(const FileInfoPtr& fileinfo)
{
  fileinfo->load_failed = true;

  const Glib::RefPtr<Glib::IOChannel> channel =
      Glib::IOChannel::create_from_file(fileinfo->fullname, "r");

  channel->set_buffer_size(BUFSIZE);

  std::string encoding = "UTF-8";
  Glib::RefPtr<FileBuffer> buffer = load_try_encoding(channel, encoding);

  if(!buffer && !Glib::get_charset(encoding)) // locale charset is _not_ UTF-8
  {
    buffer = load_try_encoding(channel, encoding);
  }

  if(!buffer && !Util::encodings_equal(encoding, fallback_encoding))
  {
    encoding = fallback_encoding;
    buffer = load_try_encoding(channel, encoding);
  }

  channel->close();

  if(!buffer)
  {
    const Glib::ustring filename = Util::filename_to_utf8_fallback(fileinfo->fullname);
    g_message("couldn't convert `%s' to UTF-8", filename.c_str());
  }
  else
  {
    buffer->set_modified(false);

    fileinfo->load_failed = false;
    fileinfo->encoding    = encoding;
    fileinfo->buffer      = buffer;
  }
}

void save_file(const FileInfoPtr& fileinfo)
{
  const Glib::RefPtr<Glib::IOChannel> channel =
      Glib::IOChannel::create_from_file(fileinfo->fullname, "w");

  channel->set_buffer_size(2 * BUFSIZE); // XXX: workaround for GLib <= 2.0.6 (#96373)

  channel->set_encoding(fileinfo->encoding);
  save_iochannel(channel, fileinfo->buffer);

  channel->close(); // might throw

  fileinfo->buffer->set_modified(false);
}

} // namespace Regexxer
