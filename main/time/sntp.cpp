#include "sntp.hpp"
#include <time.h>
#include "esp_log.h"
#include "sdkconfig.h"
#include "esp_netif_sntp.h"
#include "lwip/ip_addr.h"
#include "esp_sntp.h"

#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN 48
#endif

namespace sntp
{
        static const char *TAG = "sntp";
        std::function<void()> onsync_cb_;

        void time_sync_notification_cb(struct timeval *tv)
        {
                ESP_LOGI(TAG, "got Epoch = %lld", tv->tv_sec);
                if (onsync_cb_)
                {
                        onsync_cb_();
                }
        }

        static void print_servers(void)
        {
                ESP_LOGI(TAG, "NTP servers:");

                for (uint8_t i = 0; i < SNTP_MAX_SERVERS; ++i)
                {
                        if (esp_sntp_getservername(i))
                        {
                                ESP_LOGI(TAG, "%d: %s", i, esp_sntp_getservername(i));
                        }
                        else
                        {
                                // we have either IPv4 or IPv6 address, let's print it
                                char buff[INET6_ADDRSTRLEN];
                                ip_addr_t const *ip = esp_sntp_getserver(i);
                                if (ipaddr_ntoa_r(ip, buff, INET6_ADDRSTRLEN) != NULL)
                                        ESP_LOGI(TAG, "server %d: %s", i, buff);
                        }
                }
        }

        void init(std::function<void()> &&onsync_cb)
        {
                ESP_LOGI(TAG, "init");
                onsync_cb_ = std::move(onsync_cb);
                /**
                 * NTP server address could be acquired via DHCP,
                 * see following menuconfig options:
                 * 'LWIP_DHCP_GET_NTP_SRV' - enable STNP over DHCP
                 * 'LWIP_SNTP_DEBUG' - enable debugging messages
                 *
                 * NOTE: This call should be made BEFORE esp acquires IP address from DHCP,
                 * otherwise NTP option would be rejected by default.
                 */
                ESP_LOGI(TAG, "Initializing SNTP");
                esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG(CONFIG_SNTP_TIME_SERVER);
                config.start = false;                     // start SNTP service explicitly (after connecting)
                config.server_from_dhcp = true;           // accept NTP offers from DHCP server, if any (need to enable *before* connecting)
                config.renew_servers_after_new_IP = true; // let esp-netif update configured SNTP server(s) after receiving DHCP lease
                config.index_of_first_server = 1;         // updates from server num 1, leaving server 0 (from DHCP) intact
                                                          // configure the event on which we renew servers
                config.ip_event_to_renew = IP_EVENT_ETH_GOT_IP;
                config.sync_cb = time_sync_notification_cb; // only if we need the notification function
                esp_netif_sntp_init(&config);

                print_servers();
        }

        void start()
        {
                ESP_LOGI(TAG, "Starting/Restarting SNTP");
                esp_netif_sntp_start();
        }

}
