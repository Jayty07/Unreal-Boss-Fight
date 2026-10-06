#include "Warrior/Abilities/GA_LeapSlam.h"

#include "Abilities/Tasks/AbilityTask_ApplyRootMotionJumpForce.h"
#include "BossArenaGameplayTags.h"
#include "GAS/BossArenaGameplayEffects.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GA_LeapSlam)

UGA_LeapSlam::UGA_LeapSlam()
{
	FGameplayTagContainer Tags = GetAssetTags();
	Tags.AddTag(BossArenaTags::Ability_Warrior_LeapSlam);
	SetAssetTags(Tags);
	ActivationOwnedTags.AddTag(BossArenaTags::State_Leaping);
	CooldownGameplayEffectClass = UGE_Cooldown_LeapSlam::StaticClass();

	AimShape = FAoEShape::MakeCircle(400.0f);
	AimOffset = FVector(LeapDistance, 0.0f, 0.0f);
	BaseDamage = 500.0f;
	RageOnHit = 10.0f;
	CooldownDuration = 12.0f;
	MontageSlot = EWarriorMontage::LeapSlam;
}

void UGA_LeapSlam::ExecuteAbility()
{
	if (!CommitOrCancel())
	{
		return;
	}

	bLanded = false;
	FaceAim();
	PlayWarriorMontage(MontageSlot);
	ShowAimPreview(AimShape, FVector(LeapDistance, 0.0f, 0.0f));

	UAbilityTask_ApplyRootMotionJumpForce* Jump = UAbilityTask_ApplyRootMotionJumpForce::ApplyRootMotionJumpForce(this, NAME_None, GetAimRotation(), LeapDistance,
		LeapHeight, LeapDuration, LeapDuration * 0.5f, true, ERootMotionFinishVelocityMode::SetVelocity, FVector::ZeroVector, 0.0f, nullptr, nullptr);
	Jump->OnFinish.AddDynamic(this, &UGA_LeapSlam::OnLanded);
	Jump->ReadyForActivation();

	// Safety net if the landing callback never fires (e.g. blocked by geometry mid-air).
	WaitThen(LeapDuration + 1.0f, GET_FUNCTION_NAME_CHECKED(UGA_LeapSlam, OnLanded));
}

void UGA_LeapSlam::OnLanded()
{
	if (bLanded)
	{
		return;
	}
	bLanded = true;
	HideAimPreview();
	ResolveAoE(AimShape, GetFeetLocation(), GetAimRotation(), BaseDamage, false);
	FinishAbility();
}
