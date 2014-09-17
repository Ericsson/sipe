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

static void
_send_handshake(struct sipe_backend_media *media,
		struct sipe_backend_stream *stream)
{
      guint8 handshake[] = {0x03, 0x00, 0x00, 0x2f, 0x2a, 0xe0, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x43, 0x6f, 0x6f, 0x6b, 0x69,
                            0x65, 0x3a, 0x20, 0x6d, 0x73, 0x74, 0x73, 0x68,
                            0x61, 0x73, 0x68, 0x3d, 0x73, 0x69, 0x70, 0x3a,
                            0x6d, 0x61, 0x72, 0x63, 0x2d, 0x0d, 0x0a, 0x01,
                            0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00};
      //guint8 buffer[2048];
      //gint len;
      g_debug("*************** SENDING RDP HANDSHAKE ********************");
      sipe_backend_media_write(media, stream, handshake, sizeof(handshake), TRUE);

     /* g_debug("*************** Reading response ********************");

      len = sipe_backend_media_read(media, stream, buffer, sizeof(buffer), FALSE);

      g_debug("Read %d bytes", len);
      len = sipe_backend_media_read(media, stream, buffer, 0x10, TRUE);
      g_debug("Read %d bytes", len);
      len = sipe_backend_media_read(media, stream, buffer, sizeof(buffer), FALSE);

      g_debug("Read %d bytes", len);*/
}

struct {
	struct sipe_backend_media *media;
	struct sipe_backend_stream *stream;
} foobar;

static gboolean
_send_handshake_cb(gpointer user_data)
{
	(void)user_data;
	_send_handshake(foobar.media, foobar.stream);
	return FALSE;
}

static void
writable_cb(struct sipe_media_call *call,
	    struct sipe_backend_stream *backend_stream,
	    gboolean writable)
{
	if (writable) {
		/* We need to send the data after the reinvite, or need to set the encryption params after the first invite*/
		foobar.media = call->backend_private;
		foobar.stream = backend_stream;
		g_timeout_add(1000, _send_handshake_cb, NULL);
	}
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
		call->writable_cb = writable_cb;
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
