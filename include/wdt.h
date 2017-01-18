/*
 * Copyright 2017 Google, Inc
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef _WDT_H_
#define _WDT_H_

/*
 * Start the timer
 *
 * @dev: WDT Device
 * @timeout: Number of ticks to wait before executing the action.
 * @flags: Driver specific flags. This might be used to specify
 * 	which action needs to be executed when the timer expires
 * @return: 0 if OK, -ve on error
 */
int wdt_start(struct udevice *dev, u64 timeout, ulong flags);

/*
 * Stop the timer
 *
 * @dev: WDT Device
 * @return: 0 if OK, -ve on error
 */
int wdt_stop(struct udevice *dev);

/*
 * Restart the timer, typically restoring the counter to
 * the value configured by start()
 *
 * @dev: WDT Device
 * @return: 0 if OK, -ve on error
 */
int wdt_restart(struct udevice *dev);

/*
 * Execute the timer action immediately
 *
 * Will either use chosen wdt, besed on reset-wdt
 * chosen property, or the first one available.
 *
 * @flags: Driver specific flags
 * @return 0 if OK -ve on error. May not return.
 */
int wdt_reset(struct udevice *dev, ulong flags);

/*
 * struct wdt_ops - Driver model wdt operations
 *
 * The uclass interface is implemented by all wdt devices which use
 * driver model.
 */
struct wdt_ops {
	/*
	 * Start the timer
	 *
	 * @dev: WDT Device
	 * @timeout: Number of ticks to wait before executing the action.
	 * @flags: Driver specific flags. This might be used to specify
	 * 	which action needs to be executed when the timer expires
	 * @return: 0 if OK, -ve on error
	 */
	int (*start)(struct udevice *dev, u64 timeout, ulong flags);
	/*
	 * Stop the timer
	 *
	 * @dev: WDT Device
	 * @return: 0 if OK, -ve on error
	 */
	int (*stop)(struct udevice *dev);
	/*
	 * Restart the timer, typically restoring the counter to
	 * the value configured by start()
	 *
	 * @dev: WDT Device
	 * @return: 0 if OK, -ve on error
	 */
	int (*restart)(struct udevice *dev);
	/*
	 * Execute the timer action immediately (optional)
	 *
	 * If this function is not provided, default implementation
	 * will be used for wdt_reset(), which is set the counter to 1
	 * and wait forever. This is good enough for system level
	 * reset, but not goog enough for resetting peripherals.
	 *
	 * @dev: WDT Device
	 * @flags: Driver specific flags
	 * @return 0 if OK -ve on error. May not return.
	 */
	int (*reset)(struct udevice *dev, ulong flags);
};

#endif  /* _WDT_H_ */
