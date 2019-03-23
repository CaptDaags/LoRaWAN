# LoRaWAN
Scripts and information relating to LoRaWAN. Gateway and also sensor nodes.

Generally this is for my use but feel free to look around and use what you can as long as you abide by fair play and the license. I will indicate in the code if I've pilfered it from somebody else.

If you blow shit up with what you find here then all the more fool ya are. I profess no expertise here so use at your own peril.

Added Fritzing to the repo as well.

Current configuration that this covers:

Gateway: Raspberry Pi 3B+ with RHF shield on AU 915 frequency plan.

Node: Arduino Uno with Draguino Lora shield on AU 915 frequency plan.

Libraries: Some things to watch for. The traditional IBM LMIC libraries tend to be a little fat for the Arduino
Uno. They are good for most things and most of the code here needs this particular library unless indicated.
However, for the temp sensor to TTN sketch which is being worked on at the moment needs the slimmed down LMIC
library from the TTN gurus to fit in the Uno program memory space. But I am finding some issues with this
library at the moment when using with the 915Mhz AU radios. Stay tuned.
