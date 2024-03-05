#pragma once

#include <erebus/erebus.hxx>

#include <filesystem>
#include <vector>
#include <unordered_set>


namespace Er
{

namespace Util
{


enum class FileSearchMode
{
    FilesOnly,
    FoldersOnly,
    FilesAndFolders
};


template <typename PredicateT>
void searchFor(
    std::vector<std::string>& results, 
    std::string_view dir, 
    const std::unordered_set<std::string>* excludeDirs, 
    bool recursive,
    FileSearchMode mode,
    PredicateT pred
    )
{
    std::error_code ec;
    for (auto& d: std::filesystem::directory_iterator(dir, ec))
    {
        if (ec)
            continue;

        auto path = d.path().string();
        
        if (d.is_symlink(ec) || ec)
            continue;

        if (d.is_directory(ec) && !ec)
        {
            if (excludeDirs && (excludeDirs->find(path) != excludeDirs->end()))
                continue;

            if ((mode == FileSearchMode::FoldersOnly) || (mode == FileSearchMode::FilesAndFolders))
            {
                if (pred(path))
                    results.push_back(path); 
            }

            if (recursive)
                searchFor(results, path, excludeDirs, recursive, mode, pred);
        }
        else if (d.is_regular_file(ec) && !ec)
        {
            if ((mode == FileSearchMode::FilesOnly) || (mode == FileSearchMode::FilesAndFolders))
            {
                if (pred(path))
                    results.push_back(std::move(path));
            }
        }
    }
}


EREBUS_EXPORT std::string loadFile(const std::string& path);

} // namespace Util {}

} // namespace Er {]