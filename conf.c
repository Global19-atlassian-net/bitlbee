  /********************************************************************\
  * BitlBee -- An IRC to other IM-networks gateway                     *
  *                                                                    *
  * Copyright 2002-2005 Wilmer van der Gaast and others                *
  \********************************************************************/

/* Configuration reading code						*/

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

#include "bitlbee.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "conf.h"
#include "ini.h"
#include "url.h"

#include "protocols/proxy.h"

char *CONF_FILE;

static int conf_loadini( conf_t *conf, char *file );

conf_t *conf_load( int argc, char *argv[] )
{
	conf_t *conf;
	int opt, i;
	
	conf = g_new0( conf_t, 1 );
	
	conf->iface = "0.0.0.0";
	conf->port = 6667;
	conf->nofork = 0;
	conf->verbose = 0;
	conf->runmode = RUNMODE_INETD;
	conf->authmode = AUTHMODE_OPEN;
	conf->password = NULL;
	conf->configdir = g_strdup( CONFIG );
	conf->motdfile = g_strdup( ETCDIR "/motd.txt" );
	conf->ping_interval = 180;
	conf->ping_timeout = 300;
	
	i = conf_loadini( conf, CONF_FILE );
	if( i == 0 )
	{
		fprintf( stderr, "Error: Syntax error in configuration file `%s'.\n", CONF_FILE );
		return( NULL );
	}
	else if( i == -1 )
	{
		fprintf( stderr, "Warning: Unable to read configuration file `%s'.\n", CONF_FILE );
	}
	
	while( ( opt = getopt( argc, argv, "i:p:nvIDc:d:h" ) ) >= 0 )
	{
		if( opt == 'i' )
		{
			conf->iface = g_strdup( optarg );
		}
		else if( opt == 'p' )
		{
			if( ( sscanf( optarg, "%d", &i ) != 1 ) || ( i <= 0 ) || ( i > 65535 ) )
			{
				fprintf( stderr, "Invalid port number: %s\n", optarg );
				return( NULL );
			}
			conf->port = i;
		}
		else if( opt == 'n' )
			conf->nofork=1;
		else if( opt == 'v' )
			conf->verbose=1;
		else if( opt == 'I' )
			conf->runmode=RUNMODE_INETD;
		else if( opt == 'D' )
			conf->runmode=RUNMODE_DAEMON;
		else if( opt == 'c' )
		{
			if( strcmp( CONF_FILE, optarg ) != 0 )
			{
				g_free( CONF_FILE );
				CONF_FILE = g_strdup( optarg );
				g_free( conf );
				return( conf_load( argc, argv ) );
			}
		}
		else if( opt == 'd' )
		{
			g_free( conf->configdir );
			conf->configdir = g_strdup( optarg );
		}
		else if( opt == 'h' )
		{
			printf( "Usage: bitlbee [-D [-i <interface>] [-p <port>] [-n] [-v]] [-I]\n"
			        "               [-c <file>] [-d <dir>] [-h]\n"
			        "\n"
			        "An IRC-to-other-chat-networks gateway\n"
			        "\n"
			        "  -I  Classic/InetD mode. (Default)\n"
			        "  -D  Daemon mode. (Still EXPERIMENTAL!)\n"
			        "  -i  Specify the interface (by IP address) to listen on.\n"
			        "      (Default: 0.0.0.0 (any interface))\n"
			        "  -p  Port number to listen on. (Default: 6667)\n"
			        "  -n  Don't fork.\n"
			        "  -v  Be verbose (only works in combination with -n)\n"
			        "  -c  Load alternative configuration file\n"
			        "  -d  Specify alternative user configuration directory\n"
			        "  -h  Show this help page.\n" );
			return( NULL );
		}
	}
	
	if( conf->configdir[strlen(conf->configdir)-1] != '/' )
	{
		char *s = g_new( char, strlen( conf->configdir ) + 2 );
		
		sprintf( s, "%s/", conf->configdir );
		g_free( conf->configdir );
		conf->configdir = s;
	}
	
	return( conf );
}

static int conf_loadini( conf_t *conf, char *file )
{
	ini_t *ini;
	int i;
	
	ini = ini_open( file );
	if( ini == NULL ) return( -1 );
	while( ini_read( ini ) )
	{
		if( g_strcasecmp( ini->section, "settings" ) == 0 )
		{
			if( g_strcasecmp( ini->key, "runmode" ) == 0 )
			{
				if( g_strcasecmp( ini->value, "daemon" ) == 0 )
					conf->runmode = RUNMODE_DAEMON;
				else
					conf->runmode = RUNMODE_INETD;
			}
			else if( g_strcasecmp( ini->key, "daemoninterface" ) == 0 )
			{
				conf->iface = g_strdup( ini->value );
			}
			else if( g_strcasecmp( ini->key, "daemonport" ) == 0 )
			{
				if( ( sscanf( ini->value, "%d", &i ) != 1 ) || ( i <= 0 ) || ( i > 65535 ) )
				{
					fprintf( stderr, "Invalid port number: %s\n", ini->value );
					return( 0 );
				}
				conf->port = i;
			}
			else if( g_strcasecmp( ini->key, "authmode" ) == 0 )
			{
				if( g_strcasecmp( ini->value, "registered" ) == 0 )
					conf->authmode = AUTHMODE_REGISTERED;
				else if( g_strcasecmp( ini->value, "closed" ) == 0 )
					conf->authmode = AUTHMODE_CLOSED;
				else
					conf->authmode = AUTHMODE_OPEN;
			}
			else if( g_strcasecmp( ini->key, "authpassword" ) == 0 )
			{
				conf->password = g_strdup( ini->value );
			}
			else if( g_strcasecmp( ini->key, "hostname" ) == 0 )
			{
				conf->hostname = g_strdup( ini->value );
			}
			else if( g_strcasecmp( ini->key, "configdir" ) == 0 )
			{
				g_free( conf->configdir );
				conf->configdir = g_strdup( ini->value );
			}
			else if( g_strcasecmp( ini->key, "motdfile" ) == 0 )
			{
				g_free( conf->motdfile );
				conf->motdfile = g_strdup( ini->value );
			}
			else if( g_strcasecmp( ini->key, "pinginterval" ) == 0 )
			{
				if( sscanf( ini->value, "%d", &i ) != 1 )
				{
					fprintf( stderr, "Invalid %s value: %s\n", ini->key, ini->value );
					return( 0 );
				}
				conf->ping_interval = i;
			}
			else if( g_strcasecmp( ini->key, "pingtimeout" ) == 0 )
			{
				if( sscanf( ini->value, "%d", &i ) != 1 )
				{
					fprintf( stderr, "Invalid %s value: %s\n", ini->key, ini->value );
					return( 0 );
				}
				conf->ping_timeout = i;
			}
			else if( g_strcasecmp( ini->key, "proxy" ) == 0 )
			{
				url_t *url = g_new0( url_t, 1 );
				
				if( !url_set( url, ini->value ) )
				{
					fprintf( stderr, "Invalid %s value: %s\n", ini->key, ini->value );
					g_free( url );
					return( 0 );
				}
				
				strncpy( proxyhost, url->host, sizeof( proxyhost ) );
				strncpy( proxyuser, url->user, sizeof( proxyuser ) );
				strncpy( proxypass, url->pass, sizeof( proxypass ) );
				proxyport = url->port;
				if( url->proto == PROTO_HTTP )
					proxytype = PROXY_HTTP;
				else if( url->proto == PROTO_SOCKS4 )
					proxytype = PROXY_SOCKS4;
				else if( url->proto == PROTO_SOCKS5 )
					proxytype = PROXY_SOCKS5;
				
				g_free( url );
			}
			else
			{
				fprintf( stderr, "Error: Unknown setting `%s` in configuration file.\n", ini->key );
				return( 0 );
				/* For now just ignore unknown keys... */
			}
		}
		else if( g_strcasecmp( ini->section, "defaults" ) != 0 )
		{
			fprintf( stderr, "Error: Unknown section [%s] in configuration file. "
			                 "BitlBee configuration must be put in a [settings] section!\n", ini->section );
			return( 0 );
		}
	}
	ini_close( ini );
	
	return( 1 );
}

void conf_loaddefaults( irc_t *irc )
{
	ini_t *ini;
	
	ini = ini_open( CONF_FILE );
	if( ini == NULL ) return;
	while( ini_read( ini ) )
	{
		if( g_strcasecmp( ini->section, "defaults" ) == 0 )
		{
			set_t *s = set_find( irc, ini->key );
			
			if( s )
			{
				if( s->def ) g_free( s->def );
				s->def = g_strdup( ini->value );
			}
		}
	}
	ini_close( ini );
}