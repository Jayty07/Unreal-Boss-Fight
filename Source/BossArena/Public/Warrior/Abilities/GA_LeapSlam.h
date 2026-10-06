#pragma once

#include "CoreMinimal.h"
#include "Warrior/Abilities/WarriorGameplayAbility.h"
#include "GA_LeapSlam.generated.h"

/** Leaping slam: root-motion jump toward the aim direction; landing impact is a circle AoE. */
UCLASS()
class BOSSARENA_API UGA_LeapSlam : public UWarriorGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_LeapSlam();

protected:
	virtual void ExecuteAbility() override;

	UFUNCTION()
	void OnLanded();

	UPROPERTY(EditDefaultsOnly, Category = "Leap")
	float LeapDistance = 900.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Leap")
	float LeapHeight = 220.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Leap")
	float LeapDuration = 0.55f;

	bool bLanded = false;
};
