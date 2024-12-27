//
// Created by eli on 8/17/2022.
//

#include "fileWatcher.h"
#include "fileManager.h"
#include "utility/serializedData.h"

void FileWatcher::watchDirectory(const std::filesystem::path& directory) { _watchedDirectories.push_back(directory); }

void FileWatcher::addFileWatcher(const std::string& ext, std::function<void(const std::filesystem::path&)> callback)
{
    _fileWatchers.insert({ext, callback});
}

void FileWatcher::scanForChanges(bool callbacks)
{
    for(auto& wd : _watchedDirectories)
    {
        for(auto& file : std::filesystem::recursive_directory_iterator{wd})
        {
            if(!file.is_regular_file())
                continue;
            std::string ext = file.path().extension().string();
            if(_fileWatchers.count(ext))
            {
                auto currentUpdate = std::filesystem::last_write_time(file.path());
                std::string strPath = file.path().string();
                if(!_lastUpdate.count(strPath) || _lastUpdate.at(strPath) != currentUpdate)
                {
                    if(callbacks)
                        _fileWatchers[ext](file.path());
                    _lastUpdate[strPath] = currentUpdate;
                }
            }
        }
    }

    saveCache();
}

void FileWatcher::loadCache(std::filesystem::path changeCache)
{
    SerializedData cacheData;
    _changeCache = std::move(changeCache);
    if(!FileManager::readFile(_changeCache, cacheData.vector()))
        return;
    InputSerializer s(cacheData);
    uint32_t count = 0;
    s >> count;
    for(uint32_t i = 0; i < count; ++i)
    {
        std::string path;
        std::filesystem::file_time_type lastUpdate;
        s >> path >> lastUpdate;
        if(!std::filesystem::exists(path))
            continue;
        _lastUpdate.insert({path, lastUpdate});
    }
}

void FileWatcher::saveCache()
{
    if(_changeCache.empty())
        return;
    SerializedData cacheData;
    OutputSerializer s(cacheData);
    uint32_t count = _lastUpdate.size();
    s << count;
    for(auto& u : _lastUpdate)
        s << u.first << u.second;
    FileManager::writeFile(_changeCache, cacheData.vector());
}

FileWatcher::~FileWatcher() { saveCache(); }
