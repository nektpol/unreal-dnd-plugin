#if WITH_DEV_AUTOMATION_TESTS
#include "Dnd5eCoverageTestSupport.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
using namespace Dnd5eTests;

namespace
{
	void RawTokens(const TSharedPtr<FJsonValue>& Value, TMap<FString,int32>& Counts)
	{
		if (Value->Type == EJson::String)
		{
			const FString Text = Value->AsString(); int32 Position = 0;
			while ((Position = Text.Find(TEXT("{@"), ESearchCase::CaseSensitive, ESearchDir::FromStart, Position)) != INDEX_NONE)
			{
				int32 End = Position + 2; while (End < Text.Len() && !FChar::IsWhitespace(Text[End]) && Text[End] != '}') { ++End; }
				++Counts.FindOrAdd(Text.Mid(Position + 2, End - Position - 2)); Position += 2;
			}
		}
		else if (Value->Type == EJson::Array) { for (const auto& Child : Value->AsArray()) { RawTokens(Child, Counts); } }
		else if (Value->Type == EJson::Object) { for (const auto& Pair : Value->AsObject()->Values) { RawTokens(Pair.Value, Counts); } }
	}
	void TypedTokens(const UScriptStruct* Type, const void* Data, TMap<FString,int32>& Counts, int32& Opaque, int32& Images, int32 Depth = 0)
	{
		if (Depth > 96) { return; }
		if (Type == FDnd5eUnsupportedContent::StaticStruct()) { ++Opaque; }
		if (Type == FDnd5eImageContent::StaticStruct()) { ++Images; }
		FString Token;
		if (Type == FDnd5eAnnotationContent::StaticStruct()) { Token = static_cast<const FDnd5eAnnotationContent*>(Data)->SourceAnnotation; }
		for (const auto* Candidate : {FDnd5eReferenceContent::StaticStruct(), FDnd5eFormattedContent::StaticStruct(), FDnd5eDocumentLinkContent::StaticStruct(), FDnd5eQuickReferenceContent::StaticStruct(), FDnd5eQueryLinkContent::StaticStruct(), FDnd5eExternalLinkContent::StaticStruct()})
		{
			if (Type == Candidate) { const auto* Field = FindFProperty<FStrProperty>(Type, TEXT("SourceReference")); Token = Field->GetPropertyValue_InContainer(Data); }
		}
		if (Token.StartsWith(TEXT("{@"))) { int32 End = 2; while (End < Token.Len() && !FChar::IsWhitespace(Token[End]) && Token[End] != '}') { ++End; } ++Counts.FindOrAdd(Token.Mid(2, End - 2)); }
		if (Type == FInstancedStruct::StaticStruct()) { const auto& Value = *static_cast<const FInstancedStruct*>(Data); if (Value.IsValid()) { TypedTokens(Value.GetScriptStruct(), Value.GetMemory(), Counts, Opaque, Images, Depth + 1); } return; }
		for (TFieldIterator<FProperty> It(Type); It; ++It)
		{
			if (const auto* Struct = CastField<FStructProperty>(*It)) { TypedTokens(Struct->Struct, Struct->ContainerPtrToValuePtr<void>(Data), Counts, Opaque, Images, Depth + 1); }
			else if (const auto* Array = CastField<FArrayProperty>(*It))
			{
				if (const auto* Element = CastField<FStructProperty>(Array->Inner)) { FScriptArrayHelper Values(Array, Array->ContainerPtrToValuePtr<void>(Data)); for (int32 Index = 0; Index < Values.Num(); ++Index) { TypedTokens(Element->Struct, Values.GetRawPtr(Index), Counts, Opaque, Images, Depth + 1); } }
			}
		}
	}
}

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FDnd5eCoverageAuditTest, "Dnd5e.Json.ExternalCoverageCompletion", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
void FDnd5eCoverageAuditTest::GetTests(TArray<FString>& Names, TArray<FString>& Commands) const
{
	FString Directory; if (FParse::Value(FCommandLine::Get(), TEXT("Dnd5eJsonSmokeDirectory="), Directory)) { Names.Add(TEXT("ExplicitPrimaryAndFluffDirectory")); Commands.Add(Directory); }
}
bool FDnd5eCoverageAuditTest::RunTest(const FString& Directory)
{
	FDnd5eJsonSpellCatalogBuildRequest Request; Request.bValidateContent = true;
	TMap<FString,int32> Raw, Typed; int32 Primary = 0, Fluff = 0, RawImages = 0;
	for (bool Supplement : {false,true})
	{
		TArray<FString> Files; IFileManager::Get().FindFiles(Files, *(Directory / (Supplement ? TEXT("fluff-spells-*.json") : TEXT("spells-*.json"))), true, false); Files.Sort();
		if (!TestFalse(TEXT("Explicitly selected input family has files"), Files.IsEmpty())) { return false; }
		for (const auto& File : Files)
		{
			FDnd5eJsonDocument Document; Document.Context.DatasetNamespace = TEXT("external-completion"); Document.Context.RelativeFilePath = File;
			if (!TestTrue(TEXT("Load explicit supplied document"), FFileHelper::LoadFileToString(Document.JsonText, *(Directory / File)))) { return false; }
			TSharedPtr<FJsonObject> Root; if (!TestTrue(TEXT("Source root parses"), FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Document.JsonText), Root))) { return false; }
			const auto& Rows = Root->GetArrayField(Supplement ? TEXT("spellFluff") : TEXT("spell"));
			for (const auto& Row : Rows)
			{
				RawTokens(Row, Raw);
				if (Supplement) { ++Fluff; const TArray<TSharedPtr<FJsonValue>>* Images = nullptr; if (Row->AsObject()->TryGetArrayField(TEXT("images"), Images)) { RawImages += Images->Num(); } }
				else { ++Primary; }
			}
			if (Supplement) { Request.SpellFluffDocuments.Add(MoveTemp(Document)); } else { Request.SpellDocuments.Add(MoveTemp(Document)); }
		}
	}
	FDnd5eSpellCatalog Catalog; const auto Result = FDnd5eJsonSpellCatalogBuilder::BuildCatalog(Request, Catalog);
	if (Result.Status != EDnd5eJsonReadStatus::Success)
	{
		for (const auto& Issue : Result.ContentValidationIssues) { AddError(Issue.Diagnostic.Code.ToString() + TEXT(" at model ") + Issue.ContentPath); }
		for (const auto& Issue : Result.Diagnostics) { if (Issue.Severity == EDnd5eDiagnosticSeverity::Error) { AddError(Issue.Code.ToString() + TEXT(" at ") + Issue.RelativeFilePath + TEXT(" ") + Issue.JsonPointer); } }
		return false;
	}
	TestEqual(TEXT("All primary records loaded"), Catalog.GetSpellCount(), Primary);
	int32 Opaque = 0, Images = 0, Attached = 0;
	for (const auto& Id : Catalog.GetSpellIds()) { FDnd5eSpell Spell; Catalog.TryGetSpell(Id, Spell); TypedTokens(FDnd5eSpell::StaticStruct(), &Spell, Typed, Opaque, Images); if (Spell.bHasSupplementaryContent) { ++Attached; } }
	for (const auto& Row : Result.UnmatchedSupplements) { TypedTokens(FDnd5eSupplementaryContent::StaticStruct(), &Row.Content, Typed, Opaque, Images); }
	TestEqual(TEXT("No supplementary rows disappear"), Attached + Result.UnmatchedSupplements.Num(), Fluff);
	TestEqual(TEXT("All supplied image definitions are typed"), Images, RawImages);
	for (const auto& Pair : Raw) { TestEqual(*(TEXT("Raw source and typed semantic occurrences: ") + Pair.Key), Typed.FindRef(Pair.Key), Pair.Value); }
	TestEqual(TEXT("No unexpected extra source-token kinds"), Typed.Num(), Raw.Num());
	TestEqual(TEXT("Observed forms have no opaque payloads"), Opaque, 0);
	int32 Warnings = 0; for (const auto& D : Result.Diagnostics) { if (D.Severity == EDnd5eDiagnosticSeverity::Warning) { ++Warnings; } }
	AddInfo(FString::Printf(TEXT("Audited %d primary / %d supplementary rows, %d images, %d source-token kinds; %d unresolved/preservation warnings. No source passages logged."), Primary, Fluff, Images, Raw.Num(), Warnings));
	return !HasAnyErrors();
}
#endif
