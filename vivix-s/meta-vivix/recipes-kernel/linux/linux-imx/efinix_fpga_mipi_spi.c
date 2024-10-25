/*
 *
 * Copyright (C) 2022 Vieworks, Inc. All Rights Reserved.
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/ctype.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/of_device.h>
#include <linux/spi/spi.h>
#include <linux/pinctrl/consumer.h>
#include <linux/regulator/consumer.h>
#include <linux/v4l2-mediabus.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ctrls.h>

#define VW_FUNCALL
#ifdef VW_FUNCALL
#define vw_funcall(fmt, ...) \
        printk(KERN_INFO pr_fmt(fmt), ##__VA_ARGS__)
#else
#define vw_funcall(fmt, ...) \
        no_printk(KERN_INFO pr_fmt(fmt), ##__VA_ARGS__)
#endif

enum efinix_mode {
    efinix_mode_MIN = 0,
    efinix_mode_VGA_640_480 = 0,
    efinix_mode_MAX = 0,
};

enum efinix_frame_rate {
    efinix_15_fps
    //efinix_30_fps
};

static int efinix_framerates[] = {
    [efinix_15_fps] = 15,
	//[ov5640_30_fps] = 30,
};


struct efinix_datafmt {
    u32 code;
    enum v4l2_colorspace    colorspace;
};

struct efinix {
    struct v4l2_subdev  subdev;
    struct spi_device   *spi;
    struct v4l2_pix_format  pix;
    const struct efinix_datafmt *fmt;
    struct v4l2_captureparm     streamcap;
};

static DEFINE_MUTEX(efinix_mutex);

static const struct spi_device_id efinix_id[] = {
    { "efinix_mipi", 0 },
    { }
};
MODULE_DEVICE_TABLE(spi, efinix_id);

struct spi_device *efinix_spi;

static const struct efinix_datafmt efinix_colour_fmts[] = {
    {MEDIA_BUS_FMT_SBGGR8_1X8, V4L2_COLORSPACE_RAW},
};

static inline struct efinix *to_efinix(struct v4l2_subdev *subdev)
{
    return container_of(subdev, struct efinix, subdev);
}

static const struct efinix_datafmt *efinix_find_datafmt(u32 code)
{
    int i;

    vw_funcall("%s start\n", __func__);

    for (i = 0; i < ARRAY_SIZE(efinix_colour_fmts); i++)
        if (efinix_colour_fmts[i].code == code)
            return efinix_colour_fmts + i;

    return NULL;
}

s32 efinix_write_reg(u32 reg, u32 val)
{
	u32 tx_data[2];
	int ret;
	
	if(efinix_spi == NULL)
		pr_err("%s[%d] spi dev not init\n");

	tx_data[0] = reg;
	tx_data[1] = val;
	ret = spi_write(efinix_spi, tx_data, sizeof(tx_data));
	if (ret < 0)
    {
        printk("%s[%d] spi_transfer fail(%d)\n", __func__, __LINE__, ret);
		return -1;
    }

//  vw_funcall("<%s> reg=0x%08x, val=0x%08x\n",__func__, reg, val);

    return 0;
}
EXPORT_SYMBOL(efinix_write_reg);

s32 efinix_read_reg(u32 reg, u32 *val)
{
	struct spi_transfer t;
	u32 tx_data[2];
	u32 rx_data[2];
	int ret;
	
	if(efinix_spi == NULL)
		pr_err("%s[%d] spi dev not init\n");
#if 0 //TODO
	ret = spi_read(efinix_spi, &data, sizeof(data));
	if (ret < 0)
    {
        printk("%s[%d] spi_transfer fail(%d)\n", __func__, __LINE__, ret);
		return ret;
    }
#endif

//  vw_funcall("<%s> reg=0x%08x, val=0x%08x\n",__func__, reg, Val);

    return 0;
}
EXPORT_SYMBOL(efinix_read_reg);

#ifdef CONFIG_VIDEO_ADV_DEBUG
static int efinix_get_register(struct v4l2_subdev *sd, struct v4l2_dbg_register *reg)
{
    struct spi_device *spi = v4l2_get_subdevdata(sd);
    int ret = 0;
    u32 val;

    ret = efinix_read_reg((u32)reg->reg, &val);
	reg->val = (__u64)val;

    if(ret < 0)
        return -1;

//  vw_funcall("<%s> 0x%08x:0x%08x\n",__func__, reg->reg, reg->val);

    return val;
}

static int efinix_set_register(struct v4l2_subdev *sd, const struct v4l2_dbg_register *reg)
{
    struct spi_device *spi = v4l2_get_subdevdata(sd);
    int ret = 0;
    u32 val;

	ret = efinix_write_reg(fpga, (u32)reg->reg, (u32)reg->val);
    
	if(ret < 0)
        return -1;

//  vw_funcall("<%s> reg=0x%08x, val=0x%08x\n",__func__, reg->reg, reg->val);

    return ret;
}
#endif

static int efinix_stream_on(struct efinix *fpga)
{
    vw_funcall("%s start\n", __func__);

    return 0;
//  return efinix_write_reg(fpga, 0x01E00000, 0x1);
}

static int efinix_stream_off(struct efinix *fpga)
{
    vw_funcall("%s start\n", __func__);

    return 0;
//  return efinix_write_reg(fpga, 0x01E00000, 0x0);
}

/*!
 * efinix_s_power - V4L2 fpga interface handler for VIDIOC_S_POWER ioctl
 * @s: pointer to standard V4L2 device structure
 * @on: indicates power mode (on or off)
 *
 * Turns the power on or off, depending on the value of on and returns the
 * appropriate error code.
 */
static int efinix_s_power(struct v4l2_subdev *sd, int on)
{
    struct spi_device *spi = v4l2_get_subdevdata(sd);

    vw_funcall("%s start\n", __func__);

    return 0;
}


/*!
 * efinix_g_parm - V4L2 fpga interface handler for VIDIOC_G_PARM ioctl
 * @s: pointer to standard V4L2 sub device structure
 * @a: pointer to standard V4L2 VIDIOC_G_PARM ioctl structure
 *
 * Returns the Sensor's video CAPTURE parameters.
 */
static int efinix_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *a)
{
    struct spi_device *spi = v4l2_get_subdevdata(sd);
	struct efinix *fpga = to_efinix(sd);
    struct v4l2_captureparm *cparm = &a->parm.capture;

    vw_funcall("%s start\n", __func__);

    return 0;
}

/*!
 * ov5460_s_parm - V4L2 fpga interface handler for VIDIOC_S_PARM ioctl
 * @s: pointer to standard V4L2 sub device structure
 * @a: pointer to standard V4L2 VIDIOC_S_PARM ioctl structure
 *
 * Configures the Sensor to use the input parameters, if possible.  If
 * not possible, reverts to the old parameters and returns the
 * appropriate error code.
 */
static int efinix_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *a)
{
    struct spi_device *spi = v4l2_get_subdevdata(sd);
	struct efinix *fpga = to_efinix(sd);
    struct v4l2_fract *timeperframe = &a->parm.capture.timeperframe;

    vw_funcall("%s start\n", __func__);

    return 0;
}


/*!
 * efinix_set_fmt - V4L2 fpga interface handler for VIDIOC_S_FMT ioctl
 * @s: pointer to standard V4L2 device structure
 * @on: pointer to standard V4L2 VIDIOC_S_FMT ioctl structure
 *
 * Configures the Sensor to use the input parameters, if possible.  If
 * not possible, reverts to the default parameters
 */
static int efinix_set_fmt(struct v4l2_subdev *sd,
			struct v4l2_subdev_state *state,
            struct v4l2_subdev_format *format)
{
    struct v4l2_mbus_framefmt *mf = &format->format;
    const struct efinix_datafmt *fmt = efinix_find_datafmt(mf->code);
    struct spi_device *spi = v4l2_get_subdevdata(sd);
    struct efinix *fpga = to_efinix(sd);

    vw_funcall("%s start\n", __func__);

    if (!fmt)
    {
        mf->code        = efinix_colour_fmts[0].code;
        mf->colorspace  = efinix_colour_fmts[0].colorspace;
        fmt             = &efinix_colour_fmts[0];
    }
    mf->field   = V4L2_FIELD_NONE;

    if (format->which == V4L2_SUBDEV_FORMAT_TRY) {
        pr_err("<%s> format->which == V4L2_SUBDEV_FORMAT_TRY\n", __func__);
        return 0;
    }

    fpga->fmt = fmt;
    fpga->pix.width = mf->width;
    fpga->pix.height = mf->height;

    return 0;
}


/*!
 * efinix_g_parm - V4L2 fpga interface handler for VIDIOC_G_FMT ioctl
 * @s: pointer to standard V4L2 sub device structure
 * @a: pointer to standard V4L2 VIDIOC_G_FMT ioctl structure
 *
 * Returns the Sensor's video CAPTURE parameters.
 */
static int efinix_get_fmt(struct v4l2_subdev *sd,
			  struct v4l2_subdev_state *state,
              struct v4l2_subdev_format *format)
{
    struct v4l2_mbus_framefmt *mf = &format->format;
    struct spi_device *spi = v4l2_get_subdevdata(sd);
    struct efinix *fpga = to_efinix(sd);
    const struct efinix_datafmt *fmt = fpga->fmt;

    vw_funcall("%s start\n", __func__);

    if (format->pad)
        return -EINVAL;

    mf->code    = fmt->code;
    mf->colorspace  = fmt->colorspace;
    mf->field   = V4L2_FIELD_NONE;

    mf->width   = fpga->pix.width;
    mf->height  = fpga->pix.height;

    return 0;
}

static int efinix_enum_mbus_code(struct v4l2_subdev *sd,
				 struct v4l2_subdev_state *state,
                 struct v4l2_subdev_mbus_code_enum *code)
{
    vw_funcall("%s start\n", __func__);

    if (code->pad || code->index >= ARRAY_SIZE(efinix_colour_fmts)) {
        pr_err("%s error code->pad=%d, code->index=%d, array_size=%d\n", __func__,
            code->pad, code->index, ARRAY_SIZE(efinix_colour_fmts));
        return -EINVAL;
    }

    code->code = efinix_colour_fmts[code->index].code;

    return 0;
}

/*!
 * efinix_enum_framesizes - V4L2 fpga interface handler for
 *             VIDIOC_ENUM_FRAMESIZES ioctl
 * @s: pointer to standard V4L2 device structure
 * @fsize: standard V4L2 VIDIOC_ENUM_FRAMESIZES ioctl structure
 *
 * Return 0 if successful, otherwise -EINVAL.
 */
static int efinix_enum_framesizes(struct v4l2_subdev *sd,
				   struct v4l2_subdev_state *state,
                   struct v4l2_subdev_frame_size_enum *fse)
{
    vw_funcall("%s start\n", __func__);

	if (fse->index > efinix_mode_MAX)
		return -EINVAL;

    fse->max_width = 640;
    fse->min_width = fse->max_width;
    fse->max_height = 480;
    fse->min_height = fse->max_height;

    return 0;
}


/*!
 * efinix_enum_frameintervals - V4L2 fpga interface handler for
 *                 VIDIOC_ENUM_FRAMEINTERVALS ioctl
 * @s: pointer to standard V4L2 device structure
 * @fival: standard V4L2 VIDIOC_ENUM_FRAMEINTERVALS ioctl structure
 *
 * Return 0 if successful, otherwise -EINVAL.
 */
static int efinix_enum_frameintervals(struct v4l2_subdev *sd,
		struct v4l2_subdev_state *state,
        struct v4l2_subdev_frame_interval_enum *fie)
{
    struct spi_device *spi = v4l2_get_subdevdata(sd);

	if (fie->index > 0)
		return -EINVAL;

	fie->interval.denominator = 15; //??

    vw_funcall("%s start\n", __func__);

    return 0;
}

static int efinix_s_stream(struct v4l2_subdev *sd, int enable)
{
    struct spi_device *spi = v4l2_get_subdevdata(sd);
    struct efinix *fpga = to_efinix(sd);
    int ret;

    vw_funcall("%s start\n",__func__);
    //vw_debug("<%s> s_stream: %d\n",__func__, enable);

    if (enable)
        ret = efinix_stream_on(fpga);
    else
        ret = efinix_stream_off(fpga);

    return ret;
}

static struct v4l2_subdev_video_ops efinix_subdev_video_ops = {
    .g_parm = efinix_g_parm,
    .s_parm = efinix_s_parm,
    .s_stream = efinix_s_stream,
};

static const struct v4l2_subdev_pad_ops efinix_subdev_pad_ops = {
    .enum_frame_size       = efinix_enum_framesizes,
    //.enum_frame_interval   = efinix_enum_frameintervals,
    .enum_mbus_code        = efinix_enum_mbus_code,
    .set_fmt               = efinix_set_fmt,
    .get_fmt               = efinix_get_fmt,
};

static struct v4l2_subdev_core_ops efinix_subdev_core_ops = {
    .s_power    = efinix_s_power,
#ifdef CONFIG_VIDEO_ADV_DEBUG
    .g_register = efinix_get_register,
    .s_register = efinix_set_register,
#endif
};

static struct v4l2_subdev_ops efinix_subdev_ops = {
    .core   = &efinix_subdev_core_ops,
    .video  = &efinix_subdev_video_ops,
    .pad    = &efinix_subdev_pad_ops,
};


static int efinix_probe(struct spi_device *spi)
{
	int retval;
	struct efinix *fpga;

	fpga = devm_kzalloc(&spi->dev, sizeof(*fpga), GFP_KERNEL);
    if (!fpga)
        return -ENOMEM;

    /* default setting */
	fpga->spi = spi;
    fpga->pix.pixelformat =  V4L2_PIX_FMT_SBGGR8;
    fpga->pix.width = 640;
    fpga->pix.height = 480;
    fpga->streamcap.capability = V4L2_MODE_HIGHQUALITY | V4L2_CAP_TIMEPERFRAME;
    //fpga->streamcap.timeperframe.denominator = 30;
    //fpga->streamcap.timeperframe.numerator = 1;
	
	v4l2_spi_subdev_init(&fpga->subdev, fpga->spi, &efinix_subdev_ops);

    fpga->subdev.grp_id = 678;
    retval = v4l2_async_register_subdev(&fpga->subdev);

	efinix_spi = fpga->spi;

	printk("%s done\n", __func__);
#if 1//testcode
    printk("########### bus_num(%d) frq(%d) mode(%d)\n", efinix_spi->master->bus_num, efinix_spi->max_speed_hz, efinix_spi->mode);
#endif

    return retval;
}

static void efinix_remove(struct spi_device *spi)
{
    struct v4l2_subdev *sd = spi_get_drvdata(spi);
	struct efinix *fpga = to_efinix(sd);

    vw_funcall("%s start\n", __func__);

    v4l2_async_unregister_subdev(sd);
}

#ifdef CONFIG_OF
static const struct of_device_id efinix_mipi_of_match[] = {
    { .compatible = "vw,efinix_mipi",
    },
    { /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, efinix_mipi_of_match);
#endif

static struct spi_driver efinix_mipi_spi_driver = {
    .driver = {
		.owner = THIS_MODULE,
        .name       = "efinix_mipi",
#ifdef CONFIG_OF
		.of_match_table = of_match_ptr(efinix_mipi_of_match),
#endif
    },
    .probe      = efinix_probe,
    .remove     = efinix_remove,
    .id_table   = efinix_id,
};

module_spi_driver(efinix_mipi_spi_driver);

