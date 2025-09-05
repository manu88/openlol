import pygame
import sys
from parse import parse
BLACK = (0, 0, 0)
WHITE = (200, 200, 200)
WINDOW_HEIGHT = 1000
WINDOW_WIDTH = 1000
unknown_idx = set()


def main():
    global SCREEN, CLOCK, unknown_idx
    pygame.init()
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
                pygame.quit()
                sys.exit()

        pygame.display.update()


def test_type(i):
    global unknown_idx
    if i in [0, 1, 2]:
        return
    if is_door(i):
        return
    unknown_idx.add(i)


def is_door(i):
    return i > 2 and i < 23


def draw_line(p0, p1, index):
    color = (255, 255, 255)
    test_type(index)
    if index in unknown_idx:
        color = (255, 0, 0)
    pygame.draw.line(SCREEN, color, p0, p1, 5 if is_door(index) else 1)

# https://moddingwiki.shikadi.net/wiki/Eye_of_the_Beholder_Maze_Format


def drawGrid(indices):
    blockSize = 20  # Set the size of the grid block

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
                if south:
                    draw_line((cellOrigin[0], cellOrigin[1] + cellSize),
                              (cellOrigin[0]+cellSize, cellOrigin[1]+cellSize), south)
                if west:
                    draw_line(
                        cellOrigin, (cellOrigin[0], cellOrigin[1]+cellSize), west)
                if east:
                    draw_line((cellOrigin[0]+cellSize, cellOrigin[1]),
                              (cellOrigin[0]+cellSize, cellOrigin[1]+cellSize), east)
            except IndexError:
                pass

    # rect = pygame.Rect(x*blockSize, y*blockSize, blockSize, blockSize)
    # pygame.draw.rect(SCREEN, WHITE, rect, 1)


main()
