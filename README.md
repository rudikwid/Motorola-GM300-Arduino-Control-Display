# Motorola-GM300-Arduino-Control-Display

THE ORIGINAL HARD-WORKS THIS MOTOROLA BUSINESS RADIO MAXTRAC MAXDROID CONVERSIONS IS AVINOAM ALBO, 4X1HF, 

as detailed here: http://www.caarc.ca/articles/maxdroid#more-1714

Short Youtube demo is here: https://www.youtube.com/watch?v=R2JITDX9hZI

SINCE THIS ARDUINO SKETCH & DOCUMENTARIES IS IMPORTANT FOR MY MOTOROLA GM-300 CONVERSIONS, I am afraid this important informations is dissapear without any explanations, So i put here in Github.

YOU MAY DOWNLOAD ALL THIS INFORMATION FREE OF CHARGES, but would be grateful to give credits to Avinoam Albo, 4X1HF for his hard-works.

//**************************************************************************************************

Conversion of a Maxtrac to a transceiver with display and frequency selection

In this article I will explain how to convert the venerable Maxtrac to a rig more user friendly to radio amateurs.
The modification requires the addition of only a few inexpensive and easily available components.
When the modification is completed you will have a rig with digital display and frequency selection enabling continuous coverage over the whole range of frequencies  where the VCO locks. An external programmer is no longer needed.

Disclaimer: 
Carrying out the modification described requires basic electronics skills.
Any modifications are the sole responsibility of the technician. 
This modification will likely void type certification for commercial radio service.
This article does not make any specific claims or recommendations.

Specifications of the transceiver after this modification:
Digital display
Continuous frequency selection
Selection of transmitter output power (2 levels or more)parts required
Selection using VFO or memories
Selection of VFO frequency steps (5K, 10K, etc.)
Receive PL (on/off)
Scanning and saving frequencies to memory (future)

Required Components:
Maxtrac VHF or UHF
Arduino Nano 328 controller
Rotary encoder
LCD 08 X 02 (8 characters x 2 lines)
3K, 4.7K, 8.2K, 10K resistors
Fine gauge insulated hookup wire such as wire wrap wire

Programming the transceiver
After the modification all the functions continue to work (including programmed functions) with the exception of operating frequency, 
output power and enable / disable receive PL.

After reading the radio configuration with the help of programming software and cable,
enter the RADIO WIDE CONFIGURATION menu and in the OFF HOOK PL/DPL line mark N (no).

radio wide config screen
In the MODE CONFIGURATION menu it's recommended to erase all the channels (MODE) and to leave only one channel with receive and transmit PL of 91.5 Hz.
The transmit and receive frequencies are not important because they will be set by the new controller, and not the original Maxtrac circuitry.
