#pragma once

#include "CoreMinimal.h"

DECLARE_LOG_CATEGORY_EXTERN(LogSpearfish, Log, All);

/** Custom collision channels, declared in Config/DefaultEngine.ini. */
#define ECC_Harpoon ECC_GameTraceChannel1
#define ECC_Interact ECC_GameTraceChannel2
