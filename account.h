  /********************************************************************\
  * BitlBee -- An IRC to other IM-networks gateway                     *
  *                                                                    *
  * Copyright 2002-2004 Wilmer van der Gaast and others                *
  \********************************************************************/

/* Account management functions                                         */

/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License with
  the Debian GNU/Linux distribution in /usr/share/common-licenses/GPL;
  if not, write to the Free Software Foundation, Inc., 59 Temple Place,
  Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef _ACCOUNT_H
#define _ACCOUNT_H

typedef struct account
{
	int protocol;
	char *user;
	char *pass;
	char *server;
	
	int reconnect;
	
	struct irc *irc;
	struct gaim_connection *gc;
	struct account *next;
} account_t;

account_t *account_add( irc_t *irc, int protocol, char *user, char *pass );
account_t *account_get( irc_t *irc, char *id );
void account_del( irc_t *irc, account_t *acc );
void account_on( irc_t *irc, account_t *a );
void account_off( irc_t *irc, account_t *a );

#endif