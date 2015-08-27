/* network.c - Networking demo */

/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Intel Corporation nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#if defined(CONFIG_STDOUT_CONSOLE)
#include <stdio.h>
#define PRINT           printf
#else
#include <misc/printk.h>
#define PRINT           printk
#endif

#ifdef CONFIG_MICROKERNEL
#include <zephyr.h>
#else
#include <nanokernel.h>
#endif

#include <net/net_core.h>
#include <net/net_socket.h>
#include <net_driver_15_4.h>

/* The following uIP includes are for testing purposes only. Never
 * ever use them in your application.
 */
#include "contiki/ipv6/uip-ds6-route.h"  /* to set the route */
#include "contiki/ipv6/uip-ds6-nbr.h"    /* to set the neighbor cache */

#if 0
#define SRC_IPADDR  { { { 0xfe,0x80,0,0,0,0,0,0,0x08,0xbe,0xef,0x2d,0xbc,0x15,0xf0,0x0d } } }
#define DEST_IPADDR { { { 0xfe,0x80,0,0,0,0,0,0,0x08,0xbe,0xef,0x2d,0xbc,0x15,0xf0,0x0d } } }
#define SRC_PORT       0xF0B1
#define DEST_PORT      0xF0B0

const struct in6_addr in6addr_src = SRC_IPADDR;
const struct in6_addr in6addr_dest = DEST_IPADDR;

/* source mac address */
uint8_t src_mac[] = { 0x0a, 0xbe, 0xef, 0x2d, 0xbc, 0x15, 0xf0, 0x0d };
/* destincation mac address */
const uip_lladdr_t dest_mac = { { 0x0a, 0xbe, 0xef, 0x2d, 0xbc, 0x15, 0xf0, 0x0d } };

#else

#define SRC_PORT       0
#define DEST_PORT      4242

const struct in6_addr in6addr_src = IN6ADDR_ANY_INIT; /* ::  */
const struct in6_addr in6addr_dest = IN6ADDR_LOOPBACK_INIT;  /* ::1 */
/* source mac address */
uint8_t src_mac[] = { 0x0a, 0xbe, 0xef, 0x2d, 0xbc, 0x15, 0xf0, 0x0d };
/* destincation mac address */
const uip_lladdr_t dest_mac = { };

#endif

static struct net_addr loopback_addr;
static struct net_addr any_addr;

/* Generated by http://www.lipsum.com/
 * 2 paragraphs, 185 words, 1230 bytes of Lorem Ipsum
 * The send_data() will add one null byte at the end and
 * 6lowpan needs one byte header so the maximum
 * length for the data to send is 1231 bytes.
 */
#if 0
static const char *lorem_ipsum =
	"Lorem ipsum dolor sit amet, consectetur adipiscing elit. Etiam congue non neque vel tempor. In id porta nibh, ut cursus tortor. Morbi eleifend tristique vehicula. Nunc vitae risus mauris. Praesent vel imperdiet dolor, et ultricies nibh. Aliquam erat volutpat.";
#else
static const char *lorem_ipsum =
	"Lorem ipsum dolor sit amet, consectetur adipiscing elit. Etiam congue non neque vel tempor. In id porta nibh, ut cursus tortor. Morbi eleifend tristique vehicula. Nunc vitae risus mauris. Praesent vel imperdiet dolor, et ultricies nibh. Aliquam erat volutpat. Maecenas pellentesque dolor vitae dictum tincidunt. Fusce vel nibh nec leo tristique auctor eu a massa. Nam et tellus ac tortor sollicitudin semper vitae nec tortor. Aliquam nec lacus velit. Maecenas ornare ullamcorper justo non auctor. Donec aliquam feugiat turpis, quis elementum sem rutrum ut. Sed eu ullamcorper libero, ut suscipit magna."
	"\n"
	"Donec vehicula magna ut varius aliquam. Ut vitae commodo nulla, quis ornare dolor. Nulla tortor sem, venenatis eu iaculis id, commodo ut massa. Sed est lorem, euismod vitae enim sed, hendrerit gravida felis. Donec eros lacus, auctor ut ultricies eget, lobortis quis nisl. Aliquam sit amet blandit eros. Interdum et malesuada fames ac ante ipsum primis in faucibus. Quisque egestas nisl leo, sed consectetur leo ornare eu. Suspendisse vitae urna vel purus maximus finibus. Proin sed sollicitudin turpis. Mauris interdum neque eu tellus pellentesque, id fringilla nisi fermentum. Suspendisse gravida pharetra sodales orci aliquam";
#endif

static inline void init_test()
{
	PRINT("%s: run 802.15.4 loopback tester\n", __FUNCTION__);

	net_set_mac(src_mac, sizeof(src_mac));

	any_addr.in6_addr = in6addr_src;
	any_addr.family = AF_INET6;

	loopback_addr.in6_addr = in6addr_dest;
	loopback_addr.family = AF_INET6;
}

static void set_routes()
{
	/* Workaround to get packets from this task to listening fiber.
	 * Do not attempt to do anything like this in live environment.
	 */
	if (!uip_ds6_nbr_add((uip_ipaddr_t *)&in6addr_dest,
					&dest_mac, 0, NBR_REACHABLE))
		PRINT("Cannot add neighbor cache\n");

	if (!uip_ds6_route_add((uip_ipaddr_t *)&in6addr_dest, 128,
					(uip_ipaddr_t *)&in6addr_dest))
		PRINT("Cannot add localhost route\n");
}

static void send_data(const char *taskname, struct net_context *ctx)
{
	int len = strlen(lorem_ipsum);
	struct net_buf *buf;

	buf = net_buf_get_tx(ctx);
	if (buf) {
		uint8_t *ptr;
		uint16_t sent_len;

		ptr = net_buf_add(buf, 0);
		memcpy(ptr, lorem_ipsum, len);
		ptr = net_buf_add(buf, len);
		ptr = net_buf_add(buf, 1); /* add \0 */
		*ptr = '\0';
		sent_len = buf->len;

		if (net_send(buf) < 0) {
			PRINT("%s: %s(): sending %d bytes failed\n",
			      taskname, __FUNCTION__, len);
			net_buf_put(buf);
		} else {
			PRINT("%s: %s(): sent %d bytes\n", taskname,
			      __FUNCTION__, sent_len);
		}
	}
}

static void receive_data(const char *taskname, struct net_context *ctx)
{
	struct net_buf *buf;

	buf = net_receive(ctx, TICKS_NONE);
	if (buf) {
		PRINT("%s: %s(): received %d bytes\n%s\n", taskname,
		      __FUNCTION__, net_buf_datalen(buf), net_buf_data(buf));
		if (memcmp(net_buf_data(buf), lorem_ipsum, sizeof(lorem_ipsum))) {
			PRINT("Error: data does not match\n");
		}
		net_buf_put(buf);
	}
}

static struct net_context *get_context(const struct net_addr *remote,
				       uint16_t remote_port,
				       const struct net_addr *local,
				       uint16_t local_port)
{
	struct net_context *ctx;

	ctx = net_context_get(IPPROTO_UDP,
			      remote, remote_port,
			      local, local_port);
	if (!ctx) {
		PRINT("%s: Cannot get network context\n", __FUNCTION__);
		return NULL;
	}

	return ctx;
}

#ifdef CONFIG_MICROKERNEL

/*
 * Microkernel version of hello world demo has two tasks that utilize
 * semaphores and sleeps to take turns printing a greeting message at
 * a controlled rate.
 */

/* specify delay between greetings (in ms); compute equivalent in ticks */

#define SLEEPTIME  500
#define SLEEPTICKS (SLEEPTIME * sys_clock_ticks_per_sec / 1000)

/* specify delay between greetings (in ms); compute equivalent in ticks */

#define SLEEPTIME  500
#define SLEEPTICKS (SLEEPTIME * sys_clock_ticks_per_sec / 1000)

/*
 *
 * @param taskname    task identification string
 * @param mySem       task's own semaphore
 * @param otherSem    other task's semaphore
 *
 */
static void listen(const char *taskname, ksem_t mySem, ksem_t otherSem,
		   struct net_context *ctx)
{
	while (1) {
		task_sem_take_wait(mySem);

		receive_data(taskname, ctx);

		/* wait a while, then let other task have a turn */
		task_sleep(SLEEPTICKS);
		task_sem_give(otherSem);
	}
}

void taskA(void)
{
	struct net_context *ctx;

	net_init();
	init_test();

	ctx = get_context(&any_addr, SRC_PORT, &loopback_addr, DEST_PORT);
	if (!ctx) {
		PRINT("%s: Cannot get network context\n", __FUNCTION__);
		return;
	}

	/* taskA gives its own semaphore, allowing it to say hello right away */
	task_sem_give(TASKASEM);

	listen(__FUNCTION__, TASKASEM, TASKBSEM, ctx);
}

static void send(const char *taskname, ksem_t mySem, ksem_t otherSem,
		 struct net_context *ctx)
{
	while (1) {
		task_sem_take_wait(mySem);

		send_data(taskname, ctx);

		/* wait a while, then let other task have a turn */
		task_sleep(SLEEPTICKS);
		task_sem_give(otherSem);
	}
}

void taskB(void)
{
	struct net_context *ctx;

	ctx = get_context(&loopback_addr, DEST_PORT, &any_addr, SRC_PORT);
	if (!ctx) {
		PRINT("%s: Cannot get network context\n", __FUNCTION__);
		return;
	}

	set_routes();

	send(__FUNCTION__, TASKBSEM, TASKASEM, ctx);
}

#else /*  CONFIG_NANOKERNEL */

/*
 * Nanokernel version of hello world demo has a task and a fiber that utilize
 * semaphores and timers to take turns printing a greeting message at
 * a controlled rate.
 */

#include <nanokernel.h>

/* specify delay between greetings (in ms); compute equivalent in ticks */

#define SLEEPTIME  2000
#define SLEEPTICKS (SLEEPTIME * sys_clock_ticks_per_sec / 1000)

#define STACKSIZE 2000

char fiberStack[STACKSIZE];

struct nano_sem nanoSemTask;
struct nano_sem nanoSemFiber;

void fiberEntry(void)
{
	struct nano_timer timer;
	uint32_t data[2] = {0, 0};
	struct net_context *ctx;

	ctx = get_context(&any_addr, SRC_PORT, &loopback_addr, DEST_PORT);
	if (!ctx) {
		PRINT("%s: Cannot get network context\n", __FUNCTION__);
		return;
	}

	nano_sem_init(&nanoSemFiber);
	nano_timer_init(&timer, data);

	while (1) {
		/* wait for task to let us have a turn */
		nano_fiber_sem_take_wait(&nanoSemFiber);

		receive_data("listenFiber", ctx);

		/* wait a while, then let task have a turn */
		nano_fiber_timer_start(&timer, SLEEPTICKS);
		nano_fiber_timer_wait(&timer);
		nano_fiber_sem_give(&nanoSemTask);
	}
}

void main(void)
{
	struct nano_timer timer;
	uint32_t data[2] = {0, 0};
	struct net_context *ctx;

	PRINT("%s: run test_15_4\n", __FUNCTION__);

	net_init();
	init_test();

	ctx = get_context(&loopback_addr, DEST_PORT, &any_addr, SRC_PORT);
	if (!ctx) {
		PRINT("Cannot get network context\n");
		return;
	}

	task_fiber_start(&fiberStack[0], STACKSIZE,
			(nano_fiber_entry_t) fiberEntry, 0, 0, 7, 0);

	nano_sem_init(&nanoSemTask);
	nano_timer_init(&timer, data);

	set_routes();

	while (1) {
		/* wait a while, then let fiber have a turn */
		nano_task_timer_start(&timer, SLEEPTICKS);
		nano_task_timer_wait(&timer);
		nano_task_sem_give(&nanoSemFiber);

		send_data("sendFiber", ctx);

		/* now wait for fiber to let us have a turn */
		nano_task_sem_take_wait(&nanoSemTask);
	}
}

#endif /* CONFIG_MICROKERNE ||  CONFIG_NANOKERNEL */
