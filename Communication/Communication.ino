//included libraries
#include "avr/pgmspace.h"
#include "Thread.h"
#include "ThreadController.h"

//settings
#define serialrate        115200
#define serialmessagesize 200

//version
#define hardware		  1.13
#define firmware		  1.59

//setings  
  //threads controllers
	ThreadController controller = ThreadController();
	Thread controllercommunication = Thread();

	ThreadController communicationusb = ThreadController();
	Thread communicationusbread = Thread();

//system
	String serialmessageusb;
	int transfermode = 0;
	int glcdcontrolstep = 0;

	String recipestring[2];

void setup(void) {

	Serial.begin(serialrate);
	serialmessageusb.reserve(serialmessagesize);

	//threads  
		//controller
			//controller glcd
				controller.add(&controllercommunication);
				controllercommunication.onRun(communication);
				controllercommunication.setInterval(200);

	//communicate transfer
		//communication transfer mode
			communicationusb.add(&communicationusbread);
			communicationusbread.onRun(usbread);
			communicationusbread.setInterval(0);


	for (int i = 0; i < 2; i++) {
		recipestring[i] = F("                  130  130    20    20   1  200 0   1     1  1  ");
	}

}

void loop(void) {

	if (transfermode < 2) {
		controller.run();
	}
	else {
		communicationusb.run();
	}

}

void communication(void) {

	if (Serial.available() > 0) {
		serialmessageusb = Serial.readStringUntil('\n');
		Serial.flush();
		if (serialmessageusb.equals(F("WC00001"))) {
			transfermode = 1;
			Serial.print(F("@RC00001                                                              %092#"));
			Serial.print('\n');
		}
	}

	if (transfermode == 1) {
		glcdcontrolstep++;
	}

	if (glcdcontrolstep > 8) {
		glcdcontrolstep = 0;
		transfermode = 2;
	}
}

//usb read
void usbread(void) {

	//sttings
#define usbreadbuffer 75
#define usbreadmod 99
#define usbreadprefix "@"
#define usbreadlimiter "#"
#define usbreadtab "%"

//function
	if (Serial.available() > 0) {
		serialmessageusb = Serial.readStringUntil('\n');
		Serial.flush();

		if (serialmessageusb.substring(0, 1) == usbreadprefix && serialmessageusb.substring(usbreadbuffer - 1) == usbreadlimiter) {

			String serialdata = serialmessageusb.substring(1, serialmessageusb.indexOf(usbreadtab));
			String serialchecksum = serialmessageusb.substring(serialmessageusb.indexOf(usbreadtab) + 1, usbreadbuffer - 1);

			char dataarray[serialdata.length() + 1];

			serialdata.toCharArray(dataarray, serialdata.length() + 1);

			long datacalculations = 0;
			long datachecksum = 0;

			for (int i = 0; i < serialdata.length(); i++) {
				datacalculations = dataarray[i] * (i + 1);
				datachecksum += datacalculations;
			}
		
			datachecksum = datachecksum % usbreadmod;

			if (datachecksum == serialchecksum.toInt()) {
				usbdecoder(serialdata);
			}
			else {
				//checksum ERROR
				//usbwrite("ER0000000000");
			}

			serialmessageusb = "";
		}
	}
}

void usbdecoder(String decoder) {

#define headersize 2
#define controlsize 3
#define datasize 64

	String header = decoder.substring(0, headersize);
	String control = decoder.substring(headersize, headersize + controlsize);
	String data = decoder.substring(headersize + controlsize, headersize + controlsize + datasize);

	String controllerstring;
	String datawrite;
	String dataequalizer;

	if (header.substring(0, 1).equals("W")) {

		//Communication
		if (header.equals("WC")) {
			if (control.equals("000")) {
				transfermode = data.toInt();
			}
		}

		//Version
		if (header.equals("WV")) {
		}

		//Recipes
		if (header.equals("WR")) {
		}

	}

	if (header.substring(0, 1).equals("W") || header.substring(0, 1).equals("R")) {

		//Communication
		if (header.substring(1, 2).equals("C")) {

			if (control.equals("000")) {
				datawrite = String(transfermode);
				for (int k = 0; k < 2 - datawrite.length(); k++) {
					dataequalizer += "0";
				}
				controllerstring += dataequalizer;
				controllerstring += datawrite;
				dataequalizer = "";
			}

		}

		//Version
		if (header.substring(1, 2).equals("V")) {

			datawrite = " H " + String(hardware) + " F " + String(firmware);
			for (int k = 0; k < 14 - datawrite.length(); k++) {
				dataequalizer += " ";
			}
			controllerstring += dataequalizer;
			controllerstring += datawrite;
			dataequalizer = "";

		}

		//Recipes
		if (header.substring(1, 2).equals("R")) {

			datawrite = usbreadeeprom(control.toInt());

			for (int k = 0; k < 64 - datawrite.length(); k++) {
				dataequalizer += " ";
			}
			controllerstring += dataequalizer;
			controllerstring += datawrite;
			dataequalizer = "";

		}


		int bufferequalizer = datasize - controllerstring.length();

		for (int i = 0; i < bufferequalizer; i++) {
			controllerstring += " ";
		}

		usbwrite("R" + header.substring(1, 2) + control + controllerstring);
	
	}

}

void usbwrite(String serialdata) {
	//sttings
	#define usbwritebuffer 75
	#define usbwritemod 99
	#define usbwriteprefix "@"
	#define usbwritelimiter "#"
	#define usbwritetab "%"

//function
	char dataarray[serialdata.length() + 1];

	serialdata.toCharArray(dataarray, serialdata.length() + 1);

	long datacalculations = 0;
	long datachecksum = 0;
	
	for (int i = 0; i < serialdata.length(); i++) {
		datacalculations = dataarray[i] * (i + 1);
		datachecksum = datachecksum + datacalculations;
	}

	datachecksum = datachecksum % usbwritemod;

	String checksumsize = String(datachecksum);
	int dataequalizer = usbwritebuffer - 3 - serialdata.length() - checksumsize.length();
	String checksum;

	for (int i = 0; i < dataequalizer; i++) {
		checksum = checksum + "0";
	}

	checksum = checksum + checksumsize;

	Serial.print(usbwriteprefix + serialdata + usbwritetab + checksum + usbwritelimiter);
	Serial.print('\n');
	Serial.flush();
}

String usbreadeeprom(int controller) {

	String eepromstring = recipestring[0];
	
	return eepromstring;
}
