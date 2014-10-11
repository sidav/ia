#ifndef RENDER_H
#define RENDER_H

#include <vector>
#include <iostream>

#include <SDL_video.h>

#include "CmnData.h"
#include "GameTime.h"
#include "Config.h"
#include "Art.h"

struct Projectile;

enum class Panel {screen, map, charLines, log};

namespace Render {

extern CellRenderData renderArray[MAP_W][MAP_H];
extern CellRenderData renderArrayNoActors[MAP_W][MAP_H];
extern SDL_Surface*   screenSurface;
extern SDL_Surface*   mainMenuLogoSurface;

void init();
void cleanup();

void drawMapAndInterface(const bool SHOULD_UPDATE_SCREEN = true);

void updateScreen();

void clearScreen();

void drawTile(const TileId tile, const Panel panel, const Pos& pos,
              const Clr& clr, const Clr& bgClr = clrBlack);

void drawGlyph(const char GLYPH, const Panel panel, const Pos& pos,
               const Clr& clr, const bool DRAW_BG_CLR = true,
               const Clr& bgClr = clrBlack);

void drawText(const std::string& str, const Panel panel, const Pos& pos,
              const Clr& clr, const Clr& bgClr = clrBlack);

int drawTextCentered(const std::string& str, const Panel panel, const Pos& pos,
                     const Clr& clr, const Clr& bgClr = clrBlack,
                     const bool IS_PIXEL_POS_ADJ_ALLOWED = true);

void coverCellInMap(const Pos& pos);

void coverPanel(const Panel panel);

void coverArea(const Panel panel, const Rect& area);
void coverArea(const Panel panel, const Pos& pos, const Pos& dims);

void coverAreaPixel(const Pos& pixelPos, const Pos& pixelDims);

void drawRectangleSolid(const Pos& pixelPos, const Pos& pixelDims,
                        const Clr& clr);

void drawLineHor(const Pos& pixelPos, const int W, const Clr& clr);

void drawLineVer(const Pos& pixelPos, const int H, const Clr& clr);

void drawMarker(const Pos& p, const std::vector<Pos>& trail,
                const int EFFECTIVE_RANGE = -1);

void drawBlastAnimAtField(const Pos& centerPos, const int RADIUS,
                          bool forbiddenCells[MAP_W][MAP_H],
                          const Clr& colorInner,
                          const Clr& colorOuter);

void drawBlastAnimAtPositions(const std::vector<Pos>& positions,
                              const Clr& color);

void drawBlastAnimAtPositionsWithPlayerVision(
  const std::vector<Pos>& positions, const Clr& clr);

void drawMainMenuLogo(const int Y_POS);

void drawProjectiles(std::vector<Projectile*>& projectiles,
                     const bool SHOULD_DRAW_MAP_BEFORE);

void drawPopupBox(const Rect& area, const Panel panel = Panel::screen,
                  const Clr& clr = clrPopupBox, const bool COVER_AREA = false);

//E.g. item and spell descriptions
void drawDescrBox(const std::vector<StrAndClr>& lines);

void applySurface(const Pos& pixelPos, SDL_Surface* const src,
                  SDL_Rect* clip = nullptr);

void drawMap();

} //Render

#endif
