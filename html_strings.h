



char* config_form = 
"<div>Connect me to your wifi</div>"
"<form action=\"http://192.168.4.1/setConfig?ssid=foo&password=bar\" method=\"GET\">"
"  <div>"
"    <label for=\"say\">SSID:</label>"
"    <input name=\"say\" id=\"say\" value=\"nothing\"/>"
"  </div>"
"  <div>"
"    <label for=\"to\">Password:</label>"
"    <input name=\"to\" id=\"to\" value=\"mom\"/>"
"  </div>"
"  <div>"
"    <button>Connect</button>"
"  </div>"
"</form>";


char* rebooting =
"<div>Thanks.</div>"
"<div>Rebooting with the wifi settings you specified.</div>"
"<div>Go ahead and connect to your wifi (the one you just connected me to). I'll see you there.</div>";