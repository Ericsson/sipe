/**
 * @file sipe-applicationsharing.c
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

#include <glib.h>

#include "sipe-applicationsharing.h"
#include "sipe-backend.h"
#include "sipe-core.h"
#include "sipe-core-private.h"
#include "sipe-media.h"

static void
read_cb(struct sipe_media_call *call, struct sipe_backend_stream *backend_stream)
{
	guint8 buffer[0x800];

	SIPE_DEBUG_INFO_NOFORMAT("INCOMING APPSHARE DATA");
	sipe_backend_media_read(call->backend_private,
			backend_stream, buffer, sizeof (buffer), FALSE);
}

void
process_incoming_invite_applicationsharing(struct sipe_core_private *sipe_private,
					   struct sipmsg *msg)
{
	struct sipe_media_call *call = NULL;

	process_incoming_invite_call(sipe_private, msg);

	call = (struct sipe_media_call *)sipe_private->media_call;
	if (call) {
		sipe_backend_media_accept(call->backend_private, TRUE);
		call->read_cb = read_cb;
	}
}

/*
  Local Variables:
  mode: c
  c-file-style: "bsd"
  indent-tabs-mode: t
  tab-width: 8
  End:
*/
