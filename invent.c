
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
#include "warns.h"
#include "filebrowse.h"
#include "swfile.h"


extern int TermWidth;
extern int win_depth;
extern int BottomLine;


#include "colours.h"


void strupr (char *);

#define INVHEADING "Code          Description                    Units     Price "


/*
extern struct inventory_rec inventory_buff;
struct inventory_key0_rec inventory_key_buff;
struct filestruct inventory_file;
*/
int inventory_lnth, price_flag = TRUE;

static int new = FALSE;


struct cmndline_params edit_cmndline[] = {
  3, NULL, NULL, 0, NULL,
  F1, "Edit Recd.", NULL, 0, Edit_Record_NW,
  F9, "Previous", NULL, 0, NULL,
  F10, "Next", NULL, 0, NULL
};

void price_on ();
void price_off ();
struct menu_params invent_mask_menu[] = {
  "Cost Price On", price_on,
  "Cost Price Off", price_off
};

void invent_search ();
void gl_update_prices_material ();

void
inventsrch (msg, filedef)
     struct linedt_msg_rec *msg;
     struct filestruct *filedef;
{
  invent_search (msg, filedef, 0);
}

struct cmndline_params index_cmndline[] = {
  4, NULL, NULL, 0, NULL,
  F1, "New Item", NULL, 0, Insrt_Record,
  F2, "Search", NULL, 0, inventsrch,
  F3, "Cost Price", invent_mask_menu, 2, NULL,
  F9, "Update Costs", NULL, 0, NULL /*gl_update_prices_material */ ,
  F5, "Process.", NULL, 0, NULL
};

int
check_code (char *test)
{
  unsigned int lnth, status;
  struct inventory_key0_rec inventory_key;
  struct kit_key0_rec kit_key;
  char assigned[] = "Code is already used.";

  strupr (test);
  strcpy (inventory_key.part_code, test);
  pad (inventory_key.part_code, PARTKEYLEN);

  inventory_file.rab.rab$b_krf = INV_PART_KEY;
  inventory_file.rab.rab$l_kbf = (char *) &inventory_key;
  inventory_file.rab.rab$b_ksz = sizeof (inventory_key);
  inventory_file.rab.rab$b_rac = RAB$C_KEY;
  inventory_file.rab.rab$l_rop = RAB$M_NLK | RAB$V_CDK;

  rms_status = sys$find (&inventory_file.rab);

  if (!rmstest (rms_status, 3, RMS$_NORMAL, RMS$_RNF, RMS$_OK_RLK))
    {
      warn_user ("$FIND %s", rmserror(rms_status,&inventory_file));
      return (TRUE);
    }
  if (rmstest (rms_status, 2, RMS$_NORMAL, RMS$_OK_RLK))
    {
      warn_user (assigned);
      return (FALSE);
    }

  strcpy (kit_key.kit_code, test);
  pad (kit_key.kit_code, PARTKEYLEN);

  kit_file.rab.rab$b_krf = KIT_PART_KEY;
  kit_file.rab.rab$l_kbf = (char *) &kit_key;
  kit_file.rab.rab$b_ksz = sizeof (kit_key);
  kit_file.rab.rab$b_rac = RAB$C_KEY;
  kit_file.rab.rab$l_rop = RAB$M_NLK | RAB$V_CDK;

  rms_status = sys$find (&kit_file.rab);

  if (!rmstest (rms_status, 3, RMS$_NORMAL, RMS$_RNF, RMS$_OK_RLK))
    {
      warn_user ("$FIND %s", rmserror(rms_status,&kit_file));
      return (TRUE);
    }
  if (rmstest (rms_status, 2, RMS$_NORMAL, RMS$_OK_RLK))
    {
      warn_user (assigned);
      return (FALSE);
    }

  return (TRUE);
}

capital (char *in_str)
{
  strupr (in_str);
  return (TRUE);
}

/* *INDENT-OFF* */

#define INVSCRNLEN 15
struct scr_params inventory_entry_screen[] = {
    1,0,"Part Code    :",PROMPTCOL,UNFMT_STRING,inventory_buff.part_code,FIELDCOL,10,check_code,NULL,
    2,0,"Part Number  :",PROMPTCOL,UNFMT_STRING,inventory_buff.part_no,FIELDCOL,15,capital,NULL,
    3,0,"Description  :",PROMPTCOL,UNFMT_STRING|CAPITALIZE,inventory_buff.description,FIELDCOL,28,NULL,NULL,
    4,0,"Units        :",PROMPTCOL,UNFMT_STRING,inventory_buff.units,FIELDCOL,5,NULL,NULL,
    5,0,"Price / Unit : %10.2f",PROMPTCOL,NUMERIC,&inventory_buff.price,FIELDCOL,10,NULL,NULL,
    6,0,"Update Class : ",PROMPTCOL,UNFMT_STRING,inventory_buff.class,FIELDCOL,CLASSKEYLEN,capital,NULL,

    8,0,"Purchase Date  :",PROMPTCOL,BTRV_DATE,&inventory_buff.purchase_dt1,FIELDCOL,10,NULL,NULL,
    9,0,"From Company   :",PROMPTCOL,UNFMT_STRING|CAPITALIZE,inventory_buff.company1,FIELDCOL,50,NULL,NULL,
   10,0,"Sales Person   :",PROMPTCOL,UNFMT_STRING|CAPITALIZE,inventory_buff.salesman1,FIELDCOL,30,NULL,NULL,
   11,0,"Purchase Price : %10.2f",PROMPTCOL,NUMERIC,&inventory_buff.price1,FIELDCOL,10,NULL,NULL,

   13,0,"Purchase Date  :",PROMPTCOL,BTRV_DATE,&inventory_buff.purchase_dt2,FIELDCOL,10,NULL,NULL,
   14,0,"From Company   :",PROMPTCOL,UNFMT_STRING|CAPITALIZE,inventory_buff.company2,FIELDCOL,50,NULL,NULL,
   15,0,"Sales Person   :",PROMPTCOL,UNFMT_STRING|CAPITALIZE,inventory_buff.salesman2,FIELDCOL,30,NULL,NULL,
   16,0,"Purchase Price : %10.2f",PROMPTCOL,NUMERIC,&inventory_buff.price2,FIELDCOL,10,NULL,NULL,
   0,0,"Inventory Item Definition Record",HEADCOL,0,NULL,0,0,NULL,NULL
};

/* *INDENT-ON* */

void
inventory_trans (crnt, filedef)
     struct List_Element *crnt;
     struct filestruct *filedef;
{
  struct inventory_rec *dtr = (struct inventory_rec *) filedef->fileio;

  if (price_flag)
    sprintf (crnt->Prompt, "%.*s %-30s %-5s %10.2f",
             PARTKEYLEN, dtr->part_code,
             dtr->description, dtr->units, dtr->price);
  else
    sprintf (crnt->Prompt, "%.*s %-30s %-5s %10s",
             PARTKEYLEN, dtr->part_code, dtr->description, dtr->units, "");
}




void
price_on (struct scrllst_msg_rec *msg)
{
  price_flag = TRUE;
}

void
price_off (struct scrllst_msg_rec *msg)
{
  price_flag = FALSE;
}

void
invent_search (msg, filedef, frstchr)
     struct linedt_msg_rec *msg;
     struct filestruct *filedef;
     char frstchr;
{
  struct inventory_key0_rec srch_key;
  char srchln[15];
  int wch, lnpos;
  unsigned int cont = 0, status;
  struct rfabuff rfa;


  open_window (8, 24, 32, 3, WHITE, border);
  exscrprn ("Partial Code", 0, 9, WHITE);
  exscrprn ("^Z to Cancel", crnt_window->lnth, 2, WHITE);
  strncpy (srchln, &frstchr, 1);
  lnpos = 1;
  wch = linedt (srchln, 1, 0, 10, WHITE, &lnpos, &cont, NULL,filedef);
  close_window ();
  strupr (srchln);

  exloccur (24, 0);
  if (wch != SMG$K_TRM_CTRLZ)
    {                           /*scan code for ESC  */
      strcpy (srch_key.part_code, srchln);
      pad (srch_key.part_code, PARTKEYLEN);

      inventory_file.rab.rab$b_krf = INV_PART_KEY;
      inventory_file.rab.rab$l_kbf = (char *) &srch_key;
      inventory_file.rab.rab$b_ksz = sizeof (srch_key);
      inventory_file.rab.rab$b_rac = RAB$C_KEY;
      inventory_file.rab.rab$l_ubf = (char *) &inventory_buff;  /*record; */
      inventory_file.rab.rab$w_usz = sizeof (inventory_buff);
      inventory_file.rab.rab$l_rop = RAB$M_NLK | RAB$M_EQNXT;

      rms_status = sys$find (&inventory_file.rab);

      if (!rmstest
          (rms_status, 4, RMS$_NORMAL, RMS$_RNF, RMS$_RLK, RMS$_OK_RLK))
        {
          warn_user ("$FIND %s", rmserror(rms_status,&inventory_file));
          return;
        }
      else if (rmstest (rms_status, 2, RMS$_RNF, RMS$_RLK))
        {
          warn_user ("%s", strerror (EVMSERR, rms_status));
          return;
        }

      if (rms_status == RMS$_RNF)
        warn_user ("%s", strerror (EVMSERR, rms_status));
      else
        {
          rfa_copy (&rfa, (struct rfabuff *) &filedef->rab.rab$w_rfa);
          ReadMiddleOut (&rfa, filedef);
        }

/*      *msg->pos_chngd = TRUE;*/
    }
/*	msg->breaker = TRUE; */
}


void
inventory_defaults ()
{
  memset (&inventory_buff, 0, sizeof (inventory_buff));
  new = TRUE;
}

void
Set_Inventory_List_Key (void)
{

  memcpy (inventory_listkey.part_code, inventory_buff.part_code, PARTKEYLEN);

}

void
inventory_pad_record (void)
{

  pad (inventory_buff.part_code, PARTKEYLEN);
}

void
inventory_unpad_record (void)
{

  unpad (inventory_buff.part_code, PARTKEYLEN);
}

void
inventory_filedef_init (void)
{


  inventory_file.fileio = (char *) &inventory_buff;
  inventory_file.fileiosz = sizeof inventory_buff;

  inventory_file.setprompt = inventory_trans;
  inventory_file.entry_screen = inventory_entry_screen;
  inventory_file.entry_length = INVSCRNLEN;

  inventory_file.mainsrch = invent_search;

  inventory_file.read_cmndline = edit_cmndline;
  inventory_file.list_cmndline = index_cmndline;
  inventory_file.listkey = (char *) &inventory_listkey;
  inventory_file.listkeysz = sizeof inventory_listkey;
  inventory_file.fwrdkeyid = INV_FORWARD_KEY;
  inventory_file.revrkeyid = INV_REVERSE_KEY;
  inventory_file.setlistkey = Set_Inventory_List_Key;
  inventory_file.padrecord = inventory_pad_record;
  inventory_file.unpadrecord = inventory_unpad_record;


}

void
fork_inventory_file (struct scrllst_msg_rec *msg)
{
  unsigned int RegionLength;
  smg$get_display_attr (&crnt_window->display_id, &RegionLength);
  spawn_scroll_window (&inventory_file, RegionLength-3, 63);

  HeadLine (INVHEADING, left);

  File_Browse (&inventory_file, normal);

  close_window ();
  win_depth--;

  msg->breaker = TRUE;



}
