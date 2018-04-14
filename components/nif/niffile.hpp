///Main header for reading .nif files

#ifndef OPENMW_COMPONENTS_NIF_NIFFILE_HPP
#define OPENMW_COMPONENTS_NIF_NIFFILE_HPP

// definitions for VWD/LOD mode and quality
#define VWD_MODE_NORMAL_ONLY 0
#define VWD_MODE_NORMAL_AND_LOD 1
#define VWD_MODE_LOD_ONLY 2
#define VWD_MODE_LOD_AND_LARGE_NORMAL 3

#define VWD_QUAL_LOW 3200.0f
#define VWD_QUAL_MEDIUM 1600.0f
#define VWD_QUAL_HIGH 800.0f
#define VWD_QUAL_ULTRA 400.0f

/*
#define VWD_QUAL_LOW 800.0f
#define VWD_QUAL_MEDIUM 400.0f
#define VWD_QUAL_HIGH 200.0f
#define VWD_QUAL_ULTRA 100.0f
*/

#include <stdexcept>
#include <vector>
#include <map>
#include <iostream>

#include <components/files/constrainedfilestream.hpp>

#include "record.hpp"

namespace CSMDoc
{
	class Document;
}

namespace ESM 
{
	class ESMWriter;
}

namespace Nif
{
class NiSourceTexture;

class NIFFile
{
    enum NIFVersion {
        VER_MW    = 0x04000002    // Morrowind NIFs
    };

    /// Nif file version
    unsigned int ver;

    /// File name, used for error messages and opening the file
    std::string filename;

    /// Record list
    std::vector<Record*> records;

    /// Root list.  This is a select portion of the pointers from records
    std::vector<Record*> roots;

    bool mUseSkinning;

    /// Parse the file
    void parse(Files::IStreamPtr stream);

    /// Get the file's version in a human readable form
    ///\returns A string containing a human readable NIF version number
    std::string printVersion(unsigned int version);

    ///Private Copy Constructor
    NIFFile (NIFFile const &);
    ///\overload
    void operator = (NIFFile const &);

	void calculateModelBounds();

public:
    /// Used if file parsing fails
    void fail(const std::string &msg) const
    {
        std::string err = " NIFFile Error: " + msg;
        err += "\nFile: " + filename;
        throw std::runtime_error(err);
    }
    /// Used when something goes wrong, but not catastrophically so
    void warn(const std::string &msg) const
    {
        std::cerr << " NIFFile Warning: " << msg <<std::endl
                  << "File: " << filename <<std::endl;
    }

    /// Open a NIF stream. The name is used for error messages.
    NIFFile(Files::IStreamPtr stream, const std::string &name);
    ~NIFFile();

    /// Get a given record
    Record *getRecord(size_t index) const
    {
        Record *res = records.at(index);
        return res;
    }
    /// Number of records
    size_t numRecords() const { return records.size(); }

    /// Get a given root
    Record *getRoot(size_t index=0) const
    {
        Record *res = roots.at(index);
        return res;
    }
    /// Number of roots
    size_t numRoots() const { return roots.size(); }

    /// Set whether there is skinning contained in this NIF file.
    /// @note This is just a hint for users of the NIF file and has no effect on the loading procedure.
    void setUseSkinning(bool skinning);

    bool getUseSkinning() const;

    /// Get the name of the file
    std::string getFilename() const { return filename; }

	void exportHeader(Files::IStreamPtr inStream, std::ostream &outStream);
	void exportFooter(Files::IStreamPtr inStream, std::ostream &outStream);
	void exportRecord(Files::IStreamPtr inStream, std::ostream &outStream, int recordIndex);
	void exportRecordSourceTexture(Files::IStreamPtr inStream, std::ostream &outStream, int recordIndex, std::string pathPrefix="");
	void exportRecordNiNode(Files::IStreamPtr inStream, std::ostream &outStream, int recordIndex);

	void exportFileNif(ESM::ESMWriter &esm, Files::IStreamPtr inStream, std::string modelPath);
	void exportFileNifFar(ESM::ESMWriter &esm, Files::IStreamPtr inStream, std::string modelPath);
	void prepareExport(CSMDoc::Document &doc, ESM::ESMWriter &esm, std::string modelPath);
    void prepareExport_TextureRename(CSMDoc::Document &doc, ESM::ESMWriter &esm, std::string modelPath, Nif::NiSourceTexture *texture, int texturetype);
    void exportDDS(const std::string &oldName, const std::string &exportName, int texturetype);

	static std::string CreateResourcePaths(std::string modelPath);

	size_t mHeaderSize;
	std::vector<size_t> mRecordSizes;
	float mModelBounds=0;
	bool mReadyToExport=false;
	// map: record index to original resourceNames
//	std::map<int, std::string> mResourceNames;
    std::map<std::string, std::pair<std::string, int>> mOldName2NewName;
    CSMDoc::Document *mDocument = 0;

};
typedef std::shared_ptr<const Nif::NIFFile> NIFFilePtr;



} // Namespace
#endif
