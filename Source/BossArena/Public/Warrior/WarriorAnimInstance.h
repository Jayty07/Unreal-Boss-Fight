#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "WarriorAnimInstance.generated.h"

/**
 * Parent class for ABP_Warrior. Exposes everything the two-handed locomotion state machine needs:
 * Idle <-> Run (GroundSpeed), Jump/Fall (bIsFalling), Block pose (bIsBlocking), Dodge (bIsDodging),
 * Dead. Attack/dodge/block one-shots are montages played by the abilities.
 */
UCLASS()
class BOSSARENA_API UWarriorAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	float GroundSpeed = 0.0f;

	/** -180..180, movement direction relative to facing (for strafe blendspaces). */
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	float Direction = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	bool bShouldMove = false;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	bool bIsFalling = false;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bIsBlocking = false;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bIsDodging = false;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bIsLeaping = false;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bIsDead = false;
};
