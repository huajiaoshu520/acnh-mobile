#pragma once
// Private LAN-only opaque history identifiers, not Nintendo account credentials.
// The SDK/game transports these 64 bytes opaquely. Our AddPlayHistory service
// recognizes this namespace and actually persists records locally.
int dm_lan_history_key(void *key64,const void *uid16,int local);
int dm_lan_history_add(const void *uid16,const void *key64,const void *mine72,const void *peer72);

void dm_account_local_uid(void *uid16);
