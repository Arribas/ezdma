#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/uio_driver.h>
#include <linux/irq.h>

MODULE_LICENSE("GPL");

#define SAV_IRQ 91

static struct resource uio_resource[] = { 
	{ 
		.start = 0x1f400000,
		.end   = 0x1fffffff,
		.name = "sav_vdma_mem",
		.flags = IORESOURCE_MEM
	}, 
	{ 
		.start = 0x43000000,
		.end   = 0x4300ffff,
		.name = "sav_vdma_reg",
		.flags = IORESOURCE_MEM
	}, 
	{ 
		.start = 0x62200000,
		.end   = 0x6220ffff, 
		.name = "sav_axi_reader",
		.flags = IORESOURCE_MEM
	}, 
	{ 
		.start = 0x7B200000,
		.end   = 0x7B20ffff, 
		.name = "sav_axi_cfg",
		.flags = IORESOURCE_MEM
	}, 
}; 


static irqreturn_t irq_handler(int irq, struct uio_info *dev_info) {
	switch(irq) {
		case SAV_IRQ:
			return IRQ_HANDLED;
	}

	return IRQ_NONE;
}


static struct uio_info myfpga_uio_info = {
	.name = "uio_fpga",
	.version = "0.1",
	.irq = SAV_IRQ,
	.handler = irq_handler,
};


static struct platform_device uio_device = { 
	.name      = "uio_pdrv", 
	.id	= -1,
	.resource   = uio_resource, 
	.num_resources   = ARRAY_SIZE(uio_resource), 
	.dev.platform_data = &myfpga_uio_info,
};


static int __init mod_init(void)
{
	int ret;

	ret = platform_device_register(&uio_device);
	irq_set_irq_type(SAV_IRQ, IRQ_TYPE_EDGE_RISING);

	printk(KERN_INFO "UIO Module loaded %d\n", ret);

        return 0;
}

static void __exit mod_exit(void)
{
	platform_device_unregister(&uio_device);
}

module_init(mod_init);
module_exit(mod_exit);
