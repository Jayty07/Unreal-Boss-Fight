#pragma once

#include "CoreMinimal.h"
#include "Warrior/Abilities/WarriorGameplayAbility.h"
#include "GA_GroundRend.generated.h"

/** Ground rend: tears a lingering rectangular patch that damages enemies inside over time. */
UCLASS()
class BOSSARENA_API UGA_GroundRend : public UWarriorGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_GroundRend();

protected:
	virtual void PerformImpact() override;

	UPROPERTY(EditDefaultsOnly, Category = "Ground Rend")
	float ZoneDuration = 6.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Ground Rend")
	float ZoneTickInterval = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Ground Rend")
	float DamagePerTick = 90.0f;
};
