#include "Explosion.h"

#include "Engine.h"

#include "FeatureData.h"
#include "FeatureSmoke.h"
#include "Renderer.h"
#include "Map.h"
#include "Fov.h"
#include "ActorPlayer.h"
#include "Log.h"
#include "Postmortem.h"

void ExplosionMaker::renderExplosion(const BasicData* data,
                                     bool reach[MAP_X_CELLS][MAP_Y_CELLS]) {
  eng->renderer->drawMapAndInterface();

  int x0 = max(1, data->x0 + 1);
  int y0 = max(1, data->y0 + 1);
  int x1 = min(MAP_X_CELLS - 2, data->x1 - 1);
  int y1 = min(MAP_Y_CELLS - 2, data->y1 - 1);
  for(int x = x0; x <= x1; x++) {
    for(int y = y0; y <= y1; y++) {
      if(eng->map->playerVision[x][y]) {
        if(reach[x][y]) {
          eng->renderer->drawGlyph(
            '*', panel_map, Pos(x, y), clrYellow, true, clrBlack);
        }
      }
    }
  }

  eng->renderer->updateScreen();
  eng->sleep(eng->config->delayExplosion / 2);

  x0 = max(1, data->x0);
  y0 = max(1, data->y0);
  x1 = min(MAP_X_CELLS - 2, data->x1);
  y1 = min(MAP_Y_CELLS - 2, data->y1);
  for(int x = x0; x <= x1; x++) {
    for(int y = y0; y <= y1; y++) {
      if(eng->map->playerVision[x][y]) {
        if(reach[x][y]) {
          if(
            x == data->x0 || x == data->x1 ||
            y == data->y0 || y == data->y1) {
            eng->renderer->drawGlyph(
              '*', panel_map, Pos(x, y), clrRedLgt, true, clrBlack);
          }
        }
      }
    }
  }
  eng->renderer->updateScreen();
}

void ExplosionMaker::renderExplosionWithColorOverride(
  const BasicData* data, const SDL_Color clr,
  bool reach[MAP_X_CELLS][MAP_Y_CELLS]) {
  eng->renderer->drawMapAndInterface();

  const int X0 = max(1, data->x0);
  const int Y0 = max(1, data->y0);
  const int X1 = min(MAP_X_CELLS - 2, data->x1);
  const int Y1 = min(MAP_Y_CELLS - 2, data->y1);
  for(int x = X0; x <= X1; x++) {
    for(int y = Y0; y <= Y1; y++) {
      if(eng->map->playerVision[x][y]) {
        if(reach[x][y]) {
          eng->renderer->drawGlyph(
            '*', panel_map, Pos(x, y), clr, true, clrBlack);
        }
      }
    }
  }
  eng->renderer->updateScreen();
}

void ExplosionMaker::runExplosion(
  const Pos& origin, const Sfx_t sfx, const bool DO_EXPLOSION_DMG,
  Prop* const prop, const bool OVERRIDE_EXPLOSION_RENDERING,
  const SDL_Color colorOverride) {

  BasicData data(origin, width, height);

  //Set up explosion reach array
  bool blockers[MAP_X_CELLS][MAP_Y_CELLS];
  eng->mapTests->makeShootBlockerFeaturesArray(blockers);
  bool reach[MAP_X_CELLS][MAP_Y_CELLS];
  eng->basicUtils->resetBoolArray(reach, false);

  for(int x = max(1, data.x0); x <= min(MAP_X_CELLS - 2, data.x1); x++) {
    for(int y = max(1, data.y0); y <= min(MAP_Y_CELLS - 2, data.y1); y++) {
      reach[x][y] = eng->fov->checkCell(
                      blockers, Pos(x, y), origin, false) &&
                    !blockers[x][y];
    }
  }
  reach[origin.x][origin.y] = true;

  //TODO Explosion sound msg should be parameterized with an enumerated type
//  if(DO_EXPLOSION_DMG) {
  Sound snd("I hear an explosion!", sfx, true, origin, true, true);
  eng->soundEmitter->emitSound(snd);
//    eng->audio->playSound(audio_explosion);
//  }

  //Render
  if(eng->config->isTilesMode) {
    bool forbiddenRenderCells[MAP_X_CELLS][MAP_Y_CELLS];
    eng->basicUtils->resetBoolArray(forbiddenRenderCells, true);
    for(int y = 1; y < MAP_Y_CELLS - 2; y++) {
      for(int x = 1; x < MAP_X_CELLS - 2; x++) {
        forbiddenRenderCells[x][y] = reach[x][y] == false ||
                                     eng->map->playerVision[x][y] == false;
      }
    }

    if(OVERRIDE_EXPLOSION_RENDERING) {
      eng->renderer->drawBlastAnimationAtField(origin, (data.x1 - data.x0) / 2,
          forbiddenRenderCells, colorOverride, colorOverride);
    } else {
      eng->renderer->drawBlastAnimationAtField(origin, (data.x1 - data.x0) / 2,
          forbiddenRenderCells, clrYellow, clrRedLgt);
    }
  } else {
    if(OVERRIDE_EXPLOSION_RENDERING) {
      renderExplosionWithColorOverride(&data, colorOverride, reach);
    } else {
      renderExplosion(&data, reach);
    }
  }

  //Delay before applying damage and effects
//  eng->sleep(eng->config->delayExplosion);

  //Do damage, apply effect
  const int DMG_ROLLS = 5;
  const int DMG_SIDES = 6;
  const int DMG_PLUS = 10;
  Actor* currentActor;
  for(int x = max(1, data.x0); x <= min(MAP_X_CELLS - 2, data.x1); x++) {
    for(int y = max(1, data.y0); y <= min(MAP_Y_CELLS - 2, data.y1); y++) {

      if(DO_EXPLOSION_DMG) {
        if(eng->mapTests->isCellsAdj(Pos(x, y), origin, false)) {
          eng->map->switchToDestroyedFeatAt(Pos(x, y));

          if(eng->map->featuresStatic[x][y]->getId() == feature_door) {
            eng->map->switchToDestroyedFeatAt(Pos(x, y));
          }
        }
      }

      if(reach[x][y] == true) {
        const int CHEBY_DIST =
          eng->basicUtils->chebyshevDist(origin.x, origin.y, x, y);
        const int EXPLOSION_DMG_AT_DIST =
          eng->dice(DMG_ROLLS - CHEBY_DIST, DMG_SIDES) + DMG_PLUS;

        //Damage actor, or apply property?
        const unsigned int SIZE_OF_ACTOR_LOOP = eng->gameTime->getLoopSize();
        for(unsigned int i = 0; i < SIZE_OF_ACTOR_LOOP; i++) {
          currentActor = eng->gameTime->getActorAtElement(i);
          if(currentActor->pos.x == x && currentActor->pos.y == y) {

            if(DO_EXPLOSION_DMG) {
              if(currentActor == eng->player) {
                eng->log->addMsg("I am hit by an explosion!",
                                 clrMessageBad);
              }
              currentActor->hit(EXPLOSION_DMG_AT_DIST, dmgType_physical, true);
            }

            if(
              prop != NULL &&
              currentActor->deadState == actorDeadState_alive) {
              PropHandler* const propHandler = currentActor->getPropHandler();
              Prop* propCpy = propHandler->makePropFromId(
                                prop->getId(), propTurnsSpecified,
                                prop->turnsLeft_);
              propHandler->tryApplyProp(propCpy);
            }
          }
        }

        if(DO_EXPLOSION_DMG == true) {
          if(eng->dice.percentile() < 55) {
            eng->featureFactory->spawnFeatureAt(
              feature_smoke, Pos(x, y),
              new SmokeSpawnData(1 + eng->dice(1, 3)));
          }
        }
      }
    }
  }

  //Set graphics back to normal
  eng->player->updateFov();
  eng->renderer->drawMapAndInterface();

  if(prop != NULL) {
    delete prop;
  }
}

void ExplosionMaker::runSmokeExplosion(const Pos& origin,
                                       const bool SMALL_RADIUS) {
  const int RADIUS = SMALL_RADIUS == true ? 3 : width;
  BasicData data(origin, RADIUS, RADIUS);

  //Set up explosion reach array
  bool reach[MAP_X_CELLS][MAP_Y_CELLS];
  eng->basicUtils->resetBoolArray(reach, false);

  //There are two scans for blocking objects made, pretty unoptimised, but it doesn't matter.

  bool blockers[MAP_X_CELLS][MAP_Y_CELLS];
  eng->mapTests->makeShootBlockerFeaturesArray(blockers);

  for(int x = max(1, data.x0); x <= min(MAP_X_CELLS - 2, data.x1); x++) {
    for(int y = max(1, data.y0); y <= min(MAP_Y_CELLS - 2, data.y1); y++) {
      //As opposed to the explosion reach, the smoke explosion must not reach into walls and other solid objects
      reach[x][y] = blockers[x][y] == false && eng->fov->checkCell(
                      blockers, Pos(x, y), origin, false);
    }
  }
  reach[origin.x][origin.y] = true;

  for(int x = max(1, data.x0); x <= min(MAP_X_CELLS - 2, data.x1); x++) {
    for(int y = max(1, data.y0); y <= min(MAP_Y_CELLS - 2, data.y1); y++) {
      if(reach[x][y] == true) {
        eng->featureFactory->spawnFeatureAt(
          feature_smoke, Pos(x, y), new SmokeSpawnData(16 + eng->dice(1, 6)));
      }
    }
  }

  //Draw map
  eng->player->updateFov();
  eng->renderer->drawMapAndInterface();

  //Delay
  eng->sleep(eng->config->delayExplosion);
}
