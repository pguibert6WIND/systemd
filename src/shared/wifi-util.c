/* SPDX-License-Identifier: LGPL-2.1-or-later */

#include "log.h"
#include "string-table.h"
#include "string-util.h"
#include "wifi-util.h"

int wifi_get_interface(sd_netlink *genl, int ifindex, enum nl80211_iftype *ret_iftype, char **ret_ssid) {
        _cleanup_(sd_netlink_message_unrefp) sd_netlink_message *m = NULL, *reply = NULL;
        _cleanup_free_ char *ssid = NULL;
        const char *family;
        uint32_t iftype;
        size_t len;
        int r;

        assert(genl);
        assert(ifindex > 0);

        r = sd_genl_message_new(genl, NL80211_GENL_NAME, NL80211_CMD_GET_INTERFACE, &m);
        if (r < 0)
                return log_debug_errno(r, "Failed to create generic netlink message: %m");

        r = sd_netlink_message_append_u32(m, NL80211_ATTR_IFINDEX, ifindex);
        if (r < 0)
                return log_debug_errno(r, "Could not append NL80211_ATTR_IFINDEX attribute: %m");

        r = sd_netlink_call(genl, m, 0, &reply);
        if (r == -ENODEV) {
                /* For obsolete WEXT driver. */
                log_debug_errno(r, "Failed to request information about wifi interface %d. "
                                "The device doesn't seem to have nl80211 interface. Ignoring.",
                                ifindex);
                goto nodata;
        }
        if (r < 0)
                return log_debug_errno(r, "Failed to request information about wifi interface %d: %m", ifindex);
        if (!reply) {
                log_debug("No reply received to request for information about wifi interface %d, ignoring.", ifindex);
                goto nodata;
        }

        r = sd_netlink_message_get_errno(reply);
        if (r < 0)
                return log_debug_errno(r, "Failed to get information about wifi interface %d: %m", ifindex);

        r = sd_genl_message_get_family_name(genl, reply, &family);
        if (r < 0)
                return log_debug_errno(r, "Failed to determine genl family: %m");
        if (!streq(family, NL80211_GENL_NAME)) {
                log_debug("Received message of unexpected genl family '%s', ignoring.", family);
                goto nodata;
        }

        r = sd_netlink_message_read_u32(reply, NL80211_ATTR_IFTYPE, &iftype);
        if (r < 0)
                return log_debug_errno(r, "Failed to get NL80211_ATTR_IFTYPE attribute: %m");

        r = sd_netlink_message_read_data_suffix0(reply, NL80211_ATTR_SSID, &len, (void**) &ssid);
        if (r < 0 && r != -ENODATA)
                return log_debug_errno(r, "Failed to get NL80211_ATTR_SSID attribute: %m");
        if (r >= 0) {
                if (len == 0) {
                        log_debug("SSID has zero length, ignoring the received SSID.");
                        ssid = mfree(ssid);
                } else if (strlen_ptr(ssid) != len) {
                        log_debug("SSID contains NUL character(s), ignoring the received SSID.");
                        ssid = mfree(ssid);
                }
        }

        if (ret_iftype)
                *ret_iftype = iftype;

        if (ret_ssid)
                *ret_ssid = TAKE_PTR(ssid);

        return 1;

nodata:
        if (ret_iftype)
                *ret_iftype = 0;
        if (ret_ssid)
                *ret_ssid = NULL;
        return 0;
}

int wifi_get_station(sd_netlink *genl, int ifindex, struct ether_addr *ret_bssid) {
        _cleanup_(sd_netlink_message_unrefp) sd_netlink_message *m = NULL, *reply = NULL;
        const char *family;
        int r;

        assert(genl);
        assert(ifindex > 0);
        assert(ret_bssid);

        r = sd_genl_message_new(genl, NL80211_GENL_NAME, NL80211_CMD_GET_STATION, &m);
        if (r < 0)
                return log_debug_errno(r, "Failed to create generic netlink message: %m");

        r = sd_netlink_message_set_flags(m, NLM_F_REQUEST | NLM_F_ACK | NLM_F_DUMP);
        if (r < 0)
                return log_debug_errno(r, "Failed to set dump flag: %m");

        r = sd_netlink_message_append_u32(m, NL80211_ATTR_IFINDEX, ifindex);
        if (r < 0)
                return log_debug_errno(r, "Could not append NL80211_ATTR_IFINDEX attribute: %m");

        r = sd_netlink_call(genl, m, 0, &reply);
        if (r < 0)
                return log_debug_errno(r, "Failed to request information about wifi station: %m");
        if (!reply) {
                log_debug("No reply received to request for information about wifi station, ignoring.");
                goto nodata;
        }

        r = sd_netlink_message_get_errno(reply);
        if (r < 0)
                return log_debug_errno(r, "Failed to get information about wifi station: %m");

        r = sd_genl_message_get_family_name(genl, reply, &family);
        if (r < 0)
                return log_debug_errno(r, "Failed to determine genl family: %m");
        if (!streq(family, NL80211_GENL_NAME)) {
                log_debug("Received message of unexpected genl family '%s', ignoring.", family);
                goto nodata;
        }

        r = sd_netlink_message_read_ether_addr(reply, NL80211_ATTR_MAC, ret_bssid);
        if (r == -ENODATA)
                goto nodata;
        if (r < 0)
                return log_debug_errno(r, "Failed to get NL80211_ATTR_MAC attribute: %m");

        return 1;

nodata:
        *ret_bssid = ETHER_ADDR_NULL;
        return 0;
}

static const char * const nl80211_iftype_table[NUM_NL80211_IFTYPES] = {
        [NL80211_IFTYPE_ADHOC]      = "ad-hoc",
        [NL80211_IFTYPE_STATION]    = "station",
        [NL80211_IFTYPE_AP]         = "ap",
        [NL80211_IFTYPE_AP_VLAN]    = "ap-vlan",
        [NL80211_IFTYPE_WDS]        = "wds",
        [NL80211_IFTYPE_MONITOR]    = "monitor",
        [NL80211_IFTYPE_MESH_POINT] = "mesh-point",
        [NL80211_IFTYPE_P2P_CLIENT] = "p2p-client",
        [NL80211_IFTYPE_P2P_GO]     = "p2p-go",
        [NL80211_IFTYPE_P2P_DEVICE] = "p2p-device",
        [NL80211_IFTYPE_OCB]        = "ocb",
        [NL80211_IFTYPE_NAN]        = "nan",
};

DEFINE_STRING_TABLE_LOOKUP_TO_STRING(nl80211_iftype, enum nl80211_iftype);
