#pragma once

#include "CoreMinimal.h"
#include "Warrior/Abilities/WarriorGameplayAbility.h"
#include "GA_CleaveCombo.generated.h"

USTRUCT(BlueprintType)
struct BOSSARENA_API FWarriorComboSwing
{
	GENERATED_BODY()

	/** Frontal cone in front of the warrior. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combo")
	FAoEShape Shape;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combo")
	float Damage = 200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combo")
	float ImpactDelay = 0.25f;

	/** Time after impact before the next swing can start. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combo")
	float Recovery = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combo")
	float RageOnHit = 6.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combo")
	EWarriorMontage Montage = EWarriorMontage::Cleave1;
};

/** Cleaving combo: 3-hit chain, each swing a frontal cone AoE. Hold LMB to keep swinging. */
UCLASS()
class BOSSARENA_API UGA_CleaveCombo : public UWarriorGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_CleaveCombo();

protected:
	virtual void ExecuteAbility() override;

	UFUNCTION()
	void OnSwingImpact();

	UFUNCTION()
	void OnSwingRecovered();

	UPROPERTY(EditDefaultsOnly, Category = "Combo")
	TArray<FWarriorComboSwing> Swings;

	/** If the next swing starts later than this after the previous one ended, the chain resets. */
	UPROPERTY(EditDefaultsOnly, Category = "Combo")
	float ComboWindow = 0.9f;

	int32 NextSwingIndex = 0;
	int32 ActiveSwingIndex = 0;
	double LastSwingEndTime = -100.0;
};
