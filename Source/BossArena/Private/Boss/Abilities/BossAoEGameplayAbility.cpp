#include "Boss/Abilities/BossAoEGameplayAbility.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AoE/AoETelegraphActor.h"
#include "AoE/BossArenaAoELibrary.h"
#include "Attributes/BossAttributeSet.h"
#include "Boss/BossCharacter.h"
#include "Boss/ThreatComponent.h"
#include "BossArenaCharacterBase.h"
#include "BossArenaGameplayTags.h"
#include "Engine/DataTable.h"
#include "Engine/World.h"
#include "Game/BossArenaGameState.h"
#include "NiagaraSystem.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BossAoEGameplayAbility)

const TCHAR* UBossAoEGameplayAbility::DefaultShapeTablePath = TEXT("/Game/BossArena/Data/DT_BossAoE.DT_BossAoE");

UBossAoEGameplayAbility::UBossAoEGameplayAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

	SetAssetTags(FGameplayTagContainer(BossArenaTags::Ability_Boss));
	ActivationOwnedTags.AddTag(BossArenaTags::State_Casting);
	ActivationBlockedTags.AddTag(BossArenaTags::State_Dead);
	ActivationBlockedTags.AddTag(BossArenaTags::State_Casting);

	ShapeTable = TSoftObjectPtr<UDataTable>(FSoftObjectPath(DefaultShapeTablePath));
}

FBossAoEShapeRow UBossAoEGameplayAbility::ResolveParams() const
{
	if (!RowName.IsNone() && !ShapeTable.IsNull())
	{
		if (const UDataTable* Table = ShapeTable.LoadSynchronous())
		{
			if (const FBossAoEShapeRow* Row = Table->FindRow<FBossAoEShapeRow>(RowName, TEXT("BossAoE"), false))
			{
				return *Row;
			}
		}
	}
	return DefaultParams;
}

bool UBossAoEGameplayAbility::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}
	const UWorld* World = ActorInfo && ActorInfo->AvatarActor.IsValid() ? ActorInfo->AvatarActor->GetWorld() : nullptr;
	return !World || MinRecastInterval <= 0.0f || World->GetTimeSeconds() - LastActivationTime >= MinRecastInterval;
}

ABossArenaCharacterBase* UBossAoEGameplayAbility::GetCaster() const
{
	return CurrentActorInfo ? Cast<ABossArenaCharacterBase>(CurrentActorInfo->AvatarActor.Get()) : nullptr;
}

AActor* UBossAoEGameplayAbility::GetPrimaryTarget() const
{
	ABossArenaCharacterBase* Caster = GetCaster();
	if (!Caster)
	{
		return nullptr;
	}
	if (const UThreatComponent* Threat = Caster->FindComponentByClass<UThreatComponent>())
	{
		if (AActor* Top = Threat->GetTopThreatActor())
		{
			return Top;
		}
	}
	TArray<ABossArenaCharacterBase*> Players;
	UBossArenaAoELibrary::GetLivingPlayers(Caster, Players);
	return Players.Num() > 0 ? Players[0] : nullptr;
}

FAoEShape UBossAoEGameplayAbility::GetScaledShape() const
{
	const ABossCharacter* Boss = Cast<ABossCharacter>(GetCaster());
	const FAoEShape Base = ActiveParams.ToShape();
	return (Boss && !ActiveParams.bUnblockable) ? Base.Scaled(Boss->GetAoERadiusScale()) : Base;
}

float UBossAoEGameplayAbility::GetDamageMultiplier() const
{
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	bool bFound = false;
	const float Multiplier = ASC ? ASC->GetGameplayAttributeValue(UBossAttributeSet::GetDamageAttribute(), bFound) : 1.0f;
	return bFound ? Multiplier : 1.0f;
}

void UBossAoEGameplayAbility::ComputePlacements(TArray<FTransform>& OutPlacements) const
{
	ABossArenaCharacterBase* Caster = GetCaster();
	if (!Caster)
	{
		return;
	}

	const FVector CasterGround = UBossArenaAoELibrary::GetGroundLocation(Caster);
	const AActor* Target = GetPrimaryTarget();
	FRotator Facing(0.0f, Caster->GetActorRotation().Yaw, 0.0f);
	if (Target)
	{
		const FVector ToTarget = (Target->GetActorLocation() - Caster->GetActorLocation()).GetSafeNormal2D();
		if (!ToTarget.IsNearlyZero())
		{
			Facing = FRotator(0.0f, ToTarget.Rotation().Yaw, 0.0f);
		}
	}

	TArray<ABossArenaCharacterBase*> Players;
	UBossArenaAoELibrary::GetLivingPlayers(Caster, Players);
	const int32 Count = FMath::Max(1, ActiveParams.Count);

	auto AddFanned = [&OutPlacements, Count](const FVector& Location, const FRotator& BaseRotation)
	{
		for (int32 Index = 0; Index < Count; ++Index)
		{
			OutPlacements.Add(FTransform(FRotator(0.0f, BaseRotation.Yaw + 360.0f * Index / Count, 0.0f), Location));
		}
	};

	switch (ActiveParams.Placement)
	{
	case EBossAoEPlacement::Self:
		AddFanned(CasterGround, Facing);
		break;

	case EBossAoEPlacement::TopThreat:
		if (Target)
		{
			OutPlacements.Add(FTransform(Facing, UBossArenaAoELibrary::GetGroundLocation(Target)));
		}
		break;

	case EBossAoEPlacement::RandomPlayer:
		if (Players.Num() > 0)
		{
			const ABossArenaCharacterBase* Chosen = Players[FMath::RandRange(0, Players.Num() - 1)];
			OutPlacements.Add(FTransform(Facing, UBossArenaAoELibrary::GetGroundLocation(Chosen)));
		}
		break;

	case EBossAoEPlacement::EachPlayer:
		for (const ABossArenaCharacterBase* Player : Players)
		{
			OutPlacements.Add(FTransform(Facing, UBossArenaAoELibrary::GetGroundLocation(Player)));
		}
		break;

	case EBossAoEPlacement::ArenaCenter:
	{
		FVector Center = UBossArenaAoELibrary::GetArenaCenter(Caster);
		Center.Z = CasterGround.Z;
		AddFanned(Center, Facing);
		break;
	}
	}
}

void UBossAoEGameplayAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ABossArenaCharacterBase* Caster = GetCaster();
	UWorld* World = GetWorld();
	if (!Caster || !World)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	LastActivationTime = World->GetTimeSeconds();
	ActiveParams = ResolveParams();
	ActiveShape = GetScaledShape();

	TArray<FTransform> Placements;
	ComputePlacements(Placements);
	if (Placements.Num() == 0)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	FAoETelegraphParams TelegraphParams;
	TelegraphParams.Shape = ActiveShape;
	TelegraphParams.StartServerTime = UBossArenaAoELibrary::GetServerTime(Caster);
	TelegraphParams.CastTime = ActiveParams.CastTime;
	TelegraphParams.ActiveDuration = ActiveParams.ActiveDuration;
	TelegraphParams.SweepDegreesPerSecond = ActiveParams.SweepDegreesPerSecond;
	TelegraphParams.Color = ActiveParams.Color;
	TelegraphParams.Style = EAoETelegraphStyle::Enemy;

	Telegraphs.Reset();
	for (const FTransform& Placement : Placements)
	{
		if (AAoETelegraphActor* Telegraph = AAoETelegraphActor::SpawnTelegraph(World, Caster, TelegraphParams, Placement))
		{
			Telegraphs.Add(Telegraph);
		}
	}

	if (ABossCharacter* Boss = Cast<ABossCharacter>(Caster))
	{
		Boss->StartCast(ActiveParams.CastName, ActiveParams.CastTime, ActiveParams.bInterruptible);
	}

	if (!ActiveParams.RaidWarning.IsEmpty())
	{
		if (ABossArenaGameState* GameState = World->GetGameState<ABossArenaGameState>())
		{
			GameState->BroadcastRaidWarning(ActiveParams.RaidWarning, ActiveParams.Color, FMath::Max(2.5f, ActiveParams.CastTime));
		}
	}

	if (ActiveParams.bInterruptible)
	{
		UAbilityTask_WaitGameplayEvent* WaitInterrupt = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, BossArenaTags::Event_Interrupt, nullptr, true);
		WaitInterrupt->EventReceived.AddDynamic(this, &UBossAoEGameplayAbility::OnInterruptEvent);
		WaitInterrupt->ReadyForActivation();
	}

	UAbilityTask_WaitDelay* WaitCast = UAbilityTask_WaitDelay::WaitDelay(this, FMath::Max(0.01f, ActiveParams.CastTime));
	WaitCast->OnFinish.AddDynamic(this, &UBossAoEGameplayAbility::OnCastFinished);
	WaitCast->ReadyForActivation();
}

void UBossAoEGameplayAbility::OnCastFinished()
{
	if (ABossCharacter* Boss = Cast<ABossCharacter>(GetCaster()))
	{
		Boss->EndCast(false);
	}

	if (ActiveParams.ActiveDuration > 0.0f)
	{
		ActiveEndTime = GetWorld()->GetTimeSeconds() + ActiveParams.ActiveDuration;
		OnActiveTick();
		GetWorld()->GetTimerManager().SetTimer(ActiveTickTimer, FTimerDelegate::CreateUObject(this, &UBossAoEGameplayAbility::OnActiveTick),
			FMath::Max(0.05f, ActiveParams.TickInterval), true);
		return;
	}

	UNiagaraSystem* ImpactEffect = ActiveParams.ImpactEffect.LoadSynchronous();
	for (AAoETelegraphActor* Telegraph : Telegraphs)
	{
		if (IsValid(Telegraph))
		{
			ResolveAt(Telegraph->GetActorTransform(), ActiveShape);
			Telegraph->MulticastImpact(ImpactEffect);
		}
	}

	OnImpactResolved();
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UBossAoEGameplayAbility::OnActiveTick()
{
	UWorld* World = GetWorld();
	if (!World || World->GetTimeSeconds() >= ActiveEndTime)
	{
		if (World)
		{
			World->GetTimerManager().ClearTimer(ActiveTickTimer);
		}
		OnImpactResolved();
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	const double Now = UBossArenaAoELibrary::GetServerTime(GetCaster());
	for (AAoETelegraphActor* Telegraph : Telegraphs)
	{
		if (IsValid(Telegraph))
		{
			ResolveAt(FTransform(Telegraph->GetRotationAtServerTime(Now), Telegraph->GetActorLocation()), ActiveShape);
		}
	}
}

void UBossAoEGameplayAbility::ResolveAt(const FTransform& Where, const FAoEShape& Shape)
{
	ABossArenaCharacterBase* Caster = GetCaster();
	if (!Caster || ActiveParams.Damage <= 0.0f)
	{
		return;
	}

	TArray<ABossArenaCharacterBase*> Targets;
	UBossArenaAoELibrary::GatherTargetsInShape(Caster, Shape, Where.GetLocation(), Where.Rotator(), Caster->GetTeam(),
		ActiveParams.bRequiresLineOfSight, Caster->GetActorLocation(), Targets);
	UBossArenaAoELibrary::ApplyAoEDamage(GetAbilitySystemComponentFromActorInfo(), Targets, ActiveParams.Damage * GetDamageMultiplier(), ActiveParams.bUnblockable, this);
}

void UBossAoEGameplayAbility::OnInterruptEvent(FGameplayEventData Payload)
{
	ABossArenaCharacterBase* Caster = GetCaster();
	if (ABossCharacter* Boss = Cast<ABossCharacter>(Caster))
	{
		Boss->EndCast(true);
	}
	if (Caster)
	{
		Caster->MulticastCombatText(0.0f, EBossArenaCombatText::Interrupted);
		if (ABossArenaGameState* GameState = Caster->GetWorld()->GetGameState<ABossArenaGameState>())
		{
			GameState->BroadcastRaidWarning(FString::Printf(TEXT("%s interrupted!"), *ActiveParams.CastName), FLinearColor(0.7f, 0.5f, 1.0f), 2.5f);
		}
	}
	ClearTelegraphs(0.0f);
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UBossAoEGameplayAbility::ClearTelegraphs(float Linger)
{
	for (AAoETelegraphActor* Telegraph : Telegraphs)
	{
		if (IsValid(Telegraph))
		{
			if (Linger > 0.0f)
			{
				Telegraph->SetLifeSpan(Linger);
			}
			else
			{
				Telegraph->Destroy();
			}
		}
	}
	Telegraphs.Reset();
}

void UBossAoEGameplayAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ActiveTickTimer);
	}
	if (ABossCharacter* Boss = Cast<ABossCharacter>(GetCaster()))
	{
		Boss->EndCast(bWasCancelled);
	}
	// Leave a short linger so clients see the impact flash.
	ClearTelegraphs(bWasCancelled ? 0.05f : 0.4f);
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
