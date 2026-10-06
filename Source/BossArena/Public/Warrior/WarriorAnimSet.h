#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WarriorAnimSet.generated.h"

class UAnimMontage;

UENUM(BlueprintType)
enum class EWarriorMontage : uint8
{
	None,
	Cleave1,
	Cleave2,
	Cleave3,
	Whirlwind,
	LeapSlam,
	Shockwave,
	GroundRend,
	TauntShout,
	Execute,
	DodgeForward,
	DodgeBackward,
	DodgeLeft,
	DodgeRight,
	Block,
	Death
};

/**
 * Two-handed sword montage set. Assign montages authored on your skeleton (use the "DefaultSlot"
 * or an "UpperBody" slot in the AnimBP). Abilities run on timers, so missing montages are fine.
 */
UCLASS(BlueprintType)
class BOSSARENA_API UWarriorAnimSet : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Animation")
	UAnimMontage* GetMontage(EWarriorMontage Slot) const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
	TMap<EWarriorMontage, TObjectPtr<UAnimMontage>> Montages;
};
