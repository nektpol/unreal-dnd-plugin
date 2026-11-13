// Developed by Neko Creative Technologies

#include "Readers/Bestiary/Dnd5eJsonCreatureLoreReader.h"
#include "Dnd5eTags.h"
#include "Readers/Dnd5eJsonReaderInternal.h"

namespace
{
    using namespace Dnd5eJsonReader;

    struct FLoreRow
    {
        TSharedPtr<FJsonObject> Object;
        FString Source;
        FString Pointer;
        int32 DocumentIndex = INDEX_NONE;
    };

    struct FPendingCreatureLore
    {
        int32 CreatureIndex = INDEX_NONE;
        TArray<FDnd5eContentBlock> Entries;
        TArray<FDnd5eContentBlock> Images;
        FDnd5eRecordProvenance Provenance;
    };

    enum class ELoreChainResult : uint8
    {
        Resolved,
        Skipped,
        Invalid
    };

    FDnd5eRecordId MakeLoreJoinKey(const FString& Namespace, const FString& Name, const FString& Source)
    {
        FDnd5eRecordId Key;
        Key.DatasetNamespace = Namespace;
        Key.Category = Dnd5e::Category::Creature;
        Key.SourceKey = EscapeIdentityComponent(Name) + TEXT("|") + EscapeIdentityComponent(Source);
        return Key;
    }

    bool ReadLoreRows(const TArray<TSharedPtr<FJsonValue>>& Values, int32 DocumentIndex, FReadState& State,
           TArray<FLoreRow>& Rows, TMap<FDnd5eRecordId, int32>& Lookup, TArray<FDnd5eRecordId>& Keys)
    {
        for (int32 Index = 0; Index < Values.Num(); ++Index)
        {
            FLoreRow Row;
            Row.DocumentIndex = DocumentIndex;
            Row.Pointer = AppendPointer(TEXT("/monsterFluff"), FString::FromInt(Index));
            FString Name;
            if (!RequireObject(Values[Index], Row.Pointer, Row.Object, State) ||
                !ReadRequiredString(*Row.Object, TEXT("name"), Row.Pointer, Name, State) ||
                !ReadRequiredString(*Row.Object, TEXT("source"), Row.Pointer, Row.Source, State) ||
                !ValidateIdentityText(Name, AppendPointer(Row.Pointer, TEXT("name")), State) ||
                !ValidateIdentityText(Row.Source, AppendPointer(Row.Pointer, TEXT("source")), State))
            {
                return false;
            }

            FDnd5eRecordId Key = MakeLoreJoinKey(State.Context.DatasetNamespace, Name, Row.Source);
            if (Lookup.Contains(Key))
            {
                return Fail(State, Codes::InvalidFieldValue, Row.Pointer, TEXT("Duplicate creature lore identity."));
            }
            Lookup.Add(Key, Rows.Num());
            Keys.Add(MoveTemp(Key));
            Rows.Add(MoveTemp(Row));
        }
        return true;
    }

    ELoreChainResult CollectLoreChain(int32 StartIndex, const TArray<FLoreRow>& Rows,
        const TMap<FDnd5eRecordId, int32>& Lookup, const TArray<FDnd5eJsonDocument>& Documents,
        TArray<FDnd5eDiagnostic>& Diagnostics, TArray<int32>& Chain)
    {
        TSet<int32> Seen;
        int32 Index = StartIndex;
        for (;;)
        {
            const FLoreRow& Row = Rows[Index];
            const FDnd5eJsonDocument& Document = Documents[Row.DocumentIndex];
            FReadState RowState{Document.Context, Document.JsonText, Diagnostics, Row.Source};
            if (Seen.Contains(Index))
            {
                Fail(RowState, Codes::InvalidFieldValue, Row.Pointer, TEXT("Creature lore copy cycle."));
                return ELoreChainResult::Invalid;
            }
            if (Chain.Num() >= 64)
            {
                Fail(RowState, Codes::ResourceLimitExceeded, Row.Pointer, TEXT("Creature lore copy chain exceeds 64 rows."));
                return ELoreChainResult::Invalid;
            }
            Seen.Add(Index);
            Chain.Add(Index);
            if (FindExactField(*Row.Object, TEXT("_appendCopy")))
            {
                AddDiagnostic(RowState, EDnd5eDiagnosticSeverity::Warning, Codes::PreservedUnsupportedMetadata,
                    AppendPointer(Row.Pointer, TEXT("_appendCopy")), TEXT("Append-copy lore is outside this reader's supported forms."));
                return ELoreChainResult::Skipped;
            }

            const auto CopyValue = FindExactField(*Row.Object, TEXT("_copy"));
            if (!CopyValue) { return ELoreChainResult::Resolved; }
            const FString Pointer = AppendPointer(Row.Pointer, TEXT("_copy"));
            TSharedPtr<FJsonObject> Copy;
            FString ParentName;
            FString ParentSource;
            if (!RequireObject(CopyValue, Pointer, Copy, RowState) ||
                !CheckAllowedKeys(*Copy, {TEXT("name"), TEXT("source"), TEXT("_mod")}, Pointer, RowState) ||
                !ReadRequiredString(*Copy, TEXT("name"), Pointer, ParentName, RowState) ||
                !ReadRequiredString(*Copy, TEXT("source"), Pointer, ParentSource, RowState) ||
                !ValidateIdentityText(ParentName, AppendPointer(Pointer, TEXT("name")), RowState) ||
                !ValidateIdentityText(ParentSource, AppendPointer(Pointer, TEXT("source")), RowState))
            {
                return ELoreChainResult::Invalid;
            }

            const auto ParentKey = MakeLoreJoinKey(Document.Context.DatasetNamespace, ParentName, ParentSource);
            const int32* ParentIndex = Lookup.Find(ParentKey);
            if (!ParentIndex)
            {
                AddDiagnostic(RowState, EDnd5eDiagnosticSeverity::Warning, Codes::UnresolvedReference, Pointer,
                    TEXT("Creature lore parent is absent from the selected lore documents."));
                return ELoreChainResult::Skipped;
            }
            Index = *ParentIndex;
        }
    }

    bool ReadLoreContent(const TSharedPtr<FJsonValue>& Value, const FString& Pointer, bool bAllowSingle, bool bImages,
        FReadState& State, int32& Converted, TArray<FDnd5eContentBlock>& Out)
    {
        const int32 Before = State.Budget->ContentValues;
        if (bAllowSingle && Value.IsValid() && Value->Type != EJson::Array)
        {
            FDnd5eContentBlock Block;
            if (!ReadContent(Value, Pointer, Block, State)) { return false; }
            if (bImages && Block.Kind != Dnd5e::Content::Block::Image &&
                Block.Kind != Dnd5e::Content::Block::Unsupported)
            {
                return Fail(State, Codes::InvalidFieldValue, Pointer,
                    TEXT("Expected an image or a preserved unsupported image payload."));
            }
            Out.Reset();
            Out.Add(MoveTemp(Block));
        }
        else if (!(bImages ? ReadSupplementaryImages(Value, Pointer, Out, State) : ReadBlocks(Value, Pointer, Out, State)))
        {
            return false;
        }

        const int32 Cost = State.Budget->ContentValues - Before;
        if (Cost > 1000000 - Converted)
        {
            return Fail(State, Codes::ResourceLimitExceeded, Pointer, TEXT("Creature lore conversion budget exceeded."));
        }
        Converted += Cost;
        return true;
    }

    bool ApplyLoreOperation(const TSharedPtr<FJsonValue>& Value, const FString& Pointer,
        bool bImages, FReadState& State, int32& Converted, TArray<FDnd5eContentBlock>& Entries)
    {
        TSharedPtr<FJsonObject> Operation;
        FString Mode;
        if (!RequireObject(Value, Pointer, Operation, State) ||
            !ReadRequiredString(*Operation, TEXT("mode"), Pointer, Mode, State))
        {
            return false;
        }

        const bool bReplace = Mode.Equals(TEXT("setProp"), ESearchCase::CaseSensitive);
        const bool bPrepend = Mode.Equals(TEXT("prependArr"), ESearchCase::CaseSensitive);
        const bool bAppend = Mode.Equals(TEXT("appendArr"), ESearchCase::CaseSensitive);
        if (!bReplace && !bPrepend && !bAppend)
        {
            return Fail(State, Codes::UnsupportedRequiredValue, AppendPointer(Pointer, TEXT("mode")),
                TEXT("Unsupported creature lore modifier."));
        }

        const TCHAR* Field = bReplace ? TEXT("value") : TEXT("items");
        if (!CheckAllowedKeys(*Operation, {TEXT("mode"), Field}, Pointer, State)) { return false; }
        const auto Items = FindExactField(*Operation, Field);
        const FString ItemsPointer = AppendPointer(Pointer, Field);
        if (!Items)
        {
            return Fail(State, Codes::MissingRequiredField, ItemsPointer, TEXT("Creature lore modifier requires content."));
        }

        TArray<FDnd5eContentBlock> Blocks;
        if (!ReadLoreContent(Items, ItemsPointer, !bReplace, bImages, State, Converted, Blocks)) { return false; }
        if (bReplace)
        {
            Entries = MoveTemp(Blocks);
        }
        else if (bPrepend)
        {
            Blocks.Append(MoveTemp(Entries));
            Entries = MoveTemp(Blocks);
        }
        else
        {
            Entries.Append(MoveTemp(Blocks));
        }
        return true;
    }

     bool ResolveLoreField(const TArray<FLoreRow>& Rows, const TArray<int32>& Chain,
        const TArray<FDnd5eJsonDocument>& Documents, TArray<FDnd5eDiagnostic>& Diagnostics,
        const TCHAR* Field, bool bImages, int32& Converted, TArray<FDnd5eContentBlock>& Blocks)
    {
        for (int32 Index = Chain.Num() - 1; Index >= 0; --Index)
        {
            const FLoreRow& Row = Rows[Chain[Index]];
            const FDnd5eJsonDocument& Document = Documents[Row.DocumentIndex];
            FReadState RowState{Document.Context, Document.JsonText, Diagnostics, Row.Source};
            if (const auto Direct = FindExactField(*Row.Object, Field))
            {
                if (bImages && Direct->Type == EJson::Null)
                {
                    Blocks.Reset();
                }
                else if (!ReadLoreContent(Direct, AppendPointer(Row.Pointer, Field), false, bImages, RowState, Converted, Blocks))
                {
                    return false;
                }
            }

            const auto CopyValue = FindExactField(*Row.Object, TEXT("_copy"));
            if (!CopyValue) { continue; }
            const auto Copy = CopyValue->AsObject();
            const auto ModsValue = FindExactField(*Copy, TEXT("_mod"));
            if (!ModsValue) { continue; }

            const FString ModsPointer = AppendPointer(AppendPointer(Row.Pointer, TEXT("_copy")), TEXT("_mod"));
            TSharedPtr<FJsonObject> Mods;
            if (!RequireObject(ModsValue, ModsPointer, Mods, RowState) ||
                !CheckAllowedKeys(*Mods, {TEXT("entries"), TEXT("images")}, ModsPointer, RowState))
            {
                return false;
            }

            const auto Operations = FindExactField(*Mods, Field);
            if (!Operations) { continue; }
            const FString Pointer = AppendPointer(ModsPointer, Field);
            if (Operations->Type == EJson::Array)
            {
                const auto& Values = Operations->AsArray();
                for (int32 OpIndex = 0; OpIndex < Values.Num(); ++OpIndex)
                {
                    if (!ApplyLoreOperation(Values[OpIndex], AppendPointer(Pointer, FString::FromInt(OpIndex)),
                        bImages, RowState, Converted, Blocks))
                    {
                        return false;
                    }
                }
            }
            else if (!ApplyLoreOperation(Operations, Pointer, bImages, RowState, Converted, Blocks))
            {
                return false;
            }
        }
        return true;
    }
}

bool Dnd5eJsonCreatureLoreReader::ReadAndAttach(const TArray<FDnd5eJsonDocument>& Documents,
    TArray<FDnd5eCreature>& Creatures, TArray<FDnd5eDiagnostic>& Diagnostics)
{
    if (Documents.IsEmpty()) { return true; }

    TArray<FLoreRow> Rows;
    TMap<FDnd5eRecordId, int32> LoreLookup;
    TArray<FDnd5eRecordId> Keys;
    for (int32 DocumentIndex = 0; DocumentIndex < Documents.Num(); ++DocumentIndex)
    {
        const auto& Document = Documents[DocumentIndex];
        FReadState State{Document.Context, Document.JsonText, Diagnostics, FString{}};
        TSharedPtr<FJsonValue> RootValue;
        TSharedPtr<FJsonObject> Root;
        const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
        if (!ValidateReadContext(State) || !ValidateDocumentText(State) || !PreflightJson(State) ||
            !ParseJsonDocument(RootValue, State) || !RequireObject(RootValue, TEXT(""), Root, State) ||
            !ReadRequiredArray(*Root, TEXT("monsterFluff"), TEXT(""), Values, State))
        {
            return false;
        }
        if (Values->Num() > 100000 - Rows.Num())
        {
            return Fail(State, Codes::ResourceLimitExceeded, TEXT("/monsterFluff"),
                TEXT("Too many creature lore rows across the selected files."));
        }
        if (!ReadLoreRows(*Values, DocumentIndex, State, Rows, LoreLookup, Keys)) { return false; }
    }

    FReadState State{Documents[0].Context, Documents[0].JsonText, Diagnostics, FString{}};
    TMap<FDnd5eRecordId, int32> Primary;
    for (int32 Index = 0; Index < Creatures.Num(); ++Index)
    {
        const FDnd5eRecordHeader& Header = Creatures[Index].Header;
        if (Header.Provenance.IsEmpty()) { continue; }
        const auto Key = MakeLoreJoinKey(Header.RecordId.DatasetNamespace, Header.DisplayName, Header.Provenance[0].SourceId);
        if (Primary.Contains(Key))
        {
            return Fail(State, Codes::InvalidFieldValue, TEXT("/monsterFluff"),
                TEXT("Duplicate primary creature identity before lore joining."));
        }
        Primary.Add(Key, Index);
    }

    TArray<FPendingCreatureLore> Pending;
    int32 Converted = 0;
    for (int32 RowIndex = 0; RowIndex < Rows.Num(); ++RowIndex)
    {
        const int32* CreatureIndex = Primary.Find(Keys[RowIndex]);
        if (!CreatureIndex) { continue; }

        TArray<int32> Chain;
        const ELoreChainResult Result = CollectLoreChain(RowIndex, Rows, LoreLookup, Documents, Diagnostics, Chain);
        if (Result == ELoreChainResult::Invalid) { return false; }
        if (Result == ELoreChainResult::Skipped) { continue; }

        const FLoreRow& Row = Rows[RowIndex];
        FPendingCreatureLore Attachment;
        Attachment.CreatureIndex = *CreatureIndex;
        Attachment.Provenance.SourceId = Row.Source;
        Attachment.Provenance.RelativeFilePath = Documents[Row.DocumentIndex].Context.RelativeFilePath;
        Attachment.Provenance.JsonPointer = Row.Pointer;
        if (!ResolveLoreField(Rows, Chain, Documents, Diagnostics, TEXT("entries"), false, Converted, Attachment.Entries) ||
            !ResolveLoreField(Rows, Chain, Documents, Diagnostics, TEXT("images"), true, Converted, Attachment.Images))
        {
            return false;
        }
        if (!Attachment.Entries.IsEmpty() || !Attachment.Images.IsEmpty())
        {
            Pending.Add(MoveTemp(Attachment));
        }
    }

    for (FPendingCreatureLore& Attachment : Pending)
    {
        FDnd5eCreature& Creature = Creatures[Attachment.CreatureIndex];
        Creature.SupplementaryContent.Entries = MoveTemp(Attachment.Entries);
        Creature.SupplementaryContent.Images = MoveTemp(Attachment.Images);
        Creature.Header.Provenance.Add(MoveTemp(Attachment.Provenance));
    }
    return true;
}
