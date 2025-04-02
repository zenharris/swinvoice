

#include <stdio.h>
#include <stdlib.h>

#include ints
#include string
#include ctype

#include smgdef
#include smg$routines
#include rms
#include starlet
#include errno

#include "extools.h"
#include "screen.h"
#include "filebrowse.h"
#include "warns.h"

#include "swfile.h"
#include "colours.h"

void strupr (char *);

#define SHEADING "Select Inventory Item"

struct kit_part_rec kitsed_buff;
struct kit_part_rec comp_buff;

static int field_row,field_col;


extern struct filestruct inventory_file;

extern int TermWidth;
extern int win_depth;
extern int BottomLine;

/*
capital(char *in_str)
{
   strupr(in_str);
   return(TRUE);
}
*/

int get_inventory_item ();

extern struct inventory_rec inventory_buff;
extern struct inventory_key0_rec inventory_listkey;
int inventory_lnth;

void inventory_trans ();
int inventory_keymatch ();
int select_inventory_item (struct kit_part_rec *);

void
slct_invent ()
{
  select_inventory_item (&kitsed_buff);
}

struct cmndline_params cmnd_line[] = {
  1, NULL, NULL, 0, NULL,
  F1, "Inventory", NULL, 0, slct_invent
};

void invent_search ();
struct cmndline_params invent_cmndline[] = {
  1, NULL, NULL, 0, NULL,
  F1, "Search", NULL, 0, invent_search
};

/* *INDENT-OFF* */
#define LINEDLEN 5
struct scr_params kit_line[] = {
   0,0,"",PROMPTCOL,UNFMT_STRING,kitsed_buff.part_code,FIELDCOL,10,get_inventory_item,cmnd_line,
   0,10,"|",PROMPTCOL,UNFMT_STRING|NON_EDIT,kitsed_buff.description,FIELDCOL,28,NULL,NULL,
   0,39,"|",PROMPTCOL,UNFMT_STRING|NON_EDIT,kitsed_buff.units,FIELDCOL,5,NULL,NULL,
   0,45,"|%7.2f",PROMPTCOL,NUMERIC,&kitsed_buff.number,FIELDCOL,7,NULL,NULL,
   0,53,"|",PROMPTCOL,UNFMT_STRING,kitsed_buff.ext_description,FIELDCOL,23,NULL,NULL
};
/* *INDENT-ON* */

void inventory_search_2 ();

#define KIT_SELECT_LENGTH 5

static int topline  (int winlnth)
{
   unsigned int RegionLength;
   smg$get_display_attr (&crnt_window->display_id, &RegionLength);
   if ((field_row+2+winlnth) > RegionLength)
      return(field_row-winlnth-1);
   else
      return(field_row+2);
}

int
select_inventory_item (struct kit_part_rec *crnt_line)
{
  int ret, status;
  unsigned long rec_num;

/*  spawn_scroll_window (&inventory_file, 10, 68);*/

  open_window (crnt_window->rul+topline(KIT_SELECT_LENGTH),  /*field_row-QUOTE_SELECT_LENGTH-1,*/
               crnt_window->cul+field_col+5, 68,
               KIT_SELECT_LENGTH , WHITE, border);
  smg$get_display_attr (&crnt_window->display_id, &inventory_file.RegionLength);
  inventory_file.BottomLine = inventory_file.RegionLength;
  inventory_file.TopLine = 2;



  HeadLine (SHEADING, centre);

/*
      rfa_copy(&inventory_file.rab.rab$w_rfa,File_Browse(&inventory_file,select));

      if (!rfa_iszero((struct rfabuff *)&inventory_file.rab.rab$w_rfa)) {
         inventory_file.rab.rab$b_rac = RAB$C_RFA;
         inventory_file.rab.rab$l_ubf = (char *)inventory_file.fileio;
         inventory_file.rab.rab$w_usz = inventory_file.fileiosz;
         inventory_file.rab.rab$l_rop = RAB$M_NLK;

         rms_status = sys$get(&inventory_file.rab);
         if (!rmstest(rms_status,2,RMS$_NORMAL,RMS$_OK_RLK)) 
            warn_user("$GET %s", rmserror(rms_status,&inventory_file));
         else {
            close_window();
            win_depth--;
            Help(TEXACOHLP);
            memcpy(crnt_line->part_code,inventory_buff.part_code,PARTKEYLEN);

            exclreol(kit_line[0].row,kit_line[0].col,BLACK);
            init_screen(kit_line,LINEDLEN);
            return(TRUE);
         }
      }
*/



  if (!rfa_iszero (File_Browse (&inventory_file, select)))
    {
      close_window ();
/*      win_depth--;*/
      Help (TEXACOHLP);
      memcpy (crnt_line->part_code, inventory_buff.part_code, PARTKEYLEN);
      unpad (crnt_line->part_code, PARTKEYLEN);
/*
       strcpy(kitsed_buff.description,inventory_buff.description);
       warn_user("Desc:%s",inventory_buff.description);
       strcpy(kitsed_buff.units,inventory_buff.units);
*/
      exclreol (kit_line[0].row, kit_line[0].col, BLACK);
      init_screen (kit_line, LINEDLEN);
      return (TRUE);
    }

  close_window ();
/*  win_depth--;*/
  Help (TEXACOHLP);
  return (FALSE);
}


int
get_inventory_item (char *code)
{
  int lnth, status;
  struct inventory_key0_rec inventory_key;

  strupr (code);


  strncpy (inventory_listkey.part_code, code, PARTKEYLEN);
  pad (inventory_listkey.part_code, PARTKEYLEN);


  inventory_file.rab.rab$b_krf = INV_PART_KEY;
  inventory_file.rab.rab$l_kbf = (char *) &inventory_listkey;
  inventory_file.rab.rab$b_ksz = sizeof (inventory_listkey);
  inventory_file.rab.rab$b_rac = RAB$C_KEY;
  inventory_file.rab.rab$l_ubf = (char *) inventory_file.fileio;        /*inventory_buff; /*record; */
  inventory_file.rab.rab$w_usz = inventory_file.fileiosz;       /*sizeof inventory_buff; */
  inventory_file.rab.rab$l_rop = RAB$M_NLK | RAB$M_EQNXT;

  rms_status = sys$get (&inventory_file.rab);

  if (!rmstest (rms_status, 3, RMS$_NORMAL, RMS$_RNF, RMS$_OK_RLK))
    {
      warn_user ("GETEQ %s", rmserror(rms_status,&inventory_file));
      return (FALSE);
    }

  if (rms_status == RMS$_RNF
      || strncmp (code, inventory_buff.part_code, strlen (code)) != 0)
    {
      warn_user ("No Match Found");

      *kitsed_buff.part_code = 0;
      exclreol (kit_line[0].row, kit_line[0].col, BLACK);
      init_screen (kit_line, LINEDLEN);

      return (FALSE);
    }

  strncpy (kitsed_buff.part_code, inventory_buff.part_code, PARTKEYLEN);
  unpad (kitsed_buff.part_code, PARTKEYLEN);

  strcpy (kitsed_buff.description, inventory_buff.description);
  strcpy (kitsed_buff.units, inventory_buff.units);
  exclreol (kit_line[0].row, kit_line[0].col, BLACK);
  init_screen (kit_line, LINEDLEN);

  return (TRUE);
}


void *
lmalloc (size_t amnt)
{
  struct line_rec *temp_ptr = NULL;
  if ((temp_ptr = malloc (amnt + (sizeof (temp_ptr) * 2))) != NULL)
    {
      memset (temp_ptr->storage, 0, amnt);
      temp_ptr->next = NULL;
      temp_ptr->last = NULL;
    }
  return (temp_ptr);
}


int
kitsed (edtstr, row, col, lnth, attr, pos, control,filedef)
     char *edtstr;
     int col, row, lnth, attr, *pos;
     unsigned int *control;
     struct filestruct *filedef;
{
  int i, edited = FALSE, ret;
  static int strtfld = 0;
  struct line_rec *curs;

  field_row = row;
  field_col = col;


  memcpy (&kitsed_buff, edtstr, sizeof (kitsed_buff));
  if (*control & INIT_ONLY)
    {
      for (i = 0; i < LINEDLEN; i++)
        kit_line[i].row = row;
      init_screen (kit_line, LINEDLEN);
      return (TRUE);
    }
  else if (*control & ALLOCATE)
    {
      (*(char **) edtstr) = lmalloc (sizeof (kitsed_buff));
      strtfld = 0;
      return (TRUE);
    }
  else if (*control & COMP_LOAD)
    {
      memcpy (&comp_buff, &kitsed_buff, sizeof (kitsed_buff));
      return (TRUE);
    }
  else if (*control & COMP_CHECK)
    {
      return (memcmp (&comp_buff, &kitsed_buff, sizeof (kitsed_buff)) != 0);
    }
  for (i = 0; i < LINEDLEN; i++)
    kit_line[i].row = row;
  init_screen (kit_line, LINEDLEN);
  ret =
    enter_screen (kit_line, LINEDLEN, &edited, breakout, filedef, &strtfld);
  switch (ret)
    {
    case SMG$K_TRM_RIGHT:
    case SMG$K_TRM_CTRLZ:
    case SMG$K_TRM_CR:
    case SMG$K_TRM_HT:
      strtfld = 0;
      break;
    case SMG$K_TRM_UP:
      curs = kit_file.entry_screen[kit_file.entry_comment].field;
      if (curs->last == NULL)
        strtfld = 0;
      break;

    }

  memcpy (edtstr, &kitsed_buff, sizeof (kitsed_buff));
  return (ret);
}
