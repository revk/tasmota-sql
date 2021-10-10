# tasmota-sql

Tools for management of tasmota devices based on mysql

The mysql table tasmota has fields for tasmota settings. These can be added to as needed and hence allow for more and more settings as needed. The fields should have a mysql comment defined to explain the field in question.

The Topic field is the key. There are special cases for some fields, e.g. RuleN will send a RuleN 1 after setting the rule if it is not being set to "0". Field values that would be "OFF"/"ON" in SetOptionN are handled as "0"/"1".

A template sql table is included, and has a lot of Tasmota fields, but you can add more as needed.

Use --help to explain options, the simplest is to list topics of devices to be configured.
