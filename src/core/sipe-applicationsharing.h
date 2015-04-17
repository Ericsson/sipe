/**
 * @file sipe-applicationsharing.h
 *
 * pidgin-sipe
 *
 * Copyright (C) 2014 SIPE Project <http://sipe.sourceforge.net/>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <server/shadow.h>

/* Forward declarations */
struct sipe_core_private;
struct sipmsg;
struct sipe_media_call;

struct monitor_sharing
{
        rdpShadowServer* server;
        const gchar* user_name;
        struct monitor_sharing *next;
};

struct sip_session * sipe_session_find_call(struct sipe_core_private *sipe_private,const gchar *who);

void process_incoming_invite_applicationsharing(struct sipe_core_private *sipe_private,
						struct sipmsg *msg);

//void accept_sharing(gchar* sharing_path);

//void add_user(struct monitor_sharing** list, const gchar* new_user,rdpShadowServer* server);

//void delete_user(struct monitor_sharing **list, const gchar *user_name);

gboolean is_screen_shared(const gchar* user_name);
