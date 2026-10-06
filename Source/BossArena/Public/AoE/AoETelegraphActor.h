#pragma once

#include "CoreMinimal.h"
#include "BossArenaTypes.h"
#include "GameFramework/Actor.h"
#include "AoETelegraphActor.generated.h"

class UAoEShapeMeshComponent;
class UNiagaraSystem;

UENUM(BlueprintType)
enum class EAoETelegraphStyle : uint8
{
	/** Boss/add telegraph: area + outline + cast-progress fill. */
	Enemy,
	/** Brief faint flash of a warrior AoE where it resolved. */
	PlayerFlash
};

USTRUCT(BlueprintType)
struct BOSSARENA_API FAoETelegraphParams
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Telegraph")
	FAoEShape Shape;

	UPROPERTY(BlueprintReadOnly, Category = "Telegraph")
	double StartServerTime = 0.0;

	UPROPERTY(BlueprintReadOnly, Category = "Telegraph")
	float CastTime = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Telegraph")
	float ActiveDuration = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Telegraph")
	float SweepDegreesPerSecond = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Telegraph")
	FLinearColor Color = FLinearColor(1.0f, 0.25f, 0.05f, 1.0f);

	UPROPERTY(BlueprintReadOnly, Category = "Telegraph")
	EAoETelegraphStyle Style = EAoETelegraphStyle::Enemy;
};

/**
 * Server-spawned, replicated ground telegraph. Clients derive cast progress and sweep rotation from
 * the replicated start time and the synced server clock, so no per-frame replication is needed.
 */
UCLASS()
class BOSSARENA_API AAoETelegraphActor : public AActor
{
	GENERATED_BODY()

public:
	AAoETelegraphActor();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;

	static AAoETelegraphActor* SpawnTelegraph(UWorld* World, AActor* InOwner, const FAoETelegraphParams& Params, const FTransform& Transform);

	/** Server: replicated faint flash of a resolved warrior AoE. */
	static AAoETelegraphActor* SpawnFlash(AActor* InOwner, const FAoEShape& Shape, const FTransform& Transform, const FLinearColor& Color);

	/** Rotation of a sweep at the given server time (constant for non-sweeps). */
	FRotator GetRotationAtServerTime(double ServerTime) const;

	const FAoETelegraphParams& GetParams() const { return Params; }

	/** Server: impact burst on every client (ImpactEffect Niagara if set, else the procedural AAoEImpactVFX). */
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastImpact(UNiagaraSystem* ImpactEffect);

protected:
	UFUNCTION()
	void OnRep_Params();

	void RefreshVisual();

	UPROPERTY(VisibleAnywhere, Category = "Telegraph")
	TObjectPtr<UAoEShapeMeshComponent> ShapeMesh;

	UPROPERTY(ReplicatedUsing = OnRep_Params)
	FAoETelegraphParams Params;

	UPROPERTY(Replicated)
	float BaseYaw = 0.0f;

	void UpdateActiveVFX();

	double LocalImpactTime = -1.0;
	double NextPulseTime = 0.0;
	bool bPlayedActiveBurst = false;
	double LocalSpawnTime = 0.0;
};
