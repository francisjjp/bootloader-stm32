# bootloader-stm32
The repository contains the code of the bootloader for Sinapse Devices based on stm32 chips

# Introduction
This project covers the development and testing of the generic bootloader for Sinapse Devices based on several chips of the STM32 family. 
The Readme of the project works as a technical requirements document and like a contract in case of subcontracting parts of the project.
<hr> 

# Glossary

* Bootloader : Little and stable program that is executed in each microcontroller start and after finish continue with the execution of the main program

* Main program : Business application executed by a Sinapse Device. Each Sinapse Device has its own main program but the bootloader is the same fo each device

* Sinapse Device : HW Device designed & manufactured by Sinapse Energía equipped with a microcontroller from the STM32 family

* Configuration File (CF): Header of the Bootloader where are defined the particularities of *each* bootloader. The Bootloader is generic but in this file are defined the partcularities of each device. For example, the folder (in ftp server) that contains the upgraded versions, the gprs configuration or the limitations of a particular micro, like the maximum available memory.   
<hr>

# Development environment and needed tools

* ATOLLIC TrueSTUDIO for ARM: Complete IDE to development firmware under C/C++ language and over a lot of microcontrollers, generic ARM and STM32Fxx devices. 

* STM32CubeMX: to generate all HAL drivers needed and integrate into ATOLLIC TrueSTUDIO.

* STLink Utility: to program HEX files into STM32Fxx devices.

* GNU Tools ARM Embedded : GCC compiler and more tools.

* Sinapse Devices: Devices running with STM32Fxx processor where BOOTLOADER firmware will be loaded. They are also the devices under test (DUT): Basically devices with STM32F030CC, STM32F405VG  processor and STM32F030K6T6 processor.

<hr>

# Global view

The generic bootloader for Sinapse Devices it is a little program that will be executed at the start / restart of any Sinapse device and will check if there is a new version of the Main program in a FTP server. 
If there is a new version, the bootloader should download the new main program, check its validity, check if there is enough space in the flash memory to install the program and then remove the current main program and install the new one and perform the execution handover to the new main program.
If there is not a new version, the bootloader should realise the execution handover to the main program.

For more information about a global view of how a generic bootloader should works, it is possible to consult the following url: http://blog.atollic.com/how-to-develop-and-debug-bootloader-application-systems-on-arm-cortex-m-devices
<hr>

# Hardware description

- Microprocessors: STM32F030CC, STM32F405VG & STM32F030K6T6 
- GPRS Peripheral: Quectel M95. More information in docs/
- WIFI/ETH Peripheral: USR-WIFI232-D2. More information in docs/

# FTP Client
A basic FTP client over GPRS and WIFI/ETH should be developed. 

- GPRS: The peripheral used includes native support for FTP. It is only necessary to encapsulate the AT commands in some C functions in order to connect / download / disconnect from FTP server

- ETH / WIFI: The peripheral used for WIFI / ETH connection does not have native FTP support but provides a transparent transmission TCP protocol mode over UART. In this case is necessary to create the FTP packages and send it directly to the UART where USR device is connected, the answers will be received also over the UART.

# Flow diagram and use case diagram

The workflow of the bootloader should be the following one:

![flowchart](https://github.com/Sinapse-Energia/bootloader-stm32/blob/master/Bootloader_flowchart_2.png)


# Technical description
Here are explained the technical requirements that should be covered by the Sinapse Generic Bootloader. The technical requirements aims to be almost unitary and testable. Each technical requirement should be tested as OK in order to give it as a valid

The requirements are divided by flow diagram elements

## Process - Start

1. The Bootloader should be executed after each start / restart of the Sinapse Device before the main program
2. The Bootloader should take maximum 60 seconds if there is or not a new FW to be downloaded in all posible interfaces.
3. The Bootloader should take maximum 10 minutes for download some new update for application program.
4. The Bootloader should start in the memory address 0x08000000 and should occupy maximum 10KB of flash memory. Anyway all reserved memory for Bootloader code goes from 0x08000000 to 0x08002800
5. The Bootloader will be installed during the fabrication process and will not be updated during the device longlife. This fact could change in future versions but is out of the scope.

## Decision - ETH / WIFI enabled 

1. Determine if exists one ETH/WIFI device connected over Sinapse device or if it is enabled.
2. If device exists, jump to "Process - Check FTP folder connection through ETH / WIFI".
3. If device does not exist, jump to "Decision - GPRS enabled". 

## Process - Check FTP folder connection through ETH / WIFI 

1. To peform all needed operations to connect with one prefixed FTP server, available in the CF.
2. Return ETHWIFI_FTP_FOLDER_FOUND if ETH/WIFI has been able to connect with the prefixed FTP server and it has found the folder (this folder is prefixed over Sinapse device) return ERROR_ETHWIFI_NOT_FOUND , ERROR_ETHWIFI_DISABLED , ERROR_ETHWIFI_FTP_NOT_CONNECT, ERROR_ETHWIFI_FOLDER_NOT_FOUND in other cases.

Note:
Each device should check 2 FTP folders. 
1. Global FW folder: All devices check the same folder. Will be used on production process to upload a general FW to all devices
2. Device Folder: Each device has an unique Folder for device Update. FW mnay times contains device info, so the update have to be customizaed for device.

So Bootloader program have to contain Device ID.
Folders routes have to be generical and easily modificable.
Will be setup as described below:

ftp root/DEVICE_CODE_FW/

ftp root/DEVICE_CODE_FW/DEVICE_ID/

## Decision - FTP folder connection through ETH or WIFI

1. If answer in previous process was ETHWIFI_FTP_FOLDER_FOUND, we jump to "Process - Check availability of new FW". 
2. If answer was different we jump to "Decision - GPRS Enabled"

## Decision - GPRS enabled

1. Determine if exists one GPRS device connected on Sinapse device or if it is enabled.
2. If device exists, jump to "Process - Check FTP folder connection through GPRS"
3. If device does not exist, jump to "Process - Execution handover to main program"

## Process - Check FTP folder connection through GPRS

 1. To peform all needed operations to connect with one prefixed FTP server, available in the CF.
 2. Return GPRS_FTP_FOLDER_FOUND if GPRS has been able to connect with the prefixed FTP server and it has found the folder (this  folder is prefixed over Sinapse device) return ERROR_GPRS_NOT_FOUND , ERROR_GPRS_DISABLED , ERROR_GPRS_FTP_NOT_CONNECT, ERROR_GPRS_FOLDER_NOT_FOUND in other cases.

## Decision - FTP folder connection through GPRS

1. If answer in previous process was GPRS_FTP_FOLDER_FOUND, we jump to "Process - Check availability of new FW". 
2. If answer was different, jump to "Process - Execution handover to main program"

## Process - Check availability of new FW

1. Inside the folder there will be one .BIN file. The name describes the version of the FW, so it has to be: FW_NAME_VXpY_ddmmyy, where:

FW_NAME: No limitations, end with_

VxpY: FW version i.e. V1p4 = V1.4

ddmmyy: Release Date

3. Check if VxpY and ddmmyy are higher that actual stored version and date

*Note
The actual stored version and date are saved in the first positions of the first page of the flash memory. The bootloader from fabric has these values set to: "last_update = date_installation" and "version = Vp, for example 14 -> V1p4"

4. If yes, jump to "Process - Download new FW", returning NEW_AVAILABLE_FIRMWARE
5. If answer was different we jump to "Process - Execution handover to main program", returning NO_NEW_FIRMWARE

## Decision - New FW available

1. If answer in previous process was NEW_AVAILABLE_FIRMWARE, jump to Download new FW


3. Check if VxpY and ddmmyy are higher that actual stored version and date
4. If yes, jump to "Process - Download new FW".
2. If answer was different we jump to "Process - Execution handover to main program"

## Process - Download new FW

1. If file is downloaded, it must to be saved  beginning in position 0x08002800+MAX_SIZE_APPLICATION_PROGRAM. ( We have then three sections of different programs in Flash):

1) From 0x08000000 - 0x80027FF BOOTLOADER
2) From 0x08002800 - (0x8002800+MAX_SIZE_APPLICATION_PROGRAM-1) APPLICATION PROGRAM
3) From (0x8002800+MAX_SIZE_APPLICATION_PROGRAM) - (0x8002800+2 *(MAX_SIZE_APPLICATION_PROGRAM)-1) UPGRADE PROGRAM
 
 *IMPORTANT NOTE* It is supposed that three sections are completely independent into FLASH. It means that one erase operation in one section DO NOT erase another sector. It must been verified in three microcontrollers: STM32F030CC, STM32F405VG  STM32F030K6T6 processor.
 If not possible three sections must be separate minimal enough space to became in different sectors.
 
## Decision - Download correct

1. The .HEX files will have in last 4 bytes of file one CRC32 of all previous data
2. The process of download of UPGRADE PROGRAM in section 3 is correct if the calculate CRC32 from position:
- 0x8002800+MAX_SIZE_APPLICATION_PROGRAM to (0x8002800+2 * (MAX_SIZE_APPLICATION_PROGRAM)-5) matchs with last 4 bytes of UPGRADE PROGRAM
3. Return DOWNLOAD_CORRECT if calculated CRC32 matchs with CRC32 of 4-last-bytes saved in UPGRADE PROGRAM and jump to "Process - Stop previous FW and install the new one". 
4. If download is incorrect, it will return DOWNLOAD_INCORRECT and jump to "Process - Execution handover to main program".

## Process - Stop previous FW and install the new one

1. If download was correct, then we ERASE all flash memory from section 2) APPLICATION PROGRAM and save all data from section 3) UPGRADE PROGRAM to section 2) APPLICATION PROGRAM.
2) Section 3 is erased completely.
3) It is needed to update the flash memory with the last_update and version variables. These information is used to decide if a new version should be installed.
3) Return PROCESS_OK
3) Jump to "Process - Execution Handover to main program".

## Process - Execution Handover to main program

1. A new CRC32 calculation must be done over section 2) APPLICATION PROGRAM and the CRC must matchs with 4-last-bytes saved in APPLICATION PROGRAM.
2. If CRC32 matchs with 4-last-bytes then we load APPLICATION IRQ VECTORS on MAIN VECTORS and load program counter of microcontroller  with first entry for APPLICATION PROGRAM. (We launch application program)
3. If CRC32 DOES NOT match with 4-last-bytes, then the APPLICATION PROGRAM is corrupt, we must continue in BOOTLOADER mode, jump to " Decision - ETH / WIFI enabled"
4. If any new firmware was downloaded, the bootloader should perform the handover to the main program without the CRC32 calculations

## Process - END

1. The bootloader finish when the "Process - Execution Handover to main program" is correctly done
2. If the bootloader was not correctly done, then try again until a main program is correctly installed

## Some interesting points to define:

1) Is good idea to inform FTP server in some file status of all process? - > RAE : I think YES, TODO
2) Is good idea to do retries?. In this flowchart there is not specification of some retrie if something goes wrong. -> ALREADY Explained


# Testing

## Unit tests - Designed and developed by the Developers
Each function should contains a unit test verifying the correct behavior of each use case

## Integration tests - Designed and developed by Sinapse 
Several integration tests should be performed in order to verify the correct behaviour of the Bootloader in the most common situations. These tests should be executed in ALL the available Sinapse Devices

### Sinapse Device Empty - ETH/WIFI Connection - LED Main Program OK & LED Main Program KO


# Test 1 : Ideal situation

- Device start
- Bootloader connect to HTTP server (over WiFi)
- Bootloader download the code of the client App
- Bootloader check the client App and is correct
- Bootloader install the client App
- Bootloader perform handover to the client App

- Result: The LED blinks each second

# Test 2 : The connection is before the program is downloaded

- Device start
- Bootloader connect to HTTP server (over WiFi)
- Bootloader download the code of the client App
- The connection is lost (External)
- Bootloader check the client App and is NOT correct
- Bootloader does not install the client App
- Device does not have any client App installed
- Bootloader is not able to perform handover
- Bootloader start again and this time the connection is OK. It should work as is explained in test 1. 

- Result: The LED blinks each second

# Test 3 : The device is restarted before the application is installed

- Device start
- Bootloader connect to HTTP server (over WiFi)
- Bootloader download the code of the client App
- Bootloader check the client App and is correct
- Bootloader start to install but the device is restarted
- Bootloader start again and this time the whole process is OK. It should work as is explained in test 1.

- Result: The LED blinks each second

# Test 4 : The device is restarted after the application is installed but the handover is not done (if possible)

- Device start
- Bootloader connect to HTTP server (over WiFi)
- Bootloader download the code of the client App
- Bootloader check the client App and is correct
- Bootloader install the client App
- Device restart
- Bootloader connect to HTTP server (over WiFi)
- Bootloader check the version but the installed version is the last one
- Bootloader does not download the program
- Bootloader perform the handover to the client App

- Result: The LED blinks each second



### Sinapse Device Empty - GPRS Connection - LED Main Program OK & LED Main Program KO

# Test 5 : Ideal situation

- Device start
- Bootloader connect to HTTP server (over GPRS)
- Bootloader download the code of the client App
- Bootloader check the client App and is correct
- Bootloader install the client App
- Bootloader perform handover to the client App

- Result: The LED blinks each second

# Test 6 : The connection is before the program is downloaded

- Device start
- Bootloader connect to HTTP server (over GPRS)
- Bootloader download the code of the client App
- The connection is lost (External)
- Bootloader check the client App and is NOT correct
- Bootloader does not install the client App
- Device does not have any client App installed
- Bootloader is not able to perform handover
- Bootloader start again and this time the connection is OK. It should work as is explained in test 5. 

- Result: The LED blinks each second

# Test 7 : The device is restarted before the application is installed

- Device start
- Bootloader connect to HTTP server (over GPRS)
- Bootloader download the code of the client App
- Bootloader check the client App and is correct
- Bootloader start to install but the device is restarted
- Bootloader start again and this time the whole process is OK. It should work as is explained in test 5.

- Result: The LED blinks each second

# Test 8 : The device is restarted after the application is installed but the handover is not done (if possible)

- Device start
- Bootloader connect to HTTP server (over GPRS)
- Bootloader download the code of the client App
- Bootloader check the client App and is correct
- Bootloader install the client App
- Device restart
- Bootloader connect to HTTP server (over GPRS)
- Bootloader check the version but the installed version is the last one
- Bootloader does not download the program
- Bootloader perform the handover to the client App

### Sinapse Device with Main Program - ETH/WIFI Connection - New Main Program OK & New Main Program KO
TODO

### Sinapse Device with Main Program - GPRS Connection - New Main Program OK & New Main Program KO
TEST 5. IDEAL SITUATION:

It was tested that if there is not any firmware file in remote HTTP server, the position at starting in 0x8008000 in flash is a respone HTTP message from server ("Page not found" http message) and not any application program. (CRC is checked???). 

remote HTTP server was: sinapseenergia.com:80.
remote file is: CMC.bin.


# Validation and closing

To validate and close the project it is necessary to provide the following documentation and results, as well as the code:

1. Documentation of structure and behaviour of the code, including interaction diagrams
2. Unit tests and its results
3. Results of the integration tests, all of then should be passed
