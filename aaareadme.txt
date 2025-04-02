
SWinvoice is a system for generating invoices and receipts from a database
of inventory items. It prints or emails invoices and has a batch mode for
sending recurrent invoices on a periodical basis.


For Openvms on VAX, Alpha and X86_64

         House Harris Software
Author : Michael Brown, Newcastle Australia.
Email  : vmslib@b5.net
Http   : https://eisner.decuserve.org/~brown_mi/ For latest versions.

BUILDING EXECUTABLE:

edit SWLOCALITY.H where you can specify the path to the data
files directory, archive directory  and the privileged user
(can edit master record and delete any record).

edit SWOPTIONS.H where you can specify certain operation.
#undef DIAGS will suppress the pop up diagnostic window to catch
output from the mail or print commands. Having it defined may aid
configuration.

#define TRANSACTION if you have the DEC Transaction manager running,
swinvoice runs best with transactions enabled but enabling DECdtm
is beyond the scope of this document.

Only modify those 2 defines.
 

To build SWinvoice executable first change to the EXTOOLS directory
and then run MMS.This will build the EXTOOLS.OLB library.

Change directory back to the SWinvoice source and execute MMS
again. The result of this operation is the main client executable
SWINVOICE.EXE.

The additionally you can
execute MMS BATCHINVC.EXE for the batch invoicing.
execute MMS TRANSCAN.EXE  for the transactions report.
execute MMS INVCSCAN.EXE  for the invoices report.

INSTRUCTIONS FOR OPERATION:

SWinvoice requires a good VT100 terminal emulation with the PF1 to
PF4 function keys as well as the arrow keys, page up and down,
insert and delete to drive it. 

run SWINVOICE to start up the client. It is largely self documenting
with help line that shows the operations associated with the function
keys PF1 to PF4 which will change depending on the context.

You would work this out easily just following your nose, but I've mapped
out a procedure to tutor you.

From the First Screen,

To start, press PF4 then select Master Record. Then press PF1 to edit
the master record. Set the profit margins for wholesale/retail and
tax rates you use.

Add the printer queue name

Add your email details address and subject line that will be sent with
emailed receipts and invoices.

Add your business name and address which will be printed on invoices and
receipts.

press CTRL Z and select to save master record.

Now back at the First Screen, press PF1 to create a client record.
You have a main address and an alternate postal address for shipping.
Note when your cursor is on the date field PF2 inserts the current date.

press CTRL Z and select to save new record.

Back at the first screen press PF2 and an empty invoice window will
open up, press PF1 to enter new invoice. Again enter a date, PF2 for
todays date. Run the cursor down to the bottom with down arrow key,
you will be in the invoice editor now.

When you are in the Code field you can press PF1 and an empty inventory
item select window will open, press PF1 to enter an inventory item
record. Enter an alphanumeric part code used to reference this item.
Enter Description which will be entered into an invoice. Enter the units
that the item is sold by and the cost per unit.

press CTRL Z and select to save new inventory item.

press enter to select the newly created item and you will be back in
the invoice editor with the new item in the editor buffer.

press return until you are in the Number column and enter the number of
units for the order. Totals will recalculate.

press CTRL Z and select to save new invoice.

Now you can see the new invoice in the invoices list, you can then print
or send by email the new invoice by pressing PF3 and going through the
3 options selecting whether to email the invoice or default let it print.

press CTRL Z to return to the First Screen, press PF3 and the account
balance will show a debit of the invoice value. Press PF3 again and the
receipt window will appear, enter the amount of payment and whether the
receipt should be emailed or printed.

That is the whole cycle for producing an invoice and receipt, I think
if you've run through that you understand how it operates and the rest
is just data entry of inventory which is directly accessible from the
First Screen PF4.

BATCH PROCESSING OF RECURRENT INVOICES:

SWinvoice will automatically send recurrent invoices on a periodical
basis with the BATCHINVC utility. BATCHINVC looks for invoice records
that match the Class field that you specify.

First create or edit an existing invoice and run the cursor down to
the Class field and enter an alphabetic key eg. MONTHLY, This makes
it a recurrent invoice.
Then exit swinvoice and enter the DCL command.

$ batchinvc monthly -test

That should list the invoice just edited. You are ready to run the
batch.

$ batchinvc monthly -print -post

That will either issue a new invoice cloned from the original with new
invoice and quote numbers, or it will fail with error messages and
leave file called LEFTOVERS.TXT in the defined archive directory.

If there have been failures to send any invoice you can correct the
cause of the failure and run the failed invoices again with

$ batchinvc -left -print -post

This will read the data in LEFTOVERS.TXT and try to send the invoice
again. You can run the last command repeatedly until all invoices are
sent, at that point the leftovers file will be deleted.


An example MONTHLY.COM and FORTNIGHTLY.COM can be submitted to your
system batch queue to automate the batch invoice run.

REPRINT RECEIPT OR INVOICE:

You can at any time resend/reprint any receipt or invoice from the
archive if necessary.

$ batchinvc -invc -print 1

This will reprint invoice number 00001 from the archive directory.
Substitute -rcpt for -invc  and it will print receipt number 00001.

Finding the receipt number to reprint can be done with the TRANSCAN
utility.

$ transcan 1

This will list all the transactions for client number 00001 which
displays receipt/invoice numbers and amounts. With no parameters
transcan returns a list of all transactions.


RECOVERY UNIT JOURNALING:

SWinvoice runs well without transactions enabled but there are
error situations that could leave inconsistent data behind. So if
you have DECdtm services running on your system you must use
the following proceedure to use it. 

$ edit SWOPTIONS.H and #define TRANSACTION
$ mms
$ mms batchinvc.exe
To recompile the modules that engage in RU transactions.

$ @START$JNL

This will enable RU journaling on the swtrans.dat and swquote.dat
which are the only files that engage in RU journaling.


The main difference with transactions enabled is batch invoicing,
if an invoice fails to send it will roll back all changes made
during the issuing of the invoice,leaving the database ready to
try to reprocess the invoice later.

If transactions are disabled and an invoice fails to send and if it
has at least succeeded in writing the invoice file before sending
it can be resent at a later date (batchinvc -left). If some other
failure has occurred and there is no invoice file written, the error
is serious enough to leave account balances not updated or invoices
could be in the database un-issued.

Another difference is sending receipts from the swinvoice client. If
a receipt fails to send transactions will roll back the records of
the receipt. Without transactions the client will delete the receipt
records itself. You should not notice any difference operationally
but again errors could corrupt data.

Deleting a client will delete all records related to that client so
transactions ensure that all are deleted completely.













