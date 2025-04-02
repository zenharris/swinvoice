
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include smgdef
#include smg$routines
#include ints
#include ctype
#include rms
#include starlet
#include errno

#include "extools.h"
#include "screen.h"
#include "filebrowse.h"
#include "warns.h"

#include "swfile.h"

#include "colours.h"

#define HEADING "Code          Kit Description             "

#define VHEADING "Code      |Description                 |Units|Number |Extended Description   "

void strupr (char *);

extern int TermWidth;
extern int win_depth;
extern int BottomLine;

void *lmalloc (size_t);

#define KITBUFFSIZE 10000

static int new = FALSE;


void print_kit_A ();
void print_kit_B ();

static struct cmndline_params edit_cmndline[] = {
  3, NULL, NULL, 0, NULL,
  F1, "Edit Recd.", NULL, 0, Edit_Record_NW,
/* 	F5,"Print Kit",NULL,0,print_kit_B,*/
  F9, "Previous", NULL, 0, NULL,
  F10, "Next", NULL, 0, NULL
};


void kit_search ();

void
kitsrch (msg, filedef)
     struct linedt_msg_rec *msg;
     struct filestruct *filedef;
{
  kit_search (msg, filedef, 0);
}

static struct cmndline_params index_cmndline[] = {
  2, NULL, NULL, 0, NULL,
  F1, "New Kit", NULL, 0, Insrt_Record,
  F2, "Search", NULL, 0, kitsrch
/*	F5,"Print Kit",NULL,0,print_kit_A */
};

extern int check_code (char *);
/*
static int check_code(char *test)
{
   unsigned int lnth,status;
   struct kit_rec kit_buff;
   struct kit_key0_rec kit_key;
   struct kit_rec *dtr = (struct kit_rec *)kit_file.fileio;

   strupr(test);
   strcpy (kit_key.kit_code,test);
   pad(kit_key.kit_code,PARTKEYLEN);

   kit_file.rab.rab$b_krf = KIT_PART_KEY;
   kit_file.rab.rab$l_kbf = (char *)&kit_key;
   kit_file.rab.rab$b_ksz = sizeof(kit_key);

   kit_file.rab.rab$l_ubf = (char *)kit_file.fileio;
   kit_file.rab.rab$w_usz = kit_file.fileiosz;
                                    
   kit_file.rab.rab$b_rac = RAB$C_KEY;
   kit_file.rab.rab$l_rop = RAB$M_NLK|RAB$V_CDK;

   rms_status = sys$get(&kit_file.rab);

   if (!rmstest(rms_status,3,RMS$_NORMAL,RMS$_RNF,RMS$_OK_ALK)){ 
      warn_user("$GET %s", rmserror(rms_status,&kit_file));
      return(TRUE);
   } 
   
   
   if (rmstest(rms_status,2,RMS$_NORMAL,RMS$_OK_ALK)) {
      warn_user("That Code is already assigned");
      return(FALSE);  
   }

   return(TRUE);
}
*/

int kitsed ();
/* *INDENT-OFF* */
#define KITSCRNLEN 5
#define KITTEXTLN  3	  /*  Change in report17.c	as well as here */
struct scr_params kit_entry_screen[] = {
    1,0,"Kit Code    :",PROMPTCOL,UNFMT_STRING,kit_buff.kit_code,FIELDCOL,10,check_code,NULL,
    2,0,"Description :",PROMPTCOL,UNFMT_STRING|CAPITALIZE,kit_buff.description,FIELDCOL,28,NULL,NULL,
    3,0,"Price Offset:$ %7.2f",PROMPTCOL,NUMERIC,&kit_buff.price_offset,FIELDCOL,7,NULL,NULL,
    6,18,VHEADING,HEADCOL,FREE_TEXT,NULL,FIELDCOL,100,kitsed,NULL,
    0,0,"Kit Definition Record",HEADCOL,0,NULL,0,0,NULL,NULL
};
/* *INDENT-ON* */


void
kit_trans (crnt, filedef)
     struct List_Element *crnt;
     struct filestruct *filedef;
{
  struct kit_rec *dtr = (struct kit_rec *) filedef->fileio;
  sprintf (crnt->Prompt, "%.*s %-30s",
           PARTKEYLEN, dtr->kit_code, dtr->description);
}


void
kit_search (msg, filedef, frstchr)
     struct linedt_msg_rec *msg;
     struct filestruct *filedef;
     char frstchr;
{
  struct kit_key0_rec srch_key;
  char srchln[15];
  int wch, lnpos;
  unsigned int cont = 0, status;
  struct rfabuff rfa;

  open_window (8, 24, 32, 3, WHITE, border);
  exscrprn ("Partial Code", 0, 9, WHITE);

  strncpy (srchln, &frstchr, 1);
  lnpos = 1;
  wch = linedt (srchln, 1, 0, 10, WHITE, &lnpos, &cont, NULL,filedef);
  close_window ();
  strupr (srchln);
  exloccur (24, 0);
  if (wch != SMG$K_TRM_CTRLZ)
    {
      strcpy (srch_key.kit_code, srchln);
      strupr (srch_key.kit_code);
      pad (srch_key.kit_code, PARTKEYLEN);

      kit_file.rab.rab$b_krf = KIT_PART_KEY;
      kit_file.rab.rab$l_kbf = (char *) &srch_key;
      kit_file.rab.rab$b_ksz = sizeof (srch_key);
      kit_file.rab.rab$b_rac = RAB$C_KEY;
      kit_file.rab.rab$l_rop = RAB$M_NLK | RAB$M_EQNXT;

      rms_status = sys$find (&kit_file.rab);

      if (!rmstest (rms_status, 3, RMS$_NORMAL, RMS$_RNF, RMS$_OK_RLK))
        {
          warn_user ("$FIND %s", rmserror(rms_status,&kit_file));
          return;
        }

      if (rms_status == RMS$_RNF)
        warn_user ("Record Not Found");
      else
        {
          rfa_copy (&rfa, (struct rfabuff *) &filedef->rab.rab$w_rfa);
          ReadMiddleOut (&rfa, filedef);
        }

    }
}


void
kit_defaults ()
{
  memset (&kit_buff, 0, sizeof (kit_buff));
  new = TRUE;
}

void
Set_Kit_List_Key (void)
{
  struct kit_rec *dtr = (struct kit_rec *) kit_file.fileio;

  memcpy (kit_listkey.kit_code, dtr->kit_code, PARTKEYLEN);

}


void
kit_pad_record (void)
{
  struct kit_rec *dtr = (struct kit_rec *) kit_file.fileio;
  pad (dtr->kit_code, PARTKEYLEN);
}

void
kit_unpad_record (void)
{
  struct kit_rec *dtr = (struct kit_rec *) kit_file.fileio;
  unpad (dtr->kit_code, PARTKEYLEN);
}



int
count_lines (text_start)
     struct line_rec **text_start;
{
  struct line_rec *temp;
  int count = 0;
  if ((*text_start) != NULL)
    {
      while ((*text_start)->last != NULL)
        (*text_start) = (*text_start)->last;
      for (temp = (*text_start); temp != NULL; count++)
        temp = temp->next;
    }
  return (count);
}

void
disassemble (block, scrl_consts, v_lnth, v_ptr, reclen)
     char *block;
     struct filestruct *scrl_consts;
     int v_lnth;
     struct line_rec **v_ptr;
     uint16 reclen;
{
  struct line_rec *crnt_list = NULL, *temp_list = NULL, *first_line = NULL;
  unsigned int main_lnth, status;

  main_lnth = scrl_consts->recsize;

  if (block != NULL)
    {
      memcpy (scrl_consts->recbuff, block, scrl_consts->recsize);
      while (main_lnth < reclen)
        {
          if ((crnt_list = lmalloc (v_lnth)) != NULL)
            {
              memcpy (crnt_list->storage, block + main_lnth, v_lnth);
              main_lnth += v_lnth;

              if ((*v_ptr) == NULL)
                {
                  first_line = (*v_ptr) = crnt_list;
                }
              else
                {
                  (*v_ptr)->next = crnt_list;
                  crnt_list->last = (*v_ptr);
                  (*v_ptr) = crnt_list;
                }
            }
          else
            {
              printf ("memory allocaton error %s %u\n", filterfn (__FILE__),
                      __LINE__);
              exit;
            }
        }
      (*v_ptr) = first_line;
    }
}


int
assemble (block, scrl_consts, v_lnth, v_ptr)
     char **block;
     struct filestruct *scrl_consts;
     int v_lnth;
     struct line_rec **v_ptr;
{
  struct line_rec *crnt_line;
  unsigned int mainsz, textsz, offset = 0, counter = 0;

  mainsz = scrl_consts->recsize;
  textsz = count_lines (v_ptr);
  crnt_line = (*v_ptr);
  textsz *= v_lnth;
  memcpy ((*block), scrl_consts->fileio, mainsz);
  while (crnt_line != NULL)
    {
      memcpy (((*block) + mainsz + offset), crnt_line->storage, v_lnth);
      offset += v_lnth;
      crnt_line = crnt_line->next;
      counter++;
    }

  return (counter);
}

/* Copy line editor buffer into Variable section of record after editing */
int
kit_unload_editor_buffer (curs, filedef)
     struct line_rec *curs;
     struct filestruct *filedef;
{
  int lines = 0;
  struct kit_rec *dtr = (struct kit_rec *) filedef->fileio;

  while (curs != NULL && curs->last != NULL)
    curs = curs->last;

  lines =
    assemble (&filedef->fileio, filedef, sizeof (struct kit_part_rec), &curs);

  dtr->reclen = sizeof (kit_buff) + (lines * sizeof (struct kit_part_rec));

  return (dtr->reclen);
}

void
kit_load_editor_buffer (ptr, filedef)
     struct line_rec **ptr;
     struct filestruct *filedef;
{
  struct kit_rec *dtr = (struct kit_rec *) filedef->fileio;

  disassemble (filedef->fileio, filedef, sizeof (struct kit_part_rec), ptr,
               dtr->reclen);
  while ((*ptr) != NULL && (*ptr)->last != NULL)
    (*ptr) = (*ptr)->last;

}




void
kit_filedef_init (void)
{

  if ((kit_diskio_buff = malloc (KITBUFFSIZE)) == NULL)
    {
      printf (" Could Not Allocate disk buffer memory\n");
      exit;
    }


  kit_file.fileio = (char *) kit_diskio_buff;
  kit_file.fileiosz = KITBUFFSIZE;

  kit_file.recbuff = (char *) &kit_buff;
  kit_file.recsize = sizeof kit_buff;


  kit_file.setprompt = kit_trans;

  kit_file.entry_screen = kit_entry_screen;
  kit_file.entry_length = KITSCRNLEN;
  kit_file.entry_comment = KITTEXTLN;

  kit_file.mainsrch = kit_search;

  kit_file.read_cmndline = edit_cmndline;
  kit_file.list_cmndline = index_cmndline;

  kit_file.listkey = (char *) &kit_listkey;
  kit_file.listkeysz = sizeof kit_listkey;
  kit_file.fwrdkeyid = KIT_FORWARD_KEY;
  kit_file.revrkeyid = KIT_REVERSE_KEY;
  kit_file.setlistkey = Set_Kit_List_Key;
  kit_file.padrecord = kit_pad_record;
  kit_file.unpadrecord = kit_unpad_record;

  kit_file.loadvarbuff = kit_load_editor_buffer;
  kit_file.dumpvarbuff = kit_unload_editor_buffer;

}

void
fork_kit_file (struct scrllst_msg_rec *msg)
{
  unsigned int RegionLength;
  smg$get_display_attr (&crnt_window->display_id, &RegionLength);

  spawn_scroll_window (&kit_file, RegionLength-3, 44);

  HeadLine ("%s", left, HEADING);

  File_Browse (&kit_file, normal);

  close_window ();
  win_depth--;


}
