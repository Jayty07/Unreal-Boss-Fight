#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "BossArenaTypes.h"
#include "GameFramework/Character.h"
#include "BossArenaCharacterBase.generated.h"

class UAbilitySystemComponent;
class UGameplayAbility;

struct FBossArenaCombatTextEntry
{
	FString Text;
	FLinearColor Color = FLinearColor::White;
	double SpawnTime = 0.0;
	float HorizontalOffset = 0.0f;
};

/** Shared base for warrior, boss and adds: owns the ASC, team, health helpers and death. */
UCLASS(Abstract)
class BOSSARENA_API ABossArenaCharacterBase : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ABossArenaCharacterBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;

	UFUNCTION(BlueprintPure, Category = "BossArena")
	EBossArenaTeam GetTeam() const { return Team; }

	UFUNCTION(BlueprintPure, Category = "BossArena")
	bool IsAlive() const { return !bIsDead; }

	UFUNCTION(BlueprintPure, Category = "BossArena")
	float GetHealth() const;

	UFUNCTION(BlueprintPure, Category = "BossArena")
	float GetMaxHealth() const;

	UFUNCTION(BlueprintPure, Category = "BossArena")
	float GetHealthPercent() const;

	UFUNCTION(BlueprintPure, Category = "BossArena")
	virtual FString GetCombatName() const;

	/** Server: called by the attribute set after mitigation has been applied. */
	virtual void HandleDamageTaken(float Damage, AActor* DamageInstigator, bool bFatal);

	/** Server: blockable damage was discarded by i-frames. */
	void NotifyDamageNegated();

	/** Server. */
	virtual void Die();

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastCombatText(float Amount, EBossArenaCombatText Kind);

	/** Client-side floating combat text drawn by the HUD. */
	TArray<FBossArenaCombatTextEntry> CombatTexts;

protected:
	void InitAbilitySystem();
	virtual void GiveDefaultAbilities();
	virtual void OnAbilitySystemInitialized() {}
	virtual void OnDeathStateChanged();

	UFUNCTION()
	void OnRep_IsDead();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
	TArray<TSubclassOf<UGameplayAbility>> DefaultAbilities;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BossArena")
	EBossArenaTeam Team = EBossArenaTeam::Neutral;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BossArena")
	FString CombatName;

	UPROPERTY(ReplicatedUsing = OnRep_IsDead, BlueprintReadOnly, Category = "BossArena")
	bool bIsDead = false;

	bool bAbilitiesGiven = false;
};
