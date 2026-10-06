#include "BossArenaCharacterBase.h"

#include "AbilitySystemComponent.h"
#include "Attributes/BossArenaAttributeSetBase.h"
#include "Components/CapsuleComponent.h"
#include "Game/BossArenaGameMode.h"
#include "GAS/BossArenaGameplayEffects.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BossArenaCharacterBase)

ABossArenaCharacterBase::ABossArenaCharacterBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystem"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	SetNetUpdateFrequency(60.0f);
}

UAbilitySystemComponent* ABossArenaCharacterBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ABossArenaCharacterBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABossArenaCharacterBase, bIsDead);
}

void ABossArenaCharacterBase::BeginPlay()
{
	Super::BeginPlay();
	InitAbilitySystem();
}

void ABossArenaCharacterBase::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	InitAbilitySystem();
}

void ABossArenaCharacterBase::InitAbilitySystem()
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	AbilitySystemComponent->InitAbilityActorInfo(this, this);

	if (HasAuthority() && !bAbilitiesGiven)
	{
		bAbilitiesGiven = true;
		GiveDefaultAbilities();
	}

	OnAbilitySystemInitialized();
}

void ABossArenaCharacterBase::GiveDefaultAbilities()
{
	for (const TSubclassOf<UGameplayAbility>& AbilityClass : DefaultAbilities)
	{
		if (AbilityClass)
		{
			AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(AbilityClass, 1, INDEX_NONE, this));
		}
	}
}

float ABossArenaCharacterBase::GetHealth() const
{
	bool bFound = false;
	const float Value = AbilitySystemComponent ? AbilitySystemComponent->GetGameplayAttributeValue(UBossArenaAttributeSetBase::GetHealthAttribute(), bFound) : 0.0f;
	return bFound ? Value : 0.0f;
}

float ABossArenaCharacterBase::GetMaxHealth() const
{
	bool bFound = false;
	const float Value = AbilitySystemComponent ? AbilitySystemComponent->GetGameplayAttributeValue(UBossArenaAttributeSetBase::GetMaxHealthAttribute(), bFound) : 0.0f;
	return bFound ? Value : 0.0f;
}

float ABossArenaCharacterBase::GetHealthPercent() const
{
	const float Max = GetMaxHealth();
	return Max > 0.0f ? FMath::Clamp(GetHealth() / Max, 0.0f, 1.0f) : 0.0f;
}

FString ABossArenaCharacterBase::GetCombatName() const
{
	return CombatName.IsEmpty() ? GetName() : CombatName;
}

void ABossArenaCharacterBase::HandleDamageTaken(float Damage, AActor* DamageInstigator, bool bFatal)
{
	MulticastCombatText(Damage, EBossArenaCombatText::Damage);
	if (bFatal)
	{
		Die();
	}
}

void ABossArenaCharacterBase::NotifyDamageNegated()
{
	MulticastCombatText(0.0f, EBossArenaCombatText::Immune);
}

void ABossArenaCharacterBase::Die()
{
	if (!HasAuthority() || bIsDead)
	{
		return;
	}

	bIsDead = true;

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->CancelAllAbilities();
		FGameplayEffectContextHandle Context = AbilitySystemComponent->MakeEffectContext();
		AbilitySystemComponent->ApplyGameplayEffectToSelf(GetDefault<UGE_BossArenaDead>(), 1.0f, Context);
	}

	OnDeathStateChanged();

	if (ABossArenaGameMode* GameMode = GetWorld()->GetAuthGameMode<ABossArenaGameMode>())
	{
		GameMode->NotifyCharacterDied(this);
	}
}

void ABossArenaCharacterBase::OnRep_IsDead()
{
	OnDeathStateChanged();
}

void ABossArenaCharacterBase::OnDeathStateChanged()
{
	if (!bIsDead)
	{
		return;
	}

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
		Movement->DisableMovement();
	}
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
}

void ABossArenaCharacterBase::MulticastCombatText_Implementation(float Amount, EBossArenaCombatText Kind)
{
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	FBossArenaCombatTextEntry Entry;
	Entry.SpawnTime = GetWorld()->GetTimeSeconds();
	Entry.HorizontalOffset = FMath::FRandRange(-40.0f, 40.0f);
	switch (Kind)
	{
	case EBossArenaCombatText::Damage:
		Entry.Text = FString::Printf(TEXT("%d"), FMath::RoundToInt(Amount));
		Entry.Color = Team == EBossArenaTeam::Players ? FLinearColor(1.0f, 0.3f, 0.3f) : FLinearColor(1.0f, 0.9f, 0.3f);
		break;
	case EBossArenaCombatText::Immune:
		Entry.Text = TEXT("IMMUNE");
		Entry.Color = FLinearColor(0.4f, 0.9f, 1.0f);
		break;
	case EBossArenaCombatText::Interrupted:
		Entry.Text = TEXT("INTERRUPTED");
		Entry.Color = FLinearColor(0.8f, 0.5f, 1.0f);
		break;
	case EBossArenaCombatText::Taunted:
		Entry.Text = TEXT("TAUNTED");
		Entry.Color = FLinearColor(1.0f, 0.6f, 0.1f);
		break;
	}

	CombatTexts.Add(Entry);
	if (CombatTexts.Num() > 12)
	{
		CombatTexts.RemoveAt(0);
	}
}
