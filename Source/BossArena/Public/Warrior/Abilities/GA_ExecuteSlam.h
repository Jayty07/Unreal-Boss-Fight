#pragma once

#include "CoreMinimal.h"
#include "Warrior/Abilities/WarriorGameplayAbility.h"
#include "GA_ExecuteSlam.generated.h"

/** Execute slam: heavy circle AoE finisher, only usable while the boss is below ExecuteThreshold. */
UCLASS()
class BOSSARENA_API UGA_ExecuteSlam : public UWarriorGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_ExecuteSlam();

	virtual bool IsUsableNow(const AActor* Avatar) const override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Execute", meta = (ClampMin = "0", ClampMax = "1"))
	float ExecuteThreshold = 0.2f;
};
