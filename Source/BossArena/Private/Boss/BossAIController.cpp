#include "Boss/BossAIController.h"

#include "AbilitySystemComponent.h"
#include "AoE/BossArenaAoELibrary.h"
#include "Boss/BossAddCharacter.h"
#include "Boss/BossCharacter.h"
#include "Boss/ThreatComponent.h"
#include "BossArenaGameplayTags.h"
#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BossAIController)

ABossArenaAIController::ABossArenaAIController()
{
	PrimaryActorTick.bCanEverTick = true;
	bSetControlRotationFromPawnOrientation = true;
}

void ABossArenaAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	NextAbilityTime = GetWorld()->GetTimeSeconds() + 1.5;
}

AActor* ABossArenaAIController::SelectTarget(ABossArenaCharacterBase* Self) const
{
	UThreatComponent* Threat = Self->FindComponentByClass<UThreatComponent>();
	if (AActor* Top = Threat ? Threat->GetTopThreatActor() : nullptr)
	{
		return Top;
	}

	TArray<ABossArenaCharacterBase*> Players;
	UBossArenaAoELibrary::GetLivingPlayers(this, Players);
	ABossArenaCharacterBase* Nearest = nullptr;
	float BestDistSq = TNumericLimits<float>::Max();
	for (ABossArenaCharacterBase* Player : Players)
	{
		const float DistSq = FVector::DistSquared2D(Player->GetActorLocation(), Self->GetActorLocation());
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			Nearest = Player;
		}
	}
	if (Nearest && Threat)
	{
		Threat->AddThreat(Nearest, 1.0f);
	}
	return Nearest;
}

void ABossArenaAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	ABossArenaCharacterBase* Self = Cast<ABossArenaCharacterBase>(GetPawn());
	UAbilitySystemComponent* ASC = Self ? Self->GetAbilitySystemComponent() : nullptr;
	if (!Self || !ASC || !Self->IsAlive() || !HasAuthority())
	{
		return;
	}

	TArray<TSubclassOf<UGameplayAbility>> Kit;
	float GlobalCooldown = 2.0f;
	float PreferredRange = AddPreferredRange;

	if (ABossCharacter* Boss = Cast<ABossCharacter>(Self))
	{
		if (!Boss->IsEncounterActive())
		{
			return;
		}
		Boss->GetActiveKit(Kit, GlobalCooldown);
		PreferredRange = BossPreferredRange;
	}
	else if (ABossAddCharacter* Add = Cast<ABossAddCharacter>(Self))
	{
		Add->GetAbilityKit(Kit, GlobalCooldown);
	}

	const double Now = GetWorld()->GetTimeSeconds();

	if (ASC->HasMatchingGameplayTag(BossArenaTags::State_Casting))
	{
		bWasCasting = true;
		return;
	}
	if (bWasCasting)
	{
		bWasCasting = false;
		NextAbilityTime = Now + GlobalCooldown;
	}

	AActor* Target = SelectTarget(Self);
	if (!Target)
	{
		return;
	}

	FVector ToTarget = Target->GetActorLocation() - Self->GetActorLocation();
	ToTarget.Z = 0.0f;
	const float Distance = ToTarget.Size();
	if (Distance > 1.0f)
	{
		const FRotator Desired = ToTarget.Rotation();
		Self->SetActorRotation(FMath::RInterpTo(Self->GetActorRotation(), FRotator(0.0f, Desired.Yaw, 0.0f), DeltaSeconds, TurnRate));
	}
	if (Distance > PreferredRange)
	{
		Self->AddMovementInput(ToTarget.GetSafeNormal(), 1.0f);
	}

	if (Now < NextAbilityTime || Kit.Num() == 0)
	{
		return;
	}

	// Random pick without immediate repeats; abilities may refuse (recast interval), so try them all.
	TArray<TSubclassOf<UGameplayAbility>> Candidates = Kit;
	if (Candidates.Num() > 1)
	{
		Candidates.Remove(LastAbility);
	}
	for (int32 Index = Candidates.Num() - 1; Index > 0; --Index)
	{
		Candidates.Swap(Index, FMath::RandRange(0, Index));
	}
	if (Kit.Num() > 1 && LastAbility)
	{
		Candidates.Add(LastAbility);
	}

	for (const TSubclassOf<UGameplayAbility>& AbilityClass : Candidates)
	{
		if (AbilityClass && ASC->TryActivateAbilityByClass(AbilityClass))
		{
			LastAbility = AbilityClass;
			NextAbilityTime = Now + GlobalCooldown;
			return;
		}
	}
	NextAbilityTime = Now + 0.5;
}
