/*
 * Copyright 2017 Google, Inc
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <dm.h>
#include <errno.h>
#include <wdt.h>
#include <dm/lists.h>
#include <dm/device-internal.h>

DECLARE_GLOBAL_DATA_PTR;

/*
 * Implement a simple watchdog uclass. Watchdog is basically a timer that
 * is used to detect or recover from malfunction. During normal operation
 * the watchdog would be regularly reset to prevent it from timing out.
 * If, due to a hardware fault or program error, the computer fails to reset
 * the watchdog, the timer will elapse and generate a timeout signal.
 * The timeout signal is used to initiate corrective action or actions,
 * which typically include placing the system in a safe, known state.
 */

int wdt_start(struct udevice *dev, u64 timeout, ulong flags)
{
	const struct wdt_ops *ops = device_get_ops(dev);

	if (!ops->start)
		return -ENOSYS;

	return ops->start(dev, timeout, flags);
}

int wdt_stop(struct udevice *dev)
{
	const struct wdt_ops *ops = device_get_ops(dev);

	if (!ops->stop)
		return -ENOSYS;

	return ops->stop(dev);
}

int wdt_restart(struct udevice *dev)
{
	const struct wdt_ops *ops = device_get_ops(dev);

	if (!ops->restart)
		return -ENOSYS;

	return ops->restart(dev);
}

int wdt_reset(struct udevice *dev, ulong flags)
{
	const struct wdt_ops *ops;

	debug("WDT Resettting: %lu\n", flags);
 	ops = device_get_ops(dev);
	if (ops->reset)
		return ops->reset(dev, flags);
	else {
		if (!ops->start)
			return -ENOSYS;

		ops->start(dev, 1, flags);
		while (1)
			;
	}

	return 0;
}

UCLASS_DRIVER(wdt) = {
	.id		= UCLASS_WDT,
	.name		= "wdt",
	.flags		= DM_UC_FLAG_SEQ_ALIAS,
};
