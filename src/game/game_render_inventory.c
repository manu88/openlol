#include "game_render_inventory.h"
#include "game_rules.h"
#include "render.h"
#include "renderer.h"
#include "ui.h"

static void renderCharItem(GameContext *gameCtx, const SAVCharacter *character,
                           int invType, CharItemIndex itemIndex) {
  const GameObject *obj = &gameCtx->itemsInGame[character->items[itemIndex]];
  uint16_t frameId =
      GameContextGetItemSHPFrameIndex(gameCtx, obj->itemPropertyIndex);
  SHPFrame itemFrame = {0};
  SHPHandleGetFrame(&gameCtx->itemShapes, &itemFrame, frameId);
  SHPFrameGetImageData(&itemFrame);

  int x = 0;
  int y = 0;

  size_t bIndex = 4;
  switch (itemIndex) {
  case CharItemIndex_Hand:
    x = CHAR_INVENTORY_HAND1_X;
    y = CHAR_INVENTORY_HAND1_Y;
    break;
  case CharItemIndex_Shield:
    x = CHAR_INVENTORY_SHIELD_X;
    y = CHAR_INVENTORY_SHIELD_Y;
    break;
  case CharItemIndex_Hand2:
    x = CHAR_INVENTORY_HAND2_X;
    y = CHAR_INVENTORY_HAND2_Y;
    break;
  case CharItemIndex_Shield2:
    x = CHAR_INVENTORY_SHIELD2_X;
    y = CHAR_INVENTORY_SHIELD2_Y;
    break;
  case CharItemIndex_Helm:
    x = CHAR_INVENTORY_HELM_X;
    y = CHAR_INVENTORY_HELM_Y;
    break;
  case CharItemIndex_Armor:
    x = CHAR_INVENTORY_ARMOR_X;
    y = CHAR_INVENTORY_ARMOR_Y;
    break;
  case CharItemIndex_Necklace:
    x = CHAR_INVENTORY_NECKLACE_X;
    y = CHAR_INVENTORY_NECKLACE_Y;
    break;
  case CharItemIndex_Braces:
    x = CHAR_INVENTORY_BRACES_X;
    y = CHAR_INVENTORY_BRACES_Y;
    break;
  case CharItemIndex_Shoes:
    x = CHAR_INVENTORY_SHOES_X;
    y = CHAR_INVENTORY_SHOES_Y;
    break;
  case CharItemIndex_RingLeft:
    bIndex = 5;
    x = CHAR_INVENTORY_RING_LEFT_X;
    y = CHAR_INVENTORY_RING_LEFT_Y;
    break;
  case CharItemIndex_RingRight:
    bIndex = 5;
    x = CHAR_INVENTORY_RING_RIGHT_X;
    y = CHAR_INVENTORY_RING_RIGHT_Y;
    break;
  }

  SHPFrame bFrame = {0};

  SHPHandleGetFrame(&gameCtx->gameShapes, &bFrame, bIndex);
  SHPFrameGetImageData(&bFrame);
  drawSHPFrame(gameCtx->pixBuf, &bFrame, x, y, gameCtx->defaultPalette);
  SHPFrameRelease(&bFrame);
  drawSHPFrame(gameCtx->pixBuf, &itemFrame, x, y, gameCtx->defaultPalette);
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
