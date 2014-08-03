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
#include "sipe-ft-lync.h"
#include "sipe-media.h"
#include "sipe-mime.h"
#include "sipe-session.h"
#include "sipe-utils.h"
#include "sipe-xml.h"
#include "sipmsg.h"

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
			       ft_private->request_id + 1,
			       ft_private->id,
			       ft_private->file_name);

	sip_transport_info(ft_private->sipe_private,
			   "Content-Type: application/ms-filetransfer+xml\r\n",
			   body,
			   ft_private->dialog,
			   NULL);

	g_free(body);

	sipe_file_transfer_lync_free(ft_private);

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

void
process_incoming_invite_ft_lync(struct sipe_core_private *sipe_private,
				struct sipmsg *msg)
{
	struct sipe_file_transfer_lync *ft_private;
	gchar *from;
	struct sip_session *session;

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

	from = parse_from(sipmsg_find_header(msg, "From"));
	session = sipe_session_find_call(sipe_private, from);
	g_free(from);

	ft_private->dialog = session->dialogs->data;
	ft_private->sipe_private = sipe_private;

	g_timeout_add_seconds(10, (GSourceFunc)request_download_file, (gpointer)ft_private);
}

/*
  Local Variables:
  mode: c
  c-file-style: "bsd"
  indent-tabs-mode: t
  tab-width: 8
  End:
*/
