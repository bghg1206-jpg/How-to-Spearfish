#include "Core/SpearfishSettings.h"

#include "Engine/DataTable.h"
#include "Materials/MaterialInterface.h"

USpearfishSettings::USpearfishSettings()
{
	CategoryName = TEXT("Game");
	SectionName = TEXT("How to Spearfish");

	// Materials produced by Tools/Editor/bootstrap_content.py. Missing assets fall back gracefully.
	BaseMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Materials/M_Spearfish_Base.M_Spearfish_Base")));
	FallbackBaseMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")));
	WaterSurfaceMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Materials/M_Spearfish_OceanSurface.M_Spearfish_OceanSurface")));
	WaterUndersideMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Materials/M_Spearfish_OceanUnderside.M_Spearfish_OceanUnderside")));
	UnderwaterPostProcessMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Materials/M_Spearfish_UnderwaterPP.M_Spearfish_UnderwaterPP")));
}
