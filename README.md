# NixieClock
GRA &amp; AFCH Nixie Tubes Clock on NCM109 (NCM105 and NCM107 Discontinued Products)
1. This repository content sources of project for Nixie Clock developed by GRA & AFCH.

	Folders description:
  
	FIRMWARES - source code that must be complied in Arduino IDE, and allready compiled Binary files in *.hex format ready to be uploaded on to property board by flasher.
  
	GRA & AFCH Compiled Firmware Flasher - flasher (uploader) that must be used in prevois step.
  
	LIBRARIES - Arduino libraries without which compiling will be failed. That folders must be copied to Arduino LIBRARIES folder, (default path: C:\Users\USER_NAME\Documents\Arduino\libraries)
  
	SCHEMES - electrical shemes for boards: for Nixie Clock Main Units - MCU, and for Nixie Tubes Boards
  
	USB DRIVERS - drivers for USB-to-SERIAL(UART) converters
  
	USER MANUAL - end user's manuals.

2. Link to YouTube video with preparing, compiling and uploading firmware to clock:
https://youtu.be/DQZWPn0iAHw

3. Compatibility:</br>
        Board MCU NCM109 is compatible with tubes boards </br>
	* **NCT 3xx series**: NCT318 (6 Tubes IN-18), NCT818 (2 Tubes IN-18), NCT3566(6 Tubes Z5660, Z1042), NCT8566 (2 Tubes Z5660, ZM1042)
	* **NCT 4xx series**: NCT402(IN-2), NCT404(IN-4), NCT408(IN-8), NCT482(IN8-2), NCT412-4 Tubes (IN-12), NCT412-6 Tubes (IN-12), 
	NCT414-4 Tubes (IN-14), NCT414-6 Tubes (IN-14), NCT416 (IN-16), NCT417 (IN-17), NCT4573 (Z573/Z570)
	
	------------------- Discontinued Products ----------------------------------------------</br>
	NCM 107 is compatible with tubes boards: NCT 3xx, NCT 4xx series and NCT3566, NCT3573. (Discontinued Products)</br>
	
	NCM 105 is compatible with tubes boards: NCT 2xx series and NCT566, NCT573 . (Discontinued Products)</br>
	----------------------------------------- ----------------------------------------------</br>
	
	
	

3. eBay stores:
First (gra_and_afch) - https://www.ebay.com/sch/gra_and_afch/m.html?_nkw=&_armrs=1
Second (gra_and_afch_2) - http://stores.ebay.com/graandafch2/

4. Our own on-line store: http://gra-afch.com

5. E-mail: fominalec@gra-afch.com
