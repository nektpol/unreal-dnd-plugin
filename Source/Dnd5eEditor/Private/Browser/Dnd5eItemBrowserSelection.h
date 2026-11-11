// Developed by Neko Creative Technologies

#pragma once

#include "Items/Dnd5eItemGroup.h"
#include "UObject/Object.h"
#include "Dnd5eItemBrowserSelection.generated.h"

UCLASS(Transient)
class UDnd5eItemBrowserSelection : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, Transient, Category = "Dnd5e|Item Browser")
	FDnd5eItem Item;
};

UCLASS(Transient)
class UDnd5eItemGroupBrowserSelection : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, Transient, Category = "Dnd5e|Item Browser")
	FDnd5eItemGroup Group;
};