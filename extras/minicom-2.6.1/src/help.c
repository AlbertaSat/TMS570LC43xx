/*
 * help		Print a help window.
 *
 *		This file is part of the minicom communications package,
 *		Copyright 1991-1995 Miquel van Smoorenburg.
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *
 *  26.02.1998 - acme@conectiva.com.br - i18n
 *  mark.einon@gmail.com 16/02/11 - Added option to timestamp terminal output
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "port.h"
#include "minicom.h"
#include "intl.h"

/* Draw a help screen and return the keypress code. */
int help(void)
{
  WIN *w;
  int c;
  int i = 1;
  int x1, x2;

  x1 = (COLS / 2) - 34;
  x2 = (COLS / 2) + 32;
  w = mc_wopen(x1, 2, x2, 20 + i, BDOUBLE, stdattr, mfcolor, mbcolor, 0, 0, 1);

  mc_wlocate(w, 21, 0);
  mc_wputs(w, _("Minicom Command Summary"));
  mc_wlocate(w, 10, 2);

  mc_wprintf(w, _("Commands can be called by %s<key>"), esc_key());

  mc_wlocate(w, 15, 4);
  mc_wputs(w, _("Main Functions"));
  mc_wlocate(w, 47, 4);
  mc_wputs(w, _("Other Functions"));
  mc_wlocate(w, 0, 6);
  mc_wputs(w, _(" Dialing directory..D  run script (Go)....G | Clear Screen.......C\n"));
  mc_wputs(w, _(" Send files.........S  Receive files......R | cOnfigure Minicom..O\n"));
  mc_wputs(w, _(" comm Parameters....P  Add linefeed.......A | "));
#ifdef SIGTSTP
  mc_wputs(w, _("Suspend minicom....J\n"));
#else
  mc_wputs(w, _("Jump to a shell....J\n"));
#endif
  mc_wputs(w, _(" Capture on/off.....L  Hangup.............H | eXit and reset.....X\n"));
  mc_wputs(w, _(" send break.........F  initialize Modem...M | Quit with no reset.Q\n"));
  mc_wputs(w, _(" Terminal settings..T  run Kermit.........K | Cursor key mode....I\n"));
  mc_wputs(w, _(" lineWrap on/off....W"));
  mc_wputs(w, _("  local Echo on/off..E | Help screen........Z\n"));
  mc_wputs(w, _(" Paste file.........Y  Timestamp toggle...N | scroll Back........B"));

  mc_wlocate(w, 13, 16 + i);
  mc_wputs(w, _("Written by Miquel van Smoorenburg 1991-1995"));
  mc_wlocate(w, 13, 17 + i);
  mc_wputs(w, _("Some additions by Jukka Lahtinen 1997-2000"));
  mc_wlocate(w, 13, 18 + i);
  mc_wputs(w, _("i18n by Arnaldo Carvalho de Melo 1998"));
  mc_wlocate(w, 6, 14 + i);
  mc_wputs(w, _("Select function or press Enter for none."));
  mc_wredraw(w, 1);

  c = wxgetch();
  mc_wclose(w, 1);
  return(c);
}
