/**
 * @file sipe-ft-lync.c
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib.h>
#include <stdlib.h>
#include <string.h>

#include "sip-transport.h"
#include "sipe-backend.h"
#include "sipe-common.h"
#include "sipe-core.h"
#include "sipe-core-private.h"
#include "sipe-ft-lync.h"
#include "sipe-media.h"
#include "sipe-mime.h"
#include "sipe-session.h"
#include "sipe-utils.h"
#include "sipe-xml.h"
#include "sipmsg.h"

struct sipe_file_transfer_lync {
	struct sipe_file_transfer public;

	gchar *sdp;
	gchar *file_name;
	gchar *id;
	gsize file_size;
	guint request_id;

	guint expecting_len;

	struct sipe_core_private *sipe_private;
	struct sip_dialog *dialog;

	gboolean was_cancelled;

	void (*call_reject_parent_cb)(struct sipe_media_call *call,
				      gboolean local);
};
#define SIPE_FILE_TRANSFER         ((struct sipe_file_transfer *) ft_private)
#define SIPE_FILE_TRANSFER_PRIVATE ((struct sipe_file_transfer_lync *) ft)

static void
ft_lync_deallocate(struct sipe_file_transfer *ft);

static void
sipe_file_transfer_lync_free(struct sipe_file_transfer_lync *ft_private)
{
	g_free(ft_private->file_name);
	g_free(ft_private->sdp);
	g_free(ft_private->id);
	g_free(ft_private);
}

static gboolean
request_download_file(struct sipe_file_transfer_lync *ft_private)
{
	gchar *body;

	static const gchar *SUCCESS_RESPONSE =
		"<response xmlns=\"http://schemas.microsoft.com/rtc/2009/05/filetransfer\" requestId=\"%d\" code=\"success\"/>";

	static const gchar *DOWNLOAD_FILE_REQUEST =
		"<request xmlns=\"http://schemas.microsoft.com/rtc/2009/05/filetransfer\" requestId=\"%d\">"
			"<downloadFile>"
				"<fileInfo>"
					"<id>%s</id>"
					"<name>%s</name>"
				"</fileInfo>"
			"</downloadFile>"
		"</request>";

	body = g_strdup_printf(SUCCESS_RESPONSE, ft_private->request_id);

	sip_transport_info(ft_private->sipe_private,
			   "Content-Type: application/ms-filetransfer+xml\r\n",
			   body,
			   ft_private->dialog,
			   NULL);

	g_free(body);

	body = g_strdup_printf(DOWNLOAD_FILE_REQUEST,
			       ++ft_private->request_id,
			       ft_private->id,
			       ft_private->file_name);

	sip_transport_info(ft_private->sipe_private,
			   "Content-Type: application/ms-filetransfer+xml\r\n",
			   body,
			   ft_private->dialog,
			   NULL);

	g_free(body);

	return FALSE;
}

static void
mime_mixed_cb(gpointer user_data, const GSList *fields, const gchar *body,
	      gsize length)
{
	struct sipe_file_transfer_lync *ft_private = user_data;
	const gchar *ctype = sipe_utils_nameval_find(fields, "Content-Type");

	/* Lync 2010 file transfer */
	if (g_str_has_prefix(ctype, "application/ms-filetransfer+xml")) {
		sipe_xml *xml = sipe_xml_parse(body, length);
		const sipe_xml *node;

		const gchar *request_id_str = sipe_xml_attribute(xml, "requestId");
		if (request_id_str) {
			ft_private->request_id = atoi(request_id_str);
		}

		node = sipe_xml_child(xml, "publishFile/fileInfo/name");
		if (node) {
			ft_private->file_name = sipe_xml_data(node);
		}

		node = sipe_xml_child(xml, "publishFile/fileInfo/id");
		if (node) {
			ft_private->id = sipe_xml_data(node);
		}

		node = sipe_xml_child(xml, "publishFile/fileInfo/size");
		if (node) {
			gchar *size_str = sipe_xml_data(node);
			if (size_str) {
				ft_private->file_size = atoi(size_str);
				g_free(size_str);
			}
		}
	} else if (g_str_has_prefix(ctype, "application/sdp")) {
		ft_private->sdp = g_strndup(body, length);
	}
}

static void
candidate_pair_established_cb(struct sipe_media_call *call,
			      SIPE_UNUSED_PARAMETER struct sipe_media_stream *stream)
{
	request_download_file(sipe_media_get_file_transfer(call));
}

static void
read_cb(struct sipe_media_call *call, struct sipe_media_stream *stream)
{
	struct sipe_file_transfer_lync *ft_data =
			sipe_media_get_file_transfer(call);
	guint8 buffer[0x800];

	if (ft_data->was_cancelled) {
		// Just drop the incoming data.
		sipe_backend_media_read(call, stream, buffer, sizeof (buffer),
					FALSE);
		return;
	}

	if (ft_data->expecting_len == 0) {
		guint8 type;
		guint16 size;

		sipe_backend_media_read(call, stream, &type, sizeof (guint8),
					TRUE);
		sipe_backend_media_read(call, stream, (guint8 *)&size,
					sizeof (guint16), TRUE);
		size = GUINT16_FROM_BE(size);

		if (type == 0x01) {
			sipe_backend_media_read(call, stream, buffer, size,
						TRUE);
			buffer[size] = 0;
			SIPE_DEBUG_INFO("Received new stream for requestId : %s", buffer);
			sipe_backend_ft_start(&ft_data->public, NULL, NULL, 0);
		} else if (type == 0x02) {
			sipe_backend_media_read(call, stream, buffer, size,
						TRUE);
			buffer[size] = 0;

			SIPE_DEBUG_INFO("Received end of stream for requestId : %s", buffer);
			// TODO: finish transfer;
		} else if (type == 0x00) {
			SIPE_DEBUG_INFO("Received new data chunk of size %d", size);
			ft_data->expecting_len = size;
		}
		/* Readable will be called again so we can read the rest of
		 * the buffer or the chunk. */
	} else {
		guint len = MIN(ft_data->expecting_len, sizeof (buffer));
		len = sipe_backend_media_read(call, stream, buffer, len, FALSE);
		ft_data->expecting_len -= len;
		SIPE_DEBUG_INFO("Read %d bytes. %d remaining in chunk",
				len, ft_data->expecting_len);
		sipe_backend_ft_write_file(&ft_data->public, buffer, len);
	}
}

static void
ft_lync_incoming_init(struct sipe_file_transfer *ft,
		      SIPE_UNUSED_PARAMETER const gchar *filename,
		      SIPE_UNUSED_PARAMETER gsize size,
		      SIPE_UNUSED_PARAMETER const gchar *who)
{
	struct sipe_file_transfer_lync *ft_private =
			(struct sipe_file_transfer_lync *)ft;
	struct sipe_media_call *call =
			(struct sipe_media_call *)ft_private->sipe_private->media_call;

	if (call) {
		sipe_backend_media_accept(call->backend_private, TRUE);
	}
}

static void
call_reject_cb(struct sipe_media_call *call, gboolean local)
{
	struct sipe_file_transfer_lync *ft_private =
			sipe_media_get_file_transfer(call);

	if (ft_private->call_reject_parent_cb) {
		ft_private->call_reject_parent_cb(call, local);
	}

	if (!local) {
		sipe_backend_ft_cancel_remote(&ft_private->public);
	}
}

static gboolean
ft_lync_incoming_end(struct sipe_file_transfer *ft)
{
	struct sipe_file_transfer_lync *ft_private =
			(struct sipe_file_transfer_lync *)ft;

	static const gchar *FILETRANSFER_PROGRESS =
			"<notify xmlns=\"http://schemas.microsoft.com/rtc/2009/05/filetransfer\" notifyId=\"%d\">"
				"<fileTransferProgress>"
					"<transferId>%d</transferId>"
					"<bytesReceived>"
						"<from>0</from>"
						"<to>%d</to>"
					"</bytesReceived>"
				"</fileTransferProgress>"
			"</notify>";

	gchar *body = g_strdup_printf(FILETRANSFER_PROGRESS,
			       rand(),
			       ft_private->request_id,
			       ft_private->file_size - 1);

	sip_transport_info(ft_private->sipe_private,
			   "Content-Type: application/ms-filetransfer+xml\r\n",
			   body,
			   ft_private->dialog,
			   NULL);

	g_free(body);

	/* We still need our filetransfer structure so don't let backend
	 * deallocate it. */
	ft->deallocate = NULL;

	return TRUE;
}

static gboolean
request_cancelled_cb(struct sipe_core_private *sipe_private,
		     SIPE_UNUSED_PARAMETER struct sipmsg *msg,
		     SIPE_UNUSED_PARAMETER struct transaction *trans)
{
	struct sipe_file_transfer_lync *ft_private =
			sipe_media_get_file_transfer((struct sipe_media_call *)sipe_private->media_call);

	ft_lync_deallocate(SIPE_FILE_TRANSFER);

	return TRUE;
}

static gboolean
cancel_transfer_cb(struct sipe_core_private *sipe_private,
		   SIPE_UNUSED_PARAMETER struct sipmsg *msg,
		   SIPE_UNUSED_PARAMETER struct transaction *trans)
{
	struct sipe_file_transfer_lync *ft_private =
			sipe_media_get_file_transfer((struct sipe_media_call *)sipe_private->media_call);

	static const gchar *FILETRANSFER_CANCEL_RESPONSE =
			"<response xmlns=\"http://schemas.microsoft.com/rtc/2009/05/filetransfer\" requestId=\"%d\" code=\"failure\" reason=\"requestCancelled\"/>";
	gchar *body = g_strdup_printf(FILETRANSFER_CANCEL_RESPONSE,
				      ft_private->request_id);
	sip_transport_info(sipe_private,
			   "Content-Type: application/ms-filetransfer+xml\r\n",
			   body,
			   ft_private->dialog,
			   request_cancelled_cb);
	g_free(body);

	return TRUE;
}

static void
ft_lync_incoming_cancelled(struct sipe_file_transfer *ft, gboolean local)
{
	static const gchar *FILETRANSFER_CANCEL_REQUEST =
			"<request xmlns=\"http://schemas.microsoft.com/rtc/2009/05/filetransfer\" requestId=\"%d\"/>"
				"<cancelTransfer>"
					"<transferId>%d</transferId>"
					"<fileInfo>"
						"<id>%s</id>"
						"<name>%s</name>"
					"</fileInfo>"
				"</cancelTransfer>"
			"</request>";

	if (local) {
		gchar *body = g_strdup_printf(FILETRANSFER_CANCEL_REQUEST,
					      SIPE_FILE_TRANSFER_PRIVATE->request_id + 1,
					      SIPE_FILE_TRANSFER_PRIVATE->request_id,
					      SIPE_FILE_TRANSFER_PRIVATE->id,
					      SIPE_FILE_TRANSFER_PRIVATE->file_name);

		sip_transport_info(SIPE_FILE_TRANSFER_PRIVATE->sipe_private,
				   "Content-Type: application/ms-filetransfer+xml\r\n",
				   body,
				   SIPE_FILE_TRANSFER_PRIVATE->dialog,
				   cancel_transfer_cb);
		g_free(body);

		SIPE_FILE_TRANSFER_PRIVATE->was_cancelled = TRUE;
		/* We still need our filetransfer structure so don't let backend
		 * deallocate it. */
		ft->deallocate = NULL;
	}
}

static void
ft_lync_deallocate(struct sipe_file_transfer *ft)
{
	struct sipe_file_transfer_lync *ft_private =
			(struct sipe_file_transfer_lync *) ft;
	struct sipe_media_call *call =
			(struct sipe_media_call *)ft_private->sipe_private->media_call;
	if (call) {
		sipe_backend_media_hangup(call->backend_private, TRUE);
	}
	sipe_file_transfer_lync_free(ft_private);
}

void
process_incoming_invite_ft_lync(struct sipe_core_private *sipe_private,
				struct sipmsg *msg)
{
	struct sipe_file_transfer_lync *ft_private;
	struct sipe_media_call *call;
	struct sip_session *session = NULL;

	ft_private = g_new0(struct sipe_file_transfer_lync, 1);
	sipe_mime_parts_foreach(sipmsg_find_header(msg, "Content-Type"),
				msg->body, mime_mixed_cb, ft_private);

	if (!ft_private->file_name || !ft_private->file_size || !ft_private->sdp) {
		sip_transport_response(sipe_private, msg, 488, "Not Acceptable Here", NULL);
		sipe_file_transfer_lync_free(ft_private);
		return;
	}

	g_free(msg->body);
	msg->body = ft_private->sdp;
	msg->bodylen = strlen(msg->body);
	ft_private->sdp = NULL;

	process_incoming_invite_call(sipe_private, msg);

	call = (struct sipe_media_call *)sipe_private->media_call;
	call->candidate_pair_established_cb = candidate_pair_established_cb;
	call->read_cb = read_cb;

	ft_private->call_reject_parent_cb = call->call_reject_cb;
	call->call_reject_cb = call_reject_cb;

	ft_private->sipe_private = sipe_private;

	session = sipe_session_find_call(sipe_private, call->with);
	ft_private->dialog = session->dialogs->data;

	ft_private->public.init = ft_lync_incoming_init;
	ft_private->public.end = ft_lync_incoming_end;
	ft_private->public.cancelled = ft_lync_incoming_cancelled;
	ft_private->public.deallocate = ft_lync_deallocate;

	sipe_media_set_file_transfer(call, ft_private);

	sipe_backend_ft_incoming(SIPE_CORE_PUBLIC,
				 (struct sipe_file_transfer *)ft_private,
				 call->with, ft_private->file_name,
				 ft_private->file_size);
}

static void
process_response(struct sipe_file_transfer_lync *ft_private, sipe_xml *xml)
{
	guint request_id = atoi(sipe_xml_attribute(xml, "requestId"));
	const gchar *code;

	if (request_id != ft_private->request_id) {
		return;
	}

	code = sipe_xml_attribute(xml, "code");
	if (sipe_strequal(code, "success")) {
		/* Don't hang up the call ourselves, we'll receive BYE from the
		 * sender. */
		sipe_file_transfer_lync_free(ft_private);
	} else if (sipe_strequal(code, "failure")) {
		const gchar *reason = sipe_xml_attribute(xml, "reason");
		if (sipe_strequal(reason, "requestCancelled")) {
			sipe_backend_ft_cancel_remote(SIPE_FILE_TRANSFER);
		}
	}
}

void
process_incoming_info_ft_lync(struct sipe_core_private *sipe_private,
			      struct sipmsg *msg)
{
	struct sipe_file_transfer_lync *ft_private;
	sipe_xml *xml;

	if (!sipe_private->media_call) {
		return;
	}

	ft_private = sipe_media_get_file_transfer((struct sipe_media_call *)sipe_private->media_call);
	if (!ft_private) {
		return;
	}

	xml = sipe_xml_parse(msg->body, msg->bodylen);
	if (!xml) {
		return;
	}

	if (sipe_strequal(sipe_xml_name(xml), "response")) {
		process_response(ft_private, xml);
	}

	sipe_xml_free(xml);

	sip_transport_response(sipe_private, msg, 200, "OK", NULL);
}

/*
  Local Variables:
  mode: c
  c-file-style: "bsd"
  indent-tabs-mode: t
  tab-width: 8
  End:
*/
