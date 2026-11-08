#ifndef FOOTIE_LIFE_ASSET_MANAGER_H
#define FOOTIE_LIFE_ASSET_MANAGER_H

#include <ints.h>

typedef i64 AssetID;

typedef enum AssetType {
    ASSET_TEXTURE,
    ASSET_SHADER,
    ASSET_SOUND,
    ASSET_FONT,
    ASSET_MODEL,
    // ...
    ASSET_COUNT
} AssetType;

void asset_manager_init();
void asset_manager_destroy();

AssetID asset_load_async(const char *path, AssetType type);
AssetID asset_load(const char *path, AssetType type); // is synchronous

// used when doing asynchronous loads
void asset_manager_update();

void asset_acquire(AssetID id);
void asset_release(AssetID id);
void *asset_get_id(AssetID id);

#endif //FOOTIE_LIFE_ASSET_MANAGER_H
