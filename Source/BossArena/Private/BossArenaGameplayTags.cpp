#include "BossArenaGameplayTags.h"

namespace BossArenaTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Damage, "Data.Damage", "SetByCaller: raw damage before mitigation.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Cooldown, "Data.Cooldown", "SetByCaller: cooldown duration in seconds.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Rage, "Data.Rage", "SetByCaller: rage delta.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Invulnerable, "State.Invulnerable", "Incoming blockable damage is negated (dodge i-frames).");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Dodging, "State.Dodging", "Warrior is mid-dodge.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Blocking, "State.Blocking", "Warrior is holding block.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Dead, "State.Dead", "Character is dead.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Casting, "State.Casting", "Boss/add is casting a telegraphed AoE.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Leaping, "State.Leaping", "Warrior is airborne in a leaping slam.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Damage_Unblockable, "Damage.Unblockable", "Ignores invulnerability, block and armor (hard enrage).");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Interrupt, "Event.Interrupt", "Sent to enemies hit by an interrupting AoE.");

	UE_DEFINE_GAMEPLAY_TAG(Ability_Warrior_Attack, "Ability.Warrior.Attack");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Warrior_Cleave, "Ability.Warrior.Cleave");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Warrior_Whirlwind, "Ability.Warrior.Whirlwind");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Warrior_LeapSlam, "Ability.Warrior.LeapSlam");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Warrior_Shockwave, "Ability.Warrior.Shockwave");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Warrior_GroundRend, "Ability.Warrior.GroundRend");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Warrior_Taunt, "Ability.Warrior.Taunt");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Warrior_Execute, "Ability.Warrior.Execute");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Warrior_Dodge, "Ability.Warrior.Dodge");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Warrior_Block, "Ability.Warrior.Block");

	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Warrior_Whirlwind, "Cooldown.Warrior.Whirlwind");
	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Warrior_LeapSlam, "Cooldown.Warrior.LeapSlam");
	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Warrior_Shockwave, "Cooldown.Warrior.Shockwave");
	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Warrior_GroundRend, "Cooldown.Warrior.GroundRend");
	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Warrior_Taunt, "Cooldown.Warrior.Taunt");
	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Warrior_Execute, "Cooldown.Warrior.Execute");
	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Warrior_Dodge, "Cooldown.Warrior.Dodge");

	UE_DEFINE_GAMEPLAY_TAG(Ability_Boss, "Ability.Boss");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Boss_Transition, "Ability.Boss.Transition");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Boss_HardEnrage, "Ability.Boss.HardEnrage");
}
