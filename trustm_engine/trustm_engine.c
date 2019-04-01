/**
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

*/
#include <string.h>
#include <openssl/engine.h>

#include "trustm_helper.h"

#include "trustm_engine.h"
#include "trustm_engine_common.h"

trustm_ctx_t trustm_ctx;

#ifdef WORKAROUND
	extern void pal_os_event_disarm(void);
	extern void pal_os_event_arm(void);
	extern void pal_os_event_destroy1(void);
#endif

static const char *engine_id   = "trustm_engine";
static const char *engine_name = "Infineon OPTIGA TrustM Engine";

static uint32_t parseKeyParams(const char *aArg)
{
	uint32_t value;
	char in[1024];

	char *token[5];
	int   i;
	FILE *fp;
	  
	TRUSTM_ENGINE_DBGFN(">");
    
    strncpy(in, aArg,1024);
    
	if (aArg == NULL)
	{
		TRUSTM_ENGINE_ERRFN("No input key parameters present. (key_oid:<pubkeyfile>)");
		return EVP_FAIL;
	}
	  
	i = 0;
	token[0] = strtok((char *)aArg, ":");
	
	if (token[0] == NULL)
	{
	  TRUSTM_ENGINE_ERRFN("Too few parameters in key parameters list. (key_oid:<pubkeyfile>)");
	  return EVP_FAIL;
	}

	while (token[i] != NULL)
	{
		i++;
		token[i] = strtok(NULL, ":");
	}

	if (i > 6)
	{
	  TRUSTM_ENGINE_ERRFN("Too many parameters in key parameters list. (key_oid:<pubkeyfile>)");
	  return EVP_FAIL;
	}
	
	if (strncmp(token[0], "0x",2) == 0)
		sscanf(token[0],"%x",&value);
	else
	{
		if(i==1) // this is to workaround OpenSSL s_server 
		{
			fp = fopen((const char *)token[0],"r");
			if (!fp)
			{
				TRUSTM_ENGINE_ERRFN("failed to open key file %s\n",token[0]);
				return EVP_FAIL;
			}

			//Read file
			value = 0;
			fread(&value,2,1, fp); 
			fclose(fp);
			TRUSTM_ENGINE_DBGFN("value : %x\n",value); 
		}
		else		
			value = 0;
	}

	trustm_ctx.key_oid = value;
	if ((token[1] != NULL) && (*(token[1]) != '*'))
	{
		strncpy(trustm_ctx.pubkeyfilename, token[1], PUBKEYFILE_SIZE);
	}
	else
		trustm_ctx.pubkeyfilename[0]='\0';

	if ((i>2) && (token[2] != NULL))
	{
			if (!strcmp(token[2],"NEW"))
			{
				// Request NEW key generation
				if (((value >= 0xE0FC) && (value <= 0xE0FD)))
				{
					TRUSTM_ENGINE_DBGFN("found NEW\n");
					trustm_ctx.rsa_flag = TRUSTM_ENGINE_FLAG_NEW;
					if ((i>3) && (strncmp(token[3], "0x",2) == 0))
						sscanf(token[3],"%x",&(trustm_ctx.rsa_key_type));
					if ((i>4) && (strncmp(token[4], "0x",2) == 0))
						sscanf(token[4],"%x",&(trustm_ctx.rsa_key_usage));
					if ((i>5) && (strcmp(token[5], "LOCK") == 0))
						trustm_ctx.rsa_flag |= TRUSTM_ENGINE_FLAG_LOCK;
				}
				
				if (((value >= 0xE0F1) && (value <= 0xE0F3)))
				{
					TRUSTM_ENGINE_DBGFN("found NEW\n");
					trustm_ctx.ec_flag = TRUSTM_ENGINE_FLAG_NEW;
					if ((i>3) && (strncmp(token[3], "0x",2) == 0))
						sscanf(token[3],"%x",&(trustm_ctx.ec_key_curve));
					if ((i>4) && (strncmp(token[4], "0x",2) == 0))
						sscanf(token[4],"%x",&(trustm_ctx.ec_key_usage));
					if ((i>5) && (strcmp(token[5], "LOCK") == 0))
						trustm_ctx.ec_flag |= TRUSTM_ENGINE_FLAG_LOCK;
				}
				
			}
			else
			{
				// No NEW key request
				TRUSTM_ENGINE_DBGFN("No NEW found\n");
			}
	}


	if (((value < 0xE0F0) || (value > 0xE0F3)) &&
		((value < 0xE0FC) || (value > 0xE0FD)))
	{
	  TRUSTM_ENGINE_ERRFN("Invalid Key OID");
	  return EVP_FAIL;
	}

	TRUSTM_ENGINE_DBGFN("<");

	return value;
}

static int engine_destroy(ENGINE *e)
{
	uint16_t i;
	
	TRUSTM_ENGINE_DBGFN("> Engine 0x%x destroy", (unsigned int) e);

	//Clear TrustM context
	trustm_ctx.key_oid = 0x0000;
	trustm_ctx.rsa_key_type = 0;
	trustm_ctx.rsa_key_usage = 0;
	trustm_ctx.rsa_key_enc_scheme = 0;
	trustm_ctx.rsa_key_sig_scheme = 0;
	trustm_ctx.rsa_flag = 0;
		
	trustm_ctx.ec_flag = 0;
	
	trustm_ctx.pubkeylen = 0;

	for(i=0;i<PUBKEYFILE_SIZE;i++)
	{
		trustm_ctx.pubkeyfilename[i] = 0x00;
	}
	for(i=0;i<PUBKEY_SIZE;i++)
	{
		trustm_ctx.pubkey[i] = 0x00;
	}

#ifdef WORKAROUND
	pal_os_event_arm();
#endif
	
	trustm_Close();
#ifdef WORKAROUND	
	pal_os_event_disarm();
	pal_os_event_destroy1();
#endif

	TRUSTM_ENGINE_DBGFN("<");
	return TRUSTM_ENGINE_SUCCESS;
}

static int engine_finish(ENGINE *e)
{
	TRUSTM_ENGINE_DBGFN("> Engine 0x%x finish (releasing functional reference)", (unsigned int) e);
	TRUSTM_ENGINE_DBGFN("<");
	return TRUSTM_ENGINE_SUCCESS;
}

/**************************************************************** 
 engine_load_privkey()
 This function implements loading trustx key.
 e        : The engine for this callback (unused).
 key_id   : The name of the file with the TPM key data.
 ui The ui: functions for querying the user.
 cb_data  : Callback data.
*****************************************************************/
static EVP_PKEY * engine_load_privkey(ENGINE *e, const char *key_id, UI_METHOD *ui, void *cb_data)
{
	EVP_PKEY    *key         = NULL;	
	
	TRUSTM_ENGINE_DBGFN("> key_id : %s", key_id);

	do {
		parseKeyParams(key_id);
	
		TRUSTM_ENGINE_DBGFN("KEY_OID       : 0x%.4x \n",trustm_ctx.key_oid);		
		TRUSTM_ENGINE_DBGFN("Pubkey        : %s \n",trustm_ctx.pubkeyfilename);

		TRUSTM_ENGINE_DBGFN("RSA key type  : 0x%.2x \n",trustm_ctx.rsa_key_type);
		TRUSTM_ENGINE_DBGFN("RSA key usage : 0x%.2x \n",trustm_ctx.rsa_key_usage);	
		TRUSTM_ENGINE_DBGFN("RSA key flag  : 0x%.2x \n",trustm_ctx.rsa_flag);	

		TRUSTM_ENGINE_DBGFN("EC key type  : 0x%.2x \n",trustm_ctx.ec_key_curve);
		TRUSTM_ENGINE_DBGFN("EC key usage : 0x%.2x \n",trustm_ctx.ec_key_usage);	
		TRUSTM_ENGINE_DBGFN("EC key flag  : 0x%.2x \n",trustm_ctx.ec_flag);		
		switch(trustm_ctx.key_oid)
		{
			case 0xE0F0:
				TRUSTM_ENGINE_MSGFN("Function Not implemented.");
				break;
			case 0xE0F1:
			case 0xE0F2:
			case 0xE0F3:
				TRUSTM_ENGINE_MSGFN("Function Not implemented.");
				break;
			case 0xE0FC:
			case 0xE0FD:
				TRUSTM_ENGINE_DBGFN("RSA Private Key.");
				key = trustm_rsa_loadkey();
				break;
			case 0xE100:
			case 0xE101:
			case 0xE102:
			case 0xE103:
				TRUSTM_ENGINE_MSGFN("Function Not implemented.");
				break;
			default:
				TRUSTM_ENGINE_ERRFN("Invalid OID!!!");
		}
		
	}while(FALSE);

#ifdef WORKAROUND	
	pal_os_event_disarm();
#endif

	TRUSTM_ENGINE_DBGFN("<");
    return key;
}

/**************************************************************** 
 engine_load_pubkey()
 This function implements loading trustx key.
 e        : The engine for this callback (unused).
 key_id   : The name of the file with the TPM key data.
 ui The ui: functions for querying the user.
 cb_data  : Callback data.
*****************************************************************/
static EVP_PKEY * engine_load_pubkey(ENGINE *e, const char *key_id, UI_METHOD *ui, void *cb_data)
{
	EVP_PKEY    *key         = NULL;	
    FILE *fp;
    char *name;
    char *header;
    uint8_t *data;
    uint32_t len;
    uint16_t i;
    	
	TRUSTM_ENGINE_DBGFN("> key_id : %s", key_id);

	do {
		if (key_id == NULL)
		{
			TRUSTM_ENGINE_ERRFN("No input key parameters present. (key_oid:<pubkeyfile>)");
			break;
		}
		
		strcpy(trustm_ctx.pubkeyfilename, key_id);
				
		if (trustm_ctx.pubkeyfilename[0] != '\0')
		{
			TRUSTM_ENGINE_DBGFN("filename : %s\n",trustm_ctx.pubkeyfilename);
			//open 
			fp = fopen((const char *)trustm_ctx.pubkeyfilename,"r");
			if (!fp)
			{
				TRUSTM_ENGINE_ERRFN("failed to open file %s\n",trustm_ctx.pubkeyfilename);
				break;
			}
			PEM_read(fp, &name,&header,&data,(long int *)&len);
			//TRUSTM_ENGINE_DBGFN("name   : %s\n",name);
			//TRUSTM_ENGINE_DBGFN("len : %d\n",len);
			//trustmHexDump(data,len);
			if (!(strcmp(name,"PUBLIC KEY")))
			{
				trustm_ctx.pubkeylen = (uint16_t)len;
				for(i=0;i<len;i++)
				{
					trustm_ctx.pubkey[i] = *(data+i);
					//printf("%.x ",trustm_ctx.pubkey[i]);
				}
				key = d2i_PUBKEY(NULL,(const unsigned char **)&data,len);

				//trustmHexDump(trustm_ctx.pubkey,trustm_ctx.pubkeylen);
			}
		}
		
	}while(FALSE);

#ifdef WORKAROUND	
	pal_os_event_disarm();
#endif

	TRUSTM_ENGINE_DBGFN("<");
    return key;
}


static int engine_ctrl(ENGINE *e, int cmd, long i, void *p, void (*f) ())
{
	int ret = TRUSTM_ENGINE_SUCCESS;

	TRUSTM_ENGINE_DBGFN(">");
	TRUSTM_ENGINE_DBGFN(">");
	TRUSTM_ENGINE_DBGFN("cmd: %d", cmd);
	TRUSTM_ENGINE_DBGFN("P : %s", (char *)p);

	do {
		TRUSTM_ENGINE_MSGFN("Function Not implemented.");
		//Implement code here;
	}while(FALSE);
	
	TRUSTM_ENGINE_DBGFN("<");
    return ret;
	
}

static int engine_init(ENGINE *e)
{
    static int initialized = 0;
    optiga_lib_status_t return_status;

	int ret = TRUSTM_ENGINE_SUCCESS;
	TRUSTM_ENGINE_DBGFN("> Engine 0x%x init", (unsigned int) e);

	do {
		TRUSTM_ENGINE_DBGFN("Initializing");
		if (initialized) {
			TRUSTM_ENGINE_DBGFN("Already initialized");
			ret = TRUSTM_ENGINE_SUCCESS;
			break;
		}

		return_status = trustm_Open();
		if (return_status != OPTIGA_LIB_SUCCESS)
		{
			TRUSTM_ENGINE_ERRFN("Fail to open trustM!!");			
			break;
		}

		//Init TrustM context
		trustm_ctx.key_oid = 0x0000;
		trustm_ctx.rsa_key_type = OPTIGA_RSA_KEY_2048_BIT_EXPONENTIAL;
		trustm_ctx.rsa_key_usage = OPTIGA_KEY_USAGE_AUTHENTICATION;
		trustm_ctx.rsa_key_enc_scheme = OPTIGA_RSAES_PKCS1_V15;
		trustm_ctx.rsa_key_sig_scheme = OPTIGA_RSASSA_PKCS1_V15_SHA256;
		trustm_ctx.rsa_flag = TRUSTM_ENGINE_FLAG_NONE;
		
		trustm_ctx.ec_flag = TRUSTM_ENGINE_FLAG_NONE;
		
		trustm_ctx.pubkeyfilename[0] = '\0';
		trustm_ctx.pubkey[0] = '\0';
		trustm_ctx.pubkeylen = 0;

		// Init Random Method
		ret = trustmEngine_init_rand(e);
		if (ret != TRUSTM_ENGINE_SUCCESS) {
			TRUSTM_ENGINE_ERRFN("Init Rand Fail!!");
			break;
		}

		// Init RSA Method
		ret = trustmEngine_init_rsa(e);
		if (ret != TRUSTM_ENGINE_SUCCESS) {
			TRUSTM_ENGINE_ERRFN("Init RSA Fail!!");
			break;
		}

		initialized = 1;
	}while(FALSE);
	
	TRUSTM_ENGINE_DBGFN("<");
	return ret;
}

static int bind(ENGINE *e, const char *id)
{
	int ret = TRUSTM_ENGINE_FAIL;
	
    TRUSTM_ENGINE_DBGFN(">");

	do {
		if (!ENGINE_set_id(e, engine_id)) {
			TRUSTM_ENGINE_DBGFN("ENGINE_set_id failed\n");
			break;
		}
		if (!ENGINE_set_name(e, engine_name)) {
			TRUSTM_ENGINE_DBGFN("ENGINE_set_name failed\n");
			break;
		}

		/* The init function is not allways called so we initialize crypto methods
		   directly from bind. */
		if (!engine_init(e)) {
			TRUSTM_ENGINE_DBGFN("TrustM enigne initialization failed\n");
			break;
		}

		if (!ENGINE_set_load_privkey_function(e, engine_load_privkey)) {
			TRUSTM_ENGINE_DBGFN("ENGINE_set_load_privkey_function failed\n");
			break;
		}
		
		if (!ENGINE_set_load_pubkey_function(e, engine_load_pubkey)) {
			TRUSTM_ENGINE_DBGFN("ENGINE_set_load_pubkey_function failed\n");
			break;
		}

		
		if (!ENGINE_set_finish_function(e, engine_finish)) {
			TRUSTM_ENGINE_DBGFN("ENGINE_set_finish_function failed\n");
			break;
		}

		if (!ENGINE_set_destroy_function(e, engine_destroy)) {
			TRUSTM_ENGINE_DBGFN("ENGINE_set_destroy_function failed\n");
			break;
		}

		if (!ENGINE_set_ctrl_function(e, engine_ctrl)) {
			TRUSTM_ENGINE_DBGFN("ENGINE_set_ctrl_function failed\n");
			break;
		}

/*
		if (!ENGINE_set_cmd_defns(e, engine_cmd_defns)) {
			TRUSTM_ENGINE_DBGFN("ENGINE_set_cmd_defns failed\n");
			break;
		}
*/
		ret = TRUSTM_ENGINE_SUCCESS;
	}while(FALSE);

    TRUSTM_ENGINE_DBGFN("<");
    return ret;
  }

IMPLEMENT_DYNAMIC_BIND_FN(bind)
IMPLEMENT_DYNAMIC_CHECK_FN()

