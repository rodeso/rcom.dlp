# RC11
Data Link Protocol

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)


- **Project name:** RC11 - Data Link Protocol
- **Short description:** File Transfer Between Two Computers with Serial Cable
- **Environment:** Linux
- **Tools:** C
- **Institution:** [FEUP](https://sigarra.up.pt/feup/en/web_page.Inicial)
- **Course:** [RC](https://sigarra.up.pt/feup/en/UCURR_GERAL.FICHA_UC_VIEW?pv_ocorrencia_id=541890) (Computer Networks)
- **Project grade:** 18
- **Group members:**
    - Pedro Santos (up202205900@fe.up.pt)
    - Rodrigo de Sousa (up202205751@fe.up.pt)

---



INSTRUCTIONS FOR SERIAL PORT PROTOCOL
=====================================

This folder contains the base code of the serial port protocol.

Project Structure
-----------------

- bin/: Compiled binaries.
- src/: Source code for the implementation of the link-layer and application layer protocols. Students should edit these files to implement the project.
- include/: Header files of the link-layer and application layer protocols. These files must not be changed.
- cable/: Virtual cable program to help test the serial port. This file must not be changed.
- main.c: Main file. This file must not be changed.
- Makefile: Makefile to build the project and run the application.
- penguin.gif: Example file to be sent through the serial port.

Instructions to Run the Project
-------------------------------

1. Edit the source code in the src/ directory.
2. Compile the application and the virtual cable program using the provided Makefile.
3. Run the virtual cable program (either by running the executable manually or using the Makefile target):
	$ sudo ./bin/cable_app
	$ sudo make run_cable

4. Test the protocol without cable disconnections and noise
	4.1 Run the receiver (either by running the executable manually or using the Makefile target):
		$ ./bin/main /dev/ttyS11 9600 rx penguin-received.gif
		$ make run_tx

	4.2 Run the transmitter (either by running the executable manually or using the Makefile target):
		$ ./bin/main /dev/ttyS10 9600 tx penguin.gif
		$ make run_rx

	4.3 Check if the file received matches the file sent, using the diff Linux command or using the Makefile target:
		$ diff -s penguin.gif penguin-received.gif
		$ make check_files

5. Test the protocol with cable disconnections and noise
	5.1. Run receiver and transmitter again
	5.2. Quickly move to the cable program console and press 0 for unplugging the cable, 2 to add noise, and 1 to normal
	5.3. Check if the file received matches the file sent, even with cable disconnections or with noise
