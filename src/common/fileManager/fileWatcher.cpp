//
// Created by eli on 8/17/2022.
//

#include "fileWatcher.h"

void FileWatcher::watchDirectory(const std::filesystem::path& directory)
{
	_watchedDirectories.push_back(directory);
}

void FileWatcher::addFileWatcher(const std::string& ext, std::function<void(const std::filesystem::path&)> callback)
{
	_fileWatchers.insert({ext, callback});
}

void FileWatcher::scanForChanges()
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
				auto lastUpdate = _lastUpdate[file.path().string()];
				auto currentUpdate = std::filesystem::last_write_time(file.path());
				if(lastUpdate != currentUpdate)
				{
					_fileWatchers[ext](file.path());
					_lastUpdate[file.path().string()] = currentUpdate;
				}

			}
		}
	}
}