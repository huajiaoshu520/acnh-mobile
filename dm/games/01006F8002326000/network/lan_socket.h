// Private ACNH LAN backend. Guest descriptors never expose host file handles.
#pragma once
#include <stdint.h>
#include <stddef.h>
int dm_lan_socket(int family, int type, int protocol);
int dm_lan_close(int fd);
void dm_lan_socket_fini(void);
int dm_lan_errno(void);
int dm_lan_error(int code);
int dm_lan_bind(int fd, const void *addr, unsigned len);
int dm_lan_connect(int fd, const void *addr, unsigned len);
int dm_lan_listen(int fd, int backlog);
int dm_lan_accept(int fd, void *addr, unsigned *len);
int dm_lan_name(int fd, void *addr, unsigned *len, int peer);
int dm_lan_shutdown(int fd, int how);
int dm_lan_fcntl(int fd, int cmd, int arg);
int dm_lan_opt(int fd, int level, int opt, void *buf, unsigned *len, int set);
long dm_lan_send(int fd, const void *buf, size_t size, int flags, const void *addr, unsigned len);
long dm_lan_recv(int fd, void *buf, size_t size, int flags, void *addr, unsigned *len);
int dm_lan_poll(void *fds, uint64_t n, int ms);
int dm_lan_select(int n, uint64_t *r, uint64_t *w, uint64_t *e, const int64_t *tv);
