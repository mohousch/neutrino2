/*
 * $Header: /cvs/tuxbox/apps/misc/libs/libconnection/basicclient.cpp,v 1.17 2004/04/08 07:19:00 thegoodguy Exp $
 *
 * Basic Client Class - The Tuxbox Project
 *
 * (C) 2002-2003 by thegoodguy <thegoodguy@berlios.de>
 *
 * License: GPL
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include "basicclient.h"
#include "basicmessage.h"
#include "basicsocket.h"

#include <inttypes.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>


#define TIMEOUT_SEC  60
#define TIMEOUT_USEC 0
#define MAX_TIMEOUT_SEC  300
#define MAX_TIMEOUT_USEC 0

CBasicClient::CBasicClient()
{
	sock_fd = -1;
}

bool CBasicClient::openConnection()
{
	closeConnection();

	struct sockaddr_un servaddr;
	int clilen;

	memset(&servaddr, 0, sizeof(struct sockaddr_un));
	servaddr.sun_family = AF_UNIX;
	strcpy(servaddr.sun_path, getSocketName());              // no length check !!!
	clilen = sizeof(servaddr.sun_family) + strlen(servaddr.sun_path);

	if ((sock_fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
	{
		perror(getSocketName());
		sock_fd = -1;
		return false;
	}

	if (connect(sock_fd, (struct sockaddr*) &servaddr, clilen) < 0)
	{
		perror(getSocketName());
		closeConnection();
		return false;
	}

	return true;
}

void CBasicClient::closeConnection()
{
	if (sock_fd != -1)
	{
		close(sock_fd);
		sock_fd = -1;
	}
}

bool CBasicClient::sendData(const char* data, const size_t size)
{
	timeval timeout;

	if (sock_fd == -1)
		return false;
	
	timeout.tv_sec  = TIMEOUT_SEC;
	timeout.tv_usec = TIMEOUT_USEC;
	
	if (::send_data(sock_fd, data, size, timeout) == false)
	{
		printf("[CBasicClient] send failed: %s\n", getSocketName());
		closeConnection();
		
		return false;
	}

	return true;
}

bool CBasicClient::sendString(const char* data)
{
	uint8_t send_length;
	size_t length = strlen(data);

	if (length > 255)
	{
		printf("CBasicClient::sendString: string too long - sending only first 255 characters: %s\n", data);
		send_length = 255;
	}
	else
	{
		send_length = static_cast<uint8_t>(length);
	}

	return (sendData((char *)&send_length, sizeof(send_length)) && sendData(data, send_length));
}

bool CBasicClient::receiveData(char* data, const size_t size, bool use_max_timeout)
{
	timeval timeout;

	if (sock_fd == -1)
		return false;

	if (use_max_timeout)
	{
		timeout.tv_sec  = MAX_TIMEOUT_SEC;
		timeout.tv_usec = MAX_TIMEOUT_USEC;
	}
	else
	{
		timeout.tv_sec  = TIMEOUT_SEC;
		timeout.tv_usec = TIMEOUT_USEC;
	}

	if (::receive_data(sock_fd, data, size, timeout) == false)
	{
		printf("[CBasicClient] receive failed: %s\n", getSocketName());
		closeConnection();
		
		return false;
	}
	
	printf("[CBasicClient] receive_data: (%s)\n", getSocketName());

	return true;
}

bool CBasicClient::send(const unsigned char command, const char* data, const unsigned int size)
{
	CBasicMessage::Header msgHead;
	msgHead.version = getVersion();
	msgHead.cmd     = command;

	openConnection(); // if the return value is false, the next send_data call will return false, too

	if (!sendData((char*)&msgHead, sizeof(msgHead)))
	    return false;
	
	if (size != 0)
	    return sendData(data, size);

	return true;
}

