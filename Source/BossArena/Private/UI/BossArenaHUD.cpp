#include "UI/BossArenaHUD.h"

#include "AbilitySystemComponent.h"
#include "AoE/BossArenaAoELibrary.h"
#include "Boss/BossAddCharacter.h"
#include "Boss/BossCharacter.h"
#include "Boss/BossPhaseComponent.h"
#include "Boss/ThreatComponent.h"
#include "BossArenaGameplayTags.h"
#include "Components/CapsuleComponent.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "EngineUtils.h"
#include "Game/BossArenaGameState.h"
#include "GameFramework/PlayerController.h"
#include "Warrior/Abilities/WarriorGameplayAbility.h"
#include "Warrior/WarriorCharacter.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BossArenaHUD)

namespace
{
	UFont* SmallFont() { return GEngine ? GEngine->GetSmallFont() : nullptr; }
	UFont* MediumFont() { return GEngine ? GEngine->GetMediumFont() : nullptr; }
	UFont* LargeFont() { return GEngine ? GEngine->GetLargeFont() : nullptr; }

	FString FormatTime(float Seconds)
	{
		const int32 Total = FMath::Max(0, FMath::CeilToInt(Seconds));
		return FString::Printf(TEXT("%d:%02d"), Total / 60, Total % 60);
	}
}

float ABossArenaHUD::S() const
{
	return UIScale * FMath::Max(0.6f, Canvas ? Canvas->ClipY / 1080.0f : 1.0f);
}

void ABossArenaHUD::DrawBar(float X, float Y, float W, float H, float Fraction, const FLinearColor& Fill, const FLinearColor& Back)
{
	DrawRect(Back, X, Y, W, H);
	DrawRect(Fill, X, Y, W * FMath::Clamp(Fraction, 0.0f, 1.0f), H);
}

void ABossArenaHUD::DrawBorder(float X, float Y, float W, float H, float T, const FLinearColor& Color)
{
	DrawRect(Color, X, Y, W, T);
	DrawRect(Color, X, Y + H - T, W, T);
	DrawRect(Color, X, Y, T, H);
	DrawRect(Color, X + W - T, Y, T, H);
}

void ABossArenaHUD::DrawTextShadowed(const FString& Text, const FLinearColor& Color, float X, float Y, UFont* Font, float Scale)
{
	DrawText(Text, FLinearColor(0.0f, 0.0f, 0.0f, Color.A * 0.8f), X + 1.5f, Y + 1.5f, Font, Scale);
	DrawText(Text, Color, X, Y, Font, Scale);
}

void ABossArenaHUD::DrawTextCentered(const FString& Text, const FLinearColor& Color, float CenterX, float Y, UFont* Font, float Scale, bool bShadow)
{
	float W = 0.0f, H = 0.0f;
	GetTextSize(Text, W, H, Font, Scale);
	if (bShadow)
	{
		DrawTextShadowed(Text, Color, CenterX - W * 0.5f, Y, Font, Scale);
	}
	else
	{
		DrawText(Text, Color, CenterX - W * 0.5f, Y, Font, Scale);
	}
}

void ABossArenaHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!Canvas)
	{
		return;
	}

	const ABossArenaGameState* GameState = GetWorld()->GetGameState<ABossArenaGameState>();
	ABossCharacter* Boss = GameState ? GameState->GetBoss() : nullptr;
	AWarriorCharacter* Warrior = PlayerOwner ? Cast<AWarriorCharacter>(PlayerOwner->GetPawn()) : nullptr;

	DrawWorldOverlays();
	if (Boss)
	{
		DrawBossFrame(Boss);
		DrawThreatMeter(Boss);
	}
	DrawRaidWarnings();
	DrawEncounterBanner();
	if (Warrior)
	{
		DrawPlayerFrame(Warrior);
		DrawHotbar(Warrior);
	}
	DrawPartyFrames(Warrior);
}

void ABossArenaHUD::DrawBossFrame(ABossCharacter* Boss)
{
	const float Scale = S();
	const float W = 640.0f * Scale;
	const float H = 26.0f * Scale;
	const float X = (Canvas->ClipX - W) * 0.5f;
	const float Y = 24.0f * Scale;

	DrawTextCentered(Boss->GetCombatName(), FLinearColor(1.0f, 0.85f, 0.6f), Canvas->ClipX * 0.5f, Y - 2.0f * Scale, MediumFont(), Scale);
	const float BarY = Y + 22.0f * Scale;

	const float Fraction = Boss->GetHealthPercent();
	DrawBar(X, BarY, W, H, Fraction, Boss->IsSoftEnraged() ? FLinearColor(0.9f, 0.1f, 0.05f) : FLinearColor(0.75f, 0.12f, 0.1f));
	DrawBorder(X, BarY, W, H, 2.0f, FLinearColor(0.1f, 0.1f, 0.1f));

	// Phase threshold ticks (70% / 40%).
	if (const UBossPhaseComponent* Phases = Boss->GetPhaseComponent())
	{
		for (int32 Index = 1; Index < Phases->GetPhases().Num(); ++Index)
		{
			const float TickX = X + W * Phases->GetPhases()[Index].HealthThreshold;
			DrawRect(FLinearColor(1.0f, 0.85f, 0.2f), TickX - 1.5f, BarY - 4.0f * Scale, 3.0f, H + 8.0f * Scale);
		}
		const FString PhaseText = FString::Printf(TEXT("Phase %d / %d  -  %s"), Phases->GetCurrentPhaseIndex() + 1, Phases->GetPhases().Num(), *Phases->GetCurrentPhaseName());
		DrawTextCentered(PhaseText, FLinearColor(1.0f, 0.85f, 0.3f), Canvas->ClipX * 0.5f, BarY + H + 4.0f * Scale, SmallFont(), Scale * 1.1f);
	}

	DrawTextCentered(FString::Printf(TEXT("%.0f / %.0f  (%.1f%%)"), Boss->GetHealth(), Boss->GetMaxHealth(), Fraction * 100.0f), FLinearColor::White, Canvas->ClipX * 0.5f,
		BarY + 5.0f * Scale, SmallFont(), Scale);

	// Enrage timer on the right of the bar.
	FString EnrageText;
	FLinearColor EnrageColor = FLinearColor(0.85f, 0.85f, 0.85f);
	if (Boss->IsHardEnraged())
	{
		EnrageText = TEXT("HARD ENRAGE");
		EnrageColor = FLinearColor::Red;
	}
	else if (Boss->IsEncounterActive())
	{
		const float Elapsed = Boss->GetEncounterElapsed();
		if (Boss->IsSoftEnraged())
		{
			EnrageText = FString::Printf(TEXT("ENRAGED  |  Wipe in %s"), *FormatTime(Boss->GetHardEnrageTime() - Elapsed));
			EnrageColor = FLinearColor(1.0f, 0.35f, 0.2f);
		}
		else
		{
			EnrageText = FString::Printf(TEXT("Enrage in %s"), *FormatTime(Boss->GetSoftEnrageTime() - Elapsed));
		}
	}
	if (!EnrageText.IsEmpty())
	{
		DrawTextShadowed(EnrageText, EnrageColor, X + W + 12.0f * Scale, BarY + 3.0f * Scale, SmallFont(), Scale * 1.1f);
	}

	DrawCastBar(Boss, BarY + H + 26.0f * Scale);
}

void ABossArenaHUD::DrawCastBar(ABossCharacter* Boss, float Y)
{
	const FBossCastInfo& CastInfo = Boss->GetCastInfo();
	const double Now = UBossArenaAoELibrary::GetServerTime(this);
	const bool bRecentlyEnded = !CastInfo.bCasting && Now - CastInfo.EndServerTime < 1.0;
	if (!CastInfo.bCasting && !bRecentlyEnded)
	{
		return;
	}

	const float Scale = S();
	const float W = 420.0f * Scale;
	const float H = 18.0f * Scale;
	const float X = (Canvas->ClipX - W) * 0.5f;

	float Fraction = 1.0f;
	FLinearColor Fill = CastInfo.bInterruptible ? FLinearColor(0.6f, 0.25f, 1.0f) : FLinearColor(1.0f, 0.6f, 0.1f);
	FString Label = CastInfo.CastName;
	if (CastInfo.bCasting)
	{
		Fraction = CastInfo.Duration > 0.0f ? float((Now - CastInfo.StartServerTime) / CastInfo.Duration) : 1.0f;
		Label += FString::Printf(TEXT("  %.1fs"), FMath::Max(0.0, CastInfo.StartServerTime + CastInfo.Duration - Now));
		if (CastInfo.bInterruptible)
		{
			Label += TEXT("  [INTERRUPTIBLE]");
		}
	}
	else if (CastInfo.bWasInterrupted)
	{
		Fill = FLinearColor(0.4f, 0.4f, 0.4f);
		Label = TEXT("INTERRUPTED");
	}

	DrawBar(X, Y, W, H, Fraction, Fill);
	DrawBorder(X, Y, W, H, 1.5f, CastInfo.bInterruptible ? FLinearColor(0.85f, 0.6f, 1.0f) : FLinearColor(0.1f, 0.1f, 0.1f));
	DrawTextCentered(Label, FLinearColor::White, Canvas->ClipX * 0.5f, Y + 2.0f * Scale, SmallFont(), Scale);
}

void ABossArenaHUD::DrawThreatMeter(ABossCharacter* Boss)
{
	const UThreatComponent* Threat = Boss->GetThreatComponent();
	if (!Threat || Threat->GetThreatTable().Num() == 0)
	{
		return;
	}

	const float Scale = S();
	const float W = 220.0f * Scale;
	const float RowH = 18.0f * Scale;
	const float X = Canvas->ClipX - W - 20.0f * Scale;
	float Y = 140.0f * Scale;

	DrawTextShadowed(TEXT("Threat"), FLinearColor(1.0f, 0.8f, 0.4f), X, Y, SmallFont(), Scale * 1.1f);
	Y += RowH + 2.0f;

	const float TopThreat = FMath::Max(1.0f, Threat->GetThreatTable()[0].Threat);
	const int32 Rows = FMath::Min(5, Threat->GetThreatTable().Num());
	for (int32 Index = 0; Index < Rows; ++Index)
	{
		const FThreatEntry& Entry = Threat->GetThreatTable()[Index];
		const ABossArenaCharacterBase* Character = Cast<ABossArenaCharacterBase>(Entry.Actor);
		const bool bIsLocal = PlayerOwner && Entry.Actor == PlayerOwner->GetPawn();
		DrawBar(X, Y, W, RowH - 2.0f, Entry.Threat / TopThreat, bIsLocal ? FLinearColor(0.2f, 0.6f, 1.0f) : FLinearColor(0.5f, 0.5f, 0.55f));
		const FString Name = Character ? Character->GetCombatName() : TEXT("?");
		DrawTextShadowed(FString::Printf(TEXT("%d. %s  %.0f"), Index + 1, *Name, Entry.Threat), FLinearColor::White, X + 4.0f, Y + 1.0f, SmallFont(), Scale);
		Y += RowH;
	}
}

void ABossArenaHUD::DrawRaidWarnings()
{
	const ABossArenaGameState* GameState = GetWorld()->GetGameState<ABossArenaGameState>();
	if (!GameState)
	{
		return;
	}

	TArray<FRaidWarningEntry> Warnings;
	GameState->GetActiveRaidWarnings(Warnings);
	const double Now = GetWorld()->GetTimeSeconds();
	const float Scale = S();
	float Y = Canvas->ClipY * 0.24f;
	for (int32 Index = Warnings.Num() - 1; Index >= 0; --Index)
	{
		const FRaidWarningEntry& Entry = Warnings[Index];
		const float Age = float(Now - Entry.StartTime);
		const float Remaining = Entry.Duration - Age;
		FLinearColor Color = Entry.Color;
		Color.A = FMath::Clamp(Remaining / 0.5f, 0.0f, 1.0f);
		const float Pulse = Age < 0.3f ? 1.0f + (0.3f - Age) : 1.0f;
		DrawTextCentered(Entry.Text, Color, Canvas->ClipX * 0.5f, Y, LargeFont(), Scale * 1.15f * Pulse);
		Y += 40.0f * Scale;
	}
}

void ABossArenaHUD::DrawEncounterBanner()
{
	const ABossArenaGameState* GameState = GetWorld()->GetGameState<ABossArenaGameState>();
	if (GameState && GameState->GetEncounterState() == EBossEncounterState::Waiting)
	{
		DrawTextCentered(TEXT("Approach the boss (or hit it) to begin.  Console: StartFight / SetBossHealth 0.65 / SkipEncounterTime 475"),
			FLinearColor(0.8f, 0.8f, 0.8f, 0.8f), Canvas->ClipX * 0.5f, Canvas->ClipY - 24.0f * S(), SmallFont(), S());
	}
}

void ABossArenaHUD::DrawPlayerFrame(AWarriorCharacter* Warrior)
{
	const float Scale = S();
	const float W = 300.0f * Scale;
	const float H = 22.0f * Scale;
	const float X = 30.0f * Scale;
	const float Y = Canvas->ClipY - 150.0f * Scale;

	DrawTextShadowed(Warrior->GetCombatName(), FLinearColor(0.7f, 0.9f, 1.0f), X, Y - 22.0f * Scale, MediumFont(), Scale);

	const float HealthFraction = Warrior->GetHealthPercent();
	DrawBar(X, Y, W, H, HealthFraction, FLinearColor(0.15f, 0.75f, 0.2f));
	DrawBorder(X, Y, W, H, 1.5f, FLinearColor(0.05f, 0.05f, 0.05f));
	DrawTextCentered(FString::Printf(TEXT("%.0f / %.0f"), Warrior->GetHealth(), Warrior->GetMaxHealth()), FLinearColor::White, X + W * 0.5f, Y + 4.0f * Scale, SmallFont(), Scale);

	const float RageY = Y + H + 4.0f * Scale;
	const float MaxRage = FMath::Max(1.0f, Warrior->GetMaxRage());
	DrawBar(X, RageY, W, H * 0.7f, Warrior->GetRage() / MaxRage, FLinearColor(0.85f, 0.1f, 0.1f));
	DrawBorder(X, RageY, W, H * 0.7f, 1.5f, FLinearColor(0.05f, 0.05f, 0.05f));
	DrawTextCentered(FString::Printf(TEXT("Rage %.0f / %.0f"), Warrior->GetRage(), MaxRage), FLinearColor::White, X + W * 0.5f, RageY + 1.0f * Scale, SmallFont(), Scale * 0.9f);

	const UAbilitySystemComponent* ASC = Warrior->GetAbilitySystemComponent();
	float StatusY = RageY + H + 2.0f * Scale;
	if (ASC && ASC->HasMatchingGameplayTag(BossArenaTags::State_Invulnerable))
	{
		DrawTextShadowed(TEXT("DODGING - INVULNERABLE"), FLinearColor(0.4f, 0.95f, 1.0f), X, StatusY, SmallFont(), Scale * 1.1f);
		StatusY += 18.0f * Scale;
	}
	if (ASC && ASC->HasMatchingGameplayTag(BossArenaTags::State_Blocking))
	{
		DrawTextShadowed(TEXT("BLOCKING"), FLinearColor(0.9f, 0.9f, 0.5f), X, StatusY, SmallFont(), Scale * 1.1f);
		StatusY += 18.0f * Scale;
	}
	if (!Warrior->IsAlive())
	{
		DrawTextCentered(TEXT("YOU DIED"), FLinearColor(0.9f, 0.1f, 0.1f), Canvas->ClipX * 0.5f, Canvas->ClipY * 0.45f, LargeFont(), Scale * 2.0f);
	}
	if (Warrior->IsHoldToAimEnabled())
	{
		DrawTextShadowed(TEXT("Hold-to-aim ON (ToggleHoldToAim)"), FLinearColor(0.6f, 0.8f, 1.0f, 0.8f), X, StatusY, SmallFont(), Scale * 0.9f);
	}
}

void ABossArenaHUD::DrawPartyFrames(AWarriorCharacter* LocalWarrior)
{
	const float Scale = S();
	const float W = 180.0f * Scale;
	const float H = 14.0f * Scale;
	const float X = 30.0f * Scale;
	float Y = 140.0f * Scale;
	bool bHeader = false;

	for (TActorIterator<AWarriorCharacter> It(GetWorld()); It; ++It)
	{
		AWarriorCharacter* Member = *It;
		if (Member == LocalWarrior)
		{
			continue;
		}
		if (!bHeader)
		{
			DrawTextShadowed(TEXT("Party"), FLinearColor(0.7f, 0.9f, 1.0f), X, Y, SmallFont(), Scale * 1.1f);
			Y += 18.0f * Scale;
			bHeader = true;
		}
		DrawTextShadowed(Member->GetCombatName(), Member->IsAlive() ? FLinearColor::White : FLinearColor(0.5f, 0.5f, 0.5f), X, Y, SmallFont(), Scale);
		Y += 14.0f * Scale;
		DrawBar(X, Y, W, H, Member->GetHealthPercent(), FLinearColor(0.15f, 0.7f, 0.2f));
		DrawBar(X, Y + H, W, H * 0.4f, Member->GetRage() / FMath::Max(1.0f, Member->GetMaxRage()), FLinearColor(0.8f, 0.1f, 0.1f));
		Y += H * 1.4f + 8.0f * Scale;
	}
}

void ABossArenaHUD::DrawHotbar(AWarriorCharacter* Warrior)
{
	const UAbilitySystemComponent* ASC = Warrior->GetAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	TArray<const FWarriorHotbarSlot*> Slots;
	for (const FWarriorHotbarSlot& Slot : Warrior->GetHotbar())
	{
		if (Slot.bShowOnHotbar && Slot.AbilityClass)
		{
			Slots.Add(&Slot);
		}
	}
	if (Slots.Num() == 0)
	{
		return;
	}

	const float Scale = S();
	const float Size = 64.0f * Scale;
	const float Gap = 8.0f * Scale;
	const float TotalW = Slots.Num() * Size + (Slots.Num() - 1) * Gap;
	float X = (Canvas->ClipX - TotalW) * 0.5f;
	const float Y = Canvas->ClipY - Size - 40.0f * Scale;

	for (const FWarriorHotbarSlot* Slot : Slots)
	{
		const UWarriorGameplayAbility* Ability = Slot->AbilityClass->GetDefaultObject<UWarriorGameplayAbility>();
		float Duration = 0.0f;
		const float Remaining = Ability->GetCooldownRemaining(ASC, Duration);
		const bool bAffordable = Warrior->GetRage() + KINDA_SMALL_NUMBER >= Ability->GetRageCost();
		const bool bUsable = Ability->IsUsableNow(Warrior);
		const bool bIsDodge = Slot->InputID == EWarriorAbilityInput::Dodge;

		FLinearColor Back = bIsDodge ? FLinearColor(0.05f, 0.25f, 0.35f, 0.85f) : FLinearColor(0.12f, 0.12f, 0.14f, 0.85f);
		if (!bAffordable || !bUsable)
		{
			Back = FLinearColor(0.25f, 0.05f, 0.05f, 0.85f);
		}
		DrawRect(Back, X, Y, Size, Size);

		if (Remaining > 0.0f && Duration > 0.0f)
		{
			// Vertical cooldown sweep: dark overlay shrinks from top as the cooldown finishes.
			const float Fraction = FMath::Clamp(Remaining / Duration, 0.0f, 1.0f);
			DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.7f), X, Y + Size * (1.0f - Fraction), Size, Size * Fraction);
			DrawTextCentered(Remaining < 1.0f ? FString::Printf(TEXT("%.1f"), Remaining) : FString::Printf(TEXT("%.0f"), Remaining), FLinearColor::White, X + Size * 0.5f, Y + Size * 0.3f, MediumFont(), Scale);
		}

		const FLinearColor BorderColor = bIsDodge ? FLinearColor(0.3f, 0.9f, 1.0f) : FLinearColor(0.5f, 0.5f, 0.55f);
		DrawBorder(X, Y, Size, Size, bIsDodge ? 3.0f : 1.5f, Remaining <= 0.0f && bAffordable && bUsable ? BorderColor : BorderColor * 0.5f);

		DrawTextShadowed(Slot->KeyLabel, FLinearColor(1.0f, 0.9f, 0.5f), X + 4.0f, Y + 2.0f, SmallFont(), Scale);
		if (Ability->GetRageCost() > 0.0f)
		{
			DrawTextShadowed(FString::Printf(TEXT("%.0f"), Ability->GetRageCost()), FLinearColor(1.0f, 0.4f, 0.4f), X + Size - 22.0f * Scale, Y + 2.0f, SmallFont(), Scale * 0.9f);
		}
		DrawTextCentered(Slot->Label, FLinearColor::White, X + Size * 0.5f, Y + Size + 2.0f * Scale, SmallFont(), Scale * 0.9f);
		X += Size + Gap;
	}
}

void ABossArenaHUD::DrawWorldOverlays()
{
	const double Now = GetWorld()->GetTimeSeconds();
	const float Scale = S();
	const float TextLifetime = 1.3f;

	for (TActorIterator<ABossArenaCharacterBase> It(GetWorld()); It; ++It)
	{
		ABossArenaCharacterBase* Character = *It;
		const float HalfHeight = Character->GetCapsuleComponent() ? Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 90.0f;
		const FVector Head = Character->GetActorLocation() + FVector(0.0f, 0.0f, HalfHeight + 30.0f);
		const FVector Screen = Project(Head, false);
		const bool bOnScreen = Screen.Z > 0.0f;

		// Add nameplates.
		if (bOnScreen && Character->IsA<ABossAddCharacter>() && Character->IsAlive())
		{
			const float W = 90.0f * Scale;
			DrawTextCentered(Character->GetCombatName(), FLinearColor(1.0f, 0.6f, 0.4f), Screen.X, Screen.Y - 18.0f * Scale, SmallFont(), Scale * 0.9f);
			DrawBar(Screen.X - W * 0.5f, Screen.Y, W, 7.0f * Scale, Character->GetHealthPercent(), FLinearColor(0.8f, 0.2f, 0.1f));
		}

		Character->CombatTexts.RemoveAll([Now, TextLifetime](const FBossArenaCombatTextEntry& Entry) { return Now - Entry.SpawnTime > TextLifetime; });
		if (!bOnScreen)
		{
			continue;
		}
		for (const FBossArenaCombatTextEntry& Entry : Character->CombatTexts)
		{
			const float Age = float(Now - Entry.SpawnTime);
			FLinearColor Color = Entry.Color;
			Color.A = 1.0f - Age / TextLifetime;
			DrawTextCentered(Entry.Text, Color, Screen.X + Entry.HorizontalOffset * Scale, Screen.Y - (30.0f + Age * 60.0f) * Scale, MediumFont(), Scale * 1.1f);
		}
	}
}
