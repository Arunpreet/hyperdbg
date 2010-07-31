/*
  Copyright notice
  ================
  
  Copyright (C) 2010
      Lorenzo  Martignoni <martignlo@gmail.com>
      Roberto  Paleari    <roberto.paleari@gmail.com>
      Aristide Fattori    <joystick@security.dico.unimi.it>
  
  This program is free software: you can redistribute it and/or modify it under
  the terms of the GNU General Public License as published by the Free Software
  Foundation, either version 3 of the License, or (at your option) any later
  version.
  
  HyperDbg is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License along with
  this program. If not, see <http://www.gnu.org/licenses/>.
  
*/

#include "pager.h"
#include "gui.h"
#include "video.h"
#include "common.h"
#include "vmmstring.h"
#include "keyboard.h"
#include "debug.h"

/* ################# */
/* #### GLOBALS #### */
/* ################# */

/* PAGES matrix to be printed in the GUI */
Bit8u  pages[PAGES][OUT_SIZE_Y][OUT_SIZE_X]; 
Bit32u current_page = 0;
Bit32u current_line = 0;
Bit32u current_visible_page = 0;
/* Set default color */
Bit32u c = LIGHT_GREEN;

/* ########################## */
/* #### LOCAL PROTOTYPES #### */
/* ########################## */

static void PagerReset(void);
static void PagerShowPage(Bit32u page);
static void PagerEditGui(void);
static void PagerRestoreGui(void);

/* ################ */
/* #### BODIES #### */
/* ################ */

/* This will be called at the end of PagerLoop so that it cleans out on exit */
static void PagerReset()
{
  Bit32u i, ii, iii;
  /* leave last screen on the console */
  for(ii = 0; ii < OUT_SIZE_Y; ii++) {
    vmm_strncpy(out_matrix[ii], pages[current_visible_page][ii], OUT_SIZE_X);
  }

  /* clear pages */
  for(i = 0; i < PAGES; i++) {
    for(ii = 0; ii < OUT_SIZE_Y; ii++) {
      for(iii = 0; iii < OUT_SIZE_X; iii++)
	pages[i][ii][iii] = 0x20; /* We use spaces to reset as in gui.c */
    }
  }
  current_page = current_line = current_visible_page = 0;
}

static void PagerShowPage(Bit32u page)
{
  Bit32u ii, len;
  if(page >= PAGES) return;
  /* Copy selected page into out matrix Note: we always
     copy OUT_SIZE_X to eventually overwrite preceding chars */
  for(ii = 0; ii < OUT_SIZE_Y; ii++) 
    VideoWriteString(pages[page][ii], OUT_SIZE_X, c, OUT_START_X, OUT_START_Y+ii);

}

static void PagerEditGui()
{
  /* Edit GUI to indicate that we are in pager mode */
  VideoWriteChar('=', WHITE, 3, SHELL_SIZE_Y-3);
  VideoWriteChar('[', WHITE, 4, SHELL_SIZE_Y-3);
  VideoWriteString("n: next; p: prev; q: quit", 25, LIGHT_BLUE, 5, SHELL_SIZE_Y-3);
  VideoWriteChar(']', WHITE, 30, SHELL_SIZE_Y-3);
  VideoWriteChar('=', WHITE, 31, SHELL_SIZE_Y-3);
}

static void PagerRestoreGui()
{
  Bit32u i;
  for(i = 1; i < SHELL_SIZE_X-1; i++) {
    VideoWriteChar('-', WHITE, i, SHELL_SIZE_Y-3);
  }
}

void PagerLoop(Bit32u color)
{
  Bit8u ch;
  hvm_bool isMouse, exitLoop = FALSE;
  c = color;
  PagerEditGui();
  current_visible_page = 0;
  PagerShowPage(current_visible_page);
  VideoResetOutMatrix();
  Log("[HyperDbg] Starting Pager Loop");
  while(1) {
    if (KeyboardReadKeystroke(&ch, FALSE, &isMouse) != HVM_STATUS_SUCCESS) {
      /* Sleep for some time, just to avoid full busy waiting */
      CmSleep(150);
      continue;
    }

    if (isMouse) {
      /* Skip mouse events */
      continue;
    }

    ch = KeyboardScancodeToKeycode(ch);
    switch(ch) {
    case 0:
      /* Unrecognized key -- ignore it */
      break;
    case 'n':
      if(current_visible_page < PAGES && current_visible_page < current_page)
	 PagerShowPage(++current_visible_page);
      break;
    case 'p':
      if(current_visible_page > 0)
	PagerShowPage(--current_visible_page);
      break;
    case 'q':
      exitLoop = TRUE;
      break;
    default:
      break; /* Do nothing */
    }
    if(exitLoop) break;
  }
  PagerRestoreGui();
  PagerReset(); 
  VideoRefreshOutArea(LIGHT_GREEN);
  Log("[HyperDbg] Exiting Pager Loop");
}

hvm_bool PagerAddLine(Bit8u *line)
{
  Bit32u len;
  /* Perform some checks */
  if(current_page >= PAGES) return FALSE; /* No more space! */
  if(current_line >= OUT_SIZE_Y) return  FALSE; /* As we take care of checking this at this very function end, if we arrive here it means something is screwed up */

  len = vmm_strlen(line);
  if(len >= OUT_SIZE_X)
    len = OUT_SIZE_X - 1;
  
  vmm_strncpy(pages[current_page][current_line], line, len);
  current_line++;
  if(current_line >= OUT_SIZE_Y) { /* Flip one page */
    current_line = 0;
    current_page++;                /* The check at the beginning of the function will take care of EOPages */
  }
  return TRUE;
}
