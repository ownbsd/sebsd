/****************************************************************************
 * Copyright (c) 2002 Free Software Foundation, Inc.                        *
 *                                                                          *
 * Permission is hereby granted, free of charge, to any person obtaining a  *
 * copy of this software and associated documentation files (the            *
 * "Software"), to deal in the Software without restriction, including      *
 * without limitation the rights to use, copy, modify, merge, publish,      *
 * distribute, distribute with modifications, sublicense, and/or sell       *
 * copies of the Software, and to permit persons to whom the Software is    *
 * furnished to do so, subject to the following conditions:                 *
 *                                                                          *
 * The above copyright notice and this permission notice shall be included  *
 * in all copies or substantial portions of the Software.                   *
 *                                                                          *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS  *
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF               *
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.   *
 * IN NO EVENT SHALL THE ABOVE COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,   *
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR    *
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR    *
 * THE USE OR OTHER DEALINGS IN THE SOFTWARE.                               *
 *                                                                          *
 * Except as contained in this notice, the name(s) of the above copyright   *
 * holders shall not be used in advertising or otherwise to promote the     *
 * sale, use or other dealings in this Software without prior written       *
 * authorization.                                                           *
 ****************************************************************************/

/****************************************************************************
 *  Author: Thomas Dickey 2002                                              *
 ****************************************************************************/

/*
**	lib_ins_wch.c
**
**	The routine wins_wch().
**
*/

#include <curses.priv.h>

MODULE_ID("$Id$")

NCURSES_EXPORT(int)
wins_wch(WINDOW *win, const cchar_t * wch)
{
    int code = ERR;

    T((T_CALLED("wins_wch(%p, %s)"), win, _tracecchar_t(wch)));

    if (win) {
	struct ldat *line = &(win->_line[win->_cury]);
	NCURSES_CH_T *end = &(line->text[win->_curx]);
	NCURSES_CH_T *temp1 = &(line->text[win->_maxx]);
	NCURSES_CH_T *temp2 = temp1 - 1;

	CHANGED_TO_EOL(line, win->_curx, win->_maxx);
	while (temp1 > end)
	    *temp1-- = *temp2--;

	*temp1 = _nc_render(win, *wch);
	code = OK;
    }
    returnCode(code);
}
