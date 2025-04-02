

/*
 * Title:       swbrowse3.c
 * Author:      Michael Brown <vmslib@b5.net>
 * Created:     03 Jun 2023
 * Last update:
 *
 */


#include rms
#include stdio
#include ssdef
#include stsdef
#include string
#include stdlib
#include starlet
#include lib$routines
#include descrip
#include ints
#include jpidef
#include ctype
#include errno
#include math

#include smgdef
#include smg$routines



#include "extools.h"
#include "screen.h"
#include "menu.h"
#include "filebrowse.h"
#include "warns.h"

#define INC_INITS
#include "swfile.h"

#include "swformdef.h"
#include "swlocality.h"
#include "swoptions.h"
#include "transaction.h"
#include "procinvc.h"

#define VERSION "3.4.4"

#define SIZE_UNAME	12
#define SIZE_TIMESTR 24



extern char user_name[];
extern char priv_user[];
extern int win_depth;

extern int TopLine;
extern int BottomLine;
extern int StatusLine;
extern int RegionLength;
extern int HelpLine;
extern int in_ballance;

extern struct master_rec master_master;

void inventory_filedef_init ();
void kit_filedef_init ();
void quote_filedef_init ();
void print_receipt ();

extern float ballance(uint32);

/*  Key structure for list key index search */
struct listkeystruct
{
  char name[SIZE_NAME];
  uint16 joic;
} listkey;

struct servlistkeystruct
{
  uint16 joic;
  char date[SIZE_UID];
} servlistkey, startkey;

void Service_Records ();
void Service_Records_Fkey ();

void Invoice_Records_Fkey ();
int edit_flag = FALSE;

/* Expandable Menu definitions area  */
#define MAIN_MENU_LEN 6
struct menu_params main_menu[] = {
  "Service Records", Service_Records,
  "Search Name", Search_Record,
  "Insert Record", Insrt_Record,
  "Edit Record", Edit_Record,
  "Delete Record", Remove_Record,
  {"ReRead Index", reread_index}
};

void fork_inventory_file ();
void fork_kit_file ();
void edit_master_record ();
void fork_quote_file ();
void fork_all_quotes ();
void account_ballance ();



#define FILES_MENU_LEN 6
static struct menu_params files_menu[] = {
  "Receipt", print_receipt,
  "All Invoices",fork_all_quotes,
  "Inventory", fork_inventory_file,
  "Kits", fork_kit_file,
  "Services",Service_Records,
  "Master Record", edit_master_record
};

#define RCPT_MENU_LEN 2
static struct menu_params receipt_menu[] = {
  "Print Receipt", print_receipt,
  "Account Balance", account_ballance,
};

#define INVC_MENU_LEN 3
static struct menu_params invoice_menu[] = {
  "Invoices for Client", fork_quote_file,
  "All Invoices",fork_all_quotes,
  "Make Payment", print_receipt
};


struct cmndline_params read_cmndline[] = {
  {4, NULL, NULL, 0, NULL},
  {F1, "Edit Record", NULL, 0, Edit_Record_NW},
  {F2, "Invoices", invoice_menu, INVC_MENU_LEN, NULL},
  {F4, "Services", NULL, 0, Service_Records},
  {F3, "Receipt", receipt_menu, RCPT_MENU_LEN, NULL}
};


struct cmndline_params list_cmndline[] = {
  {4, NULL, NULL, 0, NULL},
  {F1, "New Client", NULL ,0, Insrt_Record},
  {F4, "Other Files", files_menu, FILES_MENU_LEN, NULL},
  {F3, "Balance", NULL, 0, account_ballance},
  {F2, "Invoices", NULL, 0, fork_quote_file}
};

struct cmndline_params receipt_cmndline[] = {
  {1, NULL, NULL, 0, NULL},
  {F3, "Make Payment", NULL ,0, print_receipt}
};




#define SERV_MENU_LEN 4
struct menu_params serv_main_menu[] = {
  "Insert Record", Insrt_Record,
  "Edit Record", Edit_Record,
  "Delete Record", Remove_Record,
  {"ReRead Index", reread_index}
};



struct cmndline_params serv_read_cmndline[] = {
  {1, NULL, NULL, 0, NULL},
  {F1, "Edit Record", NULL, 6, Edit_Record_NW}
};


struct cmndline_params serv_list_cmndline[] = {
  {1, NULL, NULL, 0, NULL},
  {F1, "New Service", NULL, 0, Insrt_Record}
};



/* Form scrolling list line element */
void
SetListPrompt (ptr, filedef)
     struct List_Element *ptr;
     struct filestruct *filedef;
{
  char timestr[SIZE_TIMESTR+1];
  struct primary_rec *dtr = (struct primary_rec *) filedef->fileio;
  $DESCRIPTOR (atime, timestr);


  rms_status = sys$asctim (0, &atime, &dtr->inst_date, 0);
  if ((rms_status & 1) != 1)
    lib$signal (rms_status);
  timestr[atime.dsc$w_length] = 0;
  strtok(timestr," ");

  sprintf (ptr->Prompt, "%.*s %4hu %.*s%.*s",
           SIZE_TIMESTR, timestr,
           dtr->joic_no, SIZE_NAME, dtr->name, SIZE_ADDRESS, dtr->address);

}

/* Form unique key structure for listkey search  */
void
Set_List_Key (void)
{
  int iter;

  memcpy (listkey.name, fileio.record.name, SIZE_NAME);
  for (iter = strlen (listkey.name); iter < SIZE_NAME; iter++)
    listkey.name[iter] = ' ';
  listkey.joic = fileio.record.joic_no;
}

/* Pad zero terminated Key Fields with spaces before writing */
void
pad_record (void)
{
  pad(fileio.record.name,SIZE_NAME);
  pad(fileio.record.country,SIZE_COUNTRY);

}

/* Unpad and zero terminate Key Fields after reading */
void
unpad_record (void)
{
  unpad(fileio.record.name,SIZE_NAME);
  unpad(fileio.record.country,SIZE_COUNTRY);
}

void
dump_lines (NumOfLines, ptr, lines)
     int NumOfLines;
     struct line_rec **ptr;
     char lines[TOTAL_LINES][LINE_LENGTH];
{
  struct line_rec *memptr, *first_line = NULL;
  int i;

  for (i = 0; i < NumOfLines; i++)
    {
      if ((memptr =
           (struct line_rec *) malloc (sizeof (struct line_rec))) == NULL)
        {
          warn_user ("Memory Allocation Error");
          exit;
        }
      else
        {
          memptr->storage[0] = 0;
          memptr->next = NULL;
          memptr->last = NULL;
          if ((*ptr) == NULL)
            {
              first_line = (*ptr) = memptr;
            }
          else
            {
              (*ptr)->next = memptr;
              memptr->last = (*ptr);
              (*ptr) = memptr;
            }
          strcpy ((*ptr)->storage, lines[i]);
        }
    }
  (*ptr) = first_line;
}


int
load_lines (curs, lines)
     struct line_rec *curs;
     char lines[TOTAL_LINES][LINE_LENGTH];
{
  int ind = 0, j;

  while (curs != NULL && curs->last != NULL)
    curs = curs->last;
  while (curs != NULL && ind < TOTAL_LINES)
    {
      strcpy (lines[ind], curs->storage);
      for (j = strlen (lines[ind]); j < LINE_LENGTH; j++)
        lines[ind][j] = 0;
      ind++;
      curs = curs->next;
    }
  return (ind);
}


/* Copy Variable section of record and store into line editor ring buffer before editing */


void
load_editor_buffer (ptr, filedef)
     struct line_rec **ptr;
     struct filestruct *filedef;
{
  int NumOfLines;
  struct iorec *dtr = (struct iorec *) filedef->fileio;

  if (dtr->record.reclen != 0 && dtr->record.reclen != PRIM_RECORD_SIZE)
    {
      NumOfLines = (dtr->record.reclen - PRIM_RECORD_SIZE) / LINE_LENGTH;
    }
  else
    {
      NumOfLines = 0;
    }

  dump_lines (NumOfLines, ptr, dtr->lines);

}

/* Copy line editor buffer into Variable section of record after editing */
int
unload_editor_buffer (curs, filedef)
     struct line_rec *curs;
     struct filestruct *filedef;
{
  int ind = 0;
  struct iorec *dtr = (struct iorec *) filedef->fileio;

  ind = load_lines (curs, dtr->lines);

  dtr->record.reclen = PRIM_RECORD_SIZE + (ind * LINE_LENGTH);
  return (dtr->record.reclen);
}


char searchkey[SIZE_NAME] = "";

void
Search_Record (msg, filedef, frstchr)
     struct linedt_msg_rec *msg;
     struct filestruct *filedef;
     char *frstchr;
{
  int i;
  struct rfabuff rfa;
  uint16 cust_no;

  ReadSearchTerm (searchkey);
  cust_no = atoi(searchkey);

  if(cust_no == 0) {

#ifdef CAPSEARCH
    for (i = 0; searchkey[i] != 0; i++)
       searchkey[i] = toupper (searchkey[i]);
#else
     searchkey[0] = toupper (searchkey[0]);
#endif

     pad(searchkey,SIZE_NAME);
     filedef->rab.rab$b_krf = CLNT_NAME_KEY;
     filedef->rab.rab$l_kbf = (char *) searchkey;
     filedef->rab.rab$b_ksz = SIZE_NAME;
  } else {
     filedef->rab.rab$b_krf = CLNT_CUST_KEY;
     filedef->rab.rab$l_kbf = (char *) &cust_no;
     filedef->rab.rab$b_ksz = SIZE_INTEGER;
  }

  filedef->rab.rab$b_rac = RAB$C_KEY;
  filedef->rab.rab$l_rop = RAB$M_NLK | RAB$M_EQNXT;     /* return equal to or next  */

  rms_status = sys$get (&filedef->rab);

  filedef->rab.rab$l_kbf = (char *) filedef->listkey;
  filedef->rab.rab$b_ksz = filedef->listkeysz;

  if (!rmstest (rms_status, 3, RMS$_NORMAL, RMS$_RNF, RMS$_OK_RLK))
    {
      warn_user ("$FIND %s", rmserror(rms_status,filedef));
      exit;
    }
  else if (rms_status == RMS$_RNF)
    warn_user ("%s", strerror (EVMSERR, rms_status));
  else
    {
      rfa_copy (&rfa, (struct rfabuff *) &filedef->rab.rab$w_rfa);
      ReadMiddleOut (&rfa, filedef);
    }
  *searchkey = 0;
}

void
Set_Serv_List_Key (void)
{

  memcpy (servlistkey.date, servio.record.ascdate, SIZE_UID);
  servlistkey.joic = servio.record.joic_no;
}

void
SetServListPrompt (ptr, filedef)
     struct List_Element *ptr;
     struct filestruct *filedef;
{
  char timestr[SIZE_TIMESTR+1];
  struct service_rec *dtr = (struct service_rec *) filedef->fileio;
  $DESCRIPTOR (atime, timestr);

  rms_status = sys$asctim (0, &atime, &dtr->service_datetime, 0);
  if ((rms_status & 1) != 1)
    lib$signal (rms_status);
  timestr[atime.dsc$w_length] = 0;

  sprintf (ptr->Prompt, "%4hu Serviced : %.*s Technician : %s",
           dtr->joic_no, SIZE_TIMESTR, timestr, dtr->tech);

}



void
serv_load_editor_buffer (ptr, filedef)
     struct line_rec **ptr;
     struct filestruct *filedef;
{
  int NumOfLines;
  struct serviorec *dtr = (struct serviorec *) filedef->fileio;

  if (dtr->record.reclen != 0 && dtr->record.reclen != SERV_RECORD_SIZE)
    {
      NumOfLines = (dtr->record.reclen - SERV_RECORD_SIZE) / LINE_LENGTH;
    }
  else
    {
      NumOfLines = 0;
    }
  dump_lines (NumOfLines, ptr, dtr->lines);
}

/* Copy line editor buffer into Variable section of record after editing */
int
serv_unload_editor_buffer (curs, filedef)
     struct line_rec *curs;
     struct filestruct *filedef;
{
  int ind = 0;
  struct serviorec *dtr = (struct serviorec *) filedef->fileio;

  ind = load_lines (curs, dtr->lines);

  dtr->record.reclen = SERV_RECORD_SIZE + (ind * LINE_LENGTH);
  return (dtr->record.reclen);
}



int
where_joics_eq ()
{
  if (servio.record.joic_no == fileio.record.joic_no)
    return (TRUE);
  else
    return (FALSE);
}

char *
xunpad (source, lnth)
     char *source;
     int lnth;
{
  int i;
  static char work[128];

  memcpy (work, source, lnth);
  unpad(work,lnth);
  return (work);
}

#define SERV_FORK_LENGTH 6
#define ACNTBALL_LENGTH 3
static int topline  (unsigned length)
{
   if (edit_flag) return(3);

   if ((PrimFile.CurrentLine+3+length) > PrimFile.RegionLength)
      return(PrimFile.CurrentLine-length);
   else
      return(PrimFile.CurrentLine+3);
}


void account_ballance (msg, filedef)
     struct linedt_msg_rec *msg;
     struct filestruct *filedef;
{
   struct primary_rec client_buff;
   char prntline[64];
   $DESCRIPTOR (aprint, prntline);
   float blnc;
   uint16 scan = 0;
   struct linedt_msg_rec mesg;

  if (filedef->CurrentPtr == NULL)
    return;

  if(!edit_flag) {
     filedef->rab.rab$b_rac = RAB$C_RFA;
     rfa_copy (&filedef->rab.rab$w_rfa, &filedef->CurrentPtr->rfa);
     filedef->rab.rab$l_ubf = (char *) &client_buff;
     filedef->rab.rab$w_usz = sizeof(client_buff);
     filedef->rab.rab$l_rop = RAB$M_NLK;

     rms_status = sys$get (&filedef->rab);

     if (!rmstest (rms_status, 2, RMS$_NORMAL, RMS$_OK_RLK))
       {
         warn_user ("$GET %s", rmserror(rms_status,filedef));
         return;
       }
  } else memcpy(&client_buff,filedef->fileio,sizeof(client_buff));

  in_ballance = TRUE;
  open_window (crnt_window->rul+topline(ACNTBALL_LENGTH),
               crnt_window->cul+7, 51,
               ACNTBALL_LENGTH , WHITE, border);
  HeadLine("Account Balance",centre);

  blnc = ballance(client_buff.joic_no);
  sprintf(prntline,"Balance for Account %u : $%10.2f %s",
                   client_buff.joic_no,fabs(blnc),drcr(blnc));
  aprint.dsc$w_length = strlen (prntline);
  smg$put_chars (&crnt_window->display_id,&aprint,&2,&3);
  Refresh (crnt_window->display_id);
  
/*  Get_Keystroke(); */
  command_line (receipt_cmndline, &scan, &mesg, filedef);

  in_ballance = FALSE;

  close_window();

}




void Invoice_Records_Fkey (msg, filedef)
     struct linedt_msg_rec *msg;
     struct filestruct *filedef;
{
    fork_quote_file(msg,filedef);
}
void Service_Records_Fkey (msg, filedef)
     struct linedt_msg_rec *msg;
     struct filestruct *filedef;
{
    Service_Records(msg,filedef);
}

void
Service_Records (msg, filedef)
     struct linedt_msg_rec *msg;
     struct filestruct *filedef;
{
  /*unsigned */ long x, y;

  if (filedef->CurrentPtr == NULL)
    return;

  filedef->rab.rab$b_rac = RAB$C_RFA;
  rfa_copy (&filedef->rab.rab$w_rfa, &filedef->CurrentPtr->rfa);

  filedef->rab.rab$l_ubf = (char *) filedef->fileio;    /*record; */
  filedef->rab.rab$w_usz = filedef->fileiosz;
  filedef->rab.rab$l_rop = RAB$M_NLK;

  rms_status = sys$get (&filedef->rab);

  if (!rmstest (rms_status, 2, RMS$_NORMAL, RMS$_OK_RLK))
    {
      warn_user ("$GET %s", rmserror(rms_status,filedef));
      return;
    }

/*  spawn_scroll_window (&ServFile, 10, 68); */

  open_window (crnt_window->rul+topline(SERV_FORK_LENGTH),
               crnt_window->cul+7, 61,
               SERV_FORK_LENGTH , WHITE, border);
  smg$get_display_attr (&crnt_window->display_id, &ServFile.RegionLength);
  ServFile.BottomLine = ServFile.RegionLength;
  ServFile.TopLine = 2;




  HeadLine ("Service Records For %s", centre,
            xunpad (fileio.record.name, SIZE_NAME));

/* ZEN  Restrict List to the following conditions 28-MAY-2023 23:26:43 */
  ServFile.startkey = (char *) &startkey;       /* begin list EQ or NXT */
  ServFile.startkeyln = sizeof startkey;
  ServFile.where = where_joics_eq;      /* test for member of list */

  startkey.joic = fileio.record.joic_no;
  memset (startkey.date, '0', SIZE_UID);
/* END OF MOD */

  File_Browse (&ServFile, normal);

/* ZEN example use of File_Browse select mode return rfa value 27-MAY-2023 23:25:15 */
/*      rfa_copy(&ServFile.rab.rab$w_rfa,File_Browse(&ServFile,select));

      if (!rfa_iszero((struct rfabuff *)&ServFile.rab.rab$w_rfa)) {
         ServFile.rab.rab$b_rac = RAB$C_RFA;
         ServFile.rab.rab$l_ubf = (char *)ServFile.fileio;
         ServFile.rab.rab$w_usz = ServFile.fileiosz;
         ServFile.rab.rab$l_rop = RAB$M_NLK;

         rms_status = sys$get(&ServFile.rab);
         if (!rmstest(rms_status,2,RMS$_NORMAL,RMS$_OK_RLK)) 
           {
             warn_user("$GET %s", rmserror(rms_status,&ServFile));
             exit;
           } 
      }
*/

/*  win_depth--; */
 
  close_window ();
  Help ("");
}


int delete_transactions (uint32 cust_no)
{

   transaction_file.rab.rab$b_krf = TRANS_CUST_KEY;
   transaction_file.rab.rab$l_kbf = (char *) &cust_no;
   transaction_file.rab.rab$b_ksz = SIZE_BN4;
   transaction_file.rab.rab$b_rac = RAB$C_KEY;
   transaction_file.rab.rab$l_ubf = (char *)&transaction_buff;
   transaction_file.rab.rab$w_usz = sizeof transaction_buff; 
   transaction_file.rab.rab$l_rop = RAB$M_RLK | RAB$M_EQNXT;
   rms_status = sys$get (&transaction_file.rab);
   if (!rmstest(rms_status, 2, RMS$_NORMAL,RMS$_RNF)) {
      warn_user ("GET$ %s",rmserror(rms_status,&transaction_file));
      return(FALSE);
   }
   if(rms_status == RMS$_RNF) return(TRUE);

   transaction_file.rab.rab$b_rac = RAB$C_SEQ;
   transaction_file.rab.rab$l_rop = RAB$M_RLK;

   while(rms_status != RMS$_EOF && transaction_buff.cust_no == cust_no) {
      rms_status = sys$delete(&transaction_file.rab);
      if(!rmstest(rms_status,1,RMS$_NORMAL)) {
         warn_user ("DEL$ %s",rmserror(rms_status,&transaction_file));
         return(FALSE);
      }

      rms_status = sys$get (&transaction_file.rab);
      if(!rmstest(rms_status,2,RMS$_NORMAL,RMS$_EOF)) {
         warn_user ("GET$ %s",rmserror(rms_status,&transaction_file));
         return(FALSE);
      }
   
   }
   return(TRUE);
}

int delete_invoices (uint16 cust_no)
{

   quote_file.rab.rab$b_krf = INVC_CUST_KEY;
   quote_file.rab.rab$l_kbf = (char *) &cust_no;
   quote_file.rab.rab$b_ksz = SIZE_BN2;
   quote_file.rab.rab$b_rac = RAB$C_KEY;
   quote_file.rab.rab$l_ubf = (char *)quote_file.fileio;
   quote_file.rab.rab$w_usz = quote_file.fileiosz; 
   quote_file.rab.rab$l_rop = RAB$M_RLK | RAB$M_EQNXT;
   rms_status = sys$get (&quote_file.rab);
   if (!rmstest(rms_status, 2, RMS$_NORMAL,RMS$_RNF)) {
      warn_user ("GET$ %s",rmserror(rms_status,&quote_file));
      return(FALSE);
   }
   if(rms_status == RMS$_RNF) return(TRUE);

   memcpy(&quote_buff,quote_file.fileio,sizeof quote_buff);
   quote_file.rab.rab$b_rac = RAB$C_SEQ;
   quote_file.rab.rab$l_rop = RAB$M_RLK;

   while(rms_status != RMS$_EOF && quote_buff.cust_no == cust_no) {
      rms_status = sys$delete(&quote_file.rab);
      if(!rmstest(rms_status,1,RMS$_NORMAL)) {
         warn_user ("DEL$ %s",rmserror(rms_status,&quote_file));
         return(FALSE);
      }

      rms_status = sys$get (&quote_file.rab);
      if(!rmstest(rms_status,2,RMS$_NORMAL,RMS$_EOF)) {
         warn_user ("GET$ %s",rmserror(rms_status,&quote_file));
         return(FALSE);
      }
      memcpy(&quote_buff,quote_file.fileio,sizeof quote_buff);   
   }
   return(TRUE);
}




void prim_pre_view_proc ()
{
   edit_flag = TRUE;
}
void prim_post_view_proc ()
{
   edit_flag = FALSE;
}

int prim_pre_insert_proc ()
{

  fileio.record.joic_no = Find_Free_Joic_Key (&PrimFile);
  return (TRUE);
}

int prim_pre_delete_proc ()
{
   int status;
   struct primary_rec prim_buff;


  if (PrimFile.CurrentPtr == NULL)
    return(FALSE);

  PrimFile.rab.rab$b_rac = RAB$C_RFA;
  rfa_copy (&PrimFile.rab.rab$w_rfa, &PrimFile.CurrentPtr->rfa);

  PrimFile.rab.rab$l_ubf = (char *) PrimFile.fileio;
  PrimFile.rab.rab$w_usz = PrimFile.fileiosz;
  PrimFile.rab.rab$l_rop = RAB$M_NLK;

  rms_status = sys$get (&PrimFile.rab);

  if (!rmstest (rms_status, 2, RMS$_NORMAL, RMS$_OK_RLK))
    {
      warn_user ("$GET %s", rmserror(rms_status,&PrimFile));
      return(FALSE);
    }

   memcpy(&prim_buff,PrimFile.fileio,sizeof prim_buff);

#ifdef TRANSACTION
    status = sys$start_transw (event_flag, 0, &iosb, 0, 0, tid);
    check_status ("start");
#endif

   if(!delete_invoices(prim_buff.joic_no)){
#ifdef TRANSACTION
      status = sys$abort_transw (event_flag, 0, &iosb, 0, 0, tid);
      check_status ("abort");
#endif
      return FALSE;
   }
   if(!delete_transactions(prim_buff.joic_no)){
#ifdef TRANSACTION
      status = sys$abort_transw (event_flag, 0, &iosb, 0, 0, tid);
      check_status ("abort");
#endif
      return FALSE;
   }
#ifdef TRANSACTION
   status = sys$end_transw (event_flag, 0, &iosb, 0, 0, tid);
   check_status ("end");
#endif
   return TRUE;
}

void
serv_post_insert_proc ()
{
  mkuid (servio.record.ascdate, servio.record.service_datetime);
  servio.record.joic_no = fileio.record.joic_no;
}
void
serv_post_edit_proc ()
{
  mkuid (servio.record.ascdate, servio.record.service_datetime);
}

void serv_pre_edit()
{

   service_entry_screen[SERVDATELN].format |= NON_EDIT; 

}

void serv_edit_cleanup ()
{

   service_entry_screen[SERVDATELN].format &= (~NON_EDIT);

}


main (argc, argv)
     int argc;
     char **argv;
{
  char masffn[64], primffn[64], servffn[64], kitffn[64], invffn[64], transffn[64];
  char quoteffn[64];


  if (argc < 1 || argc > 2)
    printf ("Incorrect number of arguments");
  else
    {
      printf ("RMS Indexed File Browser\n");




      PrimFile = filedef$init;
/*
      PrimFile.filename = (argc == 2 ? *++argv : "swprim.dat"); 
*/
      sprintf (primffn, "%s%s", FILE_PATH, "swprim.dat");
      PrimFile.filename = primffn;

      PrimFile.initialize = PrimaryFileInit;
      PrimFile.fileio = (char *) &fileio;
      PrimFile.fileiosz = sizeof fileio;
      PrimFile.recsize = sizeof (struct primary_rec);
      PrimFile.setprompt = SetListPrompt;

      PrimFile.mainsrch = Search_Record;

      PrimFile.entry_screen = primary_entry_screen;
      PrimFile.entry_length = PRISCRNLEN;
      PrimFile.entry_comment = COMMENTLINE;
      PrimFile.read_cmndline = read_cmndline;
      PrimFile.list_cmndline = list_cmndline;
      PrimFile.listkey = (char *) &listkey;
      PrimFile.listkeysz = sizeof listkey;
      PrimFile.fwrdkeyid = CLNT_FORWARD_KEY;
      PrimFile.revrkeyid = CLNT_REVERSE_KEY;
      PrimFile.setlistkey = Set_List_Key;
      PrimFile.padrecord = pad_record;
      PrimFile.unpadrecord = unpad_record;
      PrimFile.loadvarbuff = load_editor_buffer;
      PrimFile.dumpvarbuff = unload_editor_buffer;
      PrimFile.preinsert = prim_pre_insert_proc;
      PrimFile.preview = prim_pre_view_proc;
      PrimFile.postview = prim_post_view_proc;
      PrimFile.predelete = prim_pre_delete_proc;

      open_file (&PrimFile);

      ServFile = filedef$init;

      sprintf (servffn, "%s%s", FILE_PATH, "swserv.dat");
      ServFile.filename = servffn;
/*
      ServFile.filename = "swserv.dat";
*/
      ServFile.initialize = ServiceFileInit;
      ServFile.fileio = (char *) &servio;
      ServFile.fileiosz = sizeof servio;
      ServFile.recsize = sizeof (struct service_rec);
      ServFile.entry_screen = service_entry_screen;
      ServFile.entry_length = SERVSCRNLEN;
      ServFile.entry_comment = SERVTEXTLN;
      ServFile.read_cmndline = serv_read_cmndline;
      ServFile.list_cmndline = serv_list_cmndline;
      ServFile.listkey = (char *) &servlistkey;
      ServFile.listkeysz = sizeof servlistkey;
      ServFile.fwrdkeyid = SERV_FORWARD_KEY;
      ServFile.revrkeyid = SERV_REVERSE_KEY;
      ServFile.setprompt = SetServListPrompt;
      ServFile.setlistkey = Set_Serv_List_Key;
/* Moved to Service_Records () 
      ServFile.startkey = (char *)&startkey;
      ServFile.startkeyln = sizeof startkey;
      ServFile.where = where_joics_eq;
*/
      ServFile.loadvarbuff = serv_load_editor_buffer;
      ServFile.dumpvarbuff = serv_unload_editor_buffer;
      ServFile.postedit = serv_post_edit_proc;
      ServFile.postinsert = serv_post_insert_proc;
      ServFile.preedit = serv_pre_edit;
      ServFile.editcleanup = serv_edit_cleanup;

      open_file (&ServFile);


      kit_file = filedef$init;

      sprintf (kitffn, "%s%s", FILE_PATH, "swkit.dat");
      kit_file.filename = kitffn;
      kit_file.initialize = KitFileInit;
      kit_filedef_init ();
      open_file (&kit_file);

      inventory_file = filedef$init;

      sprintf (invffn, "%s%s", FILE_PATH, "swinvent.dat");
      inventory_file.filename = invffn;
      inventory_file.initialize = InventoryFileInit;
      inventory_filedef_init ();
      open_file (&inventory_file);

      master_file = filedef$init;
      sprintf (masffn, "%s%s", FILE_PATH, "swmaster.dat");
      master_file.filename = masffn;
      master_file.initialize = MasterFileInit;

      open_file (&master_file);

      quote_file = filedef$init;

      sprintf (quoteffn, "%s%s", FILE_PATH, "swquote.dat");
      quote_file.filename = quoteffn;
      quote_file.initialize = QuoteFileInit;
      quote_filedef_init ();

      open_file (&quote_file);

      transaction_file = filedef$init;

      sprintf (transffn, "%s%s", FILE_PATH, "swtrans.dat");
      transaction_file.filename = transffn;
      transaction_file.initialize = TransactionFileInit;
     

      open_file (&transaction_file);





      termsize (&TermWidth, &TermLength);

      TopLine = 3;
      RegionLength = TermLength - 3;
      BottomLine = TermLength - 2;
      StatusLine = TermLength - 1;
      HelpLine = TermLength;



      init_terminal (TermWidth, TermLength);



      init_windows ();




      get_username ();
      strcpy (priv_user, PRIVUSER);

      read_master_record ();
      memcpy (&master_master, &master_buff, sizeof (master_buff));


      HeadLine ("SW Invoice v%s - Logged in as %s", centre, VERSION,
                user_name);
      open_window (TopLine, 2, TermWidth - 2, BottomLine - TopLine, 0,
                   border);
      smg$get_display_attr (&crnt_window->display_id, &PrimFile.RegionLength);  /*,&TermWidth); */
      PrimFile.TopLine = 2;
      PrimFile.BottomLine = PrimFile.RegionLength;



      HeadLine ("Client Records", centre);

      File_Browse (&PrimFile, normal);

      close_window ();

      close_terminal ();


      rms_status = sys$close (&PrimFile.fab);
      if (rms_status != RMS$_NORMAL)
        printf ("failed $CLOSE %s\n", PrimFile.filename);

      rms_status = sys$close (&ServFile.fab);
      if (rms_status != RMS$_NORMAL)
        printf ("failed $CLOSE %s\n", ServFile.filename);

      rms_status = sys$close (&kit_file.fab);
      if (rms_status != RMS$_NORMAL)
        printf ("failed $CLOSE %s\n", kit_file.filename);

      rms_status = sys$close (&inventory_file.fab);
      if (rms_status != RMS$_NORMAL)
        printf ("failed $CLOSE %s\n", inventory_file.filename);

      rms_status = sys$close (&master_file.fab);
      if (rms_status != RMS$_NORMAL)
        printf ("failed $CLOSE %s\n", master_file.filename);

      rms_status = sys$close (&quote_file.fab);
      if (rms_status != RMS$_NORMAL)
        printf ("failed $CLOSE %s\n", quote_file.filename);

      rms_status = sys$close (&transaction_file.fab);
      if (rms_status != RMS$_NORMAL)
        printf ("failed $CLOSE %s\n", transaction_file.filename);


    }
}
