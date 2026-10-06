#pragma once

#include "CoreMinimal.h"
#include "BossArenaCharacterBase.h"
#include "BossAddCharacter.generated.h"

class UBossAttributeSet;
class UGameplayAbility;
class UStaticMeshComponent;
class UThreatComponent;

/** Summoned add. Only small AoE attacks: exploding puddles and charge lines. */
UCLASS()
class BOSSARENA_API ABossAddCharacter : public ABossArenaCharacterBase
{
	GENERATED_BODY()

public:
	ABossAddCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void HandleDamageTaken(float Damage, AActor* DamageInstigator, bool bFatal) override;
	virtual void Die() override;

	void GetAbilityKit(TArray<TSubclassOf<UGameplayAbility>>& OutKit, float& OutGlobalCooldown) const;
	UThreatComponent* GetThreatComponent() const { return ThreatComponent; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UThreatComponent> ThreatComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> PlaceholderBody;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	TObjectPtr<UBossAttributeSet> AddAttributes;

	UPROPERTY(EditDefaultsOnly, Category = "Add")
	float GlobalCooldown = 2.5f;
};
