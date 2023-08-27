

char* control_form_hdr =
"<div><span style=\"font-size:30px\">Activate Feeder</span></div>";

char* control_with_errors_form_hdr = 
"<div><span style=\"color:Red;font-size:30px\">---ERROR: no more than 4 digits (meaning maximun duration is 9999 seconds)---</span></div>"
"<div><span style=\"font-size:30px\">Activate Feeder</span></div>";

char* control_form =
"  <div>"
"    <label for=\"duration\">How Long (seconds):</label>"
"    <input name=\"duration\" id=\"duration\" value=\"1\"/>"
"  </div>"
"  <div>"
"    <button>GO</button>"
"  </div>"
"</form>";

char* confirmed_html =
"<div>Thanks.</div>"
"<div>Rebooting with the wifi settings you specified.</div>"
"<div>Go ahead and connect to your wifi (the one you just connected me to). I'll see you there.</div>";

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


char* rebooting_html =
"<div>Thanks.</div>"
"<div>Rebooting with the wifi settings you specified.</div>"
"<div>Go ahead and connect to your wifi (the one you just connected me to). I'll see you there.</div>";