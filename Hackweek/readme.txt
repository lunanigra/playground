MEDI-CALL README

- Copy content of libraries folder into your libraries folder located below your Arduino folder.
- External libs are used for CC3000 WiFi chip and NeoPixel Flora RGB LED.
- MediCall contains the actual Arduino sketch.


The MediCall sketch connects to the configured WiFi network. Whenever a pill box is opened a request will be send to the server. See asana docs for details...
https://app.asana.com/0/33761664954774/34523059861543

To keep the connection open or to recover from connection failures a keep alive request is send every minutes.

Each pill box has its own micro switch. The box detection follows the approach of an analog reading with different resistors on each box. Details can be found on the following page...
http://www.instructables.com/id/How-to-access-5-buttons-through-1-Arduino-input/?ALLSTEPS​
