#include stdio
#include stdlib
#include rms
#include descrip
#include starlet
#include ints
#include lib$routines
#include string
#include stdarg
#include math
#include errno

#include "[.extools]extools.h"
#include "filebrowse.h"
#define INC_INITS
#include "swfile.h"
#include "swlocality.h"


#define SIZE_TIMESTR 24
#define PRIMBUFFSIZE 10000


#ifdef __vax
float floorf(float value)
{
   return (float) (int) value;
}
#endif

char *prim_diskio_buff;

void
unpad (char *source, int lnth)
{
  int i;

  source[lnth] = 0; 
  for (i = lnth - 1; i >= 0 && source[i] == ' '; i--)
    source[i] = 0;
}



static void error(operation,filedef)
char *operation;
struct filestruct *filedef;
{
   printf("RMSEXP - file %s failed (%s)\n",operation,filedef->filename);
   exit(rms_status);
}

char *
rmserr (rmscode,filedef,srcfn,srcln)
unsigned int rmscode;
struct filestruct *filedef;
char *srcfn;
unsigned int srcln;
{
   static char outstr[128];
   char ffnstr[128],ffnstr2[128];

   strcpy(ffnstr2,filedef->filename);
   if(strtok(ffnstr2,"]") != NULL)
      strcpy(ffnstr2,strtok(NULL,";"));
   
   strcpy(ffnstr,srcfn);
   if(strtok(ffnstr,"]") != NULL)
      strcpy(ffnstr,strtok(NULL,";"));

   sprintf (outstr,"%s %u %s (%s %u)",(filedef==NULL)?"":ffnstr2,rmscode,
                 strerror (EVMSERR, rmscode),ffnstr, srcln);

   return(outstr);
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

char *txt_type(int wflag)
{
   switch(wflag) {
      case 0 : return("Retail");
      case 1 : return("Wholesale");
      case 2 : return("Cost");
      default: return("unknown");
   }

}

char *drcr(float amnt)
{
   amnt = floorf(amnt * 100) / 100;   /* Truncate to 2 decimal places */
   return ((amnt < 0.0)?"Dr":(amnt > 0.0)?"Cr":"");  
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

  if (!test (rms_status, 2, RMS$_NORMAL, RMS$_OK_RLK))
    {
      printf ("GET$ %s\n", rmserror(rms_status,&PrimFile));
      return;
    }

}


main (int argc,char **argv)
{
   int number_records;
   char timestr[SIZE_TIMESTR];
   $DESCRIPTOR(atime,timestr);
   char transffn[128],primffn[128];
   uint32 cust_no;
   int specific = FALSE;
   float total = 0.0;
   struct primary_rec prim_buff;


   if (argc > 1 ) {
      specific = TRUE;
      if((cust_no = atoi(argv[1])) == 0) {
         printf ("Bad Customer Number\n");
         exit (2);
      }
   }

   memset(&transaction_file,0,sizeof transaction_file);
   sprintf (transffn, "%s%s", FILE_PATH, "swtrans.dat");

   transaction_file.filename = transffn;
   transaction_file.initialize = TransactionFileInit;
   transaction_file.fileio = (char *)&transaction_buff;
   transaction_file.fileiosz = sizeof transaction_buff;

   open_file(&transaction_file) ;

   memset(&PrimFile,0,sizeof PrimFile);
   sprintf (primffn, "%s%s", FILE_PATH, "swprim.dat");

   PrimFile.filename = primffn;
   PrimFile.initialize = PrimaryFileInit;
   open_file(&PrimFile) ;
   
   if ((prim_diskio_buff = malloc (PRIMBUFFSIZE)) == NULL)
     {
       printf ("Memory Error %s %u\n",__FILE__,__LINE__);
       exit;
     }
   PrimFile.fileio = prim_diskio_buff;
   PrimFile.fileiosz = PRIMBUFFSIZE;


   transaction_file.rab.rab$b_krf = TRANS_CUST_KEY;
   if(!specific) {
      rms_status = sys$rewind(&transaction_file.rab);
      if (rms_status != RMS$_NORMAL) 
         error("$REWIND",&transaction_file); 
   }
   transaction_file.rab.rab$b_rac = (specific)? RAB$C_KEY : RAB$C_SEQ;
   transaction_file.rab.rab$l_ubf = (char *)&transaction_buff;
   transaction_file.rab.rab$w_usz = sizeof transaction_buff;   
   transaction_file.rab.rab$l_kbf = (char *) &cust_no;
   transaction_file.rab.rab$b_ksz = SIZE_BN4;
   transaction_file.rab.rab$l_rop = (specific)?RAB$M_NLK | RAB$M_EQNXT:RAB$M_NLK;

   for(number_records = 0; ; number_records++) {
      rms_status = sys$get(&transaction_file.rab);
      if (!test(rms_status,3,RMS$_NORMAL,RMS$_EOF,RMS$_OK_RRL)) 
         error("$GET",&transaction_file); 
      else
         if (rms_status == RMS$_EOF) break;

      if(specific)transaction_file.rab.rab$b_rac = RAB$C_SEQ;
      if(specific && transaction_buff.cust_no != cust_no) break;

      if(number_records==0 && specific){
         load_client(transaction_buff.cust_no);
         memcpy(&prim_buff,PrimFile.fileio, sizeof prim_buff);
         unpad(prim_buff.name,SIZE_NAME);

         printf("Transaction Statement for Client %5.5u %s\n\n",transaction_buff.cust_no,prim_buff.name);
      }

      rms_status = sys$asctim(0,
                              &atime,
                              &transaction_buff.date,
                              0);
      if ((rms_status & 1) != 1) lib$signal(rms_status);
      timestr[atime.dsc$w_length] = 0;
      printf("%.*s Cust :%5.5u %s :%5.5u %11.2f %2.2s",
             SIZE_TIMESTR, timestr,
             transaction_buff.cust_no,
             (transaction_buff.debit_flag)?"Invc":"Rcpt",
             (transaction_buff.debit_flag)? transaction_buff.invc_no:transaction_buff.rcpt_no,     
             transaction_buff.amnt,
             (transaction_buff.debit_flag)?"Dr":"Cr");
      

      if(transaction_buff.debit_flag) {
         total -= transaction_buff.amnt;
         printf(" %s\n", txt_type(transaction_buff.wholesale_flag));
      } else {
         total += transaction_buff.amnt;
         printf(" Receipt\n");
      }
   }
   if(specific) printf("%47s %11.2f %s\n","Balance $",fabs(total),drcr(total));
   if (number_records) 
      printf("\nTotal number of records %d.\n", 
              number_records); 
   else
      printf (" [Data file is empty.] \n") ;  

   free(prim_diskio_buff);
                       
   rms_status = sys$close(&transaction_file.fab);
   if (rms_status != RMS$_NORMAL) 
      error("$CLOSE - rmsret: ",&transaction_file); 
   rms_status = sys$close(&PrimFile.fab);
   if (rms_status != RMS$_NORMAL) 
      error("$CLOSE - rmsret: ",&PrimFile); 


}




