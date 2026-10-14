/*
 * Copyright (C) The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */



#include <strings.h>

#include "cneqmiutils.h"

char const *qmiPorts[47] = {
    "rmnet0",
    "rmnet1",
    "rmnet2",
    "rmnet3",
    "rmnet4",
    "rmnet5",
    "rmnet6",
    "rmnet7",
    "rmnet8",
    "rmnet9",
    "rmnet10",
    "rmnet11",
    "rmnet_sdio0",
    "rmnet_sdio1",
    "rmnet_sdio2",
    "rmnet_sdio3",
    "rmnet_sdio4",
    "rmnet_sdio5",
    "rmnet_sdio6",
    "rmnet_sdio7",
    "rmnet_usb0",
    "rmnet_usb1",
    "rmnet_usb2",
    "rmnet_usb3",
    "rmnet_usb4",
    "rmnet_usb5",
    "rmnet_usb6",
    "rmnet_usb7",
    "rmnet_mhi0",
    "rmnet_mhi1",
    "rmnet_data0",
    "rmnet_data1",
    "rmnet_data2",
    "rmnet_data3",
    "rmnet_data4",
    "rmnet_data5",
    "rmnet_data6",
    "rmnet_data7",
    "rmnet_smux0",
    "rmnet2_usb0",
    "rmnet2_usb1",
    "rmnet2_usb2",
    "rmnet2_usb3",
    "rmnet2_usb4",
    "rmnet2_usb5",
    "rmnet2_usb6",
    "rmnet2_usb7",
};

bool CneQmiUtils::isQmiPort(char const *ifname) {
    if (ifname == nullptr) {
        return false;
    }
    for (unsigned i = 0; i < 47; i++) {
        if (strcasecmp(ifname, qmiPorts[i]) == 0) {
            return true;
        }
    }
    return false;
}
