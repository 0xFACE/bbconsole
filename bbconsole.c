/*
 *
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2011  Nokia Corporation
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <errno.h>
#include <glib.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "lib/bluetooth.h"
#include "lib/bluetooth/sdp.h"
#include "lib/uuid.h"
#include "lib/mgmt.h"
#include "src/shared/mgmt.h"

#include <btio/btio.h>
#include "att.h"
#include "gattrib.h"
#include "gatt.h"
#include "gatttool.h"

#define IO_CAPABILITY_NOINPUTNOOUTPUT   0x03

static GIOChannel *iochannel = NULL;
static GAttrib *attrib = NULL;
static GMainLoop *event_loop;

static gchar *opt_src = NULL;
//static gchar *opt_dst = NULL;
static gchar *opt_dst_type = NULL;
static gchar *opt_sec_level = NULL;
static const int opt_psm = 0;
static int opt_mtu = 0;

static uint16_t mgmt_ind = MGMT_INDEX_NONE;
static struct mgmt *mgmt_master = NULL;

struct characteristic_data {
    uint16_t orig_start;
    uint16_t start;
    uint16_t end;
    bt_uuid_t uuid;
};

static void send_data(const unsigned char *val, size_t len)
{
  while ( len-- > 0 )
    printf("%c", *val++);
}

static void events_handler(const uint8_t *pdu, uint16_t len, gpointer user_data)
{
    uint8_t *opdu;
    uint8_t evt;
    uint16_t olen;
    size_t plen;

    evt = pdu[0];

    if ( evt != ATT_OP_HANDLE_NOTIFY && evt != ATT_OP_HANDLE_IND )
    {
        printf("#Invalid opcode %02X in event handler??\n", evt);
        return;
    }

    assert( len >= 3 );
    bt_get_le16(&pdu[1]);

    send_data( pdu+3, len-3 );

    if (evt == ATT_OP_HANDLE_NOTIFY)
        return;

    opdu = g_attrib_get_buffer(attrib, &plen);
    olen = enc_confirmation(opdu, plen);

    if (olen > 0)
        g_attrib_send(attrib, 0, opdu, olen, NULL, NULL, NULL);
}

static void connect_cb(GIOChannel *io, GError *err, gpointer user_data)
{
    uint16_t mtu;
    uint16_t cid;
    GError *gerr = NULL;
    uint8_t *value;

    if (err) {
        printf("# Connect error: %s\n", err->message);
        g_main_loop_quit(event_loop);
        return;
    }

    bt_io_get(io, &gerr, BT_IO_OPT_IMTU, &mtu,
                BT_IO_OPT_CID, &cid, BT_IO_OPT_INVALID);

    if (gerr) {
        printf("# Can't detect MTU, using default");
        g_error_free(gerr);
        mtu = ATT_DEFAULT_LE_MTU;
    }
    else if (cid == ATT_CID)
        mtu = ATT_DEFAULT_LE_MTU;

    attrib = g_attrib_new(iochannel, mtu, false);

    g_attrib_register(attrib, ATT_OP_HANDLE_NOTIFY, GATTRIB_ALL_HANDLES,
                        events_handler, attrib, NULL);
    g_attrib_register(attrib, ATT_OP_HANDLE_IND, GATTRIB_ALL_HANDLES,
                        events_handler, attrib, NULL);
  printf("Connected.\n");
  //Enabling notifying
  gatt_attr_data_from_string("0100", &value);
  gatt_write_cmd(attrib, 0x002f, value, 2, NULL, NULL);
}

static void disconnect_io()
{
    g_attrib_unref(attrib);
    attrib = NULL;
    opt_mtu = 0;

    g_io_channel_shutdown(iochannel, FALSE, NULL);
    g_io_channel_unref(iochannel);
    iochannel = NULL;

    printf("Disconnect_IO");
}

static gboolean channel_watcher(GIOChannel *chan, GIOCondition cond,
                gpointer user_data)
{
    if (chan == iochannel)
        disconnect_io();

    return FALSE;
}

static void parse_line(char *line_read)
{
    int i, c, str_len, chunks;
    size_t plen;
    uint8_t *value;
    line_read = g_strstrip(line_read);
	str_len = strlen(line_read);
    chunks = (str_len / 10);
    char  outword[20];
    for(c = 0; c<chunks; c++){
        for(i = 0; i<10; i++){
            sprintf(outword+i*2, "%02X", line_read[10*c+i]);
        }
        plen = gatt_attr_data_from_string(outword, &value);
        gatt_write_cmd(attrib, 0x0031, value, plen, NULL, NULL);
    }
// remainder
    for(i = 0; i<str_len%10; i++){
        sprintf(outword+i*2, "%02X", line_read[10*c+i]);
    }       
        sprintf(outword+i*2, "%02X", 13); // CR
        plen = gatt_attr_data_from_string(outword, &value);
        gatt_write_cmd(attrib, 0x0031, value, plen, NULL, NULL);
}

static gboolean prompt_read(GIOChannel *chan, GIOCondition cond,
                            gpointer user_data)
{
    gchar *myline;

    if (cond & (G_IO_HUP | G_IO_ERR | G_IO_NVAL)) {
        g_main_loop_quit(event_loop);
        return FALSE;
    }

    if ( G_IO_STATUS_NORMAL != g_io_channel_read_line(chan, &myline, NULL, NULL, NULL)
            || myline == NULL
    )
    {
        g_main_loop_quit(event_loop);
        return FALSE;
    }

    parse_line(myline);
    return TRUE;
}

int main(int argc, char *argv[])
{

    GIOChannel *pchan;
    gint events;

    opt_sec_level = g_strdup("low");

    opt_src = NULL;
    //opt_dst = NULL;
    opt_dst_type = g_strdup("public");

    event_loop = g_main_loop_new(NULL, FALSE);

    pchan = g_io_channel_unix_new(fileno(stdin));
    g_io_channel_set_close_on_unref(pchan, TRUE);
    events = G_IO_IN | G_IO_ERR | G_IO_HUP | G_IO_NVAL;
    g_io_add_watch(pchan, events, prompt_read, NULL);
    GError *gerr = NULL;
    printf("Connecting to.. %s\n", argv[1]);
    iochannel = gatt_connect(opt_src, argv[1], opt_dst_type, opt_sec_level, opt_psm, opt_mtu, connect_cb, &gerr);
       if (iochannel == NULL)
    {
        g_error_free(gerr);
        }
    else
        g_io_add_watch(iochannel, G_IO_HUP, channel_watcher, NULL);
    g_main_loop_run(event_loop);

    fflush(stdout);
    g_io_channel_unref(pchan);
    g_main_loop_unref(event_loop);

    g_free(opt_src);
    //g_free(opt_dst);
    g_free(opt_sec_level);

    mgmt_unregister_index(mgmt_master, mgmt_ind);
    mgmt_cancel_index(mgmt_master, mgmt_ind);
    mgmt_unref(mgmt_master);
    mgmt_master = NULL;

    return EXIT_SUCCESS;
}