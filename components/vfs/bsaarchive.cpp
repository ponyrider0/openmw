#include "bsaarchive.hpp"

#include <components/misc/stringops.hpp>

namespace VFS
{


BsaArchive::BsaArchive(const std::string &filename)
: mFilename(filename)
{
    mFile.open(filename);

    const Bsa::BSAFile::FileList &filelist = mFile.getList();
    for(Bsa::BSAFile::FileList::const_iterator it = filelist.begin();it != filelist.end();++it)
    {
        mResources.push_back(BsaArchiveFile(&*it, &mFile));
    }
}

void BsaArchive::listResources(std::map<std::string, File *> &out, char (*normalize_function)(char))
{
    for (std::vector<BsaArchiveFile>::iterator it = mResources.begin(); it != mResources.end(); ++it)
    {
        std::string ent = it->mInfo->name;
        std::transform(ent.begin(), ent.end(), ent.begin(), normalize_function);

        out[ent] = &*it;
    }
}

bool BsaArchive::exists(const std::string &filename, char (*normalize_function) (char))
{
    bool result = false;

    for (std::vector<BsaArchiveFile>::iterator it = mResources.begin(); it != mResources.end(); ++it)
    {
        std::string member_name = it->mInfo->name;
        std::transform(member_name.begin(), member_name.end(), member_name.begin(), normalize_function);

        if (Misc::StringUtils::lowerCase(member_name) == Misc::StringUtils::lowerCase(filename))
        {
            result = true;
        }
    }

    return result;
}

// ------------------------------------------------------------------------------

BsaArchiveFile::BsaArchiveFile(const Bsa::BSAFile::FileStruct *info, Bsa::BSAFile* bsa)
    : mInfo(info)
    , mFile(bsa)
{

}

Files::IStreamPtr BsaArchiveFile::open()
{
    return mFile->getFile(mInfo);
}

}
