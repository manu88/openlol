#include "game_char_inventory.h"
#include "game_rules.h"
#include "geometry.h"
#include "render.h"
#include "renderer.h"
#include "ui.h"

typedef struct {
  Point hand1;
  Point shield1;
  Point hand2;
  Point shield2;
  Point helm;
  Point armor;
  Point necklace;
  Point braces;
  Point shoes;
  Point ringRight;
  Point ringLeft;
} InventoryLayout;

#define INVALID_ITEM -1, -1

static InventoryLayout humanLayout = {
    {117, 25},      //
    {192, 25},      //
    {INVALID_ITEM}, //
    {INVALID_ITEM}, //
    {117, 1},       //
    {117, 49},      //
    {192, 1},       //
    {192, 49},      //
    {117, 97},      //
    {191, 78},      //
    {126, 78},      //
};

static InventoryLayout ulineLayout = {
    {233 / 2, 104 / 2}, {383 / 2, 103 / 2}, {INVALID_ITEM},
    {INVALID_ITEM},     {233 / 2, 8 / 2},   {233 / 2, 56 / 2},
    {383 / 2, 8 / 2},   {383 / 2, 56 / 2},  {INVALID_ITEM},
    {253 / 2, 156 / 2}, {383 / 2, 156 / 2},
};

static InventoryLayout thomgogLayout = {
    {117, 25}, {192, 25}, {117, 73}, {192, 73},      {117, 1},       {117, 49},
    {192, 1},  {192, 49}, {117, 97}, {INVALID_ITEM}, {INVALID_ITEM},
};

static InventoryLayout *layouts[] = {

    NULL,         // invalid
    &humanLayout, // MALE
    &humanLayout, // TIM
    &humanLayout, // Female
    &humanLayout, // Dracoid
    &ulineLayout, &thomgogLayout,
};

static void renderCharItem(GameContext *gameCtx, const SAVCharacter *character,
                           int invType, CharItemIndex itemIndex) {
  const InventoryLayout *layout = layouts[invType];
  const GameObject *obj = &gameCtx->itemsInGame[character->items[itemIndex]];
  uint16_t frameId =
      GameContextGetItemSHPFrameIndex(gameCtx, obj->itemPropertyIndex);
  SHPFrame itemFrame = {0};
  SHPHandleGetFrame(&gameCtx->itemShapes, &itemFrame, frameId);
  SHPFrameGetImageData(&itemFrame);

  Point pt;

  size_t bIndex = 4;
  switch (itemIndex) {
  case CharItemIndex_Hand:
    pt = layout->hand1;
    break;
  case CharItemIndex_Shield:
    pt = layout->shield1;
    break;
  case CharItemIndex_Hand2:
    pt = layout->hand2;
    break;
  case CharItemIndex_Shield2:
    pt = layout->shield2;
    break;
  case CharItemIndex_Helm:
    pt = layout->helm;
    break;
  case CharItemIndex_Armor:
    pt = layout->armor;
    break;
  case CharItemIndex_Necklace:
    pt = layout->necklace;
    break;
  case CharItemIndex_Braces:
    pt = layout->braces;
    break;
  case CharItemIndex_Shoes:
    pt = layout->shoes;
    break;
  case CharItemIndex_RingLeft:
    pt = layout->ringLeft;
    bIndex = 5;
    break;
  case CharItemIndex_RingRight:
    pt = layout->ringRight;
    bIndex = 5;
    break;
  }

  if (pt.x < 0 && pt.y < 0) {
    return;
  }
  SHPFrame bFrame = {0};

  SHPHandleGetFrame(&gameCtx->gameShapes, &bFrame, bIndex);
  SHPFrameGetImageData(&bFrame);
  drawSHPFrame(gameCtx->pixBuf, &bFrame, pt.x, pt.y, gameCtx->defaultPalette);
  SHPFrameRelease(&bFrame);
  drawSHPFrame(gameCtx->pixBuf, &itemFrame, pt.x, pt.y,
               gameCtx->defaultPalette);
  SHPFrameRelease(&itemFrame);
}

static void renderCharInventoryExperience(GameContext *gameCtx,
                                          const SAVCharacter *c,
                                          SkillIndex index) {
  const int x = 266;
  const int maxW = 34;
  float percent = 0;
  int y = 0;
  int32_t exp = 0;
  int32_t req = 0;
  GameRuleGetCharacterExpPoints(c, index, &exp, &req);
  percent = (float)exp / (float)req;
  switch (index) {
  case SkillIndex_Fighter:
    y = 64;
    break;
  case SkillIndex_Rogue:
    y = 74;
    break;
  case SkillIndex_Mage:
    y = 84;
    break;
  }
  int w = maxW * percent;
  UIFillRect(gameCtx->pixBuf, x, y, w, 5, (SDL_Color){255, 0, 0});
}

void renderCharInventory(GameContext *gameCtx) {
  const SAVCharacter *character = gameCtx->chars + gameCtx->selectedChar;

  int id = character->id < 0 ? -character->id : character->id;
  int invType = inventoryTypeForId[id];
  GameContextLoadBackgroundInventoryIfNeeded(gameCtx, id);

  const CPSImage *background =
      &gameCtx->inventoryBackgrounds[inventoryTypeForId[id]];
  renderCPSAt(gameCtx->pixBuf, background->data, background->imageSize,
              background->palette, INVENTORY_SCREEN_X, INVENTORY_SCREEN_Y,
              INVENTORY_SCREEN_W, INVENTORY_SCREEN_H, 320, 200);

  for (int i = 0; i < 11; i++) {
    if (character->items[i] == 0) {
      continue;
    }
    renderCharItem(gameCtx, character, invType, i);
  }
  UISetStyle(UIStyle_Inventory);

  // char name
  UIRenderText(&gameCtx->defaultFont, gameCtx->pixBuf, 250, 10, 50,
               character->name);

  char c[16] = "";
  // force
  int y = 24;
  GameContextGetString(gameCtx, 0X4014, c, sizeof(c));
  UIRenderText(&gameCtx->defaultFont, gameCtx->pixBuf, 218, y, 98, c);

  snprintf(c, 16, "%i", GameRuleGetCharacterMight(character));
  UIRenderText(&gameCtx->defaultFont, gameCtx->pixBuf, 307, y, 20, c);

  // protection
  y = 36;
  GameContextGetString(gameCtx, 0X4015, c, sizeof(c));
  UIRenderText(&gameCtx->defaultFont, gameCtx->pixBuf, 218, y, 98, c);

  snprintf(c, 16, "%i", GameRuleGetCharacterProtection(character));
  UIRenderText(&gameCtx->defaultFont, gameCtx->pixBuf, 307, y, 20, c);

  // Fighter stats
  y = 62;
  GameContextGetString(gameCtx, 0X4016, c, sizeof(c));
  UIRenderText(&gameCtx->defaultFont, gameCtx->pixBuf, 218, y, 40, c);

  snprintf(c, 16, "%i", GameRuleGetCharacterSkillFight(character));
  UIRenderText(&gameCtx->defaultFont, gameCtx->pixBuf, 307, y, 20, c);

  renderCharInventoryExperience(gameCtx, character, SkillIndex_Fighter);

  // Rogue stats
  y = 72;
  GameContextGetString(gameCtx, 0X4017, c, sizeof(c));
  UIRenderText(&gameCtx->defaultFont, gameCtx->pixBuf, 218, y, 40, c);

  snprintf(c, 16, "%i", GameRuleGetCharacterSkillRogue(character));
  UIRenderText(&gameCtx->defaultFont, gameCtx->pixBuf, 307, y, 20, c);

  renderCharInventoryExperience(gameCtx, character, SkillIndex_Rogue);
  // Mage stats
  y = 82;
  GameContextGetString(gameCtx, 0X4018, c, sizeof(c));
  UIRenderText(&gameCtx->defaultFont, gameCtx->pixBuf, 218, y, 40, c);

  snprintf(c, 16, "%i", GameRuleGetCharacterSkillMage(character));
  UIRenderText(&gameCtx->defaultFont, gameCtx->pixBuf, 307, y, 20, c);

  renderCharInventoryExperience(gameCtx, character, SkillIndex_Mage);

  // exit button
  GameContextGetString(gameCtx, STR_EXIT_INDEX, c, sizeof(c));
  UIRenderText(&gameCtx->defaultFont, gameCtx->pixBuf, 277, 104, 50, c);
}

static void renderInventorySlot(GameContext *gameCtx, uint8_t slot,
                                uint16_t frameId) {
  assert(slot <= 9);
  SHPFrame frame = {0};
  SHPHandleGetFrame(&gameCtx->itemShapes, &frame, frameId);
  SHPFrameGetImageData(&frame);
  drawSHPFrame(gameCtx->pixBuf, &frame,
               UI_INVENTORY_BUTTON_X + (UI_MENU_INV_BUTTON_W * (1 + slot)) + 2,
               UI_INVENTORY_BUTTON_Y, gameCtx->defaultPalette);
  SHPFrameRelease(&frame);
}

void renderInventoryStrip(GameContext *gameCtx) {
  for (int i = 0; i < 9; i++) {
    uint16_t index = (gameCtx->inventoryIndex + i) % INVENTORY_SIZE;
    uint16_t gameObjIndex = gameCtx->inventory[index];
    if (gameObjIndex == 0) {
      continue;
    }
    const GameObject *obj = gameCtx->itemsInGame + gameObjIndex;
    renderInventorySlot(
        gameCtx, i,
        GameContextGetItemSHPFrameIndex(gameCtx, obj->itemPropertyIndex));
  }
}
