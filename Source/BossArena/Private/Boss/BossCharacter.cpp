#include "Boss/BossCharacter.h"

#include "AbilitySystemComponent.h"
#include "AoE/BossArenaAoELibrary.h"
#include "Attributes/BossAttributeSet.h"
#include "Boss/Abilities/BossKitAbilities.h"
#include "Boss/BossAIController.h"
#include "Boss/BossPhaseComponent.h"
#include "Boss/ThreatComponent.h"
#include "BossArenaGameplayTags.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Game/BossArenaGameMode.h"
#include "Game/BossArenaGameState.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BossCharacter)

ABossCharacter::ABossCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
	Team = EBossArenaTeam::Enemies;
	CombatName = TEXT("Kharzul, the Ashen Warden");
	bAlwaysRelevant = true;

	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	GetCapsuleComponent()->InitCapsuleSize(130.0f, 230.0f);
	GetMesh()->SetRelativeLocationAndRotation(FVector(0.0f, 0.0f, -230.0f), FRotator(0.0f, -90.0f, 0.0f));

	bUseControllerRotationYaw = false;
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->bOrientRotationToMovement = false;
	Movement->MaxWalkSpeed = 380.0f;

	AIControllerClass = ABossArenaAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeFinder(TEXT("/Engine/BasicShapes/Cone.Cone"));

	PlaceholderBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaceholderBody"));
	PlaceholderBody->SetupAttachment(GetCapsuleComponent());
	PlaceholderBody->SetStaticMesh(CylinderFinder.Object);
	PlaceholderBody->SetRelativeScale3D(FVector(2.4f, 2.4f, 4.5f));
	PlaceholderBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	PlaceholderFacing = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaceholderFacing"));
	PlaceholderFacing->SetupAttachment(GetCapsuleComponent());
	PlaceholderFacing->SetStaticMesh(ConeFinder.Object);
	PlaceholderFacing->SetRelativeLocationAndRotation(FVector(150.0f, 0.0f, 120.0f), FRotator(-90.0f, 0.0f, 0.0f));
	PlaceholderFacing->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.2f));
	PlaceholderFacing->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ThreatComponent = CreateDefaultSubobject<UThreatComponent>(TEXT("Threat"));
	PhaseComponent = CreateDefaultSubobject<UBossPhaseComponent>(TEXT("Phases"));
	BossAttributes = CreateDefaultSubobject<UBossAttributeSet>(TEXT("BossAttributes"));

	HardEnrageAbility = UGA_Boss_HardEnrage::StaticClass();

	FBossPhaseDefinition Phase1;
	Phase1.PhaseName = TEXT("Phase 1 - The Warden Wakes");
	Phase1.HealthThreshold = 1.0f;
	Phase1.GlobalCooldown = 2.0f;
	Phase1.AbilityKit = { UGA_Boss_ConeSwipe::StaticClass(), UGA_Boss_Meteor::StaticClass(), UGA_Boss_DonutSlam::StaticClass() };

	FBossPhaseDefinition Phase2;
	Phase2.PhaseName = TEXT("Phase 2 - Ashen Fury");
	Phase2.HealthThreshold = 0.7f;
	Phase2.GlobalCooldown = 1.8f;
	Phase2.TransitionAbility = UGA_Boss_PhaseTransition::StaticClass();
	Phase2.AbilityKit = { UGA_Boss_ConeSwipe::StaticClass(), UGA_Boss_Meteor::StaticClass(), UGA_Boss_RotatingBeam::StaticClass(),
		UGA_Boss_ExpandingCircle::StaticClass(), UGA_Boss_SummonAdds::StaticClass() };

	FBossPhaseDefinition Phase3;
	Phase3.PhaseName = TEXT("Phase 3 - Cataclysm");
	Phase3.HealthThreshold = 0.4f;
	Phase3.GlobalCooldown = 1.4f;
	Phase3.TransitionAbility = UGA_Boss_PhaseTransition::StaticClass();
	Phase3.AbilityKit = { UGA_Boss_ConeSwipe::StaticClass(), UGA_Boss_MeteorRain::StaticClass(), UGA_Boss_DonutSlam::StaticClass(),
		UGA_Boss_RotatingBeam::StaticClass(), UGA_Boss_ExpandingCircle::StaticClass(), UGA_Boss_SummonAdds::StaticClass() };

	PhaseComponent->Phases = { Phase1, Phase2, Phase3 };
}

void ABossCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABossCharacter, CastInfo);
	DOREPLIFETIME(ABossCharacter, bEncounterActive);
	DOREPLIFETIME(ABossCharacter, EncounterStartServerTime);
	DOREPLIFETIME(ABossCharacter, bSoftEnraged);
	DOREPLIFETIME(ABossCharacter, bHardEnraged);
}

void ABossCharacter::GiveDefaultAbilities()
{
	Super::GiveDefaultAbilities();

	TSet<UClass*> Granted;
	auto Grant = [this, &Granted](const TSubclassOf<UGameplayAbility>& AbilityClass)
	{
		if (AbilityClass && !Granted.Contains(AbilityClass.Get()))
		{
			Granted.Add(AbilityClass.Get());
			AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(AbilityClass, 1, INDEX_NONE, this));
		}
	};

	for (const FBossPhaseDefinition& Phase : PhaseComponent->Phases)
	{
		for (const TSubclassOf<UGameplayAbility>& AbilityClass : Phase.AbilityKit)
		{
			Grant(AbilityClass);
		}
		Grant(Phase.TransitionAbility);
	}
	Grant(HardEnrageAbility);
}

void ABossCharacter::StartEncounter(int32 NumPlayers)
{
	if (!HasAuthority() || bEncounterActive || !IsAlive())
	{
		return;
	}

	const float MaxHealth = BaseMaxHealth + HealthPerExtraPlayer * FMath::Max(0, NumPlayers - 1);
	AbilitySystemComponent->SetNumericAttributeBase(UBossAttributeSet::GetMaxHealthAttribute(), MaxHealth);
	AbilitySystemComponent->SetNumericAttributeBase(UBossAttributeSet::GetHealthAttribute(), MaxHealth);
	AbilitySystemComponent->SetNumericAttributeBase(UBossAttributeSet::GetDamageAttribute(), 1.0f);
	AbilitySystemComponent->SetNumericAttributeBase(UBossAttributeSet::GetEnrageTimerAttribute(), HardEnrageTime);

	bEncounterActive = true;
	bSoftEnraged = false;
	bHardEnraged = false;
	EncounterStartServerTime = UBossArenaAoELibrary::GetServerTime(this);

	PhaseComponent->StartPhases(AbilitySystemComponent);
}

float ABossCharacter::GetEncounterElapsed() const
{
	return bEncounterActive ? float(UBossArenaAoELibrary::GetServerTime(this) - EncounterStartServerTime) : 0.0f;
}

float ABossCharacter::GetEnrageTimerRemaining() const
{
	return BossAttributes ? BossAttributes->GetEnrageTimer() : 0.0f;
}

void ABossCharacter::DebugAdvanceEncounter(float Seconds)
{
	if (HasAuthority() && bEncounterActive)
	{
		EncounterStartServerTime -= Seconds;
	}
}

void ABossCharacter::DebugSetHealthPercent(float Percent)
{
	if (HasAuthority() && IsAlive())
	{
		const float NewHealth = FMath::Clamp(Percent, 0.01f, 1.0f) * GetMaxHealth();
		AbilitySystemComponent->SetNumericAttributeBase(UBossAttributeSet::GetHealthAttribute(), NewHealth);
	}
}

void ABossCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!HasAuthority() || !bEncounterActive || !IsAlive())
	{
		return;
	}

	const float Elapsed = GetEncounterElapsed();

	EnrageAttributeAccumulator += DeltaSeconds;
	if (EnrageAttributeAccumulator >= 0.25f)
	{
		EnrageAttributeAccumulator = 0.0f;
		AbilitySystemComponent->SetNumericAttributeBase(UBossAttributeSet::GetEnrageTimerAttribute(), FMath::Max(0.0f, HardEnrageTime - Elapsed));
	}

	if (!bSoftEnraged && Elapsed >= SoftEnrageTime)
	{
		TriggerSoftEnrage();
	}
	if (!bHardEnraged && Elapsed >= HardEnrageTime)
	{
		TriggerHardEnrage();
	}
}

void ABossCharacter::TriggerSoftEnrage()
{
	bSoftEnraged = true;
	AbilitySystemComponent->SetNumericAttributeBase(UBossAttributeSet::GetDamageAttribute(), SoftEnrageDamageMultiplier);
	if (ABossArenaGameState* GameState = GetWorld()->GetGameState<ABossArenaGameState>())
	{
		GameState->BroadcastRaidWarning(TEXT("Kharzul grows furious! (damage and AoE size increased)"), FLinearColor(1.0f, 0.2f, 0.1f), 5.0f);
	}
}

void ABossCharacter::TriggerHardEnrage()
{
	bHardEnraged = true;
	const FGameplayTagContainer BossAbilityTags(BossArenaTags::Ability_Boss);
	AbilitySystemComponent->CancelAbilities(&BossAbilityTags);
	if (ABossArenaGameState* GameState = GetWorld()->GetGameState<ABossArenaGameState>())
	{
		GameState->BroadcastRaidWarning(TEXT("HARD ENRAGE - the arena burns!"), FLinearColor(1.0f, 0.0f, 0.0f), 6.0f);
	}
}

void ABossCharacter::GetActiveKit(TArray<TSubclassOf<UGameplayAbility>>& OutKit, float& OutGlobalCooldown) const
{
	OutKit.Reset();
	OutGlobalCooldown = 2.0f;

	if (bHardEnraged)
	{
		if (HardEnrageAbility)
		{
			OutKit.Add(HardEnrageAbility);
		}
		OutGlobalCooldown = 0.5f;
		return;
	}

	if (const FBossPhaseDefinition* Phase = PhaseComponent->GetCurrentPhase())
	{
		OutKit = Phase->AbilityKit;
		OutGlobalCooldown = Phase->GlobalCooldown;
	}
}

void ABossCharacter::StartCast(const FString& CastName, float Duration, bool bInterruptible)
{
	if (!HasAuthority())
	{
		return;
	}
	const double Now = UBossArenaAoELibrary::GetServerTime(this);
	CastInfo.CastName = CastName;
	CastInfo.StartServerTime = Now;
	CastInfo.EndServerTime = Now + Duration;
	CastInfo.Duration = Duration;
	CastInfo.bInterruptible = bInterruptible;
	CastInfo.bCasting = true;
	CastInfo.bWasInterrupted = false;
}

void ABossCharacter::EndCast(bool bInterrupted)
{
	if (!HasAuthority() || !CastInfo.bCasting)
	{
		return;
	}
	CastInfo.bCasting = false;
	CastInfo.bWasInterrupted = bInterrupted;
	CastInfo.EndServerTime = UBossArenaAoELibrary::GetServerTime(this);
}

void ABossCharacter::HandleDamageTaken(float Damage, AActor* DamageInstigator, bool bFatal)
{
	ThreatComponent->AddThreat(DamageInstigator, Damage);

	if (!bEncounterActive)
	{
		if (ABossArenaGameMode* GameMode = GetWorld()->GetAuthGameMode<ABossArenaGameMode>())
		{
			GameMode->StartEncounter();
		}
	}

	Super::HandleDamageTaken(Damage, DamageInstigator, bFatal);
}

void ABossCharacter::Die()
{
	EndCast(false);
	Super::Die();
	bEncounterActive = false;
}

void ABossCharacter::OnDeathStateChanged()
{
	Super::OnDeathStateChanged();
	if (bIsDead && PlaceholderBody)
	{
		PlaceholderBody->SetRelativeLocationAndRotation(FVector(0.0f, 0.0f, -150.0f), FRotator(80.0f, 0.0f, 0.0f));
		PlaceholderFacing->SetVisibility(false);
	}
}
