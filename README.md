# tasmota-sql

Tools for management of tasmota devices based on mysql.

The tasconfig command can load config from tasmota and store in sql, or load from sql and send to tasmota (but only where fields are different). It has option to handle all tasmota it can see. All done via mqtt. Makes backing up all config really simple, and applying changes very simple too.

The mysql table tasmota has fields for tasmota settings. These can be added to as needed and hence allow for more and more settings as needed. The fields should have a mysql comment defined to explain the field in question.

The Topic field is the key. There are special cases for some fields, e.g. RuleN will send a RuleN 1 after setting the rule if it is not being set to "0". Field values that would be "OFF"/"ON" in SetOptionN are handled as "0"/"1".

A template sql table is included, and has a lot of Tasmota fields, but you can add more as needed.

Use --help to explain options, the simplest is to list topics of devices to be configured.
