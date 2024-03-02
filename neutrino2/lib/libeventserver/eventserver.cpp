/*

        eventserver.cpp 02.03.2024 mohousch Exp $

	Event-Server  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/

	License: GPL

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <eventserver.h>

#include <system/debug.h>


void CEventServer::sendEvent(const unsigned int eventID, const void* eventbody, const unsigned int eventbodysize, const char * udsname)
{
	struct sockaddr_un servaddr;
	int clilen, sock_fd;

	dprintf(DEBUG_NORMAL, "CEventServer::sendEvent >(%s)\n", udsname);

	memset(&servaddr, 0, sizeof(struct sockaddr_un));
	
	servaddr.sun_family = AF_UNIX;
	strcpy(servaddr.sun_path, udsname);
	clilen = sizeof(servaddr.sun_family) + strlen(servaddr.sun_path);

	if ((sock_fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
	{
		perror("[eventserver]: socket");
		dprintf(DEBUG_INFO, "CEventServer::sendEvent <\n");
		return;
	}

	if(connect(sock_fd, (struct sockaddr*) &servaddr, clilen) < 0 )
	{
		char errmsg[128];
		snprintf(errmsg, 128, "[eventserver]: connect (%s)", udsname);
		perror(errmsg);
		close(sock_fd);
		dprintf(DEBUG_NORMAL, "CEventServer::sendEvent <(%s)\n", udsname);
		return;
	}

	eventHead head;
	head.eventID = eventID;
	head.dataSize = eventbodysize;
	
	write(sock_fd, &head, sizeof(head));

	if(eventbodysize != 0)
	{
		write(sock_fd, eventbody, eventbodysize);
	}
	
	close(sock_fd);
	
	return;
}

