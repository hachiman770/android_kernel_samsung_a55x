#ifndef __ACPM_IPC_H_
#define __ACPM_IPC_H_

#include <soc/samsung/acpm_ipc_ctrl.h>
#include "fw_header/framework.h"

struct buff_info {
	void __iomem *rear;
	void __iomem *front;
	void __iomem *base;
	void __iomem *direction;

	unsigned int size;
	unsigned int len;
	unsigned int d_buff_size;
};

struct callback_info {
	void (*ipc_callback) (unsigned int *cmd, unsigned int size);
	struct device_node *client;
	struct list_head list;
};

#define SEQUENCE_NUM_MAX		(64)
struct acpm_ipc_ch {
	struct buff_info rx_ch;
	struct buff_info tx_ch;
	struct list_head list;

	unsigned int id;
	unsigned int type;
	unsigned int seq_num;
	u32 seq_num_flag[SEQUENCE_NUM_MAX];
	unsigned int *cmd;
	spinlock_t rx_lock;
	spinlock_t tx_lock;
	spinlock_t ch_lock;
	struct mutex wait_lock;

	struct completion wait;
	bool polling;
	bool interrupt;
};

struct acpm_ipc_info {
	unsigned int num_channels;
	struct device *dev;
	struct acpm_ipc_ch *channel;
	unsigned int irq;
	void __iomem *intr;
	void __iomem *sram_base;
	struct acpm_framework *initdata;
	unsigned int initdata_base;
	unsigned int intr_status;
	bool is_mailbox_master;
};

struct acpm_debug_info {
	unsigned int period;
	void __iomem *time_index;
	unsigned int num_timestamps;
	unsigned long long *timestamps;

	void __iomem *log_buff_rear;
	void __iomem *log_buff_front;
	void __iomem *log_buff_base;
	unsigned int log_buff_len;
	unsigned int log_buff_size;
#if !defined(CONFIG_SOC_S5E9935) && !defined(CONFIG_SOC_S5E9945)
	void __iomem *dump_base;
	unsigned int dump_size;
	void __iomem *dump_dram_base;
#else
	void __iomem *apm0_dump_base;
	unsigned int apm0_dump_size;
	void __iomem *apm0_dump_dram_base;
	void __iomem *apm1_dump_base;
	unsigned int apm1_dump_size;
	void __iomem *apm1_dump_dram_base;
#endif
	unsigned int debug_log_level;
	struct acpm_ipc_info *ipc_info;
	struct delayed_work periodic_work;
	struct work_struct update_log_work;

	spinlock_t lock;
};

#define LOG_ID_SHIFT				(28)
#define LOG_TIME_INDEX				(20)
#define LOG_LEVEL				(19)
#define INTGR0					0x0008
#define INTCR0					0x000C
#define INTMR0					0x0010
#define INTSR0					0x0014
#define INTMSR0					0x0018
#define INTGR1					0x001C
#define INTCR1					0x0020
#define INTMR1					0x0024
#define INTSR1					0x0028
#define INTMSR1					0x002C
#define SR0					0x0080
#define SR1					0x0084
#define SR2					0x0088
#define SR3					0x008C

#define IPC_TIMEOUT				(15000000)
#define APM_PERITIMER_NS_PERIOD			(10416)

#define UNTIL_EQUAL(arg0, arg1, flag)			\
do {							\
	u64 timeout = sched_clock() + IPC_TIMEOUT;	\
	bool t_flag = true;				\
	do {						\
		if ((arg0) == (arg1)) {			\
			t_flag = false;			\
			break;				\
		} else {				\
			cpu_relax();			\
		}					\
	} while (timeout >= sched_clock());		\
	if (t_flag) {					\
		pr_err("%s %d Timeout error!\n",	\
				__func__, __LINE__);	\
	}						\
	(flag) = t_flag;				\
} while(0)

extern void acpm_log_print(void);
extern void timestamp_write(void);
extern void acpm_ramdump(void);
extern void acpm_fw_log_level(unsigned int on);

extern int acpm_ipc_remove(struct platform_device *pdev);
extern int acpm_ipc_probe(struct platform_device *pdev);

void __iomem *get_acpm_sram_base(void);
#endif
