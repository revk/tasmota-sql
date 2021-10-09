# tasmota-sql

Tools for management of tasmota devices based on mysql

The mysql table tasmota has fields for tasmota settings. These can be added to as needed and hence allow for more and more settings as needed. The fields should have a mysql comment defined to explain the field in question.

- Topic:	The key field for the device, the given primary topic.
- Rule/N/:	If not set to "0" then a Rule/N/ 1 will be sent to enable the rule.
