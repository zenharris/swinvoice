$ 
$!  show queue
$!  delete/entry=
$
$ DEFINE DCL$PATH []
$ set default [mbrown.swinvoice3_4]
$ SUBMIT/AFTER="+14-" fortnightly.com
$
$ batchinvc fortnightly -email
$ 

