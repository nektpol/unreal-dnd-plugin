// Developed by Neko Creative Technologies
#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Dnd5eTags.h"
#include "Items/Dnd5eItemLibrary.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Readers/Items/Dnd5eJsonItemCatalogLoader.h"
#include "Readers/Items/Dnd5eJsonItemVariantMatcher.h"
#include "Readers/Items/Dnd5eJsonItemVariantGenerator.h"
#include "Readers/Spells/Dnd5eJsonSpellCatalogLibrary.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
	bool ReadAuditDocument(const FString& Root, const FString& Namespace, const TCHAR* File,
		FDnd5eJsonDocument& Document, TSharedPtr<FJsonObject>& Json)
	{
		Document.Context.DatasetNamespace = Namespace;
		Document.Context.RelativeFilePath = File;
		return FFileHelper::LoadFileToString(Document.JsonText, *FPaths::Combine(Root, File)) &&
			FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Document.JsonText), Json) && Json.IsValid();
	}

	int32 RowCount(const FJsonObject& Json, const TCHAR* Field)
	{
		const TArray<TSharedPtr<FJsonValue>>* Rows = nullptr;
		return Json.TryGetArrayField(Field, Rows) && Rows ? Rows->Num() : INDEX_NONE;
	}

	void ReportLoad(FAutomationTestBase& Test, const TCHAR* Stage, const FDnd5eJsonItemCatalogLoadResult& Result)
	{
		TMap<FName, int32> Counts;
		for (const auto& Diagnostic : Result.Diagnostics)
		{
			++Counts.FindOrAdd(Diagnostic.Code);
			if (Diagnostic.Severity == EDnd5eDiagnosticSeverity::Error)
			{
				Test.AddInfo(FString::Printf(TEXT("%s error %s %s %s"), Stage, *Diagnostic.Code.ToString(),
					*Diagnostic.RelativeFilePath, *Diagnostic.JsonPointer));
			}
		}
		Counts.KeySort(FNameLexicalLess());
		Test.AddInfo(FString::Printf(TEXT("%s records=%d definitions=%d diagnostics=%d"),
			Stage, Result.RecordCount, Result.DefinitionCount, Result.Diagnostics.Num()));
		for (const auto& Entry : Counts)
		{
			Test.AddInfo(FString::Printf(TEXT("%s %s=%d"), Stage, *Entry.Key.ToString(), Entry.Value));
		}
	}
}

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FDnd5eOfflineItemImportAudit, "Dnd5e.Items.Loader.External", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
void FDnd5eOfflineItemImportAudit::GetTests(TArray<FString>& Names, TArray<FString>& Commands) const
{
	FString Root;
	if (FParse::Value(FCommandLine::Get(), TEXT("Dnd5eJsonItemAuditRoot="), Root))
	{
		Names.Add(TEXT("CompleteImport"));
		Commands.Add(TEXT("CompleteImport"));
	}
}

bool FDnd5eOfflineItemImportAudit::RunTest(const FString& Parameters)
{
	FDnd5eJsonItemCatalogLoadRequest Request;
	if (!FParse::Value(FCommandLine::Get(), TEXT("Dnd5eJsonItemAuditRoot="), Request.DatasetRoot)) { return false; }
	Request.DatasetNamespace = TEXT("audit-mixed-snapshot");
	Request.BaseFiles = {TEXT("items-base.json")};
	Request.DefinitionFiles = Request.BaseFiles;
	Request.ItemFiles = {TEXT("items.json")};
	FDnd5eJsonDocument BaseDocument, ItemDocument;
	TSharedPtr<FJsonObject> BaseJson, ItemJson;
	if (!TestTrue(TEXT("Read base and item inventories"), ReadAuditDocument(Request.DatasetRoot, Request.DatasetNamespace,
		TEXT("items-base.json"), BaseDocument, BaseJson) && ReadAuditDocument(Request.DatasetRoot, Request.DatasetNamespace,
		TEXT("items.json"), ItemDocument, ItemJson))) { return false; }
	const int32 BaseCount = RowCount(*BaseJson, TEXT("baseitem"));
	const int32 ItemCount = RowCount(*ItemJson, TEXT("item"));
	const int32 GroupCount = RowCount(*ItemJson, TEXT("itemGroup"));
	const int32 TypeCount = RowCount(*BaseJson, TEXT("itemType"));
	const int32 PropertyCount = RowCount(*BaseJson, TEXT("itemProperty"));
	const int32 MasteryCount = RowCount(*BaseJson, TEXT("itemMastery"));
	if (!TestTrue(TEXT("All primary arrays exist"), BaseCount >= 0 && ItemCount >= 0 && GroupCount >= 0 &&
		TypeCount >= 0 && PropertyCount >= 0 && MasteryCount >= 0)) { return false; }
	const int32 PrimaryCount = BaseCount + ItemCount + GroupCount;
	const int32 DefinitionCount = TypeCount + PropertyCount + MasteryCount;

	const auto Primary = UDnd5eJsonItemCatalogLoader::LoadItemCatalog(Request);
	ReportLoad(*this, TEXT("primary"), Primary);
	if (!TestTrue(TEXT("Primary loader publishes both complete catalogs"), Primary.Status == EDnd5eJsonReadStatus::Success &&
		Primary.ItemCatalog && Primary.DefinitionCatalog && Primary.RecordCount == PrimaryCount &&
		Primary.DefinitionCount == DefinitionCount)) { return false; }
	TStrongObjectPtr<UDnd5eItemCatalogObject> PrimaryItems(Primary.ItemCatalog.Get());

	Request.SupplementFiles = {TEXT("fluff-items.json")};
	const auto Supplemented = UDnd5eJsonItemCatalogLoader::LoadItemCatalog(Request);
	ReportLoad(*this, TEXT("supplements"), Supplemented);
	if (!TestTrue(TEXT("Supplemented load preserves primary counts"), Supplemented.Status == EDnd5eJsonReadStatus::Success &&
		Supplemented.ItemCatalog && Supplemented.DefinitionCatalog && Supplemented.RecordCount == PrimaryCount &&
		Supplemented.DefinitionCount == DefinitionCount)) { return false; }

	FDnd5eItemCatalogQuery Query;
	Query.DatasetNamespace = Request.DatasetNamespace;
	Query.Category = Dnd5e::Category::Item;
	TArray<FDnd5eRecordId> ItemIds, GroupIds;
	TArray<FDnd5eDiagnostic> Diagnostics;
	if (!TestTrue(TEXT("Item category reconciles with raw counts"), Supplemented.ItemCatalog->QueryRecordIds(Query, ItemIds, Diagnostics) &&
		ItemIds.Num() == BaseCount + ItemCount)) { return false; }
	Query.Category = Dnd5e::Category::ItemGroup;
	if (!TestTrue(TEXT("Group category reconciles with raw counts"), Supplemented.ItemCatalog->QueryRecordIds(Query, GroupIds, Diagnostics) &&
		GroupIds.Num() == GroupCount)) { return false; }

	int32 AttachedItems = 0;
	int32 AttachedGroups = 0;
	int32 ResolvedTypes = 0;
	int32 ResolvedProperties = 0;
	int32 ResolvedMasteries = 0;
	int32 MatchedMembers = 0;
	FDnd5eJsonSpellCatalogLoadRequest SpellRequest;
	SpellRequest.DatasetRoot = Request.DatasetRoot;
	SpellRequest.DatasetNamespace = Request.DatasetNamespace;
	SpellRequest.SpellFiles = {TEXT("spells/spells-xphb.json")};
	TStrongObjectPtr<UDnd5eSpellCatalogObject> Spells(NewObject<UDnd5eSpellCatalogObject>());
	const auto SpellBuild = UDnd5eJsonSpellCatalogLibrary::LoadSpellCatalog(Spells.Get(), SpellRequest);
	if (!TestTrue(TEXT("Load the selected spell file for attachment lookups"),
		SpellBuild.Status == EDnd5eJsonReadStatus::Success && SpellBuild.SpellCount > 0)) { return false; }
	int32 MatchedSpells = 0;
	int32 UnloadedSpells = 0;
	TArray<FDnd5eRecordId> Matches;
	FDnd5eItem Item;
	for (const auto& Id : ItemIds)
	{
		if (!TestTrue(TEXT("Item query returns a retrievable record with source provenance"),
			Supplemented.ItemCatalog->TryGetItem(Id, Item) && !Item.Header.Provenance.IsEmpty() &&
			!Item.Header.Provenance[0].RelativeFilePath.IsEmpty() && !Item.Header.Provenance[0].JsonPointer.IsEmpty())) { return false; }
		AttachedItems += Item.bHasSupplementaryContent ? 1 : 0;
		if (Item.bHasType)
		{
			FDnd5eItemTypeDefinition Definition;
			ResolvedTypes += Supplemented.DefinitionCatalog->MatchDefinitionSelector(Id.DatasetNamespace, Item.Type,
				Matches, Diagnostics) == EDnd5eItemDefinitionMatchStatus::Matched &&
				Matches.Num() == 1 && Supplemented.DefinitionCatalog->TryGetType(Matches[0], Definition) ? 1 : 0;
		}
		for (const auto& Selector : Item.Properties)
		{
			FDnd5eItemPropertyDefinition Definition;
			ResolvedProperties += Supplemented.DefinitionCatalog->MatchDefinitionSelector(Id.DatasetNamespace, Selector,
				Matches, Diagnostics) == EDnd5eItemDefinitionMatchStatus::Matched &&
				Matches.Num() == 1 && Supplemented.DefinitionCatalog->TryGetProperty(Matches[0], Definition) ? 1 : 0;
		}
		for (const auto& Mastery : Item.Masteries)
		{
			FDnd5eItemMasteryDefinition Definition;
			ResolvedMasteries += Supplemented.DefinitionCatalog->MatchMasterySelector(Id.DatasetNamespace, Mastery.Selector,
				Matches, Diagnostics) == EDnd5eItemDefinitionMatchStatus::Matched &&
				Matches.Num() == 1 && Supplemented.DefinitionCatalog->TryGetMastery(Matches[0], Definition) ? 1 : 0;
		}
		for (const auto& Attachment : Item.AttachedSpells.Spells)
		{
			const auto Status = UDnd5eItemLibrary::MatchItemAttachedSpell(Id.DatasetNamespace, Attachment, Spells.Get(), Matches, Diagnostics);
			FDnd5eSpell Spell;
			MatchedSpells += Status == EDnd5eSpellSelectorMatchStatus::Matched && Matches.Num() == 1 &&
				Spells->TryGetSpell(Matches[0], Spell) ? 1 : 0;
			UnloadedSpells += Status == EDnd5eSpellSelectorMatchStatus::NoMatch && Matches.IsEmpty() ? 1 : 0;
		}
	}
	for (const auto& Id : GroupIds)
	{
		FDnd5eItemGroup Group;
		if (!TestTrue(TEXT("Group query returns a retrievable record"), Supplemented.ItemCatalog->TryGetGroup(Id, Group))) { return false; }
		AttachedGroups += Group.Details.bHasSupplementaryContent ? 1 : 0;
		for (const auto& Member : Group.Members)
		{
			const auto Status = Supplemented.ItemCatalog->MatchItemSelector(Id.DatasetNamespace, Member, Matches);
			MatchedMembers += Status == EDnd5eItemSelectorMatchStatus::Matched ? 1 : 0;
			if (!Member.bHasSource)
			{
				TestTrue(TEXT("Omitted group member source cannot authorize retrieval"), Status == EDnd5eItemSelectorMatchStatus::MissingSource);
			}
		}
	}
	AddInfo(FString::Printf(TEXT("Attachments items=%d groups=%d matched types=%d properties=%d masteries=%d members=%d"),
		AttachedItems, AttachedGroups, ResolvedTypes, ResolvedProperties, ResolvedMasteries, MatchedMembers));
	AddInfo(FString::Printf(TEXT("Attached spell lookups against one selected spell file matched=%d absent=%d"), MatchedSpells, UnloadedSpells));
	TestTrue(TEXT("Snapshot demonstrates supplement, definition and member joins"),
		AttachedItems > 0 && ResolvedTypes > 0 && ResolvedProperties > 0 && ResolvedMasteries > 0 && MatchedMembers > 0);
	TestTrue(TEXT("Attached spell lookups find a loaded target and distinguish absent targets"), MatchedSpells > 0 && UnloadedSpells > 0);

	Request.VariantFiles = {TEXT("magicvariants.json")};
	using EPolicy = EDnd5eJsonItemVariantEditionPolicy;
	for (const auto Policy : {EPolicy::PreferClassic, EPolicy::SourceDefault})
	{
		Request.DatasetNamespace = Policy == EPolicy::PreferClassic ? TEXT("audit-2014") : TEXT("audit-2024");
		BaseDocument.Context.DatasetNamespace = Request.DatasetNamespace;
		FDnd5eJsonDocument VariantDocument;
		TSharedPtr<FJsonObject> VariantJson;
		if (!TestTrue(TEXT("Read variant inventory"), ReadAuditDocument(Request.DatasetRoot, Request.DatasetNamespace,
			TEXT("magicvariants.json"), VariantDocument, VariantJson))) { return false; }
		const TArray<TSharedPtr<FJsonValue>>* Templates = nullptr;
		if (!TestTrue(TEXT("Variant array exists"), VariantJson->TryGetArrayField(TEXT("magicvariant"), Templates) && Templates)) { return false; }

		Request.VariantSelections.Reset();
		TArray<FDnd5eRecordId> ExpectedGeneratedIds;
		int32 EligiblePairs = 0;
		int32 EligibleCopiedPairs = 0;
		bool bStandaloneSelected = false;
		bool bCopySelected = false;
		for (int32 Index = 0; Index < Templates->Num(); ++Index)
		{
			const auto Match = FDnd5eJsonItemVariantMatcher::MatchAtIndex(VariantDocument, Index, BaseDocument, Policy);
			if (!TestTrue(FString::Printf(TEXT("Match template /magicvariant/%d"), Index), Match.Status == EDnd5eJsonReadStatus::Success)) { return false; }
			EligiblePairs += Match.MatchingBaseIndices.Num();
			const bool bCopied = (*Templates)[Index]->AsObject()->HasField(TEXT("_copy"));
			if (bCopied) { EligibleCopiedPairs += Match.MatchingBaseIndices.Num(); }
			bool& bSelected = bCopied ? bCopySelected : bStandaloneSelected;
			if (bSelected || Match.MatchingBaseIndices.IsEmpty()) { continue; }
			const int32 BaseIndex = Match.MatchingBaseIndices[0];
			const auto Generated = FDnd5eJsonItemVariantGenerator::ReadAtIndex(VariantDocument, Index, BaseDocument, BaseIndex, Policy);
			if (!TestTrue(FString::Printf(TEXT("Convert selected template /magicvariant/%d"), Index),
				Generated.Status == EDnd5eJsonReadStatus::Success)) { return false; }
			Request.VariantSelections.Add({0, Index, 0, BaseIndex, Policy});
			ExpectedGeneratedIds.Add(Generated.Item.Header.RecordId);
			bSelected = true;
		}

		AddInfo(FString::Printf(TEXT("%s eligible pairs=%d copied pairs=%d selected=%d"), *Request.DatasetNamespace,
			EligiblePairs, EligibleCopiedPairs, ExpectedGeneratedIds.Num()));
		TestTrue(TEXT("Select a sample from each eligible template form"),
			bStandaloneSelected == (EligiblePairs > EligibleCopiedPairs) && bCopySelected == (EligibleCopiedPairs > 0) && EligiblePairs > 0);
		const auto Edition = UDnd5eJsonItemCatalogLoader::LoadItemCatalog(Request);
		ReportLoad(*this, *Request.DatasetNamespace, Edition);
		if (!TestTrue(TEXT("Variant load publishes the expected counts in a separate catalog"),
			Edition.Status == EDnd5eJsonReadStatus::Success && Edition.ItemCatalog && Edition.DefinitionCatalog &&
			Edition.ItemCatalog.Get() != PrimaryItems.Get() && Edition.RecordCount == PrimaryCount + ExpectedGeneratedIds.Num() &&
			Edition.DefinitionCount == DefinitionCount)) { return false; }
		for (const auto& Id : ExpectedGeneratedIds)
		{
			FDnd5eItem Base;
			TestTrue(TEXT("Generated ID resolves to a variant with its loaded base and source rows"),
				Id.DatasetNamespace == Request.DatasetNamespace && Edition.ItemCatalog->TryGetItem(Id, Item) &&
				Item.bHasBaseItemId && Edition.ItemCatalog->TryGetItem(Item.BaseItemId, Base) && Item.Header.Provenance.Num() >= 3 &&
				Item.Header.Provenance[1].RelativeFilePath == TEXT("magicvariants.json") &&
				Item.Header.Provenance[2].RelativeFilePath == TEXT("items-base.json"));
		}
		Query.Category = {};
		Query.DatasetNamespace = TEXT("audit-mixed-snapshot");
		TestTrue(TEXT("The mixed snapshot namespace does not leak into a separate load"),
			Edition.ItemCatalog->QueryRecordIds(Query, ItemIds, Diagnostics) && ItemIds.IsEmpty());
	}
	return !HasAnyErrors();
}

#endif
