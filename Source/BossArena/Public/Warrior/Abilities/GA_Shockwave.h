#pragma once

#include "CoreMinimal.h"
#include "Warrior/Abilities/WarriorGameplayAbility.h"
#include "GA_Shockwave.generated.h"

/** Shockwave: line/column AoE projected forward from the sword strike. Interrupts casts it hits. */
UCLASS()
class BOSSARENA_API UGA_Shockwave : public UWarriorGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Shockwave();
};
