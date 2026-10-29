#include "Readers/Dnd5eJsonSpellAvailabilityReader.h"
#include "Readers/Dnd5eJsonReaderInternal.h"
#include "Dnd5eTags.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace Dnd5eJsonReader
{
	struct FLookupPair
	{
		FString Key;
		TSharedPtr<FJsonValue> Value;
	};

	TArray<FLookupPair> SortedLookupPairs(const FJsonObject& Object)
	{
		TArray<FLookupPair> Pairs;
		for (const auto& Pair : Object.Values) { Pairs.Add({FString(Pair.Key.Len(), *Pair.Key), Pair.Value}); }
		Pairs.Sort([](const auto& A, const auto& B) { return A.Key.Compare(B.Key, ESearchCase::CaseSensitive) < 0; });
		return Pairs;
	}

	TSharedPtr<FJsonValue> FindLookupField(const FJsonObject& Object, const FString& Name, FString& ActualKey)
	{
		for (const auto& Pair : Object.Values)
		{
			const FString Key(Pair.Key.Len(), *Pair.Key);
			if (Key.Equals(Name, ESearchCase::IgnoreCase)) { ActualKey = Key; return Pair.Value; }
		}
		return nullptr;
	}

	bool AvailabilityBudget(FReadState& State, const FString& Pointer)
	{
		return ++State.Budget->ContentValues <= 100000 || Fail(State, Codes::ResourceLimitExceeded, Pointer, TEXT("Availability exceeds the 100000 converted-value budget."));
	}

	bool ReadAvailabilityStrings(const TSharedPtr<FJsonValue>& Value, const FString& Pointer, TArray<FString>& Strings, bool bAllowNull, bool& bHadNull, FReadState& State)
	{
		const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
		if (!RequireArray(Value, Pointer, Values, State)) { return false; }
		for (int32 Index = 0; Index < Values->Num(); ++Index)
		{
			const FString EntryPointer = AppendPointer(Pointer, FString::FromInt(Index));
			const auto& Entry = (*Values)[Index];
			if (!AvailabilityBudget(State, EntryPointer)) { return false; }
			if (bAllowNull && Entry->Type == EJson::Null) { bHadNull = true; continue; }
			FString Text;
			if (Entry->Type != EJson::String || !Entry->TryGetString(Text)) { return Fail(State, Codes::InvalidFieldValue, EntryPointer, TEXT("Expected a nonempty selector/qualification string.")); }
			if (!ValidateIdentityText(Text, EntryPointer, State)) { return false; }
			Strings.Add(MoveTemp(Text));
		}
		return true;
	}

	bool ReadProviderLeaf(const TSharedPtr<FJsonValue>& Value, const FString& Pointer, const FString& Branch, FDnd5eSpellAvailability& Entry, FReadState& State)
	{
		const bool bClass = Branch.Equals(TEXT("class"), ESearchCase::CaseSensitive) || Branch.Equals(TEXT("classVariant"), ESearchCase::CaseSensitive);
		const bool bSubclass = Branch.Equals(TEXT("subclass"), ESearchCase::CaseSensitive);
		Entry.bIsVariant = Branch.Equals(TEXT("classVariant"), ESearchCase::CaseSensitive);
		if (Value->Type == EJson::Boolean)
		{
			bool bEnabled = false;
			Value->TryGetBool(bEnabled);
			if (!bEnabled || bSubclass) { return Fail(State, Codes::InvalidFieldValue, Pointer, TEXT("A provider leaf must declare a supported association; subclass leaves need their full name.")); }
			Entry.bHasUnqualifiedDefinition = true;
			return true;
		}
		TSharedPtr<FJsonObject> Metadata;
		if (!RequireObject(Value, Pointer, Metadata, State)) { return false; }
		if (bClass)
		{
			if (!CheckAllowedKeys(*Metadata, {TEXT("definedInSource"), TEXT("definedInSources")}, Pointer, State)) { return false; }
			const auto Single = FindExactField(*Metadata, TEXT("definedInSource"));
			const auto Multiple = FindExactField(*Metadata, TEXT("definedInSources"));
			if (Single.IsValid() == Multiple.IsValid()) { return Fail(State, Codes::InvalidFieldValue, Pointer, TEXT("A class metadata leaf needs exactly one definition-source form.")); }
			if (Single.IsValid())
			{
				FString Source;
				if (!ReadRequiredString(*Metadata, TEXT("definedInSource"), Pointer, Source, State) || !ValidateIdentityText(Source, AppendPointer(Pointer, TEXT("definedInSource")), State)) { return false; }
				Entry.DefinedInSources.Add(MoveTemp(Source));
			}
			else if (!ReadAvailabilityStrings(Multiple, AppendPointer(Pointer, TEXT("definedInSources")), Entry.DefinedInSources, true, Entry.bHasUnqualifiedDefinition, State)) { return false; }
			if (!Entry.bHasUnqualifiedDefinition && Entry.DefinedInSources.IsEmpty()) { return Fail(State, Codes::InvalidFieldValue, Pointer, TEXT("A class association needs a definition source or an unqualified declaration.")); }
		}
		else if (bSubclass)
		{
			if (!CheckAllowedKeys(*Metadata, {TEXT("name"), TEXT("subSubclasses")}, Pointer, State)) { return false; }
			if (!ReadRequiredString(*Metadata, TEXT("name"), Pointer, Entry.Provider.Name, State) || !ValidateIdentityText(Entry.Provider.Name, AppendPointer(Pointer, TEXT("name")), State)) { return false; }
			if (const auto Subgroups = FindExactField(*Metadata, TEXT("subSubclasses")))
			{
				bool bUnused = false;
				if (!ReadAvailabilityStrings(Subgroups, AppendPointer(Pointer, TEXT("subSubclasses")), Entry.Subgroups, false, bUnused, State)) { return false; }
			}
			Entry.bHasUnqualifiedDefinition = true;
		}
		else if (Branch.Equals(TEXT("race"), ESearchCase::CaseSensitive))
		{
			if (!CheckAllowedKeys(*Metadata, {TEXT("baseName"), TEXT("baseSource")}, Pointer, State)) { return false; }
			if (!ReadRequiredString(*Metadata, TEXT("baseName"), Pointer, Entry.Provider.BaseName, State) || !ValidateIdentityText(Entry.Provider.BaseName, AppendPointer(Pointer, TEXT("baseName")), State)) { return false; }
			if (!ReadRequiredString(*Metadata, TEXT("baseSource"), Pointer, Entry.Provider.BaseSourceId, State) || !ValidateIdentityText(Entry.Provider.BaseSourceId, AppendPointer(Pointer, TEXT("baseSource")), State)) { return false; }
			Entry.bHasUnqualifiedDefinition = true;
		}
		else if (Branch.Equals(TEXT("optionalfeature"), ESearchCase::CaseSensitive))
		{
			if (!CheckAllowedKeys(*Metadata, {TEXT("featureType")}, Pointer, State)) { return false; }
			TSharedPtr<FJsonValue> FeatureTypes;
			bool bUnused = false;
			if (!ReadRequiredValue(*Metadata, TEXT("featureType"), EJson::Array, Pointer, FeatureTypes, State)) { return false; }
			if (!ReadAvailabilityStrings(FeatureTypes, AppendPointer(Pointer, TEXT("featureType")), Entry.FeatureTypes, false, bUnused, State)) { return false; }
			Entry.bHasUnqualifiedDefinition = true;
		}
		else { return Fail(State, Codes::UnsupportedRequiredValue, Pointer, TEXT("Metadata for this provider branch has no supported mapping.")); }
		return true;
	}

	bool AddProviderLeaf(const TSharedPtr<FJsonValue>& Value, const FString& Pointer, const FString& Branch, const FString& SpellNameKey, const FString& SpellSourceKey, FDnd5eSpellAvailabilityProvider Provider, TArray<FDnd5eSpellAvailability>& Entries, FReadState& State)
	{
		if (!AvailabilityBudget(State, Pointer) || !ValidateIdentityText(Provider.Name, Pointer, State) || !ValidateIdentityText(Provider.SourceId, Pointer, State)) { return false; }
		FDnd5eSpellAvailability Entry;
		Entry.Provider = MoveTemp(Provider);
		Entry.Provenance.RelativeFilePath = State.Context.RelativeFilePath;
		Entry.Provenance.JsonPointer = Pointer;
		// A generated lookup has no single publication to assert as Provenance.SourceId.
		TSharedRef<FJsonObject> Identity = MakeShared<FJsonObject>();
		Identity->SetStringField(TEXT("spellName"), SpellNameKey);
		Identity->SetStringField(TEXT("spellSource"), SpellSourceKey);
		Identity->SetStringField(TEXT("branch"), Branch);
		Identity->SetStringField(TEXT("providerName"), Entry.Provider.Name);
		Identity->SetStringField(TEXT("providerSource"), Entry.Provider.SourceId);
		if (!Entry.Provider.ClassName.IsEmpty())
		{
			Identity->SetStringField(TEXT("className"), Entry.Provider.ClassName);
			Identity->SetStringField(TEXT("classSource"), Entry.Provider.ClassSourceId);
		}
		if (!FJsonSerializer::Serialize(Identity, TJsonWriterFactory<>::Create(&Entry.Provenance.IdentityFieldsJson))) { return Fail(State, Codes::SerializationFailure, Pointer, TEXT("Could not retain lookup identity evidence.")); }
		if (!ReadProviderLeaf(Value, Pointer, Branch, Entry, State)) { return false; }
		Entries.Add(MoveTemp(Entry));
		return true;
	}

	bool ReadProviderMap(const FJsonObject& Object, const FString& Pointer, const FString& Branch, FGameplayTag Category, const FString& SpellNameKey, const FString& SpellSourceKey, TArray<FDnd5eSpellAvailability>& Entries, FReadState& State)
	{
		for (const auto& Source : SortedLookupPairs(Object))
		{
			const FString SourcePointer = AppendPointer(Pointer, Source.Key);
			if (!ValidateIdentityText(Source.Key, SourcePointer, State)) { return false; }
			TSharedPtr<FJsonObject> Names;
			if (!RequireObject(Source.Value, SourcePointer, Names, State)) { return false; }
			for (const auto& Name : SortedLookupPairs(*Names))
			{
				FDnd5eSpellAvailabilityProvider Provider;
				Provider.Category = Category;
				Provider.Name = Name.Key;
				Provider.SourceId = Source.Key;
				if (!AddProviderLeaf(Name.Value, AppendPointer(SourcePointer, Name.Key), Branch, SpellNameKey, SpellSourceKey, MoveTemp(Provider), Entries, State)) { return false; }
			}
		}
		return true;
	}

	bool ReadSubclassMap(const FJsonObject& Object, const FString& Pointer, const FString& SpellNameKey, const FString& SpellSourceKey, TArray<FDnd5eSpellAvailability>& Entries, FReadState& State)
	{
		for (const auto& ClassSource : SortedLookupPairs(Object))
		{
			const FString ClassSourcePointer = AppendPointer(Pointer, ClassSource.Key);
			TSharedPtr<FJsonObject> Classes;
			if (!ValidateIdentityText(ClassSource.Key, ClassSourcePointer, State) || !RequireObject(ClassSource.Value, ClassSourcePointer, Classes, State)) { return false; }
			for (const auto& Class : SortedLookupPairs(*Classes))
			{
				const FString ClassPointer = AppendPointer(ClassSourcePointer, Class.Key);
				TSharedPtr<FJsonObject> Sources;
				if (!ValidateIdentityText(Class.Key, ClassPointer, State) || !RequireObject(Class.Value, ClassPointer, Sources, State)) { return false; }
				for (const auto& Source : SortedLookupPairs(*Sources))
				{
					const FString SourcePointer = AppendPointer(ClassPointer, Source.Key);
					TSharedPtr<FJsonObject> Names;
					if (!ValidateIdentityText(Source.Key, SourcePointer, State) || !RequireObject(Source.Value, SourcePointer, Names, State)) { return false; }
					for (const auto& Name : SortedLookupPairs(*Names))
					{
						FDnd5eSpellAvailabilityProvider Provider;
						Provider.Category = Dnd5e::Category::Subclass;
						Provider.Name = Name.Key;
						Provider.ShortName = Name.Key;
						Provider.SourceId = Source.Key;
						Provider.ClassName = Class.Key;
						Provider.ClassSourceId = ClassSource.Key;
						if (!AddProviderLeaf(Name.Value, AppendPointer(SourcePointer, Name.Key), TEXT("subclass"), SpellNameKey, SpellSourceKey, MoveTemp(Provider), Entries, State)) { return false; }
					}
				}
			}
		}
		return true;
	}

	bool ReadAvailabilityEntry(const FJsonObject& Root, const FString& SpellName, const FString& SpellSourceId, bool& bHasEntry, TArray<FDnd5eSpellAvailability>& Availability, FReadState& State)
	{
		FString SourceKey, NameKey;
		const auto SourceValue = FindLookupField(Root, SpellSourceId, SourceKey);
		auto Missing = [&]()
		{
			bHasEntry = false;
			Availability.Reset();
			AddDiagnostic(State, EDnd5eDiagnosticSeverity::Info, TEXT("Dnd5e.Json.AvailabilityEntryNotFound"), SourceKey.IsEmpty() ? FString{} : AppendPointer(TEXT(""), SourceKey), TEXT("The supplied lookup has no entry for this selected spell; availability remains unknown."));
			return true;
		};
		if (!SourceValue.IsValid()) { return Missing(); }
		TSharedPtr<FJsonObject> Source;
		const FString SourcePointer = AppendPointer(TEXT(""), SourceKey);
		if (!RequireObject(SourceValue, SourcePointer, Source, State)) { return false; }
		const auto EntryValue = FindLookupField(*Source, SpellName, NameKey);
		if (!EntryValue.IsValid()) { return Missing(); }
		const FString Pointer = AppendPointer(SourcePointer, NameKey);
		TSharedPtr<FJsonObject> Entry;
		if (!RequireObject(EntryValue, Pointer, Entry, State)) { return false; }
		if (!CheckAllowedKeys(*Entry, {TEXT("class"), TEXT("classVariant"), TEXT("subclass"), TEXT("race"), TEXT("optionalfeature"), TEXT("background"), TEXT("feat"), TEXT("charoption"), TEXT("reward")}, Pointer, State)) { return false; }
		struct FBranch { const TCHAR* Name; FGameplayTag Category; };
		const FBranch Branches[] = {
			{TEXT("class"), Dnd5e::Category::Class}, {TEXT("classVariant"), Dnd5e::Category::Class}, {TEXT("subclass"), Dnd5e::Category::Subclass},
			{TEXT("race"), Dnd5e::Category::Species}, {TEXT("optionalfeature"), Dnd5e::Category::OptionalFeature}, {TEXT("background"), Dnd5e::Category::Background},
			{TEXT("feat"), Dnd5e::Category::Feat}, {TEXT("charoption"), Dnd5e::Category::CharacterOption}, {TEXT("reward"), Dnd5e::Category::Reward}
		};
		TArray<FDnd5eSpellAvailability> Candidate;
		for (const auto& Branch : Branches)
		{
			const auto Value = FindExactField(*Entry, Branch.Name);
			if (!Value.IsValid()) { continue; }
			const FString BranchPointer = AppendPointer(Pointer, Branch.Name);
			TSharedPtr<FJsonObject> Map;
			if (!RequireObject(Value, BranchPointer, Map, State)) { return false; }
			if (Branch.Category == Dnd5e::Category::Subclass)
			{
				if (!ReadSubclassMap(*Map, BranchPointer, NameKey, SourceKey, Candidate, State)) { return false; }
			}
			else if (!ReadProviderMap(*Map, BranchPointer, Branch.Name, Branch.Category, NameKey, SourceKey, Candidate, State)) { return false; }
		}
		bHasEntry = true;
		Availability = MoveTemp(Candidate);
		return true;
	}
}

FDnd5eJsonSpellAvailabilityReadResult FDnd5eJsonSpellAvailabilityReader::ReadAvailability(const FDnd5eJsonSpellAvailabilityReadRequest& Request)
{
	using namespace Dnd5eJsonReader;
	FDnd5eJsonSpellAvailabilityReadResult Result;
	Result.Status = EDnd5eJsonReadStatus::Failure;
	FReadState State{Request.Document.Context, Request.Document.JsonText, Result.Diagnostics, FString{}};
	if (!ValidateReadContext(State) || !ValidateIdentityText(Request.SpellName, TEXT(""), State) || !ValidateIdentityText(Request.SpellSourceId, TEXT(""), State) || !ValidateDocumentText(State) || !PreflightJson(State)) { return Result; }
	TSharedPtr<FJsonValue> Value;
	TSharedPtr<FJsonObject> Root;
	if (!ParseJsonDocument(Value, State) || !RequireObject(Value, TEXT(""), Root, State)) { return Result; }
	bool bHasEntry = false;
	TArray<FDnd5eSpellAvailability> Availability;
	if (!ReadAvailabilityEntry(*Root, Request.SpellName, Request.SpellSourceId, bHasEntry, Availability, State)) { return Result; }
	Result.bHasEntry = bHasEntry;
	Result.Availability = MoveTemp(Availability);
	Result.Status = EDnd5eJsonReadStatus::Success;
	return Result;
}
