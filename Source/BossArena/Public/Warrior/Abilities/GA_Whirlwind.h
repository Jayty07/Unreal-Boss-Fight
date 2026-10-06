#pragma once

#include "CoreMinimal.h"
#include "Warrior/Abilities/WarriorGameplayAbility.h"
#include "GA_Whirlwind.generated.h"

/** Whirlwind: several pulses of a 360-degree circle AoE around the warrior. */
UCLASS()
class BOSSARENA_API UGA_Whirlwind : public UWarriorGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Whirlwind();

protected:
	virtual void ExecuteAbility() override;

	UFUNCTION()
	void OnPulse();

	UPROPERTY(EditDefaultsOnly, Category = "Whirlwind")
	int32 PulseCount = 3;

	UPROPERTY(EditDefaultsOnly, Category = "Whirlwind")
	float PulseInterval = 0.4f;

	int32 PulsesRemaining = 0;
};
