Starting the controller
-----------------------

sudo /etc/init.d/gaggia start

Stopping the controller
-----------------------

The halt button should be pushed to provide an opportunity for the PI to
shutdown before removing mains power.

sudo /etc/init.d/gaggia stop

Configuration files
-------------------

/etc/gaggia.conf

Data files
----------

Data is recorded to:
/var/log/gaggia/

Each run results in a new CSV (Comma Separated Values) file which is named
with the start date and time as follows:
YYMMDD-HHMM.csv

There is no upper limit on the number or size of these files, so they need
to be cleaned up manually. The files are fairly small (even if the machine
is left on for a couple of hours, the file would only be around 180kb).
