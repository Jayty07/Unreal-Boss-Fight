#include "Warrior/Abilities/GA_CleaveCombo.h"

#include "BossArenaGameplayTags.h"
#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GA_CleaveCombo)

UGA_CleaveCombo::UGA_CleaveCombo()
{
	FGameplayTagContainer Tags = GetAssetTags();
	Tags.AddTag(BossArenaTags::Ability_Warrior_Cleave);
	SetAssetTags(Tags);
	bSupportsHoldToAim = false;

	auto MakeSwing = [](float Radius, float Angle, float Damage, float Impact, float Recovery, float Rage, EWarriorMontage Montage)
	{
		FWarriorComboSwing Swing;
		Swing.Shape = FAoEShape::MakeCone(Radius, Angle);
		Swing.Damage = Damage;
		Swing.ImpactDelay = Impact;
		Swing.Recovery = Recovery;
		Swing.RageOnHit = Rage;
		Swing.Montage = Montage;
		return Swing;
	};
	Swings.Add(MakeSwing(350.0f, 100.0f, 220.0f, 0.22f, 0.25f, 6.0f, EWarriorMontage::Cleave1));
	Swings.Add(MakeSwing(380.0f, 120.0f, 260.0f, 0.22f, 0.25f, 6.0f, EWarriorMontage::Cleave2));
	Swings.Add(MakeSwing(450.0f, 160.0f, 420.0f, 0.35f, 0.45f, 12.0f, EWarriorMontage::Cleave3));

	AimShape = Swings[0].Shape;
}

void UGA_CleaveCombo::ExecuteAbility()
{
	if (Swings.Num() == 0 || !CommitOrCancel())
	{
		return;
	}

	const double Now = GetWorld()->GetTimeSeconds();
	if (Now - LastSwingEndTime > ComboWindow)
	{
		NextSwingIndex = 0;
	}
	ActiveSwingIndex = NextSwingIndex % Swings.Num();
	NextSwingIndex = (ActiveSwingIndex + 1) % Swings.Num();

	const FWarriorComboSwing& Swing = Swings[ActiveSwingIndex];
	FaceAim();
	PlayWarriorMontage(Swing.Montage);
	ShowAimPreview(Swing.Shape, FVector::ZeroVector);
	WaitThen(Swing.ImpactDelay, GET_FUNCTION_NAME_CHECKED(UGA_CleaveCombo, OnSwingImpact));
}

void UGA_CleaveCombo::OnSwingImpact()
{
	const FWarriorComboSwing& Swing = Swings[ActiveSwingIndex];
	HideAimPreview();
	RageOnHit = Swing.RageOnHit;
	ResolveAoE(Swing.Shape, GetFeetLocation(), GetAimRotation(), Swing.Damage, false);
	WaitThen(Swing.Recovery, GET_FUNCTION_NAME_CHECKED(UGA_CleaveCombo, OnSwingRecovered));
}

void UGA_CleaveCombo::OnSwingRecovered()
{
	LastSwingEndTime = GetWorld()->GetTimeSeconds();
	FinishAbility();
}
