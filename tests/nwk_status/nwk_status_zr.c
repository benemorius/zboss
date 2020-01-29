/***************************************************************************
 *                      ZBOSS ZigBee Pro 2007 stack                         *
 *                                                                          *
 *          Copyright (c) 2012 DSR Corporation Denver CO, USA.              *
 *                       http://www.dsr-wireless.com                        *
 *                                                                          *
 *                            All rights reserved.                          *
 *          Copyright (c) 2011 ClarIDy Solutions, Inc., Taipei, Taiwan.     *
 *                       http://www.claridy.com/                            *
 *                                                                          *
 *          Copyright (c) 2011 Uniband Electronic Corporation (UBEC),       *
 *                             Hsinchu, Taiwan.                             *
 *                       http://www.ubec.com.tw/                            *
 *                                                                          *
 *          Copyright (c) 2011 DSR Corporation Denver CO, USA.              *
 *                       http://www.dsr-wireless.com                        *
 *                                                                          *
 *                            All rights reserved.                          *
 *                                                                          *
 *                                                                          *
 * ZigBee Pro 2007 stack, also known as ZBOSS (R) ZB stack is available     *
 * under either the terms of the Commercial License or the GNU General      *
 * Public License version 2.0.  As a recipient of ZigBee Pro 2007 stack, you*
 * may choose which license to receive this code under (except as noted in  *
 * per-module LICENSE files).                                               *
 *                                                                          *
 * ZBOSS is a registered trademark of DSR Corporation AKA Data Storage      *
 * Research LLC.                                                            *
 *                                                                          *
 * GNU General Public License Usage                                         *
 * This file may be used under the terms of the GNU General Public License  *
 * version 2.0 as published by the Free Software Foundation and appearing   *
 * in the file LICENSE.GPL included in the packaging of this file.  Please  *
 * review the following information to ensure the GNU General Public        *
 * License version 2.0 requirements will be met:                            *
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html.                   *
 *                                                                          *
 * Commercial Usage                                                         *
 * Licensees holding valid ClarIDy/UBEC/DSR Commercial licenses may use     *
 * this file in accordance with the ClarIDy/UBEC/DSR Commercial License     *
 * Agreement provided with the Software or, alternatively, in accordance    *
 * with the terms contained in a written agreement between you and          *
 * ClarIDy/UBEC/DSR.                                                        *
 *                                                                          *
 ****************************************************************************
   PURPOSE: Nwk status test
 */

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"

zb_ieee_addr_t g_ieee_addr = { 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00 };
static void zb_nwk_send_status(zb_uint8_t param) ZB_CALLBACK;

MAIN(){
    ARGV_UNUSED;

#ifndef KEIL
    if (argc < 3) {
        printf("%s <read pipe path> <write pipe path>\n", argv[0]);
        return 0;
    }
#endif

    /* Init device, load IB values from nvram or set it to default */
#ifndef ZB8051
    ZB_INIT("zdo_zr", argv[1], argv[2]);
#else
    ZB_INIT("zdo_zr", "2", "2");
#endif

    /* set ieee addr */
    ZB_IEEE_ADDR_COPY(ZB_PIB_EXTENDED_ADDRESS(), &g_ieee_addr);

#ifndef ZB_NS_BUILD
    ZB_UPDATE_LONGMAC();
#endif

    /* join as a router */
    ZB_NIB_DEVICE_TYPE() = ZB_NWK_DEVICE_TYPE_ROUTER;

#ifdef ZB_SECURITY
    /* turn off security */
    ZB_NIB_SECURITY_LEVEL() = 0;
#endif

    if (zdo_dev_start() != RET_OK) {
        TRACE_MSG(TRACE_ERROR, "zdo_dev_start failed", (FMT__0));
    }
    else {
        zdo_main_loop();
    }

    TRACE_DEINIT();

    MAIN_RETURN(0);
}


void zb_zdo_startup_complete(zb_uint8_t param) ZB_CALLBACK
{
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);

    if (buf->u.hdr.status == 0) {
        zb_nlme_route_discovery_request_t *rreq = ZB_GET_BUF_PARAM(buf,
                                                                   zb_nlme_route_discovery_request_t);

        TRACE_MSG(TRACE_ERROR, "Device STARTED OK", (FMT__0));

        rreq->address_mode = ZB_ADDR_16BIT_DEV_OR_BROADCAST;
        rreq->network_addr = 0xAAA0;
        rreq->radius = 1;
        rreq->no_route_cache = 0;

        /* send nwk status indication to the coordinator  */
        ZB_GET_OUT_BUF_DELAYED(zb_nwk_send_status);

        /* discover route to absent device to get nwk status indocation */
        ZB_SCHEDULE_CALLBACK(zb_nlme_route_discovery_request, param);
    }
    else {
        TRACE_MSG(TRACE_ERROR, "Device started FAILED status %d",
                  (FMT__D, (int)buf->u.hdr.status));
        zb_free_buf(buf);
    }
}

static void zb_nwk_send_status(zb_uint8_t param) ZB_CALLBACK
{
    zb_nlme_send_status_t *request = ZB_GET_BUF_PARAM(ZB_BUF_FROM_REF(
                                                          param),
                                                      zb_nlme_send_status_t);

    TRACE_MSG(TRACE_NWK1, ">> zb_nwk_send_status param %hd", (FMT__H, param));

    request->dest_addr = 0; /* send status indication to the coordinator */
    request->status.status = ZB_NWK_COMMAND_STATUS_LOW_BATTERY_LEVEL;
    request->status.network_addr = ZB_NIB_NETWORK_ADDRESS();
    request->ndsu_handle = 0;

    ZB_SCHEDULE_CALLBACK(zb_nlme_send_status, param);

    TRACE_MSG(TRACE_NWK1, "<< zb_nwk_send_status", (FMT__0));
}
