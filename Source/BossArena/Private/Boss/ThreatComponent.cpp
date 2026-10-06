#include "Boss/ThreatComponent.h"

#include "BossArenaCharacterBase.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ThreatComponent)

UThreatComponent::UThreatComponent()
{
	SetIsReplicatedByDefault(true);
}

void UThreatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UThreatComponent, ThreatTable);
	DOREPLIFETIME(UThreatComponent, FixateTarget);
}

bool UThreatComponent::IsValidThreatTarget(const AActor* Actor)
{
	const ABossArenaCharacterBase* Character = Cast<ABossArenaCharacterBase>(Actor);
	return IsValid(Character) && Character->IsAlive();
}

FThreatEntry& UThreatComponent::FindOrAdd(AActor* Source)
{
	for (FThreatEntry& Entry : ThreatTable)
	{
		if (Entry.Actor == Source)
		{
			return Entry;
		}
	}
	FThreatEntry& NewEntry = ThreatTable.AddDefaulted_GetRef();
	NewEntry.Actor = Source;
	return NewEntry;
}

void UThreatComponent::SortAndPrune()
{
	ThreatTable.RemoveAll([](const FThreatEntry& Entry) { return !IsValid(Entry.Actor); });
	ThreatTable.Sort([](const FThreatEntry& A, const FThreatEntry& B) { return A.Threat > B.Threat; });
}

void UThreatComponent::AddThreat(AActor* Source, float Amount)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !IsValidThreatTarget(Source) || Source == GetOwner())
	{
		return;
	}
	FindOrAdd(Source).Threat += FMath::Max(0.0f, Amount) * ThreatPerDamage;
	SortAndPrune();
}

void UThreatComponent::Taunt(AActor* Source, float BurstThreat, float FixateDuration)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !IsValidThreatTarget(Source))
	{
		return;
	}

	float TopThreat = 0.0f;
	for (const FThreatEntry& Entry : ThreatTable)
	{
		TopThreat = FMath::Max(TopThreat, Entry.Threat);
	}

	FThreatEntry& Entry = FindOrAdd(Source);
	Entry.Threat = FMath::Max(Entry.Threat, TopThreat * 1.1f) + BurstThreat;
	FixateTarget = Source;
	FixateEndTime = GetWorld()->GetTimeSeconds() + FixateDuration;
	SortAndPrune();
}

void UThreatComponent::ResetThreat()
{
	ThreatTable.Reset();
	FixateTarget = nullptr;
	FixateEndTime = 0.0;
}

AActor* UThreatComponent::GetTopThreatActor() const
{
	if (GetOwner() && GetOwner()->HasAuthority() && IsValidThreatTarget(FixateTarget) && GetWorld()->GetTimeSeconds() < FixateEndTime)
	{
		return FixateTarget;
	}

	for (const FThreatEntry& Entry : ThreatTable)
	{
		if (IsValidThreatTarget(Entry.Actor))
		{
			return Entry.Actor;
		}
	}
	return nullptr;
}

float UThreatComponent::GetThreat(const AActor* Source) const
{
	for (const FThreatEntry& Entry : ThreatTable)
	{
		if (Entry.Actor == Source)
		{
			return Entry.Threat;
		}
	}
	return 0.0f;
}
