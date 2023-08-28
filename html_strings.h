

char* control_form_hdr =
"<div><span style=\"font-size:72px\">Activate Feeder</span></div>";

char* control_with_errors_form_hdr = 
"<div><span style=\"color:Red;font-size:60px\">---Bad Input: Digits only, and no more than 4 digits (meaning maximun duration is 9999 seconds)---</span></div>"
"<div><span style=\"font-size:72px\">Activate Feeder</span></div>";

char* control_currently_running_form_hdr = 
"<div><span style=\"color:Red;font-size:60px\">---Feeder is currently running. Wait til it's done (or click stop)---</span></div>"
"<div><span style=\"font-size:72px\">Activate Feeder</span></div>";

char* rebooting_html =
"<span style=\"font-size:60px\"><div>Thanks.</div>"
"<div>Rebooting with the wifi settings you specified.</div>"
"<div>Go ahead and connect to your wifi (the one you just connected me to). I'll see you there.</div></span>";

char* config_form = 
"<div><span style=\"font-size:72px\">Connect me to your wifi</span></div>"
"<form action=\"http://192.168.4.1/setConfig?ssid=foo&password=bar\" method=\"GET\">"
"<span style=\"font-size:60px\">"
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
"</span>"
"</form>";

char* config_with_errors_form = 
"<div><span style=\"color:Red;font-size:60px\">---ERROR: max length of SSID is 32 characters, and max length of Password is 63 characters---</span></div>"
"<div><span style=\"font-size:72px\">Connect me to your wifi</span></div>"
"<form action=\"http://192.168.4.1/setConfig\" method=\"GET\">"
"<span style=\"font-size:60px\">"
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
"</span>"
"</form>";
