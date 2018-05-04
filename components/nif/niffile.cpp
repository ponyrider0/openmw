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

# define PI           3.14159265358979323846  /* pi */

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
	else if (records[recordIndex]->recType == Nif::RC_NiNode)
	{
		exportRecordNiNode(inStream, outStream, recordIndex);
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
        std::string oldName = Misc::ResourceHelpers::correctTexturePath(texture->filename, mDocument->getVFS());
        mDocument->getVFS()->normalizeFilename(oldName);

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
		// Sanity check to make sure name is correct
		std::string exportFilename = "";
		if (mOldName2NewName.find(oldName) == mOldName2NewName.end())
		{ 
			exportFilename = "";
		}
		// following code may now be redundant in automated pipeline: skip bump/glow implemented in modified export_nif blender script
		else if ( (mOldName2NewName[oldName].second == Nif::NiTexturingProperty::TextureType::BumpTexture)
			|| (mOldName2NewName[oldName].second == Nif::NiTexturingProperty::TextureType::GlowTexture) )
		{
			exportFilename = "";
		}
		else if (mOldName2NewName[oldName].first == "")
		{
			exportFilename = "";
		}
		else
		{
			exportFilename = pathPrefix + mOldName2NewName[oldName].first;
		}
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

void NIFFile::exportRecordNiNode(Files::IStreamPtr inStream, std::ostream & outStream, int recordIndex)
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
	if (records[recordIndex]->recType != Nif::RC_NiNode)
	{
		std::stringstream errMesg;
		// throw error message?
		errMesg << "ERROR: NIFFile::NiNode() wrong record type " << std::hex << records[recordIndex]->recType << "\n";
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

	Nif::NiNode *ninode = dynamic_cast<Nif::NiNode*>(records[recordIndex]);
	if (ninode != NULL)
	{
		int byteswritten = 0;
		// 32bit strlen + recordtype
		char recordType[] = "NiNode";
		uintVal = 6; // recordType is hardcoded without null terminator
		for (int j = 0; j < 4; j++) buffer[j] = reinterpret_cast<char *>(&uintVal)[j];
		len = 4;
		outStream.write(buffer, len);
		byteswritten += len;
		len = 6; // recordType is hardcoded without null terminator
		strncpy(buffer, recordType, len);
		outStream.write(buffer, len);
		byteswritten += len;
		// 10 bytes

		// 32bit strlen + recordname
		std::string nameToWrite = ninode->name;
		if (ninode->name == "Bip01")
		{
			// trigger flag to modify root node positioning and rotation later
		}
		uintVal = nameToWrite.size();
		for (int j = 0; j < 4; j++) buffer[j] = reinterpret_cast<char *>(&uintVal)[j];
		len = 4;
		outStream.write(buffer, len);
		byteswritten += len;
		len = nameToWrite.size();
		strncpy(buffer, nameToWrite.c_str(), len);
		outStream.write(buffer, len);
		// NB: Replace len with original ninode name rather than actual written name
		//     this is so input stream can be maintained at correct distance
		len = ninode->name.size();
		byteswritten += len;
		// variable bytes

		// then read Extra.index then Controller.index
		intVal = (ninode->extra.empty() == false) ? ninode->extra->recIndex : -1;
		for (int j = 0; j < 4; j++) buffer[j] = reinterpret_cast<char *>(&intVal)[j];
		len = 4;
		outStream.write(buffer, len);
		byteswritten += len;
		intVal = (ninode->controller.empty() == false) ? ninode->controller->recIndex : -1;
		for (int j = 0; j < 4; j++) buffer[j] = reinterpret_cast<char *>(&intVal)[j];
		len = 4;
		outStream.write(buffer, len);
		byteswritten += len;
		// 8 bytes

		// short: Flags
		shortVal = ninode->flags;
		for (int j = 0; j < 2; j++) buffer[j] = reinterpret_cast<char *>(&shortVal)[j];
		len = 2;
		outStream.write(buffer, len);
		byteswritten += len;
		// 2 bytes

		// Translation
		float floatVal = ninode->trafo.pos.x();
		for (int j = 0; j < 4; j++) buffer[j] = reinterpret_cast<char *>(&floatVal)[j];
		len = 4;
		outStream.write(buffer, len);
		byteswritten += len;
		floatVal = ninode->trafo.pos.y();
		for (int j = 0; j < 4; j++) buffer[j] = reinterpret_cast<char *>(&floatVal)[j];
		len = 4;
		outStream.write(buffer, len);
		byteswritten += len;
		floatVal = ninode->trafo.pos.z();
		for (int j = 0; j < 4; j++) buffer[j] = reinterpret_cast<char *>(&floatVal)[j];
		len = 4;
		outStream.write(buffer, len);
		byteswritten += len;

		// Rotation
		for (int a = 0; a < 3; a++)
		{
			for (int b = 0; b < 3; b++)
			{
				floatVal = ninode->trafo.rotation.mValues[a][b];
				for (int j = 0; j < 4; j++) buffer[j] = reinterpret_cast<char *>(&floatVal)[j];
				len = 4;
				outStream.write(buffer, len);
				byteswritten += len;
			}
		}

		// move read pointer to next byte in record
		inStream->seekg(byteswritten, std::ios_base::cur);
		// subtract byteswritten from recordsize and write the rest out to disk...
		readsize = mRecordSizes[recordIndex] - byteswritten;
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

void NIFFile::exportFileNif(ESM::ESMWriter &esm, Files::IStreamPtr inStream, std::string filePath)
{
    if (mReadyToExport == false)
        throw std::runtime_error("EXPORT NIF: exportFileNif() called prior to prepareExport()");

//    std::cout << "DEBUG: Begin NIFFile::exportFileNif(): [" << filePath << "]\n";
    for (auto it = mOldName2NewName.begin(); it != mOldName2NewName.end(); it++)
    {
        exportDDS(it->first, it->second.first, it->second.second);
    }
    // if NIF is from original BSA, just process any overrride textures and return
	if (mDocument->getVFS()->exists(filename) == true)
	{
		try
		{
			std::string archiveName = Misc::StringUtils::lowerCase(mDocument->getVFS()->lookupArchive(filename));
			if (archiveName.find("morrowind.bsa") != std::string::npos ||
				archiveName.find("tribunal.bsa") != std::string::npos ||
				archiveName.find("bloodmoon.bsa") != std::string::npos)
			{
				// log missing required NIF files
				if (mOldName2NewName.empty() == false)
				{
#ifdef _WIN32
					std::string logRoot = "";
#else
					std::string logRoot = getenv("HOME");
					logRoot += "/";
#endif
					logRoot += "modexporter_logs/";
					std::string modStem = mDocument->getSavePath().filename().stem().string();
					std::string logFileStem = "Missing_NIFs_" + modStem;
					std::ofstream logFileDDSConv;
					// Log File is initialized in savingstate.cpp::InitializeSubstitutions()
					logFileDDSConv.open(logRoot + logFileStem + ".csv", std::ios_base::out | std::ios_base::app);
					// cleanup filePath before logging:
					int cleanupOffset = Misc::StringUtils::lowerCase(filePath).find("/meshes/");
					if (cleanupOffset == std::string::npos) cleanupOffset = 0;
					std::string missingFilename = filePath.substr(cleanupOffset);
					logFileDDSConv << missingFilename << ",";
					auto it = mOldName2NewName.begin();
					while (it != mOldName2NewName.end())
					{
						logFileDDSConv << it->second.first;
						it++;
						if (it != mOldName2NewName.end())
						{
							logFileDDSConv << ",";
						}
					}
					logFileDDSConv << "\n";
					logFileDDSConv.close();
				}
				return;
			}
			//        std::cout << "NIFFile:: archive=[" << archiveName << "]" << " EXPORTING: " << filePath << "\n";
		}
		catch (std::runtime_error e)
		{
			std::cout << "NIFFile::exportFileNif(): archiveName lookup for (" << filename << "): " << e.what() << "\n";
			return;
		}
	}
	else
	{
		// file does not exist, so don't continue
		return;
	}

    std::string normalizedFilePath = Misc::ResourceHelpers::getNormalizedPath(filePath);
    boost::filesystem::path p(normalizedFilePath);
    if (boost::filesystem::exists(p.parent_path()) == false)
    {
        boost::filesystem::create_directories(p.parent_path());
    }

	// serialize modified NIF and output to newNIFFILe
	inStream->clear();
	inStream.get()->seekg(std::ios_base::beg);
	std::ofstream outStream;

	outStream.open(normalizedFilePath, std::ofstream::out | std::ofstream::binary | std::ofstream::trunc);
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
    if (mReadyToExport == false)
        throw std::runtime_error("EXPORT NIF: exportFileNif() called prior to prepareExport()");

    // export lowres textures only if exporter _far.nif
    for (auto it = mOldName2NewName.begin(); it != mOldName2NewName.end(); it++)
    {
        std::string prefix = "lowres/";
        exportDDS(it->first, prefix + it->second.first, it->second.second);
    }

    std::string normalizedFilePath = Misc::ResourceHelpers::getNormalizedPath(filePath);
    boost::filesystem::path p(normalizedFilePath);
    if (boost::filesystem::exists(p.parent_path()) == false)
    {
        boost::filesystem::create_directories(p.parent_path());
    }

	// create _far.nif filePath
	std::string path_farNIF = normalizedFilePath.substr(0, normalizedFilePath.size() - 4) + "_far.nif";
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
				std::string prefix = "lowres/";
				exportRecordSourceTexture(inStream, farNIFFile, i, prefix);
			}
            else
            {
                exportRecord(inStream, farNIFFile, i);
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

// Currently, texturetype is not used when exporting DDS
void NIFFile::exportDDS(const std::string &oldName, const std::string &exportName, int texturetype)
{
    if (mReadyToExport == false)
        throw std::runtime_error("EXPORT NIF: exportFileNif() called prior to prepareExport()");

    // skip original BSAs, if not lowres
    if (exportName.find("lowres/") == std::string::npos)
    {
        std::string archiveName = Misc::StringUtils::lowerCase(mDocument->getVFS()->lookupArchive(oldName));
        if (archiveName.find("morrowind.bsa") != std::string::npos ||
            archiveName.find("tribunal.bsa") != std::string::npos  ||
            archiveName.find("bloodmoon.bsa") != std::string::npos)
        {
            return;
        }
//        std::cout << "NIFFile:: archive=[" << archiveName << "]" << " EXPORTING: " << exportName << "\n";
    }

#ifdef _WIN32
    std::string outputRoot = "C:/";
    std::string logRoot = "";
#else
    std::string outputRoot = getenv("HOME");
    outputRoot += "/";
    std::string logRoot = outputRoot;
#endif
    logRoot += "modexporter_logs/";
    std::string modStem = mDocument->getSavePath().filename().stem().string();
    std::string logFileStem = "Exported_TextureList_" + modStem;
    std::ofstream logFileDDSConv;
	// Log File is initialized in savingstate.cpp::InitializeSubstitutions()
	logFileDDSConv.open(logRoot + logFileStem + ".csv", std::ios_base::out | std::ios_base::app);
//    logFileDDSConv << "Original texture,Exported texture,Export result\n";

    auto fileStream = mDocument->getVFS()->get(oldName);

    try {

        std::ofstream newDDSFile;
        // create output subdirectories
        boost::filesystem::path p(outputRoot + "Oblivion.output/Data/Textures/" + exportName);
        if (boost::filesystem::exists(p.parent_path()) == false)
        {
            boost::filesystem::create_directories(p.parent_path());
        }
		// DO NOT COPY IF FILE EXISTS
		if (boost::filesystem::exists(p.string()) == true)
		{
			return;
		}
		newDDSFile.open(p.string(), std::ofstream::out | std::ofstream::binary );

        int len = 0;
        char buffer[1024];
        while (fileStream->eof() == false)
        {
            fileStream->read(buffer, sizeof(buffer));
            len = fileStream->gcount();
            newDDSFile.write(buffer, len);
        }

        newDDSFile.close();
        logFileDDSConv << "export success\n";
    }
    catch (std::runtime_error e)
    {
        std::cout << "NIF Export (texture export) Error: (" << oldName << ") " << e.what() << "\n";
        logFileDDSConv << "export error\n";
    }
    catch (...)
    {
        std::cout << "NIF Export ERROR: something else bad happened!\n";
    }
    
    logFileDDSConv.close();

}

// degrees + vector x, y, z
static void SetNodeRotation(Matrix3 &m, float a, float x, float y, float z)
{

	osg::Matrixf r;
	r.makeRotate(a*PI/180, x, y, z);
	for (int a = 0; a < 3; a++)
		for (int b = 0; b < 3; b++)
			m.mValues[a][b] = r(b, a);

}


void NIFFile::prepareExport_TextureRename(CSMDoc::Document &doc, ESM::ESMWriter &esm, std::string raw_modelPath, Nif::NiSourceTexture *texture, int texturetype)
{
    if (texture != NULL)
    {
        std::string oldName = Misc::ResourceHelpers::correctTexturePath(texture->filename, doc.getVFS());
        std::string texFilename = texture->filename;
        std::string modelPath = raw_modelPath;

        // normalize to "/" separators
        doc.getVFS()->normalizeFilename(oldName);
        doc.getVFS()->normalizeFilename(texFilename);
        doc.getVFS()->normalizeFilename(modelPath);

        if (Misc::StringUtils::lowerCase(texFilename).find("textures/") == 0)
        {
            // remove "textures/" path prefix
            texFilename = texFilename.substr(strlen("textures/"));
        }
        bool bReplaceFullPath = true;
        if (Misc::StringUtils::lowerCase(texFilename).find("/") != std::string::npos)
        {
            bReplaceFullPath = false;
        }
        // extract modelPath dir and paste onto texture
        std::stringstream modelTexturePath;
        std::string texturePath = esm.generateEDIDTES4(texFilename, 1);
        if (texturePath.size() < 4)
        {
            std::cout << "DEBUG: prepareExport(): textureName=[" << texturePath << "] has less than 4 chars!\n";
            return;
        }
        //tempStr[texFilename.find_last_of(".")] = '.'; // restore '.' before filename extension
        texturePath.replace(texturePath.size()-4, 4, ".dds"); // change to DDS extension

        // TODO: lookup NIFRecord properties to identify bump maps and glow maps

        switch (texturetype)
        {
            case Nif::NiTexturingProperty::TextureType::BaseTexture:
                break;

            case Nif::NiTexturingProperty::TextureType::DetailTexture:
                break;

            case Nif::NiTexturingProperty::TextureType::GlowTexture:
                if (Misc::StringUtils::lowerCase(texturePath).find("ug.dds") != std::string::npos)
                {
                	texturePath.replace(texturePath.size()-6, 2, "_g");
                }
                else if (Misc::StringUtils::lowerCase(texturePath).find("uglow.dds") != std::string::npos)
                {
                    texturePath.replace(texturePath.size()-9, 5, "_g");
                }
                else
                {
                    texturePath.insert(texturePath.size()-4, "_g");
                }
                break;

            case Nif::NiTexturingProperty::TextureType::BumpTexture:
                // change Unrm to _n...
                if (Misc::StringUtils::lowerCase(texturePath).find("unrm.dds") != std::string::npos)
                {
                    texturePath.replace(texturePath.size()-8, 4, "_n");
                }
                else if (Misc::StringUtils::lowerCase(texturePath).find("unm.dds") != std::string::npos)
                {
                    texturePath.replace(texturePath.size()-7, 3, "_n");
                }
				else if (Misc::StringUtils::lowerCase(texturePath).find("un.dds") != std::string::npos)
				{
					texturePath.replace(texturePath.size() - 6, 2, "_n");
				}
				else
                {
                    texturePath.insert(texturePath.size()-4, "_n");
                }
                break;

			default:
				break;

        }
        std::string modelPrefix = "";
        if (bReplaceFullPath)
        {
            if (modelPath.find("/") != std::string::npos)
                modelPrefix = modelPath.substr(0, modelPath.find_last_of("/"));
        }
        else
        {
            if (Misc::StringUtils::lowerCase(modelPath).find("morro/") != std::string::npos)
                modelPrefix = modelPath.substr(0, Misc::StringUtils::lowerCase(modelPath).find("morro/") + 5);
        }
        modelTexturePath << modelPrefix;
        modelTexturePath << "/" << texturePath;

        // moved DDS export call to exportFileNif
        mOldName2NewName[oldName] = std::make_pair(modelTexturePath.str(), texturetype);
    }
}

void NIFFile::prepareExport(CSMDoc::Document &doc, ESM::ESMWriter &esm, std::string modelPath)
{
    mDocument = &doc;

    // look through each record type and process accordingly
    for (int i = 0; i < numRecords(); i++)
    {
        if (getRecord(i)->recType == Nif::RC_NiTexturingProperty)
        {
            Nif::NiTexturingProperty *tex_property = dynamic_cast<Nif::NiTexturingProperty*>(getRecord(i));
            if (tex_property != NULL)
            {
                // base texture (export this in nif-file only? - ignore all other textures)
                Nif::NiSourceTexture *base_tex = tex_property->textures[Nif::NiTexturingProperty::TextureType::BaseTexture].texture.getPtr();
                prepareExport_TextureRename(doc, esm, modelPath, base_tex, Nif::NiTexturingProperty::TextureType::BaseTexture);
                // leave the detail texture?
                Nif::NiSourceTexture *detail_tex = tex_property->textures[Nif::NiTexturingProperty::TextureType::DetailTexture].texture.getPtr();
                prepareExport_TextureRename(doc, esm, modelPath, detail_tex, Nif::NiTexturingProperty::TextureType::DetailTexture);
                // bumpmap texture (rename this to base texturename + "_n.dds"
                Nif::NiSourceTexture *bump_tex = tex_property->textures[Nif::NiTexturingProperty::TextureType::BumpTexture].texture.getPtr();
                prepareExport_TextureRename(doc, esm, modelPath, bump_tex, Nif::NiTexturingProperty::TextureType::BumpTexture);
                // glow texture (rename to base texturename + "_g.dds"
                Nif::NiSourceTexture *glow_tex = tex_property->textures[Nif::NiTexturingProperty::TextureType::GlowTexture].texture.getPtr();
                prepareExport_TextureRename(doc, esm, modelPath, glow_tex, Nif::NiTexturingProperty::TextureType::GlowTexture);

            }
        }

        // TODO: if collision node found, set properties?
        // ...

        // repose for TES4 rigging
        if (getRecord(i)->recType == Nif::RC_NiNode)
        {
            Nif::NiNode *ninode = dynamic_cast<Nif::NiNode*>(getRecord(i));
            if (ninode != NULL)
            {
                if (ninode->name == "Bip01")
                {
                    SetNodeRotation(ninode->trafo.rotation, 90, 0, 0, 1);
                }

            }
        }

    }

	// second pass through records to pick up any skipped textures
	for (int i = 0; i < numRecords(); i++)
	{
		if (getRecord(i)->recType == Nif::RC_NiSourceTexture)
		{
			Nif::NiSourceTexture *texture = dynamic_cast<Nif::NiSourceTexture*>(getRecord(i));
			if (texture != NULL)
			{
				std::string oldName = Misc::ResourceHelpers::correctTexturePath(texture->filename, doc.getVFS());
				doc.getVFS()->normalizeFilename(oldName);
				if (mOldName2NewName.find(oldName) == mOldName2NewName.end())
				{
					prepareExport_TextureRename(doc, esm, modelPath, texture, Nif::NiTexturingProperty::TextureType::BaseTexture);
				}
			}
		}

	}

    mReadyToExport = true;
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

    std::string normalizedModelPath = Misc::ResourceHelpers::getNormalizedPath(modelPath);

//	boost::filesystem::path filePath(outputRoot + "Oblivion.output/Data/Meshes/" + normalizedModelPath);
	std::string filePath = outputRoot + "Oblivion.output/temp/Meshes/" + normalizedModelPath;

	return filePath;
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
	// The following section calculates diagonal (diameter) length
	float dX2 = abs(maxx - minx); dX2 *= dX2;
	float dY2 = abs(maxy - miny); dY2 *= dY2;
	float dZ2 = abs(maxz - minz); dZ2 *= dZ2;
	float lenXY = sqrt(dX2 + dY2);
	float lenXZ = sqrt(dX2 + dZ2);
	float lenYZ = sqrt(dY2 + dZ2);
	float a, b;
	if (lenXY > lenXZ)
	{
		a = lenXY;
		// XY is winner, so compare XZ and YZ to figure out #2
		b = (lenXZ > lenYZ) ? lenXZ : lenYZ;
	}
	else
	{
		a = lenXZ;
		// XZ is winner, so compare XY and YZ to figure out #2
		b = (lenXY > lenYZ) ? lenXY : lenYZ;
	}
	float lenXYZ = sqrt(a*a + b*b);
	// Now divide by two for radius
	float radXY = lenXY / 2;
	float radXZ = lenXZ / 2;
	float radYZ = lenYZ / 2;
	float radXYZ = lenXYZ / 2;

	modelBounds = (radXY > modelBounds) ? radXY : modelBounds;
	modelBounds = (radXZ > modelBounds) ? radXZ : modelBounds;
	modelBounds = (radYZ > modelBounds) ? radYZ : modelBounds;
	modelBounds = (radXYZ > modelBounds) ? radXYZ : modelBounds;

	mModelBounds = modelBounds;

}

}
