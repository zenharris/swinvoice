
/*
 * Title:       filebrowse.c
 * Author:      Michael Brown <vmslib@b5.net>
 * Created:     03 Jun 2023
 * Last update:
 */



#include rms
#include stdio
#include ssdef
#include errno
#include stsdef
#include string
#include stdlib
#include starlet
#include lib$routines
#include descrip
#include ints
#include jpidef
#include stdarg
#include ctype

#include smgdef
#include smg$routines


#include "extools.h"
#include "screen.h"
#include "menu.h"

#define FILEBROWSE_C_
#include "filebrowse.h"
#include "warns.h"


#define VERSION "0.01"

#define SIZE_UNAME	12
#define SIZE_SUBJECT	160
#define SIZE_TIMESTR 24
#define SIZE_SEARCHTERM 128

#define INCLUDEFIRST 1

#define RIGHT_MARGIN 80

#define extscrprn(buff,row,col,attr) exscrprn(buff,row-1,col-1,attr)


#define check_status(syscall) \
    if (status != SS$_NORMAL || (status = iosb.status) != SS$_NORMAL) { \
        fprintf (stderr, "Failed to %s the transaction\n", syscall); \
        fprintf (stderr, "Error was: %s", strerror (EVMSERR, status)); \
        exit (EXIT_FAILURE); \
    }

#define event_flag 0

unsigned char tid[16];
struct iosb {
     unsigned short status, mbz;
     unsigned long  reason;
} iosb;



int TopLine;
int BottomLine;
int StatusLine;
int RegionLength;
int HelpLine;

char user_name[SIZE_UNAME + 1] = "";
char priv_user[SIZE_UNAME + 1] = "";

int win_depth = 1;


int rms_status;

char *months[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};



void Redraw_Screen (struct filestruct *);

/* int rmsget(uint16); */

void
strupr (char *searchkey)
{
  int i;

  for (i = 0; searchkey[i] != 0; i++)
    searchkey[i] = toupper (searchkey[i]);

}


typedef struct
{
  uint32 hi, lo;
} u64;

/* Return X < Y.  */
int
u64lt (x, y)
     u64 x, y;
{
  return x.hi < y.hi || (x.hi == y.hi && x.lo < y.lo);
}

/*
   int tbuff_equal(b1,b2)
   struct timebuff b1,b2;
   {

     return ((b1.b1 == b2.b1) && (b1.b2 == b2.b2));

   }
*/
void
tbuff_copy (b1, b2)
     struct timebuff *b1, *b2;
{

  b1->b1 = b2->b1;
  b1->b2 = b2->b2;

}

int
tbuff_iszero (b1)
     struct timebuff *b1;
{

  return ((b1->b1 == 0) && (b1->b2 == 0));
}

int
tbuff_equal (b1, b2)
     struct timebuff *b1, *b2;
{

  return ((b1->b1 == b2->b1) && (b1->b2 == b2->b2));

}


void
rfa_copy (b1, b2)
     struct rfabuff *b1, *b2;
{
  b1->b1 = b2->b1;
  b1->b2 = b2->b2;
  b1->b3 = b2->b3;
}

int
rfa_iszero (b1)
     struct rfabuff *b1;
{

  return ((b1->b1 == 0) && (b1->b2 == 0));
}

int
rfa_equal (b1, b2)
     struct rfabuff b1, b2;
{

  return ((b1.b1 == b2.b1) && (b1.b2 == b2.b2) && (b1.b3 == b2.b3));

}

int
rfa_equalp (b1, b2)
     struct rfabuff *b1, *b2;
{

  return ((b1->b1 == b2->b1) && (b1->b2 == b2->b2) && (b1->b3 == b2->b3));

}

void
get_time (struct timebuff *result)
{
  int status;

  status = sys$gettim (result);

  if (!$VMS_STATUS_SUCCESS (status))
    {
      warn_user ("gettim %s %s %u", strerror (status), filterfn (__FILE__),
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
      warn_user ("asctim %s %s %u", strerror (status), filterfn (__FILE__),
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
      warn_user ("asctim %s %s %u", strerror (status), filterfn (__FILE__),
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
      warn_user ("numtim %s %s %u", strerror (status), filterfn (__FILE__),
                 __LINE__);
      return (NULL);
    }

  sprintf (outstr, "%2.2hu-%3s-%4.4hu 00:00:00.00", numvec[2],
           months[numvec[1] - 1], numvec[0]);
  output_d.dsc$w_length = strlen (outstr);
  status = sys$bintim (&output_d, date_time);
  if (!$VMS_STATUS_SUCCESS (status))
    {
      warn_user ("Time Error %s %u", filterfn (__FILE__), __LINE__);
      return (NULL);
    }
  return (date_time);
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

#define BUFFSIZE 32
#define OUTSTRSIZE 256

char *
rmserr (rmscode,filedef,srcfn,srcln)
unsigned int rmscode;
struct filestruct *filedef;
char *srcfn;
unsigned int srcln;
{
  static char outstr[OUTSTRSIZE];
  char buff1[128];

  sprintf (outstr, "%s %s %s (%s %u)",(filedef==NULL)?"":trimfn (filedef->filename,buff1),xlat(rmscode),
           strerror (EVMSERR, rmscode),filterfn(srcfn), srcln);

  return (outstr);
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
spawn_scroll_window (filedef, lnth, width)
     struct filestruct *filedef;
     int lnth;
     int width;
{
  long x, y;

  x = TermWidth <= 90 ? 1 : (win_depth * 4) + 3;
  y = TermWidth <= 90 ? 1 : (win_depth++ * 2) + 3;

  open_window ((y + lnth < BottomLine) ? y : BottomLine - lnth,
               (x < 2) ? 2 : x, width, lnth, 0, border);

  smg$get_display_attr (&crnt_window->display_id, &filedef->RegionLength);      /*,&TermWidth);  */
  filedef->BottomLine = filedef->RegionLength;
  filedef->TopLine = 2;
/*
   HeadLine("%s (%s)",left,heading,filterfn(filedef->filename));
*/

}

#define EDTWIDTH 78
#define EDTLNTH 20

void
spawn_edit_window (filedef, lnth, width)
     struct filestruct *filedef;
     int lnth, width;
{
  long x, y;

  /*  x = (TermWidth/2)-(WIDTH/2)-5;  */
  /*  y = TopLine+((BottomLine-TopLine)/8); */

  x = TermWidth <= 90 ? 1 : (win_depth * 4) + 3;
  y = TermWidth <= 90 ? 1 : (win_depth * 2) + 3;
  win_depth++;

  open_window ((y < TopLine + 1) ? TopLine : y,
               (x < 2) ? 2 : x, width /*TermWidth-2 */ ,
               lnth /*BottomLine-TopLine */ ,
               0, border);

/*  Clear_Region (filedef->TopLine, filedef->BottomLine); */

  smg$erase_display(&crnt_window->display_id);

  Help ("");

}



void
open_file (filedef)
     struct filestruct *filedef;
{

  (*filedef->initialize) (filedef->filename);

  rms_status = sys$create (&filedef->fab);

  if (!rmstest (rms_status, 2, RMS$_NORMAL, RMS$_CREATED))
    error_exit ("$OPEN", filedef);

  if (rms_status == RMS$_CREATED)
    printf ("[Created New Data file.]\n");

  rms_status = sys$connect (&filedef->rab);

  if (!rmstest (rms_status, 1, RMS$_NORMAL))
    error_exit ("$CONNECT", filedef);
}


void
InsertNewLine (Ptr, filedef)
     struct List_Element **Ptr;
     struct filestruct *filedef;
{

  struct List_Element *WorkLine = NULL;

  WorkLine = malloc (sizeof (struct List_Element));

  if (filedef->FirstPtr == NULL)
    {

      filedef->FirstPtr = WorkLine;
      WorkLine->Next = NULL;
      WorkLine->Prev = NULL;
      filedef->LastPtr = WorkLine;

    }
  else
    {
      WorkLine->Prev = (*Ptr);
      WorkLine->Next = (*Ptr)->Next;

      if ((*Ptr)->Next != NULL)
        (*Ptr)->Next->Prev = WorkLine;

      (*Ptr)->Next = WorkLine;

      if (WorkLine->Next == NULL)
        filedef->LastPtr = WorkLine;
    }
  (*Ptr) = WorkLine;
  filedef->ListCount++;
}

void
PutOnTop (Ptr, filedef)
     struct List_Element **Ptr;
     struct filestruct *filedef;
{

  struct List_Element *WorkLine = NULL;

  WorkLine = malloc (sizeof (struct List_Element));

  if (filedef->FirstPtr == NULL)
    {
      filedef->FirstPtr = WorkLine;
      WorkLine->Next = NULL;
      WorkLine->Prev = NULL;
      filedef->LastPtr = WorkLine;
    }
  else
    {
      WorkLine->Next = filedef->FirstPtr;
      filedef->FirstPtr->Prev = WorkLine;
      WorkLine->Prev = NULL;
      filedef->FirstPtr = WorkLine;
    }
  (*Ptr) = WorkLine;
  filedef->ListCount++;
}

void
PutOnBottom (Ptr, filedef)
     struct List_Element **Ptr;
     struct filestruct *filedef;
{

  struct List_Element *WorkLine = NULL;

  WorkLine = malloc (sizeof (struct List_Element));

  if (filedef->FirstPtr == NULL)
    {
      filedef->FirstPtr = WorkLine;
      WorkLine->Next = NULL;
      WorkLine->Prev = NULL;
      filedef->LastPtr = WorkLine;
    }
  else
    {
      WorkLine->Prev = filedef->LastPtr;
      filedef->LastPtr->Next = WorkLine;
      WorkLine->Next = NULL;
      filedef->LastPtr = WorkLine;
    }
  (*Ptr) = WorkLine;
  filedef->ListCount++;
}


void
DeleteLine (Ptr, filedef)
     struct List_Element **Ptr;
     struct filestruct *filedef;
{
  struct List_Element *DelPtr;

  if ((*Ptr)->Next != NULL)
    {
      (*Ptr)->Next->Prev = (*Ptr)->Prev;
      if ((*Ptr)->Prev != NULL)
        (*Ptr)->Prev->Next = (*Ptr)->Next;
      DelPtr = (*Ptr);
      (*Ptr) = (*Ptr)->Next;
    }
  else
    {
      if ((*Ptr)->Prev != NULL)
        {
          /* (*Ptr)->Prev->Next = NULL; */
          (*Ptr)->Prev->Next = (*Ptr)->Next;
          if ((*Ptr)->Next != NULL)
            (*Ptr)->Next->Prev = (*Ptr)->Prev;
          DelPtr = (*Ptr);
          (*Ptr) = /* filedef->LastPtr = */ (*Ptr)->Prev;
        }
      else
        {
          DelPtr = (*Ptr);
          (*Ptr) = filedef->CurrentPtr = filedef->FirstPtr =
            filedef->LastPtr = NULL;
        }

    }
  free (DelPtr);
  filedef->ListCount--;
}


void
DeleteList (filedef)
     struct filestruct *filedef;
{
  struct List_Element *DelPtr;

  while (filedef->FirstPtr != NULL)
    {
      DelPtr = filedef->FirstPtr;
      filedef->FirstPtr = filedef->FirstPtr->Next;
      free (DelPtr);
    }
  filedef->LastPtr = NULL;
  filedef->CurrentPtr = NULL;
  filedef->ListCount = 0;
}







int
ListRead (ListLength, includefirst, reverse, filedef, middleout)
     int ListLength, includefirst, reverse;
     struct filestruct *filedef;
     int middleout;
{
  int number_elements, first = TRUE, rdfirst = TRUE;
  struct List_Element *tempptr = NULL;


  if (!includefirst)
    {
      filedef->rab.rab$b_rac = RAB$C_SEQ;
    }

  for (number_elements = 0; number_elements < ListLength;
       /* number_elements++ */ )
    {
      if ((!middleout) && filedef->startkey != NULL && rdfirst)
        {

          filedef->rab.rab$l_kbf = (char *) filedef->startkey;
          filedef->rab.rab$b_ksz = filedef->startkeyln;
          filedef->rab.rab$b_rac = RAB$C_KEY;
          filedef->rab.rab$l_rop = RAB$M_NLK | RAB$M_EQNXT;     /* return equal to or next  */

          rms_status = sys$get (&filedef->rab);
/* ZEN 6-JUN-2023 21:32:14 */
          if (!rmstest
              (rms_status, 4, RMS$_NORMAL, RMS$_RNF, RMS$_OK_RRL,
               RMS$_OK_RLK))
            warn_user ("GET$ %s",rmserror(rms_status,filedef));

/* end of mod */
          filedef->rab.rab$b_rac = RAB$C_SEQ;
          filedef->rab.rab$l_rop = RAB$M_NLK;

          rdfirst = FALSE;
        }
      else
        {
/*ZEN test*/ filedef->rab.rab$l_rop = RAB$M_NLK;

          rms_status = sys$get (&filedef->rab);

        }

      if (includefirst && first)
        {
          filedef->rab.rab$b_rac = RAB$C_SEQ;
          first = FALSE;
        }


      if (!rmstest
          (rms_status, 5, RMS$_NORMAL, RMS$_EOF, RMS$_OK_RRL, RMS$_RNF,
           RMS$_OK_RLK))
        warn_user ("$GET %s",rmserror(rms_status,filedef));
      else if (rms_status == RMS$_EOF) {
        /* warn_user ("$GET %s",rmserror(rms_status,filedef)); */
        break;
      }
      if (rms_status == RMS$_RNF)
        {
          /* warn_user("Record Not Found %s %u",filterfn(__FILE__),__LINE__); */
          break;
        }
 
      if (filedef->where != NULL && (!(*filedef->where) ()))
        return number_elements;
 
      number_elements++;


      if (reverse)
        PutOnTop (&tempptr, filedef);
      else
        PutOnBottom (&tempptr, filedef);

      if (filedef->setprompt != NULL)
        (*filedef->setprompt) (tempptr, filedef);
      else
        warn_user ("Set List Prompt Not Set %s %u", filterfn (__FILE__),
                   __LINE__);

      rfa_copy (&tempptr->rfa, &filedef->rab.rab$w_rfa);

    }

  return number_elements;
}

int
rmsget (Rec, filedef)
     struct rfabuff *Rec;
     struct filestruct *filedef;
{

  filedef->rab.rab$b_rac = RAB$C_RFA;
  rfa_copy (&filedef->rab.rab$w_rfa, Rec);


  filedef->rab.rab$l_ubf = (char *) filedef->fileio;    /*record; */
  filedef->rab.rab$w_usz = filedef->fileiosz;
  filedef->rab.rab$l_rop = RAB$M_NLK;

  rms_status = sys$get (&filedef->rab);

  if (!rmstest
      (rms_status, 6, RMS$_NORMAL, RMS$_RNF, RMS$_RLK, RMS$_OK_RLK, RMS$_EOF,
       RMS$_DEL))
    warn_user ("$GET %s",rmserror(rms_status,filedef));
  else if (rmstest (rms_status, 3, RMS$_RNF, RMS$_RLK, RMS$_DEL))
    warn_user ("%s", strerror (EVMSERR, rms_status));
  else
    return TRUE;

  return FALSE;

}


unsigned int
Count_Back (Csr)
     struct List_Element *Csr;
{
  struct List_Element *CountCsr = Csr;
  unsigned int Counter;

  if(Csr == NULL) return(0);

  for (Counter = 0; CountCsr->Prev != NULL; Counter++)
    {
      CountCsr = CountCsr->Prev;
    }
  return (Counter);
}



void
ReadMiddleOut (Rec, filedef)
     struct rfabuff *Rec;
     struct filestruct *filedef;
{

  int iter, number_elements, first = TRUE;
  struct List_Element *TempPtr;
  int testline;
  struct List_Element *midptr;
  int back = FALSE, none = FALSE;
  

  filedef->rab.rab$b_rac = RAB$C_RFA;
  rfa_copy (&filedef->rab.rab$w_rfa, Rec);

  filedef->rab.rab$l_ubf = (char *) filedef->fileio;    /*record; */
  filedef->rab.rab$w_usz = filedef->fileiosz;
  filedef->rab.rab$l_rop = RAB$M_NLK;

  rms_status = sys$get (&filedef->rab);

  if (!rmstest (rms_status, 4, RMS$_NORMAL, RMS$_RNF, RMS$_RLK, RMS$_DEL))
    {
      warn_user ("$GET %s", rmserror(rms_status,filedef));
      return;
    }
  else if (rmstest (rms_status, 2, RMS$_RNF, RMS$_RLK))
    {
      warn_user ("%s", strerror (EVMSERR, rms_status));
    }
  else
    {


      midptr = filedef->CurrentPtr;

      while (rms_status == RMS$_DEL)
        {     
          rfa_copy (&filedef->rab.rab$w_rfa,&filedef->CurrentPtr->rfa);      
          rms_status = sys$get (&filedef->rab);
          if (!rmstest (rms_status, 3, RMS$_NORMAL, RMS$_OK_RLK, RMS$_DEL))
              warn_user ("$GET %s", rmserror(rms_status,filedef));
          if (rms_status == RMS$_DEL) {
             if((filedef->CurrentPtr = (back)?filedef->CurrentPtr->Prev:filedef->CurrentPtr->Next) == NULL) {
                if(back) {
                   none = TRUE;
                   break;
                } else {
                   if((filedef->CurrentPtr = midptr->Prev) == NULL) {
                      none = TRUE;
                      break;
                   }
                   back = TRUE;
                }  
             }
          }
        }
      if (none) {
         DeleteList (filedef);
         return;
      }


      if (filedef->setlistkey != NULL)
        (*filedef->setlistkey) ();
      else
        warn_user ("Set List Key Not Set %s %u", filterfn (__FILE__),
                   __LINE__);

      DeleteList (filedef);

      filedef->rab.rab$b_krf = filedef->revrkeyid;
      filedef->rab.rab$l_kbf = (char *) filedef->listkey;
      filedef->rab.rab$b_ksz = filedef->listkeysz;
      filedef->rab.rab$b_rac = RAB$C_KEY;
/*       
         filedef->rab.rab$b_rac = RAB$C_RFA;
         rfa_copy(&filedef->rab.rab$w_rfa,Rec);
*/
      filedef->rab.rab$l_ubf = (char *) filedef->fileio;        /*record; */
      filedef->rab.rab$w_usz = filedef->fileiosz;
      filedef->rab.rab$l_rop = RAB$M_NLK | RAB$M_EQNXT /*|RAB$V_REV */ ;

      filedef->CurrentLine = (filedef->RegionLength-filedef->TopLine)/2;

      testline =
        ListRead (filedef->CurrentLine + 1 , INCLUDEFIRST,
                  TRUE, filedef, TRUE);

/* Middle Point */ filedef->CurrentPtr = filedef->LastPtr;
/* read below */
/*
          warn_user("%u %u %u",testline,Count_Back(filedef->CurrentPtr),
                            filedef->CurrentLine);
*/
       if(Count_Back(filedef->CurrentPtr) != filedef->CurrentLine)
          filedef->CurrentLine = Count_Back(filedef->CurrentPtr);


      filedef->rab.rab$b_krf = filedef->fwrdkeyid;
      filedef->rab.rab$l_kbf = (char *) filedef->listkey;
      filedef->rab.rab$b_ksz = filedef->listkeysz;
      filedef->rab.rab$b_rac = RAB$C_KEY;
/*
         filedef->rab.rab$b_rac = RAB$C_RFA;
         rfa_copy(&filedef->rab.rab$w_rfa,Rec);
*/
      filedef->rab.rab$l_ubf = (char *) filedef->fileio;        /*record; */
      filedef->rab.rab$w_usz = filedef->fileiosz;
      filedef->rab.rab$l_rop = RAB$M_NLK;

      rms_status = sys$get (&filedef->rab);

      if (!rmstest
          (rms_status, 4, RMS$_NORMAL, RMS$_RNF, RMS$_RLK, RMS$_OK_RRL))
        {
          warn_user ("$GET %s", rmserror(rms_status,filedef));
          return;
        }
      else if (rmstest (rms_status, 2, RMS$_RNF, RMS$_RLK))
        warn_user ("%s", strerror (EVMSERR, rms_status));
      else
        {
          filedef->rab.rab$b_rac = RAB$C_SEQ;

          ListRead (filedef->BottomLine -
                    (filedef->CurrentLine + filedef->TopLine), FALSE, FALSE,
                    filedef, TRUE);

          if (filedef->CurrentPtr == NULL)
            filedef->CurrentPtr = filedef->FirstPtr;

 

          /*   rmsget(Rec,filedef); /*Set Current Record to original value */
        }


    }

}

/*
int NewReadNextBlock (Count,Rec,reverse)
int Count;
struct rfabuff *Rec;
int reverse;
{
   int numread,i;
   struct List_Element *delptr;
   PrimFile.rab.rab$b_rac = RAB$C_RFA;
   rfa_copy(&PrimFile.rab.rab$w_rfa,Rec);

   PrimFile.rab.rab$l_ubf = (char *)&fileio;
   PrimFile.rab.rab$w_usz = sizeof fileio;
   PrimFile.rab.rab$l_rop = RAB$M_RRL;

   rms_status = sys$get(&PrimFile.rab);

   if (rms_status != RMS$_NORMAL && rms_status != RMS$_RNF &&
       rms_status != RMS$_RLK && rms_status != RMS$_OK_RRL) 
     {
       warn_user("$GET %s", rmserror(rms_status,&PrimFile)); 
       exit(rms_status);
     } 
   else
      if (rms_status == RMS$_RNF || rms_status == RMS$_RLK)
         if (rms_status == RMS$_RLK) warn_user ("Record Locked");
         else warn_user("Specified record does not exist."); 
      else {



   numread = ListRead(Count,FALSE,reverse,&PrimFile);

   for(i=numread;i > 0;i--) {
if (reverse) {
                     if(filedef->ListCount > filedef->RegionLength*2)
                        if(filedef->LastPtr != NULL && filedef->LastPtr->Prev != NULL) {
                           delptr = filedef->LastPtr;
                           filedef->LastPtr = filedef->LastPtr->Prev;
                           filedef->LastPtr->Next = NULL;
                           free(delptr);
                           filedef->ListCount--;
                        }
}else {
                     if(filedef->ListCount > filedef->RegionLength*2)
                        if(FirstPtr != NULL && FirstPtr->Next != NULL) {
                           delptr = FirstPtr;
                           FirstPtr = FirstPtr->Next;
                           FirstPtr->Prev = NULL;
                           free(delptr);
                           filedef->ListCount--;
                        }
}
   }
   }
   return(numread > 0);

}
*/

int
ReadNextBlock (Count, Rec, reverse, filedef)
     int Count;
     struct rfabuff *Rec;
     int reverse;
     struct filestruct *filedef;
{
  int number_elements, i;
  struct List_Element *tempptr = filedef->CurrentPtr, *delptr;
  int iter, bump = FALSE, delflag = FALSE;

  filedef->rab.rab$b_rac = RAB$C_RFA;
  rfa_copy (&filedef->rab.rab$w_rfa, Rec);

  filedef->rab.rab$l_ubf = (char *) filedef->fileio;    /*record; */
  filedef->rab.rab$w_usz = filedef->fileiosz;
  filedef->rab.rab$l_rop = RAB$M_NLK;

  rms_status = sys$get (&filedef->rab);

  if (!rmstest
      (rms_status, 5, RMS$_NORMAL, RMS$_RNF, RMS$_RLK, RMS$_OK_RLK, RMS$_DEL))
    warn_user ("$GET %s", rmserror(rms_status,filedef));
  else if (rmstest (rms_status, 2, RMS$_RNF, RMS$_RLK))
    warn_user ("%s", strerror (EVMSERR, rms_status));
  else
    {


      while (rms_status == RMS$_DEL)
        {
          rfa_copy (&filedef->rab.rab$w_rfa,(reverse)?&filedef->FirstPtr->rfa:&filedef->LastPtr->rfa);
          rms_status = sys$get (&filedef->rab);
          if (!rmstest (rms_status, 3, RMS$_NORMAL, RMS$_OK_RLK, RMS$_DEL))
              warn_user ("$GET %s", rmserror(rms_status,filedef));
          if (rms_status == RMS$_DEL) {
              DeleteLine ((reverse)?&filedef->FirstPtr:&filedef->LastPtr, filedef);
              delflag = TRUE;
              if(((reverse)?filedef->FirstPtr:filedef->LastPtr) == NULL) {
                break;
              }
          }
        }
       if(delflag) {
          if(filedef->FirstPtr != NULL) {
             filedef->CurrentPtr =(reverse)?filedef->FirstPtr:filedef->LastPtr;    
             if(Count_Back(filedef->CurrentPtr) != filedef->CurrentLine)
                filedef->CurrentLine = Count_Back(filedef->CurrentPtr);
          }
          Redraw_Screen (filedef);
       }
       if (filedef->FirstPtr == NULL) {
          Read_Directory (filedef->fwrdkeyid, filedef);
          if (filedef->FirstPtr != NULL)
            {
              filedef->CurrentPtr = filedef->FirstPtr;
              return (TRUE);
            }
            return(FALSE);
       }



      if (filedef->setlistkey != NULL)
        (*filedef->setlistkey) ();
      else
        warn_user ("Set List Key Not Set. %s %u", filterfn (__FILE__),
                   __LINE__);

      filedef->rab.rab$b_krf =
        (reverse) ? filedef->revrkeyid : filedef->fwrdkeyid;

      filedef->rab.rab$l_kbf = (char *) filedef->listkey;
      filedef->rab.rab$b_ksz = filedef->listkeysz;
      filedef->rab.rab$b_rac = RAB$C_KEY;
      filedef->rab.rab$l_ubf = (char *) filedef->fileio;        /*record; */
      filedef->rab.rab$w_usz = filedef->fileiosz;
      filedef->rab.rab$l_rop = RAB$M_NLK;

      rms_status = sys$get (&filedef->rab);

      filedef->rab.rab$b_rac = RAB$C_SEQ;


      if (!rmstest
          (rms_status, 5, RMS$_NORMAL, RMS$_RNF, RMS$_RLK, RMS$_OK_RLK,
           RMS$_OK_ALK))
        warn_user ("$GET %s", rmserror(rms_status,filedef));
      else if (rmstest (rms_status, 2, RMS$_RNF, RMS$_RLK))
        warn_user ("%s", strerror (EVMSERR, rms_status));
      else
        {

          for (number_elements = 0; number_elements < Count;
               /* number_elements++ */ )
            {

              rms_status = sys$get (&filedef->rab);

              if (!rmstest
                  (rms_status, 3, RMS$_NORMAL, RMS$_EOF, RMS$_OK_RLK))
                {
                  warn_user ("$GET %s", rmserror(rms_status,filedef));
                  break;
                }
              else if (rms_status == RMS$_EOF)
                {
                  break;
                }

              if (filedef->where != NULL && !(*filedef->where) ())
                break;

/*                  if (ShowRead || (!if_record_read(&fileio.record.posted))){ */

              number_elements++;


              if (reverse)
                {
                  if (filedef->ListCount > filedef->RegionLength * 2)
                    if (filedef->LastPtr != NULL
                        && filedef->LastPtr->Prev != NULL)
                      {
                        delptr = filedef->LastPtr;
                        filedef->LastPtr = filedef->LastPtr->Prev;
                        filedef->LastPtr->Next = NULL;
                        free (delptr);
                        filedef->ListCount--;
                      }
                }
              else
                {
                  if (filedef->ListCount > filedef->RegionLength * 2)
                    if (filedef->FirstPtr != NULL
                        && filedef->FirstPtr->Next != NULL)
                      {
                        delptr = filedef->FirstPtr;
                        filedef->FirstPtr = filedef->FirstPtr->Next;
                        filedef->FirstPtr->Prev = NULL;
                        free (delptr);
                        filedef->ListCount--;
                      }
                }


              if (reverse)
                PutOnTop (&tempptr, filedef);
              else
                PutOnBottom (&tempptr, filedef);

              if (filedef->setprompt != NULL)
                (*filedef->setprompt) (tempptr, filedef);
              else
                warn_user ("Set List Prompt Not Set. %s %u",
                           filterfn (__FILE__), __LINE__);

              rfa_copy (&tempptr->rfa, &filedef->rab.rab$w_rfa);


              /*    if (number_elements == 0) 
                 Status (" [End of Records.]") ; */
/*                  } */
            }

        }
    }
  return (number_elements > 0);
}


void
Read_Directory (index_key, filedef)
     int index_key;
     struct filestruct *filedef;
{
  int number_elements;

  filedef->rab.rab$b_krf = index_key;


  if (filedef->where == NULL)
    {
      rms_status = sys$rewind (&filedef->rab);

      if (rms_status != RMS$_NORMAL)
        warn_user ("REWIND  Error %s rmsret: %u %s %u", filedef->filename,
                   rms_status, filterfn (__FILE__), __LINE__);
    }

  filedef->rab.rab$b_rac = RAB$C_SEQ;
  filedef->rab.rab$l_rop = RAB$M_NLK;
  filedef->rab.rab$l_ubf = (char *) filedef->fileio;    /*  record;  */
  filedef->rab.rab$w_usz = filedef->fileiosz;   /*RECORD_SIZE;  */


  DeleteList (filedef);
  number_elements = ListRead (filedef->RegionLength, FALSE,
                              filedef->revrkeyid == index_key ? TRUE : FALSE,
                              filedef, FALSE);

/*   if (number_elements == 0) 
      warn_user("There are no records under this key.") ;  
*/
}



void
Increment (IncLine, filedef)
     unsigned int *IncLine;
     struct filestruct *filedef;
{
  if (filedef->TopLine + (*IncLine) < filedef->BottomLine)
    {
      (*IncLine)++;
    }
  else
    {
      Scroll_Up (filedef->TopLine, filedef->BottomLine);
    }
}

void
Decrement (DecLine, filedef)
     unsigned int *DecLine;
     struct filestruct *filedef;
{
  if ((*DecLine) > 0)
    {
      (*DecLine)--;
    }
  else
    {
      Scroll_Down (filedef->TopLine, filedef->BottomLine);
    }
}

void
HiLite (Win, Prompt, Line_Num)
     unsigned int Win;
     char *Prompt;
     unsigned int Line_Num;
{
/*      Add_Attr (Win,
           Line_Num,
           2,
           Prompt,
           SMG$M_BOLD);
      Refresh(Win);
*/
  extscrprn (Prompt, Line_Num, 2, SMG$M_BOLD);
  Refresh (crnt_window->display_id);
}

void
LoLite (Win, Prompt, Line_Num)
     unsigned int Win;
     char *Prompt;
     unsigned int Line_Num;
{
/*      Add (Win,
           Line_Num,
           2,
           Prompt);
      Refresh(Win);
*/
  extscrprn (Prompt, Line_Num, 2, 0);
  Refresh (crnt_window->display_id);

}


void
Redraw_Screen (filedef)
     struct filestruct *filedef;
{
  struct List_Element *curs2;
  int LineNum = 0, i, trip = FALSE;

/*  Clear_Region (filedef->TopLine, filedef->BottomLine); */
  smg$erase_display(&crnt_window->display_id,&filedef->TopLine,&1);

  if (filedef->FirstPtr != NULL)
    {
      curs2 = filedef->CurrentPtr;
      for (i = 1; i <= filedef->CurrentLine; i++)
        {
          if (curs2 != filedef->FirstPtr)
            curs2 = curs2->Prev;

        }

      while (curs2 != NULL)
        {

          exclreol (filedef->TopLine + LineNum, 2, 0);
          extscrprn (curs2->Prompt, filedef->TopLine + LineNum, 2, 0);
          LineNum++;
          if (LineNum + filedef->TopLine > filedef->BottomLine)
            break;

          if (!trip && curs2->Next == NULL)
            {
              trip = TRUE;
  /* Test Removal 12-MAR-2025    if(!ReadNextBlock (filedef->RegionLength, &curs2->rfa, FALSE,
                             filedef)) break;*/
            }

          curs2 = curs2->Next;
        }
    }
  Refresh (crnt_window->display_id);
}







void
mkuid (dest, time)
     char *dest;
     struct timebuff time;
{
  static unsigned short int numvec[7];
  int status;

  status = sys$numtim (numvec, &time);
  if (!$VMS_STATUS_SUCCESS (status))
    {
      warn_user ("Numtime %s %s %u", strerror (status), filterfn (__FILE__),
                 __LINE__);
      return;
    }


  (void) sprintf (dest, "%04hu%02hu%02hu%02hu%02hu%02hu%02hu",
                  numvec[0],
                  numvec[1],
                  numvec[2], numvec[3], numvec[4], numvec[5], numvec[6]);
}



int
ReadSearchTerm (initial_value)
     char *initial_value;
{
  unsigned int status, i;
  uint16 lnth, terminator_code;
  char searchterm[SIZE_SEARCHTERM] = "";

  $DESCRIPTOR (subject_d, searchterm);
  $DESCRIPTOR (prompt_d, "Search  : ");
  $DESCRIPTOR (initstr_d, initial_value);


  prompt_d.dsc$w_length = 10;
  initstr_d.dsc$w_length = strlen (initial_value);

  status = smg$end_display_update (&Standard_Window);

  for (;;)
    {
      set_cursor (Standard_Window, HelpLine, 1);
      clreol (Standard_Window);
      status = smg$read_string (&KeyBoard,
                                &subject_d,
                                &prompt_d,
                                0,
                                0,
                                0,
                                0,
                                &lnth,
                                &terminator_code,
                                &Standard_Window, &initstr_d);
      if (terminator_code == SMG$K_TRM_CTRLZ)
        return (FALSE);
      if (lnth != 0)
        break;
    }

  searchterm[lnth] = 0;

  for (i = strlen (searchterm); i < SIZE_SEARCHTERM; i++)
    searchterm[i] = 0;

  strcpy (initial_value, searchterm);
  return (TRUE);

}



void
delete_editor_buffer (listptr)
     struct line_rec **listptr;
{
  struct line_rec *delptr;

  while ((*listptr) != NULL && (*listptr)->last != NULL)
    (*listptr) = (*listptr)->last;

  while ((*listptr) != NULL)
    {
      delptr = (*listptr);
      (*listptr) = (*listptr)->next;
      free (delptr);
    }

}



int
save_file (update, filedef)
     int update;
     struct filestruct *filedef;
{
  struct line_rec *curs;
  int ind = 0, j;
  int status;

  if (filedef->entry_comment != 0)
    {
      curs = filedef->entry_screen[filedef->entry_comment].field;       /* start_line; */
      if (filedef->dumpvarbuff != NULL)
        ind = (*filedef->dumpvarbuff) (curs, filedef);

/*moved below      delete_editor_buffer(&filedef->entry_screen[filedef->entry_comment].field); */
    }

  filedef->rab.rab$b_rac = RAB$C_KEY;
  filedef->rab.rab$l_rbf = (char *) filedef->fileio;

  if (filedef->fab.fab$b_rfm == FAB$C_FIX)
    filedef->rab.rab$w_rsz = filedef->fileiosz;
  else
    filedef->rab.rab$w_rsz = (ind == 0) ? filedef->recsize : ind;

  if (filedef->padrecord != NULL)
    (*filedef->padrecord) ();

     if(filedef->transaction) {
        status = sys$start_transw (event_flag, 0, &iosb, 0, 0, tid);
        check_status ("start");
     }

  if (update)
    {
      rms_status = sys$update (&filedef->rab);
    }
  else
    {
      rms_status = sys$put (&filedef->rab);
    }

  if (!rmstest (rms_status, 3, RMS$_NORMAL, RMS$_DUP, RMS$_OK_DUP))
    {
      if (rms_status = RMS$_CHG)
        warn_user ("%s",strerror (EVMSERR, rms_status));
      else
        warn_user ("$PUT %s", rmserror(rms_status,filedef));
      if(filedef->transaction) {
          status = sys$abort_transw (event_flag, 0, &iosb, 0, 0, tid);
          check_status ("abort");
      }    
      return (FALSE);
    }
  else if (rmstest (rms_status, 2, RMS$_NORMAL, RMS$_OK_DUP))
    {

      if(filedef->transaction) {
         status = sys$end_transw (event_flag, 0, &iosb, 0, 0, tid);
         check_status ("end");
      }

      if (update && (filedef->CurrentPtr != NULL))
        (*filedef->setprompt) (filedef->CurrentPtr, filedef);

      if (filedef->entry_comment != 0)
/*moved*/ delete_editor_buffer (&filedef->entry_screen[filedef->entry_comment].field);
      /* Status ("[Record Written.]");   */
      return (TRUE);
    }
  else
    {
      if(filedef->transaction) {
         status = sys$abort_transw (event_flag, 0, &iosb, 0, 0, tid);
         check_status ("abort");
      }

      warn_user ("%s %s %u",strerror(EVMSERR,rms_status),
                 filterfn (__FILE__), __LINE__);
      return (FALSE);
    }
}

void
get_username (void)
{
  static int i, item_code = JPI$_USERNAME;      /*JPI$_PRCNAM; */
  $DESCRIPTOR (user_name_d, user_name);


  rms_status = lib$getjpi (&item_code,
                           0, 0, 0, &user_name_d, &user_name_d.dsc$w_length);

  sprintf (user_name, "%-.*s",
           user_name_d.dsc$w_length, user_name_d.dsc$a_pointer);
  unpad (user_name, user_name_d.dsc$w_length);


}



void
Edit_Current_Record (update, filedef,nowin)
     int update;
     struct filestruct *filedef;
     int nowin;
{
  int edited = FALSE, exit = FALSE;
  int file_saved = FALSE;
  struct linedt_msg_rec msg;
  struct rfabuff LFindRec;
  int strtfld = 0;

  if (filedef->entry_comment != 0)
    {
      delete_editor_buffer (&filedef->entry_screen[filedef->entry_comment].
                            field /*&start_line */ );
      if (filedef->loadvarbuff != NULL)
        (*filedef->loadvarbuff) (&filedef->
                                 entry_screen[filedef->entry_comment].field,
                                 filedef);
    }


  if (filedef->unpadrecord != NULL)
    (*filedef->unpadrecord) ();

  if (update)
    {
      if (filedef->preedit != NULL)
        (*filedef->preedit) ();
    }
  else
    {
      if (filedef->preinsert != NULL)
        if (!(*filedef->preinsert) ()){
           warn_user("Pre Insert Failure");
           return;
        }
    }

  if (filedef->recbuff != NULL)
    memcpy (filedef->recbuff, filedef->fileio, filedef->recsize);

  if(!nowin) {
     spawn_edit_window (filedef, EDTLNTH, EDTWIDTH);

     init_screen (filedef->entry_screen, filedef->entry_length);
  }
  if (filedef->recalc != NULL)
    (*filedef->recalc) ();

  Refresh (crnt_window->display_id);
  while (!exit)
    {
      Help ("Record Editor  -   Arrow Keys  ^Z to exit");
      enter_screen (filedef->entry_screen, filedef->entry_length, &edited, nobreakout, filedef, &strtfld);      /* ,&msg);  */

      if (edited)
        {
          switch (warn_user ("Save Post :  Y/N"))
            {
            case 'y':
            case 'Y':
              if (filedef->recbuff != NULL)
                memcpy (filedef->fileio, filedef->recbuff, filedef->recsize);

              if (update) { 
                 if (filedef->postedit != NULL) (*filedef->postedit) ();
              } else { 
                 if (filedef->postinsert != NULL) (*filedef->postinsert) ();
              }

              if (save_file (update, filedef))
                {
                  file_saved = TRUE;
                  exit = TRUE;
                }
              break;
            case 'n':
            case 'N':
              exit = TRUE;
              break;
            }
        }
      else
        exit = TRUE;

    }
    if (update) {
       if (filedef->editcleanup != NULL)
                (*filedef->editcleanup) ();
    } else {
       if (filedef->insertcleanup != NULL)
                (*filedef->insertcleanup) ();
    }

  if(!nowin) {
     win_depth--;
     close_window ();
  }

  Help ("");

  if (filedef->entry_comment != 0)
    delete_editor_buffer (&filedef->entry_screen[filedef->entry_comment].
                          field);

  if (!update)
    {

      if (filedef->FirstPtr == NULL)
        {
          Read_Directory (filedef->fwrdkeyid, filedef);
          filedef->CurrentPtr = filedef->FirstPtr;

        }
      else
        {
          if (!file_saved)
            rfa_copy (&LFindRec, &filedef->CurrentPtr->rfa);
          else
            rfa_copy (&LFindRec, (struct rfabuff *) &filedef->rab.rab$w_rfa);

          DeleteList (filedef);
          ReadMiddleOut (&LFindRec, filedef);
        }
      if(Count_Back(filedef->CurrentPtr) != filedef->CurrentLine)
          filedef->CurrentLine = Count_Back(filedef->CurrentPtr);


    }


}






void
Show_Record (Rec, filedef)
     struct rfabuff *Rec;
     struct filestruct *filedef;
{
  int edited = FALSE;
  uint16 c, scan = 0;
  struct linedt_msg_rec msg;
  unsigned long row, col;


/*  filedef->rab.rab$b_krf = 2; */

  rfa_copy (&filedef->rab.rab$w_rfa, Rec);
  filedef->rab.rab$b_rac = RAB$C_RFA;

  filedef->rab.rab$l_ubf = (char *) filedef->fileio;    /*record; */
  filedef->rab.rab$w_usz = filedef->fileiosz;

  filedef->rab.rab$l_rop = RAB$M_NLK;

  rms_status = sys$get (&filedef->rab);

  if (!rmstest
      (rms_status, 5, RMS$_NORMAL, RMS$_RNF, RMS$_RLK, RMS$_OK_RRL,
       RMS$_OK_RLK))
    warn_user ("$GET %s", rmserror(rms_status,filedef));
  else if (rmstest (rms_status, 2, RMS$_RNF, RMS$_RLK))
       warn_user ("%s", strerror (EVMSERR, rms_status));
  else
    {
      /*  Status(""); */
/*      Clear_Region (filedef->TopLine, filedef->BottomLine);*/
      smg$erase_display(&crnt_window->display_id,&filedef->TopLine,&1);

      if (filedef->entry_comment != 0)
        {
          delete_editor_buffer (&filedef->
                                entry_screen[filedef->entry_comment].
                                field /*&start_line */ );
          if (filedef->loadvarbuff != NULL)
            (*filedef->loadvarbuff) (&filedef->
                                     entry_screen[filedef->entry_comment].
                                     field /*&start_line */ ,
                                     filedef);
        }

      if (filedef->preview != NULL) (*filedef->preview) ();

      spawn_edit_window (filedef, EDTLNTH, EDTWIDTH);

      row = smg$cursor_row (&crnt_window->display_id);
      col = smg$cursor_column (&crnt_window->display_id);

      if (filedef->unpadrecord != NULL)
        (*filedef->unpadrecord) ();

      if (filedef->recbuff != NULL)
        memcpy (filedef->recbuff, filedef->fileio, filedef->recsize);

      smg$erase_display(&crnt_window->display_id);
      init_screen (filedef->entry_screen, filedef->entry_length);

      if (filedef->recalc != NULL)
        (*filedef->recalc) ();

      smg$set_cursor_abs (&crnt_window->display_id, &row, &col);
      Refresh (crnt_window->display_id);

      /* Get_Keystroke(); */
      while (scan != SMG$K_TRM_CTRLZ)
        {
          command_line (filedef->read_cmndline, &scan, &msg, filedef);

/* ZEN ADDITION 26-MAY-2023 23:35:50 */
          if (scan != SMG$K_TRM_CTRLZ)
            {
              rfa_copy (&filedef->rab.rab$w_rfa, Rec);
              filedef->rab.rab$b_rac = RAB$C_RFA;
              filedef->rab.rab$l_ubf = (char *) filedef->fileio;        /*record; */
              filedef->rab.rab$w_usz = filedef->fileiosz;
              filedef->rab.rab$l_rop = RAB$M_NLK;

              rms_status = sys$get (&filedef->rab);

              if (!rmstest (rms_status, 2, RMS$_NORMAL, RMS$_OK_RLK))
                {
                  warn_user ("$GET %s", rmserror(rms_status,filedef));
                  break;
                }

              if (filedef->entry_comment != 0)
                delete_editor_buffer (&filedef->
                                      entry_screen[filedef->entry_comment].
                                      field);
              if (filedef->loadvarbuff != NULL)
                (*filedef->loadvarbuff) (&filedef->
                                         entry_screen[filedef->entry_comment].
                                         field, filedef);
            }

/* ZEN END ADDITION  */

/* ZEN MOD 28-MAY-2023 22:28:58 */
/*          Clear_Region (filedef->TopLine, filedef->BottomLine);  */

/*ZEN */ if (filedef->unpadrecord != NULL)
            (*filedef->unpadrecord) ();

          if (filedef->recbuff != NULL)
            memcpy (filedef->recbuff, filedef->fileio, filedef->recsize);

 
          smg$erase_display(&crnt_window->display_id);
          init_screen (filedef->entry_screen, filedef->entry_length);

          if (filedef->recalc != NULL)
            (*filedef->recalc) ();

          Refresh (crnt_window->display_id);
        }  /* end while */

      if (filedef->postview != NULL) (*filedef->postview) ();

      win_depth--;
      close_window ();
      if (filedef->entry_comment != 0)
        delete_editor_buffer (&filedef->entry_screen[filedef->entry_comment].
                              field /*&start_line */ );


    }
}




void
Seek_and_Edit (Rec, update, filedef,nowin)
     struct rfabuff *Rec;
     int update;
     struct filestruct *filedef;
     int nowin;
{

  filedef->rab.rab$b_rac = RAB$C_RFA;
  rfa_copy (&filedef->rab.rab$w_rfa, Rec);

  filedef->rab.rab$l_ubf = (char *) filedef->fileio;    /*record; */
  filedef->rab.rab$w_usz = filedef->fileiosz;
  filedef->rab.rab$l_rop = RAB$M_RLK;   /* write lock *//*|RAB$M_ULK; /* RAB$M_RRL; */

  rms_status = sys$get (&filedef->rab);

  if (!rmstest (rms_status, 4, RMS$_NORMAL, RMS$_RNF, RMS$_RLK, RMS$_OK_RRL))
    warn_user ("$GET %s", rmserror(rms_status,filedef));

  else if (rmstest (rms_status, 2, RMS$_RNF, RMS$_RLK))
    /* ZEN  if (rms_status == RMS$_RLK) warn_user (strerror (EVMSERR, rms_status));
       else warn_user ("EDITREC Record Not Found."); */
    warn_user ("%s",strerror (EVMSERR, rms_status));
  else
    {
      Edit_Current_Record (update, filedef,nowin);
    }
  sys$free (&filedef->rab);
}

void
Edit_New_Record (filedef)
     struct filestruct *filedef;
{

  memset (filedef->fileio, 0, filedef->fileiosz);

  if (filedef->entry_comment != 0)
    filedef->entry_screen[filedef->entry_comment].field = NULL;

  Edit_Current_Record (FALSE, filedef,FALSE);

}





void
Delete_Record (Rec, filedef)
     struct rfabuff *Rec;
     struct filestruct *filedef;
{
  int status;




  if(filedef->predelete != NULL) {
     if (!(*filedef->predelete)()) {
        warn_user("Pre Delete Operation Failed");
        return;
     }
  }


  filedef->rab.rab$b_rac = RAB$C_RFA;
  rfa_copy (&filedef->rab.rab$w_rfa, Rec);

  filedef->rab.rab$l_ubf = (char *) filedef->fileio;    /*record; */
  filedef->rab.rab$w_usz = filedef->fileiosz;

  filedef->rab.rab$l_rop = RAB$M_RLK;

  rms_status = sys$find (&filedef->rab);

  if (!rmstest (rms_status, 3, RMS$_NORMAL, RMS$_RNF, RMS$_RLK))
    warn_user ("$FIND %s", rmserror(rms_status,filedef));
  else if (rms_status == RMS$_RNF)
    warn_user ("$FIND %s", rmserror(rms_status,filedef));
  else if (rms_status == RMS$_RLK)
    {
      warn_user ("%s", strerror (EVMSERR, rms_status));
    }
  else
    {


     if(filedef->transaction){
        status = sys$start_transw (event_flag, 0, &iosb, 0, 0, tid);
        check_status ("start");
     }


      rms_status = sys$delete (&filedef->rab);
      if (rms_status != RMS$_NORMAL) {
         if(filedef->transaction){
            status = sys$abort_transw (event_flag, 0, &iosb, 0, 0, tid);
            check_status ("abort");
         }
         warn_user ("$DELETE %s", rmserror(rms_status,filedef));
      } else {
         if(filedef->transaction){
            status = sys$end_transw (event_flag, 0, &iosb, 0, 0, tid);
            check_status ("end");
         }

/* ZEN Delete Last Line dilema */
        if (filedef->CurrentPtr->Next == NULL && filedef->CurrentLine > 0)
          filedef->CurrentLine--;

        DeleteLine (&filedef->CurrentPtr, filedef);


      }
    }
  if(filedef->postdelete != NULL) (*filedef->postdelete)();
  sys$free (&filedef->rab);
}

void
active_align_list (CPtr, FindRec, filedef)
     struct List_Element **CPtr;
     struct rfabuff *FindRec;
     struct filestruct *filedef;
{
  int i;

  (*CPtr) = filedef->FirstPtr;
  while ((*CPtr) != NULL)
    {
      if (rfa_equal ((*CPtr)->rfa, (*FindRec)))
        break;
      (*CPtr) = (*CPtr)->Next;
    }
  if ((*CPtr) == NULL)
    {
      (*CPtr) = filedef->FirstPtr;
      for (i = 0; i < filedef->CurrentLine; i++)
        if ((*CPtr)->Next != NULL)
          (*CPtr) = (*CPtr)->Next;

    }
}


void
RefreshList (filedef)
     struct filestruct *filedef;
{
  struct rfabuff FindRec;
  if (filedef->FirstPtr == NULL)
    {
      Read_Directory (filedef->fwrdkeyid, filedef);
      filedef->CurrentPtr = filedef->FirstPtr;
    }
  else
    {
      rfa_copy (&FindRec, &filedef->CurrentPtr->rfa);
/*      DeleteList (filedef); */
      ReadMiddleOut (&FindRec, filedef);
      if(filedef->FirstPtr == NULL) {
         Read_Directory (filedef->fwrdkeyid, filedef);
         filedef->CurrentPtr = filedef->FirstPtr;
      }
    }
}



struct rfabuff *
File_Browse (filedef, opmode)
     struct filestruct *filedef;
     enum filebrowse_mode opmode;
{
  uint16 c, scan;
  int i, exit = FALSE, ende = TRUE;
  int swtch;
  extern char searchkey[];
  static struct rfabuff retrfa;

  struct linedt_msg_rec msg;

/*  Clear_Region (filedef->TopLine, filedef->BottomLine); */
  smg$erase_display(&crnt_window->display_id,&filedef->TopLine,&1);


  filedef->CurrentLine = 0;
  Read_Directory (filedef->fwrdkeyid, filedef);
  filedef->CurrentPtr = filedef->FirstPtr;

  while (!exit)
    {
      Help
        ("|^I Insert |^R Search |^E Edit   |^D Delete |^W Reindex| PgUp/PgDn  ^Z to exit");

      if (filedef->FirstPtr != NULL)
        {
          Redraw_Screen (filedef);
          clreol (crnt_window->display_id);
          HiLite (crnt_window->display_id, filedef->CurrentPtr->Prompt,
                  filedef->CurrentLine + filedef->TopLine);
        }
      else smg$erase_display(&crnt_window->display_id,&filedef->TopLine,&1);
/*        Clear_Region (filedef->TopLine, filedef->BottomLine); */
      Refresh (crnt_window->display_id);

/*         c = Get_Keystroke(); */

      c = command_line (filedef->list_cmndline, &scan, &msg, filedef);

      if (c == 0)
        c = scan;

      if ((c >= 1 && c <= 31) || (c >= 127 && c <= 328))
        {
          switch (c)
            {
/*            case SMG$K_TRM_PF1: */
            case SMG$K_TRM_CTRLI:
              /*   Open_Menu(1,MenuSpec);  */
              Insrt_Record (&msg, filedef);
              break;
            case SMG$K_TRM_CTRLR:
/*            case SMG$K_TRM_PF2:  */

              if (filedef->mainsrch != NULL)
                (*filedef->mainsrch) (&msg, filedef, 0);

              /*  Search_Record(&msg,filedef); */
              break;
            case SMG$K_TRM_CTRLE:
/*            case SMG$K_TRM_PF3: */
              Edit_Record (&msg, filedef);
              break;
            case SMG$K_TRM_CTRLD:
/*            case SMG$K_TRM_PF4: */
              Remove_Record (&msg, filedef);
              break;
            case SMG$K_TRM_CTRLW:
              RefreshList (filedef);
              break;
            case SMG$K_TRM_CTRLU:

              break;

            case SMG$K_TRM_DOWN:
              if (filedef->FirstPtr ==NULL) break;
              if (filedef->CurrentPtr->Next == NULL)
                if (!ReadNextBlock
                    (5, &filedef->LastPtr->rfa, FALSE, filedef))
                  break;
              LoLite (crnt_window->display_id, filedef->CurrentPtr->Prompt,
                      filedef->CurrentLine + filedef->TopLine);
              Increment (&filedef->CurrentLine, filedef);
              filedef->CurrentPtr = filedef->CurrentPtr->Next;
              break;
            case SMG$K_TRM_UP:
              if (filedef->FirstPtr ==NULL) break;
              if (filedef->CurrentPtr->Prev == NULL)
                if (!ReadNextBlock
                    (5, &filedef->FirstPtr->rfa, TRUE, filedef))
                  break;
              LoLite (crnt_window->display_id, filedef->CurrentPtr->Prompt,
                      filedef->CurrentLine + filedef->TopLine);
              Decrement (&filedef->CurrentLine, filedef);
              filedef->CurrentPtr = filedef->CurrentPtr->Prev;
              break;
            case SMG$K_TRM_NEXT_SCREEN:
              if (filedef->FirstPtr ==NULL) break;
              for (i = 0; i <= filedef->BottomLine - filedef->TopLine; i++)
                {
                  if (filedef->CurrentPtr->Next == NULL)
                    if (!ReadNextBlock
                        (filedef->RegionLength, &filedef->LastPtr->rfa, FALSE,
                         filedef))
                      break;
                  filedef->CurrentPtr = filedef->CurrentPtr->Next;
                }
              break;
            case SMG$K_TRM_PREV_SCREEN:
              if (filedef->FirstPtr ==NULL) break;
              for (i = 0;
                   i <=
                   (filedef->BottomLine - filedef->TopLine) +
                   filedef->CurrentLine; i++)
                {
                  if (filedef->CurrentPtr->Prev == NULL)
                    if (!ReadNextBlock
                        (filedef->RegionLength, &filedef->FirstPtr->rfa, TRUE,
                         filedef))
                      break;
                  filedef->CurrentPtr = filedef->CurrentPtr->Prev;
                }
              for (i = 0; i < filedef->CurrentLine; i++)
                if (filedef->CurrentPtr->Next != NULL)
                  filedef->CurrentPtr = filedef->CurrentPtr->Next;

              break;
/*
--               when Key_Home =>
--                  CurrentCurs := Directory_Buffer.First;
--                  CurrentLine := 0;
--                  Redraw_Screen(filedef);
--               when Key_End =>
---                  CurrentCurs := Directory_Buffer.Last;
--                  CurrentLine := filedef->BottomLine - filedef->TopLine;
--                  Redraw_Screen(filedef);
--            when Key_Resize =>
--               Get_Size(Standard_Window,Number_Of_Lines => TermLnth,Number_Of_Columns => TermWdth);
--               filedef->BottomLine := TermLnth - 4;
--               Clear();
--               Redraw_Screen(filedef);
*/
            case SMG$K_TRM_CTRLZ:
              exit = TRUE;
              break;
            case SMG$K_TRM_LF:
            case SMG$K_TRM_CR:
              /*  Status("");  */
              if (filedef->CurrentPtr != NULL)
                {
                  if (opmode == select)
                    {

/* ZEN MODIFICATION for Selector Mode 27-MAY-2023 19:48:47 */
/* Reintroduce this block if ever needed   */
                      filedef->rab.rab$b_rac = RAB$C_RFA;
                      rfa_copy (&filedef->rab.rab$w_rfa,
                                &filedef->CurrentPtr->rfa);

                      filedef->rab.rab$l_ubf = (char *) filedef->fileio;
                      filedef->rab.rab$w_usz = filedef->fileiosz;
                      filedef->rab.rab$l_rop = RAB$M_NLK;

                      rms_status = sys$get (&filedef->rab);
                      if (!rmstest (rms_status, 2, RMS$_NORMAL, RMS$_OK_RRL))
                        warn_user ("$GET %s", rmserror(rms_status,filedef));

                      rfa_copy (&retrfa, &filedef->CurrentPtr->rfa);

                      /*   rmsget(&retrfa,filedef); */

                      DeleteList (filedef);
                      return (&retrfa);

/* END OF MOD */

                    }
                  else if (opmode == normal)
                    {
                      Show_Record (&filedef->CurrentPtr->rfa, filedef);
                      Redraw_Screen (filedef);
                    }
                }
              break;

            default:
              break;
            }
        }
      else if (c >= 32 && c <= 126)
        {
          /* Process regular alpha keys here */

          searchkey[0] = c;
          searchkey[1] = 0;
          if (filedef->mainsrch != NULL)
            (*filedef->mainsrch) (&msg, filedef, c);
        }

    }                           /*end of while */

/* ZEN ADDITION 27-MAY-2023 21:49:50 */
  DeleteList (filedef);
  retrfa.b1 = retrfa.b2 = 0;
  return (&retrfa);

}



int
menutest ()
{
  return (1);

}


void
Insrt_Record (msg, filedef)
     struct linedt_msg_rec *msg;
     struct filestruct *filedef;
{
  Edit_New_Record (filedef);

}

void
Edit_Record (msg, filedef)
     struct linedt_msg_rec *msg;
     struct filestruct *filedef;
{
  if (filedef->CurrentPtr == NULL)
    return;

  Seek_and_Edit (&filedef->CurrentPtr->rfa, EDIT_MODE, filedef,FALSE);
}

void
Edit_Record_NW (msg, filedef)
     struct linedt_msg_rec *msg;
     struct filestruct *filedef;
{
  if (filedef->CurrentPtr == NULL)
    return;

  Seek_and_Edit (&filedef->CurrentPtr->rfa, EDIT_MODE, filedef,TRUE);
}


void
Remove_Record (msg, filedef)
     struct linedt_msg_rec *msg;
     struct filestruct *filedef;
{
  if (filedef->CurrentPtr == NULL)
    return;

  if (strcmp (user_name, priv_user) != 0)
    {
      warn_user ("Not Authorised to Delete %s %u", filterfn (__FILE__),
                 __LINE__);
      return;
    }


  switch (warn_user ("Do you want to Delete this record :  Y/N"))
    {
    case 'Y':
    case 'y':
      if(toupper(warn_user("Confirm Delete : Y/N")) == 'Y') 
        if (filedef->CurrentPtr != NULL)
          Delete_Record (&filedef->CurrentPtr->rfa, filedef);

      break;
    case 'n':
    case 'N':
      break;
    default:
      break;
    }

}


void
reread_index (msg, filedef)
     struct linedt_msg_rec *msg;
     struct filestruct *filedef;
{

  RefreshList (filedef);

}
