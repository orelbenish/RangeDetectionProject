/*
 * COPYRIGHT (C) STMicroelectronics 2015. All rights reserved.
 *
 * This software is the confidential and proprietary information of
 * STMicroelectronics ("Confidential Information").  You shall not
 * disclose such Confidential Information and shall use it only in
 * accordance with the terms of the license agreement you entered into
 * with STMicroelectronics
 *
 * Programming Golden Rule: Keep it Simple!
 *
 */

/*!
 * \file   VL53L0X_platform.c
 * \brief  Code function defintions for Doppler Testchip Platform Layer
 *
 */

#include <stdio.h>    // sprintf(), vsnprintf(), printf()

#ifdef _MSC_VER
#define snprintf _snprintf
#endif

#include "vl53l0x_i2c_platform.h"
#include "vl53l0x_def.h"

/* Driver Header files */
#include <ti/drivers/GPIO.h>
#include <ti/drivers/I2C.h>
#include <ti/display/Display.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/BIOS.h>


/* Example/Board Header files */
#include "Board.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "vl53l0x_api.h"
#include "vl53l0x_platform.h"
#include "required_version.h"

//#include "SERIAL_COMMS.h"
//#include "comms_platform.h"

#include "vl53l0x_platform_log.h"

#ifdef VL53L0X_LOG_ENABLE
#define trace_print(level, ...) trace_print_module_function(TRACE_MODULE_PLATFORM, level, TRACE_FUNCTION_NONE, ##__VA_ARGS__)
#define trace_i2c(...) trace_print_module_function(TRACE_MODULE_NONE, TRACE_LEVEL_NONE, TRACE_FUNCTION_I2C, ##__VA_ARGS__)
#endif

char  debug_string[VL53L0X_MAX_STRING_LENGTH_PLT];

uint8_t cached_page = 0;



#define MIN_COMMS_VERSION_MAJOR     1
#define MIN_COMMS_VERSION_MINOR     8
#define MIN_COMMS_VERSION_BUILD     1
#define MIN_COMMS_VERSION_REVISION  0


#define MAX_STR_SIZE 255
#define MAX_MSG_SIZE 100
#define MAX_DEVICES 4
#define STATUS_OK              0x00
#define STATUS_FAIL            0x01


/*Global Variables*/
I2C_Handle      i2c;
I2C_Params      i2cParams;
I2C_Transaction i2cTransaction;
//Display_Handle display;
Semaphore_Struct semStruct;
Semaphore_Handle semHandle;


bool_t _check_min_version(void)
{
    return true;
}

void I2CCallbackFunction(I2C_Handle i2c, I2C_Transaction *i2cTransaction, bool result) {

 if (result) {

  // transfer completed successfully

  // Data is stored in i2cTransaction


  // Notify task that transfer is complete here (semaphore or event)

     Semaphore_post(semHandle);

 } else {

      // error occurred
   //  Display_printf(display, 0, 0, "error occurred\n");


 }

}

int32_t VL53L0X_i2c_init()
{
    unsigned int status = STATUS_FAIL;

    I2C_init();
    I2C_Params_init(&i2cParams);
    i2cParams.bitRate = I2C_400kHz;
    //i2cParams.transferMode = I2C_MODE_CALLBACK;
    //i2cParams.transferCallbackFxn = I2CCallbackFunction;
    Display_init();
    GPIO_init();

    /* Open the HOST display for output */
    //display = Display_open(Display_Type_UART, NULL);
    //if (display == NULL) {
      //  while (1);
    //}

    /* Turn on user LED */
    GPIO_write(Board_GPIO_LED0, Board_GPIO_LED_ON);
    //Display_printf(display, 0, 0, "Starting program\n");

    /* Create I2C for usage */
    i2c = I2C_open(Board_I2C0, &i2cParams);
    if (i2c == NULL) {
       //Display_printf(display, 0, 0, "Error Initializing I2C\n");
       while (1);
    }
    else {
       //Display_printf(display, 0, 0, "I2C Initialized!\n");
       status = VL53L0X_ERROR_NONE;
    }

    Semaphore_Params semParams;

    /* Construct a Semaphore object to be used as a resource lock, initial count 0 */
    Semaphore_Params_init(&semParams);
    Semaphore_construct(&semStruct, 0, &semParams);

    /* Obtain instance handle */
    semHandle = Semaphore_handle(&semStruct);

    return status;
}
int32_t VL53L0X_comms_close(void)
{
    /* Deinitialized I2C */
    I2C_close(i2c);
    //Display_printf(display, 0, 0, "I2C closed!\n");

    return VL53L0X_ERROR_NONE;
}

int32_t VL53L0X_write_multi(uint8_t address, uint8_t reg, uint8_t *pdata, int32_t count)
{
    int status = VL53L0X_ERROR_NONE;
    //uint8_t txBuffer[1 + count];
    uint8_t * txBuffer;
    txBuffer = (void *)malloc((count+1)*sizeof(*txBuffer));
    memcpy(txBuffer + 1, pdata, count);

    txBuffer[0] = reg;
    i2cTransaction.slaveAddress = address;
    i2cTransaction.writeBuf = txBuffer;
    i2cTransaction.writeCount = count + 1;
    i2cTransaction.readBuf = NULL;
    i2cTransaction.readCount = 0;
    /*
    I2C_transfer(i2c, &i2cTransaction);

    //Wait for transfer to complete here
    Semaphore_pend(semHandle, BIOS_WAIT_FOREVER);
    Display_printf(display, 0, 0, "Data written:%d\n", pdata[0]);
*/


    if (I2C_transfer(i2c, &i2cTransaction)) {
       //Display_printf(display, 0, 0, "Data written\n");
    } else {
      // Display_printf(display, 0, 0, "Write failed\n");
       status = VL53L0X_ERROR_UNDEFINED;
    }

    free(txBuffer);

    return status;
}

int32_t VL53L0X_read_multi(uint8_t address, uint8_t index, uint8_t *pdata, int32_t count)
{

    int status = VL53L0X_ERROR_NONE;

    uint8_t txBuffer[1];
    txBuffer[0] = index;
    i2cTransaction.slaveAddress = address;
    i2cTransaction.writeBuf = txBuffer;
    i2cTransaction.writeCount = 1;
    i2cTransaction.readBuf = NULL;
    i2cTransaction.readCount = 0;
/*
    I2C_transfer(i2c, &i2cTransaction);

    //Wait for transfer to complete here
    Semaphore_pend(semHandle, BIOS_WAIT_FOREVER);
*/

      if (I2C_transfer(i2c, &i2cTransaction)) {
        //memcpy(pdata, rxBuffer, count);
       // Display_printf(display, 0, 0, "Data copied:%d\n", pdata[0]);
    } else {
        //Display_printf(display, 0, 0, "Read failed\n");
        status = VL53L0X_ERROR_UNDEFINED;
    }

    i2cTransaction.slaveAddress = address;
    i2cTransaction.writeBuf = NULL;
    i2cTransaction.writeCount = 0;
    i2cTransaction.readBuf = pdata;
    i2cTransaction.readCount = count;
/*
    I2C_transfer(i2c, &i2cTransaction);

    //Wait for transfer to complete here
    Semaphore_pend(semHandle, BIOS_WAIT_FOREVER);
    Display_printf(display, 0, 0, "Data copied:%d\n", pdata[0]);
*/


    if (I2C_transfer(i2c, &i2cTransaction)) {
        //memcpy(pdata, rxBuffer, count);
      //  Display_printf(display, 0, 0, "Data copied:%d\n", pdata[0]);
    } else {
        //Display_printf(display, 0, 0, "Read failed\n");
        status = VL53L0X_ERROR_UNDEFINED;
    }


    /*
    uint8_t txBuffer[1];
    uint8_t rxBuffer[count];
    txBuffer[0] = index;
    i2cTransaction.slaveAddress = address; //+1?
    i2cTransaction.writeBuf = txBuffer;
    i2cTransaction.writeCount = 1;
    i2cTransaction.readBuf = pdata;
    i2cTransaction.readCount = count;

    if (I2C_transfer(i2c, &i2cTransaction)) {
        //memcpy(pdata, rxBuffer, count);
        Display_printf(display, 0, 0, "Data copied:%d\n", pdata[0]);
    } else {
        Display_printf(display, 0, 0, "Read failed\n");
        status = VL53L0X_ERROR_UNDEFINED;
    }
    */
    return status;
}


int32_t VL53L0X_write_byte(uint8_t address, uint8_t index, uint8_t data)
{
    int32_t status = STATUS_OK;
    const int32_t cbyte_count = 1;

#ifdef VL53L0X_LOG_ENABLE
    trace_print(TRACE_LEVEL_INFO,"Write reg : 0x%02X, Val : 0x%02X\n", index, data);
#endif

    status = VL53L0X_write_multi(address, index, &data, cbyte_count);

    return status;

}


int32_t VL53L0X_write_word(uint8_t address, uint8_t index, uint16_t data)
{
    int32_t status = STATUS_OK;

    uint8_t  buffer[BYTES_PER_WORD];

    // Split 16-bit word into MS and LS uint8_t
    buffer[0] = (uint8_t)(data >> 8);
    buffer[1] = (uint8_t)(data &  0x00FF);

    if(index%2 == 1)
    {
        status = VL53L0X_write_multi(address, index, &buffer[0], 1);
        status = VL53L0X_write_multi(address, index + 1, &buffer[1], 1);
        // serial comms cannot handle word writes to non 2-byte aligned registers.
    }
    else
    {
        status = VL53L0X_write_multi(address, index, buffer, BYTES_PER_WORD);
    }

    return status;

}


int32_t VL53L0X_write_dword(uint8_t address, uint8_t index, uint32_t data)
{
    int32_t status = STATUS_OK;
    uint8_t  buffer[BYTES_PER_DWORD];

    // Split 32-bit word into MS ... LS bytes
    buffer[0] = (uint8_t) (data >> 24);
    buffer[1] = (uint8_t)((data &  0x00FF0000) >> 16);
    buffer[2] = (uint8_t)((data &  0x0000FF00) >> 8);
    buffer[3] = (uint8_t) (data &  0x000000FF);

    status = VL53L0X_write_multi(address, index, buffer, BYTES_PER_DWORD);

    return status;

}


int32_t VL53L0X_read_byte(uint8_t address, uint8_t index, uint8_t *pdata)
{
    int32_t status = STATUS_OK;
    int32_t cbyte_count = 1;

    status = VL53L0X_read_multi(address, index, pdata, cbyte_count);

#ifdef VL53L0X_LOG_ENABLE
    trace_print(TRACE_LEVEL_INFO,"Read reg : 0x%02X, Val : 0x%02X\n", index, *pdata);
#endif

    return status;

}


int32_t VL53L0X_read_word(uint8_t address, uint8_t index, uint16_t *pdata)
{
    int32_t  status = STATUS_OK;
	uint8_t  buffer[BYTES_PER_WORD];

    status = VL53L0X_read_multi(address, index, buffer, BYTES_PER_WORD);
	*pdata = ((uint16_t)buffer[0]<<8) + (uint16_t)buffer[1];

    return status;

}

int32_t VL53L0X_read_dword(uint8_t address, uint8_t index, uint32_t *pdata)
{
    int32_t status = STATUS_OK;
	uint8_t  buffer[BYTES_PER_DWORD];

    status = VL53L0X_read_multi(address, index, buffer, BYTES_PER_DWORD);
    *pdata = ((uint32_t)buffer[0]<<24) + ((uint32_t)buffer[1]<<16) + ((uint32_t)buffer[2]<<8) + (uint32_t)buffer[3];

    return status;

}



// 16 bit address functions
/*

int32_t VL53L0X_write_multi16(uint8_t address, uint16_t index, uint8_t *pdata, int32_t count)
{
    int32_t status = STATUS_OK;
    unsigned int retries = 3;
    DWORD dwWaitResult;

#ifdef VL53L0X_LOG_ENABLE
    int32_t i = 0;

    char value_as_str[VL53L0X_MAX_STRING_LENGTH_PLT];
    char *pvalue_as_str;

    pvalue_as_str =  value_as_str;

    for(i = 0 ; i < count ; i++)
    {
        sprintf(pvalue_as_str,"%02X", *(pdata+i));

        pvalue_as_str += 2;
    }
    trace_i2c("Write reg : 0x%04X, Val : 0x%s\n", index, value_as_str);
#endif

    dwWaitResult = WaitForSingleObject(ghMutex, INFINITE);
    if(dwWaitResult == WAIT_OBJECT_0)
    {
        do
        {
            status = SERIAL_COMMS_Write_UBOOT(address, 0, index, pdata, count);
            // note : the field dwIndexHi is ignored. dwIndexLo will
            // contain the entire index (bits 0..15).
            if(status != STATUS_OK)
            {
                SERIAL_COMMS_Get_Error_Text(debug_string);
            }
        } while ((status != 0) && (retries-- > 0));
        ReleaseMutex(ghMutex);
    }

    // store the page from the high byte of the index
    cached_page = HIBYTE(index);

    if(status != STATUS_OK)
    {
        SERIAL_COMMS_Get_Error_Text(debug_string);
    }


    return status;
}

int32_t VL53L0X_read_multi16(uint8_t address, uint16_t index, uint8_t *pdata, int32_t count)
{
    int32_t status = STATUS_OK;
    unsigned int retries = 3;
    DWORD dwWaitResult;

#ifdef VL53L0X_LOG_ENABLE
    int32_t      i = 0;

    char   value_as_str[VL53L0X_MAX_STRING_LENGTH_PLT];
    char *pvalue_as_str;
#endif


    dwWaitResult = WaitForSingleObject(ghMutex, INFINITE);
    if(dwWaitResult == WAIT_OBJECT_0)
    {
        do
        {
            status = SERIAL_COMMS_Read_UBOOT(address, 0, index, pdata, count);
            if(status != STATUS_OK)
            {
                SERIAL_COMMS_Get_Error_Text(debug_string);
            }
        } while ((status != 0) && (retries-- > 0));
        ReleaseMutex(ghMutex);
    }

    // store the page from the high byte of the index
    cached_page = HIBYTE(index);

    if(status != STATUS_OK)
    {
        SERIAL_COMMS_Get_Error_Text(debug_string);
    }

#ifdef VL53L0X_LOG_ENABLE
    // Build  value as string;
    pvalue_as_str =  value_as_str;

    for(i = 0 ; i < count ; i++)
    {
        sprintf(pvalue_as_str, "%02X", *(pdata+i));
        pvalue_as_str += 2;
    }

    trace_i2c("Read  reg : 0x%04X, Val : 0x%s\n", index, value_as_str);
#endif

    return status;
}



int32_t VL53L0X_write_byte16(uint8_t address, uint16_t index, uint8_t data)
{
    int32_t status = STATUS_OK;
    const int32_t cbyte_count = 1;

#ifdef VL53L0X_LOG_ENABLE
    trace_print(TRACE_LEVEL_INFO,"Write reg : 0x%02X, Val : 0x%02X\n", index, data);
#endif

    status = VL53L0X_write_multi16(address, index, &data, cbyte_count);

    return status;

}


int32_t VL53L0X_write_word16(uint8_t address, uint16_t index, uint16_t data)
{
    int32_t status = STATUS_OK;

    uint8_t  buffer[BYTES_PER_WORD];

    // Split 16-bit word into MS and LS uint8_t
    buffer[0] = (uint8_t)(data >> 8);
    buffer[1] = (uint8_t)(data &  0x00FF);

    if(index%2 == 1)
    {
        status = VL53L0X_write_multi16(address, index, &buffer[0], 1);
        status = VL53L0X_write_multi16(address, index + 1, &buffer[1], 1);
        // serial comms cannot handle word writes to non 2-byte aligned registers.
    }
    else
    {
        status = VL53L0X_write_multi16(address, index, buffer, BYTES_PER_WORD);
    }

    return status;

}


int32_t VL53L0X_write_dword16(uint8_t address, uint16_t index, uint32_t data)
{
    int32_t status = STATUS_OK;
    uint8_t  buffer[BYTES_PER_DWORD];

    // Split 32-bit word into MS ... LS bytes
    buffer[0] = (uint8_t) (data >> 24);
    buffer[1] = (uint8_t)((data &  0x00FF0000) > 16);
    buffer[2] = (uint8_t)((data &  0x0000FF00) > 8);
    buffer[3] = (uint8_t) (data &  0x000000FF);

    status = VL53L0X_write_multi16(address, index, buffer, BYTES_PER_DWORD);

    return status;

}


int32_t VL53L0X_read_byte16(uint8_t address, uint16_t index, uint8_t *pdata)
{
    int32_t status = STATUS_OK;
    int32_t cbyte_count = 1;

    status = VL53L0X_read_multi16(address, index, pdata, cbyte_count);

#ifdef VL53L0X_LOG_ENABLE
    trace_print(TRACE_LEVEL_INFO,"Read reg : 0x%02X, Val : 0x%02X\n", index, *pdata);
#endif

    return status;

}


int32_t VL53L0X_read_word16(uint8_t address, uint16_t index, uint16_t *pdata)
{
    int32_t  status = STATUS_OK;
    uint8_t  buffer[BYTES_PER_WORD];

    status = VL53L0X_read_multi16(address, index, buffer, BYTES_PER_WORD);
    *pdata = ((uint16_t)buffer[0]<<8) + (uint16_t)buffer[1];

    return status;

}

int32_t VL53L0X_read_dword16(uint8_t address, uint16_t index, uint32_t *pdata)
{
    int32_t status = STATUS_OK;
    uint8_t  buffer[BYTES_PER_DWORD];

    status = VL53L0X_read_multi16(address, index, buffer, BYTES_PER_DWORD);
    *pdata = ((uint32_t)buffer[0]<<24) + ((uint32_t)buffer[1]<<16) + ((uint32_t)buffer[2]<<8) + (uint32_t)buffer[3];

    return status;

}




int32_t VL53L0X_platform_wait_us(int32_t wait_us)
{
    int32_t status = STATUS_OK;
    float wait_ms = (float)wait_us/1000.0f;


    HANDLE hEvent = CreateEvent(0, TRUE, FALSE, 0);
    WaitForSingleObject(hEvent, (int)(wait_ms + 0.5f));

#ifdef VL53L0X_LOG_ENABLE
    trace_i2c("Wait us : %6d\n", wait_us);
#endif

    return status;

}


int32_t VL53L0X_wait_ms(int32_t wait_ms)
{
    int32_t status = STATUS_OK;

    HANDLE hEvent = CreateEvent(0, TRUE, FALSE, 0);
    WaitForSingleObject(hEvent, wait_ms);

#ifdef VL53L0X_LOG_ENABLE
    trace_i2c("Wait ms : %6d\n", wait_ms);
#endif

    return status;

}
*/

int32_t VL53L0X_set_gpio(uint8_t level)
{
    int32_t status = STATUS_OK;
    //status = VL53L0X_set_gpio_sv(level);
#ifdef VL53L0X_LOG_ENABLE
    trace_i2c("// Set GPIO = %d;\n", level);
#endif
    return status;

}


int32_t VL53L0X_get_gpio(uint8_t *plevel)
{
    int32_t status = STATUS_OK;
#ifdef VL53L0X_LOG_ENABLE
    trace_i2c("// Get GPIO = %d;\n", *plevel);
#endif
    return status;
}


int32_t VL53L0X_release_gpio(void)
{
    int32_t status = STATUS_OK;
#ifdef VL53L0X_LOG_ENABLE
    trace_i2c("// Releasing force on GPIO\n");
#endif
    return status;

}

int32_t VL53L0X_cycle_power(void)
{
    int32_t status = STATUS_OK;
#ifdef VL53L0X_LOG_ENABLE
    trace_i2c("// cycle sensor power\n");
#endif
	return status;
}


int32_t VL53L0X_get_timer_frequency(int32_t *ptimer_freq_hz)
{
       *ptimer_freq_hz = 0;
       return STATUS_FAIL;
}


int32_t VL53L0X_get_timer_value(int32_t *ptimer_count)
{
       *ptimer_count = 0;
       return STATUS_FAIL;
}
