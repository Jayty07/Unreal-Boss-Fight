#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "BossAIController.generated.h"

class ABossArenaCharacterBase;
class UGameplayAbility;

/**
 * Server-side brain for the boss and adds: chase/face the top-threat player, and when not casting
 * and off global cooldown, activate a random ability from the current kit. Facing is frozen while
 * casting so frontal telegraphs stay where they were shown.
 */
UCLASS()
class BOSSARENA_API ABossArenaAIController : public AAIController
{
	GENERATED_BODY()

public:
	ABossArenaAIController();

	virtual void Tick(float DeltaSeconds) override;
	virtual void OnPossess(APawn* InPawn) override;

	UPROPERTY(EditAnywhere, Category = "AI")
	float BossPreferredRange = 450.0f;

	UPROPERTY(EditAnywhere, Category = "AI")
	float AddPreferredRange = 300.0f;

	UPROPERTY(EditAnywhere, Category = "AI")
	float TurnRate = 5.0f;

protected:
	AActor* SelectTarget(ABossArenaCharacterBase* Self) const;

	double NextAbilityTime = 0.0;
	bool bWasCasting = false;
	TSubclassOf<UGameplayAbility> LastAbility;
};
