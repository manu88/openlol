#include "game_char_inventory.h"
#include "formats/format_sav.h"
#include "game_ctx.h"
#include "game_rules.h"
#include "geometry.h"
#include "render.h"
#include "renderer.h"
#include "ui.h"

typedef enum {
  ItemType_Weapon = 0X05,
  ItemType_Helm = 0X10,
  ItemType_Shield = 0X0A,
  ItemType_Armor = 0X20,
  ItemType_Necklace = 0X40,
  ItemType_Bracers = 0X40,
  ItemType_Shoes = 0X100,
  ItemType_Ring = 0X600,
} ItemType;

typedef struct {
  Point coords;
  ItemType type;
} InventorySlot;

typedef struct {
  InventorySlot slot[11];
} InventoryLayout;

#define INVALID_ITEM {{-1, -1}, 0}

static InventoryLayout humanLayout = {{
    {{117, 25}, ItemType_Weapon},  // CharItemIndex_Hand
    {{192, 25}, ItemType_Shield},  // CharItemIndex_Shield
    INVALID_ITEM,                  // CharItemIndex_Hand2
    INVALID_ITEM,                  // CharItemIndex_Shield2
    {{117, 1}, ItemType_Helm},     // CharItemIndex_Helm
    {{117, 49}, ItemType_Armor},   // CharItemIndex_Armor
    {{192, 1}, ItemType_Necklace}, // CharItemIndex_Necklace
    {{192, 49}, ItemType_Bracers}, // CharItemIndex_Bracers
    {{117, 97}, ItemType_Shoes},   // CharItemIndex_Shoes
    {{191, 78}, ItemType_Ring},    // CharItemIndex_RingLeft
    {{126, 78}, ItemType_Ring},    // CharItemIndex_RingRight
}};

static InventoryLayout ulineLayout = {{
    {{233 / 2, 104 / 2}, ItemType_Weapon}, // CharItemIndex_Hand
    {{383 / 2, 103 / 2}, ItemType_Shield}, // CharItemIndex_Shield
    INVALID_ITEM,                          // CharItemIndex_Hand2
    INVALID_ITEM,                          // CharItemIndex_Shield2
    {{233 / 2, 8 / 2}, ItemType_Helm},     // CharItemIndex_Helm
    {{233 / 2, 56 / 2}, ItemType_Armor},   // CharItemIndex_Armor
    {{383 / 2, 8 / 2}, ItemType_Necklace}, // CharItemIndex_Necklace
    {{383 / 2, 56 / 2}, ItemType_Bracers}, // CharItemIndex_Bracers
    INVALID_ITEM,                          // CharItemIndex_Shoes
    {{253 / 2, 156 / 2}, ItemType_Ring},   // CharItemIndex_RingLeft
    {{383 / 2, 156 / 2}, ItemType_Ring},   // CharItemIndex_RingRight
}};

static InventoryLayout thomgogLayout = {{
    {{117, 25}, ItemType_Weapon},  // CharItemIndex_Hand
    {{192, 25}, ItemType_Shield},  // CharItemIndex_Shield
    {{117, 73}, ItemType_Weapon},  // CharItemIndex_Hand2
    {{192, 73}, ItemType_Shield},  // CharItemIndex_Shield2
    {{117, 1}, ItemType_Helm},     // CharItemIndex_Helm
    {{117, 49}, ItemType_Armor},   // CharItemIndex_Armor
    {{192, 1}, ItemType_Necklace}, // CharItemIndex_Necklace
    {{192, 49}, ItemType_Bracers}, // CharItemIndex_Bracers
    {{117, 97}, ItemType_Shoes},   // CharItemIndex_Shoes
    INVALID_ITEM,                  // CharItemIndex_RingLeft
    INVALID_ITEM,                  // CharItemIndex_RingRight
}};

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

  Point pt = layout->slot[itemIndex].coords;

  size_t bIndex = 4;
  if (itemIndex == CharItemIndex_RingLeft ||
      itemIndex == CharItemIndex_RingRight) {
    bIndex = 5;
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

static void selectFromCharItems(GameContext *gameCtx, SAVCharacter *character,
                                const InventorySlot *slot, int index) {
  uint16_t itemIndex = character->items[index];
  int updateCursor = 1;
  if (gameCtx->itemIndexInHand == 0 && itemIndex) {
    // getting item from slot
    character->items[index] = 0;
    gameCtx->itemIndexInHand = itemIndex;
  } else {
    // putting item in slot
    const GameObject *obj = &gameCtx->itemsInGame[gameCtx->itemIndexInHand];
    const ItemProperty *props =
        &gameCtx->itemProperties[obj->itemPropertyIndex];
    printf("type=%X slot type %X\n", props->type, slot->type);
    if (props->type == slot->type) {
      character->items[index] = gameCtx->itemIndexInHand;
      gameCtx->itemIndexInHand = itemIndex;
    } else {
      updateCursor = 0;
      GameContextGetString(gameCtx, 0X418A, gameCtx->dialogTextBuffer,
                           DIALOG_BUFFER_SIZE);
      gameCtx->dialogText = gameCtx->dialogTextBuffer;
      printf("'%s'\n", gameCtx->dialogText);
    }
  }
  if (updateCursor) {
    GameContextUpdateCursor(gameCtx);
  }
}

int processCharInventoryItemsMouse(GameContext *gameCtx) {
  printf("mouse %i %i\n", gameCtx->mouseEv.pos.x, gameCtx->mouseEv.pos.y);
  SAVCharacter *character = gameCtx->chars + gameCtx->selectedChar;
  int id = character->id < 0 ? -character->id : character->id;
  int invType = inventoryTypeForId[id];
  const InventoryLayout *layout = layouts[invType];
  for (int i = 0; i < 11; i++) {
    if (layout->slot[i].coords.x < 0 && layout->slot[i].coords.y < 0) {
      continue;
    }
    if (zoneClicked(&gameCtx->mouseEv.pos, layout->slot[i].coords.x,
                    layout->slot[i].coords.y, 20, 20)) {
      printf("Item Click %i\n", i);
      selectFromCharItems(gameCtx, character, &layout->slot[i], i);
      return 1;
    }
  }
  return 0;
}
