import pygame
import sys
import pygame.freetype  # Import the freetype module.
from parse import parse

font = None
BLACK = (0, 0, 0)
WHITE = (200, 200, 200)
WINDOW_HEIGHT = 1000
WINDOW_WIDTH = 1000
unknown_idx = set()
max_idx = 0


def main():
    global SCREEN, CLOCK, unknown_idx, font
    pygame.init()
    font = pygame.freetype.SysFont(pygame.freetype.get_default_font(), 10)
    SCREEN = pygame.display.set_mode((WINDOW_WIDTH, WINDOW_HEIGHT))
    CLOCK = pygame.time.Clock()
    SCREEN.fill(BLACK)

    f = open('test.txt')
    lines = f.readlines()
    indices = []
    print(f"read {len(lines)} lines")
    for l in lines:
        r = parse("{} {} {} {}", l)
        if r:
            indices.append((int(r[0]), int(r[1]), int(r[2]), int(r[3])))
    print(f"got {len(indices)} indices")
    while True:
        drawGrid(indices)
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                print(unknown_idx)
                print(max_idx)
                pygame.quit()
                sys.exit()

        pygame.display.update()


def test_type(i):
    global unknown_idx
    global max_idx
    if i > max_idx:
        max_idx = i
    if i in [0, 1, 2]:
        return
    if is_door(i):
        return
    unknown_idx.add(i)


def is_door(i):
    return i > 2 and i < 23


def draw_line(p0, p1, index):
    color = (255, 255, 255)
    if is_door(index):
        color = (0, 255, 0)

    test_type(index)

    if index in unknown_idx:
        color = (255, 0, 0)
    pygame.draw.line(SCREEN, color, p0, p1, 5 if is_door(index) else 1)

# https://moddingwiki.shikadi.net/wiki/Eye_of_the_Beholder_Maze_Format


def drawGrid(indices):
    blockSize = 25  # Set the size of the grid block
    text_color = (0, 255,  0)
    for y in range(0, 32):
        for x in range(0, 32):
            try:
                i = y*32 + x
                index = indices[y*32 + x]
                # print(i, x, y)
                north = index[0]
                east = index[1]
                south = index[2]
                west = index[3]

                cellOrigin = (10+x*(blockSize+5), 10+y*(blockSize+5))
                cellSize = blockSize
                if north:
                    draw_line(
                        cellOrigin, (cellOrigin[0]+cellSize, cellOrigin[1]), north)
                    font.render_to(
                        SCREEN, (cellOrigin[0]+8, cellOrigin[1]+1), f"{north}", text_color)
                if south:
                    draw_line((cellOrigin[0], cellOrigin[1] + cellSize),
                              (cellOrigin[0]+cellSize, cellOrigin[1]+cellSize), south)
                    font.render_to(
                        SCREEN, (cellOrigin[0]+8, cellOrigin[1] + cellSize-7), f"{south}", text_color)
                if west:
                    draw_line(
                        cellOrigin, (cellOrigin[0], cellOrigin[1]+cellSize), west)
                    font.render_to(
                        SCREEN, (cellOrigin[0]+1, cellOrigin[1]+8), f"{west}", text_color)
                if east:
                    draw_line((cellOrigin[0]+cellSize, cellOrigin[1]),
                              (cellOrigin[0]+cellSize, cellOrigin[1]+cellSize), east)
                    font.render_to(
                        SCREEN, (cellOrigin[0]+cellSize-2, cellOrigin[1]+8), f"{east}", text_color)
            except IndexError:
                pass

    # rect = pygame.Rect(x*blockSize, y*blockSize, blockSize, blockSize)
    # pygame.draw.rect(SCREEN, WHITE, rect, 1)


main()
