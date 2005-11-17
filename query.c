  /********************************************************************\
  * BitlBee -- An IRC to other IM-networks gateway                     *
  *                                                                    *
  * Copyright 2002-2004 Wilmer van der Gaast and others                *
  \********************************************************************/

/* Questions to the user (mainly authorization requests from IM)        */

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

#define BITLBEE_CORE
#include "bitlbee.h"

static void query_display( irc_t *irc, query_t *q );
static query_t *query_default( irc_t *irc );

query_t *query_add( irc_t *irc, struct gaim_connection *gc, char *question, void *yes, void *no, void *data )
{
	query_t *q = g_new0( query_t, 1 );
	
	q->gc = gc;
	q->question = g_strdup( question );
	q->yes = yes;
	q->no = no;
	q->data = data;
	
	if( irc->queries )
	{
		query_t *l = irc->queries;
		
		while( l->next ) l = l->next;
		l->next = q;
	}
	else
	{
		irc->queries = q;
	}
	
	if( g_strcasecmp( set_getstr( irc, "query_order" ), "lifo" ) == 0 || irc->queries == q )
		query_display( irc, q );
	
	return( q );
}

void query_del( irc_t *irc, query_t *q )
{
	query_t *l;
	
	if( irc->queries == q )
	{
		irc->queries = q->next;
	}
	else
	{
		for( l = irc->queries; l; l = l->next )
		{
			if( l->next == q )
			{
				l->next = q->next;
				break;
			}
		}
		
		if( !l )
			return; /* Hrmmm... */
	}
	
	g_free( q->question );
	if( q->data ) g_free( q->data ); /* Memory leak... */
	g_free( q );
}

void query_del_by_gc( irc_t *irc, struct gaim_connection *gc )
{
	query_t *q, *n, *def;
	int count = 0;
	
	q = irc->queries;
	def = query_default( irc );
	
	while( q )
	{
		if( q->gc == gc )
		{
			n = q->next;
			query_del( irc, q );
			q = n;
			
			count ++;
		}
		else
		{
			q = q->next;
		}
	}
	
	if( count > 0 )
		serv_got_crap( gc, "Flushed %d unanswered question(s) for this connection.", count );
	
	q = query_default( irc );
	if( q && q != def )
		query_display( irc, q );
}

void query_answer( irc_t *irc, query_t *q, int ans )
{
	int disp = 0;
	
	if( !q )
	{
		q = query_default( irc );
		disp = 1;
	}
	//Using irc_usermsg instead of serv_got_crap because \x02A is a char too, so a SPACE is needed.	
	if( ans )
	{
		q->yes( NULL, q->data );
		serv_got_crap( q->gc, "Accepted: %s", q->question );
	}
	else
	{
		q->no( NULL, q->data );
		serv_got_crap( q->gc, "Rejected: %s", q->question );
	}
	q->data = NULL;
	
	query_del( irc, q );
	
	if( disp && ( q = query_default( irc ) ) )
		query_display( irc, q );
}

static void query_display( irc_t *irc, query_t *q )
{
	if( q->gc )
	{
		serv_got_crap( q->gc, "New request: %s\nYou can use the \2yes\2/\2no\2 commands to accept/reject this request.", q->question );
	}
	else
	{
		irc_usermsg( irc, "New request: %s\nYou can use the \2yes\2/\2no\2 commands to accept/reject this request.", q->question );
	}
}

static query_t *query_default( irc_t *irc )
{
	query_t *q;
	
	if( g_strcasecmp( set_getstr( irc, "query_order" ), "fifo" ) == 0 )
		q = irc->queries;
	else
		for( q = irc->queries; q && q->next; q = q->next );
	
	return( q );
}