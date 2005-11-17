  /********************************************************************\
  * BitlBee -- An IRC to other IM-networks gateway                     *
  *                                                                    *
  * Copyright 2002-2004 Wilmer van der Gaast and others                *
  \********************************************************************/

/* MSN module                                                           */

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

/* Some hackish magicstrings to make special-purpose messages/switchboards.
 */
#define TYPING_NOTIFICATION_MESSAGE "\r\r\rBEWARE, ME R TYPINK MESSAGE!!!!\r\r\r"
#define GROUPCHAT_SWITCHBOARD_MESSAGE "\r\r\rME WANT TALK TO MANY PEOPLE\r\r\r"

#ifdef _WIN32
#define debug 
#else
#define debug( text... ) irc_usermsg( IRC, text );
#undef debug
#define debug( text... )
#endif

#define QRY_NAME "msmsgs@msnmsgr.com"
#define QRY_CODE "Q1P7W2E4J9R8U3S5"

#define MSN_SB_NEW         -24062002

#define MSN_MESSAGE_HEADERS "MIME-Version: 1.0\r\n" \
                            "Content-Type: text/plain; charset=UTF-8\r\n" \
                            "User-Agent: BitlBee " BITLBEE_VERSION "\r\n" \
                            "X-MMS-IM-Format: FN=MS%20Shell%20Dlg; EF=; CO=0; CS=0; PF=0\r\n" \
                            "\r\n"

#define MSN_TYPING_HEADERS "MIME-Version: 1.0\r\n" \
                           "Content-Type: text/x-msmsgscontrol\r\n" \
                           "TypingUser: %s\r\n" \
                           "\r\n\r\n"

#define PROFILE_URL "http://members.msn.com/"

struct msn_data
{
	struct gaim_connection *gc;
	
	int fd;
	struct msn_handler_data *handler;
	
	int trId;
	
	GSList *msgq;
	GSList *switchboards;
	int buddycount;
	struct msn_away_state *away_state;
};

struct msn_switchboard
{
	struct gaim_connection *gc;
	
	int fd;
	gint inp;
	struct msn_handler_data *handler;
	
	int trId;
	int ready;
	
	int session;
	char *key;
	
	GSList *msgq;
	char *who;
	struct conversation *chat;
};

struct msn_away_state
{
	int number;
	char code[4];
	char name[16];
};

struct msn_status_code
{
	int number;
	char *text;
	int flags;
};

struct msn_message
{
	char *who;
	char *text;
};

struct msn_handler_data
{
	int fd;
	int rxlen;
	char *rxq;
	
	int msglen;
	char *cmd_text;
	
	gpointer data;
	
	int (*exec_command) ( gpointer data, char **cmd, int count );
	int (*exec_message) ( gpointer data, char *msg, int msglen, char **cmd, int count );
};

/* Bitfield values for msn_status_code.flags */
#define STATUS_FATAL            1
#define STATUS_SB_FATAL         2

int msn_chat_id;
extern struct msn_away_state msn_away_state_list[];
extern struct msn_status_code msn_status_code_list[];

/* Keep a list of all the active connections. We need these lists because
   "connected" callbacks might be called when the connection they belong too
   is down already (for example, when an impatient user disabled the
   connection), the callback should check whether it's still listed here
   before doing *anything* else. */
GSList *msn_connections;
GSList *msn_switchboards;

/* ns.c */
void msn_ns_connected( gpointer data, gint source, GaimInputCondition cond );

/* msn_util.c */
int msn_write( struct gaim_connection *gc, char *s, int len );
int msn_logged_in( struct gaim_connection *gc );
int msn_buddy_list_add( struct gaim_connection *gc, char *list, char *who, char *realname );
int msn_buddy_list_remove( struct gaim_connection *gc, char *list, char *who );
void msn_buddy_ask( struct gaim_connection *gc, char *handle, char *realname );
char *msn_findheader( char *text, char *header, int len );
char **msn_linesplit( char *line );
int msn_handler( struct msn_handler_data *h );

/* tables.c */
struct msn_away_state *msn_away_state_by_number( int number );
struct msn_away_state *msn_away_state_by_code( char *code );
struct msn_away_state *msn_away_state_by_name( char *name );
struct msn_status_code *msn_status_by_number( int number );

/* sb.c */
int msn_sb_write( struct msn_switchboard *sb, char *s, int len );
struct msn_switchboard *msn_sb_create( struct gaim_connection *gc, char *host, int port, char *key, int session );
struct msn_switchboard *msn_sb_by_handle( struct gaim_connection *gc, char *handle );
struct msn_switchboard *msn_sb_by_id( struct gaim_connection *gc, int id );
struct msn_switchboard *msn_sb_spare( struct gaim_connection *gc );
int msn_sb_sendmessage( struct msn_switchboard *sb, char *text );
void msn_sb_to_chat( struct msn_switchboard *sb );
void msn_sb_destroy( struct msn_switchboard *sb );
void msn_sb_connected( gpointer data, gint source, GaimInputCondition cond );