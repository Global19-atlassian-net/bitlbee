  /********************************************************************\
  * BitlBee -- An IRC to other IM-networks gateway                     *
  *                                                                    *
  * Copyright 2002-2004 Wilmer van der Gaast and others                *
  \********************************************************************/

/* Some stuff to fetch, save and handle nicknames for your buddies      */

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

typedef struct __NICK
{
	char *handle;
	int proto;
	char *nick;
	struct __NICK *next;
} nick_t;

void nick_set( irc_t *irc, char *handle, int proto, char *nick );
char *nick_get( irc_t *irc, char *handle, int proto, const char *realname );
void nick_del( irc_t *irc, char *nick );
void nick_strip( char *nick );

int nick_ok( char *nick );
int nick_lc( char *nick );
int nick_uc( char *nick );
int nick_cmp( char *a, char *b );
char *nick_dup( char *nick );