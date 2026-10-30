// Developed by Neko Creative Technologies
#if WITH_DEV_AUTOMATION_TESTS

#include "Dnd5eTestSupport.h"
#include "Readers/Dnd5eJsonSpellReader.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

using namespace Dnd5eTests;

namespace
{
	FDnd5eInlineContent CollectionReference()
	{
		FDnd5eReferenceContent Reference;
		Reference.SourceFormat = TEXT("5etools"); Reference.SourceReference = TEXT("{@spell Invented Beacon|SYN}");
		return Inline(Dnd5e::Content::Inline::Reference, Reference);
	}
	FDnd5eContentBlock ReferenceParagraph()
	{
		FDnd5eParagraphContent Paragraph; Paragraph.Inlines = {CollectionReference()};
		return Block(Dnd5e::Content::Block::Paragraph, Paragraph);
	}
	struct FContainerCase { FString Name; FDnd5eContentBlock Root; FString Suffix; };
	TArray<FContainerCase> ContainerCases()
	{
		const auto Reference = CollectionReference(); const auto Paragraph = ReferenceParagraph();
		TArray<FContainerCase> Cases;
		auto Add = [&](const TCHAR* Name, const FDnd5eContentBlock& Root, const TCHAR* Suffix) { Cases.Add({Name, Root, Suffix}); };
		Add(TEXT("Paragraph"), Paragraph, TEXT("/Inlines/0/Payload"));
		FDnd5eSectionContent Section; Section.Heading = {Reference};
		Add(TEXT("SectionHeading"), Block(Dnd5e::Content::Block::Section, Section), TEXT("/Heading/0/Payload"));
		Section.Heading.Reset(); Section.Blocks = {Paragraph};
		Add(TEXT("SectionBlocks"), Block(Dnd5e::Content::Block::Section, Section), TEXT("/Blocks/0/Payload/Inlines/0/Payload"));
		FDnd5eListContent List; List.Heading = {Reference};
		Add(TEXT("ListHeading"), Block(Dnd5e::Content::Block::List, List), TEXT("/Heading/0/Payload"));
		List.Heading.Reset(); FDnd5eListItemContent Item; Item.Label = {Reference}; List.Items = {Item};
		Add(TEXT("ListItemLabel"), Block(Dnd5e::Content::Block::List, List), TEXT("/Items/0/Label/0/Payload"));
		List.Items[0].Label.Reset(); List.Items[0].Blocks = {Paragraph};
		Add(TEXT("ListItemBlocks"), Block(Dnd5e::Content::Block::List, List), TEXT("/Items/0/Blocks/0/Payload/Inlines/0/Payload"));
		FDnd5eTableContent Table; Table.Caption = {Reference};
		Add(TEXT("TableCaption"), Block(Dnd5e::Content::Block::Table, Table), TEXT("/Caption/0/Payload"));
		Table = {}; Table.Intro = {Paragraph};
		Add(TEXT("TableIntro"), Block(Dnd5e::Content::Block::Table, Table), TEXT("/Intro/0/Payload/Inlines/0/Payload"));
		Table = {}; Table.Footnotes = {Paragraph};
		Add(TEXT("TableFootnotes"), Block(Dnd5e::Content::Block::Table, Table), TEXT("/Footnotes/0/Payload/Inlines/0/Payload"));
		Table = {}; Table.Outro = {Paragraph};
		Add(TEXT("TableOutro"), Block(Dnd5e::Content::Block::Table, Table), TEXT("/Outro/0/Payload/Inlines/0/Payload"));
		FDnd5eTableCellContent Cell; Cell.Blocks = {Paragraph}; FDnd5eTableRowContent Row; Row.Cells = {Cell};
		Table = {}; Table.HeaderRows = {Row};
		Add(TEXT("TableHeaderRows"), Block(Dnd5e::Content::Block::Table, Table), TEXT("/HeaderRows/0/Cells/0/Blocks/0/Payload/Inlines/0/Payload"));
		Table = {}; Table.Rows = {Row};
		Add(TEXT("TableRows"), Block(Dnd5e::Content::Block::Table, Table), TEXT("/Rows/0/Cells/0/Blocks/0/Payload/Inlines/0/Payload"));
		FDnd5eQuoteContent Quote; Quote.Blocks = {Paragraph};
		Add(TEXT("QuoteBlocks"), Block(Dnd5e::Content::Block::Quote, Quote), TEXT("/Blocks/0/Payload/Inlines/0/Payload"));
		Quote = {}; Quote.Attribution = {Paragraph};
		Add(TEXT("QuoteAttribution"), Block(Dnd5e::Content::Block::Quote, Quote), TEXT("/Attribution/0/Payload/Inlines/0/Payload"));
		Quote = {}; Quote.Source = {Reference};
		Add(TEXT("QuoteSource"), Block(Dnd5e::Content::Block::Quote, Quote), TEXT("/Source/0/Payload"));
		FDnd5eInsetContent Inset; Inset.Heading = {Reference};
		Add(TEXT("InsetHeading"), Block(Dnd5e::Content::Block::Inset, Inset), TEXT("/Heading/0/Payload"));
		Inset = {}; Inset.Blocks = {Paragraph};
		Add(TEXT("InsetBlocks"), Block(Dnd5e::Content::Block::Inset, Inset), TEXT("/Blocks/0/Payload/Inlines/0/Payload"));
		FDnd5eImageContent Image; Image.Title = {Reference};
		Add(TEXT("ImageTitle"), Block(Dnd5e::Content::Block::Image, Image), TEXT("/Title/0/Payload"));
		Image = {}; Image.Credit = {Reference};
		Add(TEXT("ImageCredit"), Block(Dnd5e::Content::Block::Image, Image), TEXT("/Credit/0/Payload"));
		FDnd5eGalleryContent Gallery; Gallery.Heading = {Reference};
		Add(TEXT("GalleryHeading"), Block(Dnd5e::Content::Block::Gallery, Gallery), TEXT("/Heading/0/Payload"));
		Gallery = {}; Gallery.Images = {Block(Dnd5e::Content::Block::Image, Image)};
		Add(TEXT("GalleryImages"), Block(Dnd5e::Content::Block::Gallery, Gallery), TEXT("/Images/0/Payload/Credit/0/Payload"));
		auto AddInline = [&](const TCHAR* Name, const FDnd5eInlineContent& Value, const TCHAR* Field)
		{
			FDnd5eParagraphContent Body; Body.Inlines = {Value};
			Cases.Add({Name, Block(Dnd5e::Content::Block::Paragraph, Body), FString(TEXT("/Inlines/0/Payload/")) + Field + TEXT("/0/Payload")});
		};
		FDnd5eFormattedContent Formatted; Formatted.Inlines = {Reference};
		AddInline(TEXT("Formatted"), Inline(Dnd5e::Content::Inline::Formatted, Formatted), TEXT("Inlines"));
		FDnd5eReferenceContent Outer; Outer.Display = {Reference};
		// This case has two occurrences; it is checked separately below.
		FDnd5eAnnotationContent Annotation; Annotation.Display = {Reference};
		AddInline(TEXT("AnnotationDisplay"), Inline(Dnd5e::Content::Inline::Annotation, Annotation), TEXT("Display"));
		FDnd5eNumberContent Number; Number.Display = {Reference};
		AddInline(TEXT("NumberDisplay"), Inline(Dnd5e::Content::Inline::Number, Number), TEXT("Display"));
		FDnd5eExternalLinkContent External; External.Display = {Reference};
		AddInline(TEXT("ExternalLinkDisplay"), Inline(Dnd5e::Content::Inline::ExternalLink, External), TEXT("Display"));
		FDnd5eAnchorLinkContent Anchor; Anchor.Display = {Reference};
		AddInline(TEXT("AnchorLinkDisplay"), Inline(Dnd5e::Content::Inline::AnchorLink, Anchor), TEXT("Display"));
		return Cases;
	}
}

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FDnd5eCollectionContainersTest, "Dnd5e.Content.References.Containers", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
void FDnd5eCollectionContainersTest::GetTests(TArray<FString>& Names, TArray<FString>& Commands) const
{
	for (const auto& Case : ContainerCases()) { Names.Add(Case.Name); Commands.Add(Case.Name); }
}
bool FDnd5eCollectionContainersTest::RunTest(const FString& Parameters)
{
	const auto Cases = ContainerCases(); const auto* Case = Cases.FindByPredicate([&](const auto& Value) { return Value.Name == Parameters; });
	if (!TestNotNull(TEXT("Known container"), Case)) { return false; }
	FDnd5eSpell Spell; Spell.Description = {Case->Root}; const auto Before = Spell;
	TArray<FDnd5eContentReferenceOccurrence> Occurrences; TArray<FDnd5eDiagnostic> Diagnostics;
	Occurrences.AddDefaulted(); Diagnostics.AddDefaulted();
	TestTrue(TEXT("Traverse container"), UDnd5eSpellContentLibrary::CollectSpellReferences(Spell, Occurrences, Diagnostics));
	if (TestEqual(TEXT("Exactly one typed reference"), Occurrences.Num(), 1))
	{
		TestEqual(TEXT("Exact model path"), Occurrences[0].ContentPath, FString(TEXT("/Description/0/Payload")) + Case->Suffix);
		const auto Expected = CollectionReference().Payload.Get<FDnd5eReferenceContent>();
		TestTrue(TEXT("Unresolved reference copied intact"), Equal(FDnd5eReferenceContent::StaticStruct(), &Occurrences[0].Reference, &Expected));
	}
	TestTrue(TEXT("No stale diagnostics"), Diagnostics.IsEmpty());
	TestTrue(TEXT("Input unchanged"), Equal(FDnd5eSpell::StaticStruct(), &Spell, &Before));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eCollectionOrderTest, "Dnd5e.Content.References.OrderCopyAndOpaqueLeaves", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eCollectionOrderTest::RunTest(const FString& Parameters)
{
	FDnd5eReferenceContent Parent; Parent.SourceFormat = TEXT("other-format"); Parent.SourceReference = TEXT("opaque selector"); Parent.Display = {CollectionReference()};
	FDnd5eParagraphContent Body; Body.Inlines = {Text(TEXT("See ")), Inline(Dnd5e::Content::Inline::Reference, Parent)};
	FDnd5eUnsupportedContent Fallback;
	FDnd5eAnnotationContent Annotation; Annotation.Value = FInstancedStruct::Make<FDnd5eReferenceContent>(Parent);
	Body.Inlines.Add(Inline(Dnd5e::Content::Inline::Annotation, Annotation));
	Body.Inlines.Add(Inline(Dnd5e::Content::Inline::Unsupported, Fallback));
	FDnd5eSpell Spell; Spell.Description = {Block(Dnd5e::Content::Block::Paragraph, Body), Block(Dnd5e::Content::Block::Unsupported, Fallback)};
	Spell.HigherLevelDescription = {ReferenceParagraph()}; const auto Before = Spell;
	TArray<FDnd5eContentReferenceOccurrence> Out; TArray<FDnd5eDiagnostic> Diagnostics;
	TestTrue(TEXT("Known fallback remains successful"), UDnd5eSpellContentLibrary::CollectSpellReferences(Spell, Out, Diagnostics));
	if (TestEqual(TEXT("Annotation Value is not Content; display references are"), Out.Num(), 3))
	{
		TestEqual(TEXT("Parent preorder"), Out[0].ContentPath, FString(TEXT("/Description/0/Payload/Inlines/1/Payload")));
		TestEqual(TEXT("Nested display preorder"), Out[1].ContentPath, Out[0].ContentPath + TEXT("/Display/0/Payload"));
		TestEqual(TEXT("Higher-level root follows description"), Out[2].ContentPath, FString(TEXT("/HigherLevelDescription/0/Payload/Inlines/0/Payload")));
		TestFalse(TEXT("No target ID is required"), Out[0].Reference.bHasTargetRecordId);
		Out[0].Reference.Display[0].Payload.GetMutable<FDnd5eReferenceContent>().SourceReference = TEXT("changed copy");
	}
	if (TestEqual(TEXT("One aggregated warning"), Diagnostics.Num(), 1))
	{
		TestTrue(TEXT("Opaque leaf warning and count"), Diagnostics[0].Code == FName(TEXT("Dnd5e.Content.References.OpaqueFallback")) && Diagnostics[0].Severity == EDnd5eDiagnosticSeverity::Warning && Diagnostics[0].Message.Contains(TEXT("2")));
		TestTrue(TEXT("Model locations are not invented JSON locations"), Diagnostics[0].JsonPointer.IsEmpty() && Diagnostics[0].RelativeFilePath.IsEmpty());
	}
	TestTrue(TEXT("Deep output edits preserve input"), Equal(FDnd5eSpell::StaticStruct(), &Spell, &Before));
	Spell = {}; TestTrue(TEXT("Empty spell succeeds"), UDnd5eSpellContentLibrary::CollectSpellReferences(Spell, Out, Diagnostics));
	TestTrue(TEXT("Empty spell clears old results and diagnostics"), Out.IsEmpty() && Diagnostics.IsEmpty());
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eCollectionFailureTest, "Dnd5e.Content.References.InvalidWrappersAndAtomicFailure", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eCollectionFailureTest::RunTest(const FString& Parameters)
{
	TArray<FDnd5eContentBlock> BadBlocks;
	FDnd5eContentBlock Missing; Missing.Kind = Dnd5e::Content::Block::Paragraph; BadBlocks.Add(Missing);
	BadBlocks.Add(Block(Dnd5e::Content::Block::Paragraph, FDnd5eTableContent{}));
	BadBlocks.Add(Block(Dnd5e::Content::Inline::Reference, FDnd5eReferenceContent{}));
	BadBlocks.Add(Block(FGameplayTag{}, FDnd5eParagraphContent{}));
	BadBlocks.Add(Block(Dnd5e::Content::Block::Paragraph, FDnd5eMoney{}));
	FDnd5eParagraphContent BadInline; BadInline.Inlines.AddDefaulted();
	BadBlocks.Add(Block(Dnd5e::Content::Block::Paragraph, BadInline));
	BadInline.Inlines = {Inline(Dnd5e::Content::Inline::Reference, FDnd5eTextContent{})};
	BadBlocks.Add(Block(Dnd5e::Content::Block::Paragraph, BadInline));
	for (const auto& Bad : BadBlocks)
	{
		FDnd5eSpell Spell; Spell.Description = {ReferenceParagraph()}; Spell.HigherLevelDescription = {Bad}; const auto Before = Spell;
		TArray<FDnd5eContentReferenceOccurrence> Out; Out.AddDefaulted(); TArray<FDnd5eDiagnostic> Diagnostics; Diagnostics.AddDefaulted(2);
		TestFalse(TEXT("Malformed wrapper fails"), UDnd5eSpellContentLibrary::CollectSpellReferences(Spell, Out, Diagnostics));
		TestTrue(TEXT("Earlier references are not returned partially"), Out.IsEmpty());
		if (TestEqual(TEXT("One fresh error"), Diagnostics.Num(), 1))
		{
			TestTrue(TEXT("Invalid Content error"), Diagnostics[0].Code == FName(TEXT("Dnd5e.Content.References.InvalidContent")) && Diagnostics[0].Severity == EDnd5eDiagnosticSeverity::Error);
		}
		TestTrue(TEXT("Invalid input preserved"), Equal(FDnd5eSpell::StaticStruct(), &Spell, &Before));
	}
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eCollectionLimitsTest, "Dnd5e.Content.References.InclusiveLimits", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eCollectionLimitsTest::RunTest(const FString& Parameters)
{
	TArray<FDnd5eContentReferenceOccurrence> Out; TArray<FDnd5eDiagnostic> Diagnostics;
	auto CheckLimit = [&]()
	{
		TestTrue(TEXT("No partial results on exceeded limit"), Out.IsEmpty());
		if (TestEqual(TEXT("Single limit diagnostic"), Diagnostics.Num(), 1)) { TestTrue(TEXT("Controlled limit error"), Diagnostics[0].Code == FName(TEXT("Dnd5e.Content.References.Limit")) && Diagnostics[0].Severity == EDnd5eDiagnosticSeverity::Error); }
	};
	FDnd5eSpell Spell;
	for (int32 Index = 0; Index < 5000; ++Index) { Spell.Description.Add(Block(Dnd5e::Content::Block::Paragraph, FDnd5eParagraphContent{})); }
	TestTrue(TEXT("Exactly 10000 structs succeeds"), UDnd5eSpellContentLibrary::CollectSpellReferences(Spell, Out, Diagnostics));
	Spell.HigherLevelDescription.Add(Block(Dnd5e::Content::Block::Paragraph, FDnd5eParagraphContent{}));
	TestFalse(TEXT("Budget shared between roots"), UDnd5eSpellContentLibrary::CollectSpellReferences(Spell, Out, Diagnostics)); CheckLimit();
	FDnd5eParagraphContent Paragraph;
	for (int32 Index = 0; Index < 1024; ++Index) { Paragraph.Inlines.Add(CollectionReference()); }
	Spell = {}; Spell.Description = {Block(Dnd5e::Content::Block::Paragraph, Paragraph)};
	TestTrue(TEXT("Exactly 1024 occurrences succeeds"), UDnd5eSpellContentLibrary::CollectSpellReferences(Spell, Out, Diagnostics)); TestEqual(TEXT("Duplicates retained"), Out.Num(), 1024);
	Paragraph.Inlines.Add(CollectionReference()); Spell.Description = {Block(Dnd5e::Content::Block::Paragraph, Paragraph)};
	TestFalse(TEXT("1025 occurrences fails"), UDnd5eSpellContentLibrary::CollectSpellReferences(Spell, Out, Diagnostics)); CheckLimit();
	auto DeepSpell = [](int32 Layers)
	{
		auto Child = CollectionReference();
		for (int32 Index = 0; Index < Layers; ++Index) { FDnd5eFormattedContent Formatted; Formatted.Inlines = {Child}; Child = Inline(Dnd5e::Content::Inline::Formatted, Formatted); }
		FDnd5eParagraphContent Body; Body.Inlines = {Child}; FDnd5eListItemContent Item; Item.Blocks = {Block(Dnd5e::Content::Block::Paragraph, Body)};
		FDnd5eListContent List; List.Items = {Item}; FDnd5eSpell Result; Result.Description = {Block(Dnd5e::Content::Block::List, List)}; return Result;
	};
	Spell = DeepSpell(29);
	TestTrue(TEXT("Reference payload at exactly depth 64 succeeds"), UDnd5eSpellContentLibrary::CollectSpellReferences(Spell, Out, Diagnostics)); TestEqual(TEXT("Deep reference found"), Out.Num(), 1);
	Spell = DeepSpell(30);
	TestFalse(TEXT("Depth greater than 64 fails"), UDnd5eSpellContentLibrary::CollectSpellReferences(Spell, Out, Diagnostics)); CheckLimit();
	Spell = {}; Spell.Description = {ReferenceParagraph()};
	TestTrue(TEXT("Budgets reset on next call"), UDnd5eSpellContentLibrary::CollectSpellReferences(Spell, Out, Diagnostics)); TestTrue(TEXT("No stale errors"), Out.Num() == 1 && Diagnostics.IsEmpty());
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eCollectionReaderTest, "Dnd5e.Content.References.ReaderToCollection", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eCollectionReaderTest::RunTest(const FString& Parameters)
{
	FDnd5eJsonSpellReadRequest Request; Request.SpellIndex = 0; Request.Context.DatasetNamespace = TEXT("collection-synthetic");
	Request.JsonText = TEXT(R"json({"spell":[{"name":"Invented Caller","source":"SYN","level":0,"school":"V","time":[{"number":1,"unit":"action"}],"range":{"type":"point","distance":{"type":"self"}},"components":{"v":true},"duration":[{"type":"instant"}],"entries":["See {@spell Invented Beacon|SYN}."],"entriesHigherLevel":["See {@item Invented Item|SYN}."]}]})json");
	const auto Read = FDnd5eJsonSpellReader::ReadSpell(Request);
	if (!TestEqual(TEXT("Read synthetic document"), Read.Status, EDnd5eJsonReadStatus::Success)) { return false; }
	TArray<FDnd5eContentReferenceOccurrence> Out; TArray<FDnd5eDiagnostic> Diagnostics;
	TestTrue(TEXT("Collect reader output"), UDnd5eSpellContentLibrary::CollectSpellReferences(Read.Spell, Out, Diagnostics));
	if (TestEqual(TEXT("Spell and non-spell typed references both collected"), Out.Num(), 2))
	{
		TestEqual(TEXT("Primary path"), Out[0].ContentPath, FString(TEXT("/Description/0/Payload/Inlines/1/Payload")));
		TestEqual(TEXT("Higher path"), Out[1].ContentPath, FString(TEXT("/HigherLevelDescription/0/Payload/Inlines/1/Payload")));
		TestTrue(TEXT("No ID inference for non-spell reference"), !Out[1].Reference.bHasTargetRecordId);
	}
	return !HasAnyErrors();
}

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FDnd5eExternalCollectionTest, "Dnd5e.Json.ExternalReferenceCollection", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
void FDnd5eExternalCollectionTest::GetTests(TArray<FString>& Names, TArray<FString>& Commands) const
{
	FString Directory;
	if (FParse::Value(FCommandLine::Get(), TEXT("Dnd5eJsonSmokeDirectory="), Directory)) { Names.Add(TEXT("SuppliedSpellDirectory")); Commands.Add(Directory); }
}
bool FDnd5eExternalCollectionTest::RunTest(const FString& Directory)
{
	TArray<FString> Files; IFileManager::Get().FindFiles(Files, *(Directory / TEXT("spells-*.json")), true, false); Files.Sort();
	if (!TestTrue(TEXT("Explicit directory has spell files"), !Files.IsEmpty())) { return false; }
	int32 RecordCount = 0; int32 ReferenceCount = 0; int32 FallbackRecords = 0;
	for (const auto& File : Files)
	{
		FDnd5eJsonSpellReadRequest Request; Request.Context.DatasetNamespace = TEXT("external-collection-audit"); Request.Context.RelativeFilePath = File;
		if (!TestTrue(TEXT("Load supplied file"), FFileHelper::LoadFileToString(Request.JsonText, *(Directory / File)))) { return false; }
		TSharedPtr<FJsonObject> Root; const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
		if (!TestTrue(TEXT("Source document spell array"), FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Request.JsonText), Root) && Root.IsValid() && Root->TryGetArrayField(TEXT("spell"), Values))) { return false; }
		for (int32 Index = 0; Index < Values->Num(); ++Index)
		{
			Request.SpellIndex = Index; const auto Read = FDnd5eJsonSpellReader::ReadSpell(Request);
			if (!TestEqual(TEXT("Read before collection"), Read.Status, EDnd5eJsonReadStatus::Success)) { return false; }
			TArray<FDnd5eContentReferenceOccurrence> Out; TArray<FDnd5eDiagnostic> Diagnostics;
			if (!TestTrue(TEXT("Collect every converted spell within bounds"), UDnd5eSpellContentLibrary::CollectSpellReferences(Read.Spell, Out, Diagnostics))) { return false; }
			TSet<FString> Paths;
			for (const auto& Occurrence : Out)
			{
				TestFalse(TEXT("Each occurrence has a unique model location"), Paths.Contains(Occurrence.ContentPath)); Paths.Add(Occurrence.ContentPath);
				TestTrue(TEXT("Location is rooted in a spell description"), Occurrence.ContentPath.StartsWith(TEXT("/Description/")) || Occurrence.ContentPath.StartsWith(TEXT("/HigherLevelDescription/")));
				TestTrue(TEXT("Retained typed reference has source context"), Occurrence.Reference.SourceFormat == TEXT("5etools") && !Occurrence.Reference.SourceReference.IsEmpty());
			}
			if (!Diagnostics.IsEmpty()) { ++FallbackRecords; }
			++RecordCount; ReferenceCount += Out.Num();
		}
	}
	TestTrue(TEXT("External snapshot exercises references"), ReferenceCount > 0);
	AddInfo(FString::Printf(TEXT("Collected %d typed reference occurrences across %d spells in %d files; %d spells returned opaque-fallback warnings. No passages are logged; this does not count references hidden in fallback text."), ReferenceCount, RecordCount, Files.Num(), FallbackRecords));
	return !HasAnyErrors();
}

#endif
