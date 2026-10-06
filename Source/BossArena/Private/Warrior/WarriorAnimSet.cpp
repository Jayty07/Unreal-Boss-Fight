#include "Warrior/WarriorAnimSet.h"

#include "Animation/AnimMontage.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(WarriorAnimSet)

UAnimMontage* UWarriorAnimSet::GetMontage(EWarriorMontage Slot) const
{
	const TObjectPtr<UAnimMontage>* Found = Montages.Find(Slot);
	return Found ? Found->Get() : nullptr;
}
