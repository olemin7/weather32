#pragma once

namespace blink {
    enum
    {
        BLINK_FACTORY_RESET, /*!< restoring factory settings */
        BLINK_UPDATING,      /*!< updating software */
        BLINK_CONNECTED,     /*!< connected to AP (or Cloud) succeed */
        BLINK_PROVISIONED,   /*!< provision done */
        BLINK_CONNECTING,    /*!< connecting to AP (or Cloud) */
        BLINK_RECONNECTING,  /*!< reconnecting to AP (or Cloud), if lose connection */
        BLINK_PROVISIONING,  /*!< provisioning */
        MAX_
    };

    void init();
    void start(int state);
    void stop(int state);

}; // namespace blink