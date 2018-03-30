#include "niffile.hpp"
#include "effect.hpp"

#include <map>
#include <sstream>
#include <iostream>
#include <fstream>
#include <boost/filesystem.hpp>

#include <apps/opencs/model/doc/document.hpp>
#include <components/esm/esmwriter.hpp>
#include <components/vfs/manager.hpp>
#include <components/misc/resourcehelpers.hpp>

namespace Nif
{

/// Open a NIF stream. The name is used for error messages.
NIFFile::NIFFile(Files::IStreamPtr stream, const std::string &name)
    : ver(0)
    , filename(name)
    , mUseSkinning(false)
{
    parse(stream);
}

NIFFile::~NIFFile()
{
    for (std::vector<Record*>::iterator it = records.begin() ; it != records.end(); ++it)
    {
        delete *it;
    }
}

template <typename NodeType> static Record* construct() { return new NodeType; }

struct RecordFactoryEntry {

    typedef Record* (*create_t) ();

    create_t        mCreate;
    RecordType      mType;

};

///Helper function for adding records to the factory map
static std::pair<std::string,RecordFactoryEntry> makeEntry(std::string recName, Record* (*create_t) (), RecordType type)
{
    RecordFactoryEntry anEntry = {create_t,type};
    return std::make_pair(recName, anEntry);
}

///These are all the record types we know how to read.
static std::map<std::string,RecordFactoryEntry> makeFactory()
{
    std::map<std::string,RecordFactoryEntry> newFactory;
    newFactory.insert(makeEntry("NiNode",                     &construct <NiNode>                      , RC_NiNode                        ));
    newFactory.insert(makeEntry("NiSwitchNode",               &construct <NiSwitchNode>                , RC_NiSwitchNode                  ));
    newFactory.insert(makeEntry("NiLODNode",                  &construct <NiLODNode>                   , RC_NiLODNode                     ));
    newFactory.insert(makeEntry("AvoidNode",                  &construct <NiNode>                      , RC_AvoidNode                     ));
    newFactory.insert(makeEntry("NiBSParticleNode",           &construct <NiNode>                      , RC_NiBSParticleNode              ));
    newFactory.insert(makeEntry("NiBSAnimationNode",          &construct <NiNode>                      , RC_NiBSAnimationNode             ));
    newFactory.insert(makeEntry("NiBillboardNode",            &construct <NiNode>                      , RC_NiBillboardNode               ));
    newFactory.insert(makeEntry("NiTriShape",                 &construct <NiTriShape>                  , RC_NiTriShape                    ));
    newFactory.insert(makeEntry("NiRotatingParticles",        &construct <NiRotatingParticles>         , RC_NiRotatingParticles           ));
    newFactory.insert(makeEntry("NiAutoNormalParticles",      &construct <NiAutoNormalParticles>       , RC_NiAutoNormalParticles         ));
    newFactory.insert(makeEntry("NiCamera",                   &construct <NiCamera>                    , RC_NiCamera                      ));
    newFactory.insert(makeEntry("RootCollisionNode",          &construct <NiNode>                      , RC_RootCollisionNode             ));
    newFactory.insert(makeEntry("NiTexturingProperty",        &construct <NiTexturingProperty>         , RC_NiTexturingProperty           ));
    newFactory.insert(makeEntry("NiFogProperty",              &construct <NiFogProperty>               , RC_NiFogProperty                 ));
    newFactory.insert(makeEntry("NiMaterialProperty",         &construct <NiMaterialProperty>          , RC_NiMaterialProperty            ));
    newFactory.insert(makeEntry("NiZBufferProperty",          &construct <NiZBufferProperty>           , RC_NiZBufferProperty             ));
    newFactory.insert(makeEntry("NiAlphaProperty",            &construct <NiAlphaProperty>             , RC_NiAlphaProperty               ));
    newFactory.insert(makeEntry("NiVertexColorProperty",      &construct <NiVertexColorProperty>       , RC_NiVertexColorProperty         ));
    newFactory.insert(makeEntry("NiShadeProperty",            &construct <NiShadeProperty>             , RC_NiShadeProperty               ));
    newFactory.insert(makeEntry("NiDitherProperty",           &construct <NiDitherProperty>            , RC_NiDitherProperty              ));
    newFactory.insert(makeEntry("NiWireframeProperty",        &construct <NiWireframeProperty>         , RC_NiWireframeProperty           ));
    newFactory.insert(makeEntry("NiSpecularProperty",         &construct <NiSpecularProperty>          , RC_NiSpecularProperty            ));
    newFactory.insert(makeEntry("NiStencilProperty",          &construct <NiStencilProperty>           , RC_NiStencilProperty             ));
    newFactory.insert(makeEntry("NiVisController",            &construct <NiVisController>             , RC_NiVisController               ));
    newFactory.insert(makeEntry("NiGeomMorpherController",    &construct <NiGeomMorpherController>     , RC_NiGeomMorpherController       ));
    newFactory.insert(makeEntry("NiKeyframeController",       &construct <NiKeyframeController>        , RC_NiKeyframeController          ));
    newFactory.insert(makeEntry("NiAlphaController",          &construct <NiAlphaController>           , RC_NiAlphaController             ));
    newFactory.insert(makeEntry("NiUVController",             &construct <NiUVController>              , RC_NiUVController                ));
    newFactory.insert(makeEntry("NiPathController",           &construct <NiPathController>            , RC_NiPathController              ));
    newFactory.insert(makeEntry("NiMaterialColorController",  &construct <NiMaterialColorController>   , RC_NiMaterialColorController     ));
    newFactory.insert(makeEntry("NiBSPArrayController",       &construct <NiBSPArrayController>        , RC_NiBSPArrayController          ));
    newFactory.insert(makeEntry("NiParticleSystemController", &construct <NiParticleSystemController>  , RC_NiParticleSystemController    ));
    newFactory.insert(makeEntry("NiFlipController",           &construct <NiFlipController>            , RC_NiFlipController              ));
    newFactory.insert(makeEntry("NiAmbientLight",             &construct <NiLight>                     , RC_NiLight                       ));
    newFactory.insert(makeEntry("NiDirectionalLight",         &construct <NiLight>                     , RC_NiLight                       ));
    newFactory.insert(makeEntry("NiPointLight",               &construct <NiPointLight>                , RC_NiLight                       ));
    newFactory.insert(makeEntry("NiSpotLight",                &construct <NiSpotLight>                 , RC_NiLight                       ));
    newFactory.insert(makeEntry("NiTextureEffect",            &construct <NiTextureEffect>             , RC_NiTextureEffect               ));
    newFactory.insert(makeEntry("NiVertWeightsExtraData",     &construct <NiVertWeightsExtraData>      , RC_NiVertWeightsExtraData        ));
    newFactory.insert(makeEntry("NiTextKeyExtraData",         &construct <NiTextKeyExtraData>          , RC_NiTextKeyExtraData            ));
    newFactory.insert(makeEntry("NiStringExtraData",          &construct <NiStringExtraData>           , RC_NiStringExtraData             ));
    newFactory.insert(makeEntry("NiGravity",                  &construct <NiGravity>                   , RC_NiGravity                     ));
    newFactory.insert(makeEntry("NiPlanarCollider",           &construct <NiPlanarCollider>            , RC_NiPlanarCollider              ));
    newFactory.insert(makeEntry("NiSphericalCollider",        &construct <NiSphericalCollider>         , RC_NiSphericalCollider           ));
    newFactory.insert(makeEntry("NiParticleGrowFade",         &construct <NiParticleGrowFade>          , RC_NiParticleGrowFade            ));
    newFactory.insert(makeEntry("NiParticleColorModifier",    &construct <NiParticleColorModifier>     , RC_NiParticleColorModifier       ));
    newFactory.insert(makeEntry("NiParticleRotation",         &construct <NiParticleRotation>          , RC_NiParticleRotation            ));
    newFactory.insert(makeEntry("NiFloatData",                &construct <NiFloatData>                 , RC_NiFloatData                   ));
    newFactory.insert(makeEntry("NiTriShapeData",             &construct <NiTriShapeData>              , RC_NiTriShapeData                ));
    newFactory.insert(makeEntry("NiVisData",                  &construct <NiVisData>                   , RC_NiVisData                     ));
    newFactory.insert(makeEntry("NiColorData",                &construct <NiColorData>                 , RC_NiColorData                   ));
    newFactory.insert(makeEntry("NiPixelData",                &construct <NiPixelData>                 , RC_NiPixelData                   ));
    newFactory.insert(makeEntry("NiMorphData",                &construct <NiMorphData>                 , RC_NiMorphData                   ));
    newFactory.insert(makeEntry("NiKeyframeData",             &construct <NiKeyframeData>              , RC_NiKeyframeData                ));
    newFactory.insert(makeEntry("NiSkinData",                 &construct <NiSkinData>                  , RC_NiSkinData                    ));
    newFactory.insert(makeEntry("NiUVData",                   &construct <NiUVData>                    , RC_NiUVData                      ));
    newFactory.insert(makeEntry("NiPosData",                  &construct <NiPosData>                   , RC_NiPosData                     ));
    newFactory.insert(makeEntry("NiRotatingParticlesData",    &construct <NiRotatingParticlesData>     , RC_NiRotatingParticlesData       ));
    newFactory.insert(makeEntry("NiAutoNormalParticlesData",  &construct <NiAutoNormalParticlesData>   , RC_NiAutoNormalParticlesData     ));
    newFactory.insert(makeEntry("NiSequenceStreamHelper",     &construct <NiSequenceStreamHelper>      , RC_NiSequenceStreamHelper        ));
    newFactory.insert(makeEntry("NiSourceTexture",            &construct <NiSourceTexture>             , RC_NiSourceTexture               ));
    newFactory.insert(makeEntry("NiSkinInstance",             &construct <NiSkinInstance>              , RC_NiSkinInstance                ));
    return newFactory;
}


///Make the factory map used for parsing the file
static const std::map<std::string,RecordFactoryEntry> factories = makeFactory();

std::string NIFFile::printVersion(unsigned int version)
{
    union ver_quad
    {
        uint32_t full;
        uint8_t quad[4];
    } version_out;

    version_out.full = version;

    std::stringstream stream;
    stream  << version_out.quad[3] << "."
            << version_out.quad[2] << "."
            << version_out.quad[1] << "."
            << version_out.quad[0];
    return stream.str();
}

void NIFFile::parse(Files::IStreamPtr stream)
{
    NIFStream nif (this, stream);
	size_t previousPos = stream->tellg();
	size_t currentPos = stream->tellg();

    // Check the header string
    std::string head = nif.getVersionString();
    if(head.compare(0, 22, "NetImmerse File Format") != 0)
        fail("Invalid NIF header:  " + head);

    // Get BCD version
    ver = nif.getUInt();
    if(ver != VER_MW)
        fail("Unsupported NIF version: " + printVersion(ver));
    // Number of records
    size_t recNum = nif.getInt();
    records.resize(recNum);

    /* The format for 10.0.1.0 seems to be a bit different. After the
     header, it contains the number of records, r (int), just like
     4.0.0.2, but following that it contains a short x, followed by x
     strings. Then again by r shorts, one for each record, giving
     which of the above strings to use to identify the record. After
     this follows two ints (zero?) and then the record data. However
     we do not support or plan to support other versions yet.
    */

	previousPos = currentPos;
	currentPos = stream->tellg();
	mHeaderSize = currentPos - previousPos;
    for(size_t i = 0;i < recNum;i++)
    {
        Record *r = NULL;

        std::string rec = nif.getString();
        if(rec.empty())
        {
            std::stringstream error;
            error << "Record number " << i << " out of " << recNum << " is blank.";
            fail(error.str());
        }

        std::map<std::string,RecordFactoryEntry>::const_iterator entry = factories.find(rec);

        if (entry != factories.end())
        {
            r = entry->second.mCreate ();
            r->recType = entry->second.mType;
        }
        else
            fail("Unknown record type " + rec);

        assert(r != NULL);
        assert(r->recType != RC_MISSING);
        r->recName = rec;
        r->recIndex = i;
        records[i] = r;
        r->read(&nif);

		previousPos = currentPos;
		currentPos = stream->tellg();
		mRecordSizes.push_back(currentPos - previousPos);
    }

    size_t rootNum = nif.getUInt();
    roots.resize(rootNum);

    //Determine which records are roots
    for(size_t i = 0;i < rootNum;i++)
    {
        int idx = nif.getInt();
        if (idx >= 0 && idx < int(records.size()))
        {
            roots[i] = records[idx];
        }
        else
        {
            roots[i] = NULL;
            warn("Null Root found");
        }
    }

    // Once parsing is done, do post-processing.
    for(size_t i=0; i<recNum; i++)
        records[i]->post(this);

	calculateModelBounds();
}

void NIFFile::setUseSkinning(bool skinning)
{
    mUseSkinning = skinning;
}

bool NIFFile::getUseSkinning() const
{
    return mUseSkinning;
}

void NIFFile::exportHeader(Files::IStreamPtr inStream, std::ostream &outStream)
{
	size_t len = 0;
	int intVal = 0;
	uint16_t shortVal = 0;
	uint32_t uintVal = 0;
	uint8_t byteVal = 0;
	char buffer[4096];
	size_t readsize = mHeaderSize;

	inStream->seekg(std::ios_base::beg);

/*
	// fake header
	// getVersionString: Version String (eol terminated?)
	char VersionString[] = "Gamebryo File Format, Version 20.0.0.5\n";
	//			char VersionString[] = "NetImmerse File Format, Version 4.0.0.2\n";
	len = strlen(VersionString);
	strncpy(buffer, VersionString, len);
	outStream.write(buffer, len);
	// getUInt: BCD version
	//			uintVal = 0x04000002; 
	uintVal = 0x14000005;
	for (int j = 0; j < 4; j++) buffer[j] = reinterpret_cast<char *>(&uintVal)[j];
	len = 4;
	outStream.write(buffer, len);
	// NEW: Endian Type
	byteVal = 1; // LITTLE ENDIAN
	buffer[0] = byteVal;
	len = 1;
	outStream.write(buffer, len);
	// NEW: User Version
	uintVal = 11;
	for (int j = 0; j < 4; j++) buffer[j] = reinterpret_cast<char *>(&uintVal)[j];
	len = 4;
	outStream.write(buffer, len);
	// getInt: number of records
	uintVal = nifFile.numRecords();
	for (int j = 0; j < 4; j++) buffer[j] = reinterpret_cast<char *>(&uintVal)[j];
	len = 4;
	outStream.write(buffer, len);
	// NEW: User Version 2
	uintVal = 11;
	for (int j = 0; j < 4; j++) buffer[j] = reinterpret_cast<char *>(&uintVal)[j];
	len = 4;
	outStream.write(buffer, len);
	// NEW: ExportInfo
	// --ExportInfo: 3 short strings (byte + string)
	for (int i = 0; i < 3; i++)
	{
		byteVal = 1;
		buffer[0] = byteVal;
		len = 1;
		outStream.write(buffer, len);
		len = 1;
		strncpy(buffer, "\0", len);
		outStream.write(buffer, len);
	}
	// NEW: Number of Block Types (uint16)
	shortVal = 8;
	for (int j = 0; j < 3; j++) buffer[j] = reinterpret_cast<char *>(&shortVal)[j];
	len = 2;
	outStream.write(buffer, len);
	// NEW: Block Types (int32 + string)
	std::string blockTypes[] = { "NiNode","NiTriShape","NiTexturingProperty","NiSourceTexture","NiAlphaProperty","NiStencilProperty","NiMaterialProperty","NiTriShapeData","","" };
	for (int i = 0; i < shortVal; i++)
	{
		std::string blockType = blockTypes[i];
		uintVal = blockType.size();
		for (int j = 0; j < 4; j++) buffer[j] = reinterpret_cast<char *>(&uintVal)[j];
		len = 4;
		outStream.write(buffer, len);
		len = blockType.size();
		strncpy(buffer, blockType.c_str(), len);
		outStream.write(buffer, len);
	}
	// NEW: Block Type Index (uint16) x Number of Records
	int blockIndexes[] = { 0,0,0,0,1,2, 3,4,5,6,7, 1,2,3,7,1, 2,3,7,1,2, 3,7,1,7,1, 2,3,7,1,2, 7 };
	for (int i = 0; i < nifFile.numRecords(); i++)
	{
		shortVal = blockIndexes[i];
		for (int j = 0; j < 3; j++) buffer[j] = reinterpret_cast<char *>(&shortVal)[j];
		len = 2;
		outStream.write(buffer, len);
	}
	// NEW: Unknown Int 2 (int32)
	uintVal = 0;
	for (int j = 0; j < 4; j++) buffer[j] = reinterpret_cast<char *>(&uintVal)[j];
	len = 4;
	outStream.write(buffer, len);
*/

	// original header
	while (readsize > sizeof(buffer))
	{
		inStream->read(buffer, sizeof(buffer));
		len = inStream->gcount();
		outStream.write(buffer, len);
		readsize -= len;
	}
	inStream->read(buffer, readsize);
	len = inStream->gcount();
	outStream.write(buffer, len);

}

void NIFFile::exportRecord(Files::IStreamPtr inStream, std::ostream & outStream, int recordIndex)
{
	size_t len = 0;
	int intVal = 0;
	uint16_t shortVal = 0;
	uint32_t uintVal = 0;
	uint8_t byteVal = 0;
	char buffer[4096];
	size_t readsize = mHeaderSize;

	// sanity check: check bounds of index
	if (recordIndex >= mRecordSizes.size())
	{
		std::stringstream errMesg;
		// throw error message?
		errMesg << "ERROR: NIFFile::exportRecord() recordIndex out of bounds: " << recordIndex << "\n";
		throw(std::runtime_error(errMesg.str()));
		return;
	}
//	std::cout << "DEBUG: recordIndex= " << recordIndex << ", recordSize=" << mRecordSizes[recordIndex] << "  ; mRecordSizes.size()=" << mRecordSizes.size() << "\n";

	// move inStream pointer to beginning of relevant record
	int recordmark = mHeaderSize;
	for (int i = 0; i < recordIndex; i++)
	{
		recordmark += mRecordSizes[i];
	}
	inStream->seekg(recordmark, std::ios_base::beg);

	// write record
	if (records[recordIndex]->recType == Nif::RC_NiSourceTexture)
	{
		exportRecordSourceTexture(inStream, outStream, recordIndex, "");
	}
	else
	{
		// generic export of all other records
		readsize = mRecordSizes[recordIndex];
		while (readsize > sizeof(buffer))
		{
			inStream->read(buffer, sizeof(buffer));
			len = inStream->gcount();
			if (len == 0 || inStream->rdstate() != std::ios_base::goodbit)
			{
				// get stream state
				bool isEOF = inStream->eof();
				bool isFAIL = inStream->fail();
				bool isBADio = inStream->bad();
				std::stringstream errMesg;
				// throw error message?
				errMesg << "ERROR: NIFFile::exportRecord() read error occured at index=" << recordIndex << "STATE: Failed=" << isFAIL << " EOF=" << isEOF << " BAD i/o=" << isBADio << "\n";
				throw(std::runtime_error(errMesg.str()));
				return;
			}
			outStream.write(buffer, len);
			if (len == 0)
			{
				std::stringstream errMesg;
				// throw error message?
				errMesg << "ERROR: NIFFile::exportRecord() write error occured at index=" << recordIndex << "\n";
				throw(std::runtime_error(errMesg.str()));
				return;
			}
			readsize -= len;
		}
		inStream->read(buffer, readsize);
		len = inStream->gcount();
		outStream.write(buffer, len);

	}


}

void NIFFile::exportRecordSourceTexture(Files::IStreamPtr inStream, std::ostream & outStream, int recordIndex, std::string pathPrefix)
{
	size_t len = 0;
	int intVal = 0;
	uint16_t shortVal = 0;
	uint32_t uintVal = 0;
	uint8_t byteVal = 0;
	char buffer[4096];
	size_t readsize = mHeaderSize;

	// sanity check: check bounds of index
	if (recordIndex > records.size())
	{
		std::stringstream errMesg;
		// throw error message?
		errMesg << "ERROR: NIFFile::exportRecord() recordIndex out of bounds: " << recordIndex << "\n";
		throw(std::runtime_error(errMesg.str()));
		return;
	}

	// sanity check: record type
	if (records[recordIndex]->recType != Nif::RC_NiSourceTexture)
	{
		std::stringstream errMesg;
		// throw error message?
		errMesg << "ERROR: NIFFile::exportRecordSourceTexture() wrong record type " << std::hex << records[recordIndex]->recType << "\n";
		throw(std::runtime_error(errMesg.str()));
		return;
	}

	// move inStream pointer to beginning of relevant record
	int recordmark = mHeaderSize;
	for (int i = 0; i < recordIndex; i++)
	{
		recordmark += mRecordSizes[i];
	}
	inStream->seekg(recordmark, std::ios_base::beg);

	// custom export with substitution of texture filenames
	Nif::NiSourceTexture *texture = dynamic_cast<Nif::NiSourceTexture*>(records[recordIndex]);
	if (texture != NULL && texture->external == true)
	{
		// 32bit strlen + recordtype
		char recordName[] = "NiSourceTexture";
		uintVal = 15; // recordName is hardcoded without null terminator
		for (int j = 0; j < 4; j++) buffer[j] = reinterpret_cast<char *>(&uintVal)[j];
		len = 4;
		outStream.write(buffer, len);
		len = 15; // recordName is hardcoded without null terminator
		strncpy(buffer, recordName, len);
		outStream.write(buffer, len);

		// 32bit strlen + recordname
		uintVal = texture->name.size();
		for (int j = 0; j < 4; j++) buffer[j] = reinterpret_cast<char *>(&uintVal)[j];
		len = 4;
		outStream.write(buffer, len);
		len = texture->name.size();
		strncpy(buffer, texture->name.c_str(), len);
		outStream.write(buffer, len);
		// then read Extra.index then Controller.index
		intVal = (texture->extra.empty() == false) ? texture->extra->recIndex : -1;
		for (int j = 0; j < 4; j++) buffer[j] = reinterpret_cast<char *>(&intVal)[j];
		len = 4;
		outStream.write(buffer, len);
		intVal = (texture->controller.empty() == false) ? texture->controller->recIndex : -1;
		for (int j = 0; j < 4; j++) buffer[j] = reinterpret_cast<char *>(&intVal)[j];
		len = 4;
		outStream.write(buffer, len);
		// 8bit (bool external = true)
		buffer[0] = true;
		len = 1;
		outStream.write(buffer, len);

		//*********************filename replacement here*************************/
		// 32bit strlen + filename
		std::string exportFilename = pathPrefix + texture->filename;
		uintVal = exportFilename.size();
		for (int j = 0; j < 4; j++) buffer[j] = reinterpret_cast<char *>(&uintVal)[j];
		len = 4;
		outStream.write(buffer, len);
		len = exportFilename.size();
		strncpy(buffer, exportFilename.c_str(), len);
		outStream.write(buffer, len);
		//***********************************************************************/

		// 32bit int (pixel)
		for (int j = 0; j < 4; j++) buffer[j] = reinterpret_cast<char *>(&texture->pixel)[j];
		len = 4;
		outStream.write(buffer, len);
		// 32bit int (mipmap)
		for (int j = 0; j < 4; j++) buffer[j] = reinterpret_cast<char *>(&texture->mipmap)[j];
		len = 4;
		outStream.write(buffer, len);
		// 32bit int (alpha)
		for (int j = 0; j < 4; j++) buffer[j] = reinterpret_cast<char *>(&texture->alpha)[j];
		len = 4;
		outStream.write(buffer, len);
		// byte (always 1)
		buffer[0] = 1;
		len = 1;
		outStream.write(buffer, len);

		// move read pointer to end of current record
		readsize = mRecordSizes[recordIndex];
		inStream->seekg(readsize, std::ios_base::cur);
	}
	else
	{
		// nothing to replace, export using generic code
		// do NOT call back to generic function since it may have been the caller for this function (infinite loop)
		readsize = mRecordSizes[recordIndex];
		while (readsize > sizeof(buffer))
		{
			inStream->read(buffer, sizeof(buffer));
			len = inStream->gcount();
			outStream.write(buffer, len);
			readsize -= len;
		}
		inStream->read(buffer, readsize);
		len = inStream->gcount();
		outStream.write(buffer, len);
	}

}

void NIFFile::exportFileNif(Files::IStreamPtr inStream, std::string filePath)
{
	// serialize modified NIF and output to newNIFFILe
	inStream->clear();
	inStream.get()->seekg(std::ios_base::beg);
	std::ofstream outStream;

	outStream.open(filePath, std::ofstream::out | std::ofstream::binary | std::ofstream::trunc);
	// write header
	exportHeader(inStream, outStream);
	// write records
	for (int i = 0; i < records.size(); i++)
	{
		exportRecord(inStream, outStream, i);
	}
	// write footer data
	exportFooter(inStream, outStream);
	outStream.close();
}

void NIFFile::exportFileNifFar(ESM::ESMWriter &esm, Files::IStreamPtr inStream, std::string filePath)
{
	// create _far.nif filePath
	std::string path_farNIF = filePath.substr(0, filePath.size() - 4) + "_far.nif";
	std::ofstream farNIFFile;

	farNIFFile.open(path_farNIF, std::ofstream::out | std::ofstream::binary | std::ofstream::trunc);
	inStream->clear(); // reset state
	exportHeader(inStream, farNIFFile);
	for (int i = 0; i < numRecords(); i++)
	{
		if (getRecord(i)->recType == Nif::RC_NiSourceTexture)
		{
			Nif::NiSourceTexture *texture = dynamic_cast<Nif::NiSourceTexture*>(getRecord(i));
			if (texture != NULL)
			{
				// rename texture filepath to /textures/lowres/...
				std::string prefix = "lowres\\";
				exportRecordSourceTexture(inStream, farNIFFile, i, prefix);
				esm.mDDSToExportList.push_back(std::make_pair(mResourceNames[i], std::make_pair(prefix + texture->filename, 3)));
			}
		}
		else
		{
			exportRecord(inStream, farNIFFile, i);
		}
	}
	// write footer data
	exportFooter(inStream, farNIFFile);
	farNIFFile.close();
}

static void SetNodeRotation(Matrix3 &m, float a, float x, float y, float z)
{
	osg::Matrixf r;
	r.makeRotate(a, x, y, z);
	for (int a = 0; a<3; a++)
		for (int b = 0; b<3; b++)
			m.mValues[a][b] = r(b, a);
}

void NIFFile::prepareExport(CSMDoc::Document &doc, ESM::ESMWriter &esm, std::string modelPath)
{
	// look through each record, process accordingly
	for (int i = 0; i < numRecords(); i++)
	{
		// TODO: if collision node found, set properties?
		// ...

		// for each NiNode "Bip01" bone, repose for TES4 rigging
		if (getRecord(i)->recType == Nif::RC_NiNode)
		{
			Nif::NiNode *ninode = dynamic_cast<Nif::NiNode*>(getRecord(i));
			if (ninode != NULL)
			{
				if (ninode->name == "Bip01")
				{
					SetNodeRotation(ninode->trafo.rotation, 90, 0, 0, 1);
				}
				if (ninode->name == "Bip01 Pelvis")
				{
//					SetNodeRotation(ninode->trafo.rotation, 240, 0.57735, 0.57735, 0.57735);
				}
				if (ninode->name == "Bip01 L Thigh")
				{
					SetNodeRotation(ninode->trafo.rotation, 181.02, -0.00310, 0.99989, -0.01472);
				}
				if (ninode->name == "Bip01 L Calf")
				{
					SetNodeRotation(ninode->trafo.rotation, 8.52, 0, 0, -1);
				}
				if (ninode->name == "Bip01 L Foot")
				{
					SetNodeRotation(ninode->trafo.rotation, 9.89, -0.42654, -0.13563, 0.89424);
				}
				if (ninode->name == "Bip01 R Thigh")
				{
					SetNodeRotation(ninode->trafo.rotation, 178.98, -0.00310, 0.99989, 0.01472);
				}
				if (ninode->name == "Bip01 R Calf")
				{
					SetNodeRotation(ninode->trafo.rotation, 8.52, 0, 0, -1);
				}
				if (ninode->name == "Bip01 R Foot")
				{
					SetNodeRotation(ninode->trafo.rotation, 9.89, 0.42654, 0.13563, 0.89424);
				}
				if (ninode->name == "Bip01 L Clavicle")
				{
					SetNodeRotation(ninode->trafo.rotation, 187.01, 0.60044, -0.09847, 0.79358);
				}
				if (ninode->name == "Bip01 L UpperArm")
				{
					SetNodeRotation(ninode->trafo.rotation, 16.88, 0.40576, -0.69868, -0.58924);
				}
				if (ninode->name == "Bip01 L Forearem")
				{
					SetNodeRotation(ninode->trafo.rotation, 14.47, 0, 0, -1);
				}
				if (ninode->name == "Bip01 L Hand")
				{
					SetNodeRotation(ninode->trafo.rotation, 90.68, -0.99981, -0.01439, -0.01302);
				}
				if (ninode->name == "Bip01 R Clavicle")
				{
					SetNodeRotation(ninode->trafo.rotation, 187.01, -0.60044, 0.09847, 0.79358);
				}
				if (ninode->name == "Bip01 R UpperArm")
				{
					SetNodeRotation(ninode->trafo.rotation, 16.88, -0.40576, 0.69868, -0.58924);
				}
				if (ninode->name == "Bip01 R Forearem")
				{
					SetNodeRotation(ninode->trafo.rotation, 14.47, 0, 0, -1);
				}
				if (ninode->name == "Bip01 R Hand")
				{
					SetNodeRotation(ninode->trafo.rotation, 90.68, 0.99981, 0.01439, -0.01302);
				}

			}
		}

		// for each texturesource node, change name
		if (getRecord(i)->recType == Nif::RC_NiSourceTexture)
		{
			Nif::NiSourceTexture *texture = dynamic_cast<Nif::NiSourceTexture*>(getRecord(i));
			if (texture != NULL)
			{
				mResourceNames[i] = Misc::ResourceHelpers::correctTexturePath(texture->filename, doc.getVFS());
				std::string texFilename = texture->filename;
				if (Misc::StringUtils::lowerCase(texFilename).find("textures/") == 0 ||
					Misc::StringUtils::lowerCase(texFilename).find("textures\\") == 0)
				{
					// remove "textures/" path prefix
					texFilename = texFilename.substr(strlen("textures/"));
				}
				bool bReplaceFullPath = true;
				if (Misc::StringUtils::lowerCase(texFilename).find("\\") != std::string::npos ||
					Misc::StringUtils::lowerCase(texFilename).find("/") != std::string::npos)
				{
					bReplaceFullPath = false;
				}
				// extract modelPath dir and paste onto texture
				boost::filesystem::path modelP(modelPath);
				std::stringstream tempPath;
				std::string tempStr = esm.generateEDIDTES4(texFilename, 1);
//				tempStr[texFilename.find_last_of(".")] = '.'; // restore '.' before filename extension
				tempStr.replace(tempStr.size()-4, 4, ".dds"); // change to DDS extension
				// TODO: lookup NIFRecord properties to identify bump maps and glow maps
				// change Unrm to _n...
				if (Misc::StringUtils::lowerCase(tempStr).find("unrm.dds") != std::string::npos)
				{
					tempStr.replace(tempStr.size()-8, 4, "_n");
				}
				if (Misc::StringUtils::lowerCase(tempStr).find("unm.dds") != std::string::npos)
				{
					tempStr.replace(tempStr.size()-7, 3, "_n");
				}
				if (Misc::StringUtils::lowerCase(tempStr).find("ug.dds") != std::string::npos)
				{
					tempStr.replace(tempStr.size()-6, 2, "_g");
				}
				if (Misc::StringUtils::lowerCase(tempStr).find("uglow.dds") != std::string::npos)
				{
					tempStr.replace(tempStr.size()-9, 2, "_g");
				}
				if (bReplaceFullPath)
				{
					tempPath << modelP.parent_path().string();
				}
				else
				{
					std::string morroPath = modelP.parent_path().string();
					morroPath = morroPath.substr(0, Misc::StringUtils::lowerCase(morroPath).find("morro\\") + 5);
					tempPath << morroPath;
				}
				tempPath << "\\" << tempStr;
				esm.mDDSToExportList.push_back(std::make_pair(mResourceNames[i], std::make_pair(tempPath.str(), 3)));
//				esm.mDDSToExportList[resourceName] = std::make_pair(tempPath.str(), 3);
				texture->filename = tempPath.str();
			}
		}
	}
}

std::string NIFFile::CreateResourcePaths(std::string modelPath)
{
#ifdef _WIN32
	std::string outputRoot = "C:/";
	std::string logRoot = "";
#else
	std::string outputRoot = getenv("HOME");
	outputRoot += "/";
	std::string logRoot = outputRoot;
#endif

	boost::filesystem::path rootDir(outputRoot + "Oblivion.output/Data/Meshes/");
	if (boost::filesystem::exists(rootDir) == false)
	{
		boost::filesystem::create_directories(rootDir);
	}

	boost::filesystem::path filePath(outputRoot + "Oblivion.output/Data/Meshes/" + modelPath);
	if (boost::filesystem::exists(filePath.parent_path()) == false)
	{
		boost::filesystem::create_directories(filePath.parent_path());
	}

	return filePath.string();
}

void NIFFile::exportFooter(Files::IStreamPtr inStream, std::ostream & outStream)
{
	size_t len = 0;
	char buffer[4096];

	// move input-pointer to past last record
	int footermark = mHeaderSize;
	for (int i = 0; i < records.size(); i++)
	{
		footermark += mRecordSizes[i];
	}
	inStream->seekg(footermark, std::ios_base::beg);

	while (inStream->eof() == false)
	{
		inStream->read(buffer, sizeof(buffer));
		len = inStream->gcount();
		if (len == 0)
		{
			std::stringstream errMesg;
			// throw error message?
			errMesg << "ERROR: NIFFile::exportFooter() read error occured.\n";
			throw(std::runtime_error(errMesg.str()));
			return;
		}
		outStream.write(buffer, len);
	}
}

void NIFFile::calculateModelBounds()
{
	float modelBounds = mModelBounds;
	bool init=false;
	float maxx, maxy, maxz, minx, miny, minz;

	for (int i = 0; i < records.size(); i++)
	{
		// for each mesh data record, calculate boundradius
		if (records[i]->recType == Nif::RC_NiTriShape)
		{
			Nif::NiTriShape *triNode = dynamic_cast<Nif::NiTriShape*>(records[i]);
			if (triNode == NULL) continue;

			osg::Matrixf meshMatrix = triNode->trafo.toMatrix();
			// code block to multiply from inside to out
			Nif::NiNode *parent = triNode->parent;
			while (parent != NULL)
			{
				meshMatrix *= parent->trafo.toMatrix();
				parent = parent->parent;
			}
			/** This code block is to multiply matrix from outside to in 
			std::vector<Nif::Node*> parentStack;
			parentStack.push_back(triNode->parent);
			// get TriShape, Trishape->parent(s) and Trishape->children/Data
			while (parentStack.back()->parent != NULL)
			{
				parentStack.push_back(parentStack.back()->parent);
			}
			// build matrix by multiplying from top parent to bottom
			while (parentStack.empty() != true)
			{
				meshMatrix *= parentStack.back()->trafo.toMatrix();
				parentStack.pop_back();
			}
			*/

			// NiTriShape only has one relevant child: NiTriShapeData
			Nif::NiTriShapeData *meshdata = triNode->data.getPtr();
			if (meshdata != NULL)
			{
				// These two commands are to use pre-existing radius measurements per mesh
				// downside: ignores distance between mesh centers
//				float shapeBounds = meshdata->radius * meshMatrix.getScale().x();
//				modelBounds = (modelBounds > shapeBounds) ? modelBounds : shapeBounds;

				// compare xyz to minmax xyz
				for (std::vector<osg::Vec3f>::iterator vert_it = meshdata->vertices.begin(); vert_it != meshdata->vertices.end(); vert_it++)
				{
					osg::Vec3f vert = (*vert_it) * meshMatrix;
					float x = vert.x();
					float y = vert.y();
					float z = vert.z();
					if (init == false)
					{
						init = true;
						maxx = minx = x;
						maxy = miny = y;
						maxz = minz = z;
						continue;
					}
					maxx = (maxx > x) ? maxx : x;
					maxy = (maxy > y) ? maxy : y;
					maxz = (maxz > z) ? maxz : z;
					minx = (minx < x) ? minx : x;
					miny = (miny < y) ? miny : y;
					minz = (minz < z) ? minz : z;

				}
		
			}
		}
	}
	// after complete min/max calculated, determine greatest bound Size
	float dX2 = abs(maxx - minx); dX2 *= dX2;
	float dY2 = abs(maxy - miny); dY2 *= dY2;
	float dZ2 = abs(maxz - minz); dZ2 *= dZ2;

	float radXY = sqrt(dX2 + dY2);
	float radXZ = sqrt(dX2 + dZ2);
	float radYZ = sqrt(dY2 + dZ2);

	float a, b;
	if (radXY > radXZ)
	{
		a = radXY;
		// XY is winner, so compare XZ and YZ to figure out #2
		b = (radXZ > radYZ) ? radXZ : radYZ;
	}
	else
	{
		a = radXZ;
		// XZ is winner, so compare XY and YZ to figure out #2
		b = (radXY > radYZ) ? radXY : radYZ;
	}

	float radXYZ = sqrt(a*a + b*b);

	modelBounds = (radXY > modelBounds) ? radXY : modelBounds;
	modelBounds = (radXZ > modelBounds) ? radXZ : modelBounds;
	modelBounds = (radYZ > modelBounds) ? radYZ : modelBounds;
	modelBounds = (radXYZ > modelBounds) ? radXYZ : modelBounds;

	mModelBounds = modelBounds;

}

}
