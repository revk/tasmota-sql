# tasmota-sql

Tools for management of tasmota devices based on mysql

The mysql table tasmota has fields for tasmota settings. These can be added to as needed and hence allow for more and more settings as needed. The fields should have a mysql comment defined to explain the field in question.

- Topic:	The key field for the device, the given primary topic.
- RuleN:	If not set to "0" then a "RuleN 1" will be sent to enable the rule.

Use --help to explain options, the simplest is to list topics of devices to be configured.
