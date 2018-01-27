// ConsoleApplication10.cpp : �������̨Ӧ�ó������ڵ㡣develop
//

#include "stdafx.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

typedef enum
{
	MIPL_DEBUG_LEVEL_NONE = 0,
	MIPL_DEBUG_LEVEL_RXL,
	MIPL_DEBUG_LEVEL_RXL_RXD,
	MIPL_DEBUG_LEVEL_TXL_TXD,
} MIPL_DEBUG_LEVEL_E;

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned uint32;

typedef struct
{
	uint8 boot;
	uint8 encrypt;
	MIPL_DEBUG_LEVEL_E debug;
	uint16 port;
	uint32 keep_alive;
	size_t uri_len;
	char* uri;
	size_t ep_len;
	char* ep;
	uint8 block1;	//COAP option BLOCK1(PUT or POST),0-6. 2^(4+n)  bytes
	uint8 block2;	//COAP option BLOCK2(GET),0-6. 2^(4+n)  bytes
	uint8 block2th;	//max size to trigger block-wise operation,0-2. 2^(8+n) bytes
} MIPL_T;

void usage(const char *name)
{
	printf("Usage: %s [OPTION]\r\n", name);
	printf("Launch a client.\r\n");
	printf("version: v1.2\r\n");
	printf("Options:\r\n");
	printf("-b BOOT\t\tSet the bootstrap mode of the client.Default: 0\r\n");
	printf("-d DEBUG\tSet the debug mode of the client.Default: 0\r\n");
	printf("-e ENCRYPT\tSet the encrypt of the client.\r\n");
	printf("-i URI\t\tSet the coap uri of the server to connect to. For example: coap://localhost:5683\r\n");
	printf("-n NAME\t\tSet the endpoint name[imei;imsi] of the client.\r\n");
	printf("-p PORT\t\tSet the local port of the client to bind to. Default: srand\r\n");
	printf("-t TIME\t\tSet the lifetime of the client. Default: 300\r\n");
	printf("-u BLOCK1\tSet COAP option BLOCK1(PUT or POST),0-6. Default 5(512B),2^(4+n)\r\n");
	printf("-g BLOCK2\tSet COAP option BLOCK2(GET),0-6. Default 5(512B),2^(4+n)\r\n");
	printf("-x BLOCK2TH\tSet max size to trigger block-wise operation,0-2. Default 2(1024B),2^(8+n)\r\n");
	printf("\r\n");
}

void output_buffer(unsigned char *buffer, int length, int index, int flag)
{
	int i = 0;
	while (i < length)
	{
		printf("%02X", buffer[i++]);
	}
	printf(",%d,%d\r\n", index, flag);
}
/*****************************************************************
0               1               2               3
0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
| boot | encrypt|     debug     |          local_port         |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                          life_time                          |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|           host_len            |             host            |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|          ep_name_len          |      ep_name(imei;imsi)     |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|  coap_block1  |  coap_block2  |    block2th   |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*****************************************************************/
void mipl_generate(char *buf, size_t buflen, MIPL_T *mipl)
{
	uint32 offset = 0;
	int mipl_num = 0;
	memset(buf, 0, buflen);

	char mode = ((mipl->boot & 0x1) << 4) | (mipl->encrypt & 0x1);
	memcpy(buf + offset, &mode, 1);
	offset++;
	memcpy(buf + offset, &mipl->debug, 1);
	offset++;
	memcpy(buf + offset, &mipl->port, 2);
	offset += 2;
	memcpy(buf + offset, &mipl->keep_alive, 4);
	offset += 4;

	memcpy(buf + offset, &mipl->uri_len, 2);
	offset += 2;
	memcpy(buf + offset, mipl->uri, mipl->uri_len);
	offset += mipl->uri_len;
	memcpy(buf + offset, &mipl->ep_len, 2);
	offset += 2;
	memcpy(buf + offset, mipl->ep, mipl->ep_len);
	offset += mipl->ep_len;

	*(buf + offset) = mipl->block1;
	offset++;
	*(buf + offset) = mipl->block2;
	offset++;
	*(buf + offset) = mipl->block2th;
	offset++;

	mipl_num = 0;
	while (offset > 1024)//ÿ��+MIPLCONF�����������1024�ֽ���������
	{
		printf("AT+MIPLCONF=%d,", 1024);
		output_buffer((unsigned char *)(buf + (mipl_num >> 10)), 1024, mipl_num + 1, 0);//index��1��ʼ
		mipl_num++;
		offset -= 1024;
	}
	printf("AT+MIPLCONF=%d,", offset);
	output_buffer((unsigned char *)(buf + (mipl_num >> 10)), offset, mipl_num + 1, 1);//����
}



int _tmain(int argc, _TCHAR* argv[])
{
	char conf[1024];

	//OneNET_config_v1.2.exe - b 1 - e 0 - d 3 - i "coap://183.230.40.39:5683" - n "865820030133123;460041995401123" - p 0 - t 3000 - u 5 - g 5 - x 1
	MIPL_T mipl;

	mipl.block1 = atoi("5");//-u ���� PUT �� POST ָ���Ƭ���ȣ� ��Χ 0~6�� ָʾ��Ƭ����Ϊ 2^(4+u)�� ȱʡֵΪ 5��
	mipl.block2 = atoi("5");//-g ���� GET ָ���Ƭ���ȣ���Χ 0~6��ָʾ��Ƭ����Ϊ 2^(4+g)��ȱʡֵΪ 5��
	mipl.block2th = atoi("1");// -x ���ô�����Ƭ��������󳤶ȣ� ��Χ 0~2��ָʾ��ֵΪ2 ^ (4 + x)�� ȱʡֵΪ 2��

	/*
	-i host ���ã���ʽΪ��ServerURI:Port����
	ServerURI Զ�˷�������ַ�� ����ƽ̨ʹ�� 183.230.40.39
	Port Զ�˷������˿ںţ� ����ƽ̨ʹ�� 5683
	*/
	mipl.uri ="coap://183.230.40.39:5683"; 
	mipl.uri_len =  strlen("coap://183.230.40.39:5683");

	/*
	-p ���˶˿ںţ� ��Χ 0~65535�� ȱʡֵ 0�� ��ѡ��ȱʡʱ�� ģ��
	���Զ��� 32768~65535 ��ѡ��һ�����õĶ˿ںţ�
	*/
	mipl.port = atoi("0");
	
	/*
	-t �豸���ʱ�䣬 ��ʾ�ն˺� OneNET ƽ̨֮�����ӵĴ����
	�ڣ� ���÷�ΧΪ 10s~86400s��
	*/
	mipl.keep_alive = atoi("3000");


	/*
	-n ��Ȩ������ ��ʽΪ��IMEI;IMSI���� ��Ӧƽ̨���豸ע��ʱ�ļ�Ȩ���� IMEI �� IMSI��
	*/
	mipl.ep ="865820030133123;460041995401123"; 
	mipl.ep_len =strlen("865820030133123;460041995401123");

	/*
	-d Debug �ȼ����ã�
	0 �ر� debug ģʽ
	1 ����ӡ����/���Ͱ����ݳ���
	2 ��ӡ�������ݰ����Ⱥͽ����������ݼ����ݳ���
	3 ��ӡ����/�����������ݼ����ݳ���
	*/
	mipl.debug = MIPL_DEBUG_LEVEL_TXL_TXD;

	mipl.boot = atoi("1");//-b Bootstrap ģʽ���ã� 1 Ϊ�򿪣� 0 Ϊ�رգ� ��Ҫ����Ϊ 1��

	mipl.encrypt = atoi("0");//-e ����ģʽ���ã� 1 Ϊ�򿪣� 0 Ϊ�رգ�Ŀǰ��֧�ּ���ģʽ��

	//
	mipl_generate(conf,sizeof(conf),&mipl);	
	
	return 0;
}

