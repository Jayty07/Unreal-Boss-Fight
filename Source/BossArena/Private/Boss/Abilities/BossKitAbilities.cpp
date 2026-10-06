#include "Boss/Abilities/BossKitAbilities.h"

#include "AoE/AoEPersistentZone.h"
#include "AoE/AoETelegraphActor.h"
#include "Boss/BossAddCharacter.h"
#include "BossArenaGameplayTags.h"
#include "Engine/World.h"
#include "Game/BossArenaGameMode.h"
#include "GameFramework/Character.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BossKitAbilities)

namespace
{
	FBossAoEShapeRow MakeRow(const TCHAR* Name, EBossAoEShape Shape, EBossAoEPlacement Placement, float CastTime, float Damage, const FLinearColor& Color)
	{
		FBossAoEShapeRow Row;
		Row.CastName = Name;
		Row.Shape = Shape;
		Row.Placement = Placement;
		Row.CastTime = CastTime;
		Row.Damage = Damage;
		Row.Color = Color;
		return Row;
	}

	const FLinearColor Orange(1.0f, 0.35f, 0.05f);
	const FLinearColor Red(1.0f, 0.05f, 0.05f);
	const FLinearColor Purple(0.7f, 0.2f, 1.0f);
	const FLinearColor Yellow(1.0f, 0.85f, 0.1f);
}

UGA_Boss_ConeSwipe::UGA_Boss_ConeSwipe()
{
	RowName = TEXT("ConeSwipe");
	DefaultParams = MakeRow(TEXT("Ashen Cleave"), EBossAoEShape::Cone, EBossAoEPlacement::Self, 1.8f, 3500.0f, Orange);
	DefaultParams.Radius = 1000.0f;
	DefaultParams.ConeAngle = 100.0f;
}

UGA_Boss_Meteor::UGA_Boss_Meteor()
{
	RowName = TEXT("Meteor");
	DefaultParams = MakeRow(TEXT("Meteor"), EBossAoEShape::Circle, EBossAoEPlacement::TopThreat, 2.4f, 4500.0f, Red);
	DefaultParams.Radius = 450.0f;
	DefaultParams.RaidWarning = TEXT("Meteor on the tank - dodge out!");
}

UGA_Boss_MeteorRain::UGA_Boss_MeteorRain()
{
	RowName = TEXT("MeteorRain");
	DefaultParams = MakeRow(TEXT("Meteor Rain"), EBossAoEShape::Circle, EBossAoEPlacement::EachPlayer, 2.4f, 3500.0f, Red);
	DefaultParams.Radius = 400.0f;
	DefaultParams.RaidWarning = TEXT("Meteors on everyone - spread and dodge!");
	MinRecastInterval = 10.0f;
}

UGA_Boss_DonutSlam::UGA_Boss_DonutSlam()
{
	RowName = TEXT("DonutSlam");
	DefaultParams = MakeRow(TEXT("Cinder Ring"), EBossAoEShape::Donut, EBossAoEPlacement::Self, 3.0f, 5000.0f, Orange);
	DefaultParams.Radius = 2600.0f;
	DefaultParams.InnerRadius = 380.0f;
	DefaultParams.RaidWarning = TEXT("Cinder Ring - get under the boss!");
	MinRecastInterval = 12.0f;
}

UGA_Boss_RotatingBeam::UGA_Boss_RotatingBeam()
{
	RowName = TEXT("RotatingBeam");
	DefaultParams = MakeRow(TEXT("Searing Beams"), EBossAoEShape::RotatingSweep, EBossAoEPlacement::Self, 2.5f, 700.0f, Yellow);
	DefaultParams.Length = 2600.0f;
	DefaultParams.Width = 220.0f;
	DefaultParams.Count = 2;
	DefaultParams.ActiveDuration = 6.0f;
	DefaultParams.TickInterval = 0.25f;
	DefaultParams.SweepDegreesPerSecond = 40.0f;
	DefaultParams.RaidWarning = TEXT("Searing Beams - move with the sweep or dodge through!");
	MinRecastInterval = 18.0f;
}

UGA_Boss_ExpandingCircle::UGA_Boss_ExpandingCircle()
{
	RowName = TEXT("ExpandingCircle");
	DefaultParams = MakeRow(TEXT("Searing Nova"), EBossAoEShape::Circle, EBossAoEPlacement::Self, 5.0f, 9000.0f, Purple);
	DefaultParams.Radius = 2600.0f;
	DefaultParams.bInterruptible = true;
	DefaultParams.RaidWarning = TEXT("Searing Nova - INTERRUPT with Shockwave!");
	MinRecastInterval = 20.0f;
}

UGA_Boss_SummonAdds::UGA_Boss_SummonAdds()
{
	RowName = TEXT("SummonAdds");
	DefaultParams = MakeRow(TEXT("Call the Embers"), EBossAoEShape::Circle, EBossAoEPlacement::ArenaCenter, 2.0f, 1500.0f, Orange);
	DefaultParams.Radius = 250.0f;
	DefaultParams.Count = 3;
	DefaultParams.RaidWarning = TEXT("Ember Spawns incoming!");
	MinRecastInterval = 35.0f;
	AddClass = ABossAddCharacter::StaticClass();
}

bool UGA_Boss_SummonAdds::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}
	const UWorld* World = ActorInfo && ActorInfo->AvatarActor.IsValid() ? ActorInfo->AvatarActor->GetWorld() : nullptr;
	const ABossArenaGameMode* GameMode = World ? World->GetAuthGameMode<ABossArenaGameMode>() : nullptr;
	return GameMode && GameMode->GetLivingAddCount() < MaxLivingAdds;
}

void UGA_Boss_SummonAdds::ComputePlacements(TArray<FTransform>& OutPlacements) const
{
	const UWorld* World = GetWorld();
	const ABossArenaGameMode* GameMode = World ? World->GetAuthGameMode<ABossArenaGameMode>() : nullptr;
	if (GameMode)
	{
		GameMode->GetAddSpawnTransforms(FMath::Max(1, ActiveParams.Count), OutPlacements);
	}
	if (OutPlacements.Num() == 0)
	{
		Super::ComputePlacements(OutPlacements);
	}
}

void UGA_Boss_SummonAdds::OnImpactResolved()
{
	ABossArenaGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ABossArenaGameMode>() : nullptr;
	if (!GameMode || !AddClass)
	{
		return;
	}
	for (AAoETelegraphActor* Telegraph : Telegraphs)
	{
		if (IsValid(Telegraph))
		{
			GameMode->SpawnAdd(AddClass, Telegraph->GetActorTransform());
		}
	}
}

UGA_Boss_PhaseTransition::UGA_Boss_PhaseTransition()
{
	SetAssetTags(FGameplayTagContainer::CreateFromArray(TArray<FGameplayTag>{ BossArenaTags::Ability_Boss, BossArenaTags::Ability_Boss_Transition }));
	RowName = TEXT("PhaseTransition");
	DefaultParams = MakeRow(TEXT("Ashfall Eruption"), EBossAoEShape::Circle, EBossAoEPlacement::ArenaCenter, 3.0f, 6000.0f, Red);
	DefaultParams.Radius = 4000.0f;
	DefaultParams.bRequiresLineOfSight = true;
	DefaultParams.RaidWarning = TEXT("Ashfall Eruption - hide behind a pillar or dodge the blast!");
}

UGA_Boss_HardEnrage::UGA_Boss_HardEnrage()
{
	SetAssetTags(FGameplayTagContainer::CreateFromArray(TArray<FGameplayTag>{ BossArenaTags::Ability_Boss, BossArenaTags::Ability_Boss_HardEnrage }));
	RowName = TEXT("HardEnrage");
	DefaultParams = MakeRow(TEXT("Annihilation"), EBossAoEShape::Circle, EBossAoEPlacement::ArenaCenter, 4.0f, 999999.0f, Red);
	DefaultParams.Radius = 6000.0f;
	DefaultParams.bUnblockable = true;
	DefaultParams.RaidWarning = TEXT("ANNIHILATION - there is no escape!");
}

UGA_Add_ExplodingPuddle::UGA_Add_ExplodingPuddle()
{
	RowName = TEXT("AddExplodingPuddle");
	DefaultParams = MakeRow(TEXT("Exploding Puddle"), EBossAoEShape::Circle, EBossAoEPlacement::TopThreat, 1.6f, 800.0f, Orange);
	DefaultParams.Radius = 260.0f;
	MinRecastInterval = 5.0f;
}

void UGA_Add_ExplodingPuddle::OnImpactResolved()
{
	ABossArenaCharacterBase* Caster = GetCaster();
	if (!Caster)
	{
		return;
	}
	for (AAoETelegraphActor* Telegraph : Telegraphs)
	{
		if (IsValid(Telegraph))
		{
			AAoEPersistentZone::SpawnZone(Caster, GetAbilitySystemComponentFromActorInfo(), Caster->GetTeam(), ActiveShape, Telegraph->GetActorTransform(),
				PuddleDuration, PuddleTickInterval, PuddleDamagePerTick * GetDamageMultiplier(), ActiveParams.Color);
		}
	}
}

UGA_Add_ChargeLine::UGA_Add_ChargeLine()
{
	RowName = TEXT("AddChargeLine");
	DefaultParams = MakeRow(TEXT("Ember Charge"), EBossAoEShape::Line, EBossAoEPlacement::Self, 1.4f, 1000.0f, Orange);
	DefaultParams.Length = 900.0f;
	DefaultParams.Width = 160.0f;
	MinRecastInterval = 4.0f;
}

void UGA_Add_ChargeLine::OnImpactResolved()
{
	ACharacter* Caster = GetCaster();
	if (Caster && Telegraphs.Num() > 0 && IsValid(Telegraphs[0]))
	{
		const FVector Direction = Telegraphs[0]->GetActorForwardVector().GetSafeNormal2D();
		Caster->LaunchCharacter(Direction * 2400.0f + FVector(0.0f, 0.0f, 50.0f), true, true);
	}
}
