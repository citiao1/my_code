#ifndef APP_CONFIG_H
#define APP_CONFIG_H

/* R25 diagnostic build: remove the 88 KiB HLOG/WLOG capture buffer.
 * Set to 1 and rebuild to restore stopped RAM capture/download. */
#ifndef APP_ENABLE_TUNING_LOG
#define APP_ENABLE_TUNING_LOG 0
#endif

#endif
