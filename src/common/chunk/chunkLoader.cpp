//
// Created by eli on 9/18/2022.
//

#include "chunkLoader.h"
#include "assets/assetManager.h"
#include "utility/threadPool.h"

void ChunkLoader::loadChunk(WorldChunk* chunk)
{

    _chunkLock.lock();
    _chunks.insert({chunk->id, {chunk}});
    _chunkLock.unlock();

    assert(chunk->maxLOD != NullLOD);
    if(chunk->maxLOD != NullLOD)
        setChunkLOD(chunk->id, chunk->maxLOD);
}

void ChunkLoader::unloadChunk(const AssetID& chunk)
{
    setChunkLOD(chunk, NullLOD);
    std::scoped_lock l(_chunkLock);
    _chunks.erase(chunk);
}

ChunkCallbackID ChunkLoader::addOnLODChangeCallback(ChunkLODCallback callback)
{
    return _onLODChange.push(std::move(callback));
}

void ChunkLoader::removeOnLODChangeCallback(ChunkCallbackID id)
{
    assert(_onLODChange.hasIndex(id));
    _onLODChange.remove(id);
}

void ChunkLoader::setChunkLOD(const AssetID& chunk, uint32_t lod)
{
    _chunkLock.lock_shared();
    assert(_chunks.contains(chunk));
    auto& cctx = _chunks.at(chunk);
    _chunkLock.unlock_shared();
    assert(lod <= cctx.chunk->maxLOD);

    uint32_t oldLod = cctx.lod;
    cctx.lod = lod;

    std::vector<WorldChunk::LOD*> LODsToLoad;
    for(auto& l : cctx.chunk->LODs)
    {
        if(l.min <= lod && lod <= l.max)
            LODsToLoad.push_back(&l);
    }

    WorldChunk* chunkPtr = cctx.chunk;

    auto cJob = ThreadPool::conditionalEnqueue(
        [this, chunkPtr, oldLod, lod]() {
        for(auto& callback : _onLODChange)
            callback(chunkPtr, oldLod, lod);
        },
        LODsToLoad.size());

    auto* am = Runtime::getModule<AssetManager>();
    for(auto& l : LODsToLoad)
    {
        AssetID id = l->assembly;
        if(id.address().empty())
            id.setAddress(chunkPtr->id.address());
        am->fetchAsset<Assembly>(id).then(
                                        [cJob](Assembly* asset) {
            cJob->signal();
        }).onError([](const std::string& error) { Runtime::error("Could not load chunk lod: " + error); });
    }
}

const char* ChunkLoader::name() { return "chunkLoader"; }
