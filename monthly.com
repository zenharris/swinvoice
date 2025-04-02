$ 
$!  show queue
$!  delete/entry=
$
$ DEFINE DCL$PATH []
$ set default [mbrown.swinvoice3_4]
$! 5th day of the month
$! N = 5
$ nth="''N'-"+-
F$CVTIME("1-+31-","ABSOLUTE","MONTH")+-
"-"+F$CVTIME("1-+31-",,"YEAR")
$ SUBMIT/AFTER="''nth'" monthly.com
$
$ batchinvc monthly -email
$ 

