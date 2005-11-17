  /********************************************************************\
  * BitlBee -- An IRC to other IM-networks gateway                     *
  *                                                                    *
  * Copyright 2002-2004 Wilmer van der Gaast and others                *
  \********************************************************************/

/* MSN module - Main file; functions to be called from BitlBee          */

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

#include "nogaim.h"
#include "msn.h"

static struct prpl *my_protocol = NULL;

static void msn_login( struct aim_user *acct )
{
	struct gaim_connection *gc = new_gaim_conn( acct );
	struct msn_data *md = g_new0( struct msn_data, 1 );
	
	set_login_progress( gc, 1, "Connecting" );
	
	gc->proto_data = md;
	md->fd = -1;
	
	if( strchr( acct->username, '@' ) == NULL )
	{
		hide_login_progress( gc, "Invalid account name" );
		signoff( gc );
		return;
	}
	
	md->fd = proxy_connect( "messenger.hotmail.com", 1863, msn_ns_connected, gc );
	if( md->fd < 0 )
	{
		hide_login_progress( gc, "Could not connect to server" );
		signoff( gc );
	}
	else
	{
		md->gc = gc;
		md->away_state = msn_away_state_list;
		
		msn_connections = g_slist_append( msn_connections, gc );
	}
}

static void msn_close( struct gaim_connection *gc )
{
	struct msn_data *md = gc->proto_data;
	GSList *l;
	
	if( md->fd >= 0 )
		closesocket( md->fd );
	
	if( md->handler )
	{
		if( md->handler->rxq ) g_free( md->handler->rxq );
		if( md->handler->cmd_text ) g_free( md->handler->cmd_text );
		g_free( md->handler );
	}
	
	while( md->switchboards )
		msn_sb_destroy( md->switchboards->data );
	
	if( md->msgq )
	{
		struct msn_message *m;
		
		for( l = md->msgq; l; l = l->next )
		{
			m = l->data;
			g_free( m->who );
			g_free( m->text );
			g_free( m );
		}
		g_slist_free( md->msgq );
		
		serv_got_crap( gc, "Warning: Closing down MSN connection with unsent message(s), you'll have to resend them." );
	}
	
	for( l = gc->permit; l; l = l->next )
		g_free( l->data );
	g_slist_free( gc->permit );
	
	for( l = gc->deny; l; l = l->next )
		g_free( l->data );
	g_slist_free( gc->deny );
	
	g_free( md );
	
	msn_connections = g_slist_remove( msn_connections, gc );
}

static int msn_send_im( struct gaim_connection *gc, char *who, char *message, int len, int away )
{
	struct msn_switchboard *sb;
	struct msn_data *md = gc->proto_data;
	
	if( ( sb = msn_sb_by_handle( gc, who ) ) )
	{
		return( msn_sb_sendmessage( sb, message ) );
	}
	else
	{
		struct msn_message *m;
		char buf[1024];
		
		/* Create a message. We have to arrange a usable switchboard, and send the message later. */
		m = g_new0( struct msn_message, 1 );
		m->who = g_strdup( who );
		m->text = g_strdup( message );
		
		/* FIXME: *CHECK* the reliability of using spare sb's! */
		if( ( sb = msn_sb_spare( gc ) ) )
		{
			debug( "Trying to use a spare switchboard to message %s", who );
			
			sb->who = g_strdup( who );
			g_snprintf( buf, sizeof( buf ), "CAL %d %s\r\n", ++sb->trId, who );
			if( msn_sb_write( sb, buf, strlen( buf ) ) )
			{
				/* He/She should join the switchboard soon, let's queue the message. */
				sb->msgq = g_slist_append( sb->msgq, m );
				return( 1 );
			}
		}
		
		debug( "Creating a new switchboard to message %s", who );
		
		/* If we reach this line, there was no spare switchboard, so let's make one. */
		g_snprintf( buf, sizeof( buf ), "XFR %d SB\r\n", ++md->trId );
		if( !msn_write( gc, buf, strlen( buf ) ) )
		{
			g_free( m->who );
			g_free( m->text );
			g_free( m );
			
			return( 0 );
		}
		
		/* And queue the message to md. We'll pick it up when the switchboard comes up. */
		md->msgq = g_slist_append( md->msgq, m );
		
		/* FIXME: If the switchboard creation fails, the message will not be sent. */
		
		return( 1 );
	}
	
	return( 0 );
}

static GList *msn_away_states( struct gaim_connection *gc )
{
	GList *l = NULL;
	int i;
	
	for( i = 0; msn_away_state_list[i].number > -1; i ++ )
		l = g_list_append( l, msn_away_state_list[i].name );
	
	return( l );
}

static char *msn_get_status_string( struct gaim_connection *gc, int number )
{
	struct msn_away_state *st = msn_away_state_by_number( number );
	
	if( st )
		return( st->name );
	else
		return( "" );
}

static void msn_set_away( struct gaim_connection *gc, char *state, char *message )
{
	char buf[1024];
	struct msn_data *md = gc->proto_data;
	struct msn_away_state *st;
	
	if( strcmp( state, GAIM_AWAY_CUSTOM ) == 0 )
		st = msn_away_state_by_name( "Away" );
	else
		st = msn_away_state_by_name( state );
	
	if( !st ) st = msn_away_state_list;
	md->away_state = st;
	
	g_snprintf( buf, sizeof( buf ), "CHG %d %s\r\n", ++md->trId, st->code );
	msn_write( gc, buf, strlen( buf ) );
}

static void msn_set_info( struct gaim_connection *gc, char *info )
{
	int i;
	char buf[1024], *fn, *s;
	struct msn_data *md = gc->proto_data;
	
	if( strlen( info ) > 129 )
	{
		do_error_dialog( gc, "Maximum name length exceeded", "MSN" );
		return;
	}
	
	/* Of course we could use http_encode() here, but when we encode
	   every character, the server is less likely to complain about the
	   chosen name. However, the MSN server doesn't seem to like escaped
	   non-ASCII chars, so we keep those unescaped. */
	s = fn = g_new0( char, strlen( info ) * 3 + 1 );
	for( i = 0; info[i]; i ++ )
		if( info[i] & 128 )
		{
			*s = info[i];
			s ++;
		}
		else
		{
			g_snprintf( s, 4, "%%%02X", info[i] );
			s += 3;
		}
	
	g_snprintf( buf, sizeof( buf ), "REA %d %s %s\r\n", ++md->trId, gc->username, fn );
	msn_write( gc, buf, strlen( buf ) );
	g_free( fn );
}

static void msn_get_info(struct gaim_connection *gc, char *who) 
{
	/* Just make an URL and let the user fetch the info */
	serv_got_crap( gc, "%s\n%s: %s%s", _("User Info"), _("For now, fetch yourself"), PROFILE_URL, who );
}

static void msn_add_buddy( struct gaim_connection *gc, char *who )
{
	msn_buddy_list_add( gc, "FL", who, who );
}

static void msn_remove_buddy( struct gaim_connection *gc, char *who, char *group )
{
	msn_buddy_list_remove( gc, "FL", who );
}

static int msn_chat_send( struct gaim_connection *gc, int id, char *message )
{
	struct msn_switchboard *sb = msn_sb_by_id( gc, id );
	
	if( sb )
		return( msn_sb_sendmessage( sb, message ) );
	else
		return( 0 );
}

static void msn_chat_invite( struct gaim_connection *gc, int id, char *msg, char *who )
{
	struct msn_switchboard *sb = msn_sb_by_id( gc, id );
	char buf[1024];
	
	if( sb )
	{
		g_snprintf( buf, sizeof( buf ), "CAL %d %s\r\n", ++sb->trId, who );
		msn_sb_write( sb, buf, strlen( buf ) );
	}
}

static void msn_chat_leave( struct gaim_connection *gc, int id )
{
	struct msn_switchboard *sb = msn_sb_by_id( gc, id );
	
	if( sb )
		msn_sb_write( sb, "OUT\r\n", 5 );
}

static int msn_chat_open( struct gaim_connection *gc, char *who )
{
	struct msn_switchboard *sb;
	struct msn_data *md = gc->proto_data;
	char buf[1024];
	
	if( ( sb = msn_sb_by_handle( gc, who ) ) )
	{
		debug( "Converting existing switchboard to %s to a groupchat", who );
		msn_sb_to_chat( sb );
		return( 1 );
	}
	else
	{
		struct msn_message *m;
		
		if( ( sb = msn_sb_spare( gc ) ) )
		{
			debug( "Trying to reuse an existing switchboard as a groupchat with %s", who );
			g_snprintf( buf, sizeof( buf ), "CAL %d %s\r\n", ++sb->trId, who );
			if( msn_sb_write( sb, buf, strlen( buf ) ) )
			{
				msn_sb_to_chat( sb );
				return( 1 );
			}
		}
		
		/* If the stuff above failed for some reason: */
		debug( "Creating a new switchboard to groupchat with %s", who );
		
		/* Request a new switchboard. */
		g_snprintf( buf, sizeof( buf ), "XFR %d SB\r\n", ++md->trId );
		if( !msn_write( gc, buf, strlen( buf ) ) )
			return( 0 );
		
		/* Create a magic message. This is quite hackish, but who cares? :-P */
		m = g_new0( struct msn_message, 1 );
		m->who = g_strdup( who );
		m->text = g_strdup( GROUPCHAT_SWITCHBOARD_MESSAGE );
		
		/* Queue the magic message and cross your fingers. */
		md->msgq = g_slist_append( md->msgq, m );
		
		return( 1 );
	}
	
	return( 0 );
}

static void msn_keepalive( struct gaim_connection *gc )
{
	msn_write( gc, "PNG\r\n", strlen( "PNG\r\n" ) );
}

static void msn_add_permit( struct gaim_connection *gc, char *who )
{
	msn_buddy_list_add( gc, "AL", who, who );
}

static void msn_rem_permit( struct gaim_connection *gc, char *who )
{
	msn_buddy_list_remove( gc, "AL", who );
}

static void msn_add_deny( struct gaim_connection *gc, char *who )
{
	struct msn_switchboard *sb;
	
	msn_buddy_list_add( gc, "BL", who, who );
	
	/* If there's still a conversation with this person, close it. */
	if( ( sb = msn_sb_by_handle( gc, who ) ) )
	{
		msn_sb_destroy( sb );
	}
}

static void msn_rem_deny( struct gaim_connection *gc, char *who )
{
	msn_buddy_list_remove( gc, "BL", who );
}

static int msn_send_typing( struct gaim_connection *gc, char *who, int typing )
{
	if( typing )
		return( msn_send_im( gc, who, TYPING_NOTIFICATION_MESSAGE, strlen( TYPING_NOTIFICATION_MESSAGE ), 0 ) );
	else
		return( 1 );
}

void msn_init(struct prpl *ret)
{
	ret->protocol = PROTO_MSN;
	ret->login = msn_login;
	ret->close = msn_close;
	ret->send_im = msn_send_im;
	ret->away_states = msn_away_states;
	ret->get_status_string = msn_get_status_string;
	ret->set_away = msn_set_away;
	ret->set_info = msn_set_info;
	ret->get_info = msn_get_info;
	ret->add_buddy = msn_add_buddy;
	ret->remove_buddy = msn_remove_buddy;
	ret->chat_send = msn_chat_send;
	ret->chat_invite = msn_chat_invite;
	ret->chat_leave = msn_chat_leave;
	ret->chat_open = msn_chat_open;
	ret->keepalive = msn_keepalive;
	ret->add_permit = msn_add_permit;
	ret->rem_permit = msn_rem_permit;
	ret->add_deny = msn_add_deny;
	ret->rem_deny = msn_rem_deny;
	ret->send_typing = msn_send_typing;
	ret->cmp_buddynames = g_strcasecmp;

	my_protocol = ret;
}