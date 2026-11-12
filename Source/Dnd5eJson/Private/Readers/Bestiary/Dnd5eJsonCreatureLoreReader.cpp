// Developed by Neko Creative Technologies

#include "Readers/Bestiary/Dnd5eJsonCreatureLoreReader.h"
#include "Dnd5eTags.h"
#include "Readers/Dnd5eJsonReaderInternal.h"

namespace
{
    FDnd5eRecordId MakeLoreJoinKey(const FString& Namespace, const FString& Name, const FString& Source)
    {
        FDnd5eRecordId Key;
        Key.DatasetNamespace = Namespace;
        Key.Category = Dnd5e::Category::Creature;
        Key.SourceKey = Dnd5eJsonReader::EscapeIdentityComponent(Name) + TEXT("|") +
            Dnd5eJsonReader::EscapeIdentityComponent(Source);
        return Key;
    }

    struct FPendingCreatureLore
    {
        int32 CreatureIndex = INDEX_NONE;
        TArray<FDnd5eContentBlock> Entries;
        FDnd5eRecordProvenance Provenance;
    };
}

bool Dnd5eJsonCreatureLoreReader::ReadAndAttach(const FDnd5eJsonDocument& Document,
    TArray<FDnd5eCreature>& Creatures, TArray<FDnd5eDiagnostic>& Diagnostics)
{
    using namespace Dnd5eJsonReader;
    FReadState State{Document.Context, Document.JsonText, Diagnostics, FString{}};
    TSharedPtr<FJsonValue> RootValue;
    TSharedPtr<FJsonObject> Root;
    const TArray<TSharedPtr<FJsonValue>>* Rows = nullptr;
    if (!ValidateReadContext(State) || !ValidateDocumentText(State) || !PreflightJson(State) ||
        !ParseJsonDocument(RootValue, State) || !RequireObject(RootValue, TEXT(""), Root, State) ||
        !ReadRequiredArray(*Root, TEXT("monsterFluff"), TEXT(""), Rows, State))
    {
        return false;
    }
    if (Rows->Num() > 100000)
    {
        return Fail(State, Codes::ResourceLimitExceeded, TEXT("/monsterFluff"), TEXT("Too many creature lore rows."));
    }

    TMap<FDnd5eRecordId, int32> Primary;
    for (int32 Index = 0; Index < Creatures.Num(); ++Index)
    {
        const FDnd5eRecordHeader& Header = Creatures[Index].Header;
        if (Header.Provenance.IsEmpty()) { continue; }
        const FDnd5eRecordId Key = MakeLoreJoinKey(Header.RecordId.DatasetNamespace,
            Header.DisplayName, Header.Provenance[0].SourceId);
        if (Primary.Contains(Key))
        {
            return Fail(State, Codes::InvalidFieldValue, TEXT("/monsterFluff"),
                TEXT("Duplicate primary creature identity before lore joining."));
        }
        Primary.Add(Key, Index);
    }

    TSet<FDnd5eRecordId> Seen;
    TArray<FPendingCreatureLore> Pending;
    int32 Converted = 0;
    for (int32 Index = 0; Index < Rows->Num(); ++Index)
    {
        const FString Pointer = AppendPointer(TEXT("/monsterFluff"), FString::FromInt(Index));
        TSharedPtr<FJsonObject> Object;
        FString Name;
        FString Source;
        if (!RequireObject((*Rows)[Index], Pointer, Object, State) ||
            !ReadRequiredString(*Object, TEXT("name"), Pointer, Name, State) ||
            !ReadRequiredString(*Object, TEXT("source"), Pointer, Source, State) ||
            !ValidateIdentityText(Name, AppendPointer(Pointer, TEXT("name")), State) ||
            !ValidateIdentityText(Source, AppendPointer(Pointer, TEXT("source")), State))
        {
            return false;
        }

        const FDnd5eRecordId Key = MakeLoreJoinKey(Document.Context.DatasetNamespace, Name, Source);
        FReadState RowState{Document.Context, Document.JsonText, Diagnostics, Source};
        if (Seen.Contains(Key))
        {
            return Fail(RowState, Codes::InvalidFieldValue, Pointer, TEXT("Duplicate creature lore identity."));
        }
        Seen.Add(Key);

        const int32* CreatureIndex = Primary.Find(Key);
        if (!CreatureIndex) { continue; }
        const bool bCopy = FindExactField(*Object, TEXT("_copy")).IsValid();
        if (bCopy || FindExactField(*Object, TEXT("_appendCopy")))
        {
            AddDiagnostic(RowState, EDnd5eDiagnosticSeverity::Warning, Codes::PreservedUnsupportedMetadata,
                AppendPointer(Pointer, bCopy ? TEXT("_copy") : TEXT("_appendCopy")),
                TEXT("Creature lore copy directives are outside this reader's direct-row support."));
            continue;
        }

        const auto Entries = FindExactField(*Object, TEXT("entries"));
        if (!Entries) { continue; }
        FPendingCreatureLore Attachment;
        Attachment.CreatureIndex = *CreatureIndex;
        Attachment.Provenance.SourceId = Source;
        Attachment.Provenance.RelativeFilePath = Document.Context.RelativeFilePath;
        Attachment.Provenance.JsonPointer = Pointer;
        if (!ReadBlocks(Entries, AppendPointer(Pointer, TEXT("entries")), Attachment.Entries, RowState))
        {
            return false;
        }
        if (RowState.Budget->ContentValues > 1000000 - Converted)
        {
            return Fail(RowState, Codes::ResourceLimitExceeded, Pointer, TEXT("Creature lore conversion budget exceeded."));
        }
        Converted += RowState.Budget->ContentValues;
        if (!Attachment.Entries.IsEmpty()) { Pending.Add(MoveTemp(Attachment)); }
    }

    for (FPendingCreatureLore& Attachment : Pending)
    {
        FDnd5eCreature& Creature = Creatures[Attachment.CreatureIndex];
        Creature.SupplementaryContent.Entries = MoveTemp(Attachment.Entries);
        Creature.Header.Provenance.Add(MoveTemp(Attachment.Provenance));
    }
    return true;
}