#pragma once

#include "CoreMinimal.h"
#include "ActiveGameplayEffectHandle.h"
#include "Warrior/Abilities/WarriorGameplayAbility.h"
#include "GA_Block.generated.h"

/** Hold to block: grants State.Blocking (damage mitigation in UWarriorAttributeSet). Not an i-frame. */
UCLASS()
class BOSSARENA_API UGA_Block : public UWarriorGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Block();

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	virtual void ExecuteAbility() override;

	UFUNCTION()
	void OnBlockReleased(float TimeHeld);

	FActiveGameplayEffectHandle BlockEffectHandle;
};
