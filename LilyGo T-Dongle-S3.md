modification to use sdcard:4bit with the LilyGo T-Dongle-S3

micropython/ports/esp32/boards/GENERIC_S3/mpconfigboard.h
add this:
#define MICROPY_HW_ENABLE_SDCARD            (1)
#define MICROPY_HW_SDCARD_SDMMC             (1)
#define MICROPY_HW_SDCARD_SDSPI             (0)
#define MICROPY_HW_SDCARD_BUS_WIDTH         (4)
#define MICROPY_HW_SDMMC_D0                 (GPIO_NUM_14)
#define MICROPY_HW_SDMMC_D1                 (GPIO_NUM_17)
#define MICROPY_HW_SDMMC_D2                 (GPIO_NUM_21)
#define MICROPY_HW_SDMMC_D3                 (GPIO_NUM_18)
#define MICROPY_HW_SDMMC_CK                 (GPIO_NUM_12)
#define MICROPY_HW_SDMMC_CMD                (GPIO_NUM_16)

micropython/ports/esp32/machine_sdcard.c
add this:
        // SD/MMC interface
        DEBUG_printf("  Setting up SDMMC slot configuration");
        sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
#ifdef MICROPY_HW_SDCARD_SDMMC
		slot_config.clk = MICROPY_HW_SDMMC_CK;
		slot_config.cmd = MICROPY_HW_SDMMC_CMD;
		slot_config.d0 = MICROPY_HW_SDMMC_D0;
		slot_config.d1 = MICROPY_HW_SDMMC_D1;
		slot_config.d2 = MICROPY_HW_SDMMC_D2;
		slot_config.d3 = MICROPY_HW_SDMMC_D3;
		slot_config.width = 4; // SDMMC_SLOT_WIDTH_DEFAULT,
		slot_config.flags = SDMMC_SLOT_FLAG_INTERNAL_PULLUP;
		gpio_set_pull_mode((gpio_num_t)MICROPY_HW_SDMMC_CMD, GPIO_PULLUP_ONLY); // CMD, needed in 4- and 1- line modes
		gpio_set_pull_mode((gpio_num_t)MICROPY_HW_SDMMC_D0, GPIO_PULLUP_ONLY);  // D0, needed in 4- and 1-line modes
		gpio_set_pull_mode((gpio_num_t)MICROPY_HW_SDMMC_D1, GPIO_PULLUP_ONLY);  // D1, needed in 4-line mode only
		gpio_set_pull_mode((gpio_num_t)MICROPY_HW_SDMMC_D2, GPIO_PULLUP_ONLY);  // D2, needed in 4-line mode only
		gpio_set_pull_mode((gpio_num_t)MICROPY_HW_SDMMC_D3, GPIO_PULLUP_ONLY);  // D3, needed in 4- and 1-line modes
#endif
