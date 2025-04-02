#include stdio
#include stdlib
#include rms
#include descrip
#include starlet
#include ints
#include lib$routines
#include string
#include stdarg

#include "[.extools]extools.h"
#include "filebrowse.h"
#define INC_INITS
#include "swfile.h"
#include "swlocality.h"


#define SIZE_TIMESTR 24
#define QUOTEBUFFSIZE 10000

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


main (int argc,char **argv)
{
   int number_records;
   char timestr[SIZE_TIMESTR];
   $DESCRIPTOR(atime,timestr);
   char quoteffn[64];
   char transffn[64];
   char *diskio_buff;
   uint32 trans_key;
   int no_trans,invoice_not_printed;
   uint16 cust_no;
   int specific = FALSE;


   if (argc > 1 ) {
      specific = TRUE;
      if((cust_no = atoi(argv[1])) == 0) {
         printf ("Bad Customer Number\n");
         exit (2);
      }
   }



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


   if ((diskio_buff = malloc (QUOTEBUFFSIZE)) == NULL)
     {
       printf (" Could Not Allocate disk buffer memory\n");
       exit;
     }


   quote_file.rab.rab$b_krf = INVC_CUST_KEY;
   if (!specific) {
      rms_status = sys$rewind(&quote_file.rab);
      if (rms_status != RMS$_NORMAL) 
         error("$REWIND",&quote_file); 
   }
   quote_file.rab.rab$b_rac = (specific)? RAB$C_KEY : RAB$C_SEQ;
   quote_file.rab.rab$l_ubf = (char *)diskio_buff;
   quote_file.rab.rab$w_usz = QUOTEBUFFSIZE;
   quote_file.rab.rab$l_kbf = (char *) &cust_no;
   quote_file.rab.rab$b_ksz = SIZE_BN2; 
   quote_file.rab.rab$l_rop = (specific)?RAB$M_NLK | RAB$M_EQNXT:RAB$M_NLK;

   transaction_file.rab.rab$b_krf = TRANS_INVC_KEY;
   transaction_file.rab.rab$b_rac = RAB$C_KEY;
   transaction_file.rab.rab$l_ubf = (char *)&transaction_buff;
   transaction_file.rab.rab$w_usz = sizeof transaction_buff; 
   transaction_file.rab.rab$l_rop = RAB$M_NLK;
 

   for(number_records = 0; ; number_records++) {
      rms_status = sys$get(&quote_file.rab);
      if (!test(rms_status,3,RMS$_NORMAL,RMS$_EOF,RMS$_OK_RRL)) 
         error("$GET",&quote_file); 
      else
         if (rms_status == RMS$_EOF) break;

      memcpy(&quote_buff,diskio_buff,sizeof quote_buff);

      if (specific) quote_file.rab.rab$b_rac = RAB$C_SEQ;
      if(specific && quote_buff.cust_no != cust_no) break;
   
      if (quote_buff.invoice_no != 0) {     
         trans_key = quote_buff.invoice_no;
         transaction_file.rab.rab$l_kbf = (char *)&trans_key;
         transaction_file.rab.rab$b_ksz = SIZE_BN4;
         rms_status = sys$get(&transaction_file.rab);
         if (!test(rms_status,3,RMS$_NORMAL,RMS$_RNF,RMS$_OK_RRL)) 
            error("$GET",&transaction_file);
         if(rms_status == RMS$_RNF) no_trans = TRUE;
         else no_trans = FALSE;
         invoice_not_printed = FALSE; 
      } else invoice_not_printed = TRUE; 

      rms_status = sys$asctim(0,
                              &atime,
                              &quote_buff.date,
                              0);
      if ((rms_status & 1) != 1) lib$signal(rms_status);
      timestr[atime.dsc$w_length] = 0;
      printf("%.*s Cust :%5.5u %s :%5.5u",
             SIZE_TIMESTR, timestr,
             quote_buff.cust_no,
             "Invc",
             quote_buff.invoice_no);
      if (invoice_not_printed)
          printf("     Invoice Not Sent\n");
      else
       if(!no_trans) {
         printf("%11.2f %s\n",
                transaction_buff.amnt,
                (transaction_buff.wholesale_flag)?"Wholesale":"Retail");
       } else printf("     No Transaction \n");

   }
   if (number_records) 
      printf("\nTotal number of records %d.\n", 
              number_records); 
   else
      printf (" [Data file is empty.] \n") ;  

   free(diskio_buff);
   rms_status = sys$close(&quote_file.fab);
   if (rms_status != RMS$_NORMAL) 
      error("$CLOSE - rmsret: ",&quote_file); 
   rms_status = sys$close(&transaction_file.fab);
   if (rms_status != RMS$_NORMAL) 
      error("$CLOSE - rmsret: ",&transaction_file); 


}




