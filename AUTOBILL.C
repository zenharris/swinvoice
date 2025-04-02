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

#include "[.extools]extools.h"
#include "filebrowse.h"
#define INC_INITS
#include "swfile.h"
#include "swlocality.h"


#define SIZE_TIMESTR 24
#define QUOTEBUFFSIZE 10000
#define PRIMBUFFSIZE 10000

static char *prim_diskio_buff;


static void error(operation,filedef)
char *operation;
struct filestruct *filedef;
{
   printf("RMSEXP - file %s failed (%s)\n",operation,filedef->filename);
   exit(rms_status);
}

static int
test (int rmscode, int num, ...)
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


int get_transaction ()
{
   uint32 trans_key;
   int no_trans,invoice_not_printed;

         trans_key = quote_buff.invoice_no;
         transaction_file.rab.rab$l_kbf = (char *)&trans_key;
         transaction_file.rab.rab$b_ksz = SIZE_BN4;
         rms_status = sys$get(&transaction_file.rab);
         if (!test(rms_status,3,RMS$_NORMAL,RMS$_RNF,RMS$_OK_RRL)) 
            error("$GET",&transaction_file);
         if(rms_status == RMS$_RNF) return(FALSE);
         else return(TRUE);

}

void load_client ()
{
  
  PrimFile.rab.rab$b_krf = CLNT_CUST_KEY;
  PrimFile.rab.rab$l_kbf = (char *) &quote_buff.cust_no;
  PrimFile.rab.rab$b_ksz = SIZE_BN2;
  PrimFile.rab.rab$b_rac = RAB$C_KEY;
  
  PrimFile.rab.rab$l_ubf = (char *) prim_diskio_buff;
  PrimFile.rab.rab$w_usz = PRIMBUFFSIZE;
  PrimFile.rab.rab$l_rop = RAB$M_NLK;                                /* RAB$M_RRL; */

  rms_status = sys$get (&PrimFile.rab);

  if (!test (rms_status, 2, RMS$_NORMAL, RMS$_OK_RLK))
    {
      printf ("GET$ %s\n", strerror(EVMSERR,rms_status));
      return;
    }
/*  if(PrimFile.unpadrecord != NULL) (*PrimFile.unpadrecord)(); */

}


main ()
{
   int number_records;
   char timestr[SIZE_TIMESTR];
   $DESCRIPTOR(atime,timestr);
   char quoteffn[64];
   char transffn[64],primffn[64];
   struct primary_rec prim_buff;

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
       printf (" Could Not Allocate disk buffer memory\n");
       exit;
     }
   PrimFile.fileio = prim_diskio_buff;
   PrimFile.fileiosz = PRIMBUFFSIZE;

   if ((quote_diskio_buff = malloc (QUOTEBUFFSIZE)) == NULL)
     {
       printf (" Could Not Allocate disk buffer memory\n");
       exit;
     }


   quote_file.rab.rab$b_krf = INVC_CLASS_KEY;

   rms_status = sys$rewind(&quote_file.rab);
   if (rms_status != RMS$_NORMAL) 
      error("$REWIND",&quote_file); 

   quote_file.rab.rab$b_rac = RAB$C_SEQ;
   quote_file.rab.rab$l_ubf = (char *)quote_diskio_buff;
   quote_file.rab.rab$w_usz = QUOTEBUFFSIZE; 
   quote_file.rab.rab$l_rop = RAB$M_RRL;

   transaction_file.rab.rab$b_krf = TRANS_INVC_KEY;
   transaction_file.rab.rab$b_rac = RAB$C_KEY;
   transaction_file.rab.rab$l_ubf = (char *)&transaction_buff;
   transaction_file.rab.rab$w_usz = sizeof transaction_buff; 
   transaction_file.rab.rab$l_rop = RAB$M_RRL;
 

   for(number_records = 0; ; number_records++) {
      rms_status = sys$get(&quote_file.rab);
      if (!test(rms_status,3,RMS$_NORMAL,RMS$_EOF,RMS$_OK_RRL)) 
         error("$GET",&quote_file); 
      else
         if (rms_status == RMS$_EOF) break;

      memcpy(&quote_buff,quote_diskio_buff,sizeof quote_buff);
      
      load_client();
      memcpy(&prim_buff,prim_diskio_buff, sizeof prim_buff);

      unpad(prim_buff.name,SIZE_NAME);
      unpad(quote_buff.class,SIZE_QUOTECLASS);
      printf("%s %u %u %s\n",quote_buff.class,quote_buff.cust_no,quote_buff.invoice_no,prim_buff.name);





   }
   if (number_records) 
      printf("\nTotal number of records %d.\n", 
              number_records); 
   else
      printf (" [Data file is empty.] \n") ;  

   free(quote_diskio_buff);
   free(prim_diskio_buff);

   rms_status = sys$close(&quote_file.fab);
   if (rms_status != RMS$_NORMAL) 
      error("$CLOSE - rmsret: ",&quote_file); 
   rms_status = sys$close(&transaction_file.fab);
   if (rms_status != RMS$_NORMAL) 
      error("$CLOSE - rmsret: ",&transaction_file); 
   rms_status = sys$close(&PrimFile.fab);
   if (rms_status != RMS$_NORMAL) 
      error("$CLOSE - rmsret: ",&PrimFile); 


}




