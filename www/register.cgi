#!../login/logincheck --query /bin/csh -f
setenv USERNAME `weblink --check`
if($status) then
	setenv MSG "The link you have used is not valid or has expired, sorry"
	source login.cgi
	exit 0
endif
setenv C `sql -c "$DB" 'INSERT IGNORE INTO user SET email="$USERNAME"'`
dologin --force --redirect
