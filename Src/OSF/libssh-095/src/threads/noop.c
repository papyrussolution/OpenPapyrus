/*
 * This file is part of the SSH Library
 *
 * Copyright (c) 2018 by Anderson Toshiyuki Sasaki
 *
 * The SSH Library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at your option) any later version.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with the SSH Library; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
 */
#include <libssh-internal.h>
#pragma hdrstop

static int threads_noop(void ** lock)
{
	(void)lock;
	return 0;
}

static ulong threads_id_noop()
{
	return 1;
}

static struct ssh_threads_callbacks_struct ssh_threads_noop = {
	"threads_noop", threads_noop, threads_noop, threads_noop, threads_noop, threads_id_noop
};

/* Threads interface implementation */

#if !(HAVE_PTHREAD) && !(defined _WIN32 || defined _WIN64)
	void ssh_mutex_lock(SSH_MUTEX * mutex)
	{
		(void)mutex;
		return;
	}

	void ssh_mutex_unlock(SSH_MUTEX * mutex)
	{
		(void)mutex;
		return;
	}

	struct ssh_threads_callbacks_struct * ssh_threads_get_default()
	{
		return &ssh_threads_noop;
	}
#endif

struct ssh_threads_callbacks_struct * ssh_threads_get_noop()
{
	return &ssh_threads_noop;
}
