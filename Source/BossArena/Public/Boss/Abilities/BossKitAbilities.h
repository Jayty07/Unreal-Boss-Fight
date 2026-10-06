#pragma once

#include "CoreMinimal.h"
#include "Boss/Abilities/BossAoEGameplayAbility.h"
#include "BossKitAbilities.generated.h"

class ABossAddCharacter;

/**
 * Concrete boss/add kit. Defaults below are mirrored in Content/BossArena/Data/BossAoE.csv (DT_BossAoE);
 * when that table exists its row (RowName) overrides the defaults so designers can tune without C++.
 *
 * Tuning rule: every dodgeable telegraph gives >= 1.4 s of lead time, and the dodge covers 650 uu in
 * 0.35 s with 0.45 s of i-frames, so either running out early or a late dodge through the edge works.
 */

UCLASS()
class BOSSARENA_API UGA_Boss_ConeSwipe : public UBossAoEGameplayAbility
{
	GENERATED_BODY()
public:
	UGA_Boss_ConeSwipe();
};

UCLASS()
class BOSSARENA_API UGA_Boss_Meteor : public UBossAoEGameplayAbility
{
	GENERATED_BODY()
public:
	UGA_Boss_Meteor();
};

UCLASS()
class BOSSARENA_API UGA_Boss_MeteorRain : public UBossAoEGameplayAbility
{
	GENERATED_BODY()
public:
	UGA_Boss_MeteorRain();
};

UCLASS()
class BOSSARENA_API UGA_Boss_DonutSlam : public UBossAoEGameplayAbility
{
	GENERATED_BODY()
public:
	UGA_Boss_DonutSlam();
};

UCLASS()
class BOSSARENA_API UGA_Boss_RotatingBeam : public UBossAoEGameplayAbility
{
	GENERATED_BODY()
public:
	UGA_Boss_RotatingBeam();
};

/** Interruptible: a Shockwave hit cancels it. Otherwise near-lethal raid-wide circle. */
UCLASS()
class BOSSARENA_API UGA_Boss_ExpandingCircle : public UBossAoEGameplayAbility
{
	GENERATED_BODY()
public:
	UGA_Boss_ExpandingCircle();
};

/** Small telegraphed impacts at add spawn points; adds appear where they land. */
UCLASS()
class BOSSARENA_API UGA_Boss_SummonAdds : public UBossAoEGameplayAbility
{
	GENERATED_BODY()
public:
	UGA_Boss_SummonAdds();

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr,
		const FGameplayTagContainer* TargetTags = nullptr, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

protected:
	virtual void ComputePlacements(TArray<FTransform>& OutPlacements) const override;
	virtual void OnImpactResolved() override;

	UPROPERTY(EditDefaultsOnly, Category = "Summon")
	TSubclassOf<ABossAddCharacter> AddClass;

	UPROPERTY(EditDefaultsOnly, Category = "Summon")
	int32 MaxLivingAdds = 6;
};

/** Phase transition: room-wide burst. Survive with dodge i-frames or by breaking LOS behind a pillar. */
UCLASS()
class BOSSARENA_API UGA_Boss_PhaseTransition : public UBossAoEGameplayAbility
{
	GENERATED_BODY()
public:
	UGA_Boss_PhaseTransition();
};

/** Hard enrage: unblockable (ignores i-frames, block, armor) arena-wide wipe. */
UCLASS()
class BOSSARENA_API UGA_Boss_HardEnrage : public UBossAoEGameplayAbility
{
	GENERATED_BODY()
public:
	UGA_Boss_HardEnrage();
};

/** Add: lobs an exploding puddle on its target; the impact leaves a lingering damage patch. */
UCLASS()
class BOSSARENA_API UGA_Add_ExplodingPuddle : public UBossAoEGameplayAbility
{
	GENERATED_BODY()
public:
	UGA_Add_ExplodingPuddle();

protected:
	virtual void OnImpactResolved() override;

	UPROPERTY(EditDefaultsOnly, Category = "Puddle")
	float PuddleDuration = 5.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Puddle")
	float PuddleTickInterval = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Puddle")
	float PuddleDamagePerTick = 250.0f;
};

/** Add: telegraphs a charge line toward its target, then dashes along it. */
UCLASS()
class BOSSARENA_API UGA_Add_ChargeLine : public UBossAoEGameplayAbility
{
	GENERATED_BODY()
public:
	UGA_Add_ChargeLine();

protected:
	virtual void OnImpactResolved() override;
};
