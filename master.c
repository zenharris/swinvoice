
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
#include lib$routines

#include "extools.h"
#include "screen.h"
#include "filebrowse.h"


#include "swfile.h"

#include "colours.h"

extern char user_name[];
extern char priv_user[];

struct master_rec master_master;

/* *INDENT-OFF* */
#define MASSCRNLEN 29
struct scr_params master_entry_screen[] = {
    0,0,"Retail Markup    : %7.2f",PROMPTCOL,NUMERIC,&master_buff.retail_markup,FIELDCOL,7,NULL,NULL,
    1,0,"Wholesale Markup : %7.2f",PROMPTCOL,NUMERIC,&master_buff.wholesale_markup,FIELDCOL,7,NULL,NULL,
    2,0,"Sales Tax A      : %7.2f",PROMPTCOL,NUMERIC,&master_buff.sales_tax[0],FIELDCOL,7,NULL,NULL,
    2,27,"Sales Tax B : %7.2f",PROMPTCOL,NUMERIC,&master_buff.sales_tax[1],FIELDCOL,7,NULL,NULL,
    3,0,"Sales Tax C      : %7.2f",PROMPTCOL,NUMERIC,&master_buff.sales_tax[2],FIELDCOL,7,NULL,NULL,
    3,27,"Sales Tax D : %7.2f",PROMPTCOL,NUMERIC,&master_buff.sales_tax[3],FIELDCOL,7,NULL,NULL,
    4,0,"Sales Tax E      : %7.2f",PROMPTCOL,NUMERIC,&master_buff.sales_tax[4],FIELDCOL,7,NULL,NULL,
    6,0,"Printer Queue : ",PROMPTCOL,UNFMT_STRING,master_buff.print_queue,FIELDCOL,30,NULL,NULL,
    8,0,"Email Details",0,0,NULL,0,0,NULL,NULL,
    9,0,"Subj:",PROMPTCOL,UNFMT_STRING,master_buff.email_subject,FIELDCOL,46,NULL,NULL,
    10,0,"Addr:",PROMPTCOL,UNFMT_STRING,master_buff.email_address,FIELDCOL,46,NULL,NULL,
    12,0,"Postal Address",0,0,NULL,0,0,NULL,NULL,
    13,0,"Name    : ",PROMPTCOL,UNFMT_STRING|CAPITALIZE,master_buff.address1,FIELDCOL,30,NULL,NULL,
    14,0,"Address : ",PROMPTCOL,UNFMT_STRING|CAPITALIZE,master_buff.address2,FIELDCOL,30,NULL,NULL,
    15,0,"        : ",PROMPTCOL,UNFMT_STRING|CAPITALIZE,master_buff.address3,FIELDCOL,30,NULL,NULL,
    16,0,"        : ",PROMPTCOL,UNFMT_STRING|CAPITALIZE,master_buff.address4,FIELDCOL,30,NULL,NULL,
    17,0,"        : ",PROMPTCOL,UNFMT_STRING|CAPITALIZE,master_buff.address5,FIELDCOL,30,NULL,NULL,
    19,0,"Customer Number  : %5hu",PROMPTCOL,NUMERIC,&master_buff.cust_no,FIELDCOL,5,NULL,NULL,
    19,25,"Quote Number     : %5hu",PROMPTCOL,NUMERIC,&master_buff.quote_no,FIELDCOL,5,NULL,NULL,
    20,0,"Job Number       : %5hu",PROMPTCOL,NUMERIC,&master_buff.job_no,FIELDCOL,5,NULL,NULL,
    20,25,"Invoice Number   : %5hu",PROMPTCOL,NUMERIC,&master_buff.invoice_no,FIELDCOL,5,NULL,NULL,
    21,0,"Receipt Number   : %5hu",PROMPTCOL,NUMERIC,&master_buff.receipt_no,FIELDCOL,5,NULL,NULL,
    22,0,"Display Flag Y,R,W or A : ",PROMPTCOL,SINGLE_CHAR|CAPITALIZE,&master_buff.d_flag_w,FIELDCOL,1,NULL,NULL,
    24,0,"Invoice Length   : %5i",PROMPTCOL,NUMERIC,&master_buff.invoice_lnth,FIELDCOL,5,NULL,NULL,
    24,25,"Receipt Length   : %5i",PROMPTCOL,NUMERIC,&master_buff.receipt_lnth,FIELDCOL,5,NULL,NULL,
    25,0,"Labels Per Line  : %5i",PROMPTCOL,NUMERIC,&master_buff.labels_pl,FIELDCOL,5,NULL,NULL,
    25,25,"Label Width      : %5i",PROMPTCOL,NUMERIC,&master_buff.labels_wdth,FIELDCOL,5,NULL,NULL,
    26,0,"Label Length     : %5i",PROMPTCOL,NUMERIC,&master_buff.labels_lnth,FIELDCOL,5,NULL,NULL,
    27,0,"Statement Start Date  :",PROMPTCOL,BTRV_DATE,&master_buff.statement_date,FIELDCOL,10,NULL,NULL
};
/* *INDENT-ON* */

int
lock_read_master ()
{
  uint32 masrec = 1;

  master_file.rab.rab$b_rac = RAB$C_KEY;
  master_file.rab.rab$l_kbf = (char *) &masrec;
  master_file.rab.rab$b_ksz = 4;

  master_file.rab.rab$l_ubf = (char *) &master_buff;
  master_file.rab.rab$w_usz = sizeof (master_buff);
  master_file.rab.rab$l_rop = RAB$M_RLK;

  rms_status = sys$get (&master_file.rab);

  if (!rmstest (rms_status, 2, RMS$_NORMAL, RMS$_RLK)) {
    warn_user ("$GET %s", rmserror(rms_status,&master_file));
    return(FALSE);
  }
  if(rms_status == RMS$_RLK) return (FALSE);
  else return(TRUE);

}

void
read_master_record ()
{
  uint32 masrec = 1;

  master_file.rab.rab$b_rac = RAB$C_KEY;
  master_file.rab.rab$l_kbf = (char *) &masrec;
  master_file.rab.rab$b_ksz = 4;

  master_file.rab.rab$l_ubf = (char *) &master_buff;
  master_file.rab.rab$w_usz = sizeof (master_buff);
  master_file.rab.rab$l_rop = RAB$M_NLK;

  rms_status = sys$get (&master_file.rab);

  if (!rmstest
      (rms_status, 4, RMS$_NORMAL, RMS$_RNF, RMS$_OK_ALK, RMS$_OK_RLK))
    {
      warn_user ("$GET %s",rmserror(rms_status,&master_file));
      return;
    }
  if (rms_status == RMS$_RNF)
    {
      memset (&master_buff, 0, sizeof (master_buff));
      master_buff.cust_no = 1;
      master_buff.quote_no = 1;
      master_buff.job_no = 1;
      master_buff.invoice_no = 1;
      master_buff.receipt_no = 1;
      master_buff.d_flag_w = 'Y';

      master_file.rab.rab$l_rbf = (char *) &master_buff;
      master_file.rab.rab$w_rsz = sizeof (master_buff);
      rms_status = sys$put (&master_file.rab);
      if (!rmstest (rms_status, 1, RMS$_NORMAL))
        warn_user ("$PUT %s",rmserror(rms_status,&master_file));
    }
}



void
write_master_record ()
{
  uint32 masrec = 1;

  master_file.rab.rab$b_rac = RAB$C_KEY;
  master_file.rab.rab$l_kbf = (char *) &masrec;
  master_file.rab.rab$b_ksz = 4;
  master_file.rab.rab$l_rop = RAB$M_RLK;

  rms_status = sys$find (&master_file.rab);

  if (!rmstest (rms_status, 3, RMS$_NORMAL, RMS$_OK_ALK, RMS$_RLK))
    {
      warn_user ("$GET %s", rmserror(rms_status,&master_file));
      return;
    }
  if (rms_status == RMS$_RLK)
    {
      warn_user ("%s", strerror (EVMSERR, rms_status));
      return;
    }

  master_file.rab.rab$l_rbf = (char *) &master_buff;
  master_file.rab.rab$w_rsz = sizeof (master_buff);

  master_file.rab.rab$l_rop = RAB$V_UIF;

  rms_status = sys$update (&master_file.rab);
  if (!rmstest (rms_status, 1, RMS$_NORMAL))
    warn_user ("$PUT %s", rmserror(rms_status,&master_file));

}

struct cmndline_params master_cmndline[] = {
  1, NULL, NULL, 0, NULL,
  F1, "Edit Recd", NULL, 0, NULL,
};


void
edit_master_record (struct scrllst_msg_rec *msg, struct filestruct *filedef)
{
  int edited = FALSE, exit = FALSE;
  uint16 scan = 0;
  struct linedt_msg_rec mesg;
  int strtfld = 0;

  read_master_record ();        /* no lock yet */

  open_window (3, 19, 52, 29, WHITE, border);
/*    init_screen (master_entry_screen,MASSCRNLEN);  */

  while (scan != SMG$K_TRM_CTRLZ)
    {
      init_screen (master_entry_screen, MASSCRNLEN);
      command_line (master_cmndline, &scan, &mesg, filedef);
      if (scan != SMG$K_TRM_CTRLZ)
        {
          if (scan == SMG$K_TRM_PF1)
            {

              if (strcmp (user_name, priv_user) != 0)
                {
                  warn_user ("Not Authorised to Edit Master");
                  close_window ();
                  return;
                }

              if(!lock_read_master ()) {      /*lock master record */
                  warn_user ("Lock Master Failed");
                  close_window ();
                  return;
              }
              init_screen (master_entry_screen, MASSCRNLEN);
              if (rmstest (rms_status, 1, RMS$_RLK))
                {
                  warn_user ("%s", strerror (EVMSERR, rms_status));
                }
              else
                scan =
                  enter_screen (master_entry_screen, MASSCRNLEN, &edited,
                                nobreakout, NULL, &strtfld);
            }
        }
    }
  if (edited)
    {
      while (!exit)
        {
          switch (warn_user ("Save Changes :  Y/N"))
            {
            case 'y':
            case 'Y':
              memcpy (&master_master, &master_buff, sizeof (master_buff));
              write_master_record ();
              exit = TRUE;
              break;
            case 'n':
            case 'N':
              exit = TRUE;
              break;
            }
        }
    }
  sys$free (&master_file.rab);
  close_window ();

}
