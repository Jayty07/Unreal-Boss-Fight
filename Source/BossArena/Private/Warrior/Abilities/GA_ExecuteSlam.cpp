#include "Warrior/Abilities/GA_ExecuteSlam.h"

#include "Boss/BossCharacter.h"
#include "BossArenaGameplayTags.h"
#include "Engine/World.h"
#include "Game/BossArenaGameState.h"
#include "GAS/BossArenaGameplayEffects.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GA_ExecuteSlam)

UGA_ExecuteSlam::UGA_ExecuteSlam()
{
	FGameplayTagContainer Tags = GetAssetTags();
	Tags.AddTag(BossArenaTags::Ability_Warrior_Execute);
	SetAssetTags(Tags);
	CooldownGameplayEffectClass = UGE_Cooldown_Execute::StaticClass();

	AimShape = FAoEShape::MakeCircle(500.0f);
	AimOffset = FVector(150.0f, 0.0f, 0.0f);
	BaseDamage = 2000.0f;
	ImpactDelay = 0.55f;
	RageCost = 30.0f;
	CooldownDuration = 6.0f;
	MontageSlot = EWarriorMontage::Execute;
	IndicatorColor = FLinearColor(1.0f, 0.2f, 0.2f);
}

bool UGA_ExecuteSlam::IsUsableNow(const AActor* Avatar) const
{
	const UWorld* World = Avatar ? Avatar->GetWorld() : nullptr;
	const ABossArenaGameState* GameState = World ? World->GetGameState<ABossArenaGameState>() : nullptr;
	const ABossCharacter* Boss = GameState ? GameState->GetBoss() : nullptr;
	return Boss && Boss->IsAlive() && Boss->GetHealthPercent() <= ExecuteThreshold;
}
