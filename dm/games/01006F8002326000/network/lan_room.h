// ACNH SDK layout, cross-checked with SDK load offsets and public libnx LDN ABI.
#pragma once
#include <stdint.h>
#include <stddef.h>
typedef struct {uint64_t communication_id;uint16_t reserved;uint16_t scene;uint32_t reserved2;} lan_intent;
typedef struct {lan_intent intent;uint8_t session[16];} lan_id;
typedef struct {uint32_t ip;uint8_t mac[6];int8_t id;uint8_t connected;char name[33];uint8_t platform;int16_t version;uint8_t reserved[16];} lan_node;
typedef struct {uint8_t mac[6];uint8_t ssid_length;char ssid[33];int16_t channel;int8_t link;uint8_t type;uint8_t reserved[4];} lan_common;
typedef struct {lan_id id;lan_common common;uint8_t random[16];uint16_t security;uint8_t policy,version,reserved[2];int8_t max_nodes;uint8_t count;lan_node nodes[8];uint8_t reserved2[2];uint16_t advert_size;uint8_t advert[384];uint8_t reserved3[140];uint64_t reserved4;} lan_info;
typedef struct {lan_intent intent;int16_t channel;int8_t max_nodes;uint8_t reserved;int16_t version;uint8_t reserved2[10];} lan_config;
typedef struct {uint16_t mode,length;uint8_t key[64];} lan_security;
typedef struct {char name[33];uint8_t reserved[15];} lan_user;
typedef struct {lan_id id;uint32_t type;uint8_t mac[6];uint8_t ssid_length;char ssid[33];uint8_t reserved[16];uint32_t flags;} lan_filter;
_Static_assert(sizeof(lan_info)==0x480,"NetworkInfo size");
_Static_assert(offsetof(lan_info,nodes)==0x68,"NetworkInfo nodes");
_Static_assert(offsetof(lan_info,advert_size)==0x26a,"NetworkInfo advertise length");
_Static_assert(sizeof(lan_node)==0x40,"NodeInfo size");
_Static_assert(sizeof(lan_filter)==0x60,"ScanFilter size");
_Static_assert(sizeof(lan_config)==0x20,"NetworkConfig size");
_Static_assert(sizeof(lan_security)==0x44,"SecurityConfig size");
_Static_assert(sizeof(lan_user)==0x30,"UserConfig size");
int dm_room_init(void (*changed)(void));
void dm_room_fini(void);
int dm_room_state(void);
int dm_room_open(int ap);
int dm_room_close(int ap);
int dm_room_create(const lan_config *,const lan_security *,const lan_user *,const void *private_parameter);
int dm_room_scan(lan_info *,int *count,int capacity,const lan_filter *,int private_scan);
int dm_room_connect(const lan_info *,const lan_security *,const lan_user *,int version);
int dm_room_disconnect(int destroy);
int dm_room_info(lan_info *);
int dm_room_config(lan_config *);
int dm_room_security(void *);
int dm_room_advert(const void *,size_t);
int dm_room_policy(int);
int dm_room_filter(uint64_t);
int dm_room_filters(const void *,int);
int dm_room_reject(uint32_t);
int dm_room_reason(void);
void dm_room_local_mode(int enabled);
int dm_room_network_ip(uint32_t *,uint32_t *);
int dm_room_ip(uint32_t *,uint32_t *);
// Host-order IP of the joined peer, or zero. Used to unicast LDN broadcast data.
uint32_t dm_room_peer_ip(void);

// Nonblocking platform notice; weak logging default, native iOS override.
void front_lan_failed(int error,const char *stage);
