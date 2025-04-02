
#include stdio
#include stdlib
#include rms
#include descrip
#include starlet
#include ints
#include lib$routines
#include string
#include stdarg
#include errno

#include stsdef
#include ssdef
#include unistd
#include ssdef
#include ctype
#include stat

#include "[.extools]extools.h"
#include "filebrowse.h"
#define INC_INITS
#include "swfile.h"
#include "swlocality.h"
#include "transaction.h"
#include "swoptions.h"
#include "procinvc.h"


#define SIZE_TIMESTR 24
#define QUOTEBUFFSIZE 10000
#define PRIMBUFFSIZE 10000

char *months[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

enum exit_type { unproc, fail, requeue, coda, resend };
enum send_type { load, noload };

static char *prim_diskio_buff;


int whole_flag, itemize_flag;
float total;
float tax[5];
int reprint = FALSE,resend_invc = FALSE,resend_rcpt = FALSE;
char invtype,itemised,postal,emailinv;
static char filenm[64];
float total_pretax = 0.0,previous_ballance = 0.0;
static int status;

FILE *stdprn;

char leftovers_ffn[] = "leftovers.txt";

struct rfa_list_element {
   struct rfabuff rfa;
   int wsflag;
   enum exit_type status;
   uint32 quote;
   uint32 invc;
   struct rfa_list_element *next;
};

struct rfa_list_element *first = NULL,*last = NULL,*crnt = NULL;

struct primary_rec prim_buff;


void
strupr (char *upstr)
{
  int i;

  for (i = 0; upstr[i] != 0; i++)
    upstr[i] = toupper (upstr[i]);

}


static void error(operation,filedef,filenm,linenm)
char *operation;
struct filestruct *filedef;
char  *filenm;
unsigned linenm;
{
   printf("RMSEXP %s failed (%s) %s %u\n",operation,filedef->filename,filenm,linenm);
   exit(rms_status);
}


char *filterfn(char *fullfn)
{
   char *fn,*fn2;
   char delim[] = "];";
   static char filebuff[256];


   if(strchr(fullfn,']') == NULL) return(fullfn);
 
   strcpy (filebuff,fullfn);

   fn = strtok(filebuff,delim);

   if (fn != NULL) {
      fn = strtok(NULL,delim);
   }else
      return(fullfn);
   
   return(fn);

}
 


char *xlat(uint32 errorc)
{
   char msg[256];
   static char outstr[64];
   uint32 code, stat;
   uint16 msglen;
   $DESCRIPTOR(msgdesc, msg);

   stat = lib$sys_getmsg(&errorc, &msglen, &msgdesc,&8);
   msg[msglen] = 0;
   if(strcmp(msg+1,"RMS") == 0 ) {
      strcpy(outstr,msg+1);
      strcat(outstr,"$_");
      stat = lib$sys_getmsg(&errorc, &msglen, &msgdesc,&2);
      msg[msglen] = 0;
      strcat(outstr,msg+1);
   } else {
      stat = lib$sys_getmsg(&errorc, &msglen, &msgdesc,&14);
      msg[msglen] = 0;
      strcpy(outstr,msg);
   }
   return(outstr);
}


char *
rmserr (rmscode,filedef,srcfn,srcln)
unsigned int rmscode;
struct filestruct *filedef;
char *srcfn;
unsigned int srcln;
{
   static char outstr[256];
   char ffnstr[128];

 
   if(filedef!=NULL) strcpy(ffnstr,filterfn(filedef->filename));
   
   sprintf (outstr,"%s %s %s (%s %u)",(filedef==NULL)?"":ffnstr,xlat(rmscode),
                 strerror (EVMSERR, rmscode),filterfn(srcfn), srcln);

   return(outstr);
}

char warn_user(const char *format, ...)
{
   va_list argptr;
   char buffer[1024];

   va_start(argptr, format);
   vsprintf(buffer,format,argptr);
   va_end(argptr);

   printf("%s\n",buffer);
   return '\n';
}


int
rmstest (int rmscode, int num, ...)
{

  va_list valist;
  int i, found = FALSE;

  /* initialize valist for num number of arguments */
  va_start (valist, num);

  /* access all the arguments assigned to valist */
  for (i = 0; i < num; i++)
    {
      if (rmscode == va_arg (valist, int))
        {
          found = TRUE;
          break;
        }
    }

  /* clean memory reserved for valist */
  va_end (valist);

  return found;
}

void
rfa_copy (b1, b2)
     struct rfabuff *b1, *b2;
{
  b1->b1 = b2->b1;
  b1->b2 = b2->b2;
  b1->b3 = b2->b3;
}

void
pad (char *source, int lnth)
{
  int i;
  char buffer[64];

  sprintf (buffer, "%-*.*s", lnth, lnth, source);
  memcpy (source, buffer, lnth);

}


void
unpad (char *source, int lnth)
{
  int i;

  source[lnth] = 0; 
  for (i = lnth - 1; i >= 0 && source[i] == ' '; i--)
    source[i] = 0;
}


void
get_time (struct timebuff *result)
{
  int status;

  status = sys$gettim (result);

  if (!$VMS_STATUS_SUCCESS (status))
    {
      printf ("gettim %s %s %u\n", strerror (EVMSERR,status),__FILE__,
                 __LINE__);
      return;
    }

/*   dttodo(result); */


}

char *
print_time (struct timebuff *rawtime)
{
  int status;
  static char timestr[SIZE_TIMESTR+1];
  $DESCRIPTOR (atime, timestr);

  status = sys$asctim (0, &atime, rawtime,      /*&quote_buff.invoice_date, */
                       0);
  if (!$VMS_STATUS_SUCCESS (status))
    {
      printf ("asctim %s %s %u\n", strerror (EVMSERR,status),__FILE__,
                 __LINE__);
      return ("");
    }
  timestr[atime.dsc$w_length] = 0;

  return (timestr);

}

char *
print_date (struct timebuff *rawtime)
{
  int status;
  static char timestr[SIZE_TIMESTR+1];
  $DESCRIPTOR (atime, timestr);
  static char *fn;
  char delim[] = " ";
/*   char filebuff[256]; */


  status = sys$asctim (0, &atime, rawtime, 0);
  if (!$VMS_STATUS_SUCCESS (status))
    {
      printf ("asctim %s %s %u\n", strerror (EVMSERR,status), __FILE__,
                 __LINE__);
      return ("");
    }
  timestr[atime.dsc$w_length] = 0;


/*   strcpy (filebuff,timestr);*/
  fn = strtok (timestr, delim);
  if (fn == NULL)
    return (timestr);
  else
    return (fn);

/*   return(timestr); */
}

/* DateTime to DateOnly */
struct timebuff *
dttodo (struct timebuff *date_time)
{
  char outstr[25] = "";
  $DESCRIPTOR (output_d, outstr);
  unsigned short int numvec[7];
  int status;

  status = sys$numtim (numvec, date_time);
  if (!$VMS_STATUS_SUCCESS (status))
    {
      printf ("numtim %s %s %u\n", strerror (EVMSERR,status), __FILE__,
                 __LINE__);
      return (NULL);
    }

  sprintf (outstr, "%2.2hu-%3s-%4.4hu 00:00:00.00", numvec[2],
           months[numvec[1] - 1], numvec[0]);
  output_d.dsc$w_length = strlen (outstr);
  status = sys$bintim (&output_d, date_time);
  if (!$VMS_STATUS_SUCCESS (status))
    {
      printf ("Time Error %s %u\n",__FILE__, __LINE__);
      return (NULL);
    }
  return (date_time);
}




int
lock_read_master ()
{
  uint32 masrec = 1;

  master_file.rab.rab$b_rac = RAB$C_KEY;
  master_file.rab.rab$l_kbf = (char *) &masrec;
  master_file.rab.rab$b_ksz = 4;
  master_file.rab.rab$b_tmo = 10;
  master_file.rab.rab$l_ubf = (char *) &master_buff;
  master_file.rab.rab$w_usz = sizeof (master_buff);
  master_file.rab.rab$l_rop = RAB$M_RLK|RAB$M_WAT|RAB$M_TMO;

  rms_status = sys$get (&master_file.rab);

  if (!rmstest (rms_status, 3, RMS$_NORMAL, RMS$_RLK, RMS$_OK_WAT)) {
    printf ("$GET %s\n", rmserror(rms_status,&master_file));
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
      (rms_status, 3, RMS$_NORMAL, RMS$_OK_ALK, RMS$_OK_RLK))
    {
      printf ("$GET %s\n",rmserror(rms_status,&master_file));
      return;
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
      printf ("$GET %s\n", rmserror(rms_status,&master_file));
      return;
    }
  if (rms_status == RMS$_RLK)
    {
      printf ("%s\n", rmserror (rms_status,&master_file));
      return;
    }

  master_file.rab.rab$l_rbf = (char *) &master_buff;
  master_file.rab.rab$w_rsz = sizeof (master_buff);

  master_file.rab.rab$l_rop = RAB$M_UIF;

  rms_status = sys$update (&master_file.rab);
  if (!rmstest (rms_status, 1, RMS$_NORMAL))
    printf ("$UPDATE %s\n", rmserror(rms_status,&master_file));

}


void
mkascdate (dest, time)
     char *dest;
     struct timebuff time;
{
  static unsigned short int numvec[7];
  int status;

  status = sys$numtim (numvec, &time);
  if (!$VMS_STATUS_SUCCESS (status))
    {
      printf ("Numtime %s %s %u", strerror (status),__FILE__,
                 __LINE__);
      return;
    }


  (void) sprintf (dest, "%04hu%02hu%02hu%02hu%02hu%02hu%02hu",
                  numvec[0],
                  numvec[1],
                  numvec[2], numvec[3], numvec[4], numvec[5], numvec[6]);
}                                                                  


int load_transaction_by_invc (uint32 invc_no)
{

   transaction_file.rab.rab$b_krf = TRANS_INVC_KEY;
   transaction_file.rab.rab$l_kbf = (char *)&invc_no;
   transaction_file.rab.rab$b_ksz = SIZE_BN4;
   transaction_file.rab.rab$l_rop = RAB$M_NLK;

   rms_status = sys$get(&transaction_file.rab);
   if (!rmstest(rms_status,2,RMS$_NORMAL,RMS$_OK_RRL)) { 
      printf("%s\n",rmserror(rms_status,&transaction_file));
      return FALSE;
   }

   return TRUE;
}

int load_transaction_by_rcpt (uint32 rcpt_no)
{
 
   transaction_file.rab.rab$b_krf = TRANS_RCPT_KEY;
   transaction_file.rab.rab$l_kbf = (char *)&rcpt_no;
   transaction_file.rab.rab$b_ksz = SIZE_BN4;
   transaction_file.rab.rab$l_rop = RAB$M_NLK;

   rms_status = sys$get(&transaction_file.rab);
   if (!rmstest(rms_status,2,RMS$_NORMAL,RMS$_OK_RRL)) { 
      printf("%s\n",rmserror(rms_status,&transaction_file));
      return FALSE;       
   }
   return(TRUE);
}



void load_client (uint16 cust_no)
{
  
  PrimFile.rab.rab$b_krf = CLNT_CUST_KEY;
  PrimFile.rab.rab$l_kbf = (char *) &cust_no;
  PrimFile.rab.rab$b_ksz = SIZE_BN2;
  PrimFile.rab.rab$b_rac = RAB$C_KEY;
  
  PrimFile.rab.rab$l_ubf = (char *) PrimFile.fileio;
  PrimFile.rab.rab$w_usz = PrimFile.fileiosz;
  PrimFile.rab.rab$l_rop = RAB$M_NLK;                                /* RAB$M_RRL; */

  rms_status = sys$get (&PrimFile.rab);

  if (!rmstest (rms_status, 2, RMS$_NORMAL, RMS$_OK_RLK))
    {
      printf ("GET$ %s\n", rmserror(rms_status,&PrimFile));
      return;
    }

}


int load_quote(struct rfabuff *quote_rfa)
{
   quote_file.rab.rab$b_rac = RAB$C_RFA;
   rfa_copy (&quote_file.rab.rab$w_rfa, quote_rfa);

   quote_file.rab.rab$l_ubf = (char *) quote_file.fileio;
   quote_file.rab.rab$w_usz = quote_file.fileiosz;
   quote_file.rab.rab$l_rop = RAB$M_NLK;

   rms_status = sys$get (&quote_file.rab);

   if (!rmstest (rms_status, 2, RMS$_NORMAL, RMS$_OK_RLK))
     {
       printf ("$GET %s\n", rmserror(rms_status,&quote_file));
       return FALSE;
     }
   return TRUE;
}

int load_quote_by_invc(uint16 invc_no)
{
      quote_file.rab.rab$b_krf = INVC_INVC_KEY;
      quote_file.rab.rab$l_kbf = (char *) &invc_no;
      quote_file.rab.rab$b_ksz = SIZE_BN2;
      quote_file.rab.rab$l_ubf = (char *) quote_file.fileio;
      quote_file.rab.rab$w_usz = quote_file.fileiosz;
      quote_file.rab.rab$l_rop = RAB$M_NLK;

      rms_status = sys$get(&quote_file.rab);
      if (!rmstest(rms_status,1,RMS$_NORMAL)) {
         printf("$GET %s",rmserror(rms_status,&quote_file));
         return FALSE;
      }
      return TRUE;
}



enum exit_type send_invoice (char *invc_file,enum send_type context)
{
     char mailcmd[1024],email_subject[1024];
     struct stat statbuff;

     sprintf(mailcmd,"%s%s",ARCHIVE_DIR,invc_file);
     if (stat(mailcmd,&statbuff) != 0) {
        printf("%s does not exist\n",mailcmd);
        return resend;
     }

     if(context == load) {
        if(!load_quote(&crnt->rfa)) return resend;
        memcpy(&quote_buff,quote_file.fileio,sizeof quote_buff);

        load_client(quote_buff.cust_no);
        memcpy(&prim_buff,PrimFile.fileio, sizeof prim_buff);
     }

     if(emailinv == 'Y') {
        sprintf(email_subject,"%s Invoice",master_buff.email_subject);
        sprintf(mailcmd,"$mail/subject=\"%s\" %s%s \"%s\"",email_subject,ARCHIVE_DIR,invc_file,prim_buff.email);
     } else 
        sprintf(mailcmd,"$print %s%s /queue=%s",ARCHIVE_DIR,invc_file,master_buff.print_queue);

     printf("%s\n",mailcmd);

     status = system(mailcmd);

     if(status != SS$_NORMAL) {
        printf("Failed %s %s\n",xlat(status),strerror(EVMSERR,status));
        return resend;
     }
     printf("%s %s\n",xlat(status),strerror(EVMSERR,status));    
     return coda;
}


enum exit_type clone_invoice (struct rfabuff *quote_rfa, int wsflag)
{
 
   if(!load_quote(quote_rfa)) return fail;
   memcpy(&quote_buff,quote_file.fileio,sizeof quote_buff);

   load_client(quote_buff.cust_no);
   memcpy(&prim_buff,PrimFile.fileio, sizeof prim_buff);

   unpad(prim_buff.name,SIZE_NAME);
   unpad(prim_buff.country,SIZE_COUNTRY);
   unpad(quote_buff.class,SIZE_QUOTECLASS);


   whole_flag = wsflag;
   itemize_flag = (itemised=='Y');


   read_master_record ();


   if((quote_buff.invoice_no = invc_allocate()) == 0) {
      printf ("Master Locked\n");
      return(requeue);
   }
   get_time (&quote_buff.invoice_date);
   dttodo (&quote_buff.invoice_date);

   printf("Processing %u %s Invc No: %5.5u\n",quote_buff.cust_no,prim_buff.name,quote_buff.invoice_no);

   crnt->invc = quote_buff.invoice_no;


   if((quote_buff.quote_no = quote_allocate()) == 0) {
      printf ("Master Locked\n");
      return(requeue);
   }
   get_time (&quote_buff.quote_date);
   dttodo (&quote_buff.quote_date);

   quote_buff.job_no = 0;
   *quote_buff.class = 0;
   pad(quote_buff.class,SIZE_QUOTECLASS);

   get_time (&quote_buff.date);
   mkascdate (quote_buff.ascdate, quote_buff.date);


#ifdef TRANSACTION
   status = sys$start_transw (1, 0, &iosb, NULL, 0, tid);
   check_status ("start");
#endif

   memcpy (quote_diskio_buff, &quote_buff, sizeof (quote_buff));

   quote_file.rab.rab$b_rac = RAB$C_KEY;
   quote_file.rab.rab$l_rbf = (char *) quote_diskio_buff;
   quote_file.rab.rab$w_rsz = quote_buff.reclen;

   rms_status = sys$put (&quote_file.rab);
   if (!rmstest (rms_status, 2, RMS$_NORMAL, RMS$_OK_DUP))
     {
       printf ("PUT$ %s\n", rmserror(rms_status,&quote_file));
#ifdef TRANSACTION
       status = sys$abort_transw (1, 0, &iosb, NULL, 0, tid);
       check_status ("abort");
#endif
       return(fail);
     }

   sprintf(filenm,"invc%5.5u.txt",quote_buff.invoice_no);
   if (!open_stdprn (filenm,"w")) {
#ifdef TRANSACTION
      status = sys$abort_transw (1, 0, &iosb, NULL, 0, tid);
      check_status ("abort");
#endif
      return(fail);
   }


   process_invc(quote_buff,prim_buff);
   previous_ballance = ballance(quote_buff.cust_no);


   if (!write_transaction(debit,whole_flag,quote_buff.cust_no,quote_buff.invoice_no,total)) {
      fclose (stdprn);
      delete_file(filenm);
#ifdef TRANSACTION
      status = sys$abort_transw (1, 0, &iosb, NULL, 0, tid);
      check_status ("abort");
#endif
      return (fail);
   }


   heading_invc (ende,quote_buff,prim_buff);

   fclose (stdprn);

   if(emailinv == 'Y' && strlen(prim_buff.email) == 0)  {
      printf("No Email Address\n");
#ifdef TRANSACTION
      delete_file(filenm);
      status = sys$abort_transw (1, 0, &iosb, NULL, 0, tid);
      check_status ("abort");
#endif       
      return(resend);
   }

   if(send_invoice(filenm,noload) == resend) {  
#ifdef TRANSACTION
      delete_file(filenm);
      status = sys$abort_transw (1, 0, &iosb, NULL, 0, tid);
      check_status ("abort");
#endif       
      return(resend);
   }


#ifdef TRANSACTION
   status = sys$end_transw (1, 0, &iosb, NULL, 0, tid);
   check_status ("end");
#endif

   return(coda);
}

int where(char *testclass) {

   return(strncmp(quote_buff.class,testclass,SIZE_QUOTECLASS) == 0);

}

enqueue(struct rfa_list_element **new)
{

   if((*new = malloc(sizeof(struct rfa_list_element))) == NULL) {
      printf("Memory Error\n");
      exit;
   }

   if(first == NULL) first = *new;
   else last->next = *new; 
   last = *new;
   (*new)->next = NULL;
}

void close_files()
{

   free(quote_diskio_buff);
   free(prim_diskio_buff);


   rms_status = sys$close(&quote_file.fab);
   if (rms_status != RMS$_NORMAL) 
      error("$CLOSE - rmsret: ",&quote_file,__FILE__,__LINE__); 
   rms_status = sys$close(&transaction_file.fab);
   if (rms_status != RMS$_NORMAL) 
      error("$CLOSE - rmsret: ",&transaction_file,__FILE__,__LINE__); 
   rms_status = sys$close(&PrimFile.fab);
   if (rms_status != RMS$_NORMAL) 
      error("$CLOSE - rmsret: ",&PrimFile,__FILE__,__LINE__); 
   rms_status = sys$close(&master_file.fab);
   if (rms_status != RMS$_NORMAL) 
      error("$CLOSE - rmsret: ",&master_file,__FILE__,__LINE__); 

}

void open_files()
{
   static char quoteffn[64],mastffn[64];
   static char transffn[64],primffn[64];

   memset(&master_file,0,sizeof master_file);
   sprintf (mastffn, "%s%s", FILE_PATH, "swmaster.dat");

   master_file.filename = mastffn;
   master_file.initialize = MasterFileInit;
   open_file(&master_file) ;


   memset(&quote_file,0,sizeof quote_file);
   sprintf (quoteffn, "%s%s", FILE_PATH, "swquote.dat");

   quote_file.filename = quoteffn;
   quote_file.initialize = QuoteFileInit;
   open_file(&quote_file) ;

   memset(&transaction_file,0,sizeof transaction_file);
   sprintf (transffn, "%s%s", FILE_PATH, "swtrans.dat");

   transaction_file.filename = transffn;
   transaction_file.initialize = TransactionFileInit;
   open_file(&transaction_file) ;

   memset(&PrimFile,0,sizeof PrimFile);
   sprintf (primffn, "%s%s", FILE_PATH, "swprim.dat");

   PrimFile.filename = primffn;
   PrimFile.initialize = PrimaryFileInit;
   open_file(&PrimFile) ;
   
   if ((prim_diskio_buff = malloc (PRIMBUFFSIZE)) == NULL)
     {
       printf (" Could Not Allocate disk buffer memory %s %u\n",__FILE__,__LINE__);
       exit;
     }
   PrimFile.fileio = prim_diskio_buff;
   PrimFile.fileiosz = PRIMBUFFSIZE;

   if ((quote_diskio_buff = malloc (QUOTEBUFFSIZE)) == NULL)
     {
       printf (" Could Not Allocate disk buffer memory %s %u\n",__FILE__,__LINE__);
       exit;
     }
   quote_file.fileio = quote_diskio_buff;
   quote_file.fileiosz = QUOTEBUFFSIZE;

}

main (int argc,char **argv)
{
   int iter,number_records;
   char timestr[SIZE_TIMESTR];
   $DESCRIPTOR(atime,timestr);
   char useage[] = "useage: batchinvc [invoice_class|-left|-rcpt|-invc] [-test|-print|-email] [number|-post] \n";
   int test_flag = FALSE,some_left = FALSE,loop_count = 0;
   int leftovers = FALSE;
   unsigned int quote_no,invc_no,invc_status,left_flag = FALSE;
   char fail_type[32];
   unsigned int resend_no;
   char resendffn[64];
   char classkey[SIZE_QUOTECLASS+1];

   itemised = 'Y';
   postal = 'N';

   if(argc < 3) {


      printf(useage);
      exit(1);
   }

   if (strcmp(argv[1],"-left") == 0) left_flag = TRUE;
   else if(strcmp(argv[1],"-invc") == 0) resend_invc = TRUE; 
   else if(strcmp(argv[1],"-rcpt") == 0) resend_rcpt = TRUE; 

   if(argc > 2){
      if(strcmp(argv[2],"-test") == 0)  test_flag = TRUE;
      else if (strcmp(argv[2],"-print") == 0) emailinv = 'N';
      else if (strcmp(argv[2],"-email") == 0) emailinv = 'Y';
      else {
         printf(useage);
         exit(SS$_NORMAL);
      }
   }
   if(resend_invc || resend_rcpt) {
      if(argc < 4) {
         printf(useage);
         exit(SS$_NORMAL);
      }
      if((resend_no=atoi(argv[3])) == 0) {
         printf(useage);
         exit(SS$_NORMAL);
      }
   } else {
      if(argc > 3)
         if (strcmp(argv[3],"-post") == 0) postal = 'Y';
         else {
            printf(useage);
            exit(SS$_NORMAL);
         }
   }




   open_files();

   strcpy(classkey,argv[1]);
   strupr(classkey);
   pad(classkey,SIZE_QUOTECLASS);

   quote_file.rab.rab$b_rac = RAB$C_KEY;
   quote_file.rab.rab$b_krf = INVC_CLASS_KEY;
   quote_file.rab.rab$l_kbf = (char *) &classkey;
   quote_file.rab.rab$b_ksz = SIZE_QUOTECLASS;
   quote_file.rab.rab$l_rop = RAB$M_NLK | RAB$M_EQNXT;
   quote_file.rab.rab$l_ubf = (char *) quote_diskio_buff;
   quote_file.rab.rab$w_usz = QUOTEBUFFSIZE;


   transaction_file.rab.rab$b_krf = TRANS_INVC_KEY;
   transaction_file.rab.rab$b_rac = RAB$C_KEY;
   transaction_file.rab.rab$l_ubf = (char *)&transaction_buff;
   transaction_file.rab.rab$w_usz = sizeof transaction_buff; 
   transaction_file.rab.rab$l_rop = RAB$M_NLK;

   if(resend_invc || resend_rcpt) {
      read_master_record ();
      if(resend_invc) {
         if(!load_transaction_by_invc(resend_no)) exit(2);
         load_client(transaction_buff.cust_no);
      } else {
         if(!load_transaction_by_rcpt(resend_no)) exit(2);
         load_client(transaction_buff.cust_no);
      }
      memcpy(&prim_buff,PrimFile.fileio, sizeof prim_buff);
      sprintf(resendffn,"%s%5.5u.txt",(resend_invc)?"invc":"rcpt",resend_no);
      printf("Resending %s\n",resendffn);
      send_invoice(resendffn,noload);
      close_files();
      exit(SS$_NORMAL);
   }

 
   printf("Batch Processing Begins\n");
   printf("Roundup Phase\n");

   if(left_flag) {
      if (!open_stdprn(leftovers_ffn,"r+")) {
         printf("No Leftovers File\n");
         close_files();
         exit(SS$_NORMAL);
      }

      quote_file.rab.rab$b_krf = INVC_QUOTE_KEY;
      quote_file.rab.rab$l_kbf = (char *) &quote_no;
      quote_file.rab.rab$b_ksz = SIZE_BN2;
      quote_file.rab.rab$l_rop = RAB$M_NLK;

      number_records = 0;
      while (fscanf(stdprn,"%u:%u:%u:%s",&quote_no,&invc_no,&invc_status,fail_type ) != EOF) {
         rms_status = sys$get(&quote_file.rab);
         if (!rmstest(rms_status,1,RMS$_NORMAL)) {
            printf("$GET %s\n",rmserror(rms_status,&quote_file));
            continue; 
         }
         memcpy(&quote_buff,quote_file.fileio,sizeof quote_buff);

         load_client(quote_buff.cust_no);
         memcpy(&prim_buff,PrimFile.fileio, sizeof prim_buff);

         unpad(prim_buff.name,SIZE_NAME);
         unpad(quote_buff.class,SIZE_QUOTECLASS);
         printf("Found %s %u %s Quote : %5.5u %s\n",quote_buff.class,
                             quote_buff.cust_no,prim_buff.name,quote_buff.quote_no,fail_type);


         enqueue(&crnt);
         
         crnt->quote = quote_buff.quote_no;
         crnt->invc = invc_no;
         crnt->status = invc_status;
         crnt->wsflag = mkwsflag(quote_buff.type);
         rfa_copy (&crnt->rfa,&quote_file.rab.rab$w_rfa);

         number_records++;
      }
      fclose(stdprn);

   } else {


      for(number_records = 0; ; number_records++) {
         rms_status = sys$get(&quote_file.rab);
         if (!rmstest(rms_status,3,RMS$_NORMAL,RMS$_EOF,RMS$_OK_RRL)) {
            printf("$GET %s\n",rmserror(rms_status,&quote_file));
            exit;
         }else
            if (rms_status == RMS$_EOF) break;

         quote_file.rab.rab$b_rac = RAB$C_SEQ;

         memcpy(&quote_buff,quote_diskio_buff,sizeof quote_buff);
         if(!where (classkey)) break;
    

         load_client(quote_buff.cust_no);
         memcpy(&prim_buff,prim_diskio_buff, sizeof prim_buff);

  
         unpad(prim_buff.name,SIZE_NAME);
         unpad(quote_buff.class,SIZE_QUOTECLASS);
         printf("Found %s %u %s\n",quote_buff.class,quote_buff.cust_no,prim_buff.name);

         enqueue(&crnt);

         crnt->quote = quote_buff.quote_no;
         crnt->status = unproc;
         crnt->wsflag = mkwsflag(quote_buff.type);
         rfa_copy (&crnt->rfa,&quote_file.rab.rab$w_rfa);
       
      }

   }

   if (number_records) 
      printf("\nTotal number of records %d.\n", 
              number_records); 
   else
      printf (" [Data file is empty.] \n") ;  


   printf("Processing Phase\n");

   if(!test_flag) {
      read_master_record ();
      do {
         some_left = FALSE;
         for(crnt=first; crnt!=NULL; crnt=crnt->next){
            if(crnt->status != coda && crnt->status != fail ) {

#ifdef TRANSACTION
               if (FALSE) {
#else
               if (crnt->status == resend) {
#endif
                  sprintf(resendffn,"invc%5.5u.txt",crnt->invc);
                  printf("Resending %s\n",resendffn);
                  crnt->status=send_invoice(resendffn,load);
               } else { /*unproc or requeue */
                  if((crnt->status=clone_invoice(&crnt->rfa,crnt->wsflag)) == requeue) {
                     some_left = TRUE;
                  }
               }    
            }
            if(crnt->status != coda) leftovers = TRUE;
         }
         if (some_left) printf("Some had locked master, retrying. count %i\n",loop_count+1);
      } while(some_left && loop_count++ < 5);

      if(leftovers) {
         if(!open_stdprn(leftovers_ffn,"r+")) {
            printf("Creating %s\n",leftovers_ffn);
            if(!open_stdprn(leftovers_ffn,"w")) { 
               printf("Open %s failed %s\n",leftovers_ffn,strerror(EVMSERR,errno));
               exit;
            }
         }
         for(crnt=first; crnt!=NULL; crnt=crnt->next){
            if(crnt->status != coda)
               fprintf (stdprn,"%5.5u:%5.5u:%u:%s\n",crnt->quote,crnt->invc,crnt->status,
                            (crnt->status==fail)?"Failed":(crnt->status==resend)?"Resend":"Locked");
         }
         if(ftruncate(fileno(stdprn),ftell(stdprn)) != 0){
             printf("Truncate Error\n");
             exit;
         }
         fclose(stdprn);
      } else delete_file(leftovers_ffn);
   }


   printf("Finished Batch Run\n");

   for(;(crnt=first)!=NULL;free(crnt)) first=first->next;

   close_files();

}




