#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/cpu.h>
#include <linux/smp.h>
#include <linux/cpumask.h>
#include <linux/notifier.h>
#include <mach/mt_reg_base.h>
#include <mach/etm.h>
#include <mach/mt_boot.h>

/* use either ETR_SRAM, ETR_DRAM, or ETB */
//#define ETR_SRAM
//#define ETR_DRAM
#define ETB

#define ETR_BUFF_SIZE 0x200
#define ETR_SRAM_PHYS_BASE (0x00100000 + 0xF800)
#define ETR_SRAM_VIRT_BASE (INTER_SRAM + 0xF800)
#define ETR_SHARE_SRAM_PHYS_BASE (0x00200000 + 0x0)
#define ETR_SHARE_SRAM_VIRT_BASE (SYSRAM_BASE + 0x0)

static struct etm_driver_data etm_driver_data[] =
{
        {
		.etm_regs = IOMEM(DEBUGTOP_BASE + 0x7C000),
		.is_ptm = 0,
	},
	{
		.etm_regs = IOMEM(DEBUGTOP_BASE + 0x7D000),
		.is_ptm = 0,
	},
	{
		.etm_regs = IOMEM(DEBUGTOP_BASE + 0x7E000),
		.is_ptm = 0,
	},
	{
		.etm_regs = IOMEM(DEBUGTOP_BASE + 0x7F000),
		.is_ptm = 0,
	},
	{
		.etm_regs = IOMEM(DEBUGTOP_BASE + 0x9C000),
		.is_ptm = 0,
	},
	{
		.etm_regs = IOMEM(DEBUGTOP_BASE + 0x9D000),
		.is_ptm = 0,
	},
	{
		.etm_regs = IOMEM(DEBUGTOP_BASE + 0x9E000),
		.is_ptm = 0,
	},
	{
		.etm_regs = IOMEM(DEBUGTOP_BASE + 0x9F000),
		.is_ptm = 0,
	},
};

static struct platform_device etm_device[] = 
{
	{
		.name = "etm",
		.id = 0,
		.dev = 
		{
			.platform_data = &(etm_driver_data[0]),
		},
	},
	{
		.name = "etm",
		.id = 1,
		.dev = 
		{
			.platform_data = &(etm_driver_data[1]),
		},
	},
	{
		.name = "etm",
		.id = 2,
		.dev = 
		{
			.platform_data = &(etm_driver_data[2]),
		},
	},
	{
		.name = "etm",
		.id = 3,
		.dev = 
		{
			.platform_data = &(etm_driver_data[3]),
		},
	},
        {
		.name = "etm",
		.id = 4,
		.dev = 
		{
			.platform_data = &(etm_driver_data[4]),
		},
	},
        {
		.name = "etm",
		.id = 5,
		.dev = 
		{
			.platform_data = &(etm_driver_data[5]),
		},
	},
        {
		.name = "etm",
		.id = 6,
		.dev = 
		{
			.platform_data = &(etm_driver_data[6]),
		},
	},
        {
		.name = "etm",
		.id = 7,
		.dev = 
		{
			.platform_data = &(etm_driver_data[7]),
		},
	},

};

static struct etb_driver_data etb_driver_data =
{
	.funnel_regs = IOMEM(DEBUGTOP_BASE + 0x14000),
	.tpiu_regs = IOMEM(DEBUGTOP_BASE + 0x13000),
	.dem_regs = IOMEM(DEBUGTOP_BASE + 0x1A000),

};

static struct platform_device etb_device =
{
	.name = "etb",
	.id = -1,
	.dev =
	{
		.platform_data = &etb_driver_data,
	},
};

DEFINE_PER_CPU(int, trace_pwr_down);
#if 0
void dump_trace_data(void)
{
    int cpu;
    for (cpu = 0; cpu < NR_CPUS; cpu++) {
        printk("CPU_%d trace_pwr_down:%d\n",cpu,per_cpu(trace_pwr_down, cpu));
    }

}
#endif
void trace_start_dormant(void)
{
	int cpu;

	trace_start_by_cpus(cpumask_of(0), 1);

	/*
	 * XXX: This function is called just before entering the suspend mode.
	 *	  The Linux kernel is already freeze.
	 *	  So it is safe to do the trick to access the per-cpu variable directly.
	 */
	for (cpu = 1; cpu < num_possible_cpus(); cpu++) {
		per_cpu(trace_pwr_down, cpu) = 1;
	}
}

static int
restart_trace(struct notifier_block *nfb, unsigned long action, void *hcpu)
{
	unsigned int cpu;
	volatile int *pwr_down;
	unsigned int cpu_ix = 0;

	cpu = (unsigned int)hcpu;

	switch (action) {
	case CPU_STARTING:
		switch (cpu) {
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
			pwr_down = &get_cpu_var(trace_pwr_down);
			if (*pwr_down) {
				trace_start_by_cpus(cpumask_of(cpu), 0);
				*pwr_down = 0;
			}
			put_cpu_var(trace_pwr_down);
			break;

		default:
			break;
		}

		break;

	case CPU_DYING:
		switch (cpu) {
		case 4:
		case 5:
		case 6:
		case 7:
                    if (num_possible_cpus() > 4) 
                    {
                        if ((!cpu_online(4)) && (!cpu_online(5)) && (!cpu_online(6)) && (!cpu_online(7))) {
			    // num_possible_cpus could be 4, 6, 8
			    for (cpu_ix = 4; cpu_ix < num_possible_cpus(); cpu_ix++) {
				per_cpu(trace_pwr_down, cpu_ix) = 1;
			    }
                        }
                    }
			break;

		default:
			break;
		}

		break;

	default:
		break;
	}

	return NOTIFY_OK;
}

static struct notifier_block __cpuinitdata pftracer_notifier = {
	.notifier_call = restart_trace,
};

static int __init pftracer_init(void)
{
	int i, err;

#if defined(ETR_DRAM)
	/* DRAM */
	void *buff;
	dma_addr_t dma_handle;

	buff = dma_alloc_coherent(NULL, ETR_BUFF_SIZE, &dma_handle, GFP_KERNEL);
	if (!buff) {
		return -ENOMEM;
	}
	etb_driver_data.etr_virt = (u32)buff;
	etb_driver_data.etr_phys = dma_handle;
	etb_driver_data.etr_len = ETR_BUFF_SIZE;
	etb_driver_data.use_etr = 1;
	etb_driver_data.etb_regs = IOMEM(DEBUGTOP_BASE + 0x13000);
#elif defined(ETR_SRAM)
	/* SRAM */
	etb_driver_data.etr_virt = (u32)ETR_SRAM_VIRT_BASE;
	etb_driver_data.etr_phys = (dma_addr_t)ETR_SRAM_PHYS_BASE;
	etb_driver_data.etr_len = ETR_BUFF_SIZE;
	etb_driver_data.use_etr = 1;
	etb_driver_data.etb_regs = IOMEM(DEBUGTOP_BASE + 0x13000);
#else
	/* ETB */
	etb_driver_data.use_etr = 0;
	etb_driver_data.etb_regs = IOMEM(DEBUGTOP_BASE + 0x11000);
#endif

	for (i = 0; i < num_possible_cpus(); i++) {
		per_cpu(trace_pwr_down, i) = 0;
		etm_driver_data[i].pwr_down = &(per_cpu(trace_pwr_down, i));
	}

        for (i = 0; i < num_possible_cpus(); i++) {
            err = platform_device_register(&(etm_device[i]));
            if (err) {
                pr_err("Fail to register etm_device %d",i);
                return err;
            }
        }

	err = platform_device_register(&etb_device);
	if (err) {
		pr_err("Fail to register etb_device");
		return err;
	}

	register_cpu_notifier(&pftracer_notifier);

	return 0;
}

arch_initcall(pftracer_init);
