#pragma once

#include "CoreMinimal.h"
#include "BossArenaTypes.h"
#include "GameFramework/Actor.h"
#include "AoEPersistentZone.generated.h"

class UAbilitySystemComponent;
class UAoEShapeMeshComponent;

/**
 * Lingering ground AoE (warrior Ground Rend, add exploding puddles). Damage ticks are resolved on the
 * server with the same overlap + shape test as every other AoE, so dodge i-frames negate ticks too.
 */
UCLASS()
class BOSSARENA_API AAoEPersistentZone : public AActor
{
	GENERATED_BODY()

public:
	AAoEPersistentZone();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Server only. */
	static AAoEPersistentZone* SpawnZone(AActor* InInstigator, UAbilitySystemComponent* SourceASC, EBossArenaTeam InstigatorTeam, const FAoEShape& Shape,
		const FTransform& Transform, float Duration, float TickInterval, float DamagePerTick, const FLinearColor& Color);

protected:
	void ApplyTick();

	UPROPERTY(VisibleAnywhere, Category = "Zone")
	TObjectPtr<UAoEShapeMeshComponent> ShapeMesh;

	UPROPERTY(Replicated)
	FAoEShape Shape;

	UPROPERTY(Replicated)
	FLinearColor Color = FLinearColor(1.0f, 0.4f, 0.1f);

	UPROPERTY(Replicated)
	float Duration = 6.0f;

	TWeakObjectPtr<UAbilitySystemComponent> SourceASC;
	EBossArenaTeam InstigatorTeam = EBossArenaTeam::Neutral;
	float TickInterval = 0.5f;
	float DamagePerTick = 100.0f;
	double LocalSpawnTime = 0.0;
	double NextPulseTime = 0.0;
	FTimerHandle TickTimer;
};
