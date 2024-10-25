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
#include <linux/i2c.h>
#include <linux/of_gpio.h>
#include <linux/pinctrl/consumer.h>
#include <linux/regulator/consumer.h>
#include <linux/v4l2-mediabus.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ctrls.h>

//#include "fpga_reg.h"
//#include "efinix_fpga_mipi.h"

#define VW_FUNCALL
#ifdef VW_FUNCALL
#define vw_funcall(fmt, ...) \
		printk(KERN_INFO pr_fmt(fmt), ##__VA_ARGS__)
#else
#define vw_funcall(fmt, ...) \
		no_printk(KERN_INFO pr_fmt(fmt), ##__VA_ARGS__)
#endif

//#define VW_DEBUG
#ifdef VW_DEBUG
#define vw_debug(fmt, ...) \
	printk(KERN_INFO pr_fmt(fmt), ##__VA_ARGS__)
#else
#define vw_debug(fmt, ...) \
	no_printk(KERN_INFO pr_fmt(fmt), ##__VA_ARGS__)
#endif

#define vw_err(fmt, ...) \
	printk(KERN_ERR pr_fmt(fmt), ##__VA_ARGS__)


struct efinix_datafmt {
	u32	code;
	enum v4l2_colorspace	colorspace;
};

struct efinix {
	struct v4l2_subdev	subdev;
	struct i2c_client	*i2c_client;
	struct v4l2_pix_format	pix;
	const struct efinix_datafmt	*fmt;
	struct v4l2_captureparm		streamcap;
};

static DEFINE_MUTEX(efinix_mutex);
static int efinix_probe(struct i2c_client *adapter);
static void efinix_remove(struct i2c_client *client);
static u32 efinix_read_reg(struct efinix *fpga, u32 reg);
static s32 efinix_write_reg(struct efinix *fpga, u32 reg, u32 val);

#ifdef CONFIG_OF
static const struct of_device_id efinix_mipi_of_match[] = {
	{ .compatible = "ovti,efinix_mipi",
	},
	{ /* sentinel */ }
};

MODULE_DEVICE_TABLE(of, efinix_mipi_of_match);
#endif

static const struct i2c_device_id efinix_id[] = {
	{"efinix_mipi", 0},
	{},
};

MODULE_DEVICE_TABLE(i2c, efinix_id);

static struct i2c_driver efinix_i2c_driver = {
	.driver = {
		  .owner = THIS_MODULE,
		  .name  = "efinix_mipi",
#ifdef CONFIG_OF
		  .of_match_table = of_match_ptr(efinix_mipi_of_match),
#endif
		  },
	.probe  = efinix_probe,
	.remove = efinix_remove,
	.id_table = efinix_id,
};

static const struct efinix_datafmt efinix_colour_fmts[] = {	
	{MEDIA_BUS_FMT_SBGGR8_1X8, V4L2_COLORSPACE_RAW},	
};

static struct efinix *to_efinix(const struct i2c_client *client)
{
//	vw_funcall("%s start\n", __func__);
	
	return container_of(i2c_get_clientdata(client), struct efinix, subdev);
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

static s32 efinix_write_reg(struct efinix *fpga, u32 reg, u32 val)
{
	u8 au8Buf[8] = {0};

	au8Buf[0] = reg >> 24;
	au8Buf[1] = reg >> 16;
	au8Buf[2] = reg >> 8;
	au8Buf[3] = reg & 0xff;
	au8Buf[4] = val >> 24;
	au8Buf[5] = val >> 16;
	au8Buf[6] = val >> 8;
	au8Buf[7] = val & 0xff;

	if (i2c_master_send(fpga->i2c_client, au8Buf, 8) < 0) {
		vw_err("<%s> error: reg=0x%08x, val=0x%08x\n",__func__, reg, val);
		return -1;
	}

//	vw_funcall("<%s> reg=0x%08x, val=0x%08x\n",__func__, reg, val);

	return 0;
}

static u32 efinix_read_reg(struct efinix *fpga, u32 reg)
{
	u8 au8RegBuf[4] = {0};
	u8 u8RdVal[4] = {0};
	u32	Val;

	au8RegBuf[0] = reg >> 24;
	au8RegBuf[1] = reg >> 16;
	au8RegBuf[2] = reg >> 8;
	au8RegBuf[3] = reg & 0xff;
	
	if (i2c_master_send(fpga->i2c_client, au8RegBuf, 4) != 4) {
		vw_err("<%s> error: reg=0x%08x\n",__func__, reg);
		return -1;
	}

	if (i2c_master_recv(fpga->i2c_client, u8RdVal, 4) != 4) {
		vw_err("<%s> error: reg=0x%08x, val=0x%08x\n",__func__, reg, u8RdVal);
		return -1;
	}

	Val = (u8RdVal[0] << 24) | (u8RdVal[1] << 16) | (u8RdVal[2] << 8) | u8RdVal[3];

//	vw_funcall("<%s> reg=0x%08x, val=0x%08x\n",__func__, reg, Val);

	return Val;
}

#ifdef CONFIG_VIDEO_ADV_DEBUG
static int efinix_get_register(struct v4l2_subdev *sd, struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct efinix *fpga = to_efinix(client);
	int ret = 0;
	u32 val;
	
	val = efinix_read_reg(fpga, (u32)reg->reg);
	reg->val = (__u64)val;

	if(ret < 0)
		return -1;

//	vw_funcall("<%s> 0x%08x:0x%08x\n",__func__, reg->reg, reg->val);
	
	return ret;
}

static int efinix_set_register(struct v4l2_subdev *sd, const struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct efinix *fpga = to_efinix(client);
	int ret;

	ret = efinix_write_reg(fpga, (u32)reg->reg, (u32)reg->val);

	if(ret < 0)
		return -1;

//	vw_funcall("<%s> reg=0x%08x, val=0x%08x\n",__func__, reg->reg, reg->val);
	
	return ret;
}
#endif

static int efinix_stream_on(struct efinix *fpga)
{
	vw_funcall("%s start\n", __func__);

	return 0;
//	return efinix_write_reg(fpga, 0x01E00000, 0x1);
}

static int efinix_stream_off(struct efinix *fpga)
{
	vw_funcall("%s start\n", __func__);

	return 0;
//	return efinix_write_reg(fpga, 0x01E00000, 0x0);
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
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct efinix *fpga = to_efinix(client);

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
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct efinix *fpga = to_efinix(client);
	struct device *dev = &fpga->i2c_client->dev;
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
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct efinix *fpga = to_efinix(client);
	struct device *dev = &fpga->i2c_client->dev;
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
			struct v4l2_subdev_state *sd_state,
			struct v4l2_subdev_format *format)
{
	struct v4l2_mbus_framefmt *mf = &format->format;
	const struct efinix_datafmt *fmt = efinix_find_datafmt(mf->code);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct efinix *fpga = to_efinix(client);

	vw_funcall("%s start\n", __func__);
	
	if (!fmt) 
	{
		mf->code		= efinix_colour_fmts[0].code;
		mf->colorspace	= efinix_colour_fmts[0].colorspace;
		fmt				= &efinix_colour_fmts[0];
	}
	mf->field	= V4L2_FIELD_NONE;

	if (format->which == V4L2_SUBDEV_FORMAT_TRY) {
		vw_err("<%s> format->which == V4L2_SUBDEV_FORMAT_TRY\n", __func__);
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
			  struct v4l2_subdev_state *sd_state,
			  struct v4l2_subdev_format *format)
{
	struct v4l2_mbus_framefmt *mf = &format->format;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct efinix *fpga = to_efinix(client);
	const struct efinix_datafmt *fmt = fpga->fmt;

	vw_funcall("%s start\n", __func__);
	
	if (format->pad)
		return -EINVAL;

	mf->code	= fmt->code;
	mf->colorspace	= fmt->colorspace;
	mf->field	= V4L2_FIELD_NONE;

	mf->width	= fpga->pix.width;
	mf->height	= fpga->pix.height;

	return 0;
}

static int efinix_enum_mbus_code(struct v4l2_subdev *sd,
				struct v4l2_subdev_state *sd_state,
				 struct v4l2_subdev_mbus_code_enum *code)
{
	vw_funcall("%s start\n", __func__);
	
	if (code->pad || code->index >= ARRAY_SIZE(efinix_colour_fmts)) {
		vw_err("%s error code->pad=%d, code->index=%d, array_size=%d\n", __func__, 
			code->pad, code->index, ARRAY_SIZE(efinix_colour_fmts));
		return -EINVAL;
	}

	code->code = efinix_colour_fmts[code->index].code;
	
	return 0;
}

/*!
 * efinix_enum_framesizes - V4L2 fpga interface handler for
 *			   VIDIOC_ENUM_FRAMESIZES ioctl
 * @s: pointer to standard V4L2 device structure
 * @fsize: standard V4L2 VIDIOC_ENUM_FRAMESIZES ioctl structure
 *
 * Return 0 if successful, otherwise -EINVAL.
 */
static int efinix_enum_framesizes(struct v4l2_subdev *sd,
    			   struct v4l2_subdev_state *sd_state,
			       struct v4l2_subdev_frame_size_enum *fse)
{
	vw_funcall("%s start\n", __func__);

	fse->max_width = 640;
	fse->min_width = fse->max_width;
	fse->max_height = 480;
	fse->min_height = fse->max_height;

	return 0;
}

/*!
 * efinix_enum_frameintervals - V4L2 fpga interface handler for
 *			       VIDIOC_ENUM_FRAMEINTERVALS ioctl
 * @s: pointer to standard V4L2 device structure
 * @fival: standard V4L2 VIDIOC_ENUM_FRAMEINTERVALS ioctl structure
 *
 * Return 0 if successful, otherwise -EINVAL.
 */
static int efinix_enum_frameintervals(struct v4l2_subdev *sd,
		struct v4l2_subdev_state *sd_state,
		struct v4l2_subdev_frame_interval_enum *fie)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct device *dev = &client->dev;

	vw_funcall("%s start\n", __func__);
	
	return 0;
}

#if 0
/*********** python Sensor Code ***********/
static void Sensor_Reset(struct efinix *fpga, u32 channel)
{
    u32 data;

	vw_funcall("%s start\n", __func__);
	
    data = efinix_read_reg(fpga, FPGA_REG_SENSOR_RESET);

    data &= ~(1 << channel);
    efinix_write_reg(fpga, FPGA_REG_SENSOR_RESET,   data);
    msleep(10);
	
    data |= (1 << channel);
    efinix_write_reg(fpga, FPGA_REG_SENSOR_RESET,   data);
    msleep(10);
}

int Sensor_Sync(struct efinix *fpga)
{
	FPGA	reg;
	s32		timeout;
	int		ret	= 0;

	vw_funcall("%s start\n", __func__);
	
	/* 데이터 입력 로직 리셋 */
	reg.TRAIN_CTRL.DATA			= efinix_read_reg(fpga, FPGA_REG_TRAIN_CTRL);
	reg.TRAIN_CTRL.BIT.TOP_RST		= TRUE;
	reg.TRAIN_CTRL.BIT.BOT_RST		= TRUE;
	reg.TRAIN_CTRL.BIT.MID_RST		= TRUE;
	efinix_write_reg(fpga, FPGA_REG_TRAIN_CTRL,	reg.TRAIN_CTRL.DATA);
	reg.TRAIN_CTRL.BIT.TOP_RST		= FALSE;
	reg.TRAIN_CTRL.BIT.BOT_RST		= FALSE;
	reg.TRAIN_CTRL.BIT.MID_RST		= FALSE;
	efinix_write_reg(fpga, FPGA_REG_TRAIN_CTRL,	reg.TRAIN_CTRL.DATA);
	
	/* fpga training 모드 설정		*/
	reg.TRAIN_CTRL.DATA			= efinix_read_reg(fpga, FPGA_REG_TRAIN_CTRL);
	reg.TRAIN_CTRL.BIT.TOP_EN		= TRUE;
	reg.TRAIN_CTRL.BIT.BOT_EN		= TRUE;
	reg.TRAIN_CTRL.BIT.MID_EN		= TRUE;
	efinix_write_reg(fpga, FPGA_REG_TRAIN_CTRL,	reg.TRAIN_CTRL.DATA);
	
	msleep(10);
	
    timeout	= 10;
    reg.TRAIN_CTRL.DATA		= efinix_read_reg(fpga, FPGA_REG_TRAIN_CTRL);
    while((!(reg.TRAIN_CTRL.BIT.DONE)) && timeout)
    {
        msleep(10);
        timeout--;
        reg.TRAIN_CTRL.DATA		= efinix_read_reg(fpga, FPGA_REG_TRAIN_CTRL);
    }

    if(timeout <= 0)
    {
        vw_err("training timeout.\r\n");
        ret = -1;
    }
    else
    {
        vw_debug("training success\r\n");
    }

    reg.TRAIN_CTRL.BIT.TOP_EN		= FALSE;
    reg.TRAIN_CTRL.BIT.BOT_EN		= FALSE;
    reg.TRAIN_CTRL.BIT.MID_EN		= FALSE;
    efinix_write_reg(fpga, FPGA_REG_TRAIN_CTRL,	reg.TRAIN_CTRL.DATA);
	
	return ret;
}

static void enable_clock_management(struct efinix *fpga, u8 channel)
{
	volatile s16 timeout = 10000;
	u32 data;

	vw_funcall("%s start\n", __func__);
	
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | VITA_REG_CHIP_CFG,		    0x0000);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | VITA_REG_SOFT_RESET_PLL,		0x0000);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | VITA_REG_PLL_POWER_DOWN,		0x0003);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 17,					    	0x2113);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | VITA_REG_IO_CFG,				0x0000);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 26,					    	0x2280);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 27,					    	0x3D2D);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | VITA_REG_CLK_CFG,			0x7004);

	data = efinix_read_reg(fpga, CIS_CH2ADDR(channel) | VITA_REG_PLL_LOCK);
	while(!(data & 0x01) && timeout)
	{
		timeout--;
	}
	
	if(timeout <= 0)
	{
		vw_err("enable clock management timeout.\r\n");
		return;
	}
	
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | VITA_REG_SOFT_RESET_CGEN,	0x0000);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | VITA_REG_CLK_CFG,			0x7006);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | VITA_REG_LOGIC_CFG,		    0x0001);
}

void disable_clock_management(struct efinix *fpga, u8 channel)
{
	vw_funcall("%s start\n", __func__);
	
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | VITA_REG_SOFT_RESET_CGEN,	0x0000); // Soft reset clock generator
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | VITA_REG_CLK_CFG,			0x7004); // Disable logic clock
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | VITA_REG_LOGIC_CFG,		    0x0000); // Disable logic blocks
	
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | VITA_REG_SOFT_RESET_PLL,		0x0099);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | VITA_REG_PLL_POWER_DOWN,		0x0000);
}
		
void upload_required_registers(struct efinix *fpga, u8 channel)
{
	CIS_REG	reg;
	u32	i;

	vw_funcall("%s start\n", __func__);

	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 41,				0x085F);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 42,				0x4110);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 43,				0x0008);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 65,				0x382B);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 66,				0x53C8);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 67,				0x0665);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 68,				0x0085); //TODO: check 0x88 -> 0x85
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 70,				0x1111);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 72,				0x0017);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 128,				0x4714);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 129,				0x8001);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 171,				0x1002);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 175,				0x0080);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 176,				0x00E6);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 177,				0x0400);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 192,				0x080C);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 194,				0x0224);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 197,				0x0306);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 204,				0x01E4);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 207,				0x0000);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 211,				0x0E49);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 215,				0x111F);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 216,				0x7F00);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 219,				0x0020);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 220,				0x3A28);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 221,				0x624D);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 222,				0x624D);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 224,				0x3E11);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 227,				0x0000);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 250,				0x2081);
	
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 384,				0xC800);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 385,				0xFB1F);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 386,				0xFB1F);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 387,				0xFB12);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 388,				0xF903);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 389,				0xF802);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 390,				0xF30F);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 391,				0xF30F);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 392,				0xF30F);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 393,				0xF30A);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 394,				0xF101);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 395,				0xF00A);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 396,				0xF24B);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 397,				0xF226);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 398,				0xF001);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 399,				0xF402);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 400,				0xF001);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 401,				0xF402);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 402,				0xF001);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 403,				0xF401);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 404,				0xF007);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 405,				0xF20F);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 406,				0xF20F);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 407,				0xF202);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 408,				0xF006);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 409,				0xEC02);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 410,				0xE801);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 411,				0xEC02);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 412,				0xE801);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 413,				0xEC02);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 414,				0xC801);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 415,				0xC800);
	
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 416,				0xC800);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 417,				0xCC02);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 418,				0xC801);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 419,				0xCC02);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 420,				0xC801);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 421,				0xCC02);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 422,				0xC805);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 423,				0xC800);
	
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 424,				0x0030);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 425,				0x207C);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 426,				0x2071);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 427,				0x0074);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 428,				0x107F);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 429,				0x1072);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 430,				0x1074);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 431,				0x0076);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 432,				0x0031);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 433,				0x21BB);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 434,				0x20B1);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 435,				0x20B1);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 436,				0x00B1);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 437,				0x10BF);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 438,				0x10B2);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 439,				0x10B4);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 440,				0x00B1);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 441,				0x0030);

	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 442,				0x0030);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 443,				0x217B);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 444,				0x2071);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 445,				0x2071);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 446,				0x0074);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 447,				0x107F);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 448,				0x1072);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 449,				0x1074);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 450,				0x0076);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 451,				0x0031);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 452,				0x20BB);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 453,				0x20B1);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 454,				0x20B1);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 455,				0x00B1);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 456,				0x10BF);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 457,				0x10B2);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 458,				0x10B4);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 459,				0x00B1);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 460,				0x0030);
	
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 461,				0x0030);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 462,				0x207C);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 463,				0x2071);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 464,				0x0073);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 465,				0x017A);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 466,				0x0078);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 467,				0x1074);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 468,				0x0076);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 469,				0x0031);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 470,				0x21BB);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 471,				0x20B1);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 472,				0x20B1);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 473,				0x00B1);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 474,				0x10BF);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 475,				0x10B2);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 476,				0x10B4);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 477,				0x00B1);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 478,				0x0030);
	
	reg.DATA_CFG.DATA		= efinix_read_reg(fpga, CIS_CH2ADDR(channel) | VITA_REG_DATA_CFG);
	reg.DATA_CFG.BIT.BIT_8	= FALSE;
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | VITA_REG_DATA_CFG,	reg.DATA_CFG.DATA);
	
	reg.SEQ_CFG.DATA			= efinix_read_reg(fpga, CIS_CH2ADDR(channel) | VITA_REG_SEQ_CFG);
	reg.SEQ_CFG.BIT.SLAVE_MODE	= TRUE;
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | VITA_REG_SEQ_CFG,reg.SEQ_CFG.DATA);
	
	reg.BLKO_CFG.DATA			= efinix_read_reg(fpga, CIS_CH2ADDR(channel) | VITA_REG_DATA_BLACKCAL);
	reg.BLKO_CFG.BIT.BLK_SAMPLE	= 7; // Dark 상태에서  column 방향의 Fixed pattern 노이즈를 줄일 수 있는 설정값
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | VITA_REG_DATA_BLACKCAL,		reg.BLKO_CFG.DATA);
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | VITA_REG_SEQ_BLACK_LINES,	0x103);
}

void soft_power_up(struct efinix *fpga, u8 channel)
{
	vw_funcall("%s start\n", __func__);
	
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | VITA_REG_SOFT_RESET_ANALOG,	0x0000); // Release soft reset state
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | VITA_REG_CLK_CFG,			0x7007); // Enable analog clock
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | VITA_REG_IMG_CORE_CFG1,		0x0003); // Enable column multiplexer
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 42,							0x4113); // Configure image core
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | VITA_REG_AFE_PWR_DOWN,		0x0001); // Enable AFE
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | VITA_REG_BIAS_PWR_DOWN,		0x0001); // Enable biasing block
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | VITA_REG_CHARGE_PUMP_CFG,	0x0017); // Enable charge pump
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | VITA_REG_LVDS_PWR_DOWN,		0x0007); // Enable LVDS transmitters
}

void soft_power_down(struct efinix *fpga, u8 channel)
{
	vw_funcall("%s start\n", __func__);
	
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | VITA_REG_SOFT_RESET_ANALOG,	0x0999); // Soft reset
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | VITA_REG_CLK_CFG,			0x7006); // Disable analog clock
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | VITA_REG_IMG_CORE_CFG1,		0x0000); // Disable column multiplexer
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | 42,							0x4110); // Image core config
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | VITA_REG_AFE_PWR_DOWN,		0x0000); // Disable AFE
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | VITA_REG_BIAS_PWR_DOWN,		0x0000); // Disable biasing block
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | VITA_REG_CHARGE_PUMP_CFG,	0x0010); // Disable charge pump
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | VITA_REG_LVDS_PWR_DOWN,		0x0000); // Disable LVDS transmitters
}

u8 is_sequencer_enabled(struct efinix *fpga, u8 channel)
{
	CIS_REG	reg;

	vw_funcall("%s start\n", __func__);
	
	reg.SEQ_CFG.DATA		= efinix_read_reg(fpga, CIS_CH2ADDR(channel) | VITA_REG_SEQ_CFG);

	return reg.SEQ_CFG.BIT.EN;
}

void enable_sequencer(struct efinix *fpga, u8 channel)
{
	CIS_REG	reg;

	vw_funcall("%s start\n", __func__);
	
	reg.SEQ_CFG.DATA		= efinix_read_reg(fpga, CIS_CH2ADDR(channel) | VITA_REG_SEQ_CFG);
	reg.SEQ_CFG.BIT.EN		= TRUE;
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | VITA_REG_SEQ_CFG,	reg.SEQ_CFG.DATA);
}

void disable_sequencer(struct efinix *fpga, u8 channel)
{
	CIS_REG	reg;

	vw_funcall("%s start\n", __func__);
		
	reg.SEQ_CFG.DATA		= efinix_read_reg(fpga, CIS_CH2ADDR(channel) | VITA_REG_SEQ_CFG);
	reg.SEQ_CFG.BIT.EN		= FALSE;
	efinix_write_reg(fpga, CIS_CH2ADDR(channel) | VITA_REG_SEQ_CFG,	reg.SEQ_CFG.DATA);
}

static int Sensor_Initialize(struct efinix *fpga)
{
	struct device *dev = &fpga->i2c_client->dev;
	int ret = 0;
	int i;

	vw_funcall("%s start\n", __func__);

	for(i = 0; i < CFG_SENSOR_MAX; i++)
	{
		Sensor_Reset(fpga, i);
	}

	for(i = 0; i < CFG_SENSOR_MAX; i++)
    {
    	enable_clock_management(fpga, i);
	    upload_required_registers(fpga, i);

	    soft_power_up(fpga, i);
	    enable_sequencer(fpga, i);

        /* 모니터링 핀 설정 */
		efinix_write_reg(fpga, CIS_CH2ADDR(i)		|	192, efinix_read_reg(fpga, CIS_CH2ADDR(i) | 192) | (6 << 11));

		/* 트레이닝 패턴 설정 */
		efinix_write_reg(fpga, CIS_CH2ADDR(i)		|	VITA_REG_DATA_TRAIN_PAT,	0x03A6);
		efinix_write_reg(fpga, FPGA_CH2ADDR(i)		|	FPGA_REG_TRAIN_PAT,			0x03A6);
	}

	if(Sensor_Sync(fpga) < 0)
		return -1;

	return ret;
}
#endif

static int efinix_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct efinix *fpga = to_efinix(client);
	struct device *dev = &fpga->i2c_client->dev;
	int ret;

	vw_funcall("%s start\n",__func__);
	vw_debug("<%s> s_stream: %d\n",__func__, enable);
	
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
	.enum_frame_interval   = efinix_enum_frameintervals,
	.enum_mbus_code        = efinix_enum_mbus_code,
	.set_fmt               = efinix_set_fmt,
	.get_fmt               = efinix_get_fmt,
};

static struct v4l2_subdev_core_ops efinix_subdev_core_ops = {
	.s_power	= efinix_s_power,
#ifdef CONFIG_VIDEO_ADV_DEBUG
	.g_register	= efinix_get_register,
	.s_register	= efinix_set_register,
#endif
};

static struct v4l2_subdev_ops efinix_subdev_ops = {
	.core	= &efinix_subdev_core_ops,
	.video	= &efinix_subdev_video_ops,
	.pad	= &efinix_subdev_pad_ops,
};

/*!
 * efinix I2C probe function
 *
 * @param adapter            struct i2c_adapter *
 * @return  Error code indicating success or failure
 */
static int efinix_probe(struct i2c_client *client)
{
	struct device *dev = &client->dev;
	int retval;
	struct efinix *fpga;

	vw_funcall("%s start\n", __func__);
	
	fpga = devm_kzalloc(dev, sizeof(*fpga), GFP_KERNEL);
	if (!fpga)
		return -ENOMEM;

	/* default setting */
	fpga->i2c_client = client;
	fpga->pix.pixelformat =  V4L2_PIX_FMT_SBGGR8;
	fpga->pix.width = 640;
	fpga->pix.height = 480;
	fpga->streamcap.capability = V4L2_MODE_HIGHQUALITY | V4L2_CAP_TIMEPERFRAME;
	//fpga->streamcap.timeperframe.denominator = 30;
	//fpga->streamcap.timeperframe.numerator = 1;
#if 0
	retval = Sensor_Initialize(fpga);
	if (retval < 0)
		dev_err(&client->dev, "Sensor_Initialize failed, ret=%d\n", retval);
#endif
	v4l2_i2c_subdev_init(&fpga->subdev, client, &efinix_subdev_ops);

	fpga->subdev.grp_id = 678;
	retval = v4l2_async_register_subdev(&fpga->subdev);
	if (retval < 0)
		dev_err(&client->dev, "Async register failed, ret=%d\n", retval);

	dev_info(dev, "Camera is found\n");

	return retval;
}

/*!
 * efinix I2C detach function
 *
 * @param client            struct i2c_client *
 * @return  Error code indicating success or failure
 */
static void efinix_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct efinix *fpga = to_efinix(client);

	vw_funcall("%s start\n", __func__);
	
	v4l2_async_unregister_subdev(sd);
}

module_i2c_driver(efinix_i2c_driver);

MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_DESCRIPTION("EFINIX MIPI Camera Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");
MODULE_ALIAS("CSI");

