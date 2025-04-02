
#include stdio
#include stdlib
#include ints
#include rms
#include string
#include math
#include starlet
#include errno
#include ssdef
#include stat

#include "[.extools]extools.h"
#include "filebrowse.h"
#include "swfile.h"
#include "swoptions.h"
#include "procinvc.h"


#define rmserror(rmscode,filedef) rmserr(rmscode,filedef,__FILE__,__LINE__)
extern int rmstest(int,int,...);

extern float total;
extern float tax[5];
extern float total_pretax;
extern float previous_ballance;

extern FILE *stdprn;
extern int reprint;
extern char postal;
extern int whole_flag;
extern int itemize_flag;

extern float ballance(uint32);
extern char warn_user(const char *, ...);
extern int lock_read_master ();
extern void write_master_record ();

extern char *ARCHIVE_DIR;

static int list_count;

#ifdef __vax
float floorf(float value)
{
   return (float) (int) value;
}
#endif


int
open_stdprn (char *fn,char *mode)
{
  char ffn[256];
  struct stat statbuff;

  if (stat(ARCHIVE_DIR,&statbuff) != 0) {
     warn_user("No Archive Dir, Creating %s",ARCHIVE_DIR);
     if (mkdir(ARCHIVE_DIR,0700) != 0){
        warn_user ("Create Directory failed");
        return FALSE;
     }  
  }

  sprintf(ffn,"%s%s",ARCHIVE_DIR,fn);
  stdprn = fopen (ffn,mode);
  if (stdprn == NULL)
    {
       warn_user ("Open Fail %s %s %s %s %u",xlat(errno), strerror (EVMSERR,errno), ffn,
                    filterfn(__FILE__), __LINE__);
       return (FALSE);
    }
  rewind (stdprn);
  return (TRUE);
}

int delete_file(char *fn)
{
   char rmfn[64];
   struct stat statbuff;
   sprintf(rmfn,"%s%s",ARCHIVE_DIR,fn);
   if(stat(rmfn,&statbuff) == 0) {
      if (remove(rmfn) != 0) {
         warn_user("Delete Fail %s",rmfn);
         return FALSE;
      }
   }
   return TRUE;
}


float ballance (uint32 cust_no)
{
   float ballance = 0.0;

   transaction_file.rab.rab$b_krf = TRANS_CUST_KEY;
   transaction_file.rab.rab$l_kbf = (char *) &cust_no;
   transaction_file.rab.rab$b_ksz = SIZE_BN4;
   transaction_file.rab.rab$b_rac = RAB$C_KEY;
   transaction_file.rab.rab$l_ubf = (char *)&transaction_buff;
   transaction_file.rab.rab$w_usz = sizeof transaction_buff; 
   transaction_file.rab.rab$l_rop = RAB$M_NLK | RAB$M_EQNXT;     /* return equal to or next  */
   rms_status = sys$get (&transaction_file.rab);
   if (!rmstest
       (rms_status, 4, RMS$_NORMAL, RMS$_RNF, RMS$_OK_RRL,
        RMS$_OK_RLK))
      warn_user ("GET$ %s",rmserror(rms_status,&transaction_file));
   transaction_file.rab.rab$b_rac = RAB$C_SEQ;
   transaction_file.rab.rab$l_rop = RAB$M_NLK;

   while(rms_status != RMS$_EOF && transaction_buff.cust_no == cust_no) {

      if(transaction_buff.debit_flag)
         ballance -= transaction_buff.amnt;
      else
         ballance += transaction_buff.amnt;

      rms_status = sys$get (&transaction_file.rab);
   }
   return(ballance);
}


uint32 invc_allocate()
{
   static uint32 searchkey;

   if(!lock_read_master()) {
/*      warn_user("Master Locked");*/
      return(FALSE);
   }

   transaction_file.rab.rab$b_krf = TRANS_INVC_KEY;
   transaction_file.rab.rab$l_kbf = (char *)&searchkey;
   transaction_file.rab.rab$b_ksz = SIZE_BN4;
   transaction_file.rab.rab$b_rac = RAB$C_KEY;
   transaction_file.rab.rab$l_rop = RAB$M_CDK;  /* check for duplicate key  */

   while((searchkey=master_buff.invoice_no++) < 65535) {

      rms_status = sys$find(&transaction_file.rab);

      if (!rmstest(rms_status,2,RMS$_NORMAL,RMS$_RNF)) {
         warn_user("$CHECK %s",rmserror(rms_status,&transaction_file));
         break;          
      } else
         if (rms_status == RMS$_RNF) {
            write_master_record(); 
            return(searchkey);
         }
   }
   sys$free(&master_file.rab);
   return(FALSE);
}

uint16 quote_allocate()
{
   static uint16 searchkey;

   if(!lock_read_master()) {
    /*  warn_user("Master Locked"); */
      return(FALSE);
   }

   quote_file.rab.rab$b_krf = INVC_QUOTE_KEY;
   quote_file.rab.rab$l_kbf = (char *)&searchkey;
   quote_file.rab.rab$b_ksz = SIZE_BN2;
   quote_file.rab.rab$b_rac = RAB$C_KEY;
   quote_file.rab.rab$l_rop = RAB$M_CDK;  /* check for duplicate key  */

   while((searchkey=master_buff.quote_no++) < 65535) {

      rms_status = sys$find(&quote_file.rab);

      if (!rmstest(rms_status,2,RMS$_NORMAL,RMS$_RNF)) {
         warn_user("$CHECK %s",rmserror(rms_status,&quote_file));
         break;          
      } else
         if (rms_status == RMS$_RNF) {
            write_master_record(); 
            return(searchkey);
         }
   }
   sys$free(&master_file.rab);
   return(FALSE);
}

uint32 rcpt_allocate()
{
   static uint32 searchkey;

   if(!lock_read_master()) {
/*      warn_user("Master Locked");*/
      return(FALSE);
   }

   transaction_file.rab.rab$b_krf = TRANS_RCPT_KEY;
   transaction_file.rab.rab$l_kbf = (char *)&searchkey;
   transaction_file.rab.rab$b_ksz = SIZE_BN4;
   transaction_file.rab.rab$b_rac = RAB$C_KEY;
   transaction_file.rab.rab$l_rop = RAB$M_CDK;  /* check for duplicate key  */

   while((searchkey=master_buff.receipt_no++) < 65535) {

      rms_status = sys$find(&transaction_file.rab);

      if (!rmstest(rms_status,2,RMS$_NORMAL,RMS$_RNF)) {
         warn_user("$CHECK %s",rmserror(rms_status,&transaction_file));
         break;          
      } else
         if (rms_status == RMS$_RNF) {
            write_master_record(); 
            return(searchkey);
         }
   }
   sys$free(&master_file.rab);
   return(FALSE);
}

int write_transaction (enum trans_type type, int whole,uint32 cust,uint32 num, float amnt )
{

      memset (&transaction_buff, 0, sizeof (transaction_buff));

      get_time (&transaction_buff.date);

      transaction_buff.debit_flag = (type==debit)?TRUE:FALSE;
      transaction_buff.wholesale_flag = whole;
      transaction_buff.cust_no = cust;
      if(type==debit)
         transaction_buff.invc_no = num;
      else
         transaction_buff.rcpt_no = num;

      transaction_buff.amnt = floorf(amnt * 100) / 100;   /* Truncate to 2 decimal places */
  
      transaction_file.rab.rab$b_rac = RAB$C_KEY;
      transaction_file.rab.rab$l_rbf = (char *)&transaction_buff;
      transaction_file.rab.rab$w_rsz = sizeof(transaction_buff);;
 
      rms_status = sys$put(&transaction_file.rab);

     if (!rmstest (rms_status, 2, RMS$_NORMAL,RMS$_OK_DUP))
     {
        printf ("$PUT %s\n", rmserror(rms_status,&transaction_file));
        return FALSE;
     }
     return TRUE;

}


int mkwsflag(char type)
{
 
  switch(type) {
     case 'R' : return(0);
     case 'W' : return(1);
     case 'C' : return(2);
     default  : return(0);  /* unknown returns default */
  }

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


void
heading_invc (funct, q_buff, p_buff )
     enum funct_type funct;
     struct quote_rec q_buff;
     struct primary_rec p_buff;
{
  struct timebuff adjtime;
  int iter,pageno = 1;
  char statepcode[64];

  get_time (&adjtime);

  switch (funct)
    {
    case header:

      for (iter = 0; iter < 4; iter++)
        fprintf (stdprn, "\n");

      fprintf (stdprn,"%s Invoice for Client %5.5u\n",txt_type(whole_flag),p_buff.joic_no);

      fprintf (stdprn, "Invoice Number : %5.5u      Quote Number : %5.5u\n",
               q_buff.invoice_no, q_buff.quote_no);


      fprintf (stdprn, "Invoice Date   : %-11s     ",
               print_date (&q_buff.invoice_date));
      fprintf (stdprn, "%s : %s\n", (reprint) ? "RePrint" : "Printed",
               print_time (&adjtime));


      fprintf (stdprn,
               "Order Number   : %-10s     Tax Code : %c   Tax Number : %-10s\n\n",
               q_buff.order_num, q_buff.tax_code, q_buff.tax_num);

      fprintf(stdprn,"Page %u\n",pageno++);
      fprintf (stdprn, "\n");
      if(postal == 'Y') {
         fprintf (stdprn, "%8s%-30s%10s%-30s\n","To : ", p_buff.name,"From : ",master_buff.address1);
         fprintf (stdprn, "%8s%-30s%10s%-30s\n","", p_buff.p_address1,"",master_buff.address2);
         fprintf (stdprn, "%8s%-30s%10s%-30s\n","", p_buff.p_address2,"",master_buff.address3);
         fprintf (stdprn, "%8s%-30s%10s%-30s\n","", p_buff.p_address3,"",master_buff.address4);
         fprintf (stdprn, "%8s%-30s%10s%-30s\n","", p_buff.p_address4,"",master_buff.address5);
         fprintf (stdprn, "%8s%-30s%10s%-30s\n","Email : ", p_buff.email,"Email : ",master_buff.email_address);

      } else {
         sprintf (statepcode,"%s, %s",p_buff.state, p_buff.postcode);
         fprintf (stdprn, "%8s%-30s%10s%-30s\n","To : ",p_buff.name,"From : ",master_buff.address1);
         fprintf (stdprn, "%8s%-30s%10s%-30s\n","",p_buff.address,"",master_buff.address2);
         fprintf (stdprn, "%8s%-30s%10s%-30s\n","",p_buff.suburb,"",master_buff.address3);
         fprintf (stdprn, "%8s%-30s%10s%-30s\n","",statepcode,"",master_buff.address4);
         fprintf (stdprn, "%8s%-30s%10s%-30s\n","",p_buff.country,"",master_buff.address5);
         fprintf (stdprn, "%8s%-30s%10s%-30s\n","Email : ", p_buff.email,"Email : ",master_buff.email_address);

      }
      fprintf (stdprn, "\n");





      break;
    case cancel:
      fprintf (stdprn, "*****      REPORT  TERMINATED BY  USER  at %s",
               print_time (&adjtime));
      break;
    case ende:

      if(list_count > 1)
         fprintf (stdprn, "%65s %10.2f\n", "Total :",total_pretax);

      fprintf (stdprn, "\n");
      
      for (iter = 0; iter < 5; iter++)
        if (tax[iter] != 0.0)
          fprintf (stdprn, "%55s%c %3.2f%% : %10.2f\n", "Tax at Rate ",
                   iter + 'A', master_buff.sales_tax[iter], tax[iter]);

      if (q_buff.discount != 0.0) {
        fprintf (stdprn, "%65s %10.2f\n", "- Discount :", q_buff.discount);
      }
        fprintf (stdprn, "%57s Total : %10.2f\n",
               txt_type(whole_flag), total);

      fprintf (stdprn, "\n");
      if(!reprint)
         fprintf (stdprn, "%65s %10.2f %s\n", "Previous Balance :",fabs(previous_ballance),
                                             drcr(previous_ballance));
      fprintf (stdprn, "%65s %10.2f\n", "This Invoice :",total);

      previous_ballance = ballance(q_buff.cust_no);
      if(!reprint)
         fprintf (stdprn, "%65s %10.2f %s\n", "Balance Owing :",fabs(previous_ballance),
                                             drcr(previous_ballance));


/*      fputc (EJECT, stdprn);*/
      break;
    }
}

float result(int wflag,struct quote_part_rec *varl)
{
   
   switch(wflag) {
      case 0 : return(varl->retail);
      case 1 : return(varl->wholesale);
      case 2 : return(varl->cost_price);
      default: return(0.0);
   }
}


void process_invc (struct quote_rec q_buff, struct primary_rec p_buff)
{
  struct quote_part_rec *varl;
  int linecount;
  int iter;
  float sub_total = 0.0, total_tax = 0.0;

  heading_invc (header,q_buff,p_buff);

  linecount = 0;
  total = 0.0;
  memset (tax, 0, sizeof (tax));

  fprintf (stdprn, "For     : %-70s\n", q_buff.quote_for);

  fprintf (stdprn, "Comment : %-70s\n%-79s\n%-79s\n\n", q_buff.comment1,
           q_buff.comment2, q_buff.comment3);
  linecount += 5;
  if (itemize_flag)
    fprintf (stdprn, "%-12s|%-30s|%7s   |%-5s|Tax|%10s\n", "Code",
             "Description", "Number", "Units", "Price");

   
  varl =
    (struct quote_part_rec *) (quote_file.fileio +
                               (iter = sizeof (quote_buff)));
  list_count = 0;
  for (; iter < q_buff.reclen;
       iter += sizeof (struct quote_part_rec), varl++)
    {
      list_count++;
      if (itemize_flag)
        {
          if (master_buff.invoice_lnth > 0 && linecount > master_buff.invoice_lnth)
            {
              fputc (EJECT, stdprn);
              heading_invc (header,q_buff,p_buff);
              if (itemize_flag)
                fprintf (stdprn, "%-12s|%-30s|%7s   |%-5s|Tax|%10s\n", "Code",
                         "Description", "Number", "Units", "Price");
              linecount = 0;
            }
          if (strcmp (varl->kit_part_code, "SUBTOT") == 0)
            {
              fprintf (stdprn, "%65s %10.2f\n", "Subtotal:", sub_total);
              sub_total = 0.0;
            }
          else if (varl->kit_part_code[0] == 0)
            fprintf (stdprn, "%-12s %-30s %-5s\n", varl->kit_part_code,
                     varl->description, varl->units);
          else
            fprintf (stdprn,"%-12s %-30s %7.2f    %-5s  %c   %10.2f\n",
                     varl->kit_part_code, varl->description, varl->number,
                     varl->units, varl->tax_code,
                     result(whole_flag,varl));
          linecount++;
          fflush(stdprn);
        }
      total += result(whole_flag,varl);
      sub_total += result(whole_flag,varl);



#ifdef TAX_OPTION1
      if (varl->tax_code != 'X')
        tax[varl->tax_code - 'A'] +=
          result(whole_flag,varl);
#else
#ifdef TAX_OPTION2
      if (varl->tax_code != 'X')
        tax[varl->tax_code - 'A'] += varl->wholesale;
#else
#ifdef TAX_OPTION3
      if (varl->tax_code != 'X')
        tax[varl->tax_code - 'A'] += varl->retail;
#else
      if (varl->tax_code != 'X')
        tax[varl->tax_code - 'A'] += varl->cost_price;
#endif
#endif
#endif


    }

  total_pretax = total;

  for (iter = 0; iter < 5; iter++)
    {
      tax[iter] *= (master_buff.sales_tax[iter] / 100);
      total_tax += tax[iter];
    }

  total += total_tax; 
  total -= q_buff.discount;


}

