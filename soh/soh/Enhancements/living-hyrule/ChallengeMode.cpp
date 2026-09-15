#include "ChallengeMode.h"
#include "ChallengePolicy.h"

#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/ShipInit.hpp"
#include "soh/cvar_prefixes.h"

#include <cstdint>
#include <unordered_set>
#include <imgui.h>
#include <libultraship/bridge/consolevariablebridge.h>
#include <ship/Context.h>
#include <ship/window/Window.h>
#include <ship/window/gui/Gui.h>

extern "C" {
#include "functions.h"
#include "macros.h"
#include "variables.h"
extern PlayState* gPlayState;
}

namespace LivingHyrule {
namespace {

constexpr const char* kChallengeCVar = CVAR_ENHANCEMENT("LivingHyruleChallenge");
std::unordered_set<const Actor*> consideredHearts;
uint32_t heartOrdinal = 0;

ChallengeContext CurrentContext() {
    ChallengeContext context;
    context.enabled = CVarGetInteger(kChallengeCVar, 0) != 0;
    context.supportedAdventure = IS_VANILLA || IS_MASTER_QUEST;
    context.realSave = GameInteractor::IsSaveLoaded(false);
    if (!context.realSave) {
        return context;
    }
    context.normalScene = !IS_CUTSCENE_LAYER;
    context.gameplayActive = !GameInteractor::IsGameplayPaused() && gPlayState->pauseCtx.debugState == 0 &&
                             gPlayState->gameOverCtx.state == GAMEOVER_INACTIVE &&
                             gPlayState->transitionTrigger == TRANS_TRIGGER_OFF &&
                             gPlayState->transitionMode == TRANS_MODE_OFF && gSaveContext.health > 0;
    return context;
}

ChallengeDamageConflicts CurrentDamageConflicts() {
    return {
        CVarGetInteger(CVAR_ENHANCEMENT("DamageMult"), 0) != 0,
        GameInteractor_DefenseModifier() != 0,
        GameInteractor_OneHitKOActive() != 0,
        CVarGetInteger(CVAR_CHEAT("InfiniteHealth"), 0) != 0,
        CVarGetInteger(CVAR_ENHANCEMENT("PermanentHeartLoss"), 0) != 0,
    };
}

bool HasDropOverride() {
    return CVarGetInteger(CVAR_ENHANCEMENT("NoHeartDrops"), 0) != 0 ||
           CVarGetInteger(CVAR_ENHANCEMENT("NoRandomDrops"), 0) != 0;
}

// Check identity against live lists before reading a collision source or its
// parent. Projectile parent pointers may outlive the enemy that fired them.
const Actor* FindLiveActor(const Actor* candidate) {
    if (candidate == nullptr) {
        return nullptr;
    }
    for (int category = 0; category < ACTORCAT_MAX; ++category) {
        for (const Actor* actor = gPlayState->actorCtx.actorLists[category].head; actor != nullptr;
             actor = actor->next) {
            if (actor == candidate) {
                return actor->update != nullptr ? actor : nullptr;
            }
        }
    }
    return nullptr;
}

bool IsEnemy(const Actor* actor) {
    return actor != nullptr && actor->id != ACTOR_EN_FIRE_ROCK && actor->id != ACTOR_EN_ENCOUNT2 &&
           (actor->category == ACTORCAT_ENEMY || actor->category == ACTORCAT_BOSS || actor->id == ACTOR_EN_TORCH2);
}

bool IsEnemyCollision(const Player* player) {
    if ((player->cylinder.base.acFlags & AC_HIT) == 0) {
        return false;
    }
    const Actor* source = FindLiveActor(player->cylinder.base.ac);
    return IsEnemy(source) || (source != nullptr && IsEnemy(FindLiveActor(source->parent)));
}

void BeforePlayerUpdate(void* actorRef, bool* shouldUpdate) {
    const auto context = CurrentContext();
    if (!*shouldUpdate || !IsChallengeActive(context)) {
        return;
    }
    auto* player = GET_PLAYER(gPlayState);
    if (actorRef != player || player->actor.category != ACTORCAT_PLAYER) {
        return;
    }
    const bool boost = ShouldBoostChallengeDamage(context, CurrentDamageConflicts(), IsEnemyCollision(player),
                                                  IsChallengeInvulnerableOnDamageFrame(player->invincibilityTimer),
                                                  (player->shieldQuad.base.acFlags & AC_BOUNCED) != 0,
                                                  player->knockbackType != PLAYER_KNOCKBACK_NONE);
    // This is the pending collision damage consumed by the ordinary player
    // update, then reset by Actor_UpdateAll. No second Health_ChangeBy call:
    // shield handling, Double Defense, fairy revival, and death run normally.
    // Scripted hits, grabs, falls, voids, and damage over time are unchanged.
    player->actor.colChkInfo.damage = ScaleChallengeCollisionDamage(player->actor.colChkInfo.damage, boost);
}

ChallengeHeartDrop DescribeHeart(const EnItem00* item) {
    return {
        item->actor.params == ITEM00_HEART,
        item->actor.room == -1 && item->unk_15A > 0,
        item->collectibleFlag == 0,
        (static_cast<uint16_t>(item->ogParams) & 0x8000u) != 0,
        item->unk_154 > 0 || item->actor.parent != nullptr,
    };
}

void BeforeCollectibleUpdate(void* actorRef, bool* shouldUpdate) {
    const auto context = CurrentContext();
    if (!*shouldUpdate || !IsChallengeActive(context) || HasDropOverride()) {
        return;
    }
    auto* item = static_cast<EnItem00*>(actorRef);
    const auto drop = DescribeHeart(item);
    if (!IsChallengeHeartCandidate(drop) || !consideredHearts.insert(&item->actor).second) {
        return;
    }
    // Drop helpers assign lifetime and room after Actor_Spawn returns. Inspect
    // them before the first eligible update, never inside OnActorInit/Spawn.
    const uint32_t ordinal = heartOrdinal++;
    if (ShouldSuppressChallengeHeart(context, false, drop, gSaveContext.health, ordinal)) {
        Actor_Kill(&item->actor);
        *shouldUpdate = false;
    }
}

void ClearDropTracking() {
    consideredHearts.clear();
    heartOrdinal = 0;
}

void RegisterChallengeMode() {
    static bool registered = false;
    if (registered) {
        return;
    }
    registered = true;
    GameInteractor::Instance->RegisterGameHookForID<GameInteractor::ShouldActorUpdate>(ACTOR_PLAYER,
                                                                                       BeforePlayerUpdate);
    GameInteractor::Instance->RegisterGameHookForID<GameInteractor::ShouldActorUpdate>(ACTOR_EN_ITEM00,
                                                                                       BeforeCollectibleUpdate);
    GameInteractor::Instance->RegisterGameHookForID<GameInteractor::OnActorDestroy>(
        ACTOR_EN_ITEM00, [](void* actor) { consideredHearts.erase(static_cast<Actor*>(actor)); });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSceneInit>([](int16_t) { ClearDropTracking(); });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayDestroy>(ClearDropTracking);
}

static RegisterShipInitFunc initFunc(RegisterChallengeMode);

} // namespace

void DrawChallengeControls() {
    ImGui::Separator();
    ImGui::TextUnformatted("Combat and preparation");
    bool enabled = CVarGetInteger(kChallengeCVar, 0) != 0;
    if (ImGui::Checkbox("Dangerous combat and scarce recovery", &enabled)) {
        CVarSetInteger(kChallengeCVar, enabled ? 1 : 0);
        Ship::Context::GetRawInstance()->GetWindow()->GetGui()->SaveConsoleVariablesNextFrame();
    }
    ImGui::TextWrapped("Double damage from enemy and boss collision hits. Keep one of every two temporary loose "
                       "heart drops; emergency hearts at one heart or less are kept.");
    ImGui::TextWrapped("Placed hearts, heart pieces, containers and fairies keep their normal rules. Double Defense "
                       "still helps. Falls, scripted damage and enemy health are unchanged.");
    if (!enabled) {
        ImGui::TextWrapped("Challenge mode is off. This preference applies to normal adventures and Master Quest, "
                           "independently of your bank account.");
        return;
    }

    const auto conflicts = CurrentDamageConflicts();
    if (conflicts.infiniteHealth) {
        ImGui::TextWrapped("Extra combat damage is suspended while Infinite Health is enabled. Your cheat setting "
                           "is kept; turn it off in Cheats to experience the combat challenge.");
    } else if (conflicts.oneHitKO) {
        ImGui::TextWrapped("One-hit KO takes priority. Challenge mode adds no extra combat damage.");
    } else if (conflicts.damageMultiplier) {
        ImGui::TextWrapped("Your existing Damage Multiplier takes priority. Challenge mode adds no extra combat "
                           "damage while that setting differs from normal.");
    } else if (conflicts.defenseModifier) {
        ImGui::TextWrapped("An external defense modifier takes priority. Challenge mode adds no extra combat damage "
                           "until that modifier ends.");
    } else if (conflicts.permanentHeartLoss) {
        ImGui::TextWrapped("Permanent Heart Loss takes priority. Challenge mode adds no extra combat damage while "
                           "that setting is enabled.");
    }
    if (HasDropOverride()) {
        ImGui::TextWrapped("Your No Heart Drops or No Random Drops setting takes priority over scarce recovery.");
    }
    if (CVarGetInteger(CVAR_ENHANCEMENT("HyperEnemies"), 0) != 0 ||
        CVarGetInteger(CVAR_ENHANCEMENT("HyperBosses"), 0) != 0) {
        ImGui::TextWrapped("Your faster enemy or boss setting also remains active.");
    }
    if (GameInteractor::IsSaveLoaded(false) && !(IS_VANILLA || IS_MASTER_QUEST)) {
        ImGui::TextWrapped("Challenge mode is inactive in this adventure type.");
    }
}

} // namespace LivingHyrule
