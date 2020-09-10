/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_kscan_composite

#include <device.h>
#include <drivers/kscan.h>
#include <logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#define MATRIX_NODE_ID DT_DRV_INST(0)
#define MATRIX_ROWS DT_PROP(MATRIX_NODE_ID, rows)
#define MATRIX_COLS DT_PROP(MATRIX_NODE_ID, columns)

struct kscan_composite_child_config
{
    char *label;
    u8_t row_offset;
    u8_t column_offset;
};

#define CHILD_CONFIG(inst)                          \
    {                                               \
        .label = DT_LABEL(DT_PHANDLE(inst, kscan)), \
        .row_offset = DT_PROP(inst, row_offset),    \
        .column_offset = DT_PROP(inst, column_offset)},

const struct kscan_composite_child_config kscan_composite_children[] = {
    DT_FOREACH_CHILD(MATRIX_NODE_ID, CHILD_CONFIG)};

struct kscan_composite_config
{
};

struct kscan_composite_data
{
    kscan_callback_t callback;

    struct device *dev;
};

static int kscan_composite_enable_callback(struct device *dev)
{
    for (int i = 0; i < sizeof(kscan_composite_children) / sizeof(kscan_composite_children[0]); i++)
    {
        const struct kscan_composite_child_config *cfg = &kscan_composite_children[i];

        kscan_enable_callback(device_get_binding(cfg->label));
    }
    return 0;
}

static int kscan_composite_disable_callback(struct device *dev)
{
    for (int i = 0; i < sizeof(kscan_composite_children) / sizeof(kscan_composite_children[0]); i++)
    {
        const struct kscan_composite_child_config *cfg = &kscan_composite_children[i];

        kscan_disable_callback(device_get_binding(cfg->label));
    }
    return 0;
}

static void kscan_composite_child_callback(struct device *child_dev, u32_t row, u32_t column, bool pressed)
{
    // TODO: Ideally we can get this passed into our callback!
    struct device *dev = device_get_binding(DT_INST_LABEL(0));
    struct kscan_composite_data *data = dev->driver_data;

    for (int i = 0; i < sizeof(kscan_composite_children) / sizeof(kscan_composite_children[0]); i++)
    {
        const struct kscan_composite_child_config *cfg = &kscan_composite_children[i];

        if (device_get_binding(cfg->label) != child_dev)
        {
            continue;
        }

        data->callback(dev, row + cfg->row_offset, column + cfg->column_offset, pressed);
    }
}

static int kscan_composite_configure(struct device *dev, kscan_callback_t callback)
{
    struct kscan_composite_data *data = dev->driver_data;

    if (!callback)
    {
        return -EINVAL;
    }

    for (int i = 0; i < sizeof(kscan_composite_children) / sizeof(kscan_composite_children[0]); i++)
    {
        const struct kscan_composite_child_config *cfg = &kscan_composite_children[i];

        kscan_config(device_get_binding(cfg->label), &kscan_composite_child_callback);
    }

    data->callback = callback;

    return 0;
}

static int kscan_composite_init(struct device *dev)
{
    struct kscan_composite_data *data = dev->driver_data;

    data->dev = dev;

    return 0;
}

static const struct kscan_driver_api mock_driver_api = {
    .config = kscan_composite_configure,
    .enable_callback = kscan_composite_enable_callback,
    .disable_callback = kscan_composite_disable_callback,
};

static const struct kscan_composite_config kscan_composite_config = {};

static struct kscan_composite_data kscan_composite_data;

DEVICE_AND_API_INIT(kscan_composite, DT_INST_LABEL(0), kscan_composite_init,
                    &kscan_composite_data,
                    &kscan_composite_config,
                    APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
                    &mock_driver_api);
