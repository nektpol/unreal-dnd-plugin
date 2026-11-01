// Developed by Neko Creative Technologies

#include "Readers/Dnd5eJsonReaderInternal.h"
#include "Dnd5eTags.h"
#include "Serialization/JsonSerializer.h"

namespace Dnd5eJsonReader
{
	bool ReadTypedSpellMetadata(const FJsonObject& Object, const FString& Pointer, FDnd5eSpell& Spell, FReadState& State)
	{
		struct FMembershipField { const TCHAR* Key; const TCHAR* Id; bool AllowsName; };
		const FMembershipField Fields[] = {
			{TEXT("srd"), TEXT("5etools.srd"), true}, {TEXT("srd52"), TEXT("5etools.srd52"), true},
			{TEXT("basicRules"), TEXT("5etools.basicRules"), false}, {TEXT("basicRules2024"), TEXT("5etools.basicRules2024"), false}
		};
		auto& Memberships = Spell.Header.Provenance[0].Memberships; Memberships.Reset();
		for (const auto& Field : Fields)
		{
			const auto Value = FindExactField(Object, Field.Key); if (!Value) { continue; }
			FDnd5eSourceMembership Membership; Membership.MembershipId = Field.Id;
			if (Value->Type == EJson::Boolean) { Membership.Inclusion = Value->AsBool() ? EDnd5eSourceInclusion::Included : EDnd5eSourceInclusion::Excluded; }
			else if (Field.AllowsName && Value->Type == EJson::String && IsValidIdentityText(Value->AsString()))
			{
				Membership.Inclusion = EDnd5eSourceInclusion::IncludedWithAlternateName; Membership.AlternateName = Value->AsString();
			}
			else { return Fail(State, Codes::InvalidFieldValue, AppendPointer(Pointer, Field.Key), TEXT("Source membership needs a Boolean or a supported nonblank alternate name.")); }
			Memberships.Add(MoveTemp(Membership));
		}
		auto Hint = [&](const TCHAR* Key, EDnd5eDataAvailability& Out)
		{
			Out = EDnd5eDataAvailability::Unknown; const auto Value = FindExactField(Object, Key); if (!Value) { return true; }
			if (Value->Type != EJson::Boolean) { return Fail(State, Codes::InvalidFieldValue, AppendPointer(Pointer, Key), TEXT("Source availability hint must be Boolean.")); }
			Out = Value->AsBool() ? EDnd5eDataAvailability::Present : EDnd5eDataAvailability::Absent; return true;
		};
		return Hint(TEXT("hasFluff"), Spell.SupplementaryTextHint) && Hint(TEXT("hasFluffImages"), Spell.SupplementaryImagesHint);
	}

	bool ValidateEditionAssignments(const TArray<FDnd5eSourceEditionAssignment>& Assignments, const FString& PolicyId, FReadState& State)
	{
		if (Assignments.IsEmpty()) { return true; }
		if (Assignments.Num() > 1024 || !IsValidIdentityText(PolicyId)) { return Fail(State, Codes::InvalidFieldValue, TEXT(""), TEXT("Supply a named edition policy with at most 1024 assignments.")); }
		for (int32 Index = 0; Index < Assignments.Num(); ++Index)
		{
			const auto& Assignment = Assignments[Index];
			if (!IsValidIdentityText(Assignment.DatasetNamespace) || !IsValidIdentityText(Assignment.SourceId) || Assignment.EditionTags.IsEmpty())
			{
				return Fail(State, Codes::InvalidFieldValue, TEXT(""), TEXT("Edition assignments need exact namespace/source identity and supported edition tags."));
			}
			for (const auto& Tag : Assignment.EditionTags)
			{
				if (Tag != Dnd5e::Rules::Edition::Original2014 && Tag != Dnd5e::Rules::Edition::Revised2024)
				{
					return Fail(State, Codes::UnsupportedRequiredValue, TEXT(""), TEXT("Edition assignment contains an unsupported tag."));
				}
			}
			for (int32 Earlier = 0; Earlier < Index; ++Earlier)
			{
				if (Assignments[Earlier].DatasetNamespace.Equals(Assignment.DatasetNamespace, ESearchCase::CaseSensitive) && Assignments[Earlier].SourceId.Equals(Assignment.SourceId, ESearchCase::CaseSensitive))
				{
					return Fail(State, Codes::InvalidFieldValue, TEXT(""), TEXT("Duplicate namespace/source edition assignment."));
				}
			}
		}
		return true;
	}

	bool ApplyEditionAssignment(const TArray<FDnd5eSourceEditionAssignment>& Assignments, const FString& PolicyId, FDnd5eSpell& Spell, FReadState& State)
	{
		for (const auto& Assignment : Assignments)
		{
			if (!Assignment.DatasetNamespace.Equals(Spell.Header.RecordId.DatasetNamespace, ESearchCase::CaseSensitive) ||
				!Assignment.SourceId.Equals(Spell.Header.Provenance[0].SourceId, ESearchCase::CaseSensitive)) { continue; }
			TSharedPtr<FJsonObject> Evidence; auto& Json = Spell.Header.Provenance[0].ClassificationEvidenceJson;
			if (!Json.IsEmpty())
			{
				if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Evidence, FJsonSerializer::EFlags::StoreNumbersAsStrings) || !Evidence.IsValid())
				{
					return Fail(State, Codes::SerializationFailure, TEXT(""), TEXT("Could not retain edition classification evidence."));
				}
			}
			else { Evidence = MakeShared<FJsonObject>(); }
			TArray<TSharedPtr<FJsonValue>> Tags;
			for (const auto& Tag : Assignment.EditionTags) { Spell.Header.Classification.AddTag(Tag); Tags.Add(MakeShared<FJsonValueString>(Tag.ToString())); }
			Evidence->SetStringField(TEXT("editionPolicyId"), PolicyId); Evidence->SetArrayField(TEXT("editionTags"), Tags);
			return SerializeValue(MakeShared<FJsonValueObject>(Evidence), Json, TEXT(""), State);
		}
		return true;
	}
}
