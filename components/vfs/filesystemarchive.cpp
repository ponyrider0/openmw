#include "filesystemarchive.hpp"

#include <components/misc/stringops.hpp>
#include <boost/filesystem.hpp>

namespace VFS
{

    FileSystemArchive::FileSystemArchive(const std::string &path)
        : mBuiltIndex(false)
        , mPath(path)
    {

    }

    void FileSystemArchive::listResources(std::map<std::string, File *> &out, char (*normalize_function)(char))
    {
        if (!mBuiltIndex)
        {
            typedef boost::filesystem::recursive_directory_iterator directory_iterator;

            directory_iterator end;

            size_t prefix = mPath.size ();

            if (mPath.size () > 0 && mPath [prefix - 1] != '\\' && mPath [prefix - 1] != '/')
                ++prefix;

            for (directory_iterator i (mPath); i != end; ++i)
            {
                if(boost::filesystem::is_directory (*i))
                    continue;

                std::string proper = i->path ().string ();

                FileSystemArchiveFile file(proper);

                std::string searchable;

                std::transform(proper.begin() + prefix, proper.end(), std::back_inserter(searchable), normalize_function);

                mIndex.insert (std::make_pair (searchable, file));
            }

            mBuiltIndex = true;
        }

        for (index::iterator it = mIndex.begin(); it != mIndex.end(); ++it)
        {
            out[it->first] = &it->second;
        }
    }

    bool FileSystemArchive::exists(const std::string &filename, char (*normalize_function) (char))
    {
        bool result = false;

        typedef boost::filesystem::recursive_directory_iterator directory_iterator;
        directory_iterator end;
        for (directory_iterator i (mPath); i != end; ++i)
        {
            if(boost::filesystem::is_directory (*i))
                continue;

            std::string member_name = i->path().string();
            std::transform(member_name.begin(), member_name.end(), member_name.begin(), normalize_function);

            if (Misc::StringUtils::lowerCase(member_name) == Misc::StringUtils::lowerCase(filename))
            {
                result = true;
            }
        }

        return result;
    }

    // ----------------------------------------------------------------------------------

    FileSystemArchiveFile::FileSystemArchiveFile(const std::string &path)
        : mPath(path)
    {
    }

    Files::IStreamPtr FileSystemArchiveFile::open()
    {
        return Files::openConstrainedFileStream(mPath.c_str());
    }

}
