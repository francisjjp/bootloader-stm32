#include "Bootloader.h"

// R/W buffer
char boot_buff[BOOT_BUFFER_SIZE];

/**
  * @brief  Close bootloader and start Application
  * @param  none
  * @retval true if OK,  otherwise return false
  */
uint8_t Boot_StartApplication(void)
{
    typedef void (*pFunction)(void);
    pFunction appEntry;
    uint32_t appStack;
    uint32_t app_check_address;
    uint32_t *app_check_address_ptr;

    app_check_address = BOOT_APPLICATION_ADDR;
    app_check_address_ptr = (uint32_t *) app_check_address;

    // Read the first location of application section
    // which contains the address of stack pointer
    // If it is 0xFFFFFFFF then the application section is empty
    if (*app_check_address_ptr == 0xFFFFFFFF) {
        return 0;
    }

    // Get the application stack pointer (First entry in the application vector table)
    appStack = (uint32_t) *((__IO uint32_t*)BOOT_APPLICATION_ADDR);

    // Get the application entry point (Second entry in the application vector table)
    appEntry = (pFunction) *(__IO uint32_t*) (BOOT_APPLICATION_ADDR + 4);

    // Reconfigure vector table offset register to match the application location
    SCB->VTOR = BOOT_APPLICATION_ADDR;

    // Set the application stack pointer
    __set_MSP(appStack);

    // Start the application
    appEntry();

    return 1; // OK (but in real app should never reach this point)
}


/**
  * @brief  Check connection to server
  * @param  ssource - SOCKET_SRC_WIFI / SOCKET_SRC_GPRS
  * @retval Return true if connection OK
  */
uint8_t Boot_CheckConnection(SOCKETS_SOURCE ssource)
{
	char* p;
    uint32_t len, len_left;

    // Check connection available
    sprintf(boot_buff, "GET / HTTP/1.1\r\nHost: %s\r\n\r\n", HTTP_SERVER_IP);
    Socket_Clear(ssource);
    Socket_Write(ssource, boot_buff, strlen(boot_buff));
    // Read answer
    Socket_ClearTimeout(ssource);
    p = boot_buff;
    len_left = BOOT_BUFFER_SIZE;
    while (!Socket_GetTimeout(ssource)) {
        len = Socket_Read(ssource, p, len_left);
        if (len) {
        	p += len;
        	len_left -= len;
        }
    }
    if ((BOOT_BUFFER_SIZE - len_left) < 10) {
    	return 0; // Err
    }
    return 1; // Ok
}

/**
  * @brief  Check for new application firmware available on remote FTP server
  * 		If No - Exit
  * 		If Yes- Download New firmware in BankC. Erase memory (BankA).
  * 		Flash (move) new firmware From BankC to BankA and verify it
  * @param  none
  * @retval Return BOOT_OK on success or BOOT_ERR_xxx on error
  */
BOOT_ERRORS Boot_PerformFirmwareUpdate(void)
{
    char* p;
    uint32_t len, i;
    uint32_t total_len, crc32;
    uint32_t fl_addr = FlashNVM_GetBankStartAddress(FLASH_BANK_COPY);
    uint32_t app_addr;
    unsigned int fw_len;
    SOCKETS_SOURCE ssource;

    // Init sources
    Socket_Init(SOCKET_SRC_WIFI);
    Socket_Init(SOCKET_SRC_GPRS);

    // Check connection available first
    // and select source
    // WiFi first
/*
    if (Boot_CheckConnection(SOCKET_SRC_WIFI)) {
    	ssource = SOCKET_SRC_WIFI;
    } else {
    	if ((Socket_Connect(SOCKET_SRC_GPRS) == SOCKET_OK) && Boot_CheckConnection(SOCKET_SRC_GPRS)) {
        	ssource = SOCKET_SRC_GPRS;
        } else {
        	//No connections!
        	return BOOT_ERR_CONNECTION; //Err
        }
    }

*/
	if (Socket_Connect(SOCKET_SRC_GPRS) == SOCKET_OK) {
    	ssource = SOCKET_SRC_GPRS;
    } else {
    	//No connections!
    	return BOOT_ERR_CONNECTION; //Err
    }


    // Clear buffer flash first
    FlashNVM_EraseBank(FLASH_BANK_COPY);

    // Get data
    sprintf(boot_buff, "GET /%s HTTP/1.1\r\nHost: %s\r\n\r\n", HTTP_SERVER_FW_FILENAME, HTTP_SERVER_IP);
    Socket_Clear(ssource);
    Socket_Write(ssource, boot_buff, strlen(boot_buff));
    // Read answer
    Socket_ClearTimeout(ssource);
    total_len = 0;
    while (!Socket_GetTimeout(ssource)) {
        len = Socket_Read(ssource, boot_buff, BOOT_BUFFER_SIZE);
        if (len) {
        	FlashNVM_Write(fl_addr, (uint8_t*)boot_buff, len);
        	total_len += len;
        	fl_addr += len;
        	Socket_ClearTimeout(ssource);
        }
    }
    if (total_len < 10) {
    	//No file on server!
    	return BOOT_ERR_CONNECTION; //Err
    }

    // Stop HTTP session
    Socket_Close(ssource);

    // NVM flash operation

	// Find firmware length
    fl_addr = FlashNVM_GetBankStartAddress(FLASH_BANK_COPY);
	len = total_len - 24;
    for (i = 0; i < len; i++)
    {
    	FlashNVM_Read(fl_addr + i, (uint8_t*)boot_buff, 24);
    	boot_buff[15] = '\0';
    	if (strstr(boot_buff, "Content-Length:")) {
        	sscanf(boot_buff + 15 + 1, "%ui", &fw_len);
        	fw_len -= 4; //dec CRC
        	break;
    	}
    }
    if (i == len) {
    	//no data length found
    	return BOOT_ERR_NODATA; //Err
    }

	// Find firmware start position
    fl_addr = FlashNVM_GetBankStartAddress(FLASH_BANK_COPY);
    len = total_len - 4;
    for (i = 0; i < len; i++)
    {
    	FlashNVM_Read(fl_addr + i, (uint8_t*)boot_buff, 4);
    	boot_buff[4] = '\0';
    	p = strstr(boot_buff, "\r\n\r\n");
    	if (p) {
    		fl_addr += (i + 4);
    		break;
    	}
    }
    if (i == len) {
    	//no file found
    	return BOOT_ERR_NODATA; //Err
    }

    // Count firmware CRC (last 4 byte of the firmware will be - CRC32 checksum)
    crc32_Clear();
	len = 0;
    for (i = 0; i < fw_len; i++)
    {
    	FlashNVM_Read(fl_addr + i, (uint8_t*)boot_buff, 1);
    	crc32_Add(boot_buff[0]);
    }
    crc32 = crc32_Value();
    FlashNVM_Read(fl_addr + i, (uint8_t*)boot_buff, 4);
    // And check it
    if (!memcmp(boot_buff, &crc32, 4)) {
    	//Firmware CRC error!
    	return BOOT_ERR_CRC; //Err
    }


    // Check is NEW firmware update available
    FlashNVM_Read(fl_addr + APP_VER_ADDR_LOW, (uint8_t*)&len, 2);
    FlashNVM_Read(FlashNVM_GetBankStartAddress(FLASH_BANK_APPLICATION) + APP_VER_ADDR_LOW, (uint8_t*)&i, 2);
    i   &= 0xFFFF;
    len &= 0xFFFF;
    if (i != 0xFFFF)
    if (i >= len) {
    	//"No new firmware available!");
    	return BOOT_OK;
    }

    // Update firmware (copy buffer to Application flash memory)
    FlashNVM_EraseBank(FLASH_BANK_APPLICATION);
    app_addr = FlashNVM_GetBankStartAddress(FLASH_BANK_APPLICATION);
    for (i = 0; i < fw_len; i += 128)
    {
    	FlashNVM_Read(fl_addr + i, (uint8_t*)boot_buff, BOOT_BUFFER_SIZE);
    	FlashNVM_Write(app_addr + i, (uint8_t*)boot_buff, BOOT_BUFFER_SIZE);
    }
    // Compare memories again?

    return BOOT_OK;
}


/**
  * @brief  Reboot device via WDT or special command
  * @param  none
  * @retval none
  */
void Boot_RebootMCU(void)
{
    // Reset MCU
    NVIC_SystemReset();
}

