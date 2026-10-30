// Developed by Neko Creative Technologies
#if WITH_DEV_AUTOMATION_TESTS

#include "Dnd5eTestSupport.h"
#include "Readers/Dnd5eJsonSpellReader.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"

using namespace Dnd5eTests;

namespace
{
	using R = EDnd5eReferenceResolveStatus;
	using N = EDnd5eJsonSpellReferenceNormalizeStatus;
	FDnd5eSpell ReportTarget(const TCHAR* Key = TEXT("opaque/A"), const TCHAR* Name = TEXT("Invented Beacon"), const TCHAR* Source = TEXT("SYN"))
	{
		auto Spell = SyntheticSpell();
		Spell.Header.RecordId.DatasetNamespace = TEXT("report-synthetic"); Spell.Header.RecordId.Category = Dnd5e::Category::Spell; Spell.Header.RecordId.SourceKey = Key;
		Spell.Header.DisplayName = Name; Spell.Header.Provenance.Reset(); FDnd5eRecordProvenance Provenance; Provenance.SourceId = Source; Spell.Header.Provenance.Add(Provenance);
		return Spell;
	}
	FDnd5eReferenceContent ReportReference()
	{
		FDnd5eReferenceContent Reference; Reference.SourceFormat = TEXT("5etools"); Reference.SourceReference = TEXT("{@spell invented beacon|syn}");
		Reference.Display = {Text(TEXT("A label"))}; Reference.bHasTargetRecordId = true; Reference.TargetRecordId = ReportTarget().Header.RecordId;
		return Reference;
	}
	FDnd5eSpell ReportOwner(const FDnd5eReferenceContent& Reference)
	{
		FDnd5eSpell Spell; Spell.Header.RecordId.DatasetNamespace = TEXT("report-synthetic");
		FDnd5eParagraphContent Body; Body.Inlines = {Text(TEXT("See ")), Inline(Dnd5e::Content::Inline::Reference, Reference)};
		Spell.Description = {Block(Dnd5e::Content::Block::Paragraph, Body)}; return Spell;
	}
	struct FReportCase
	{
		FString Name; FDnd5eReferenceContent Reference; R Original; N Normalization;
		FString Namespace = TEXT("report-synthetic"); bool bEmpty = false; bool bAmbiguous = false;
	};
	TArray<FReportCase> ReportCases()
	{
		const auto Base = ReportReference(); TArray<FReportCase> Cases;
		auto Add = [&](const TCHAR* Name, const FDnd5eReferenceContent& Reference, R Original, N Normalization) { Cases.Add({Name, Reference, Original, Normalization}); };
		Add(TEXT("BothResolved"), Base, R::Resolved, N::Normalized);
		auto Value = Base; Value.TargetRecordId.SourceKey = TEXT("not-canonical"); Add(TEXT("CanonicalCorrection"), Value, R::TargetNotFound, N::Normalized);
		Value = Base; Value.bHasTargetRecordId = false; Add(TEXT("MissingDeclaredTarget"), Value, R::MissingTargetId, N::Normalized);
		Value = Base; Value.TargetRecordId.SourceKey.Reset(); Add(TEXT("InvalidDeclaredTarget"), Value, R::InvalidTargetId, N::Normalized);
		Value = Base; Value.TargetRecordId.Category = Dnd5e::Category::Feat; Add(TEXT("UnsupportedDeclaredCategory"), Value, R::UnsupportedCategory, N::Normalized);
		Value = Base; Value.SourceFormat = TEXT("other"); Add(TEXT("OriginalResolvedUnsupportedFormat"), Value, R::Resolved, N::UnsupportedReference);
		Value = Base; Value.SourceReference = TEXT("{@spell invented beacon}"); Add(TEXT("OriginalResolvedMissingSource"), Value, R::Resolved, N::MissingSource);
		Value = Base; Value.SourceReference = TEXT("{@spell absent|SYN}"); Add(TEXT("OriginalResolvedNoMatch"), Value, R::Resolved, N::NoMatch);
		Value = Base; Value.TargetRecordId = ReportTarget(TEXT("opaque/B"), TEXT("Invented Spark"), TEXT("ALT")).Header.RecordId;
		Add(TEXT("ConflictingResolvedTargets"), Value, R::Resolved, N::Normalized);
		Value = Base; Value.SourceReference = TEXT("{@item Invented Item|SYN}"); Value.bHasTargetRecordId = false; Add(TEXT("NonSpellReference"), Value, R::MissingTargetId, N::UnsupportedReference);
		Value = Base; Value.SourceReference = TEXT("{@spell |SYN}"); Add(TEXT("InvalidName"), Value, R::Resolved, N::InvalidSelector);
		Add(TEXT("InvalidOwnerNamespace"), Base, R::Resolved, N::InvalidSelector); Cases.Last().Namespace.Reset();
		Add(TEXT("ExactNamespaceIsolation"), Base, R::Resolved, N::NoMatch); Cases.Last().Namespace = TEXT("Report-synthetic");
		Add(TEXT("OriginalResolvedAmbiguousSelector"), Base, R::Resolved, N::Ambiguous); Cases.Last().bAmbiguous = true;
		Add(TEXT("EmptyCatalog"), Base, R::TargetNotFound, N::NoMatch); Cases.Last().bEmpty = true;
		Value = Base; Value.SourceReference = TEXT("{@spell invented beacon}"); Add(TEXT("MissingSourceEmptyCatalog"), Value, R::TargetNotFound, N::MissingSource); Cases.Last().bEmpty = true;
		return Cases;
	}
}

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FDnd5eReferenceReportMatrixTest, "Dnd5e.Json.ReferenceReport.Stages", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
void FDnd5eReferenceReportMatrixTest::GetTests(TArray<FString>& Names, TArray<FString>& Commands) const
{
	for (const auto& Case : ReportCases()) { Names.Add(Case.Name); Commands.Add(Case.Name); }
}
bool FDnd5eReferenceReportMatrixTest::RunTest(const FString& Parameters)
{
	const auto Cases = ReportCases(); const auto* Case = Cases.FindByPredicate([&](const auto& Value) { return Value.Name == Parameters; });
	if (!TestNotNull(TEXT("Known report case"), Case)) { return false; }
	TArray<FDnd5eSpell> Records;
	if (!Case->bEmpty) { Records = {ReportTarget(), ReportTarget(TEXT("opaque/B"), TEXT("Invented Spark"), TEXT("ALT"))}; }
	if (Case->bAmbiguous) { Records.Add(ReportTarget(TEXT("opaque/duplicate"))); }
	FDnd5eSpellCatalog Catalog; TArray<FDnd5eDiagnostic> Diagnostics;
	if (!TestTrue(TEXT("Install"), Catalog.ReplaceSpells(Records, Diagnostics))) { return false; }
	auto Owner = ReportOwner(Case->Reference); Owner.Header.RecordId.DatasetNamespace = Case->Namespace; const auto Before = Owner;
	const auto Report = FDnd5eJsonSpellReferenceReporter::BuildReport(Catalog, Owner);
	TestTrue(TEXT("Collection succeeds independently of targets"), Report.bCollected && Report.Diagnostics.IsEmpty());
	if (TestEqual(TEXT("One report row"), Report.Entries.Num(), 1))
	{
		const auto& Entry = Report.Entries[0];
		TestEqual(TEXT("Location retained"), Entry.Occurrence.ContentPath, FString(TEXT("/Description/0/Payload/Inlines/1/Payload")));
		TestTrue(TEXT("Original retained"), Equal(FDnd5eReferenceContent::StaticStruct(), &Entry.Occurrence.Reference, &Case->Reference));
		TestEqual(TEXT("Independent original outcome"), Entry.OriginalResolutionStatus, Case->Original);
		TestEqual(TEXT("Independent normalization outcome"), Entry.NormalizationStatus, Case->Normalization);
		TestEqual(TEXT("Original diagnostics stay with original stage"), Entry.OriginalResolutionDiagnostics.Num(), Case->Original == R::Resolved ? 0 : 1);
		TestEqual(TEXT("Normalization diagnostics stay with normalization stage"), Entry.NormalizationDiagnostics.Num(), Case->Normalization == N::Normalized ? 0 : 1);
		TestEqual(TEXT("Second resolution only after Normalized"), Entry.NormalizedResolutionStatus, Case->Normalization == N::Normalized ? R::Resolved : R::NotAttempted);
		TestTrue(TEXT("No diagnostics from unattempted/successful second lookup"), Entry.NormalizedResolutionDiagnostics.IsEmpty());
		auto Expected = Case->Reference;
		if (Case->Normalization == N::Normalized) { Expected.bHasTargetRecordId = true; Expected.TargetRecordId = Records[0].Header.RecordId; }
		TestTrue(TEXT("Normalized copy follows the normalizer policy"), Equal(FDnd5eReferenceContent::StaticStruct(), &Entry.NormalizedReference, &Expected));
	}
	TestTrue(TEXT("Owner immutable"), Equal(FDnd5eSpell::StaticStruct(), &Owner, &Before));
	for (const auto& Record : Records)
	{
		FDnd5eSpell Again; TestTrue(TEXT("Catalog immutable"), Catalog.TryGetSpell(Record.Header.RecordId, Again) && Equal(FDnd5eSpell::StaticStruct(), &Again, &Record));
	}
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDnd5eReferenceReportLifecycleTest, "Dnd5e.Json.ReferenceReport.CollectionCopyAndReplacement", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDnd5eReferenceReportLifecycleTest::RunTest(const FString& Parameters)
{
	FDnd5eSpellCatalog Catalog; TArray<FDnd5eDiagnostic> Diagnostics; Catalog.ReplaceSpells({ReportTarget()}, Diagnostics);
	auto Owner = ReportOwner(ReportReference()); Owner.HigherLevelDescription = Owner.Description;
	FDnd5eUnsupportedContent Fallback; Owner.Description.Add(Block(Dnd5e::Content::Block::Unsupported, Fallback)); const auto Before = Owner;
	auto Report = FDnd5eJsonSpellReferenceReporter::BuildReport(Catalog, Owner);
	TestTrue(TEXT("Warning collection succeeds with two ordered rows"), Report.bCollected && Report.Entries.Num() == 2 && Report.Diagnostics.Num() == 1);
	if (Report.Entries.Num() != 2) { return false; }
	TestTrue(TEXT("Higher root follows primary root"), Report.Entries[1].Occurrence.ContentPath.StartsWith(TEXT("/HigherLevelDescription/")));
	Report.Entries[0].NormalizedReference.Display[0].Payload.GetMutable<FDnd5eTextContent>().Text = TEXT("changed");
	TestTrue(TEXT("Normalized and original copies are independent"), Report.Entries[0].Occurrence.Reference.Display[0].Payload.Get<FDnd5eTextContent>().Text == TEXT("A label"));
	TestTrue(TEXT("Owner copy isolation"), Equal(FDnd5eSpell::StaticStruct(), &Owner, &Before));
	Catalog.ReplaceSpells({}, Diagnostics); const auto EmptyCatalogReport = FDnd5eJsonSpellReferenceReporter::BuildReport(Catalog, Owner);
	TestTrue(TEXT("Fresh lookup after clear"), EmptyCatalogReport.bCollected && EmptyCatalogReport.Entries[0].OriginalResolutionStatus == R::TargetNotFound && EmptyCatalogReport.Entries[0].NormalizationStatus == N::NoMatch);
	TestEqual(TEXT("Older report is a detached snapshot"), Report.Entries[0].NormalizationStatus, N::Normalized);
	Owner.HigherLevelDescription.AddDefaulted(); Report = FDnd5eJsonSpellReferenceReporter::BuildReport(Catalog, Owner);
	TestTrue(TEXT("Structural failure has no partial rows or previous warning"), !Report.bCollected && Report.Entries.IsEmpty() && Report.Diagnostics.Num() == 1 && Report.Diagnostics[0].Severity == EDnd5eDiagnosticSeverity::Error);
	Owner = {}; Report = FDnd5eJsonSpellReferenceReporter::BuildReport(Catalog, Owner);
	TestTrue(TEXT("Fresh empty success"), Report.bCollected && Report.Entries.IsEmpty() && Report.Diagnostics.IsEmpty());
	Owner = ReportOwner(ReportReference()); FDnd5eParagraphContent Many;
	for (int32 Index = 0; Index < 1025; ++Index) { Many.Inlines.Add(Inline(Dnd5e::Content::Inline::Reference, ReportReference())); }
	Owner.Description = {Block(Dnd5e::Content::Block::Paragraph, Many)}; Report = FDnd5eJsonSpellReferenceReporter::BuildReport(Catalog, Owner);
	TestTrue(TEXT("Collector limit propagated without partial report"), !Report.bCollected && Report.Entries.IsEmpty() && Report.Diagnostics.Num() == 1 && Report.Diagnostics[0].Code == FName(TEXT("Dnd5e.Content.References.Limit")));
	return !HasAnyErrors();
}

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FDnd5eExternalReportTest, "Dnd5e.Json.ExternalReferenceReport", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
void FDnd5eExternalReportTest::GetTests(TArray<FString>& Names, TArray<FString>& Commands) const
{
	FString Directory; if (FParse::Value(FCommandLine::Get(), TEXT("Dnd5eJsonSmokeDirectory="), Directory)) { Names.Add(TEXT("SuppliedSpellDirectory")); Commands.Add(Directory); }
}
bool FDnd5eExternalReportTest::RunTest(const FString& Directory)
{
	FDnd5eJsonSpellCatalogLoadRequest Request; Request.DatasetRoot = FPaths::ConvertRelativePathToFull(Directory); Request.DatasetNamespace = TEXT("external-report-audit");
	IFileManager::Get().FindFiles(Request.SpellFiles, *(Request.DatasetRoot / TEXT("spells-*.json")), true, false); Request.SpellFiles.Sort();
	if (!TestTrue(TEXT("Explicit files supplied"), !Request.SpellFiles.IsEmpty())) { return false; }
	FDnd5eSpellCatalog Catalog; const auto Build = FDnd5eJsonSpellCatalogLoader::LoadCatalog(Request, Catalog);
	if (!TestEqual(TEXT("Load catalog"), Build.Status, EDnd5eJsonReadStatus::Success)) { return false; }
	int32 Entries = 0; int32 OriginalResolved = 0; int32 Normalized = 0; int32 NormalizedResolved = 0; int32 FallbackOwners = 0;
	for (const auto& Id : Catalog.GetSpellIds())
	{
		FDnd5eSpell Spell; if (!TestTrue(TEXT("Retrieve input"), Catalog.TryGetSpell(Id, Spell))) { return false; }
		const auto Report = FDnd5eJsonSpellReferenceReporter::BuildReport(Catalog, Spell);
		if (!TestTrue(TEXT("Report collection completes"), Report.bCollected)) { return false; }
		TArray<FDnd5eContentReferenceOccurrence> Occurrences; TArray<FDnd5eDiagnostic> Diagnostics;
		UDnd5eSpellContentLibrary::CollectSpellReferences(Spell, Occurrences, Diagnostics);
		TestEqual(TEXT("Report includes every collected typed occurrence"), Report.Entries.Num(), Occurrences.Num());
		if (!Report.Diagnostics.IsEmpty()) { ++FallbackOwners; }
		for (const auto& Entry : Report.Entries)
		{
			++Entries; OriginalResolved += Entry.OriginalResolutionStatus == R::Resolved;
			Normalized += Entry.NormalizationStatus == N::Normalized; NormalizedResolved += Entry.NormalizedResolutionStatus == R::Resolved;
			TestTrue(TEXT("Second lookup gated by normalization"), Entry.NormalizedResolutionStatus == (Entry.NormalizationStatus == N::Normalized ? R::Resolved : R::NotAttempted));
		}
	}
	TestTrue(TEXT("Audit exercises reports"), Entries > 0);
	AddInfo(FString::Printf(TEXT("Reported %d occurrences across %d spells; %d original targets resolved, %d selectors normalized, %d normalized targets resolved, %d owners have opaque fallbacks. No source passages logged; hidden fallback references are not counted."), Entries, Catalog.GetSpellCount(), OriginalResolved, Normalized, NormalizedResolved, FallbackOwners));
	return !HasAnyErrors();
}

#endif
