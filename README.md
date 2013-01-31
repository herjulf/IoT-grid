The IoT-grid project
====================

Introduction
-------------
Internet-Of-Things-grid (IoT-grid) is the communication and protocol support 
for power and energy management for Internet and IoT devices. Virtually 
all networked devices can collaborate.

### Key components:
* IoT-grid controller-units (IoTgc) 
* IoT-grid servers (IoTgs)

The concept has been developed at Royal Institute of Technology/KTH school
of ICT Stockholm Sweden (http://www.kth.se). Several student projects and 
courses has been contributing to the concept.  


The current hardware platform for the IoT-grid controller unit is based ARM 
MCU. The current software implementation is based the Contiki platform using 
IETF COAP protocol (draft) for grid communication. Of course other hardware
and software platforms are possible.

### For more info:
(http://ttaportal.org/menu/projects/microgrid/csd-2012-fall/) 

### License: Open Source. LGPL.


### Current software platform:
* Ubuntu/Linux
* Contiki OS


Development
-----------
This repository consist of simple scripts to facilitate building and cloning
the current projects to boost research, development, collaboration and
education.


* part1.sh Bash script building the ARM toolchain for the MCU.
* part2.sh Bash script building the programmer for the ARM MCU

