#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "BossArenaHUD.generated.h"

class ABossCharacter;
class AWarriorCharacter;
class UFont;

/**
 * Asset-free canvas HUD: boss HP/cast bar, phase indicator + threshold ticks, enrage timer, threat meter,
 * raid warnings, player HP/rage, party frames, hotbar with cooldown sweeps (dodge highlighted), add
 * nameplates and floating combat text. A UMG version can replace it by reading the same getters.
 */
UCLASS()
class BOSSARENA_API ABossArenaHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

	UPROPERTY(EditAnywhere, Category = "HUD")
	float UIScale = 1.0f;

protected:
	void DrawBossFrame(ABossCharacter* Boss);
	void DrawCastBar(ABossCharacter* Boss, float Y);
	void DrawThreatMeter(ABossCharacter* Boss);
	void DrawRaidWarnings();
	void DrawPlayerFrame(AWarriorCharacter* Warrior);
	void DrawPartyFrames(AWarriorCharacter* LocalWarrior);
	void DrawHotbar(AWarriorCharacter* Warrior);
	void DrawWorldOverlays();
	void DrawEncounterBanner();

	void DrawBar(float X, float Y, float W, float H, float Fraction, const FLinearColor& Fill, const FLinearColor& Back = FLinearColor(0.0f, 0.0f, 0.0f, 0.6f));
	void DrawBorder(float X, float Y, float W, float H, float Thickness, const FLinearColor& Color);
	void DrawTextCentered(const FString& Text, const FLinearColor& Color, float CenterX, float Y, UFont* Font, float Scale = 1.0f, bool bShadow = true);
	void DrawTextShadowed(const FString& Text, const FLinearColor& Color, float X, float Y, UFont* Font, float Scale = 1.0f);

	float S() const;
};
