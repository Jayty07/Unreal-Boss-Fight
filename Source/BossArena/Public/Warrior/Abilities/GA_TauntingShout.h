#pragma once

#include "CoreMinimal.h"
#include "Warrior/Abilities/WarriorGameplayAbility.h"
#include "GA_TauntingShout.generated.h"

/** Taunting shout: radial AoE that adds burst threat (and a short fixate) on every enemy hit. */
UCLASS()
class BOSSARENA_API UGA_TauntingShout : public UWarriorGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_TauntingShout();

protected:
	virtual void OnTargetsHit(const TArray<ABossArenaCharacterBase*>& Targets) override;

	UPROPERTY(EditDefaultsOnly, Category = "Taunt")
	float BurstThreat = 2000.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Taunt")
	float FixateDuration = 4.0f;
};
