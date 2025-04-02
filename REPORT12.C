/* Test Inclusion */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <ssdef.h>

#include <ints>
#include <rms>
#include smgdef
#include smg$routines
#include starlet
#include descrip
#include lib$routines
#include ctype
#include math

#include "extools.h"
#include "filebrowse.h"
#include "warns.h"
#include "colours.h"


#include "swfile.h"
#include "swoptions.h"
#include "transaction.h"
#include "procinvc.h"


#define SIZE_TIMESTR 23


static int reprint = FALSE;
char postal,emailinv;
static char filenm[64];
static float receipt_amnt = 0.0;
static uint32 receipt_no;
static char being_for[64],email_subject[64];
static struct primary_rec client_buff;

extern struct filestruct PrimFile, quote_file, master_file;

int in_ballance;

int execute_DCL (char *,unsigned,unsigned int *); 
int send_invoice (char *,unsigned,unsigned int *,char *,char *);

FILE *stdprn;

extern int lock_read_master ();
extern void write_master_record ();

extern int errno;

extern int edit_flag;


static void
heading (funct)
     enum funct_type funct;
{
  struct timebuff adjtime;
  int iter,pageno = 1;
  char statepcode[64];
  float blnc;
  get_time (&adjtime);

  switch (funct)
    {
    case header:

      for (iter = 0; iter < 4; iter++)
        fprintf (stdprn, "\n");

      fprintf (stdprn, "Receipt Number : %5.5u       For Client : %5.5u\n",receipt_no,client_buff.joic_no);

      fprintf (stdprn, "Receipt Date   : %s    ",
               print_date (&transaction_buff.date));
      fprintf (stdprn, "%s : %s\n", (reprint) ? "RePrint" : "Printed",
               print_time (&adjtime));


      fprintf(stdprn,"Page %u\n",pageno++);
      fprintf (stdprn, "\n");
/*      fprintf (stdprn, "%8s%-30s%10s%-30s\n","","To :","","From :"); */
      if(postal == 'Y') {
         fprintf (stdprn, "%8s%-30s%10s%-30s\n","To : ", client_buff.name,"From : ",master_buff.address1);
         fprintf (stdprn, "%8s%-30s%10s%-30s\n","", client_buff.p_address1,"",master_buff.address2);
         fprintf (stdprn, "%8s%-30s%10s%-30s\n","", client_buff.p_address2,"",master_buff.address3);
         fprintf (stdprn, "%8s%-30s%10s%-30s\n","", client_buff.p_address3,"",master_buff.address4);
         fprintf (stdprn, "%8s%-30s%10s%-30s\n","", client_buff.p_address4,"",master_buff.address5);
         fprintf (stdprn, "%8s%-30s%10s%-30s\n","Email : ", client_buff.email,"Email : ",master_buff.email_address);

      } else {
         sprintf (statepcode,"%s, %s",client_buff.state, client_buff.postcode);
         fprintf (stdprn, "%8s%-30s%10s%-30s\n","To : ",client_buff.name,"From : ",master_buff.address1);
         fprintf (stdprn, "%8s%-30s%10s%-30s\n","",client_buff.address,"",master_buff.address2);
         fprintf (stdprn, "%8s%-30s%10s%-30s\n","",client_buff.suburb,"",master_buff.address3);
         fprintf (stdprn, "%8s%-30s%10s%-30s\n","",statepcode,"",master_buff.address4);
         fprintf (stdprn, "%8s%-30s%10s%-30s\n","",client_buff.country,"",master_buff.address5);
         fprintf (stdprn, "%8s%-30s%10s%-30s\n","Email : ", client_buff.email,"Email : ",master_buff.email_address);

      }
      fprintf (stdprn, "\n");





      break;
    case cancel:
      fprintf (stdprn, "*****      REPORT  TERMINATED BY  USER  at %s",
               print_time (&adjtime));
      break;
    case ende:

      blnc = ballance(client_buff.joic_no);
      fprintf (stdprn,"Balance Owing $%10.2f %s\n", fabs(blnc), drcr(blnc));
                                   
      fprintf (stdprn, "\n");

/*      fputc (EJECT, stdprn); */
      break;
    }
}


#define check_status(syscall) \
    if (status != SS$_NORMAL || (status = iosb.status) != SS$_NORMAL) { \
        fprintf (stderr, "Failed to %s the transaction\n", syscall); \
        fprintf (stderr, "Error was: %s", strerror (EVMSERR, status)); \
        exit (EXIT_FAILURE); \
    }

static int check_YN (char *test)
{
  *test = toupper (*test);
  switch (*test)
    {
    case 'Y':
    case 'N':
      return (TRUE);
    default:
      warn_user ("Must Be   Y or N");
      return (FALSE);
    }
}





/* *INDENT-OFF* */
#define RCPTSCRNLEN 4
struct scr_params receipt_entry_screen[] = {
/*    1,0,"Filename : ",PROMPTCOL,UNFMT_STRING,filenm,FIELDCOL,38,NULL,NULL,*/
    2,0,"Payment  Amount   : %10.2f ",PROMPTCOL,NUMERIC,&receipt_amnt,FIELDCOL,10,NULL,NULL,
    3,0,"Being For: ",PROMPTCOL,UNFMT_STRING|CAPITALIZE,being_for,FIELDCOL,38,NULL,NULL,
    4,0,"Use Postal Address: ",PROMPTCOL,SINGLE_CHAR|CAPITALIZE,&postal,FIELDCOL,1,check_YN,NULL,
    5,0,"Email Receipt     : ",PROMPTCOL,SINGLE_CHAR|CAPITALIZE,&emailinv,FIELDCOL,1,check_YN,NULL,

};
/* *INDENT-ON* */

static int confirm()
{
   char wch;
   while((wch = toupper(warn_user ("Print Receipt Y/N"))) != 'Y' && wch != 'N');
   return (wch=='Y');
}



#define RCPT_INPUT_LENGTH 7

static int topline  ()
{

   if(edit_flag) return(3);

   if(in_ballance) {
      if(crnt_window->rul+RCPT_INPUT_LENGTH  > PrimFile.RegionLength) return (-RCPT_INPUT_LENGTH);
      else  return(3);
   }

   if ((PrimFile.CurrentLine+3+RCPT_INPUT_LENGTH) > PrimFile.RegionLength)
      return(PrimFile.CurrentLine-RCPT_INPUT_LENGTH);
   else
      return(PrimFile.CurrentLine+3);
}

void delete_latest_transaction(struct rfabuff *transaction)
{
      transaction_file.rab.rab$b_rac = RAB$C_RFA;
      transaction_file.rab.rab$l_rop = RAB$M_RLK;
      rfa_copy(&transaction_file.rab.rab$w_rfa,transaction);

      rms_status = sys$find(&transaction_file.rab);
      if (!rmstest (rms_status, 1, RMS$_NORMAL))
         warn_user("$FIND %s",rmserror(rms_status,&transaction_file));

      rms_status = sys$delete(&transaction_file.rab);
      if (!rmstest (rms_status, 1, RMS$_NORMAL))
         warn_user("$DEL %s",rmserror(rms_status,&transaction_file));

}


void
print_receipt (struct scrllst_msg_rec *msg, struct filestruct *filedef)
{
  int status, lnth, receipt_lnth, linecount;
  unsigned iter;
   
  int wch, edited = TRUE,startfld=0;
  char mailcmd[256];
  $DESCRIPTOR(amailcmd,mailcmd);
  unsigned int diagwin;
  unsigned int mailret;
  struct rfabuff transaction_rfa;
  float blnc;

  if (PrimFile.CurrentPtr == NULL) return;


  open_window (crnt_window->rul+topline(),
               crnt_window->cul+5, 50,
               RCPT_INPUT_LENGTH, WHITE, border);

  HeadLine("Print Receipt",centre);

  *being_for = 0;
  receipt_amnt = 0.0;
  postal = 'N';
  emailinv = 'N';
  strcpy(being_for ,"Account Payment");

  init_screen(receipt_entry_screen,RCPTSCRNLEN);
  do {
     wch = enter_screen (receipt_entry_screen, RCPTSCRNLEN, &edited,
                            breakout, NULL, &startfld);
     if(wch == SMG$K_TRM_UP && startfld > 0) startfld--;
     else if(wch == SMG$K_TRM_DOWN && startfld < RCPTSCRNLEN-1) startfld++;
  } while (wch != SMG$K_TRM_CTRLZ && wch != SMG$K_TRM_CR);
  close_window ();
  if (wch == SMG$K_TRM_CTRLZ || !confirm())
    {
       /*   warn_user ("Cancelling Receipt"); */
      return;
    }


  PrimFile.rab.rab$b_rac = RAB$C_RFA;
  rfa_copy (&PrimFile.rab.rab$w_rfa, &PrimFile.CurrentPtr->rfa);

  PrimFile.rab.rab$l_ubf = (char *) PrimFile.fileio;        /*record; */
  PrimFile.rab.rab$w_usz = PrimFile.fileiosz;
  PrimFile.rab.rab$l_rop = RAB$M_NLK;

  rms_status = sys$get (&PrimFile.rab);

  if (!rmstest (rms_status, 2, RMS$_NORMAL, RMS$_OK_RLK))
    {
      warn_user ("$GET %s", rmserror(rms_status,&quote_file));
      return;
    }


  memcpy (&client_buff, PrimFile.fileio, sizeof (client_buff));

  if(!(receipt_no=rcpt_allocate()))
    {
      warn_user ("Locked Master %u",receipt_no);
      return;
    }

#ifdef TRANSACTION
   status = sys$start_transw (event_flag, 0, &iosb, 0, 0, tid);
   check_status ("start");
#endif


  sprintf(filenm,"rcpt%5.5u.txt",receipt_no);
  
  if (!open_stdprn (filenm,"w")) {
#ifdef TRANSACTION
    status = sys$abort_transw (event_flag, 0, &iosb, 0, 0, tid);
    check_status ("abort");
#endif
    return;
  }

  heading (header);

  linecount = 0;

  blnc = ballance(client_buff.joic_no);
  fprintf (stdprn,"Prev Balance  $%10.2f %s\n", fabs(blnc), drcr(blnc));
  fprintf (stdprn,"Received      $%10.2f being for %s\n",receipt_amnt,being_for);


   if (!write_transaction(credit,0,client_buff.joic_no,receipt_no,receipt_amnt)) {
      warn_user ("$PUT %s", rmserror(rms_status,&transaction_file));
      fclose (stdprn);
      delete_file(filenm);
#ifdef TRANSACTION
      status = sys$abort_transw (event_flag, 0, &iosb, 0, 0, tid);
      check_status ("abort");
#endif
      return;
   }

   rfa_copy(&transaction_rfa,&transaction_file.rab.rab$w_rfa);
  
  heading (ende);

  fclose (stdprn);

  if(emailinv == 'Y' && strlen(client_buff.email) ==  0) {
      warn_user("No Email Address");
      delete_file(filenm);
#ifdef TRANSACTION
      status = sys$abort_transw (event_flag, 0, &iosb, 0, 0, tid);
      check_status ("abort");
#else 
      delete_latest_transaction(&transaction_rfa);
#endif
      return; 
  }


#ifdef DIAGS 
  open_window (8, 3, 100, 15, WHITE, border);
  diagwin = crnt_window->display_id;
#else 
  diagwin = Standard_Window;
#endif

   send_invoice(filenm,diagwin,&mailret,client_buff.email,"Receipt");

#ifdef DIAGS
   sprintf(mailcmd,"%s returns %s %s\n\rAny Key to Continue",(emailinv=='Y')?"mail":"print",xlat(mailret) ,strerror(EVMSERR,mailret));
   amailcmd.dsc$w_length = strlen (mailcmd);
   smg$put_line (&diagwin,&amailcmd);
   Refresh (diagwin);
   Get_Keystroke();
   close_window();
#endif
    
   if(mailret != SS$_NORMAL) {
      warn_user ("%s Failed %s %s",(emailinv=='Y')?"Mailer":"Print",xlat(mailret),strerror(EVMSERR,mailret));
      delete_file(filenm);
#ifdef TRANSACTION
      status = sys$abort_transw (event_flag, 0, &iosb, 0, 0, tid);
      check_status ("abort");
#else
      delete_latest_transaction(&transaction_rfa);
#endif
      return;
   }

   
#ifdef TRANSACTION
  status = sys$end_transw (event_flag, 0, &iosb, 0, 0, tid);
  check_status ("end");
#endif

}
