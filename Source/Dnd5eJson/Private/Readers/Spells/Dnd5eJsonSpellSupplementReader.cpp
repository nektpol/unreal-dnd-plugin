// Developed by Neko Creative Technologies

#include "Readers/Spells/Dnd5eJsonSpellSupplementReader.h"
#include "Readers/Dnd5eJsonReaderInternal.h"

bool FDnd5eJsonSpellSupplementReader::ReadDocument(const FDnd5eJsonDocument& Document,
	TArray<FDnd5eJsonSpellSupplement>& Rows, int32& ConvertedValues, TArray<FDnd5eDiagnostic>& Diagnostics)
{
	using namespace Dnd5eJsonReader;
	Rows.Reset(); ConvertedValues = 0; Diagnostics.Reset();
	FReadState State{Document.Context, Document.JsonText, Diagnostics, FString{}};
	TSharedPtr<FJsonValue> RootValue; TSharedPtr<FJsonObject> Root; const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
	if (!ValidateReadContext(State) || !ValidateDocumentText(State) || !PreflightJson(State) || !ParseJsonDocument(RootValue, State) ||
		!RequireObject(RootValue, TEXT(""), Root, State) || !ReadRequiredArray(*Root, TEXT("spellFluff"), TEXT(""), Values, State)) { return false; }
	if (Values->Num() > 100000) { return Fail(State, Codes::ResourceLimitExceeded, TEXT("/spellFluff"), TEXT("Too many supplementary records.")); }
	TArray<FDnd5eJsonSpellSupplement> Candidate; int32 Total = 0;
	for (int32 Index = 0; Index < Values->Num(); ++Index)
	{
		const FString Pointer = AppendPointer(TEXT("/spellFluff"), FString::FromInt(Index));
		TSharedPtr<FJsonObject> Object; FDnd5eJsonSpellSupplement Row;
		if (!RequireObject((*Values)[Index], Pointer, Object, State) ||
			!CheckAllowedKeys(*Object, {TEXT("name"), TEXT("source"), TEXT("entries"), TEXT("images")}, Pointer, State) ||
			!ReadRequiredString(*Object, TEXT("name"), Pointer, Row.Name, State) || !ReadRequiredString(*Object, TEXT("source"), Pointer, Row.SourceId, State) ||
			!ValidateIdentityText(Row.Name, AppendPointer(Pointer, TEXT("name")), State) || !ValidateIdentityText(Row.SourceId, AppendPointer(Pointer, TEXT("source")), State)) { return false; }
		Row.DatasetNamespace = Document.Context.DatasetNamespace; Row.Provenance.SourceId = Row.SourceId;
		Row.Provenance.RelativeFilePath = Document.Context.RelativeFilePath; Row.Provenance.JsonPointer = Pointer;
		FReadState Child{Document.Context, Document.JsonText, Diagnostics, Row.SourceId};
		if (const auto Entries = FindExactField(*Object, TEXT("entries")))
		{
			if (!ReadBlocks(Entries, AppendPointer(Pointer, TEXT("entries")), Row.Content.Entries, Child)) { return false; }
		}
		if (const auto Images = FindExactField(*Object, TEXT("images")))
		{
			if (!ReadSupplementaryImages(Images, AppendPointer(Pointer, TEXT("images")), Row.Content.Images, Child)) { return false; }
		}
		if (Child.Budget->ContentValues > 1000000 - Total) { return Fail(Child, Codes::ResourceLimitExceeded, Pointer, TEXT("Supplement conversion budget exceeded.")); }
		Total += Child.Budget->ContentValues; Candidate.Add(MoveTemp(Row));
	}
	Rows = MoveTemp(Candidate); ConvertedValues = Total; return true;
}
