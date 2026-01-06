#include "game_char_inventory.h"
#include "formats/format_sav.h"
#include "game_ctx.h"
#include "game_render.h"
#include "game_rules.h"
#include "geometry.h"
#include "render.h"
#include "renderer.h"
#include "ui.h"
#include <stdint.h>

typedef enum {
  ItemType_Weapon = 0X05,
  ItemType_Helm = 0X10,
  ItemType_Shield = 0X0A,
  ItemType_Armor = 0X20,
  ItemType_Necklace = 0X40,
  ItemType_Bracers = 0X80,
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
    {{117, 52}, ItemType_Weapon},  // CharItemIndex_Hand
    {{192, 28}, ItemType_Shield},  // CharItemIndex_Shield
    INVALID_ITEM,                  // CharItemIndex_Hand2
    INVALID_ITEM,                  // CharItemIndex_Shield2
    {{117, 4}, ItemType_Helm},     // CharItemIndex_Helm
    {{117, 28}, ItemType_Armor},   // CharItemIndex_Armor
    {{192, 4}, ItemType_Necklace}, // CharItemIndex_Necklace
    {{192, 52}, ItemType_Bracers}, // CharItemIndex_Bracers
    {{117, 93}, ItemType_Shoes},   // CharItemIndex_Shoes
    {{192, 78}, ItemType_Ring},    // CharItemIndex_RingLeft
    {{127, 78}, ItemType_Ring},    // CharItemIndex_RingRight
}};

static InventoryLayout ulineLayout = {{
    {{117, 52}, ItemType_Weapon},  // CharItemIndex_Hand
    {{192, 52}, ItemType_Shield},  // CharItemIndex_Shield
    INVALID_ITEM,                  // CharItemIndex_Hand2
    INVALID_ITEM,                  // CharItemIndex_Shield2
    {{117, 4}, ItemType_Helm},     // CharItemIndex_Helm
    {{117, 28}, ItemType_Armor},   // CharItemIndex_Armor
    {{192, 4}, ItemType_Necklace}, // CharItemIndex_Necklace
    {{192, 28}, ItemType_Bracers}, // CharItemIndex_Bracers
    INVALID_ITEM,                  // CharItemIndex_Shoes
    {{127, 78}, ItemType_Ring},    // CharItemIndex_RingLeft
    {{192, 78}, ItemType_Ring},    // CharItemIndex_RingRight
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

  Point itemPt = layout->slot[itemIndex].coords;
  Point backgroundPt = itemPt;
  size_t bIndex = 4;
  if (itemIndex == CharItemIndex_RingLeft ||
      itemIndex == CharItemIndex_RingRight) {
    bIndex = 5;
    itemPt.x -= 4;
    itemPt.y -= 4;
  }

  if (itemPt.x < 0 && itemPt.y < 0) {
    return;
  }
  SHPFrame bFrame = {0};

  SHPHandleGetFrame(&gameCtx->gameShapes, &bFrame, bIndex);
  SHPFrameGetImageData(&bFrame);

  drawSHPFrame(gameCtx->pixBuf, &bFrame, backgroundPt.x, backgroundPt.y,
               gameCtx->defaultPalette);
  SHPFrameRelease(&bFrame);

  drawSHPFrame(gameCtx->pixBuf, &itemFrame, itemPt.x, itemPt.y,
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

static uint16_t getSlotDescStringID(ItemType type) {
  switch (type) {
  case ItemType_Weapon:
    return 0X4182;
  case ItemType_Shield:
    return 0X4183;
  case ItemType_Helm:
    return 0X4184;
  case ItemType_Armor:
    return 0X4185;
  case ItemType_Necklace:
    return 0X4186;
  case ItemType_Bracers:
    return 0X4187;
  case ItemType_Shoes:
    return 0X4188;
  case ItemType_Ring:
    return 0X4189;
  }
  assert(0);
}

static uint16_t getSlotNameStringID(ItemType type) {
  switch (type) {
  case ItemType_Weapon:
    return 0X417A;
  case ItemType_Shield:
    return 0X417B;
  case ItemType_Helm:
    return 0X417C;
  case ItemType_Armor:
    return 0X417D;
  case ItemType_Necklace:
    return 0X417E;
  case ItemType_Bracers:
    return 0X417F;
  case ItemType_Shoes:
    return 0X4180;
  case ItemType_Ring:
    return 0X4181;
  }
  assert(0);
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
    } else if (gameCtx->itemIndexInHand) {
      updateCursor = 0;
      if (props->type == 0) {
        GameContextGetString(gameCtx, 0X418C, gameCtx->dialogTextBuffer,
                             DIALOG_BUFFER_SIZE);
        gameCtx->dialogText = gameCtx->dialogTextBuffer;
      } else {
        char *itemName = GameContextGetString2(gameCtx, props->stringId);
        char *destName =
            GameContextGetString2(gameCtx, getSlotNameStringID(slot->type));
        GameRenderSetDialogF(gameCtx, 0X418A, itemName, destName);
        free(itemName);
        free(destName);
      }
    } else {
      GameRenderSetDialogF(gameCtx, getSlotDescStringID(slot->type));
    }
  }
  if (updateCursor) {
    GameContextUpdateCursor(gameCtx);
  }
}

int processCharInventoryItemsMouse(GameContext *gameCtx) {
  SAVCharacter *character = gameCtx->chars + gameCtx->selectedChar;
  int id = character->id < 0 ? -character->id : character->id;
  int invType = inventoryTypeForId[id];
  const InventoryLayout *layout = layouts[invType];
  for (int i = 0; i < 11; i++) {
    if (layout->slot[i].coords.x < 0 && layout->slot[i].coords.y < 0) {
      continue;
    }
    int width = 20;
    int height = 20;
    if (layout->slot[i].type == ItemType_Ring) {
      width = 12;
      height = 11;
    }
    if (zoneClicked(&gameCtx->mouseEv.pos, layout->slot[i].coords.x,
                    layout->slot[i].coords.y, width, height)) {
      selectFromCharItems(gameCtx, character, &layout->slot[i], i);
      return 1;
    }
  }
  return 0;
}
