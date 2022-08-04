# Motorola-GM300-Arduino-Control-Display

THE ORIGINAL HARDWORKS THIS MOTOROLA MAXTRAC MAXDROID IS Avinoam Albo, 4X1HF, as detailed here:

http://www.caarc.ca/articles/maxdroid#more-1714

SINCE THIS ARDUINO SKETCH IS IMPORTANT TO MAKE IT WORKS TO IMPLEMENTED TO MOTOROLA GM-300, and i afraid the web pages will gone with no explanation for reasons,

So i put here in Github.

YOU MAY DOWNLOAD ALL THIS INFORMATION FREE OF CHARGES, but would be grateful to give credits to Avinoam Albo, 4X1HF for his hardworks.



Conversion of a Maxtrac to a transceiver with display and frequency selection

In this article I will explain how to convert the venerable Maxtrac to a rig more user friendly to radio amateurs.Â  The modification requires Â the addition of only a few inexpensive and easily available components.Â  When the modification is completed you will have a rig with digital display and frequency selection enabling continuous Â coverage over the whole range of frequencies Â where the VCO locks.Â  An external programmer is no longer needed.

Disclaimer:Â  Carrying out the modification described requires basic electronics skills.Â  Any modifications are the sole responsibility of the technician. This modification will likely void type certification for commercial radio service.Â Â  This article does not make any specific claims or recommendations.

Specifications of the transceiver after this modification

Digital display
Continuous frequency selection
Selection of transmitter output power (2 levels or more)parts required
Selection using VFO or memories
Selection of VFO frequency steps (5K, 10K, etc.)
Receive PL (on/off)
Scanning and saving frequencies to memory (future)
Required ComponentsÂ 

Maxtrac VHF or UHF
Arduino Nano 328 controller
Rotary encoder
LCD 08 X 02 (8 characters x 2 lines)
3K, 4.7K, 8.2K, 10K resistors
Fine gauge insulated hookup wire such as wire wrap wire
Programming the transceiver

After the modification all the functions continue to work (including programmed functions) with the exception of operating frequency, output power and enable / disable receive PL.

After reading the radio configuration with the help of programming software and cable,

enter the â€œRADIO WIDE CONFIGURATION menu and in the OFF HOOK PL/DPL line mark N (no).

radio wide config screen

In the MODE CONFIGURATION menu itâ€™s recommended to erase all the channels (MODE) and to leave only one channel with receive and transmit PL of 91.5 Hz.Â  The transmit and receive frequencies are not important because they will be set by the new controller, and not the original Maxtrac circuitry.
