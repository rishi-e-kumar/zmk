/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_ext_power_generic

#include <stdio.h>
#include <device.h>
#include <init.h>
#include <kernel.h>
#include <settings/settings.h>
#include <drivers/gpio.h>
#include <drivers/ext_power.h>

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

#include <logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct ext_power_generic_config {
    const char *label;
    const u8_t pin;
    const u8_t flags;
};

struct ext_power_generic_data {
    struct device *gpio;
    bool status;
#if IS_ENABLED(CONFIG_SETTINGS)
    bool settings_init;
#endif
};

#if IS_ENABLED(CONFIG_SETTINGS)
static void ext_power_save_state_work(struct k_work *work) {
    char setting_path[40];
    struct device *ext_power = device_get_binding(DT_INST_LABEL(0));
    struct ext_power_generic_data *data = ext_power->driver_data;

    snprintf(setting_path, 40, "ext_power/state/%s", DT_INST_LABEL(0));
    settings_save_one(setting_path, &data->status, sizeof(data->status));
}

static struct k_delayed_work ext_power_save_work;
#endif

int ext_power_save_state() {
#if IS_ENABLED(CONFIG_SETTINGS)
    k_delayed_work_cancel(&ext_power_save_work);
    return k_delayed_work_submit(&ext_power_save_work, K_MSEC(CONFIG_ZMK_SETTINGS_SAVE_DEBOUNCE));
#else
    return 0;
#endif
}

static int ext_power_generic_enable(struct device *dev) {
    struct ext_power_generic_data *data = dev->driver_data;
    const struct ext_power_generic_config *config = dev->config_info;

    if (gpio_pin_set(data->gpio, config->pin, 1)) {
        LOG_WRN("Failed to set ext-power control pin");
        return -EIO;
    }
    data->status = true;
    return ext_power_save_state();
}

static int ext_power_generic_disable(struct device *dev) {
    struct ext_power_generic_data *data = dev->driver_data;
    const struct ext_power_generic_config *config = dev->config_info;

    if (gpio_pin_set(data->gpio, config->pin, 0)) {
        LOG_WRN("Failed to clear ext-power control pin");
        return -EIO;
    }
    data->status = false;
    return ext_power_save_state();
}

static int ext_power_generic_get(struct device *dev) {
    struct ext_power_generic_data *data = dev->driver_data;
    return data->status;
}

#if IS_ENABLED(CONFIG_SETTINGS)
static int ext_power_settings_set(const char *name, size_t len, settings_read_cb read_cb,
                                  void *cb_arg) {
    const char *next;
    int rc;

    if (settings_name_steq(name, DT_INST_LABEL(0), &next) && !next) {
        struct device *ext_power = device_get_binding(DT_INST_LABEL(0));
        struct ext_power_generic_data *data = ext_power->driver_data;

        if (len != sizeof(data->status)) {
            return -EINVAL;
        }

        rc = read_cb(cb_arg, &data->status, sizeof(data->status));
        if (rc >= 0) {
            data->settings_init = true;

            if (ext_power == NULL) {
                LOG_ERR("Unable to retrieve ext_power device: %s", DT_INST_LABEL(0));
                return -EIO;
            }

            if (data->status) {
                ext_power_generic_enable(ext_power);
            } else {
                ext_power_generic_disable(ext_power);
            }

            return 0;
        }
        return rc;
    }
    return -ENOENT;
}

struct settings_handler ext_power_conf = {.name = "ext_power/state",
                                          .h_set = ext_power_settings_set};
#endif

static int ext_power_generic_init(struct device *dev) {
    struct ext_power_generic_data *data = dev->driver_data;
    const struct ext_power_generic_config *config = dev->config_info;

    data->gpio = device_get_binding(config->label);
    if (data->gpio == NULL) {
        LOG_ERR("Failed to get ext-power control device");
        return -EINVAL;
    }

    if (gpio_pin_configure(data->gpio, config->pin, config->flags | GPIO_OUTPUT)) {
        LOG_ERR("Failed to configure ext-power control pin");
        return -EIO;
    }

#if IS_ENABLED(CONFIG_SETTINGS)
    settings_register(&ext_power_conf);
    k_delayed_work_init(&ext_power_save_work, ext_power_save_state_work);

    // Set default value (on) if settings isn't set
    settings_load_subtree("ext_power");
    if (!data->settings_init) {

        data->status = true;
        k_delayed_work_submit(&ext_power_save_work, K_NO_WAIT);

        ext_power_enable(dev);
    }
#else
    // Default to the ext_power being open when no settings
    ext_power_enable(dev);
#endif

    return 0;
}

static const struct ext_power_generic_config config = {
    .label = DT_INST_GPIO_LABEL(0, control_gpios),
    .pin = DT_INST_GPIO_PIN(0, control_gpios),
    .flags = DT_INST_GPIO_FLAGS(0, control_gpios)};

static struct ext_power_generic_data data = {
    .status = false,
#if IS_ENABLED(CONFIG_SETTINGS)
    .settings_init = false,
#endif
};

static const struct ext_power_api api = {.enable = ext_power_generic_enable,
                                         .disable = ext_power_generic_disable,
                                         .get = ext_power_generic_get};

DEVICE_AND_API_INIT(ext_power_generic, DT_INST_LABEL(0), ext_power_generic_init, &data, &config,
                    APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY, &api);

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */