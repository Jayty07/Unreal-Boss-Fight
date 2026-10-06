#include "Game/BossArenaGameInstance.h"

#include "AbilitySystemGlobals.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BossArenaGameInstance)

void UBossArenaGameInstance::Init()
{
	Super::Init();

	UAbilitySystemGlobals& Globals = UAbilitySystemGlobals::Get();
	if (!Globals.IsAbilitySystemGlobalsInitialized())
	{
		Globals.InitGlobalData();
	}
}
