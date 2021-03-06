/**
* \copyright
* MIT License
*
* Copyright (c) 2019 Infineon Technologies AG
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE
*
* \endcopyright
*
* \author Infineon Technologies AG
*
* \file pal_os_datastore.c
*
* \brief   This file implements the platform abstraction layer APIs for data store.
*
* \ingroup  grPAL
* @{
*/

#include "optiga/pal/pal_os_datastore.h"
#include "trustm_helper.h"

/// @endcond

/// Size of data store buffer
#define DATA_STORE_BUFFERSIZE   (0x42)

uint8_t data_store_buffer [DATA_STORE_BUFFERSIZE];

//Internal buffer to store application context data use for data store
uint8_t data_store_app_context_buffer [APP_CONTEXT_SIZE];
#define TESTKEY 1
#ifdef TESTKEY
const uint8_t optiga_platform_binding_shared_secret [] = {
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 
    0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10,
    0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 
    0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20,
    0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 
    0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x30,
    0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 
    0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F, 0x40
};
#else
const uint8_t optiga_platform_binding_shared_secret [] = {
    0xbe, 0x35, 0x09, 0xf6, 0x9b, 0x6b, 0x68, 0x6e, 
    0x40, 0x20, 0x29, 0x40, 0x99, 0x20, 0x4b, 0x8e, 
    0x9d, 0x20, 0xeb, 0xbd, 0x1d, 0x47, 0xea, 0xcd, 
    0xe5, 0x54, 0xa9, 0xc1, 0x3d, 0xa0, 0xb7, 0xa6,
    0x64, 0x2a, 0xae, 0x5c, 0xb0, 0x36, 0x85, 0x5c, 
    0xb1, 0xf4, 0x58, 0x33, 0x13, 0x34, 0x85, 0x86, 
    0x83, 0x61, 0xf1, 0x83, 0xf0, 0x4a, 0x4c, 0xaf, 
    0xf6, 0xf4, 0x31, 0x1c, 0x47, 0x8f, 0xcd, 0x29
};
#endif

/// To write the data in the datastore
pal_status_t pal_os_datastore_write(uint16_t datastore_id,
                                    const uint8_t * p_buffer,
                                    uint16_t length)
{
    pal_status_t return_status = PAL_STATUS_FAILURE;

    switch(datastore_id)
    {
        case OPTIGA_PLATFORM_BINDING_SHARED_SECRET_ID:
        {
            // !!!OPTIGA_LIB_PORTING_REQUIRED
            // This has to be enhanced by user only, in case of updating
            // the platform binding shared secret during the runtime.

            return_status = PAL_STATUS_SUCCESS;
            break;
        }
        case OPTIGA_COMMS_MANAGE_CONTEXT_ID:
        {
            // !!!OPTIGA_LIB_PORTING_REQUIRED
            // This has to be enhanced by user only, in case of storing 
            // the manage context information in non-volatile memory 
            // to reuse for later during hard reset scenarios where the 
            // RAM gets flushed out.
            //printf("pal_os_datastore_write : %s!!\n",TRUSTM_CTX_FILENAME);            
            FILE *fp;           
            fp = fopen(TRUSTM_CTX_FILENAME,"wb");
            if (!fp)
            {
                printf("error creating file : %s!!\n",TRUSTM_CTX_FILENAME);
            }
            fwrite(p_buffer, 1, length, fp);                
            fclose(fp);

            return_status = PAL_STATUS_SUCCESS;
            break;
        }
        case OPTIGA_HIBERNATE_CONTEXT_ID:
        {
            // !!!OPTIGA_LIB_PORTING_REQUIRED
            // This has to be enhanced by user only, in case of storing 
            // the application context information in non-volatile memory 
            // to reuse for later during hard reset scenarios where the 
            // RAM gets flushed out.
            //printf("pal_os_datastore_write : %s!!\n",TRUSTM_HIBERNATE_CTX_FILENAME);
            FILE *fp;           
            fp = fopen(TRUSTM_HIBERNATE_CTX_FILENAME,"wb");
            if (!fp)
            {
                printf("error creating file : %s!!\n",TRUSTM_HIBERNATE_CTX_FILENAME);
            }
            fwrite(p_buffer, 1, length, fp);                
            fclose(fp);
                        
            return_status = PAL_STATUS_SUCCESS;
            break;
        }
        default:
        {
            break;
        }
    }

    return return_status;
}

/// Function to read the data from data store
pal_status_t pal_os_datastore_read(uint16_t datastore_id, 
                                   uint8_t * p_buffer, 
                                   uint16_t * p_buffer_length)
{
    pal_status_t return_status = PAL_STATUS_FAILURE;

    switch(datastore_id)
    {
        case OPTIGA_PLATFORM_BINDING_SHARED_SECRET_ID:
        {
            // !!!OPTIGA_LIB_PORTING_REQUIRED
            // This has to be enhanced by user only,
            // if the platform binding shared secret is stored in non-volatile 
            // memory with a specific location and not as a const text segement 
            // else updating the share secret content is good enough.

            if (*p_buffer_length >= sizeof(optiga_platform_binding_shared_secret))
            {
                memcpy(p_buffer,optiga_platform_binding_shared_secret, 
                       sizeof(optiga_platform_binding_shared_secret));
                *p_buffer_length = sizeof(optiga_platform_binding_shared_secret);
                return_status = PAL_STATUS_SUCCESS;
            }
            break;
        }
        case OPTIGA_COMMS_MANAGE_CONTEXT_ID:
        {
            // !!!OPTIGA_LIB_PORTING_REQUIRED
            // This has to be enhanced by user only,
            // if manage context information is stored in NVM during the hibernate, 
            // else this is not required to be enhance
            //printf("pal_os_datastore_read : %s!!\n",TRUSTM_CTX_FILENAME);
            FILE *fp;
            uint8_t data[256];
            int tempLen;           
            fp = fopen(TRUSTM_CTX_FILENAME,"rb");
            if (!fp)
            {
                printf("error reading file : %s!!\n",TRUSTM_CTX_FILENAME);
            }

            tempLen = fread(data, 1, sizeof(data), fp); 
            if (tempLen > 0)
            {
                memcpy(p_buffer,data,tempLen);
                *p_buffer_length = tempLen;
            }
            else
            {
                *p_buffer_length = 0;
            }
            fclose(fp);
            return_status = PAL_STATUS_SUCCESS;
            break;
        }
        case OPTIGA_HIBERNATE_CONTEXT_ID:
        {
            // !!!OPTIGA_LIB_PORTING_REQUIRED
            // This has to be enhanced by user only,
            // if application context information is stored in NVM during the hibernate, 
            // else this is not required to be enhanced.
            //printf("pal_os_datastore_read : %s!!\n",TRUSTM_HIBERNATE_CTX_FILENAME);
            FILE *fp;
            uint8_t data[256];
            int tempLen;           
            fp = fopen(TRUSTM_HIBERNATE_CTX_FILENAME,"rb");
            if (!fp)
            {
                printf("error reading file : %s!!\n",TRUSTM_HIBERNATE_CTX_FILENAME);
            }

            tempLen = fread(data, 1, sizeof(data), fp); 
            if (tempLen > 0)
            {
                memcpy(p_buffer,data,tempLen);
                *p_buffer_length = tempLen;
            }
            else
            {
                *p_buffer_length = 0;
            }
            fclose(fp);
            return_status = PAL_STATUS_SUCCESS;
            break;
        }
        default:
        {
            break;
        }
    }

    return return_status;
}
