//included libraries
#include "avr/pgmspace.h"
#include "Wire.h"
#include "Thread.h"
#include "ThreadController.h"

//settings
#define serialrate        115200
#define serialmessagesize 200

//version
#define product           500
#define serialnumber      1004
#define hardware		  1.13
#define firmware		  1.59

//eeprom
#define eeprom               0x50
#define eeprompage           64
#define configurationaddress 501
#define calibrationaddress   503
#define counteraddress       505

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

void setup(void) {

	Wire.begin();

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
      char   eepromdata [eeprompage + 2];
      data.toCharArray(eepromdata, eeprompage + 2);
      eepromwrite(eeprom, (control.toInt() - 1) * eeprompage, eepromdata);
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

			datawrite = " P " + String(product) + " H " + String(hardware) + " F " + String(firmware)+ " S " + String(serialnumber);
			for (int k = 0; k < 27 - datawrite.length(); k++) {
				dataequalizer += " ";
			}
			controllerstring += dataequalizer;
			controllerstring += datawrite;
			dataequalizer = "";

		}

		//Recipes
		if (header.substring(1, 2).equals("R")) {

			datawrite = readrecipe(control.toInt());

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

//eeprom read
void eepromread(int deviceaddress, unsigned int eeaddress, unsigned char* data, unsigned int num_chars){

  unsigned char i = 0;

  Wire.beginTransmission(deviceaddress);
  Wire.write((int)(eeaddress >> 8));
  Wire.write((int)(eeaddress & 0xFF));
  Wire.endTransmission();

  Wire.requestFrom(deviceaddress, num_chars);

  while(Wire.available()){
    data[i ++] = Wire.read();
  }

}

//eeprom write
void eepromwrite(int deviceaddress, unsigned int eeaddress, char* data){

  unsigned char i = 0;
  unsigned char counter = 0;
  unsigned int  address;
  unsigned int  page_space;
  unsigned int  page = 0;
  unsigned int  num_writes;
  unsigned int  data_len = 0;
  unsigned char first_write_size;
  unsigned char last_write_size;
  unsigned char write_size;

  do{
    data_len ++;
  }
  while(data[data_len]);

  page_space = int(((eeaddress / 64) + 1) * 64) - eeaddress;

  if (page_space>16){
    first_write_size = page_space - ((page_space / 16) * 16);

    if (first_write_size == 0){
      first_write_size = 16;
    }
  }
  else{
    first_write_size = page_space;
  }

  if (data_len > first_write_size){
    last_write_size = (data_len - first_write_size) % 16;
  }

  if (data_len > first_write_size){
    num_writes = ((data_len - first_write_size) / 16) + 2;
  }
  else{
    num_writes = 1;
  }

  i = 0;
  address = eeaddress;

  for(page = 0; page < num_writes; page ++){

    if(page == 0){
      write_size = first_write_size;
    }
    else if(page == (num_writes - 1)){
      write_size = last_write_size;
    }
    else{
      write_size = 16;
    }

    Wire.beginTransmission(deviceaddress);
    Wire.write((int)((address) >> 8));
    Wire.write((int)((address) & 0xFF));

    counter = 0;

    do{
      Wire.write((byte) data[i]);
      i ++;
      counter ++;
    }
    while((data[i]) && (counter < write_size));

    Wire.endTransmission();

    address += write_size;
    delay(10);
  }
}

//read recipe
String readrecipe(int controller){

	//------
	/*
	recip                abcdefghijklmnop   16
	resistancelower     +2000               05
	resistanceupper     +2000               05
	frequencylower      +30000              06
	frequencyupper      +30000              06
	sweepspeed          +100                04
	level               +5000               05
	select              +9                  02
	wav                 +255                04
	limit               +10000              06
	timeoutplayback	    +10				          03
	*/
	//------

	unsigned char rdata[32];
	char bytearray[eeprompage / 2];

	String eepromstring;

	for (int i = (controller - 1) * 2; i < controller * 2; i ++){
		eepromread(eeprom, i * (eeprompage / 2), rdata, eeprompage / 2);

		for (int k = 0; k < eeprompage / 2; k ++){
			bytearray[k] = rdata[k];
			eepromstring += bytearray[k];
		}

	}

	return eepromstring;
}

