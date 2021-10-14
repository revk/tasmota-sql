#!../login/loggedin --query /bin/csh -f
xmlsql -C -d "$DB" head.html - foot.html << 'END'
<p>Tasmota config</p>
'END'
