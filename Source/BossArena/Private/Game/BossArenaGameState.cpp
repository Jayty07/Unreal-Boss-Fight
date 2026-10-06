#include "Game/BossArenaGameState.h"

#include "Boss/BossCharacter.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BossArenaGameState)

void ABossArenaGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABossArenaGameState, Boss);
	DOREPLIFETIME(ABossArenaGameState, EncounterState);
}

void ABossArenaGameState::BroadcastRaidWarning(const FString& Text, FLinearColor Color, float Duration)
{
	if (HasAuthority() && !Text.IsEmpty())
	{
		MulticastRaidWarning(Text, Color, Duration);
	}
}

void ABossArenaGameState::MulticastRaidWarning_Implementation(const FString& Text, FLinearColor Color, float Duration)
{
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	const double Now = GetWorld()->GetTimeSeconds();
	RaidWarnings.RemoveAll([Now](const FRaidWarningEntry& Entry) { return Now > Entry.StartTime + Entry.Duration; });

	FRaidWarningEntry& Entry = RaidWarnings.AddDefaulted_GetRef();
	Entry.Text = Text;
	Entry.Color = Color;
	Entry.StartTime = Now;
	Entry.Duration = Duration;
	if (RaidWarnings.Num() > 3)
	{
		RaidWarnings.RemoveAt(0);
	}
	OnRaidWarning.Broadcast(Entry);
}

void ABossArenaGameState::GetActiveRaidWarnings(TArray<FRaidWarningEntry>& OutWarnings) const
{
	const double Now = GetWorld()->GetTimeSeconds();
	for (const FRaidWarningEntry& Entry : RaidWarnings)
	{
		if (Now <= Entry.StartTime + Entry.Duration)
		{
			OutWarnings.Add(Entry);
		}
	}
}
