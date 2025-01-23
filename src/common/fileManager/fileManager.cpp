#include "fileManager.h"
#include <fstream>
#include <tinyfiledialogs.h>
#include "gtest/internal/gtest-port.h"
#include <config/config.h>
#include <openssl/md5.h>
#include <utility/hex.h>
#include <utility/serializedData.h>
#include <utility/strCaseCompare.h>

void FileManager::writeAsset(const Asset* asset, const std::filesystem::path& filename)
{
    SerializedData data;
    OutputSerializer s(data);
    asset->serialize(s);

    std::filesystem::path path{filename};
    std::filesystem::create_directories(path.parent_path());

    std::ofstream f(path, std::ios::out | std::ofstream::binary);
    f.write((char*)data.data(), data.size());
    f.close();
}

bool FileManager::readFile(const std::filesystem::path& filename, std::string& data)
{
    std::ifstream f(filename, std::ios::binary | std::ios::ate);
    if(!f.is_open())
        return false;

    data.resize(f.tellg());
    f.seekg(0);
    f.read((char*)data.data(), data.size());
    f.close();

    return true;
}

bool FileManager::readFile(const std::filesystem::path& filename, Json::Value& data)
{
    std::ifstream f(filename, std::ios::binary);
    if(!f.is_open())
        return false;

    f >> data;
    f.close();
    return true;
}

void FileManager::writeFile(const std::filesystem::path& filename, const std::string& data)
{
    std::filesystem::path path{filename};
    std::filesystem::create_directories(path.parent_path());

    std::ofstream f(path, std::ios::out | std::ofstream::binary);
    f.write((char*)data.data(), data.size());
    f.close();
}

void FileManager::writeFile(const std::filesystem::path& filename, const Json::Value& data)
{
    std::filesystem::path path{filename};
    std::filesystem::create_directories(path.parent_path());

    Json::StreamWriterBuilder builder;
    builder["indentation"] = "  ";
    auto* writer = builder.newStreamWriter();

    std::ofstream f(path, std::ios::out | std::ofstream::binary);
    writer->write(data, &f);
    f.close();

    delete writer;
}

AsyncData<Asset*> FileManager::async_readUnknownAsset(const std::filesystem::path& filename)
{
    AsyncData<Asset*> asset;
    ThreadPool::enqueue([this, filename, asset] { asset.setData(readUnknownAsset(filename)); });
    return asset;
}

const char* FileManager::name()
{
    return "fileManager";
}

FileManager::FileManager() {}

std::vector<std::filesystem::directory_entry> FileManager::getDirectoryContents(const std::filesystem::path& path)
{
    std::vector<std::filesystem::directory_entry> paths;
    for(auto& file : std::filesystem::directory_iterator(path))
        paths.push_back(file);
    std::sort(
        paths.begin(), paths.end(), [](std::filesystem::directory_entry& p1, std::filesystem::directory_entry& p2) {
        if(p1.is_directory() != p2.is_directory())
            return std::filesystem::is_directory(p1);
        return strCaseCompare(p1.path().filename().string(), p2.path().filename().string());
    });

    return paths;
}

std::unique_ptr<FileManager::Directory> FileManager::getDirectoryTree(const std::filesystem::path& path)
{
    auto d = std::make_unique<Directory>();
    d->name = path.filename().string();
    d->open = true;
    for(auto& file : std::filesystem::directory_iterator{path})
    {
        if(file.is_directory())
        {
            auto c = getDirectoryTree(file);
            c->parent = d.get();
            d->children.push_back(std::move(c));
        }
    }
    std::sort(
        d->children.begin(), d->children.end(), [](auto& a, auto& b) { return strCaseCompare(a->name, b->name); });

    return std::move(d);
}

void FileManager::refreshDirectoryTree(Directory* dir, const std::filesystem::path& root)
{
    auto& children = dir->children;
    children.erase(
        std::remove_if(children.begin(), children.end(), [](auto& c) { return !std::filesystem::exists(c->path()); }),
        children.end());
    for(auto& file : std::filesystem::directory_iterator{root / dir->path()})
    {
        if(!file.is_directory())
            continue;

        bool found = false;
        for(auto& c : children)
        {
            if(c->name == file.path().filename())
            {
                found = true;
                break;
            }
        }
        if(!found)
        {
            auto c = getDirectoryTree(file);
            c->parent = dir;
            children.push_back(std::move(c));
        }
    }
    std::sort(children.begin(), children.end(), [](auto& a, auto& b) { return strCaseCompare(a->name, b->name); });
}

void FileManager::createDirectory(const std::filesystem::path& path)
{
    std::filesystem::create_directories(path);
}

bool FileManager::deleteFile(const std::filesystem::path& path)
{
    return std::filesystem::remove_all(path);
}

void FileManager::moveFile(const std::filesystem::path& source, const std::filesystem::path& destination)
{
    std::filesystem::rename(source, destination);
}

std::string FileManager::requestLocalFilePath(const std::string& title, const std::vector<const char*>& filters)
{
    char* path = tinyfd_openFileDialog(title.c_str(), NULL, static_cast<int>(filters.size()), filters.data(), NULL, 0);
    if(!path)
        return "";
    return std::string(path);
}

std::string FileManager::checksum(const std::filesystem::path& filename)
{
    std::vector<unsigned char> fileContents;
    readFile(filename, fileContents);

    MD5_CTX ctx;
    MD5_Init(&ctx);
    MD5_Update(&ctx, fileContents.data(), fileContents.size());

    // Didn't want to include md5.h in header, so just making sure I got the length right
    std::array<unsigned char, MD5_DIGEST_LENGTH> hash;
    MD5_Final((unsigned char*)hash.data(), &ctx);

    return toHex(hash);
}

std::filesystem::path FileManager::Directory::path() const
{
    if(parent)
        return parent->path() / name;
    return "";
}

void FileManager::Directory::setParentsOpen()
{
    if(parent)
    {
        parent->open = true;
        parent->setParentsOpen();
    }
}
