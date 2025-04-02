
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include ints
#include string
#include ctype
#include errno

#include smgdef
#include smg$routines
#include rms
#include starlet


#include "extools.h"
#include "screen.h"
#include "warns.h"
#include "filebrowse.h"
#include "swoptions.h"
#include "swfile.h"


#define SHEADING "Select Inventory Item"
#define SHEADING2 "Select Kit"

#define TEXT_LINE 19  /* should be the same as QUOTETEXTLN in invoice.c*/

struct quote_part_rec quotesed_buff;
int inhibit_display = FALSE;
float non_inv_price, non_inv_number;
char non_inv_units[7];

static int field_row,field_col,noninv_flag = FALSE;

int BTRV ();

extern int TopLine;
extern int BottomLine;
extern int StatusLine;
extern int RegionLength;
extern int HelpLine;
extern int win_depth;

extern struct master_rec master_master;

void strupr (char *);
void *lmalloc (size_t);



extern struct scr_params quote_entry_screen[];

void invent_search ();
void kit_search ();

static struct cmndline_params invent_cmndline[] = {
  1, NULL, NULL, 0, NULL,
  F1, "Search", NULL, 0, invent_search
};

static struct cmndline_params kit_cmndline[] = {
  1, NULL, NULL, 0, NULL,
  F1, "Search", NULL, 0, kit_search
};

void inventory_trans ();
int inventory_keymatch ();

int quote_select_inventory_item ();
int get_item ();
int calc_costs (float *);


/* struct kit_rec kit_buff; */
struct kit_key0_rec kit_key;
int kit_lnth;

void kit_trans ();
int kit_keymatch ();
int select_kit (struct quote_part_rec *, struct filestruct *);
int quotesed (char *,int,int, int, int, int *,unsigned int *,struct filestruct *);


static void
slct_invent (struct linedt_msg_rec *msg, struct filestruct *filedef)
{
  quote_select_inventory_item (&quotesed_buff, filedef);
}


static void
slct_kit (struct linedt_msg_rec *msg, struct filestruct *filedef)
{
  select_kit (&quotesed_buff, filedef);
}

void subtot ();
void non_inventory_item ();
void non_inventory_stub ();
static void
subs ()
{
  subtot (&quotesed_buff);
}

static void
nons ()
{
  if(strcmp(quotesed_buff.kit_part_code,"NONINV") == 0 || strlen(quotesed_buff.kit_part_code) == 0) {
     non_inventory_item (&quotesed_buff);
     calc_costs(&quotesed_buff.number);
     noninv_flag = TRUE;
  }
}

void recalc_fkey (struct linedt_msg_rec *, struct filestruct *);

static struct cmndline_params cmnd_line[] = {
  4, NULL, NULL, 0, NULL,
  F1, "Inventory", NULL, 0, slct_invent,
  F2, "Kits", NULL, 0, slct_kit,
  F4, "Recalculate", NULL, 0, recalc_fkey,
  F3, "Non Inventory", NULL, 0, nons
};



int calc_costs ();
int check_tax (char *);
int
calc_costs_tax (char *test)
{
  return (check_tax (test) && calc_costs (&quotesed_buff.number));
}


#define PROMPTCOL 0
#define FIELDCOL SMG$M_BOLD
#define HEADCOL SMG$M_BOLD

/* *INDENT-OFF* */
#define LINEDLEN 7
struct scr_params quote_line_all[] = {
   0,0,"",PROMPTCOL,UNFMT_STRING,quotesed_buff.kit_part_code,FIELDCOL,10,get_item,cmnd_line,
   0,10,"|",PROMPTCOL,UNFMT_STRING,quotesed_buff.description,FIELDCOL,28,NULL,cmnd_line,
   0,39,"|%7.2f",PROMPTCOL,NUMERIC,&quotesed_buff.number,FIELDCOL,7,calc_costs,NULL,
   0,47,"|",PROMPTCOL,UNFMT_STRING|NON_EDIT,quotesed_buff.units,FIELDCOL,6,NULL,NULL,
   0,54,"|",PROMPTCOL,SINGLE_CHAR|CAPITALIZE,&quotesed_buff.tax_code,FIELDCOL,1,calc_costs_tax,NULL,
   0,56,"|%10.2f",PROMPTCOL,NUMERIC|NON_EDIT,&quotesed_buff.cost_price,FIELDCOL,9,NULL,NULL,
   0,67,"|%10.2f",PROMPTCOL,NUMERIC|NON_EDIT,&quotesed_buff.wholesale,FIELDCOL,9,NULL,NULL
};

struct scr_params quote_line_all_retail[] = {
   0,0,"",PROMPTCOL,UNFMT_STRING,quotesed_buff.kit_part_code,FIELDCOL,10,get_item,cmnd_line,
   0,10,"|",PROMPTCOL,UNFMT_STRING,quotesed_buff.description,FIELDCOL,28,NULL,cmnd_line,
   0,39,"|%7.2f",PROMPTCOL,NUMERIC,&quotesed_buff.number,FIELDCOL,7,calc_costs,NULL,
   0,47,"|",PROMPTCOL,UNFMT_STRING|NON_EDIT,quotesed_buff.units,FIELDCOL,6,NULL,NULL,
   0,54,"|",PROMPTCOL,SINGLE_CHAR|CAPITALIZE,&quotesed_buff.tax_code,FIELDCOL,1,calc_costs_tax,NULL,
   0,56,"|%10.2f",PROMPTCOL,NUMERIC|NON_EDIT,&quotesed_buff.cost_price,FIELDCOL,10,NULL,NULL,
   0,67,"|%10.2f",PROMPTCOL,NUMERIC|NON_EDIT,&quotesed_buff.retail,FIELDCOL,10,NULL,NULL
};


struct scr_params quote_line_whole[] = {
   0,0,"",PROMPTCOL,UNFMT_STRING,quotesed_buff.kit_part_code,FIELDCOL,10,get_item,cmnd_line,
   0,10,"|",PROMPTCOL,UNFMT_STRING,quotesed_buff.description,FIELDCOL,28,NULL,cmnd_line,
   0,39,"|%7.2f",PROMPTCOL,NUMERIC,&quotesed_buff.number,FIELDCOL,7,calc_costs,NULL,
   0,47,"|",PROMPTCOL,UNFMT_STRING|NON_EDIT,quotesed_buff.units,FIELDCOL,6,NULL,NULL,
   0,54,"|",PROMPTCOL,SINGLE_CHAR|CAPITALIZE,&quotesed_buff.tax_code,FIELDCOL,1,calc_costs_tax,NULL,
   0,56,"|",PROMPTCOL,0,NULL,0,0,NULL,NULL,
   0,67,"|%10.2f",PROMPTCOL,NUMERIC|NON_EDIT,&quotesed_buff.wholesale,FIELDCOL,10,NULL,NULL
};

struct scr_params quote_line_retail[] = {
   0,0,"",PROMPTCOL,UNFMT_STRING,quotesed_buff.kit_part_code,FIELDCOL,10,get_item,cmnd_line,
   0,10,"|",PROMPTCOL,UNFMT_STRING,quotesed_buff.description,FIELDCOL,28,NULL,cmnd_line,
   0,39,"|%7.2f",PROMPTCOL,NUMERIC,&quotesed_buff.number,FIELDCOL,7,calc_costs,NULL,
   0,47,"|",PROMPTCOL,UNFMT_STRING|NON_EDIT,quotesed_buff.units,FIELDCOL,6,NULL,NULL,
   0,54,"|",PROMPTCOL,SINGLE_CHAR|CAPITALIZE,&quotesed_buff.tax_code,FIELDCOL,1,calc_costs_tax,NULL,
   0,56,"|",PROMPTCOL,0,NULL,0,0,NULL,NULL,
   0,67,"|%10.2f",PROMPTCOL,NUMERIC|NON_EDIT,&quotesed_buff.retail,FIELDCOL,10,NULL,NULL
};

struct scr_params quote_line_retail_whole[] = {
   0,0,"",PROMPTCOL,UNFMT_STRING,quotesed_buff.kit_part_code,FIELDCOL,10,get_item,cmnd_line,
   0,10,"|",PROMPTCOL,UNFMT_STRING,quotesed_buff.description,FIELDCOL,28,NULL,cmnd_line,
   0,39,"|%7.2f",PROMPTCOL,NUMERIC,&quotesed_buff.number,FIELDCOL,7,calc_costs,NULL,
   0,47,"|",PROMPTCOL,UNFMT_STRING|NON_EDIT,quotesed_buff.units,FIELDCOL,6,NULL,NULL,
   0,54,"|",PROMPTCOL,SINGLE_CHAR|CAPITALIZE,&quotesed_buff.tax_code,FIELDCOL,1,calc_costs_tax,NULL,
   0,56,"|%10.2f",PROMPTCOL,NUMERIC|NON_EDIT,&quotesed_buff.retail,FIELDCOL,10,NULL,NULL,
   0,67,"|%10.2f",PROMPTCOL,NUMERIC|NON_EDIT,&quotesed_buff.wholesale,FIELDCOL,10,NULL,NULL
};

/* *INDENT-ON* */


static struct scr_params *quote_line;
static int linlen;

struct quote_linerec
{
  struct quote_linerec *next;
  struct quote_linerec *last;
  struct quote_part_rec storage;
};

struct taxable_rec
{
  float w;
  float r;
  float c;
};


int
quotesed_totals (int deleting, int disc)
{
  char outbuff[81], test;
  float cost = 0, wholesale = 0, retail = 0, wtax = 0, rtax = 0, ctax = 0;
  struct taxable_rec taxable[5];
  struct quote_linerec *crnt_line =
    quote_entry_screen[TEXT_LINE].field, *del_test_line;
  int lnth, status, key, iter;

  del_test_line = crnt_line;
  if (crnt_line != NULL)
    {

      read_master_record ();

      memset (taxable, 0, sizeof (taxable));

      while (crnt_line->last != NULL)
        crnt_line = crnt_line->last;
      for (; crnt_line != NULL; crnt_line = crnt_line->next)
        {
          if (del_test_line == crnt_line && !disc)
            {
              cost += quotesed_buff.cost_price;
              wholesale += quotesed_buff.wholesale;
              retail += quotesed_buff.retail;
#ifdef TAX_OPTION1
              if (quotesed_buff.tax_code != 'X')
                {
                  taxable[quotesed_buff.tax_code - 'A'].r +=
                    quotesed_buff.retail;
                  taxable[quotesed_buff.tax_code - 'A'].w +=
                    quotesed_buff.wholesale;
                  taxable[quotesed_buff.tax_code - 'A'].c +=
                    quotesed_buff.cost_price;
                }
#else
#ifdef TAX_OPTION2
              if (quotesed_buff.tax_code != 'X')
                {
                  taxable[quotesed_buff.tax_code - 'A'].r +=
                    quotesed_buff.wholesale;
                  taxable[quotesed_buff.tax_code - 'A'].w +=
                    quotesed_buff.wholesale;
                }
#else
#ifdef TAX_OPTION3
              if (quotesed_buff.tax_code != 'X')
                {
                  taxable[quotesed_buff.tax_code - 'A'].r +=
                    quotesed_buff.retail;
                  taxable[quotesed_buff.tax_code - 'A'].w +=
                    quotesed_buff.retail;
                }
#else
              if (quotesed_buff.tax_code != 'X')
                {
                  taxable[quotesed_buff.tax_code - 'A'].r +=
                    quotesed_buff.cost_price;
                  taxable[quotesed_buff.tax_code - 'A'].w +=
                    quotesed_buff.cost_price;
                }
#endif
#endif
#endif

            }
          else
            {
              cost += crnt_line->storage.cost_price;
              wholesale += crnt_line->storage.wholesale;
              retail += crnt_line->storage.retail;

#ifdef TAX_OPTION1
              if (crnt_line->storage.tax_code != 'X')
                {
                  taxable[crnt_line->storage.tax_code - 'A'].r +=
                    crnt_line->storage.retail;
                  taxable[crnt_line->storage.tax_code - 'A'].w +=
                    crnt_line->storage.wholesale;
                  taxable[crnt_line->storage.tax_code - 'A'].c +=
                    crnt_line->storage.cost_price;
                }
#else
#ifdef TAX_OPTION2
              if (crnt_line->storage.tax_code != 'X')
                {
                  taxable[crnt_line->storage.tax_code - 'A'].r +=
                    crnt_line->storage.wholesale;
                  taxable[crnt_line->storage.tax_code - 'A'].w +=
                    crnt_line->storage.wholesale;
                }
#else
#ifdef TAX_OPTION3
              if (crnt_line->storage.tax_code != 'X')
                {
                  taxable[crnt_line->storage.tax_code - 'A'].r +=
                    crnt_line->storage.retail;
                  taxable[crnt_line->storage.tax_code - 'A'].w +=
                    crnt_line->storage.retail;
                }
#else
              if (crnt_line->storage.tax_code != 'X')
                {
                  taxable[crnt_line->storage.tax_code - 'A'].r +=
                    crnt_line->storage.cost_price;
                  taxable[crnt_line->storage.tax_code - 'A'].w +=
                    crnt_line->storage.cost_price;
                }
#endif
#endif
#endif

            }
        }

      if (deleting
          && !(del_test_line->last == NULL && del_test_line->next == NULL))
        {
          retail -= quotesed_buff.retail;
          wholesale -= quotesed_buff.wholesale;
          cost -= quotesed_buff.cost_price;


#ifdef TAX_OPTION1
          if (quotesed_buff.tax_code != 'X')
            {
              taxable[quotesed_buff.tax_code - 'A'].r -= quotesed_buff.retail;
              taxable[quotesed_buff.tax_code - 'A'].w -=
                quotesed_buff.wholesale;
              taxable[quotesed_buff.tax_code - 'A'].c -=
                quotesed_buff.cost_price;
            }
#else
#ifdef TAX_OPTION2
          if (quotesed_buff.tax_code != 'X')
            {
              taxable[quotesed_buff.tax_code - 'A'].r -=
                quotesed_buff.wholesale;
              taxable[quotesed_buff.tax_code - 'A'].w -=
                quotesed_buff.wholesale;
            }
#else
#ifdef TAX_OPTION3
          if (quotesed_buff.tax_code != 'X')
            {
              taxable[quotesed_buff.tax_code - 'A'].r -= quotesed_buff.retail;
              taxable[quotesed_buff.tax_code - 'A'].w -= quotesed_buff.retail;
            }
#else
          if (quotesed_buff.tax_code != 'X')
            {
              taxable[quotesed_buff.tax_code - 'A'].r -=
                quotesed_buff.cost_price;
              taxable[quotesed_buff.tax_code - 'A'].w -=
                quotesed_buff.cost_price;
            }
#endif
#endif
#endif


        }


      for (iter = 0; iter < 5; iter++)
        {
          if (wholesale != 0.0)
          wtax += taxable[iter].w * (master_buff.sales_tax[iter] / 100.0);
        }
      for (iter = 0; iter < 5; iter++)
        {
          if (retail != 0.0)
             rtax += taxable[iter].r * (master_buff.sales_tax[iter] / 100.0);
        }
      for (iter = 0; iter < 5; iter++)
        {
          if (cost != 0.0)
             ctax += taxable[iter].c * (master_buff.sales_tax[iter] / 100.0);
        }


      wholesale += wtax;
      retail += rtax;
      cost += ctax;

      wholesale -= quote_buff.discount;
      retail -= quote_buff.discount;
      cost -= quote_buff.discount;

    }
  test = master_master.d_flag_w;
  sprintf (outbuff, "%55s%10.2f %10.2f", "Tax.", (test == 'Y') ? rtax : ctax, /*0.0, */
           (test == 'A' || test == 'W' || test == 'Y') ? wtax : rtax);
  exclreol (18, 2, 0);
  exscrprn (outbuff, 18, 2, WHITE);
  sprintf (outbuff, "%55s%10.2f %10.2f", "Totals.",
           (test == 'A'
            || test == 'X') ? cost : (test == 'Y') ? retail : 0.0,
           (test == 'A' || test == 'W' || test == 'Y') ? wholesale : retail);
  exclreol (19, 2, 0);
  exscrprn (outbuff, 19, 2, WHITE);
  return (TRUE);
}


int
calc_costs (float *number)
{
  int lnth, status, key, maxtext, kit_lnth, iter;
  struct kit_part_rec *varl;

  struct kit_rec kit_buff;
  struct kit_key0_rec kit_key;
  struct inventory_key0_rec inventory_key;

  float cost = 0;
  struct kit_rec *dtr = (struct kit_rec *) kit_file.fileio;


  read_master_record ();


  if (strcmp (quotesed_buff.kit_part_code, "NONINV") == 0)
    {

/*       quotesed_buff.wholesale = quotesed_buff.cost_price + (quotesed_buff.cost_price * (master_buff.wholesale_markup/100));
       quotesed_buff.retail = quotesed_buff.cost_price + (quotesed_buff.cost_price * (master_buff.retail_markup/100));
/* no markup on non inventory items */

      quotesed_buff.wholesale = quotesed_buff.cost_price;
      quotesed_buff.retail = quotesed_buff.cost_price;
    }
  else if (quotesed_buff.kit)
    {
      strcpy (kit_key.kit_code, quotesed_buff.kit_part_code);
      pad (kit_key.kit_code, PARTKEYLEN);

      kit_file.rab.rab$b_krf = KIT_PART_KEY;
      kit_file.rab.rab$l_kbf = (char *) &kit_key;
      kit_file.rab.rab$b_ksz = sizeof (kit_key);
      kit_file.rab.rab$b_rac = RAB$C_KEY;
      kit_file.rab.rab$l_ubf = (char *) kit_file.fileio;        /*record; */
      kit_file.rab.rab$w_usz = kit_file.fileiosz;
      kit_file.rab.rab$l_rop = RAB$M_NLK;

      rms_status = sys$get (&kit_file.rab);

      if (!rmstest (rms_status, 3, RMS$_NORMAL, RMS$_OK_RLK, RMS$_RNF))
        {
          warn_user ("$GET %s", rmserror(rms_status,&kit_file));
          return (TRUE);
        }

      if (rms_status == RMS$_RNF)
        warn_user ("Kit %s Not Found", quotesed_buff.kit_part_code);
      else
        {
          memcpy (&kit_buff, kit_file.fileio, sizeof (kit_buff));
          unpad (kit_buff.kit_code, PARTKEYLEN);
          varl =
            (struct kit_part_rec *) (kit_file.fileio +
                                     (iter = sizeof (kit_buff)));

          for (; iter < dtr->reclen;
               varl++, iter += sizeof (struct kit_part_rec))
            {
              if (varl->part_code[0] == 0)
                continue;


              strcpy (inventory_key.part_code, varl->part_code);
              pad (inventory_key.part_code, PARTKEYLEN);

              inventory_file.rab.rab$b_krf = INV_PART_KEY;
              inventory_file.rab.rab$l_kbf = (char *) &inventory_key;
              inventory_file.rab.rab$b_ksz = sizeof (inventory_key);
              inventory_file.rab.rab$b_rac = RAB$C_KEY;
              inventory_file.rab.rab$l_ubf = (char *) &inventory_buff;  /*record; */
              inventory_file.rab.rab$w_usz = sizeof (inventory_buff);
              inventory_file.rab.rab$l_rop = RAB$M_NLK;

              rms_status = sys$get (&inventory_file.rab);

              if (!rmstest
                  (rms_status, 3, RMS$_NORMAL, RMS$_OK_RLK, RMS$_RNF))
                warn_user ("$GET %s %s",
                           varl->part_code,
                           rmserror(rms_status,&inventory_file));


              if (rms_status != RMS$_RNF)
                cost += inventory_buff.price * varl->number;
              else
                warn_user ("In Kit %s Part %s Not Found", kit_buff.kit_code,
                           varl->part_code);

            }
        }
    }
  else
    {
      if (strcmp (quotesed_buff.kit_part_code, "NONINV") != 0 &&
          strcmp (quotesed_buff.kit_part_code, "SUBTOT") != 0 &&
          strcmp (quotesed_buff.kit_part_code, "") != 0)
        {

          strcpy (inventory_key.part_code, quotesed_buff.kit_part_code);
          pad (inventory_key.part_code, PARTKEYLEN);

          inventory_file.rab.rab$b_krf = INV_PART_KEY;
          inventory_file.rab.rab$l_kbf = (char *) &inventory_key;
          inventory_file.rab.rab$b_ksz = sizeof (inventory_key);
          inventory_file.rab.rab$b_rac = RAB$C_KEY;
          inventory_file.rab.rab$l_ubf = (char *) &inventory_buff;      /*record; */
          inventory_file.rab.rab$w_usz = sizeof (inventory_buff);
          inventory_file.rab.rab$l_rop = RAB$M_NLK;

          rms_status = sys$get (&inventory_file.rab);

          if (!rmstest (rms_status, 3, RMS$_NORMAL, RMS$_OK_RLK, RMS$_RNF))
            warn_user ("$GET %s %s",
                       quotesed_buff.kit_part_code, rmserror(rms_status,&inventory_file));


          if (rms_status != RMS$_RNF)
            cost = inventory_buff.price;
          else
            warn_user ("Part %s Not Found", quotesed_buff.kit_part_code);
        }
    }


  if (strcmp (quotesed_buff.kit_part_code, "NONINV") == 0)
    /*quotesed_buff.cost_price *= (*number) */ ;
  else
    quotesed_buff.cost_price = cost * (*number);

  if (strcmp (quotesed_buff.kit_part_code, "NONINV") == 0)
    {
      quotesed_buff.wholesale = quotesed_buff.cost_price;
      quotesed_buff.retail = quotesed_buff.cost_price;
    }
  else
    {
      if (!quotesed_buff.kit || kit_buff.price_offset == 0.0)
        {
          quotesed_buff.wholesale =
            quotesed_buff.cost_price +
            (quotesed_buff.cost_price * (master_buff.wholesale_markup / 100));
          quotesed_buff.retail =
            quotesed_buff.cost_price +
            (quotesed_buff.cost_price * (master_buff.retail_markup / 100));
        }
      else
        {
          cost += kit_buff.price_offset;
          cost *= (*number);
          quotesed_buff.wholesale =
            cost + (cost * (master_buff.wholesale_markup / 100));
          quotesed_buff.retail =
            cost + (cost * (master_buff.retail_markup / 100));
        }
    }

  quotesed_totals (FALSE, FALSE);
  if (!inhibit_display)
    {
      exclreol (quote_line[0].row, quote_line[0].col, 0);
      init_screen (quote_line, linlen);
    }
  return (TRUE);
}

int
recalc_costs (msg, filedef)
     struct linedt_msg_rec *msg;
     struct filestruct *filedef;
{
  struct quote_linerec *crnt_line =
    filedef->entry_screen[filedef->entry_comment].field, *top;
  unsigned int iter, status, maxtext;
  struct quote_key0_rec quote_key;
  char *block;

  if (filedef->CurrentPtr == NULL)
    return (FALSE);

  filedef->rab.rab$b_rac = RAB$C_RFA;
  rfa_copy (&filedef->rab.rab$w_rfa, &filedef->CurrentPtr->rfa);

/*   filedef->rab.rab$l_ubf = (char *)filedef->fileio;
   filedef->rab.rab$w_usz = filedef->fileiosz; */
  filedef->rab.rab$l_rop = RAB$M_RLK;

  rms_status = sys$find (&filedef->rab);

  if (!rmstest (rms_status, 2, RMS$_NORMAL, RMS$_RLK))
    {
      warn_user ("$FIND %s", rmserror(rms_status,filedef));
      return (FALSE);
    }
  if (rmstest (rms_status, 1, RMS$_RLK))
    {
      warn_user ("%s", strerror (EVMSERR, rms_status));
      return (FALSE);
    }


  while (crnt_line->last != NULL)
    crnt_line = crnt_line->last;
  top = crnt_line;
  for (; crnt_line != NULL; crnt_line = crnt_line->next)
    {
      inhibit_display = TRUE;
      memcpy (&quotesed_buff, &crnt_line->storage, sizeof (quotesed_buff));
      calc_costs (&crnt_line->storage.number);
      memcpy (&crnt_line->storage, &quotesed_buff, sizeof (quotesed_buff));
      inhibit_display = FALSE;
    }

  block = quote_file.fileio;
  memcpy (&quote_buff, block, sizeof (quote_buff));
  maxtext = quote_buff.reclen;

  iter = sizeof (quote_buff);
  for (crnt_line = top; crnt_line != NULL && iter < maxtext;
       crnt_line = crnt_line->next)
    {
      memcpy ((block + iter), &crnt_line->storage, sizeof (quotesed_buff));
      iter += sizeof (quotesed_buff);
    }

  quote_buff.reclen = maxtext;  /*iter; */

  memcpy (block, &quote_buff, sizeof (quote_buff));

  quote_file.rab.rab$b_rac = RAB$C_KEY;
  quote_file.rab.rab$l_rbf = (char *) block;
  quote_file.rab.rab$w_rsz = iter;

  /* pad() */

  rms_status = sys$update (&quote_file.rab);
  if (!rmstest (rms_status, 3, RMS$_NORMAL, RMS$_DUP, RMS$_OK_DUP))
    {
      warn_user ("PUT/UPD %s", rmserror(rms_status,&quote_file));
      return (FALSE);
    }

  return (TRUE);

}






void
subtot (struct quote_part_rec *crnt_line)
{

  memset (crnt_line, 0, sizeof (struct quote_part_rec));
  strcpy (crnt_line->kit_part_code, "SUBTOT");
  crnt_line->tax_code = 'A';

  exclreol (quote_line[0].row, quote_line[0].col, 0);
  init_screen (quote_line, linlen);

}

/* *INDENT-OFF* */
#define SCRNLEN 3
struct scr_params noninv_screen[] = {
 /*   0,5,"Non Inventory Item ",HEADCOL,0,NULL,0,0,NULL,NULL, */
    0,0,"Units          : ",PROMPTCOL,UNFMT_STRING,non_inv_units,FIELDCOL,6,NULL,NULL,
    1,0,"Price Per Unit : %10.2f",PROMPTCOL,NUMERIC,&non_inv_price,FIELDCOL,10,NULL,NULL,
    2,0,"Number of Units: %10.2f",PROMPTCOL,NUMERIC,&non_inv_number,FIELDCOL,10,NULL,NULL
};
/* *INDENT-ON* */

void
non_inventory_item (struct quote_part_rec *crnt_line)
{

  int edited = FALSE, retkey, strtfld = 0;

  open_window (crnt_window->rul+field_row-SCRNLEN-1,crnt_window->cul+field_col+5, 32, SCRNLEN, WHITE, border);
  init_screen (noninv_screen, SCRNLEN);
  do {
     retkey = enter_screen (noninv_screen, SCRNLEN, &edited, breakout, NULL, &strtfld);
     if(retkey == SMG$K_TRM_UP && strtfld > 0) strtfld--;
     else if(retkey == SMG$K_TRM_DOWN && strtfld < SCRNLEN-1) strtfld++;
  } while (retkey != SMG$K_TRM_CTRLZ && retkey != SMG$K_TRM_CR);
  close_window ();
  
  if (edited && (retkey == SMG$K_TRM_CR) && (non_inv_number != 0.0))
    {
      if (strcmp (crnt_line->kit_part_code, "NONINV") != 0)
        {
          memset (crnt_line, 0, sizeof (struct quote_part_rec));
        }

      strcpy (crnt_line->kit_part_code, "NONINV");
      crnt_line->tax_code = 'X';
      crnt_line->number = non_inv_number;
      crnt_line->cost_price = non_inv_price * non_inv_number;
      strcpy (crnt_line->units, non_inv_units);


/* ZEN no markup on NONINV */
/*        crnt_line->wholesale = crnt_line->cost_price + (crnt_line->cost_price *
                                                               (master_buff.wholesale_markup/100));
        crnt_line->retail = crnt_line->cost_price + (crnt_line->cost_price * (master_buff.retail_markup/100));
*/

      crnt_line->wholesale = crnt_line->cost_price;
      crnt_line->retail = crnt_line->cost_price;


/* END */

      exclreol (quote_line[0].row, quote_line[0].col, 0);
      init_screen (quote_line, linlen);
    }
}


void
non_inventory_stub (struct quote_part_rec *crnt_line)
{

  if (strcmp (crnt_line->kit_part_code, "NONINV") == 0)
    non_inventory_item (crnt_line);
  else
    {
      memset (crnt_line, 0, sizeof (struct quote_part_rec));
      strcpy (crnt_line->kit_part_code, "NONINV");

      exclreol (quote_line[0].row, quote_line[0].col, 0);
      init_screen (quote_line, linlen);
    }
}




void
inventory_search_2 (int inchr, int scode, int *pos_chngd)
{


  struct inventory_key0_rec srch_key;
  char srchln[15];
  int wch, lnpos, status;
  unsigned int cont = 0;

  open_window (8, 24, 32, 5, WHITE, border);
  exscrprn ("Partial Code", 0, 9, WHITE);
  exscrprn ("Esc to Cancel", crnt_window->lnth, 2, WHITE);
  srchln[0] = inchr;
  srchln[1] = 0;
  lnpos = 2;
  wch = linedt (srchln, 1, 0, 10, WHITE, &lnpos, &cont, NULL,&inventory_file);
  close_window ();
  strupr (srchln);
  exloccur (24, 0);
  if (wch != 01)
    {                           /*scan code for ESC  */
      strcpy (srch_key.part_code, srchln);
      pad (srch_key.part_code, PARTKEYLEN);

/*
      inventory_lnth = sizeof(inventory_buff);
      status = BTRV(B_GETGE,select_scrlindx.main_file,
                            select_scrlindx.main_buff,
                            select_scrlindx.main_lnth,
                            &srch_key,
                            select_scrlindx.path_no);
*/

      inventory_file.rab.rab$b_krf = INV_PART_KEY;
      inventory_file.rab.rab$l_kbf = (char *) &srch_key;
      inventory_file.rab.rab$b_ksz = sizeof (srch_key);
      inventory_file.rab.rab$b_rac = RAB$C_KEY;
      inventory_file.rab.rab$l_ubf = (char *) &inventory_buff;  /*record; */
      inventory_file.rab.rab$w_usz = sizeof (inventory_buff);
      inventory_file.rab.rab$l_rop = RAB$M_NLK | RAB$M_EQNXT;

      rms_status = sys$get (&inventory_file.rab);

      if (!rmstest
          (rms_status, 4, RMS$_NORMAL, RMS$_RNF, RMS$_RLK, RMS$_OK_RLK))
        {
          warn_user ("$GET %s",  rmserror(rms_status,&inventory_file));
          return;
        }
      else if (rmstest (rms_status, 2, RMS$_RNF, RMS$_RLK))
        {
          warn_user ("%s", strerror (EVMSERR, rms_status));
          return;
        }



      /*     *pos_chngd = TRUE;   */
    }

}

#define QUOTE_SELECT_LENGTH 5

static int topline  (int winlnth)
{
   unsigned int RegionLength;
   smg$get_display_attr (&crnt_window->display_id, &RegionLength);
   if ((field_row+2+winlnth) > RegionLength)
      return(field_row-winlnth-1);
   else
      return(field_row+2);
}


int
quote_select_inventory_item (struct quote_part_rec *crnt_line,
                             struct filestruct *filedef)
{
  char heading[80];
  int ret, status;
  unsigned long rec_num;

/*  spawn_scroll_window (&inventory_file, 10, 63);*/

  open_window (crnt_window->rul+topline(QUOTE_SELECT_LENGTH),  /*field_row-QUOTE_SELECT_LENGTH-1,*/
               crnt_window->cul+field_col+5, 63,
               QUOTE_SELECT_LENGTH , WHITE, border);
  smg$get_display_attr (&crnt_window->display_id, &inventory_file.RegionLength);
  inventory_file.BottomLine = inventory_file.RegionLength;
  inventory_file.TopLine = 2;

  HeadLine (SHEADING, centre);

  if (!rfa_iszero (File_Browse (&inventory_file, select)))
    {
      close_window ();
/*      win_depth--; */
      Help (TEXACOHLP);
      memcpy (crnt_line->kit_part_code, inventory_buff.part_code, PARTKEYLEN);
      unpad (crnt_line->kit_part_code, PARTKEYLEN);
      strcpy (quotesed_buff.description, inventory_buff.description);
      strcpy (quotesed_buff.units, inventory_buff.units);
      quotesed_buff.kit = FALSE;
      quotesed_buff.number = 0;
      quotesed_buff.cost_price = 0;
      quotesed_buff.wholesale = 0;
      quotesed_buff.retail = 0;
      quotesed_totals (FALSE, FALSE);
      exclreol (quote_line[0].row, quote_line[0].col, BLACK);
      init_screen (quote_line, linlen);
      return (TRUE);
    }
  else
    {
      close_window ();
/*      win_depth--; */
    }
  Help (TEXACOHLP);
  return (FALSE);
}


void
kit_search_2 (int inchr, int scode, int *pos_chngd)
{



  struct kit_key0_rec srch_key;
  char srchln[15];
  int wch, lnpos, status;
  unsigned int cont = 0;

  open_window (8, 24, 32, 5, WHITE, border);
  exscrprn ("Partial Code", 0, 9, WHITE);
  exscrprn ("Esc to Cancel", crnt_window->lnth, 2, WHITE);
  srchln[0] = inchr;
  srchln[1] = 0;
  lnpos = 2;
  wch = linedt (srchln, 1, 0, 10, WHITE, &lnpos, &cont, NULL,&kit_file);
  close_window ();
  strupr (srchln);
  exloccur (24, 0);
  if (wch != 01)
    {                           /*scan code for ESC  */
      strcpy (srch_key.kit_code, srchln);
      pad (srch_key.kit_code, PARTKEYLEN);
/*
      kit_lnth = sizeof(kit_buff);
      status = BTRV(B_GETGE,kit_select_scrlindx.main_file,
                            kit_select_scrlindx.main_buff,
                            kit_select_scrlindx.main_lnth,
                            &srch_key,
                            kit_select_scrlindx.path_no);
*/
      kit_file.rab.rab$b_krf = KIT_PART_KEY;
      kit_file.rab.rab$l_kbf = (char *) &srch_key;
      kit_file.rab.rab$b_ksz = sizeof (srch_key);
      kit_file.rab.rab$b_rac = RAB$C_KEY;
      kit_file.rab.rab$l_ubf = (char *) &kit_buff;      /*record; */
      kit_file.rab.rab$w_usz = sizeof (kit_buff);
      kit_file.rab.rab$l_rop = RAB$M_NLK | RAB$M_EQNXT;

      rms_status = sys$get (&kit_file.rab);

      if (!rmstest
          (rms_status, 4, RMS$_NORMAL, RMS$_RNF, RMS$_RLK, RMS$_OK_RLK))
        {
          warn_user ("$GET %s", rmserror(rms_status,&kit_file));
          return;
        }
      else if (rmstest (rms_status, 2, RMS$_RNF, RMS$_RLK))
        {
          warn_user ("%s", strerror (EVMSERR, rms_status));
          return;
        }




      *pos_chngd = TRUE;
    }

}


#define KIT_SELECT_LENGTH 5

int
select_kit (struct quote_part_rec *crnt_line, struct filestruct *filedef)
{
  int ret, status;
  unsigned long rec_num;

  struct kit_rec *dtr = (struct kit_rec *) kit_file.fileio;

/*  spawn_scroll_window (&kit_file, 10, 44); */

  open_window (crnt_window->rul+topline(KIT_SELECT_LENGTH), /*   field_row-KIT_SELECT_LENGTH-1,*/
               crnt_window->cul+field_col+5, 44,
               KIT_SELECT_LENGTH , WHITE, border);
  smg$get_display_attr (&crnt_window->display_id, &kit_file.RegionLength);
  kit_file.BottomLine = kit_file.RegionLength;
  kit_file.TopLine = 2;
/*  win_depth++;*/


  HeadLine (SHEADING2, centre);

  if (!rfa_iszero (File_Browse (&kit_file, select)))
    {
      close_window ();
/*      win_depth--; */
      Help (TEXACOHLP);
      memcpy (crnt_line->kit_part_code, dtr->kit_code, PARTKEYLEN);
      unpad (crnt_line->kit_part_code, PARTKEYLEN);
      strcpy (quotesed_buff.description, dtr->description);
      strcpy (quotesed_buff.units, "each");
      quotesed_buff.kit = TRUE;
      quotesed_buff.number = 0;
      quotesed_buff.cost_price = 0;
      quotesed_buff.wholesale = 0;
      quotesed_buff.retail = 0;
      quotesed_totals (FALSE, FALSE);
      exclreol (quote_line[0].row, quote_line[0].col, BLACK);
      init_screen (quote_line, linlen);
      return (TRUE);
    }
  else
    {
      close_window ();
/*      win_depth--;*/
    }
  Help (TEXACOHLP);
  return (FALSE);
}

get_item (char *code)
{
  int lnth, status;
  struct inventory_rec inventory_buff;
  struct inventory_key0_rec inventory_key;
  struct kit_rec kit_buff;
  struct kit_key0_rec kit_key;
  struct kit_rec *dtr = (struct kit_rec *) kit_file.fileio;

  /* don't search for null code */
  if (strlen (code) == 0)
    return (TRUE);

  strupr (code);

  if (strcmp (code, "SUBTOT") == 0)
    {
      subtot (&quotesed_buff);
      calc_costs (&quotesed_buff.number);
      return (TRUE);
    }
  if (strcmp (code, "NONINV") == 0)
    {  
      if(!noninv_flag) {
         non_inventory_item (&quotesed_buff);
         calc_costs(&quotesed_buff.number);
      } else noninv_flag = FALSE;
      return (TRUE);
    }

  strcpy (kit_key.kit_code, code);
  pad (kit_key.kit_code, PARTKEYLEN);

/*
   lnth = sizeof(kit_buff);
   status = BTRV(B_GETEQ,kit_file,&kit_buff,&lnth,&kit_key,0);
*/
  kit_file.rab.rab$b_krf = KIT_PART_KEY;
  kit_file.rab.rab$l_kbf = (char *) &kit_key;
  kit_file.rab.rab$b_ksz = sizeof (kit_key);
  kit_file.rab.rab$b_rac = RAB$C_KEY;
  kit_file.rab.rab$l_ubf = (char *) kit_file.fileio;    /*record; */
  kit_file.rab.rab$w_usz = kit_file.fileiosz;
  kit_file.rab.rab$l_rop = RAB$M_NLK | RAB$M_EQNXT;

  rms_status = sys$get (&kit_file.rab);

  if (!rmstest
      (rms_status, 5, RMS$_NORMAL, RMS$_RNF, RMS$_RLK, RMS$_RNF, RMS$_OK_RLK))
    warn_user ("$GET %s", rmserror(rms_status,&kit_file));
  else if (rmstest (rms_status, 1, RMS$_RLK))
    {
      warn_user ("%s", strerror (EVMSERR, rms_status));
      return (FALSE);
    }



  if (rms_status != RMS$_RNF
      && strncmp (code, dtr->kit_code, strlen (code)) == 0)
    {
      strncpy (quotesed_buff.kit_part_code, dtr->kit_code, PARTKEYLEN);
      unpad (quotesed_buff.kit_part_code, PARTKEYLEN);
      strcpy (quotesed_buff.description, dtr->description);
      strcpy (quotesed_buff.units, "each");
      quotesed_buff.kit = TRUE;
      quotesed_buff.number = 0;
      quotesed_buff.cost_price = 0;
      quotesed_buff.wholesale = 0;
      quotesed_buff.retail = 0;

      exclreol (quote_line[0].row, quote_line[0].col, BLACK);
      init_screen (quote_line, linlen);
      return (TRUE);
    }


  strcpy (inventory_key.part_code, code);
  pad (inventory_key.part_code, PARTKEYLEN);

  inventory_file.rab.rab$b_krf = INV_PART_KEY;
  inventory_file.rab.rab$l_kbf = (char *) &inventory_key;
  inventory_file.rab.rab$b_ksz = sizeof (inventory_key);
  inventory_file.rab.rab$b_rac = RAB$C_KEY;
  inventory_file.rab.rab$l_ubf = (char *) &inventory_buff;      /*record; */
  inventory_file.rab.rab$w_usz = sizeof (inventory_buff);
  inventory_file.rab.rab$l_rop = RAB$M_NLK | RAB$M_EQNXT;

  rms_status = sys$get (&inventory_file.rab);

  if (!rmstest (rms_status, 4, RMS$_NORMAL, RMS$_RNF, RMS$_RLK, RMS$_OK_RLK))
    warn_user ("$GET %s", rmserror(rms_status,&inventory_file));
  else if (rmstest (rms_status, 1, RMS$_RLK))
    {
      warn_user ("%s", strerror (EVMSERR, rms_status));
      return (FALSE);
    }


  if (rms_status != RMS$_RNF
      && strncmp (code, inventory_buff.part_code, strlen (code)) == 0)
    {
      strncpy (quotesed_buff.kit_part_code, inventory_buff.part_code,
               PARTKEYLEN);
      unpad (quotesed_buff.kit_part_code, PARTKEYLEN);
      strcpy (quotesed_buff.description, inventory_buff.description);
      strcpy (quotesed_buff.units, inventory_buff.units);
      quotesed_buff.kit = FALSE;
      quotesed_buff.number = 0;
      quotesed_buff.cost_price = 0;
      quotesed_buff.wholesale = 0;
      quotesed_buff.retail = 0;

      exclreol (quote_line[0].row, quote_line[0].col, BLACK);
      init_screen (quote_line, linlen);
      return (TRUE);
    }
  else
    {
      warn_user ("No Match Found");

      /*   *quotesed_buff.kit_part_code = 0; */
      memset (&quotesed_buff, 0, sizeof (quotesed_buff));
      quotesed_buff.tax_code = quote_buff.tax_code;

      exclreol (quote_line[0].row, quote_line[0].col, BLACK);
      init_screen (quote_line, linlen);
      return (FALSE);
    }

  warn_user ("$GETEQ %s %s %u", strerror (EVMSERR, rms_status), filterfn (__FILE__),
             __LINE__);
  return (TRUE);
}

int
quotesed (edtstr, row, col, lnth, attr, pos, control,filedef)
     char *edtstr;
     int col, row, lnth, attr, *pos;
     unsigned int *control;
     struct filestruct *filedef;
{
  int i, edited = FALSE, ret = 0;
  struct quote_linerec *default_ptr;
  static struct quote_part_rec comp_buff;
  static int strtfld = 0;
  struct line_rec *curs;

  field_row = row;
  field_col = col;

  non_inv_number = non_inv_price = 0.0;
  switch (master_master.d_flag_w)
    {
    case 'W':
      quote_line = quote_line_whole;
      linlen = LINEDLEN;
      break;
    case 'R':
      quote_line = quote_line_retail;
      linlen = LINEDLEN;
      break;
    case 'X':
      quote_line = quote_line_all_retail;
      linlen = LINEDLEN;
      break;
    case 'Y':
      quote_line = quote_line_retail_whole;
      linlen = LINEDLEN;
      break;
    default:
      quote_line = quote_line_all;
      linlen = LINEDLEN;
      break;
    }


  memcpy (&quotesed_buff, edtstr, sizeof (quotesed_buff));
  if (*control & INIT_ONLY)
    {
      for (i = 0; i < linlen; i++)
        quote_line[i].row = row;
      init_screen (quote_line, linlen);
      return (TRUE);
    }
  else if (*control & ALLOCATE)
    {
      (*(struct quote_linerec **) edtstr) = default_ptr =
        lmalloc (sizeof (quotesed_buff));
      default_ptr->storage.tax_code = quote_buff.tax_code;
      strtfld = 0;
      return (TRUE);
    }
  else if (*control & COMP_LOAD)
    {
      memcpy (&comp_buff, &quotesed_buff, sizeof (quotesed_buff));
      return (TRUE);
    }
  else if (*control & COMP_CHECK)
    {
      return (memcmp (&comp_buff, &quotesed_buff, sizeof (quotesed_buff)) !=
              0);
    }
  for (i = 0; i < linlen; i++)
    quote_line[i].row = row;

  init_screen (quote_line, linlen);
  ret =
    enter_screen (quote_line, linlen, &edited, breakout, &quote_file,
                  &strtfld);
  switch (ret)
    {
    case SMG$K_TRM_CTRLD:
      quotesed_totals (TRUE, FALSE);
      break;
    case SMG$K_TRM_RIGHT:
    case SMG$K_TRM_CTRLZ:
    case SMG$K_TRM_CR:
    case SMG$K_TRM_HT:
      strtfld = 0;
      break;
    case SMG$K_TRM_UP:
      curs = quote_file.entry_screen[quote_file.entry_comment].field;
      if (curs->last == NULL)
        strtfld = 0;
      break;

    }
  memcpy (edtstr, &quotesed_buff, sizeof (quotesed_buff));
  return (ret);
}
