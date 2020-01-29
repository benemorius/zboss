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
   PURPOSE: TP/PRO/BV-35 Frequency Agility Network - Channel Manager - ZC
   Verify that the DUT acting as a ZC can operate as default Network Channel
   Manager, router side.
 */

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"

/* For NS build first ieee addr byte should be unique */
#ifdef ZB_NS_BUILD
zb_ieee_addr_t g_ieee_addr = { 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00 };
#else
zb_ieee_addr_t g_ieee_addr = { 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00 };
#endif

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

    /* join as a router */
    ZB_NIB_DEVICE_TYPE() = ZB_NWK_DEVICE_TYPE_ROUTER;

#ifndef ZB_NS_BUILD
    ZB_UPDATE_LONGMAC();
#endif

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

void get_nwk_manager_cb(zb_uint8_t param)
{
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);
    zb_uint8_t *zdp_cmd = ZB_BUF_BEGIN(buf);
    zb_zdo_system_server_discovery_resp_t *resp =
        (zb_zdo_system_server_discovery_resp_t *)(zdp_cmd);

    if (resp->status == ZB_ZDP_STATUS_SUCCESS &&
        resp->server_mask & ZB_NETWORK_MANAGER) {
        TRACE_MSG(TRACE_APS2, "system_server_discovery received, status: OK",
                  (FMT__0));
    }
    else {
        TRACE_MSG(TRACE_ERROR,
                  "ERROR receiving system_server_discovery status %x, mask %x",
                  (FMT__D_D, resp->status, resp->server_mask));
    }
    zb_free_buf(buf);
}

void get_nwk_manager()
{
    zb_buf_t *asdu;

    asdu = zb_get_out_buf();
    if (!asdu) {
        TRACE_MSG(TRACE_ERROR, "out buf alloc failed!", (FMT__0));
    }
    else {
        zb_zdo_system_server_discovery_param_t *req_param;

        req_param = ZB_GET_BUF_PARAM(asdu,
                                     zb_zdo_system_server_discovery_param_t);
        req_param->server_mask = ZB_NETWORK_MANAGER;

        zb_zdo_system_server_discovery_req(ZB_REF_FROM_BUF(
                                               asdu), get_nwk_manager_cb);
    }

}

void zb_zdo_startup_complete(zb_uint8_t param) ZB_CALLBACK
{
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);

    if (buf->u.hdr.status == 0) {
        TRACE_MSG(TRACE_ERROR, "Device STARTED OK", (FMT__0));
        get_nwk_manager();
    }
    else {
        TRACE_MSG(TRACE_ERROR, "Device started FAILED status %d",
                  (FMT__D, (int)buf->u.hdr.status));
        zb_free_buf(buf);
    }
}
