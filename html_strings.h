

char* welcome_form =
"<div><span style=\"font-size:30px\">I'm connected!</span></div>";

char* config_form = 
"<div><span style=\"font-size:30px\">Connect me to your wifi</span></div>"
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

char* config_with_errors_form = 
"<div><span style=\"color:Red;font-size:30px\">---ERROR: max length of SSID is 32 characters, and max length of Password is 63 characters---</span></div>"
"<div><span style=\"font-size:30px\">Connect me to your wifi</span></div>"
"<form action=\"http://192.168.4.1/setConfig\" method=\"GET\">"
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