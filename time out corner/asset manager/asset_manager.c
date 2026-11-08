#include "../common.h"
#include "asset_manager.h"
#include "loaders.h"

#include <stdlib.h>
#include <pthread.h>

#define MAX_ASSETS 256
#define JOB_QUEUE_SIZE 64

typedef enum AssetState {
    ASSET_REQUESTED,
    ASSET_READY,
} AssetState;

typedef struct Asset {
    AssetID id;
    size_t ref_count;

    AssetType type;

    void *cpu_data; // CPU sided stuff thats done
    void *gpu_data; // actual raylib representation that just so happens tp be what is necessary to push to GPU

    AssetState state;

    bool valid;
} Asset;

typedef struct AssetJob {
    AssetID id;
    const char *path;
    AssetType type;

    bool valid;
} AssetJob;

typedef struct JobQueue {
    AssetJob job_queue[JOB_QUEUE_SIZE];
    size_t read_pointer;
    size_t write_pointer;

    pthread_mutex_t mutex;
} JobQueue;

typedef struct AssetResult {
    AssetID id;
    void *data;
} AssetResult;

typedef struct ResultQueue {
    AssetResult job_queue[JOB_QUEUE_SIZE];
    size_t read_pointer;
    size_t write_pointer;

    pthread_mutex_t mutex;
} ResultQueue;

typedef struct AssetManager { // this is a global state object that only this file may access
    Asset assets[MAX_ASSETS];

    JobQueue jobs;
    ResultQueue completed;

    pthread_t worker;
} AssetManager;

typedef struct AssetLoader {
    void *(*load_cpu)(const char *path);
    void (*free_cpu)(void *data);

    void *(*load_gpu)(void *data);
    void (*free_gpu)(void *data);
} AssetLoader;

static AssetManager asset_manager = {0};

static void push_job(AssetJob job) {
    // pthread_mutex_lock(&asset_manager.jobs.mutex);

    asset_manager.jobs.job_queue[asset_manager.jobs.write_pointer] = job;

    asset_manager.jobs.write_pointer++;
    asset_manager.jobs.write_pointer %= JOB_QUEUE_SIZE;

    // pthread_mutex_unlock(&asset_manager.jobs.mutex);
}

static AssetJob get_next_job() {
    // pthread_mutex_lock(&asset_manager.jobs.mutex);

    if (asset_manager.jobs.read_pointer == asset_manager.jobs.write_pointer) {
        // pthread_mutex_unlock(&asset_manager.jobs.mutex);
        return (AssetJob){0};
    }

    AssetJob job = asset_manager.jobs.job_queue[asset_manager.jobs.read_pointer];

    asset_manager.jobs.read_pointer++;
    asset_manager.jobs.read_pointer %= JOB_QUEUE_SIZE;

    // pthread_mutex_unlock(&asset_manager.jobs.mutex);

    job.valid = true;

    return job;
}

static void completed_queue_push(AssetResult job) {
    // pthread_mutex_lock(&asset_manager.completed.mutex);

    asset_manager.completed.job_queue[asset_manager.completed.write_pointer] = job;

    asset_manager.completed.write_pointer++;
    asset_manager.completed.write_pointer %= JOB_QUEUE_SIZE;

    // pthread_mutex_unlock(&asset_manager.completed.mutex);
}

bool completed_queue_try_pop(AssetResult *result) {
    // pthread_mutex_lock(&asset_manager.completed.mutex);

    if (asset_manager.completed.read_pointer == asset_manager.completed.write_pointer) {
        // pthread_mutex_unlock(&asset_manager.completed.mutex);
        return false;
    }

    *result = asset_manager.completed.job_queue[asset_manager.completed.read_pointer];

    asset_manager.completed.read_pointer++;
    asset_manager.completed.read_pointer %= JOB_QUEUE_SIZE;

    // pthread_mutex_unlock(&asset_manager.completed.mutex);

    return true;
}

Asset *asset_get(AssetID id) {
    return &asset_manager.assets[id];
}

AssetID idx_to_id(size_t i) {
    return (AssetID)i;
}

AssetID asset_alloc() {
    for (size_t i = 0; i < MAX_ASSETS; i++) {
        if (!asset_manager.assets[i].valid) {
            return idx_to_id(i);
        }
    }

    return -1;
}

static const AssetLoader asset_loaders[ASSET_COUNT] = {
    [ASSET_TEXTURE] = {

    },
    [ASSET_SHADER] = {

    },
    [ASSET_SOUND] = {

    },
    [ASSET_FONT] = {
        LoadFontCPU,
        FreeFontCPU,

        LoadFontGPU,
        FreeFontGPU,
    },
    [ASSET_MODEL] = {
        nullptr,
        nullptr,

        nullptr,
        nullptr
    },
};

void *worker(void *arg) {
    for (;;) {
        AssetJob job = get_next_job();

        if (!job.valid) continue;

        void *data = asset_loaders[job.type].load_cpu(job.path);

        completed_queue_push((AssetResult){job.id, data});
    }

    return nullptr;
}

void asset_manager_init() {
    pthread_mutex_init(&asset_manager.jobs.mutex, nullptr);
    pthread_mutex_init(&asset_manager.completed.mutex, nullptr);

    pthread_create(&asset_manager.worker, nullptr, worker, nullptr);
}

void asset_manager_destroy() {
    pthread_join(asset_manager.worker, nullptr);

    pthread_mutex_destroy(&asset_manager.jobs.mutex);
    pthread_mutex_destroy(&asset_manager.completed.mutex);
}

AssetID asset_load_async(const char *path, const AssetType type) {
    if (type >= ASSET_COUNT) return -1;

    const Asset asset = {
        .id = asset_alloc(),
        .ref_count = 1,
        .valid = true,
        .type = type,
        .state = ASSET_REQUESTED
    };

    push_job(
        (AssetJob){
            .id = asset.id,
            .path = path,
            .type = type
        }
    );

    asset_manager.assets[asset.id] = asset;

    return asset.id;
}

AssetID asset_load(const char *path, const AssetType type) {
    if (type >= ASSET_COUNT) return -1;

    Asset asset = {
        .id = asset_alloc(),
        .ref_count = 1,
        .valid = true,
        .type = type,
    };

    const AssetLoader loader = asset_loaders[type];

    asset.cpu_data = loader.load_cpu(path);
    asset.gpu_data = loader.load_gpu(asset.cpu_data);

    asset.state = ASSET_READY;

    asset_manager.assets[asset.id] = asset;

    return asset.id;
}

void asset_manager_update() {
    AssetResult result;

    if (!completed_queue_try_pop(&result)) return;

    Asset *asset = asset_get(result.id);

    asset->cpu_data = result.data;

    asset->gpu_data = asset_loaders[asset->type].load_gpu(asset->cpu_data);

    asset->state = ASSET_READY;
}

void asset_acquire(const AssetID id) {
    Asset *asset = &asset_manager.assets[id];

    if (!asset->valid) return;

    asset->ref_count++;
}

void asset_release(const AssetID id) {
    Asset *asset = &asset_manager.assets[id];

    if (!asset->valid) return;

    asset->ref_count--;

    const AssetLoader loader = asset_loaders[asset->type];

    if (asset->ref_count == 0) {
        loader.free_gpu(asset->gpu_data);
        loader.free_cpu(asset->cpu_data);
    }
}

void *asset_get_id(const AssetID id) {
    if (id >= MAX_ASSETS) {
        return nullptr;
    }

    const Asset *asset = asset_get(id);

    if (!asset->valid) {
        return nullptr;
    }

    return asset->gpu_data;
}

