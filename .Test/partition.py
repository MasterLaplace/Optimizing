import pygame
import threading
import time

# Définition de la taille d'une cellule
CELL_SIZE = 256

class Partition:
    def __init__(self, grid_x, grid_y):
        self.grid_x = grid_x
        self.grid_y = grid_y
        self.rect = pygame.Rect(grid_x * CELL_SIZE, grid_y * CELL_SIZE, CELL_SIZE, CELL_SIZE)
        self.loaded = False
        # Vous pouvez charger ici les objets spécifiques à cette cellule
        self.objects = []  # Liste d'objets, sprites, etc.

    def load_data(self):
        # Simulation du chargement de ressources (par exemple, images, objets, etc.)
        time.sleep(1)  # Simulation d'un chargement qui prend du temps
        self.loaded = True
        print(f"Cellule {self.grid_x}, {self.grid_y} chargée.")


    def draw(self, screen, offset):
        if not self.loaded:
            return  # Ne pas dessiner si les données ne sont pas chargées
        # Dessiner le contour de la cellule (pour la démo)
        color = (0, 255, 0)
        rect_offset = self.rect.move(-offset[0], -offset[1])
        pygame.draw.rect(screen, color, rect_offset, 1)
        # Dessinez vos objets ici en fonction de offset
        for obj in self.objects:
            obj.draw(screen, offset)

class WorldPartition:
    def __init__(self):
        self.partitions = {}

    def load_partition(self, grid_x, grid_y):
        if (grid_x, grid_y) not in self.partitions:
            partition = Partition(grid_x, grid_y)
            self.partitions[(grid_x, grid_y)] = partition
            # Charger la cellule dans un thread séparé
            threading.Thread(target=partition.load_data, daemon=True).start()
            print(f"Lancement du chargement de la cellule {grid_x}, {grid_y}")
            # Vous pouvez initialiser les données spécifiques à la cellule ici
            # print(f"Chargement de la cellule {grid_x}, {grid_y}")

    def unload_partition(self, grid_x, grid_y):
        if (grid_x, grid_y) in self.partitions:
            del self.partitions[(grid_x, grid_y)]
            print(f"Déchargement de la cellule {grid_x}, {grid_y}")

    def update(self, player_rect):
        # Déterminez la cellule actuelle du joueur
        player_grid_x = player_rect.centerx // CELL_SIZE
        player_grid_y = player_rect.centery // CELL_SIZE

        # Charger une zone 3x3 autour du joueur
        for i in range(player_grid_x - 1, player_grid_x + 2):
            for j in range(player_grid_y - 1, player_grid_y + 2):
                self.load_partition(i, j)

        # Décharger les cellules éloignées (par exemple, celles hors de la zone 3x3)
        cells_to_unload = []
        for (gx, gy) in self.partitions:
            if abs(gx - player_grid_x) > 1 or abs(gy - player_grid_y) > 1:
                cells_to_unload.append((gx, gy))
        for cell in cells_to_unload:
            self.unload_partition(cell[0], cell[1])

    def draw(self, screen, offset):
        for partition in self.partitions.values():
            partition.draw(screen, offset)

# Exemple d'utilisation dans la boucle principale de pygame

def main():
    pygame.init()
    screen = pygame.display.set_mode((800, 600))
    clock = pygame.time.Clock()

    # Simuler un joueur avec un rectangle
    player_rect = pygame.Rect(400, 300, 50, 50)
    player_speed = 5

    # Initialisation du world partition
    world = WorldPartition()

    running = True
    while running:
        for event in pygame.event.get():
            if event.type == pygame.QUIT or (event.type == pygame.KEYDOWN and event.key == pygame.K_ESCAPE):
                running = False

        # Déplacement du joueur avec les touches fléchées
        keys = pygame.key.get_pressed()
        if keys[pygame.K_LEFT]:
            player_rect.x -= player_speed
        if keys[pygame.K_RIGHT]:
            player_rect.x += player_speed
        if keys[pygame.K_UP]:
            player_rect.y -= player_speed
        if keys[pygame.K_DOWN]:
            player_rect.y += player_speed

        # Mettre à jour le world partition selon la position du joueur
        world.update(player_rect)

        # Calculer un offset pour centrer la vue sur le joueur (simplifié)
        offset = (player_rect.centerx - 400, player_rect.centery - 300)

        screen.fill((0, 0, 0))
        world.draw(screen, offset)
        pygame.draw.rect(screen, (255, 0, 0), player_rect.move(-offset[0], -offset[1]))
        pygame.display.flip()
        clock.tick(60)

    pygame.quit()

if __name__ == '__main__':
    main()
