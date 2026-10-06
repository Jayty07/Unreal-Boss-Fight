#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "BossArenaGameplayEffects.generated.h"

/** Native GameplayEffects so the encounter works without any GE Blueprint assets. */
UCLASS(Abstract)
class BOSSARENA_API UBossArenaGameplayEffect : public UGameplayEffect
{
	GENERATED_BODY()

protected:
	/** Constructor-only: grants Tags to the target while the effect is active. */
	void AddGrantedTags(const FGameplayTagContainer& Tags);
};

/** Instant: IncomingDamage += SetByCaller(Data.Damage). Used by every AoE in the game. */
UCLASS()
class BOSSARENA_API UGE_BossArenaDamage : public UBossArenaGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_BossArenaDamage();
};

/** Instant: Rage += SetByCaller(Data.Rage). Used for rage gain on hit and rage costs. */
UCLASS()
class BOSSARENA_API UGE_WarriorRageDelta : public UBossArenaGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_WarriorRageDelta();
};

/**
 * Dodge i-frames. Grants State.Invulnerable + State.Dodging for a short window. The server's
 * application is authoritative; the attribute set rejects blockable damage while it is active.
 */
UCLASS()
class BOSSARENA_API UGE_DodgeInvulnerability : public UBossArenaGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_DodgeInvulnerability();
};

/** Infinite while the block ability is held. */
UCLASS()
class BOSSARENA_API UGE_WarriorBlocking : public UBossArenaGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_WarriorBlocking();
};

/** Infinite: grants State.Dead, which blocks every ability. */
UCLASS()
class BOSSARENA_API UGE_BossArenaDead : public UBossArenaGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_BossArenaDead();
};

/** Cooldown with SetByCaller(Data.Cooldown) duration. Subclasses only choose the granted tag. */
UCLASS(Abstract)
class BOSSARENA_API UGE_WarriorCooldownBase : public UBossArenaGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_WarriorCooldownBase();
};

UCLASS()
class BOSSARENA_API UGE_Cooldown_Whirlwind : public UGE_WarriorCooldownBase
{
	GENERATED_BODY()
public:
	UGE_Cooldown_Whirlwind();
};

UCLASS()
class BOSSARENA_API UGE_Cooldown_LeapSlam : public UGE_WarriorCooldownBase
{
	GENERATED_BODY()
public:
	UGE_Cooldown_LeapSlam();
};

UCLASS()
class BOSSARENA_API UGE_Cooldown_Shockwave : public UGE_WarriorCooldownBase
{
	GENERATED_BODY()
public:
	UGE_Cooldown_Shockwave();
};

UCLASS()
class BOSSARENA_API UGE_Cooldown_GroundRend : public UGE_WarriorCooldownBase
{
	GENERATED_BODY()
public:
	UGE_Cooldown_GroundRend();
};

UCLASS()
class BOSSARENA_API UGE_Cooldown_Taunt : public UGE_WarriorCooldownBase
{
	GENERATED_BODY()
public:
	UGE_Cooldown_Taunt();
};

UCLASS()
class BOSSARENA_API UGE_Cooldown_Execute : public UGE_WarriorCooldownBase
{
	GENERATED_BODY()
public:
	UGE_Cooldown_Execute();
};

UCLASS()
class BOSSARENA_API UGE_Cooldown_Dodge : public UGE_WarriorCooldownBase
{
	GENERATED_BODY()
public:
	UGE_Cooldown_Dodge();
};
