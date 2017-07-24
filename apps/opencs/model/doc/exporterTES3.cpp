#include "../world/data.hpp"
#include "../world/idcollection.hpp"
#include "document.hpp"
#include "savingstages.hpp"
#include "exporterTES3.hpp"

CSMDoc::TES3Exporter::TES3Exporter(Document& document, const boost::filesystem::path projectPath, ToUTF8::FromType encoding)
  : Exporter(document, projectPath, encoding) 
{
}

void CSMDoc::TES3Exporter::defineExportOperation()
{

	// Export to ESM file
	mExportOperation.appendStage (new OpenSaveStage (mDocument, mState, false));

	mExportOperation.appendStage (new WriteHeaderStage (mDocument, mState, false));

	mExportOperation.appendStage (new WriteCollectionStage<CSMWorld::IdCollection<ESM::Global> >
		(mDocument.getData().getGlobals(), mState));

	mExportOperation.appendStage (new WriteCollectionStage<CSMWorld::IdCollection<ESM::GameSetting> >
		(mDocument.getData().getGmsts(), mState));

	mExportOperation.appendStage (new WriteCollectionStage<CSMWorld::IdCollection<ESM::Skill> >
		(mDocument.getData().getSkills(), mState));

	mExportOperation.appendStage (new WriteCollectionStage<CSMWorld::IdCollection<ESM::Class> >
		(mDocument.getData().getClasses(), mState));

	mExportOperation.appendStage (new WriteCollectionStage<CSMWorld::IdCollection<ESM::Faction> >
		(mDocument.getData().getFactions(), mState));

	mExportOperation.appendStage (new WriteCollectionStage<CSMWorld::IdCollection<ESM::Race> >
		(mDocument.getData().getRaces(), mState));

	mExportOperation.appendStage (new WriteCollectionStage<CSMWorld::IdCollection<ESM::Sound> >
		(mDocument.getData().getSounds(), mState));

	mExportOperation.appendStage (new WriteCollectionStage<CSMWorld::IdCollection<ESM::Script> >
		(mDocument.getData().getScripts(), mState));

	mExportOperation.appendStage (new WriteCollectionStage<CSMWorld::IdCollection<ESM::Region> >
		(mDocument.getData().getRegions(), mState));

	mExportOperation.appendStage (new WriteCollectionStage<CSMWorld::IdCollection<ESM::BirthSign> >
		(mDocument.getData().getBirthsigns(), mState));

	mExportOperation.appendStage (new WriteCollectionStage<CSMWorld::IdCollection<ESM::Spell> >
		(mDocument.getData().getSpells(), mState));

	mExportOperation.appendStage (new WriteCollectionStage<CSMWorld::IdCollection<ESM::Enchantment> >
		(mDocument.getData().getEnchantments(), mState));

	mExportOperation.appendStage (new WriteCollectionStage<CSMWorld::IdCollection<ESM::BodyPart> >
		(mDocument.getData().getBodyParts(), mState));

	mExportOperation.appendStage (new WriteCollectionStage<CSMWorld::IdCollection<ESM::SoundGenerator> >
		(mDocument.getData().getSoundGens(), mState));

	mExportOperation.appendStage (new WriteCollectionStage<CSMWorld::IdCollection<ESM::MagicEffect> >
		(mDocument.getData().getMagicEffects(), mState));

	mExportOperation.appendStage (new WriteCollectionStage<CSMWorld::IdCollection<ESM::StartScript> >
		(mDocument.getData().getStartScripts(), mState));

	mExportOperation.appendStage (new WriteRefIdCollectionStage (mDocument, mState));

	mExportOperation.appendStage (new CollectionReferencesStage (mDocument, mState));

	mExportOperation.appendStage (new WriteCellCollectionStage (mDocument, mState));

	// Dialogue can reference objects and cells so must be written after these records for vanilla-compatible files

	mExportOperation.appendStage (new WriteDialogueCollectionStage (mDocument, mState, false));

	mExportOperation.appendStage (new WriteDialogueCollectionStage (mDocument, mState, true));

	mExportOperation.appendStage (new WritePathgridCollectionStage (mDocument, mState));

	mExportOperation.appendStage (new WriteLandTextureCollectionStage (mDocument, mState));

	// references Land Textures
	mExportOperation.appendStage (new WriteLandCollectionStage (mDocument, mState));

	// close file and clean up
	mExportOperation.appendStage (new CloseSaveStage (mState));

	mExportOperation.appendStage (new FinalSavingStage (mDocument, mState));

}
