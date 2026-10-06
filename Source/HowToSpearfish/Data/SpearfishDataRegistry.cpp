#include "Data/SpearfishDataRegistry.h"

#include "Core/SpearfishSettings.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Engine/DataTable.h"
#include "Engine/GameInstance.h"
#include "HAL/IConsoleManager.h"
#include "HowToSpearfish.h"
#include "JsonObjectConverter.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace SpearfishDataRegistryPrivate
{
	template <typename TRow>
	void LoadRows(const TSoftObjectPtr<UDataTable>& TableRef, const FString& JsonFile, TMap<FName, TRow>& OutRows, int32& Problems)
	{
		OutRows.Reset();

		if (!TableRef.IsNull())
		{
			if (UDataTable* Table = TableRef.LoadSynchronous())
			{
				Table->ForeachRow<TRow>(TEXT("SpearfishDataRegistry"), [&OutRows](const FName& Key, const TRow& Row)
				{
					TRow Copy = Row;
					Copy.Id = Key;
					OutRows.Add(Key, Copy);
				});
				UE_LOG(LogSpearfish, Log, TEXT("Data: %d rows from DataTable %s"), OutRows.Num(), *Table->GetName());
				return;
			}
			UE_LOG(LogSpearfish, Warning, TEXT("Data: DataTable %s could not be loaded, falling back to JSON"), *TableRef.ToString());
		}

		const FString Directory = FPaths::ProjectContentDir() / USpearfishSettings::Get()->JsonDataDirectory;
		const FString Path = Directory / JsonFile;
		FString Json;
		if (!FFileHelper::LoadFileToString(Json, *Path))
		{
			UE_LOG(LogSpearfish, Error, TEXT("Data: missing %s"), *Path);
			++Problems;
			return;
		}

		TArray<TSharedPtr<FJsonValue>> Values;
		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
		if (!FJsonSerializer::Deserialize(Reader, Values))
		{
			UE_LOG(LogSpearfish, Error, TEXT("Data: %s is not a JSON array"), *Path);
			++Problems;
			return;
		}

		for (const TSharedPtr<FJsonValue>& Value : Values)
		{
			const TSharedPtr<FJsonObject>* Object = nullptr;
			if (!Value.IsValid() || !Value->TryGetObject(Object) || !Object || !Object->IsValid())
			{
				++Problems;
				continue;
			}

			FString Name;
			if (!(*Object)->TryGetStringField(TEXT("Name"), Name) || Name.IsEmpty())
			{
				UE_LOG(LogSpearfish, Error, TEXT("Data: row without Name in %s"), *JsonFile);
				++Problems;
				continue;
			}

			TRow Row;
			if (!FJsonObjectConverter::JsonObjectToUStruct((*Object).ToSharedRef(), &Row))
			{
				UE_LOG(LogSpearfish, Error, TEXT("Data: failed to convert %s in %s"), *Name, *JsonFile);
				++Problems;
				continue;
			}
			Row.Id = FName(*Name);
			OutRows.Add(Row.Id, Row);
		}
		UE_LOG(LogSpearfish, Log, TEXT("Data: %d rows from %s"), OutRows.Num(), *JsonFile);
	}

	template <typename TRow>
	FText DisplayNameIn(const TMap<FName, TRow>& Rows, FName Id, bool& bFound)
	{
		if (const TRow* Row = Rows.Find(Id))
		{
			bFound = true;
			return Row->DisplayName;
		}
		return FText::GetEmpty();
	}
}

static FAutoConsoleCommandWithWorld GSpearfishReloadDataCommand(
	TEXT("Spearfish.ReloadData"),
	TEXT("Reloads Spearfish content definitions (JSON / DataTables)."),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
	{
		if (USpearfishDataRegistry* Registry = USpearfishDataRegistry::Get(World))
		{
			Registry->Reload();
		}
	}));

void USpearfishDataRegistry::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Reload();
}

USpearfishDataRegistry* USpearfishDataRegistry::Get(const UObject* WorldContextObject)
{
	const UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(WorldContextObject);
	return GameInstance ? GameInstance->GetSubsystem<USpearfishDataRegistry>() : nullptr;
}

void USpearfishDataRegistry::Reload()
{
	using namespace SpearfishDataRegistryPrivate;
	const USpearfishSettings* Settings = USpearfishSettings::Get();
	ProblemCount = 0;

	LoadRows(Settings->FishTable, TEXT("Fish.json"), Fish, ProblemCount);
	LoadRows(Settings->EquipmentTable, TEXT("Equipment.json"), Equipment, ProblemCount);
	LoadRows(Settings->RecipeTable, TEXT("Recipes.json"), Recipes, ProblemCount);
	LoadRows(Settings->CustomerTable, TEXT("Customers.json"), Customers, ProblemCount);
	LoadRows(Settings->RegionTable, TEXT("Regions.json"), Regions, ProblemCount);
	LoadRows(Settings->LootTable, TEXT("Loot.json"), Loot, ProblemCount);
	LoadRows(Settings->UpgradeTable, TEXT("Upgrades.json"), Upgrades, ProblemCount);
	LoadRows(Settings->EventTable, TEXT("Events.json"), Events, ProblemCount);

	Validate();
	UE_LOG(LogSpearfish, Log, TEXT("Data registry ready: %d fish, %d equipment, %d recipes, %d regions (%d problems)"),
		Fish.Num(), Equipment.Num(), Recipes.Num(), Regions.Num(), ProblemCount);
}

void USpearfishDataRegistry::Validate()
{
	auto Report = [this](const FString& Message)
	{
		++ProblemCount;
		UE_LOG(LogSpearfish, Warning, TEXT("Data: %s"), *Message);
	};

	for (const TPair<FName, FSpearfishRecipeDef>& Pair : Recipes)
	{
		for (const FSpearfishIngredientReq& Req : Pair.Value.Ingredients)
		{
			if (!Req.SpeciesId.IsNone() && !Fish.Contains(Req.SpeciesId))
			{
				Report(FString::Printf(TEXT("recipe %s uses unknown species %s"), *Pair.Key.ToString(), *Req.SpeciesId.ToString()));
			}
		}
	}
	for (const TPair<FName, FSpearfishRegionDef>& Pair : Regions)
	{
		for (const FSpearfishRegionSpawn& Spawn : Pair.Value.Spawns)
		{
			if (!Fish.Contains(Spawn.SpeciesId))
			{
				Report(FString::Printf(TEXT("region %s spawns unknown species %s"), *Pair.Key.ToString(), *Spawn.SpeciesId.ToString()));
			}
		}
		for (const FSpearfishWeightedId& Entry : Pair.Value.Loot)
		{
			if (!Loot.Contains(Entry.Id))
			{
				Report(FString::Printf(TEXT("region %s uses unknown loot %s"), *Pair.Key.ToString(), *Entry.Id.ToString()));
			}
		}
		for (const FName& EventId : Pair.Value.Events)
		{
			if (!Events.Contains(EventId))
			{
				Report(FString::Printf(TEXT("region %s uses unknown event %s"), *Pair.Key.ToString(), *EventId.ToString()));
			}
		}
	}
	for (uint8 SlotIndex = 0; SlotIndex < static_cast<uint8>(ESpearfishEquipmentSlot::Count); ++SlotIndex)
	{
		if (!GetStarterEquipment(static_cast<ESpearfishEquipmentSlot>(SlotIndex)))
		{
			Report(FString::Printf(TEXT("no starter equipment for slot %d"), SlotIndex));
		}
	}
}

TArray<const FSpearfishEquipmentDef*> USpearfishDataRegistry::GetEquipmentForSlot(ESpearfishEquipmentSlot Slot) const
{
	TArray<const FSpearfishEquipmentDef*> Result;
	for (const TPair<FName, FSpearfishEquipmentDef>& Pair : Equipment)
	{
		if (Pair.Value.Slot == Slot)
		{
			Result.Add(&Pair.Value);
		}
	}
	Result.Sort([](const FSpearfishEquipmentDef& A, const FSpearfishEquipmentDef& B) { return A.Tier < B.Tier; });
	return Result;
}

const FSpearfishEquipmentDef* USpearfishDataRegistry::GetStarterEquipment(ESpearfishEquipmentSlot Slot) const
{
	for (const TPair<FName, FSpearfishEquipmentDef>& Pair : Equipment)
	{
		if (Pair.Value.Slot == Slot && Pair.Value.bStarter)
		{
			return &Pair.Value;
		}
	}
	return nullptr;
}

FText USpearfishDataRegistry::GetDisplayName(FName Id) const
{
	using namespace SpearfishDataRegistryPrivate;
	bool bFound = false;
	FText Name = DisplayNameIn(Fish, Id, bFound);
	if (!bFound) { Name = DisplayNameIn(Loot, Id, bFound); }
	if (!bFound) { Name = DisplayNameIn(Recipes, Id, bFound); }
	if (!bFound) { Name = DisplayNameIn(Equipment, Id, bFound); }
	if (!bFound) { Name = DisplayNameIn(Upgrades, Id, bFound); }
	if (!bFound) { Name = DisplayNameIn(Regions, Id, bFound); }
	if (!bFound) { Name = DisplayNameIn(Customers, Id, bFound); }
	return bFound ? Name : FText::FromName(Id);
}

TArray<FName> USpearfishDataRegistry::GetSortedFishIds() const
{
	TArray<FName> Ids;
	Fish.GetKeys(Ids);
	Ids.Sort([](const FName& A, const FName& B) { return A.LexicalLess(B); });
	return Ids;
}

const FSpearfishDepthZoneDef* USpearfishDataRegistry::FindDepthZone(FName RegionId, float DepthM) const
{
	const FSpearfishRegionDef* Region = FindRegion(RegionId);
	if (!Region || Region->DepthZones.Num() == 0)
	{
		return nullptr;
	}
	const FSpearfishDepthZoneDef* Shallowest = &Region->DepthZones[0];
	const FSpearfishDepthZoneDef* Deepest = &Region->DepthZones[0];
	for (const FSpearfishDepthZoneDef& Zone : Region->DepthZones)
	{
		if (DepthM >= Zone.MinDepthM && DepthM < Zone.MaxDepthM)
		{
			return &Zone;
		}
		Shallowest = Zone.MinDepthM < Shallowest->MinDepthM ? &Zone : Shallowest;
		Deepest = Zone.MaxDepthM > Deepest->MaxDepthM ? &Zone : Deepest;
	}
	return DepthM < Shallowest->MinDepthM ? Shallowest : Deepest;
}
