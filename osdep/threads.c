/*
 * This file is part of mpv.
 *
 * mpv is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * mpv is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with mpv.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <errno.h>
#include <pthread.h>

#include "common/common.h"
#include "config.h"
#include "threads.h"
#include "timer.h"

#if HAVE_BSD_THREAD_NAME
#include <pthread_np.h>
#endif

int mpthread_mutex_init_recursive(pthread_mutex_t *mutex)
{
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    int r = pthread_mutex_init(mutex, &attr);
    pthread_mutexattr_destroy(&attr);
    return r;
}

void mpthread_set_name(const char *name)
{
#if HAVE_GLIBC_THREAD_NAME
    if (pthread_setname_np(pthread_self(), name) == ERANGE) {
        char tname[16] = {0}; // glibc-checked kernel limit
        strncpy(tname, name, sizeof(tname) - 1);
        pthread_setname_np(pthread_self(), tname);
    }
#elif HAVE_WIN32_INTERNAL_PTHREADS || HAVE_BSD_THREAD_NAME
    pthread_set_name_np(pthread_self(), name);
#elif HAVE_OSX_THREAD_NAME
    pthread_setname_np(name);
#endif
}

int mp_ptwrap_check(const char *file, int line, int res)
{
    if (res && res != ETIMEDOUT) {
        fprintf(stderr, "%s:%d: internal error: pthread result %d (%s)\n",
                file, line, res, mp_strerror(res));
        abort();
    }
    return res;
}

int mp_ptwrap_mutex_init(const char *file, int line, pthread_mutex_t *m,
                         const pthread_mutexattr_t *attr)
{
    pthread_mutexattr_t m_attr;
    if (!attr) {
        attr = &m_attr;
        pthread_mutexattr_init(&m_attr);
        // Force normal mutexes to error checking.
        pthread_mutexattr_settype(&m_attr, PTHREAD_MUTEX_ERRORCHECK);
    }
    int res = mp_ptwrap_check(file, line, (pthread_mutex_init)(m, attr));
    if (attr == &m_attr)
        pthread_mutexattr_destroy(&m_attr);
    return res;
}

int mp_ptwrap_mutex_trylock(const char *file, int line, pthread_mutex_t *m)
{
    int res = (pthread_mutex_trylock)(m);

    if (res != EBUSY)
        mp_ptwrap_check(file, line, res);

    return res;
}
