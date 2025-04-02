$ 
$!  show queue
$!  delete/entry=
$
$ DEFINE DCL$PATH []
$ set default [mbrown.swinvoice3_4]
$ SUBMIT/AFTER="+1" hourly.com
$
$ batchinvc testclass -email
$ 

