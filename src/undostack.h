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

#ifndef REGEXXER_UNDOSTACK_H_INCLUDED
#define REGEXXER_UNDOSTACK_H_INCLUDED

#include "sharedptr.h"
#include <stack>


namespace Regexxer
{

class UndoAction : public Util::SharedObject
{
public:
  UndoAction() {}
  virtual ~UndoAction() = 0;

  bool undo();

private:
  UndoAction(const UndoAction&);
  UndoAction& operator=(const UndoAction&);

  virtual bool do_undo() = 0;
};

typedef Util::SharedPtr<UndoAction> UndoActionPtr;


class UndoStack : public UndoAction
{
public:
  UndoStack();
  virtual ~UndoStack();

  void push(const UndoActionPtr& action);
  bool empty() const;

  void undo_step();

private:
  std::stack<UndoActionPtr> actions_;

  virtual bool do_undo();
};

typedef Util::SharedPtr<UndoStack> UndoStackPtr;


} // namespace Regexxer

#endif /* REGEXXER_UNDOSTACK_H_INCLUDED */
