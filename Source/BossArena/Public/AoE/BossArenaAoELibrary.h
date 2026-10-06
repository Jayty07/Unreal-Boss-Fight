#pragma once

#include "CoreMinimal.h"
#include "BossArenaTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "BossArenaAoELibrary.generated.h"

class ABossArenaCharacterBase;
class UAbilitySystemComponent;

/** Shape math and server-side AoE resolution shared by warrior, boss, adds and zones. */
UCLASS()
class BOSSARENA_API UBossArenaAoELibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** 2D containment test (height ignored). PointRadius lets a capsule clip the edge. */
	UFUNCTION(BlueprintPure, Category = "BossArena|AoE")
	static bool IsPointInShape(const FAoEShape& Shape, const FVector& Origin, const FRotator& Rotation, const FVector& Point, float PointRadius = 0.0f);

	/**
	 * Overlap query + precise shape test. Returns living characters hostile to InstigatorTeam.
	 * When bRequireLineOfSight is set, targets occluded by WorldStatic geometry from LOSOrigin are skipped.
	 */
	UFUNCTION(BlueprintCallable, Category = "BossArena|AoE", meta = (WorldContext = "WorldContextObject"))
	static void GatherTargetsInShape(const UObject* WorldContextObject, const FAoEShape& Shape, const FVector& Origin, const FRotator& Rotation,
		EBossArenaTeam InstigatorTeam, bool bRequireLineOfSight, const FVector& LOSOrigin, TArray<ABossArenaCharacterBase*>& OutTargets);

	/** Server only. Applies UGE_BossArenaDamage from SourceASC to each target. Returns targets affected. */
	static int32 ApplyAoEDamage(UAbilitySystemComponent* SourceASC, const TArray<ABossArenaCharacterBase*>& Targets, float Damage, bool bUnblockable, const UObject* SourceObject = nullptr);

	/** Server only. Sends Event.Interrupt to every target (cancels interruptible casts). */
	static void SendInterrupt(AActor* Instigator, const TArray<ABossArenaCharacterBase*>& Targets);

	UFUNCTION(BlueprintPure, Category = "BossArena", meta = (WorldContext = "WorldContextObject"))
	static double GetServerTime(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "BossArena")
	static EBossArenaTeam GetTeam(const AActor* Actor);

	static bool AreHostile(EBossArenaTeam A, EBossArenaTeam B);

	static void GetLivingPlayers(const UObject* WorldContextObject, TArray<ABossArenaCharacterBase*>& OutPlayers);

	/** Arena centre (first ABossArenaBlockout) or world origin. */
	static FVector GetArenaCenter(const UObject* WorldContextObject);

	/** Location of the actor's feet (capsule bottom) for ground telegraphs. */
	static FVector GetGroundLocation(const AActor* Actor);

	/** Max vertical distance between AoE origin and target centre. Jumping does not avoid AoEs. */
	static constexpr float MaxHeightDifference = 600.0f;
};
