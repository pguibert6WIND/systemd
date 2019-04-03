/* SPDX-License-Identifier: LGPL-2.1+ */
#pragma once

#include <linux/if_macsec.h>

#include "in-addr-util.h"
#include "netdev.h"
#include "networkd-util.h"
#include "sparse-endian.h"

typedef struct MACsec MACsec;

typedef union MACsecSCI {
        uint64_t as_uint64;

        struct {
                struct ether_addr mac;
                be16_t port;
        } _packed_;
} MACsecSCI;

assert_cc(sizeof(MACsecSCI) == sizeof(uint64_t));

typedef struct SecurityAssociation {
        uint8_t association_number;
        uint32_t packet_number;
        uint8_t key_id[MACSEC_KEYID_LEN];
        uint8_t *key;
        uint32_t key_len;
} SecurityAssociation;

typedef struct TransmitAssociation {
        MACsec *macsec;
        NetworkConfigSection *section;

        SecurityAssociation sa;
} TransmitAssociation;

typedef struct ReceiveAssociation {
        MACsec *macsec;
        NetworkConfigSection *section;

        MACsecSCI sci;
        SecurityAssociation sa;
} ReceiveAssociation;

typedef struct ReceiveChannel {
        MACsec *macsec;
        NetworkConfigSection *section;

        MACsecSCI sci;
} ReceiveChannel;

struct MACsec {
        NetDev meta;

        uint16_t port;
        int encrypt;

        OrderedHashmap *receive_channels_by_section;
        OrderedHashmap *transmit_associations_by_section;
        OrderedHashmap *receive_associations_by_section;
};

DEFINE_NETDEV_CAST(MACSEC, MACsec);
extern const NetDevVTable macsec_vtable;

CONFIG_PARSER_PROTOTYPE(config_parse_macsec_port);
CONFIG_PARSER_PROTOTYPE(config_parse_macsec_hw_address);
CONFIG_PARSER_PROTOTYPE(config_parse_macsec_packet_number);
CONFIG_PARSER_PROTOTYPE(config_parse_macsec_key_id);
CONFIG_PARSER_PROTOTYPE(config_parse_macsec_key);
