

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

#include "extools.h"
#include "filebrowse.h"
#include "warns.h"
#include "colours.h"

#include "swfile.h"
#include "swoptions.h"
#include "procinvc.h"
#include "transaction.h"

#define SIZE_TIMESTR 23


int whole_flag, itemize_flag;
float total;
float tax[5];
int reprint = FALSE;
char invtype,itemised,postal,emailinv;
static char filenm[64],email_subject[64];
float total_pretax = 0.0,previous_ballance = 0.0;

extern struct filestruct PrimFile, quote_file, master_file, receipt_file;
extern char *ARCHIVE_DIR;

static struct primary_rec prim_buff;


FILE *stdprn;

extern int lock_read_master ();
extern void write_master_record ();
extern void load_client(uint16);

extern int errno;


int check_YN (char *test)
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

int check_WR (char *test)
{
  *test = toupper (*test);
  switch (*test)
    {
    case 'W':
    case 'R':
      return (TRUE);
    default:
      warn_user ("Must Be   W or R");
      return (FALSE);
    }
}



/* *INDENT-OFF* */
#define INVSCRNLEN 3
struct scr_params invoice_entry_screen[] = {
/*    1,0,"Filename : ",PROMPTCOL,UNFMT_STRING,filenm,FIELDCOL,38,NULL,NULL,*/
/*    2,0,"Wholesale/Retail  : ",PROMPTCOL,SINGLE_CHAR|CAPITALIZE,&invtype,FIELDCOL,1,check_WR,NULL,*/
    3,0,"Itemised Invoice  : ",PROMPTCOL,SINGLE_CHAR|CAPITALIZE,&itemised,FIELDCOL,1,check_YN,NULL,
    4,0,"Use Postal Address: ",PROMPTCOL,SINGLE_CHAR|CAPITALIZE,&postal,FIELDCOL,1,check_YN,NULL,
    5,0,"Email Invoice     : ",PROMPTCOL,SINGLE_CHAR|CAPITALIZE,&emailinv,FIELDCOL,1,check_YN,NULL,

};
/* *INDENT-ON* */

int confirm()
{
   char wch;
   while((wch = toupper(warn_user ("Print Invoice Y/N"))) != 'Y' && wch != 'N');
   return (wch=='Y');
}

int execute_DCL (char *DCL,unsigned window,unsigned int *retval) 
{
   char cmdstr[256];
   $DESCRIPTOR (acmdstr, cmdstr);
   unsigned int status;

   strcpy(cmdstr,DCL);
   acmdstr.dsc$w_length = strlen (cmdstr);
   status = smg$create_subprocess(&window);
   if (status != SS$_NORMAL && status != SMG$_SUBALREXI) {
      warn_user("%s %s %u",strerror(EVMSERR,status),filterfn(__FILE__),__LINE__);
      return FALSE;       
   }
   status = smg$execute_command(&window ,&acmdstr,&0,retval);
   if(status != SS$_NORMAL) {
      warn_user("%s (%s %u)",strerror(EVMSERR,status),filterfn(__FILE__),__LINE__);
      return FALSE;      
   }
   (*retval) &= 0xffffff;
   if(*retval != SS$_NORMAL) {
/*      warn_user("DCL failed %s %s %u",strerror(EVMSERR,*retval),DCL,*retval);  */
      return FALSE;      
   }
   return TRUE;
}

int send_invoice (char *invc_file,unsigned window,unsigned int *retval,char *email,char *subj)
{
     char dclcmd[1024],email_subject[1024];

     if(emailinv == 'Y') {
        sprintf(email_subject,"%s %s",master_buff.email_subject,subj);
        sprintf(dclcmd,"$mail/subject=\"%s\" %s%s \"%s\"",email_subject,ARCHIVE_DIR,invc_file,email);
     } else 
        sprintf(dclcmd,"$print %s%s /queue=%s",ARCHIVE_DIR,invc_file,master_buff.print_queue);

     execute_DCL(dclcmd,window,retval);

     if(*retval != SS$_NORMAL) {
/*        warn_user("Failed %s %u",strerror(EVMSERR,*retval),*retval); */
        return FALSE;
     }
     return TRUE;
}




void
print_invoice (struct scrllst_msg_rec *msg, struct filestruct *filedef)
{
   struct quote_part_rec *varl;
   int status, lnth, receipt_lnth, linecount;
   unsigned iter;
   float sub_total = 0.0, total_tax = 0.0;

   int wch, lnpos,edited = TRUE,startfld=0;
   unsigned cont = 0;

   char mailcmd[1024];
   $DESCRIPTOR (amailcmd, mailcmd);
   unsigned int diagwin;
   unsigned int mailret;


   if (quote_file.CurrentPtr == NULL) return;

   open_window (8, 10, 50, 7, WHITE, border);

   HeadLine("Print Invoice",centre);

   strcpy (filenm, "invoice.txt");
   lnpos = strlen (filenm);
/*      invtype = 'R';  */
   itemised = 'Y';
   postal = 'N';
   emailinv = 'N';

   init_screen(invoice_entry_screen,INVSCRNLEN);
   do {
      wch = enter_screen (invoice_entry_screen, INVSCRNLEN, &edited,
                                breakout, NULL, &startfld);
      if(wch == SMG$K_TRM_UP && startfld > 0) startfld--;
      else if(wch == SMG$K_TRM_DOWN && startfld < INVSCRNLEN-1) startfld++;
   } while (wch != SMG$K_TRM_CTRLZ && wch != SMG$K_TRM_CR);
   close_window ();
   if (wch == SMG$K_TRM_CTRLZ || !confirm()) return;
  


   quote_file.rab.rab$b_rac = RAB$C_RFA;
   rfa_copy (&quote_file.rab.rab$w_rfa, &quote_file.CurrentPtr->rfa);

   quote_file.rab.rab$l_ubf = (char *) quote_file.fileio;        /*record; */
   quote_file.rab.rab$w_usz = quote_file.fileiosz;
   quote_file.rab.rab$l_rop = RAB$M_NLK;

   rms_status = sys$get (&quote_file.rab);

   if (!rmstest (rms_status, 2, RMS$_NORMAL, RMS$_OK_RLK))
     {
       warn_user ("$GET %s", rmserror(rms_status,&quote_file));
       return;
     }


   read_master_record ();
   memcpy (&quote_buff, quote_file.fileio, sizeof (quote_buff));

   invtype = quote_buff.type;
   whole_flag = mkwsflag(invtype);
   itemize_flag = (itemised=='Y');

   load_client(quote_buff.cust_no);


#ifdef TRANSACTION
   status = sys$start_transw (event_flag, 0, &iosb, 0, 0, tid);
   check_status ("start");
#endif


   if (quote_buff.invoice_no == 0)       /* if this is not a reprint */
     {
       reprint = FALSE;

       quote_file.rab.rab$b_rac = RAB$C_RFA;
       rfa_copy (&quote_file.rab.rab$w_rfa, &quote_file.CurrentPtr->rfa);
       quote_file.rab.rab$l_ubf = (char *) quote_file.fileio;
       quote_file.rab.rab$w_usz = quote_file.fileiosz;
       quote_file.rab.rab$l_rop = RAB$M_RLK;

       rms_status = sys$get (&quote_file.rab);
       if (!rmstest (rms_status, 2, RMS$_NORMAL, RMS$_RLK))
         {
           warn_user ("$GET %s", rmserror(rms_status,&quote_file));
           return;
         }
       if (rms_status == RMS$_RLK)
         {
           warn_user (strerror (EVMSERR, rms_status));
           return;
         }

       memcpy (&quote_buff, quote_file.fileio, sizeof (quote_buff));

       if((quote_buff.invoice_no = invc_allocate()) == 0) {  /*  master_buff.invoice_no++;*/
          warn_user("Locked Master");
          return;
       }
       get_time (&quote_buff.invoice_date);
       dttodo (&quote_buff.invoice_date);

       memcpy (quote_file.fileio, &quote_buff, sizeof (quote_buff));


#ifndef TRANSACTION
       (*quote_file.setprompt) (quote_file.CurrentPtr, &quote_file);
#endif


       quote_file.rab.rab$b_rac = RAB$C_KEY;
       quote_file.rab.rab$l_rbf = (char *) quote_file.fileio;
       quote_file.rab.rab$w_rsz = quote_buff.reclen;

       rms_status = sys$update (&quote_file.rab);
       if (!rmstest (rms_status, 3, RMS$_NORMAL, RMS$_DUP, RMS$_OK_DUP))
         {
           warn_user ("UPD$ %s", rmserror(rms_status,&quote_file));
#ifdef TRANSACTION
           status = sys$abort_transw (event_flag, 0, &iosb, 0, 0, tid);
           check_status ("abort");
#endif
           return;
         }
     }
   else
     reprint = TRUE;

   sprintf(filenm,"invc%5.5u.txt",quote_buff.invoice_no);

   if (!open_stdprn (filenm,"w")) {
#ifdef TRANSACTION
      status = sys$abort_transw (event_flag, 0, &iosb, 0, 0, tid);
      check_status ("abort");
#endif
      return;
   }


   memcpy(&prim_buff,PrimFile.fileio,sizeof prim_buff);

   process_invc(quote_buff,prim_buff);
   previous_ballance = ballance(quote_buff.cust_no);


   if (!reprint) {
      if (!write_transaction(debit,whole_flag,quote_buff.cust_no,quote_buff.invoice_no,total)) {
        warn_user ("$PUT %s", rmserror(rms_status,&transaction_file));
        fclose(stdprn);
        delete_file(filenm);
#ifdef TRANSACTION
        status = sys$abort_transw (event_flag, 0, &iosb, 0, 0, tid);
        check_status ("abort");
#endif
        return;
      }
   }


   heading_invc (ende,quote_buff,prim_buff);

   fclose (stdprn);

   if(emailinv == 'Y' && strlen(prim_buff.email) == 0)  {
      warn_user("No Email Address %s",prim_buff.email);
      delete_file(filenm);
#ifdef TRANSACTION
      status = sys$abort_transw (event_flag, 0, &iosb, 0, 0, tid);
      check_status ("abort");
#endif       
      return;
   }



#ifdef DIAGS 
   open_window (8, 3, 100, 15, WHITE, border);
   diagwin = crnt_window->display_id;
#else 
   diagwin = Standard_Window;
#endif

   send_invoice(filenm,diagwin,&mailret,prim_buff.email,"Invoice");

#ifdef DIAGS
      sprintf(mailcmd,"%s returns %s %s\n\rAny Key to Continue",(emailinv=='Y')?"mail":"print",
                      xlat(mailret) ,strerror(EVMSERR,mailret));
      amailcmd.dsc$w_length = strlen (mailcmd);
      smg$put_line (&diagwin,&amailcmd);
      Refresh (diagwin);
      Get_Keystroke();
      close_window();
#endif

   if(mailret != SS$_NORMAL) {
      warn_user("%s Failed %s %s",(emailinv=='Y')?"Mailer":"Print",xlat(mailret),strerror(EVMSERR,mailret));  
      delete_file(filenm);
#ifdef TRANSACTION
      status = sys$abort_transw (event_flag, 0, &iosb, 0, 0, tid);
      check_status ("abort");
#endif      
      return;
   }

   if (reprint) delete_file(filenm);

#ifdef TRANSACTION
   status = sys$end_transw (event_flag, 0, &iosb, 0, 0, tid);
   check_status ("end");
   (*quote_file.setprompt) (quote_file.CurrentPtr, &quote_file);
#endif


}
