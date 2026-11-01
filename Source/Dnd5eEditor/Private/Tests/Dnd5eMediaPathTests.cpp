#if WITH_DEV_AUTOMATION_TESTS
#include "Dnd5eTestSupport.h"
IMPLEMENT_COMPLEX_AUTOMATION_TEST(FDnd5eMediaPathsTest, "Dnd5e.Values.MediaRelativePaths", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
void FDnd5eMediaPathsTest::GetTests(TArray<FString>& Names, TArray<FString>& Commands) const
{
	const TArray<FString> Values = {TEXT("art/invented.png"),TEXT("art/α.png"),TEXT(""),TEXT("/root.png"),TEXT("C:/root.png"),TEXT("../escape.png"),TEXT("art/../escape.png"),TEXT("art/./image.png"),TEXT("art//image.png"),TEXT("art\\image.png"),TEXT(" art/image.png"),TEXT("art/image.png "),TEXT("art/*.png"),TEXT("art/image?.png"),TEXT("art/"),TEXT("https://invented.invalid/image.png"),TEXT("art/line\nbreak.png")};
	for (int32 Index = 0; Index < Values.Num(); ++Index) { Names.Add(FString::FromInt(Index)); Commands.Add(FString::FromInt(Index) + TEXT("|") + Values[Index]); }
}
bool FDnd5eMediaPathsTest::RunTest(const FString& Parameters)
{
	FString Index, Path; Parameters.Split(TEXT("|"), &Index, &Path);
	TestEqual(TEXT("Explicit relative resource path grammar"), Dnd5eMedia::IsDatasetRelativePath(Path), FCString::Atoi(*Index) < 2);
	TestFalse(TEXT("Invalid UTF16 rejected"), Dnd5eMedia::IsDatasetRelativePath(FString::ChrN(1, static_cast<TCHAR>(0xD800))));
	return !HasAnyErrors();
}
#endif
